/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	OFDM modulation;
 *	OFDM demodulation, SNR estimation, PSD estimation
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

#include "OFDM.h"


/* Implementation *************************************************************/
/******************************************************************************\
* OFDM-modulation                                                              *
\******************************************************************************/
void COFDMModulation::ProcessDataInternal(CParameter& TransmParam)
{
	int			i;
	const int	iEndInd = iShiftedKmax + 1;

	/* Copy input vector in matlib vector and place bins at the correct
	   position */
	for (i = iShiftedKmin; i < iEndInd; i++)
		veccFFTInput[i] = (*pvecInputData)[i - iShiftedKmin];

	/* Calculate inverse fast Fourier transformation */
	veccFFTOutput = Ifft(veccFFTInput, FftPlan);

	/* Copy complex FFT output in output buffer and scale */
	for (i = 0; i < iDFTSize; i++)
		(*pvecOutputData)[i + iGuardSize] = veccFFTOutput[i] * (CReal) iDFTSize;

	/* Copy data from the end to the guard-interval (Add guard-interval) */
	for (i = 0; i < iGuardSize; i++)
		(*pvecOutputData)[i] = (*pvecOutputData)[iDFTSize + i];
}

void COFDMModulation::InitInternal(CParameter& TransmParam)
{
	/* Get global parameters */
	iDFTSize = TransmParam.iFFTSizeN;
	iGuardSize = TransmParam.iGuardSize;
	iShiftedKmin = TransmParam.iShiftedKmin;
	iShiftedKmax = TransmParam.iShiftedKmax;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iDFTSize);

	/* Allocate memory for intermediate result of fft. Zero out input vector
	   (because only a few bins are used, the rest has to be zero) */
	veccFFTInput.Init(iDFTSize, (CReal) 0.0);
	veccFFTOutput.Init(iDFTSize);

	/* Define block-sizes for input and output */
	iInputBlockSize = TransmParam.iNumCarrier;
	iOutputBlockSize = TransmParam.iSymbolBlockSize;
}


/******************************************************************************\
* OFDM-demodulation                                                            *
\******************************************************************************/
void COFDMDemodulation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i;
	int			iNumAve;
	_REAL		rNormCurFreqOffset;
	_REAL		rSkipGuardIntPhase;
	_REAL		rCurPower;
	_REAL		rPowLeftVCar, rPowRightVCar, rAvNoisePow, rUsNoPower;
	_COMPLEX	cExpStep;

	/* Total frequency offset from acquisition and tracking (we calculate the
	   normalized frequency offset) */
	rNormCurFreqOffset = (_REAL) 2.0 * crPi * (ReceiverParam.rFreqOffsetAcqui +
		ReceiverParam.rFreqOffsetTrack - rInternIFNorm);

	/* New rotation vector for exp() calculation */
	cExpStep = _COMPLEX(cos(rNormCurFreqOffset), sin(rNormCurFreqOffset));

	/* To get a continuous counter we need to take the guard-interval and
	   timing corrections into account */
	rSkipGuardIntPhase = rNormCurFreqOffset *
		(iGuardSize - (*pvecInputData).GetExData().iCurTimeCorr);

	/* Apply correction */
	cCurExp *= _COMPLEX(cos(rSkipGuardIntPhase), sin(rSkipGuardIntPhase));

	/* Input data is real, make complex and compensate for frequency offset */
	for (i = 0; i < iInputBlockSize; i++)
	{
		veccFFTInput[i] = (*pvecInputData)[i] * Conj(cCurExp);
		
		/* Rotate exp-pointer on step further by complex multiplication with
		   precalculated rotation vector cExpStep. This saves us from
		   calling sin() and cos() functions all the time (iterative
		   calculation of these functions) */
		cCurExp *= cExpStep;
	}

	/* Calculate Fourier transformation (actual OFDM demodulation) */
	veccFFTOutput = Fft(veccFFTInput, FftPlan);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData)[i - iShiftedKmin] =
			veccFFTOutput[i] / (CReal) iDFTSize;


	/* SNR estimation. Use virtual carriers at the edges of the spectrum ---- */
	/* Power of useful part plus noise */
	rUsNoPower = (_REAL) 0.0;
	iNumAve = 0;
	for (i = 0; i < iNumCarrier; i++)
	{
		/* Use all carriers except of DC carriers */
		if (!_IsDC(ReceiverParam.matiMapTab[0][i]))
		{
			rUsNoPower += SqMag((*pvecOutputData)[i]);

			iNumAve++;
		}
	}

	/* Normalize and average result */
	IIR1(rUsefPowAv, rUsNoPower / iNumAve, rLamSNREst);

	/* Estimate power of noise */
	/* First, check if virtual carriers are in range */
	if ((iShiftedKmin > 1) && (iShiftedKmax + 1 < iDFTSize))
	{
		/* Get current powers */
		rPowLeftVCar = SqMag(veccFFTOutput[iShiftedKmin - 1]);
		rPowRightVCar = SqMag(veccFFTOutput[iShiftedKmin + 1]);

		/* Average results */
		IIR1(rNoisePowAvLeft, rPowLeftVCar, rLamSNREst);
		IIR1(rNoisePowAvRight, rPowRightVCar, rLamSNREst);

		/* Take the smallest value of both to avoid getting sinusoid 
		   interferer in the measurement */
		rAvNoisePow = Min(rNoisePowAvLeft, rNoisePowAvRight);
	}

	/* Calculate SNR estimate (\hat{s} - \hat{n}) / \hat{n}. The result
	   must be multiplicated with squared "iDFTSize" because of the
	   FFT operation */
	rSNREstimate = (rUsefPowAv / rAvNoisePow * iDFTSize * iDFTSize - 1) / 2;


	/* Save averaged spectrum for plotting ---------------------------------- */
	for (i = 0; i < iLenPowSpec; i++)
	{
		/* Power of this tap */
		rCurPower = SqMag(veccFFTOutput[i]);

		/* Averaging (first order IIR filter) */
		IIR1(vecrPowSpec[i], rCurPower, rLamPSD);
	}
}

void COFDMDemodulation::InitInternal(CParameter& ReceiverParam)
{
	iDFTSize = ReceiverParam.iFFTSizeN;
	iGuardSize = ReceiverParam.iGuardSize;
	iNumCarrier = ReceiverParam.iNumCarrier;
	iShiftedKmin = ReceiverParam.iShiftedKmin;
	iShiftedKmax = ReceiverParam.iShiftedKmax;

	/* Calculate the desired frequency position (normalized) */
	rInternIFNorm = (_REAL) ReceiverParam.iIndexDCFreq / iDFTSize;

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Initialize useful and noise power estimation */
	rNoisePowAvLeft = (_REAL) 0.0;
	rNoisePowAvRight = (_REAL) 0.0;
	rUsefPowAv = (_REAL) 0.0;
	rLamSNREst = IIR1Lam(TICONST_SIGNOIEST_OFDM, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize); /* Lambda for IIR filter */

	/* Init SNR estimate */
	rSNREstimate = (_REAL) 0.0;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iDFTSize);

	/* Allocate memory for intermediate result of fftw */
	veccFFTInput.Init(iDFTSize);
	veccFFTOutput.Init(iDFTSize);

	/* Vector for power density spectrum of input signal */
	iLenPowSpec = iDFTSize / 2;
	vecrPowSpec.Init(iLenPowSpec, (_REAL) 0.0);
	rLamPSD = IIR1Lam(TICONST_PSD_EST_OFDM, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize); /* Lambda for IIR filter */

	/* Define block-sizes for input and output */
	iInputBlockSize = iDFTSize;
	iOutputBlockSize = iNumCarrier;
}

void COFDMDemodulation::GetPowDenSpec(CVector<_REAL>& vecrData,
									  CVector<_REAL>& vecrScale)
{
	/* Init output vectors */
	vecrData.Init(iLenPowSpec, (_REAL) 0.0);
	vecrScale.Init(iLenPowSpec, (_REAL) 0.0);

	/* Do copying of data only if vector is of non-zero length which means that
	   the module was already initialized */
	if (iLenPowSpec != 0)
	{
		/* Lock resources */
		Lock();

		_REAL rNormData = (_REAL) iDFTSize * iDFTSize * _MAXSHORT;
		_REAL rFactorScale = 
			(_REAL) SOUNDCRD_SAMPLE_RATE / iLenPowSpec / 2000;

		/* Apply the normalization (due to the FFT) */
		for (int i = 0; i < iLenPowSpec; i++)
		{
			_REAL rNormPowSpec = vecrPowSpec[i] / rNormData;

			if (rNormPowSpec > 0)
				vecrData[i] = (_REAL) 10.0 * log10(vecrPowSpec[i] / rNormData);
			else
				vecrData[i] = RET_VAL_LOG_0;

			vecrScale[i] = (_REAL) i * rFactorScale;
		}

		/* Release resources */
		Unlock();
	}
}

_REAL COFDMDemodulation::GetSNREstdB() const
{
	/* Bound the SNR at 0 dB */
	if (rSNREstimate > (_REAL) 1.0)
		return 10 * log10(rSNREstimate);
	else
		return (_REAL) 0.0;
}


/******************************************************************************\
* OFDM-demodulation only for simulation purposes, with guard interval removal  *
\******************************************************************************/
void COFDMDemodSimulation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i, j;
	int iEndPointGuardRemov;

	/* Guard-interval removal parameter */
	iEndPointGuardRemov = iSymbolBlockSize - iGuardSize + iStartPointGuardRemov;


	/* Regular signal *********************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval */
	for (i = iStartPointGuardRemov; i < iEndPointGuardRemov; i++)
		veccFFTInput[i - iStartPointGuardRemov] = (*pvecInputData)[i].tOut;

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	veccFFTOutput = Fft(veccFFTInput, FftPlan);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData2)[i - iShiftedKmin].tOut =
			veccFFTOutput[i] / (CReal) iDFTSize;

	/* We need the same information for channel estimation evaluation, too */
	for (i = 0; i < iNumCarrier; i++)
		(*pvecOutputData)[i] = (*pvecOutputData2)[i].tOut;


	/* Channel-in signal ******************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval
	   We have to cut out the FFT window at the correct position, because the
	   channel estimation has information only about the original pilots
	   which are not phase shifted due to a timing-offset. To be able to
	   compare reference signal and channel estimation output we have to use
	   the synchronous reference signal for input */
	for (i = iGuardSize; i < iSymbolBlockSize; i++)
		veccFFTInput[i - iGuardSize] = (*pvecInputData)[i].tIn;

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	veccFFTOutput = Fft(veccFFTInput, FftPlan);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData2)[i - iShiftedKmin].tIn =
			veccFFTOutput[i] / (CReal) iDFTSize;


	/* Reference signal *******************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval */
	for (i = iStartPointGuardRemov; i < iEndPointGuardRemov; i++)
		veccFFTInput[i - iStartPointGuardRemov] = (*pvecInputData)[i].tRef;

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	veccFFTOutput = Fft(veccFFTInput, FftPlan);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData2)[i - iShiftedKmin].tRef =
			veccFFTOutput[i] / (CReal) iDFTSize;


	/* Channel tap gains ******************************************************/
	/* Loop over all taps */
	for (j = 0; j < iNumTapsChan; j++)
	{
		/* Convert input vector in fft-vector type and cut out the
		   guard-interval */
		for (i = iStartPointGuardRemov; i < iEndPointGuardRemov; i++)
			veccFFTInput[i - iStartPointGuardRemov] =
				(*pvecInputData)[i].veccTap[j];

		/* Actual OFDM demodulation */
		/* Calculate Fourier transformation (actual OFDM demodulation) */
		veccFFTOutput = Fft(veccFFTInput, FftPlan);

		/* Use only useful carriers and normalize with the block-size ("N") */
		for (i = 0; i < iNumCarrier; i++)
			(*pvecOutputData2)[i].veccTap[j] =
				veccFFTOutput[i] / (CReal) iDFTSize;

		/* Store the end of the vector, too */
		for (i = 0; i < iNumCarrier; i++)
			(*pvecOutputData2)[i].veccTapBackw[j] =
				veccFFTOutput[iDFTSize - i - 1] / (CReal) iDFTSize;
	}


	/* Take care of symbol IDs ---------------------------------------------- */
	iSymbolCounterTiSy++;
	if (iSymbolCounterTiSy == iNumSymPerFrame)
		iSymbolCounterTiSy = 0;

	/* Set current symbol ID in extended data of output vector */
	(*pvecOutputData).GetExData().iSymbolID = iSymbolCounterTiSy;
	(*pvecOutputData2).GetExData().iSymbolID = iSymbolCounterTiSy;

	/* No timing corrections, timing is constant in this case */
	(*pvecOutputData).GetExData().iCurTimeCorr = 0;
	(*pvecOutputData2).GetExData().iCurTimeCorr = 0;
}

void COFDMDemodSimulation::InitInternal(CParameter& ReceiverParam)
{
	/* Set internal parameters */
	iDFTSize = ReceiverParam.iFFTSizeN;
	iGuardSize = ReceiverParam.iGuardSize;
	iNumCarrier = ReceiverParam.iNumCarrier;
	iShiftedKmin = ReceiverParam.iShiftedKmin;
	iShiftedKmax = ReceiverParam.iShiftedKmax;
	iSymbolBlockSize = ReceiverParam.iSymbolBlockSize;
	iNumSymPerFrame = ReceiverParam.iNumSymPerFrame;

	iNumTapsChan = ReceiverParam.iNumTaps;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iDFTSize);

	/* Allocate memory for intermediate result of fftw, init input signal with
	   zeros because imaginary part is not written */
	veccFFTInput.Init(iDFTSize, (CReal) 0.0);
	veccFFTOutput.Init(iDFTSize);

	/* Init internal counter for symbol number. Set it to this value to get
	   a "0" for the first time */
	iSymbolCounterTiSy = iNumSymPerFrame - 1;

	/* Set guard-interval removal start index. Adapt this parameter to the
	   channel which was chosen. Place the delay spread centered in the
	   middle of the guard-interval */
	iStartPointGuardRemov =
		(iGuardSize + ReceiverParam.iPathDelay[iNumTapsChan - 1]) / 2;

	/* Check the case if impulse response is longer than guard-interval */
	if (iStartPointGuardRemov > iGuardSize)
		iStartPointGuardRemov = iGuardSize;

	/* Set start point of useful part extraction in global struct */
	ReceiverParam.iOffUsfExtr = iGuardSize - iStartPointGuardRemov;

	/* Define block-sizes for input and output */
	iInputBlockSize = iSymbolBlockSize;
	iOutputBlockSize = iNumCarrier;
	iOutputBlockSize2 = iNumCarrier;

	/* We need to store as many symbols in output buffer as long the channel
	   estimation delay is */
	iMaxOutputBlockSize2 = iNumCarrier * ReceiverParam.iChanEstDelay;
}
