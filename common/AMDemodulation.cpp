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

		/* Cut out a spectrum part of bandwidth "HILB_FILT_BNDWIDTH_5" */
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

		if ((eDemodType == DT_AM_10) || (eDemodType == DT_AM_5))
		{
			/* Use envelope of signal and DC filter. Reuse temp buffer
			   "rvecInpTmp" */
			rvecInpTmp = Filter(rvecBAM, rvecAAM, Abs(cvecHilbert), rvecZAM);

			/* Write in both output channels */
			for (i = 0, j = 0; i < 2 * iSymbolBlockSize; i += 2, j++)
				(*pvecOutputData)[i] = (*pvecOutputData)[i + 1] =
					Real2Sample(rvecInpTmp[j] * 2);
		}
		else
		{
			/* Make signal real and write mono signal in both channels (left and
			   right) */
			for (i = 0, j = 0; i < 2 * iSymbolBlockSize; i += 2, j++)
				(*pvecOutputData)[i] = (*pvecOutputData)[i + 1] =
					Real2Sample(Real(cvecHilbert[j]) * 4);
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
	/* Calculate filter taps for complex Hilbert filter --------------------- */
	CReal rFiltCentOffs;

	/* Adjust center of filter for respective demodulation types */
	switch (eDemodType)
	{
	case DT_AM_10:
	case DT_AM_5:
		/* No offset in normal AM mode */
		rFiltCentOffs = rNormCurFreqOffset;
		break;

	case DT_LSB:
		/* Shift filter to the left side of the carrier */
		rFiltCentOffs = rNormCurFreqOffset -
			(CReal) HILB_FILT_BNDWIDTH_5 / 2 / SOUNDCRD_SAMPLE_RATE;
		break;

	case DT_USB:
		/* Shift filter to the right side of the carrier */
		rFiltCentOffs = rNormCurFreqOffset +
			(CReal) HILB_FILT_BNDWIDTH_5 / 2 / SOUNDCRD_SAMPLE_RATE;
		break;
	}


	/* Set filter coefficients */
	if (eDemodType == DT_AM_10)
	{
		/* Use 10 kHz filter for normal AM demodulation */
		rvecBReal.Init(NUM_TAPS_HILB_FILT_10);
		rvecBImag.Init(NUM_TAPS_HILB_FILT_10);

		for (int i = 0; i < NUM_TAPS_HILB_FILT_10; i++)
		{
			rvecBReal[i] =
				fHilLPProt10[i] * Cos((CReal) 2.0 * crPi * rFiltCentOffs * i);

			rvecBImag[i] =
				fHilLPProt10[i] * Sin((CReal) 2.0 * crPi * rFiltCentOffs * i);
		}

		/* Init state vector for filtering with zeros */
		rvecZReal.Init(NUM_TAPS_HILB_FILT_10 - 1, (CReal) 0.0);
		rvecZImag.Init(NUM_TAPS_HILB_FILT_10 - 1, (CReal) 0.0);
	}
	else
	{
		/* 5 kHz filter for LSB and USB and narrow AM mode */
		rvecBReal.Init(NUM_TAPS_HILB_FILT_5);
		rvecBImag.Init(NUM_TAPS_HILB_FILT_5);

		for (int i = 0; i < NUM_TAPS_HILB_FILT_5; i++)
		{
			rvecBReal[i] =
				fHilLPProt5[i] * Cos((CReal) 2.0 * crPi * rFiltCentOffs * i);

			rvecBImag[i] =
				fHilLPProt5[i] * Sin((CReal) 2.0 * crPi * rFiltCentOffs * i);
		}

		/* Init state vector for filtering with zeros */
		rvecZReal.Init(NUM_TAPS_HILB_FILT_5 - 1, (CReal) 0.0);
		rvecZImag.Init(NUM_TAPS_HILB_FILT_5 - 1, (CReal) 0.0);
	}

	/* Only FIR filter */
	rvecA.Init(1, (CReal) 1.0);

	/* DC filter for AM demodulation */
	if ((eDemodType == DT_AM_10) || (eDemodType == DT_AM_5))
	{
		rvecZAM.Init(2, (CReal) 0.0);

		/* IIR filter: H(Z) = (1 - z^{-1}) / (1 - 0.99 * z^{-1}) */
		rvecBAM.Init(2);
		rvecAAM.Init(2);
		rvecBAM[0] = (CReal) 1.0;
		rvecBAM[1] = (CReal) -1.0;
		rvecAAM[0] = (CReal) 1.0;
		rvecAAM[1] = (CReal) -0.99;
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
