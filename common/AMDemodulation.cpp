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

			veccFFTOutput = rfft(vecrFFTInput, FftPlanAcq);

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
			FftFilt(cvecBReal, rvecInpTmp, rvecZReal, FftPlansHilFilt),
			FftFilt(cvecBImag, rvecInpTmp, rvecZImag, FftPlansHilFilt));

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
			/* Use envelope of signal and apply low-pass filter */
			rvecInpTmp = FftFilt(cvecBAMAfterDem, Abs(cvecHilbert),
				rvecZAMAfterDem, FftPlansHilFilt);

			/* Apply DC filter (high-pass filter) */
			rvecInpTmp = Filter(rvecBDC, rvecADC, rvecInpTmp, rvecZAM);
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


		/* Noise reduction -------------------------------------------------- */
		if (NoiRedType != NR_OFF)
			NoiseReduction.Process(rvecInpTmp);


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

	/* Init noise reducation object */
	NoiseReduction.Init(iSymbolBlockSize);


	/* Inits for Hilbert and DC filter -------------------------------------- */
	/* Hilbert filter block length is the same as input block length */
	iHilFiltBlLen = iSymbolBlockSize;

	/* Init state vector for filtering with zeros */
	rvecZReal.Init(iHilFiltBlLen, (CReal) 0.0);
	rvecZImag.Init(iHilFiltBlLen, (CReal) 0.0);
	rvecZAMAfterDem.Init(iHilFiltBlLen, (CReal) 0.0);

	/* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
	cvecBReal.Init(iHilFiltBlLen + 1);
	cvecBImag.Init(iHilFiltBlLen + 1);
	cvecBAMAfterDem.Init(iHilFiltBlLen + 1);

	/* FFT plans are initialized with the long length */
	FftPlansHilFilt.Init(iHilFiltBlLen * 2);

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
	FftPlanAcq.Init(iTotalBufferSize);

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
	int i;

	/* Calculate filter taps for complex Hilbert filter --------------------- */
	const CReal rBWNormCutOff = (CReal) iFilterBW / SOUNDCRD_SAMPLE_RATE;

	/* Actual filter design */
	CRealVector vecrFilter(iHilFiltBlLen);
	vecrFilter = FirLP(rBWNormCutOff, Nuttallwin(iHilFiltBlLen));

	/* Actual bandwidth up to the point of approx. -60 dB */
	rBandWidthNorm = (CReal) (iFilterBW + 200 /* Hz */) / SOUNDCRD_SAMPLE_RATE;


#if 0
/* Save filter coefficients */
static FILE* pFile = fopen("test/AMDemFilt.dat", "w");
for (int j = 0; j < iHilFiltBlLen; j++)
	fprintf(pFile, "%e ", vecrFilter[j]);
fprintf(pFile, "\n");
fflush(pFile);
// close all;load AMDemFilt.dat;[m,n]=size(AMDemFilt);for i=1:m;figure;freqz(AMDemFilt(i,:));end
#endif


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
	const CReal rStartPhase = (CReal) iHilFiltBlLen * crPi * rFiltCentOffsNorm;

	/* Copy actual filter coefficients. It is important to initialize the
	   vectors with zeros because we also do a zero-padding */
	CRealVector rvecBReal(2 * iHilFiltBlLen, (CReal) 0.0);
	CRealVector rvecBImag(2 * iHilFiltBlLen, (CReal) 0.0);
	for (i = 0; i < iHilFiltBlLen; i++)
	{
		rvecBReal[i] = vecrFilter[i] *
			Cos((CReal) 2.0 * crPi * rFiltCentOffsNorm * i - rStartPhase);

		rvecBImag[i] = vecrFilter[i] *
			Sin((CReal) 2.0 * crPi * rFiltCentOffsNorm * i - rStartPhase);
	}

	/* Transformation in frequency domain for fft filter */
	cvecBReal = rfft(rvecBReal, FftPlansHilFilt);
	cvecBImag = rfft(rvecBImag, FftPlansHilFilt);

	/* Set filter coefficients for AM filter after demodulation (use same low-
	   pass design as for the bandpass filter) */
	CRealVector rvecBAMAfterDem(2 * iHilFiltBlLen, (CReal) 0.0);
	for (i = 0; i < iHilFiltBlLen; i++)
		rvecBAMAfterDem[i] = vecrFilter[i];

	cvecBAMAfterDem = rfft(rvecBAMAfterDem, FftPlansHilFilt); 


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


/* Noise reduction implementation *********************************************/
/*
	The noise reduction algorithms is based on optimal filters, whereas the
	PDS of the noise is estimated with a minimum statistic.
	We use an overlap and add method to avoid clicks caused by fast changing
	optimal filters between successive blocks.

	[Ref] A. Engel: "Transformationsbasierte Systeme zur einkanaligen
		Stoerunterdrueckung bei Sprachsignalen", PhD Thesis, Christian-
		Albrechts-Universitaet zu Kiel, 1998
*/
void CNoiseReduction::Process(CRealVector& vecrIn)
{
	/* Regular block (updates the noise estimate) --------------------------- */
	/* Update history of input signal */
	vecrLongSignal.Merge(vecrOldSignal, vecrIn);

	/* Update signal PSD estimation */
	veccSigFreq = rfft(vecrLongSignal, FftPlan);
	vecrSqMagSigFreq = SqMag(veccSigFreq);

	/* Update minimum statistic for noise PSD estimation. This update is made
	   only once a regular (non-shited) block */
	UpdateNoiseEst(vecrNoisePSD, vecrSqMagSigFreq, eNoiRedDegree);

	/* Actual noise reducation filtering based on the noise PSD estimation and
	   the current squared magnitude of the input signal */
	vecrFiltResult = OptimalFilter(veccSigFreq, vecrSqMagSigFreq, vecrNoisePSD);

	/* Apply windowing */
	vecrFiltResult *= vecrTriangWin;

	/* Build output signal vector with old half and new half */
	vecrOutSig1.Merge(vecrOldOutSignal, vecrFiltResult(1, iHalfBlockLen));

	/* Save second half of output signal for next block (for overlap and add) */
	vecrOldOutSignal = vecrFiltResult(iHalfBlockLen + 1, iBlockLen);


	/* "Half-shifted" block for overlap and add ----------------------------- */
	/* Build input vector for filtering the "half-shifted" blocks. It is the
	   second half of the very old signal plus the complete old signal and the
	   first half of the current signal */
	vecrLongSignal.Merge(vecrVeryOldSignal(iHalfBlockLen + 1, iBlockLen),
		vecrOldSignal, vecrIn(1, iHalfBlockLen));

	/* Store old input signal blocks */
	vecrVeryOldSignal = vecrOldSignal;
	vecrOldSignal = vecrIn;

	/* Update signal PSD estimation for "half-shifted" block and calculate
	   optimal filter */
	veccSigFreq = rfft(vecrLongSignal, FftPlan);
	vecrSqMagSigFreq = SqMag(veccSigFreq);

	vecrFiltResult = OptimalFilter(veccSigFreq, vecrSqMagSigFreq, vecrNoisePSD);

	/* Apply windowing */
	vecrFiltResult *= vecrTriangWin;

	/* Overlap and add operation */
	vecrIn = vecrFiltResult + vecrOutSig1;
}

CRealVector CNoiseReduction::OptimalFilter(const CComplexVector& veccSigFreq,
										   const CRealVector& vecrSqMagSigFreq,
										   const CRealVector& vecrNoisePSD)
{
	CRealVector vecrReturn(iBlockLenLong);

	/* Calculate optimal filter coefficients in the frequency domain:
	   G_opt = max(1 - S_nn(n) / S_xx(n), 0) */
	veccOptFilt = Max(Zeros(iFreqBlLen), Ones(iFreqBlLen) -
		vecrNoisePSD / vecrSqMagSigFreq);

	/* Constrain the optimal filter in time domain to avoid aliasing */
	vecrOptFiltTime = rifft(veccOptFilt, FftPlan);
	vecrOptFiltTime.Merge(vecrOptFiltTime(1, iBlockLen), Zeros(iBlockLen));
	veccOptFilt = rfft(vecrOptFiltTime, FftPlan);

	/* Actual filtering in frequency domain */
	vecrReturn = rifft(veccSigFreq * veccOptFilt, FftPlan);

	/* Cut out correct samples (to get from cyclic convolution to linear
	   convolution) */
	return vecrReturn(iBlockLen + 1, iBlockLenLong);
}

void CNoiseReduction::UpdateNoiseEst(CRealVector& vecrNoisePSD,
									 const CRealVector& vecrSqMagSigFreq,
									 const ENoiRedDegree eNoiRedDegree)
{
/*
	Implements a mimium statistic proposed by R. Martin
*/
	/* Set weightning factor for minimum statistic */
	CReal rWeiFact;
	switch (eNoiRedDegree)
	{
	case NR_LOW:
		rWeiFact = MIN_STAT_WEIGTH_FACTOR_LOW;
		break;

	case NR_MEDIUM:
		rWeiFact = MIN_STAT_WEIGTH_FACTOR_MED;
		break;

	case NR_HIGH:
		rWeiFact = MIN_STAT_WEIGTH_FACTOR_HIGH;
		break;
	}

	/* Update signal PSD estimation (first order IIR filter) */
	IIR1(vecrSigPSD, vecrSqMagSigFreq, rLamPSD);

	for (int i = 0; i < iFreqBlLen; i++)
	{
// TODO: Update of minimum statistic can be done much more efficient
		/* Update history */
		matrMinimumStatHist[i].Merge(
			matrMinimumStatHist[i](2, iMinStatHistLen), vecrSigPSD[i]);

		/* Minimum values in history are taken for noise estimation */
		vecrNoisePSD[i] = Min(matrMinimumStatHist[i]) * rWeiFact;
	}
}

void CNoiseReduction::Init(const int iNewBlockLen)
{
	iBlockLen = iNewBlockLen;
	iHalfBlockLen = iBlockLen / 2;
	iBlockLenLong = 2 * iBlockLen;

	/* Block length of signal in frequency domain. "+ 1" because of the Nyquist
	   frequency */
	iFreqBlLen = iBlockLen + 1;

	/* Length of the minimum statistic history */
	iMinStatHistLen = (int) (MIN_STAT_HIST_LENGTH_SEC *
		(CReal) SOUNDCRD_SAMPLE_RATE / iBlockLen);

	/* Lambda for IIR filter */
	rLamPSD = IIR1Lam(TICONST_PSD_EST_SIG_NOISE_RED,
		(CReal) SOUNDCRD_SAMPLE_RATE / iBlockLen);

	/* Init vectors storing time series signals */
	vecrOldSignal.Init(iBlockLen, (CReal) 0.0);
	vecrVeryOldSignal.Init(iBlockLen, (CReal) 0.0);
	vecrFiltResult.Init(iBlockLen);
	vecrOutSig1.Init(iBlockLen);
	vecrLongSignal.Init(iBlockLenLong);
	vecrOptFiltTime.Init(iBlockLenLong);
	vecrOldOutSignal.Init(iHalfBlockLen, (CReal) 0.0);

	/* Init plans for FFT (faster processing of Fft and Ifft commands). FFT
	   plans are initialized with the long length */
	FftPlan.Init(iBlockLenLong);

	/* Init vectors storing data in frequency domain */
	veccOptFilt.Init(iFreqBlLen);

	/* Init signal and noise PDS estimation vectors */
	veccSigFreq.Init(iFreqBlLen);
	vecrSqMagSigFreq.Init(iFreqBlLen);
	vecrSigPSD.Init(iFreqBlLen, (CReal) 0.0);
	vecrNoisePSD.Init(iFreqBlLen, (CReal) 0.0);

	matrMinimumStatHist.Init(iFreqBlLen, iMinStatHistLen, (CReal) 0.0);

	/* Init window for overlap and add */
	vecrTriangWin.Init(iBlockLen);
	vecrTriangWin = Triang(iBlockLen);
}

void CAMDemodulation::SetNoiRedType(const ENoiRedType eNewType)
{
	NoiRedType = eNewType;

	switch (NoiRedType)
	{
		case NR_LOW:
			NoiseReduction.SetNoiRedDegree(CNoiseReduction::NR_LOW);
			break;

		case NR_MEDIUM:
			NoiseReduction.SetNoiRedDegree(CNoiseReduction::NR_MEDIUM);
			break;

		case NR_HIGH:
			NoiseReduction.SetNoiRedDegree(CNoiseReduction::NR_HIGH);
			break;
	}
}
