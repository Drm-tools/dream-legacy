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
	int i;

	/* Convert our _COMPLEX format in fftw-complex format and place bins
	   at the right position*/
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
	{
		veccFFTWInput[i].re = (*pvecInputData)[i - iShiftedKmin].real();
		veccFFTWInput[i].im = (*pvecInputData)[i - iShiftedKmin].imag();
	}

	/* Calculate Fourier transformation */
	fftw_one(FFTWPlan, &veccFFTWInput[0], &veccFFTWOutput[0]);

	/* Copy complex FFT output in output buffer */
	for (i = 0; i < iDFTSize; i++)
		(*pvecOutputData)[i + iGuardSize] = 
			_COMPLEX(veccFFTWOutput[i].re, veccFFTWOutput[i].im);

	/* Copy data from the end to the guard-interval (Add guard-interval) */
	for (i = 0; i < iGuardSize; i++)
		(*pvecOutputData)[i] = (*pvecOutputData)[iDFTSize + i];
}

void COFDMModulation::InitInternal(CParameter& TransmParam)
{
	iDFTSize = TransmParam.iFFTSizeN;

	/* Length of guard-interval measured in "time-bins" */
	iGuardSize = TransmParam.iGuardSize;

	/* No. of minimum useful carrier */
	iShiftedKmin = TransmParam.iShiftedKmin;

	/* No. of maximum useful carrier */
	iShiftedKmax = TransmParam.iShiftedKmax; 
	
	/* Create plan for fftw */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
	FFTWPlan = fftw_create_plan(iDFTSize, FFTW_BACKWARD, FFTW_ESTIMATE);

	/* Allocate memory for intermediate result of fftw */
	veccFFTWInput.Init(iDFTSize);
	veccFFTWOutput.Init(iDFTSize);

	/* Zero out input vector (because only a few bins are used, the rest has to
	   be zero */
	for (int i = 0; i < iDFTSize; i++)
	{
		veccFFTWInput[i].re = 0;
		veccFFTWInput[i].im = 0;
	}

	/* Define block-sizes for input and output */
	iInputBlockSize = TransmParam.iNoCarrier;
	iOutputBlockSize = TransmParam.iSymbolBlockSize;
}

COFDMModulation::~COFDMModulation()
{
	/* Destroy FFTW plan */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
}


/******************************************************************************\
* OFDM-demodulation                                                            *
\******************************************************************************/
void COFDMDemodulation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i;
	_REAL		rNormCurFreqOffset;
	_REAL		rSkipGuardIntPhase;
	_COMPLEX	cExpStep;
	_REAL		rCurPower;

	/* Total frequency offset from acquisition and tracking (we calculate the
	   normalized frequency offset) */
	rNormCurFreqOffset = (_REAL) 2 * crPi * (ReceiverParam.rFreqOffsetAcqui + 
		ReceiverParam.rFreqOffsetTrack);

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
		veccFFTWInput[i].re = (*pvecInputData)[i] * real(cCurExp);
		veccFFTWInput[i].im = -(*pvecInputData)[i] * imag(cCurExp);
		
		/* Rotate exp-pointer on step further by complex multiplication with 
		   precalculated rotation vector cExpStep. This saves us from
		   calling sin() and cos() functions all the time (iterative 
		   calculation of these functions) */
		cCurExp *= cExpStep;
	}

	/* Calculate Fourier transformation (actual OFDM demodulation) */
	fftw_one(FFTWPlan, &veccFFTWInput[0], &veccFFTWOutput[0]);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData)[i - iShiftedKmin] = _COMPLEX(veccFFTWOutput[i].re / 
			iDFTSize, veccFFTWOutput[i].im / iDFTSize);


	/* SNR estimation. Use virtual carriers at the end of spectrum ---------- */
	_REAL	rPowLeftVCar, rPowRightVCar, rAvNoisePow, rUsNoPower;
	int		iNoAve;

	/* Power of useful part plus noise */
	rUsNoPower = (_REAL) 0.0;
	iNoAve = 0;
	for (i = 0; i < iNoCarrier; i++)
	{
		/* Use all carriers except of DC carriers */
		if (!_IsDC(ReceiverParam.matiMapTab[0][i]))
		{
			rUsNoPower += SqMag((*pvecOutputData)[i]);

			iNoAve++;
		}
	}

	/* Normalize result */
	rUsNoPower /= iNoAve;

	/* Estimate power of noise */
	/* First, check if virtual carriers are in range */
	if ((iShiftedKmin > 1) && (iShiftedKmax + 1 < iDFTSize))
	{
		/* Get current powers */
		rPowLeftVCar = veccFFTWOutput[iShiftedKmin - 1].re *
			veccFFTWOutput[iShiftedKmin - 1].re +
			veccFFTWOutput[iShiftedKmin - 1].im *
			veccFFTWOutput[iShiftedKmin - 1].im;

		rPowRightVCar = veccFFTWOutput[iShiftedKmax + 1].re *
			veccFFTWOutput[iShiftedKmax + 1].re +
			veccFFTWOutput[iShiftedKmax + 1].im *
			veccFFTWOutput[iShiftedKmax + 1].im;

		/* Average results */
		const _REAL rLam = 0.98;
		IIR1(rNoisePowAvLeft, rPowLeftVCar, rLam);
		IIR1(rNoisePowAvRight, rPowRightVCar, rLam);

		/* Take the smallest value of both to avoid getting sinusoid 
		   interferer in the measurement */
		rAvNoisePow = _min(rNoisePowAvLeft, rNoisePowAvRight);
	}

	/* Calculate SNR estimate */
	rSNREstimate = rUsNoPower / rAvNoisePow * iDFTSize * iDFTSize - 1;


	/* Save averaged spectrum for plotting ---------------------------------- */
	for (i = 0; i < iLenPowSpec; i++)
	{
		/* Power of this tap */
		rCurPower = veccFFTWOutput[i].re * veccFFTWOutput[i].re + 
			veccFFTWOutput[i].im * veccFFTWOutput[i].im;

		/* Averaging (first order IIR filter) */
		IIR1(vecrPowSpec[i], rCurPower, (_REAL) 0.99);
	}
}

void COFDMDemodulation::InitInternal(CParameter& ReceiverParam)
{
	iDFTSize = ReceiverParam.iFFTSizeN;
	iGuardSize = ReceiverParam.iGuardSize;
	iNoCarrier = ReceiverParam.iNoCarrier;
	iShiftedKmin = ReceiverParam.iShiftedKmin;
	iShiftedKmax = ReceiverParam.iShiftedKmax; 

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Initialize useful and noise power estimation */
	rNoisePowAvLeft = (_REAL) 0.0;
	rNoisePowAvRight = (_REAL) 0.0;

	/* Init SNR estimate */
	rSNREstimate = (_REAL) 0.0;

	/* Create plan for fftw */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
	FFTWPlan = fftw_create_plan(iDFTSize, FFTW_FORWARD, FFTW_ESTIMATE);

	/* Allocate memory for intermediate result of fftw */
	veccFFTWInput.Init(iDFTSize);
	veccFFTWOutput.Init(iDFTSize);

	/* Vector for power density spectrum of input signal */
	iLenPowSpec = iDFTSize / 2;
	vecrPowSpec.Init(iLenPowSpec, (_REAL) 0.0);

	/* Define block-sizes for input and output */
	iInputBlockSize = iDFTSize;
	iOutputBlockSize = iNoCarrier;
}

COFDMDemodulation::~COFDMDemodulation()
{
	/* Destroy FFTW plan */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
}

void COFDMDemodulation::GetPowDenSpec(CVector<_REAL>& vecrData,
									  CVector<_REAL>& vecrScale) 
{
	/* Init output vectors */
	vecrData.Init(iLenPowSpec, (_REAL) 0.0);
	vecrScale.Init(iLenPowSpec, (_REAL) 0.0);

	if (IsInInit() == FALSE)
	{
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
	int i;
	int iEndPointGuardRemov;

	/* Guard-interval removal parameter */
	iEndPointGuardRemov = iSymbolBlockSize - iGuardSize + iStartPointGuardRemov;


	/* Regular signal *********************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval */
	for (i = iStartPointGuardRemov; i < iEndPointGuardRemov; i++)
		veccFFTWInput[i - iStartPointGuardRemov].re = (*pvecInputData)[i];

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	fftw_one(FFTWPlan, &veccFFTWInput[0], &veccFFTWOutput[0]);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData)[i - iShiftedKmin] = _COMPLEX(veccFFTWOutput[i].re / 
			iDFTSize, veccFFTWOutput[i].im / iDFTSize);

	/* We need the same information for channel estimation evaluation, too */
	for (i = 0; i < iNoCarrier; i++)
		(*pvecOutputData2)[i] = (*pvecOutputData)[i];


	/* Channel-in signal ******************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval.
	   We have to cut out the FFT window at the correct position, because the
	   channel estimation has information only about the original pilots 
	   which are not phase shifted due to a timing-offset. To be able to 
	   compare reference signal and channel estimation output we have to use
	   the synchronous reference signal for input */
	for (i = iGuardSize; i < iSymbolBlockSize; i++)
		veccFFTWInput[i - iGuardSize].re = (*pvecInputData2)[i];

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	fftw_one(FFTWPlan, &veccFFTWInput[0], &veccFFTWOutput[0]);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData3)[i - iShiftedKmin] = _COMPLEX(veccFFTWOutput[i].re / 
			iDFTSize, veccFFTWOutput[i].im / iDFTSize);


	/* Reference signal *******************************************************/
	/* Convert input vector in fft-vector type and cut out the guard-interval */
	for (i = iStartPointGuardRemov; i < iEndPointGuardRemov; i++)
		veccFFTWInput[i - iStartPointGuardRemov].re = (*pvecInputData3)[i];

	/* Actual OFDM demodulation */
	/* Calculate Fourier transformation (actual OFDM demodulation) */
	fftw_one(FFTWPlan, &veccFFTWInput[0], &veccFFTWOutput[0]);

	/* Use only useful carriers and normalize with the block-size ("N") */
	for (i = iShiftedKmin; i < iShiftedKmax + 1; i++)
		(*pvecOutputData4)[i - iShiftedKmin] = _COMPLEX(veccFFTWOutput[i].re / 
			iDFTSize, veccFFTWOutput[i].im / iDFTSize);


	/* Take care of symbol IDs ---------------------------------------------- */
	iSymbolCounterTiSy++;
	if (iSymbolCounterTiSy == iNoSymPerFrame)
		iSymbolCounterTiSy = 0;

	/* Set current symbol number in extended data of output vector */
	(*pvecOutputData).GetExData().iSymbolNo = iSymbolCounterTiSy;
	(*pvecOutputData2).GetExData().iSymbolNo = iSymbolCounterTiSy;
	(*pvecOutputData3).GetExData().iSymbolNo = iSymbolCounterTiSy;
	(*pvecOutputData4).GetExData().iSymbolNo = iSymbolCounterTiSy;

	/* No timing corrections, timing is constant in this case */
	(*pvecOutputData).GetExData().iCurTimeCorr = 0;
	(*pvecOutputData2).GetExData().iCurTimeCorr = 0;
	(*pvecOutputData3).GetExData().iCurTimeCorr = 0;
	(*pvecOutputData4).GetExData().iCurTimeCorr = 0;
}

void COFDMDemodSimulation::InitInternal(CParameter& ReceiverParam)
{
	int iStartGR;
	int iChanDelSpLen;

	/* Set internal parameters */
	iDFTSize = ReceiverParam.iFFTSizeN;
	iGuardSize = ReceiverParam.iGuardSize;
	iNoCarrier = ReceiverParam.iNoCarrier;
	iShiftedKmin = ReceiverParam.iShiftedKmin;
	iShiftedKmax = ReceiverParam.iShiftedKmax;
	iSymbolBlockSize = ReceiverParam.iSymbolBlockSize;
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;

	/* Create plan for fftw */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
	FFTWPlan = fftw_create_plan(iDFTSize, FFTW_FORWARD, FFTW_ESTIMATE);

	/* Allocate memory for intermediate result of fftw */
	veccFFTWInput.Init(iDFTSize);
	veccFFTWOutput.Init(iDFTSize);

	/* Set imaginary part to zero, this is not written by the input signal */
	for (int i = 0; i < iDFTSize; i++)
		veccFFTWInput[i].im = (_REAL) 0.0;


	/* Init internal counter for symbol number. Set it to this value to get
	   a "0" for the first time */
	iSymbolCounterTiSy = iNoSymPerFrame - 1;

	/* Set guard-interval removal start index. Adapt this parameter to the 
	   channel which was chosen. Place the delay spread centered in the 
	   middle of the guard-interval */
	switch (ReceiverParam.iDRMChannelNo)
	{
	case 1:
		/* AWGN */
		iChanDelSpLen = 0;
		break;

	case 2:
		/* Rice with delay */
		iChanDelSpLen = (int) ((_REAL) 1.0 * SOUNDCRD_SAMPLE_RATE / 1000);
		break;

	case 3:
		/* US Consortium */
		iChanDelSpLen = (int) ((_REAL) 2.2 * SOUNDCRD_SAMPLE_RATE / 1000);
		break;

	case 4:
		/* CCIR Poor */
		iChanDelSpLen = (int) ((_REAL) 2.0 * SOUNDCRD_SAMPLE_RATE / 1000);
		break;
		
	case 5:
		/* Channel no 5 */
		iChanDelSpLen = (int) ((_REAL) 4.0 * SOUNDCRD_SAMPLE_RATE / 1000);
		break;
		
	case 6:
		/* Channel no 6 */
		iChanDelSpLen = (int) ((_REAL) 6.0 * SOUNDCRD_SAMPLE_RATE / 1000);
		break;

	default:
		/* My own channels */
		iChanDelSpLen = iGuardSize;
		break;
	}

	iStartGR = (iGuardSize - iChanDelSpLen) / 2;
	if (iStartGR < 0)
	{
		/* In this case we get Inter-Symbol-Interference (ISI) */
		iStartGR = 0;
	}

	iStartPointGuardRemov = iGuardSize - iStartGR;

	/* Define block-sizes for input and output */
	iInputBlockSize = iSymbolBlockSize;
	iInputBlockSize2 = iSymbolBlockSize;
	iInputBlockSize3 = iSymbolBlockSize;
	iOutputBlockSize = iNoCarrier;
	iOutputBlockSize2 = iNoCarrier;
	iOutputBlockSize3 = iNoCarrier;
	iOutputBlockSize4 = iNoCarrier;

	/* We need to store as many symbols in output buffer as long the channel
	   estimation delay is */
	iMaxOutputBlockSize2 = iNoCarrier * ReceiverParam.iChanEstDelay;
	iMaxOutputBlockSize3 = iNoCarrier * ReceiverParam.iChanEstDelay;
	iMaxOutputBlockSize4 = iNoCarrier * ReceiverParam.iChanEstDelay;
}

COFDMDemodSimulation::~COFDMDemodSimulation()
{
	/* Destroy FFTW plan */
	if (FFTWPlan != NULL)
		fftw_destroy_plan(FFTWPlan);
}
