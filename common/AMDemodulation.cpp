/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implementation of an analog AM demodulation	
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "AMDemodulation.h"


/* Implementation *************************************************************/
void CAMDemodulation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int		i, j;
	int		iIndMaxPeak;
	CReal	rMaxPeak;
	CReal	rAttack;
	CReal	rDecay;
	CReal	rOutSignal;

	/* Acquisition ---------------------------------------------------------- */
	if (bAcquisition == TRUE)
	{
		/* Add new symbol in history (shift register) */
		vecrFFTHistory.AddEnd((*pvecInputData), iSymbolBlockSize);

		if (iAquisitionCounter > 0)
		{
			/* Decrease counter */
			iAquisitionCounter--;
		}
		else
		{
			/* Copy vector to matlib vector and calculate real-valued FFTW */
			for (i = 0; i < iTotalBufferSize; i++)
				vecrFFTInput[i] = vecrFFTHistory[i];

			veccFFTOutput = rfft(vecrFFTInput, FftPlan);

			/* Calculate power spectrum (X = real(F)^2 + imag(F)^2) and average
			   results */
			for (i = 1; i < iHalfBuffer; i++)
				vecrPSD[i] += SqMag(veccFFTOutput[i]);

			/* Calculate frequency from maximum peak in spectrum */
			rMaxPeak = (CReal) 0.0;
			iIndMaxPeak = iSearchWinStart;
			for (i = iSearchWinStart; i < iSearchWinEnd; i++)
			{
				if (vecrPSD[i] > rMaxPeak)
				{
					/* Remember maximum value and index of it */
					rMaxPeak = vecrPSD[i];
					iIndMaxPeak = i;
				}
			}

			/* Calculate relative frequency offset */
			rNormCurFreqOffset = (CReal) iIndMaxPeak / iHalfBuffer / 2;

			/* Generate filter taps and set mixing constant */
			SetCarrierFrequency(rNormCurFreqOffset);

			/* Set global parameter for GUI */
			ReceiverParam.rFreqOffsetAcqui = rNormCurFreqOffset;

			/* Reset acquisition flag */
			bAcquisition = FALSE;
		}
	}
	else
	{
		/* Check, if new demodulation type was chosen */
		if (bNewDemodType == TRUE)
		{
			/* Generate filter taps and set mixing constant */
			SetCarrierFrequency(rNormCurFreqOffset);

			/* Reset flag */
			bNewDemodType = FALSE;
		}


		/* AM demodulation -------------------------------------------------- */
		/* Copy CVector data in CMatlibVector */
		for (i = 0; i < iInputBlockSize; i++)
			rvecInpTmp[i] = (*pvecInputData)[i];

		/* Cut out a spectrum part of desired bandwidth */
		cvecHilbert = CComplexVector(
			Filter(rvecBReal, rvecA, rvecInpTmp, rvecZReal),
			Filter(rvecBImag, rvecA, rvecInpTmp, rvecZImag));

		/* Mix it down to zero frequency */
		for (i = 0; i < iInputBlockSize; i++)
		{
			cvecHilbert[i] *= cCurExp;

			/* Rotate exp-pointer on step further by complex multiplication with
			   precalculated rotation vector cExpStep. This saves us from
			   calling sin() and cos() functions all the time (iterative
			   calculation of these functions) */
			cCurExp *= cExpStep;
		}

		/* Actual demodulation. Reuse temp buffer "rvecInpTmp" for output
		   signal */
		switch (eDemodType)
		{
		case DT_AM:
			/* Use envelope of signal and DC filter */
			rvecInpTmp = Filter(rvecBDC, rvecADC, Abs(cvecHilbert), rvecZAM);
			break;

		case DT_LSB:
		case DT_USB:
			/* Make signal real and compensate for removed spectrum part */
			rvecInpTmp = Real(cvecHilbert) * (CReal) 2.0;
			break;

		case DT_FM:
			/* Get phase of complex signal and apply differentiation */
			for (i = 0; i < iInputBlockSize; i++)
			{
				/* Back-rotate new input sample by old value to get
				   differentiation operation, get angle of complex signal and
				   amplify result */
				rvecInpTmp[i] = Angle(cvecHilbert[i] * Conj(cOldVal)) *
					_MAXSHORT / ((CReal) 4.0 * crPi);

				/* Store old value */
				cOldVal = cvecHilbert[i];
			}

			/* Low-pass filter */
			rvecInpTmp = Filter(rvecBFM, rvecAFM, rvecInpTmp, rvecZFM);
		}


		/* AGC -------------------------------------------------------------- */
		/*          Slow     Medium   Fast    */
		/* Attack: 0.025 s, 0.015 s, 0.005 s  */
		/* Decay : 4.000 s, 2.000 s, 0.200 s  */
		switch (eAGCType)
		{
		case AT_SLOW:
			rAttack = IIR1Lam(0.025, SOUNDCRD_SAMPLE_RATE);
			rDecay = IIR1Lam(4.000, SOUNDCRD_SAMPLE_RATE);
			break;

		case AT_MEDIUM:
			rAttack = IIR1Lam(0.015, SOUNDCRD_SAMPLE_RATE);
			rDecay = IIR1Lam(2.000, SOUNDCRD_SAMPLE_RATE);
			break;

		case AT_FAST:
			rAttack = IIR1Lam(0.005, SOUNDCRD_SAMPLE_RATE);
			rDecay = IIR1Lam(0.200, SOUNDCRD_SAMPLE_RATE);
			break;
		}

		for (i = 0, j = 0; i < 2 * iSymbolBlockSize; i += 2, j++)
		{
			if (eAGCType == AT_NO_AGC)
			{
				/* No modification of the signal (except of an amplitude
				   correction factor) */
				rOutSignal = rvecInpTmp[j] * AM_AMPL_CORR_FACTOR;
			}
			else
			{
				/* Two sided one-pole recursion for average amplitude
				   estimation */
				IIR1TwoSided(rAvAmplEst, Abs(rvecInpTmp[j]), rAttack, rDecay);

				/* Lower bound for estimated average amplitude */
				if (rAvAmplEst < LOWER_BOUND_AMP_LEVEL)
					rAvAmplEst = LOWER_BOUND_AMP_LEVEL;

				/* Normalize to average amplitude and then amplify to the
				   desired level */
				rOutSignal = rvecInpTmp[j] * DES_AV_AMPL_AM_SIGNAL / rAvAmplEst;
			}

			/* Write mono signal in both channels (left and right) */
			(*pvecOutputData)[i] = (*pvecOutputData)[i + 1] =
				Real2Sample(rOutSignal);
		}
	}
}

void CAMDemodulation::InitInternal(CParameter& ReceiverParam)
{
	/* Get parameters from info class */
	iSymbolBlockSize = ReceiverParam.iSymbolBlockSize;

	/* Init temporary vector for filter input and output */
	rvecInpTmp.Init(iSymbolBlockSize);
	cvecHilbert.Init(iSymbolBlockSize);

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (CReal) 1.0;

	/* Init average amplitude estimation (needed for AGC) with desired
	   amplitude */
	rAvAmplEst = DES_AV_AMPL_AM_SIGNAL / AM_AMPL_CORR_FACTOR;

	/* Init old value needed for differentiation */
	cOldVal = (CReal) 0.0;


	/* Inits for Hilbert and DC filter -------------------------------------- */
	/* Init state vector for filtering with zeros */
	rvecZReal.Init(NUM_TAPS_AM_DEMOD_FILTER - 1, (CReal) 0.0);
	rvecZImag.Init(NUM_TAPS_AM_DEMOD_FILTER - 1, (CReal) 0.0);

	rvecBReal.Init(NUM_TAPS_AM_DEMOD_FILTER);
	rvecBImag.Init(NUM_TAPS_AM_DEMOD_FILTER);

	/* Only FIR filter */
	rvecA.Init(1, (CReal) 1.0);

	/* Init DC filter */
	/* IIR filter: H(Z) = (1 - z^{-1}) / (1 - 0.999 * z^{-1}) */
	rvecZAM.Init(2, (CReal) 0.0); /* Memory */
	rvecBDC.Init(2);
	rvecADC.Init(2);
	rvecBDC[0] = (CReal) 1.0;
	rvecBDC[1] = (CReal) -1.0;
	rvecADC[0] = (CReal) 1.0;
	rvecADC[1] = (CReal) -0.999;

	/* Init FM audio filter. This is a butterworth IIR filter with cut-off
	   of 3 kHz. It was generated in Matlab with
	   [b, a] = butter(4, 3000 / 24000); */
	rvecZFM.Init(5, (CReal) 0.0); /* Memory */
	rvecBFM.Init(5);
	rvecAFM.Init(5);
	rvecBFM[0] = (CReal) 0.00093349861295;
	rvecBFM[1] = (CReal) 0.00373399445182;
	rvecBFM[2] = (CReal) 0.00560099167773;
	rvecBFM[3] = (CReal) 0.00373399445182;
	rvecBFM[4] = (CReal) 0.00093349861295;
	rvecAFM[0] = (CReal) 1.0;
	rvecAFM[1] = (CReal) -2.97684433369673;
	rvecAFM[2] = (CReal) 3.42230952937764;
	rvecAFM[3] = (CReal) -1.78610660021804;
	rvecAFM[4] = (CReal) 0.35557738234441;


	/* Inits for acquisition ------------------------------------------------ */
	/* Total buffer size */
	iTotalBufferSize = NUM_BLOCKS_CARR_ACQUISITION * iSymbolBlockSize;

	/* Length of the half of the spectrum of real input signal (the other half
	   is the same because of the real input signal). We have to consider the
	   Nyquist frequency ("iTotalBufferSize" is always even!) */
	iHalfBuffer = iTotalBufferSize / 2 + 1;

	/* Allocate memory for FFT-histories and init with zeros */
	vecrFFTHistory.Init(iTotalBufferSize, (CReal) 0.0);
	vecrFFTInput.Init(iTotalBufferSize);
	veccFFTOutput.Init(iHalfBuffer);

	vecrPSD.Init(iHalfBuffer);

	/* Set flag for aquisition */
	bAcquisition = TRUE;
	iAquisitionCounter = NUM_BLOCKS_CARR_ACQUISITION;
	/* Reset FFT-history */
	vecrFFTHistory.Reset((CReal) 0.0);

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iTotalBufferSize);

	/* Search window indices for aquisition */
	if (bSearWinWasSet == TRUE)
	{
		const int iWinCenter = (int) (rNormCenter * iHalfBuffer);
		const int iHalfWidth = (int) (PERC_SEARCH_WIN_HALF_SIZE * iHalfBuffer);

		iSearchWinStart = iWinCenter - iHalfWidth;
		iSearchWinEnd = iWinCenter + iHalfWidth;

		/* Check the values that they are within the valid range */
		if (iSearchWinStart < 1)
			iSearchWinStart = 1;

		if (iSearchWinEnd > iHalfBuffer)
			iSearchWinEnd = iHalfBuffer;

		/* Reset flag */
		bSearWinWasSet = FALSE;
	}
	else
	{
		/* If there is no search window defined, use entire bandwidth */
		iSearchWinStart = 1;
		iSearchWinEnd = iHalfBuffer;
	}


	/* Define block-sizes for input and output */
	iMaxOutputBlockSize = (int) ((CReal) SOUNDCRD_SAMPLE_RATE *
		(CReal) 0.4 /* 400 ms */ * 2 /* stereo */) +
		2 /* stereo */ * iSymbolBlockSize;

	iInputBlockSize = iSymbolBlockSize;
	iOutputBlockSize = 2 * iSymbolBlockSize;
}

void CAMDemodulation::SetCarrierFrequency(const CReal rNormCurFreqOffset)
{
	CVector<_REAL> vecrFilter;

	/* Calculate filter taps for complex Hilbert filter --------------------- */
	GetBWFilter(iFilterBW, rBandWidthNorm, vecrFilter);

	/* Adjust center of filter for respective demodulation types */
	switch (eDemodType)
	{
	case DT_AM:
	case DT_FM:
		/* No offset */
		rFiltCentOffsNorm = rNormCurFreqOffset;
		break;

	case DT_LSB:
		/* Shift filter to the left side of the carrier */
		rFiltCentOffsNorm = rNormCurFreqOffset - rBandWidthNorm / 2;
		break;

	case DT_USB:
		/* Shift filter to the right side of the carrier */
		rFiltCentOffsNorm = rNormCurFreqOffset + rBandWidthNorm / 2;
		break;
	}


	/* Set filter coefficients ---------------------------------------------- */
	/* Make sure that the phase in the middle of the filter is always the same
	   to avaoid clicks when the filter coefficients are changed */
	const CReal rStartPhase = Ceil((CReal) NUM_TAPS_AM_DEMOD_FILTER / 2) *
		(CReal) 2.0 * crPi * rFiltCentOffsNorm;

	for (int i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
	{
		rvecBReal[i] = vecrFilter[i] *
			Cos((CReal) 2.0 * crPi * rFiltCentOffsNorm * i - rStartPhase);

		rvecBImag[i] = vecrFilter[i] *
			Sin((CReal) 2.0 * crPi * rFiltCentOffsNorm * i - rStartPhase);
	}


	/* Set mixing constant -------------------------------------------------- */
	cExpStep = CComplex(cos((CReal) 2.0 * crPi * rNormCurFreqOffset),
		-sin((CReal) 2.0 * crPi * rNormCurFreqOffset));
}

void CAMDemodulation::SetAcqFreq(const CReal rNewNormCenter)
{
	/* Define search window for center frequency (used when aquisistion is
	   active) */
	rNormCenter = rNewNormCenter;

	/* Set the flag so that the parameters are not overwritten in the init
	   function */
	bSearWinWasSet = TRUE;
}

void CAMDemodulation::GetBWFilter(const int iFiltBW, CReal& rFreq,
								  CVector<CReal>& vecrFilter)
{
	int i, j;

	/* Init vector for storing the filter coefficients */
	vecrFilter.Init(NUM_TAPS_AM_DEMOD_FILTER);

	/* Search for filter which fits best */
	/* Init for minimum filter bandwidth */
	_BOOLEAN bFiltFound = FALSE;

	if (iFiltBW < iHilLPProtAMDemodBW[0])
	{
		/* Minimum bandwidth is the lowest bandwidth of prototype filters */
		rFreq = (CReal) iHilLPProtAMDemodBW[0];

		/* Copy filter coefficients directly */
		for (i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
			vecrFilter[i] = fHilLPProtAMDemod[0][i];
	}
	else
	{
		for (int i = 1; i < NUM_AM_DEMOD_FILTER; i++)
		{
			if (bFiltFound == FALSE)
			{
				int iHilFilIn;

				if (iFiltBW < iHilLPProtAMDemodBW[i])
				{
					/* Correct intervall found, use resampling routine to
					   get a filter in between the prototype filters */
					iHilFilIn = i - 1;
					bFiltFound = TRUE;
				}

				if ((i == NUM_AM_DEMOD_FILTER - 1) &&
					(iFiltBW >= iHilLPProtAMDemodBW[NUM_AM_DEMOD_FILTER - 1]))
				{
					/* Selected bandwidth is larger than the largest prototype
					   filter */
					iHilFilIn = NUM_AM_DEMOD_FILTER - 1;
					bFiltFound = TRUE;
				}

				if (bFiltFound == TRUE)
				{
					/* Calculate ratio for resampler */
					const CReal rResRatio =
						(CReal) iHilLPProtAMDemodBW[iHilFilIn] / iFiltBW;

					vecrFilter = ResampleFilterCoeff(
						fHilLPProtAMDemod[iHilFilIn], rResRatio);

					rFreq = (CReal) iFiltBW;
				}
			}
		}

	}

	/* Return normalized frequency */
	rFreq /= SOUNDCRD_SAMPLE_RATE;
}

CVector<CReal> CAMDemodulation::ResampleFilterCoeff(const float* pfFilt,
													CReal rRatio)
{
	int				i;
	CVector<CReal>	vecrReturn;
	CResample		ResampleObj;

	/* This resampling only works for ratios smaller than 1 */
	if (rRatio > (CReal) 1.0)
		rRatio = (CReal) 1.0;

	/* Calculate delay of resample filters */
	const int iResampleDelay = RES_FILT_NUM_TAPS_PER_PHASE / 2 + 1;

	/* Calculate length for input vector so that the length after resampling
	   is the same as the filter was before -> zero padding */
	const int iResLen =
		iResampleDelay + (int) Ceil((CReal) NUM_TAPS_AM_DEMOD_FILTER / rRatio);

	/* Calculate offset to put input filter in the middle */
	const CReal rInpFiltOffs = ((CReal) NUM_TAPS_AM_DEMOD_FILTER / rRatio -
		NUM_TAPS_AM_DEMOD_FILTER) / 2;

	/* We need an index -> use next integer */
	const int iInpFiltOffsInt =	(int) Ceil(rInpFiltOffs);

	/* Compensate for the rounding at the initialization of the resampler */
	const CReal rResOutInitOffs =
		(iInpFiltOffsInt - rInpFiltOffs) * INTERP_DECIM_I_D / rRatio;

	ResampleObj.Init(iResLen,
		iResampleDelay * INTERP_DECIM_I_D + rResOutInitOffs);

	/* Init intermediate vectors (it is important to initialize with zeros) */
	CVector<CReal> vecInp(iResLen, (CReal) 0.0);
	CVector<CReal> vecOut(iResLen, (CReal) 0.0);

	for (i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
		vecInp[i + iInpFiltOffsInt] = pfFilt[i];

	/* Actual resampling */
	ResampleObj.Resample(&vecInp, &vecOut, rRatio);

	/* Copy new filter in output vector */
	vecrReturn.Init(NUM_TAPS_AM_DEMOD_FILTER);
	for (i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
		vecrReturn[i] = vecOut[i];

#if 0
/* Save filter coefficients */
static FILE* pFile = fopen("test/AMDemFilt.dat", "w");
for (i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
	fprintf(pFile, "%e ", vecrReturn[i]);
fprintf(pFile, "\n");
fflush(pFile);
// close all;load AMDemFilt.dat;[m,n]=size(AMDemFilt);for i=1:m;figure;freqz(AMDemFilt(i,:));end
#endif

	return vecrReturn;
}
