/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Channel estimation and equalization
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

#include "ChannelEstimation.h"


/* Implementation *************************************************************/
void CChannelEstimation::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i, j, k;
	int			iModSymNum;
	_COMPLEX	cModChanEst;
	_REAL		rSNRAftTiInt;
	_REAL		rCurSNREst;

	/* Move data in history-buffer (from iLenHistBuff - 1 towards 0) */
	for (j = 0; j < iLenHistBuff - 1; j++)
	{
		for (i = 0; i < iNoCarrier; i++)
			matcHistory[j][i] = matcHistory[j + 1][i];
	}

	/* Write new symbol in memory */
	for (i = 0; i < iNoCarrier; i++)
		matcHistory[iLenHistBuff - 1][i] = (*pvecInputData)[i];


	/* Time interpolation *****************************************************/
	/* Get symbol-counter for next symbol. Use the count from the frame 
	   synchronization (in OFDM.cpp). Call estimation routine */
	rSNRAftTiInt = 
		pTimeInt->Estimate(pvecInputData, veccPilots, 
						   ReceiverParam.matiMapTab[(*pvecInputData).
						   GetExData().iSymbolNo],
						   ReceiverParam.matcPilotCells[(*pvecInputData).
						   GetExData().iSymbolNo], rSNREstimate);

	/* Special case with robustness mode D: since "iScatPilFreqInt" is "1", we
	   get the DC carrier as a pilot position. We have to interpolate this
	   position since there are no pilots (we do a linear interpolation) */
	if (iDCPos != 0)
	{
		/* Actual linear interpolation with carrieres left and right from DC */
		veccPilots[iDCPos] = veccPilots[iDCPos - 1] + 
			(veccPilots[iDCPos + 1] - veccPilots[iDCPos - 1]) / (_REAL) 2.0;
	}


	/* -------------------------------------------------------------------------
	   Use time-interpolated channel estimate for timing synchronization 
	   tracking */
	rDelaySprEstInd = TimeSyncTrack.Process(ReceiverParam, veccPilots, 
		(*pvecInputData).GetExData().iCurTimeCorr);


	/* Frequency-interploation ************************************************/
	switch (TypeIntFreq)
	{
	case FLINEAR:
		/**********************************************************************\
		 * Linear interpolation												  *
		\**********************************************************************/
		/* Set first pilot position */
		veccChanEst[0] = veccPilots[0];

		for (j = 0, k = 1; j < iNoCarrier - iScatPilFreqInt;
			j += iScatPilFreqInt, k++)
		{
			/* Set values at second time pilot position in cluster */
			veccChanEst[j + iScatPilFreqInt] = veccPilots[k];

			/* Interpolation cluster */
			for (i = 1; i < iScatPilFreqInt; i++)
			{
				/* E.g.: c(x) = (c_4 - c_0) / 4 * x + c_0 */
				veccChanEst[j + i] = 
					(veccChanEst[j + iScatPilFreqInt] - veccChanEst[j]) /
					(_REAL) (iScatPilFreqInt * i) + veccChanEst[j];
			}
		}
		break;

	case FDFTFILTER:
		/**********************************************************************\
		 * DFT based algorithm												  *
		\**********************************************************************/
		/* ---------------------------------------------------------------------
		   Put all pilots at the beginning of the vector. The "real" length of
		   the vector "pcFFTWInput" is longer than the No of pilots, but we 
		   calculate the FFT only over "iNoCarrier / iScatPilFreqInt + 1" values
		   (this is the number of pilot positions) */
		/* Weighting pilots with window */
		veccPilots *= vecrDFTWindow;

		/* Transform in time-domain */
		veccPilots = Ifft(veccPilots, FftPlanShort);

		/* Set values outside a defined bound to zero, zero padding (noise
		   filtering). Copy second half of spectrum at the end of the new vector 
		   length and zero out samples between the two parts of the spectrum */
		veccIntPil.Merge(
			/* First part of spectrum */
			veccPilots(1, iStartZeroPadding), 
			/* Zero padding in the middle, length: Total length minus length of
			   the two parts at the beginning and end */
			CComplexVector(Zeros(iLongLenFreq - 2 * iStartZeroPadding), 
			Zeros(iLongLenFreq - 2 * iStartZeroPadding)), 
			/* Set the second part of the actual spectrum at the end of the new
			   vector */
			veccPilots(iNoIntpFreqPil - iStartZeroPadding + 1, 
			iNoIntpFreqPil));

		/* Transform back in frequency-domain */
		veccIntPil = Fft(veccIntPil, FftPlanLong);

		/* Remove weighting with DFT window by inverse multiplication */
		veccChanEst = veccIntPil(1, iNoCarrier) * vecrDFTwindowInv;
		break;

	case FWIENER:
		/**********************************************************************\
		 * Wiener filter													   *
		\**********************************************************************/
		/* Update filter coefficients once in one DRM frame */
		if (iUpCntWienFilt > 0)
		{
			iUpCntWienFilt--;

			/* Get maximum delay spread in one DRM frame */
			if (rDelaySprEstInd > rMaxDelaySprInFra)
				rMaxDelaySprInFra = rDelaySprEstInd;
		}
		else
		{
			/* Update filter taps */
			UpdateWienerFiltCoef(rSNRAftTiInt, rMaxDelaySprInFra / iNoCarrier);

			/* Reset counter and maximum storage variable */
			iUpCntWienFilt = iNoSymPerFrame;
			rMaxDelaySprInFra = (_REAL) 0.0;
		}

		/* FIR filter of the pilots with filter taps. We need to filter the
		   pilot positions as well to improve the SNR estimation (which 
		   follows this procedure) */
		for (j = 0; j < iNoCarrier; j++)
		{
			/* Convolution */
			veccChanEst[j] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);

			for (i = 0; i < iLengthWiener; i++)
				veccChanEst[j] += 
					matcFiltFreq[j][i] * veccPilots[veciPilOffTab[j] + i];
		}
		break;
	}


	/* Equalize the output vector ------------------------------------------- */
	/* Write to output vector. Take oldest symbol of history for output. Also,
	   ship the channel state at a certain cell */
	for (i = 0; i < iNoCarrier; i++)
	{
		(*pvecOutputData)[i].cSig = matcHistory[0][i] / veccChanEst[i];
		(*pvecOutputData)[i].rChan = SqMag(veccChanEst[i]);
	}


	/* -------------------------------------------------------------------------
	   Calculate symbol number of the current output block and set parameter */
	(*pvecOutputData).GetExData().iSymbolNo = 
		(*pvecInputData).GetExData().iSymbolNo - iLenHistBuff + 1;


	/* SNR estimation ------------------------------------------------------- */
	/* Use estimated channel and compare it to the received pilots. This 
	   estimation works only if the channel estimation was successful */
	/* Modified symbol number, check range {0, ..., iNoSymPerFrame} */
	iModSymNum = (*pvecOutputData).GetExData().iSymbolNo;

	while (iModSymNum < 0)
		iModSymNum += iNoSymPerFrame;

	for (i = 0; i < iNoCarrier; i++)
	{
		/* Identify pilot positions. Use MODIFIED "iSymbolNo" (See lines
		   above) */
		if (_IsScatPil(ReceiverParam.matiMapTab[iModSymNum][i]))
		{
			/* We assume that the channel estimation in "veccChanEst" is noise
			   free (e.g., the wiener interpolation does noise reduction). 
			   Thus, we have an estimate of the received signal power 
			   \hat{r} = s * \hat{h}_{wiener} */
			cModChanEst = 
				veccChanEst[i] * ReceiverParam.matcPilotCells[iModSymNum][i];


			/* Calculate and average noise and signal estimates ------------- */
			/* The noise estimation is difference between the noise reduced
			   signal and the noisy received signal
			   \tilde{n} = \hat{r} - r */
			IIR1(rNoiseEst, SqMag(matcHistory[0][i] - cModChanEst),
				rLamSNREstFast);

			/* The received signal power estimation is just \hat{r} */
			IIR1(rSignalEst, SqMag(cModChanEst), rLamSNREstFast);

			/* Calculate final result (signal to noise ratio) */
			if (rNoiseEst != 0)
				rCurSNREst = rSignalEst / rNoiseEst;
			else
				rCurSNREst = (_REAL) 1.0;

			/* Bound the SNR at 0 dB */
			if (rCurSNREst < (_REAL) 1.0)
				rCurSNREst = (_REAL) 1.0;

			/* Average the SNR with a two sided recursion */
			IIR1TwoSided(rSNREstimate, rCurSNREst, rLamSNREstFast,
				rLamSNREstSlow);
		}
	}


	/* Debar initialization ------------------------------------------------- */
	if (iInitCnt > 0)
	{
		iInitCnt--;

		/* Do not put out data in initialization phase */
		iOutputBlockSize = 0;
	}
	else
		iOutputBlockSize = iNoCarrier; 
}

void CChannelEstimation::InitInternal(CParameter& ReceiverParam)
{
	/* Get parameters from global struct */
	iScatPilTimeInt = ReceiverParam.iScatPilTimeInt;
	iScatPilFreqInt = ReceiverParam.iScatPilFreqInt;
	iNoIntpFreqPil = ReceiverParam.iNoIntpFreqPil;
	iNoCarrier = ReceiverParam.iNoCarrier;
	iFFTSizeN = ReceiverParam.iFFTSizeN;
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;

	/* Length of guard-interval with respect to FFT-size! */
	iGuardSizeFFT = iNoCarrier * 
		ReceiverParam.RatioTgTu.iEnum / ReceiverParam.RatioTgTu.iDenom;

	/* If robustness mode D is active, get DC position. This position cannot
	   be "0" since in mode D no 5 kHz mode is defined (see DRM-standard). 
	   Therefor we can also use this variable to get information whether
	   mode D is active or not (by simply do: "if (iDCPos != 0)" */
	if (ReceiverParam.GetWaveMode() == RM_ROBUSTNESS_MODE_D)
	{
		/* Identify CD carrier position */
		for (int i = 0; i < iNoCarrier; i++)
		{
			if (_IsDC(ReceiverParam.matiMapTab[0][i]))
				iDCPos = i;
		}
	}
	else
		iDCPos = 0;

	/* FFT must be longer than "iNoCarrier" because of zero padding effect (
	   not in robustness mode D! -> "iLongLenFreq = iNoCarrier") */
	iLongLenFreq = iNoCarrier + iScatPilFreqInt - 1;

	/* Init vector for received data at pilot positions */
	veccPilots.Init(iNoIntpFreqPil);

	/* Init vector for interpolated pilots */
	veccIntPil.Init(iLongLenFreq);

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlanShort.Init(iNoIntpFreqPil);
	FftPlanLong.Init(iLongLenFreq);

	/* Choose time interpolation method and set pointer to correcponding 
	   object */
	switch (TypeIntTime)
	{
	case TLINEAR:
		pTimeInt = &TimeLinear;
		break;

	case TWIENER:
		pTimeInt = &TimeWiener;
		break;

	case TDECIDIR:
		pTimeInt = &TimeDecDir;
		break;
	}

	/* Init time interpolation interface and set delay for interpolation */
	iLenHistBuff = pTimeInt->Init(ReceiverParam);

	/* Init time synchronization tracking unit */
	TimeSyncTrack.Init(ReceiverParam, iLenHistBuff);

	/* Set channel estimation delay in global struct. This is needed for 
	   simulation */
	ReceiverParam.iChanEstDelay = iLenHistBuff;


	/* Init window for DFT operation for frequency interpolation ------------ */
	/* Init memory */
	vecrDFTWindow.Init(iNoIntpFreqPil);
	vecrDFTwindowInv.Init(iNoCarrier);

	/* Set window coefficients */
	switch (eDFTWindowingMethod)
	{
	case DFT_WIN_RECT:
		vecrDFTWindow = Ones(iNoIntpFreqPil);
		vecrDFTwindowInv = Ones(iNoCarrier);
		break;

	case DFT_WIN_HAMM:
		vecrDFTWindow = Hamming(iNoIntpFreqPil);
		vecrDFTwindowInv = (CReal) 1.0 / Hamming(iNoCarrier);
		break;
	}


	/* Set start index for zero padding in time domain for DFT method */
	iStartZeroPadding = iGuardSizeFFT;
	if (iStartZeroPadding > iNoIntpFreqPil)
		iStartZeroPadding = iNoIntpFreqPil;

	/* Allocate memory for channel estimation */
	veccChanEst.Init(iNoCarrier);

	/* Allocate memory for history buffer (Matrix) and zero out */
	matcHistory.Init(iLenHistBuff, iNoCarrier,
		_COMPLEX((_REAL) 0.0, (_REAL) 0.0));

	/* After an initialization we do not put out data befor the number symbols
	   of the channel estimation delay have been processed */
	iInitCnt = iLenHistBuff - 1;

	/* Inits for SNR estimation (noise and signal averages) */
	rSNREstimate = (_REAL) pow(10, (_REAL) 20.0 / 10);
	rNoiseEst = (_REAL) 0.0;
	rSignalEst = (_REAL) 0.0;

	/* Lambda for IIR filter */
	rLamSNREstFast = IIR1Lam(TICONST_SNREST_FAST, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize);
	rLamSNREstSlow = IIR1Lam(TICONST_SNREST_SLOW, (CReal) SOUNDCRD_SAMPLE_RATE /
		ReceiverParam.iSymbolBlockSize);


	/* SNR correction factor. We need this factor since we evalute the 
	   signal-to-noise ratio only on the pilots and these have a higher power as
	   the other cells */
	rSNRCorrectFact = 
		ReceiverParam.rAvPilPowPerSym /	ReceiverParam.rAvPowPerSymbol;

	/* Init delay spread length estimation (index) */
	rDelaySprEstInd = (_REAL) 0.0;
	rMaxDelaySprInFra = (_REAL) 0.0; /* Maximum estimated delay spread in
									    one DRM frame */


	/* Inits for Wiener interpolation in frequency direction ---------------- */
	/* Length of wiener filter */
	switch (ReceiverParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMA;
		break;

	case RM_ROBUSTNESS_MODE_B:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMB;
		break;

	case RM_ROBUSTNESS_MODE_C:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMC;
		break;

	case RM_ROBUSTNESS_MODE_D:
		iLengthWiener = LEN_WIENER_FILT_FREQ_RMD;
		break;
	}


	/* Inits for wiener filter ---------------------------------------------- */
	/* In frequency direction we can use pilots from both sides for 
	   interpolation */
	iPilOffset = iLengthWiener / 2;

	/* Allocate memory */
	matcFiltFreq.Init(iNoCarrier, iLengthWiener);

	/* Pilot offset table */
	veciPilOffTab.Init(iNoCarrier);

	/* Number of different wiener filters */
	iNoWienerFilt = (iLengthWiener - 1) * iScatPilFreqInt + 1;

	/* Allocate temporary matlib vector for filter coefficients */
	matcWienerFilter.Init(iNoWienerFilt, iLengthWiener);

	/* Init Update counter for wiener filter update */
	iUpCntWienFilt = iNoSymPerFrame;


	/* SNR definition */
	const _REAL rSNRdB = (_REAL) 30.0;
	_REAL rSNR = pow(10, rSNRdB / 10);

	/* Init wiener filter */
	UpdateWienerFiltCoef(rSNR, (_REAL) ReceiverParam.RatioTgTu.iEnum / 
		ReceiverParam.RatioTgTu.iDenom);


	/* Define block-sizes for input and output */
	iInputBlockSize = iNoCarrier;
	iMaxOutputBlockSize = iNoCarrier; 
}

CComplexVector CChannelEstimation::FreqOptimalFilter(int iFreqInt, int iDiff,
													 _REAL rSNR,
													 _REAL rRatGuarLen,
													 int iLength)
{
	int				i;
	int				iCurPos;
	CComplexVector	veccReturn(iLength);
	CComplexVector	veccRpp(iLength);
	CComplexVector	veccRhp(iLength);

	/* Calculation of R_hp, this is the SHIFTED correlation function */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iFreqInt - iDiff;

		veccRhp[i] = FreqCorrFct(iCurPos, rRatGuarLen);
	}

	/* Calculation of R_pp */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iFreqInt;

		veccRpp[i] = FreqCorrFct(iCurPos, rRatGuarLen);
	}

	/* Add SNR at first tap */
	veccRpp[0] += (_REAL) 1.0 / rSNR;

	/* Call levinson algorithm to solve matrix system for optimal solution */
	veccReturn = Levinson(veccRpp, veccRhp);

	return veccReturn;
}

_COMPLEX CChannelEstimation::FreqCorrFct(int iCurPos, _REAL rRatGuarLen)
{
	/* We assume that the power delay spread is a rectangle function in the time
	   domain with the lenght of the guard interval (sinc-function in the
	   frequency domain) */
	if (iCurPos == 0)
		return (_REAL) 1.0;
	else
	{
		/* First calculate the argument of the sinc- and exp-function */
		_REAL rArg = (_REAL) crPi * iCurPos * rRatGuarLen;

		/* si(pi * n * rat) * exp(pi * n * rat) */
		return sin(rArg) / rArg * _COMPLEX(cos(rArg), sin(rArg));
	}
}

void CChannelEstimation::UpdateWienerFiltCoef(_REAL rNewSNR, _REAL rNewRatio)
{
	int	j, i;
	int	iDiff;
	int	iCurPil;

	/* Calculate all possible wiener filters */
	for (j = 0; j < iNoWienerFilt; j++)
		matcWienerFilter[j] = FreqOptimalFilter(iScatPilFreqInt, j, rNewSNR,
			rNewRatio, iLengthWiener);


#if 0
#ifdef _DEBUG_
/* Save filter coefficients */
static FILE* pFile = fopen("test/wienerfreq.dat", "w");
for (j = 0; j < iNoWienerFilt; j++)
	for (i = 0; i < iLengthWiener; i++)
		fprintf(pFile, "%e\n", matcWienerFilter[j][i]);
fflush(pFile);
#endif
#endif



	/* Set matrix with filter taps, one filter for each carrier */
	for (j = 0; j < iNoCarrier; j++)
	{
		/* We define the current pilot position as the last pilot which the
		   index "j" has passed */
		iCurPil = (int) (j / iScatPilFreqInt);

		/* Consider special cases at the edges of the DRM spectrum */
		if (iCurPil < iPilOffset)
		{
			/* Special case: left edge */
			veciPilOffTab[j] = 0;
		}
		else if (iCurPil - iPilOffset > iNoIntpFreqPil - iLengthWiener)
		{
			/* Special case: right edge */
			veciPilOffTab[j] = iNoIntpFreqPil - iLengthWiener;
		}
		else
		{
			/* In the middle */
			veciPilOffTab[j] = iCurPil - iPilOffset;
		}

		/* Difference between the position of the first pilot (for filtering)
		   and the position of the observed carrier */
		iDiff = j - veciPilOffTab[j] * iScatPilFreqInt;

		/* Copy correct filter in matrix */
		for (i = 0; i < iLengthWiener; i++)
			matcFiltFreq[j][i] = matcWienerFilter[iDiff][i];
	}
}

_REAL CChannelEstimation::GetSNREstdB() const
{
	/* Bound the SNR at 0 dB */
	if (rSNREstimate * rSNRCorrectFact > (_REAL) 1.0)
		return 10 * log10(rSNREstimate * rSNRCorrectFact);
	else
		return (_REAL) 0.0;
}

_REAL CChannelEstimation::GetDelay() const
{
	/* Delay in ms */
	return rDelaySprEstInd * iFFTSizeN / 
		(SOUNDCRD_SAMPLE_RATE * iNoIntpFreqPil * 2) * 1000;
}

void CChannelEstimation::GetTransferFunction(CVector<_REAL>& vecrData,
											 CVector<_REAL>& vecrScale)
{
	/* Init output vectors */
	vecrData.Init(iNoCarrier, (_REAL) 0.0);
	vecrScale.Init(iNoCarrier, (_REAL) 0.0);

	/* Lock resources */
	Lock();

	/* Copy data in output vector and set scale 
	   (carrier index as x-scale) */
	for (int i = 0; i < iNoCarrier; i++)
	{
		_REAL rNormChanEst = abs(veccChanEst[i]) / (_REAL) iNoCarrier;
			
		if (rNormChanEst > 0)
			vecrData[i] = (_REAL) 20.0 * log10(rNormChanEst);
		else
			vecrData[i] = RET_VAL_LOG_0;

		/* Scale */
		vecrScale[i] = i;
	}

	/* Release resources */
	Unlock();
}

void CChannelEstimation::GetAvPoDeSp(CVector<_REAL>& vecrData,
									 CVector<_REAL>& vecrScale,
									 _REAL& rLowerBound, _REAL& rHigherBound,
									 _REAL& rStartGuard, _REAL& rEndGuard,
									 _REAL& rLenIR)
{
	/* Lock resources */
	Lock();

	TimeSyncTrack.GetAvPoDeSp(vecrData, vecrScale, rLowerBound,
		rHigherBound, rStartGuard, rEndGuard, rLenIR);

	/* Release resources */
	Unlock();
}


