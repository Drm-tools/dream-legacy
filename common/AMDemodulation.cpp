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
		if (eDemodType == DT_AM)
		{
			/* Use envelope of signal and DC filter */
			rvecInpTmp = Filter(rvecBAM, rvecAAM, Abs(cvecHilbert), rvecZAM);
		}
		else
		{
			/* Make signal real and compensate for removed spectrum part */
			rvecInpTmp = Real(cvecHilbert) * (CReal) 2.0;
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
	float*	pHilFilter;
	CReal	rBandWidth;

	GetBWFilter(eFilterBW, rBandWidthNorm, &pHilFilter);

	/* Calculate filter taps for complex Hilbert filter --------------------- */
	/* Adjust center of filter for respective demodulation types */
	switch (eDemodType)
	{
	case DT_AM:
		/* No offset in normal AM mode */
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
	rvecBReal.Init(NUM_TAPS_AM_DEMOD_FILTER);
	rvecBImag.Init(NUM_TAPS_AM_DEMOD_FILTER);

	for (int i = 0; i < NUM_TAPS_AM_DEMOD_FILTER; i++)
	{
		rvecBReal[i] =
			pHilFilter[i] * Cos((CReal) 2.0 * crPi * rFiltCentOffsNorm * i);

		rvecBImag[i] =
			pHilFilter[i] * Sin((CReal) 2.0 * crPi * rFiltCentOffsNorm * i);
	}

	/* Init state vector for filtering with zeros */
	rvecZReal.Init(NUM_TAPS_AM_DEMOD_FILTER - 1, (CReal) 0.0);
	rvecZImag.Init(NUM_TAPS_AM_DEMOD_FILTER - 1, (CReal) 0.0);

	/* Only FIR filter */
	rvecA.Init(1, (CReal) 1.0);

	/* DC filter for AM demodulation */
	if (eDemodType == DT_AM)
	{
		rvecZAM.Init(2, (CReal) 0.0);

		/* IIR filter: H(Z) = (1 - z^{-1}) / (1 - 0.999 * z^{-1}) */
		rvecBAM.Init(2);
		rvecAAM.Init(2);
		rvecBAM[0] = (CReal) 1.0;
		rvecBAM[1] = (CReal) -1.0;
		rvecAAM[0] = (CReal) 1.0;
		rvecAAM[1] = (CReal) -0.999;
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

void CAMDemodulation::GetBWFilter(const EFilterBW eFiltBW, CReal& rFreq,
								  float** ppfFilter)
{
	switch (eFiltBW)
	{
	case BW_1KHZ:
		*ppfFilter = fHilLPProtAMDemod1;
		rFreq = (CReal) 1000.0; /* Hz */
		break;

	case BW_2KHZ:
		rFreq = (CReal) 2000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod2;
		break;

	case BW_3KHZ:
		rFreq = (CReal) 3000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod3;
		break;

	case BW_4KHZ:
		rFreq = (CReal) 4000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod4;
		break;

	case BW_5KHZ:
		rFreq = (CReal) 5000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod5;
		break;

	case BW_6KHZ:
		rFreq = (CReal) 6000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod6;
		break;

	case BW_7KHZ:
		rFreq = (CReal) 7000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod7;
		break;

	case BW_8KHZ:
		rFreq = (CReal) 8000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod8;
		break;

	case BW_9KHZ:
		rFreq = (CReal) 9000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod9;
		break;

	case BW_10KHZ:
		rFreq = (CReal) 10000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod10;
		break;

	case BW_11KHZ:
		rFreq = (CReal) 11000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod11;
		break;

	case BW_12KHZ:
		rFreq = (CReal) 12000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod12;
		break;

	case BW_13KHZ:
		rFreq = (CReal) 13000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod13;
		break;

	case BW_14KHZ:
		rFreq = (CReal) 14000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod14;
		break;

	case BW_15KHZ:
		rFreq = (CReal) 15000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod15;
		break;

	default:
		rFreq = (CReal) 10000.0; /* Hz */
		*ppfFilter = fHilLPProtAMDemod10;
		break;
	}

	/* Return normalized frequency */
	rFreq = (rFreq + HILB_FILT_BNDWIDTH_ADD) / SOUNDCRD_SAMPLE_RATE;
}
