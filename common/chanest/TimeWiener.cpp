/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Wiener filter in time direction for channel estimation
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

#include "TimeWiener.h"


/* Implementation *************************************************************/
_REAL CTimeWiener::Estimate(CVectorEx<_COMPLEX>* pvecInputData, 
						    CComplexVector& veccOutputData, 
						    CVector<int>& veciMapTab, 
						    CVector<_COMPLEX>& veccPilotCells, _REAL rSNR)
{
	int				j, i;
	int				iPiHiIndex;
	int				iCurrFiltPhase;
	int				iTimeDiffNew;
	_COMPLEX		cNewPilot;
	CVector<_REAL>	vecrTiCorrEstSym;

	/* Timing correction history -------------------------------------------- */
	/* Shift old vaules and add a "0" at the beginning of the vector */
	vecTiCorrHist.AddBegin(0);

	/* Add new one to all history values except of the current one */
	for (i = 1; i < iLenTiCorrHist; i++)
		vecTiCorrHist[i] += (*pvecInputData).GetExData().iCurTimeCorr;


	/* Update histories for channel estimates at the pilot positions -------- */
	/* Init vector for storing time correlation estimation for one symbol */
	vecrTiCorrEstSym.Init(iNumTapsSigEst, (_REAL) 0.0);

	for (i = 0; i < iNoCarrier; i++)
	{
		/* Identify and calculate transfer function at the pilot positions */
		if (_IsScatPil(veciMapTab[i]))
		{
			/* Pilots are only every "iScatPilFreqInt"'th carrier. It is not
			   possible just to increase the "iPiHiIndex" because not in all 
			   cases a pilot is at position zero in "matiMapTab[]" */
			iPiHiIndex = i / iScatPilFreqInt;

			/* Save channel estimates at the pilot positions for each carrier.
			   Move old estimates and put new value. Use reversed order to
			   prepare vector for convolution */
			for (j = iLengthWiener - 1; j > 0; j--)
				matcChanAtPilPos[j][iPiHiIndex] = 
					matcChanAtPilPos[j - 1][iPiHiIndex];

			/* Add new channel estimate: h = r / s, h: transfer function of the
			   channel, r: received signal, s: transmitted signal */
			matcChanAtPilPos[0][iPiHiIndex] = 
				(*pvecInputData)[i] / veccPilotCells[i];


			/* Estimation of the channel correlation function --------------- */
			/* We calcuate the estimation for one symbol first and average this
			   result */
			for (j = 0; j < iNumTapsSigEst; j++)
			{
				/* Correct pilot information for phase rotation */
				iTimeDiffNew = -vecTiCorrHist[iScatPilTimeInt * j];
				cNewPilot = 
					Rotate(matcChanAtPilPos[j][iPiHiIndex], i, iTimeDiffNew);

				/* Simply add all results together and increment count */
				vecrTiCorrEstSym[j] += 
					real(conj(matcChanAtPilPos[0][iPiHiIndex]) * cNewPilot);
			}
		}
	}


	/* Update sigma estimation ---------------------------------------------- */
	/* Update moving average for overall averaging for correlation estimation */
	for (i = 0; i < iNumTapsSigEst; i++)
	{
		/* Subtract oldest value in history from current estimation */
		vecrTiCorrEst[i] -= matrTiCorrEstHist[iCurIndTiCor][i];

		/* Add new value and write in memory */
		vecrTiCorrEst[i] += vecrTiCorrEstSym[i];
		matrTiCorrEstHist[iCurIndTiCor][i] = vecrTiCorrEstSym[i];
	}

	/* Increase position pointer and test if wrap */
	iCurIndTiCor++;
	if (iCurIndTiCor == NO_SYM_AVER_TI_CORR)
		iCurIndTiCor = 0;

	/* Update filter coefficients once in one DRM frame */
	if (iUpCntWienFilt > 0)
		iUpCntWienFilt--;
	else
	{
		/* Actual estimation of sigma */
		rSigma = ModLinRegr(vecrTiCorrEst);

		/* Update the wiener filter */
		rMMSE = UpdateFilterCoef(rSNR, rSigma);

		/* Reset counter */
		iUpCntWienFilt = iNoSymPerFrame;
	}


	/* Wiener interpolation, filtering and prediction ----------------------- */
	for (i = 0; i < iNoCarrier; i += iScatPilFreqInt)
	{
		/* This check is for robustness mode D since "iScatPilFreqInt" is "1"
		   in this case which would include the DC carrier in the for-loop */
		if (!_IsDC(veciMapTab[i]))
		{
			/* Pilots are only every "iScatPilFreqInt"'th carrier */
			iPiHiIndex = i / iScatPilFreqInt;

			/* Calculate current filter phase, use distance to next pilot */
			iCurrFiltPhase = (iScatPilTimeInt - DisToNextPil(iPiHiIndex, 
				(*pvecInputData).GetExData().iSymbolNo)) % iScatPilTimeInt;

			/* Convolution with one phase of the optimal filter */
			/* Init sum */
			veccChanEst[iPiHiIndex] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
			for (j = 0; j < iLengthWiener; j++)
			{
				/* We need to correct pilots due to timing corrections ------ */
				/* Calculate timing difference */
				iTimeDiffNew = vecTiCorrHist[iLenHistBuff - 1] - 
					vecTiCorrHist[j * iScatPilTimeInt + iCurrFiltPhase];

				/* Correct pilot information for phase rotation */
				cNewPilot = 
					Rotate(matcChanAtPilPos[j][iPiHiIndex], i, iTimeDiffNew);

				/* Actual convolution with filter phase */
				veccChanEst[iPiHiIndex] += 
					cNewPilot * matrFiltTime[iCurrFiltPhase][j];
			}

			
			/* Copy channel estimation from current symbol in output buffer - */
			veccOutputData[i] = veccChanEst[iPiHiIndex];
		}
	}

	/* Return the SNR improvement by wiener interpolation in time direction. If
	   no SNR improvent was achieved, just return old SNR */
	if (1 / rMMSE < rSNR)
		return rSNR;
	else
		return 1 / rMMSE;
}

int CTimeWiener::DisToNextPil(int iPiHiIndex, int iSymNo)
{
	/* Distance to next pilot (later in time!) of one specific 
	   carrier (with pilot). We do the "iNoSymPerFrame - iSymNo" to avoid
	   negative numbers in the modulo operation */
	return (iNoSymPerFrame - iSymNo + iFirstSymbWithPi + iPiHiIndex) %
		iScatPilTimeInt;
}

int CTimeWiener::Init(CParameter& ReceiverParam)
{
	int		i, j;
	int		iNoPiFreqDirAll;
	int		iSymDelyChanEst;
	_REAL	rSNR;

	/* Init base class, must be at the beginning of this init! */
	CPilotModiClass::InitRot(ReceiverParam);
	
	/* Set local parameters */
	iNoCarrier = ReceiverParam.iNoCarrier;
	iScatPilTimeInt = ReceiverParam.iScatPilTimeInt;
	iScatPilFreqInt = ReceiverParam.iScatPilFreqInt;
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;

	/* We have to consider the last pilot at the end of the symbol ("+ 1") */
	iNoPiFreqDirAll = iNoCarrier / iScatPilFreqInt + 1;

	/* Parameters found by looking at resulting filter coefficients. The values
	   "rSigma" are set to the maximum possible doppler frequency which can be
	   interpolated by the pilot frequency grid. Since we have a Gaussian 
	   power spectral density, the power is never exactely zero. Therefore we 
	   determine the point where the PDS has fallen below a 30 dB limit */
	switch (ReceiverParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		iLengthWiener = 15;
		rSigma = (_REAL) 2.0 / 2;
		break;

	case RM_ROBUSTNESS_MODE_B:
		iLengthWiener = 25;
		rSigma = (_REAL) 3.36 / 2;
		break;
	
	case RM_ROBUSTNESS_MODE_C:
		iLengthWiener = 9;
		rSigma = (_REAL) 6.73 / 2;
		break;
	
	case RM_ROBUSTNESS_MODE_D:
		iLengthWiener = 9;
		rSigma = (_REAL) 5.38 / 2;
		break;
	}

	/* Set delay of this channel estimation type. The longer the delay is, the
	   more "acausal" pilots can be used for interpolation. We use the same
	   amount of causal and acausal filter taps here */
	iSymDelyChanEst = (iLengthWiener / 2) * iScatPilTimeInt - 1;

	/* Set number of phases for wiener filter */
	iNoFiltPhasTi = iScatPilTimeInt;

	/* Set length of history-buffer */
	iLenHistBuff = iSymDelyChanEst + 1;

	/* Duration of useful part plus-guard interval */
	Ts = (_REAL) (ReceiverParam.iFFTSizeN + ReceiverParam.iGuardSize) / 
		SOUNDCRD_SAMPLE_RATE;

	/* Allocate memory for Channel at pilot positions (Matrix) and zero out */
	matcChanAtPilPos.Init(iLengthWiener, iNoPiFreqDirAll, 
		_COMPLEX((_REAL) 0.0, (_REAL) 0.0));

	/* Set number of taps for sigma estimation */
	if (iLengthWiener < NO_TAPS_USED4SIGMA_EST)
		iNumTapsSigEst = iLengthWiener;
	else
		iNumTapsSigEst = NO_TAPS_USED4SIGMA_EST;

	/* Init matrix for estimation the correlation function in time direction
	   (moving average) */
	matrTiCorrEstHist.Init(NO_SYM_AVER_TI_CORR, iNumTapsSigEst, (_REAL) 0.0);
	vecrTiCorrEst.Init(iNumTapsSigEst, (_REAL) 0.0);
	iCurIndTiCor = 0;


	/* Init Update counter for wiener filter update */
	iUpCntWienFilt = iNoSymPerFrame;

	/* Allocate memory for filter phases (Matrix) */
	matrFiltTime.Init(iNoFiltPhasTi, iLengthWiener);

	/* Allocate memory for channel estimation */
	veccChanEst.Init(iNoPiFreqDirAll);

	/* Length of the timing correction history buffer */
	iLenTiCorrHist = iLengthWiener * iNoFiltPhasTi;

	/* Init timing correction history with zeros */
	vecTiCorrHist.Init(iLenTiCorrHist, 0);

	/* Get the index of first symbol in a super-frame on where the first cell
	   (carrier-index = 0) is a pilot. This is needed for determining the
	   right filter-phase for the convolution */
	iFirstSymbWithPi = 0;
	while (!_IsScatPil(ReceiverParam.matiMapTab[iFirstSymbWithPi][0]))
		iFirstSymbWithPi++;


	/* Calculate optimal filter --------------------------------------------- */
	/* Init SNR value */
	const _REAL rSNRdB = (_REAL) 25.0;
	rSNR = pow(10, rSNRdB / 10);

	/* Calculate initialization wiener filter taps and init MMSE */
	rMMSE = UpdateFilterCoef(rSNR, rSigma);

	/* Return delay of channel equalization */
	return iLenHistBuff;
}

_REAL CTimeWiener::UpdateFilterCoef(_REAL rNewSNR, _REAL rNewSigma)
{
	int		i, j;
	int		iCurrDiffPhase;
	_REAL	rMMSE;

	/* Vector for intermedia result */
	CRealVector vecrTempFilt(iLengthWiener);

	/* Calculate MMSE for wiener filtering for all phases and average */
	rMMSE = (_REAL) 0.0;

	/* One filter for all possible filter phases */
	for (j = 0; j < iNoFiltPhasTi; j++)
	{
		/* We have to define the dependency between the difference between the
		   current pilot to the observed symbol in the history buffer and the
		   indizes of the FiltTime array. Definition:
		   Largest distance = index zero, index increases to smaller 
		   distances */
		iCurrDiffPhase = -(iLenHistBuff - j - 1);
			
		/* Calculate filter phase and average MMSE */
		rMMSE += TimeOptimalFilter(vecrTempFilt, iScatPilTimeInt, 
			iCurrDiffPhase,	rNewSNR, rNewSigma, Ts, iLengthWiener);

		/* Copy data from Matlib vector in regular vector */
		for (i = 0; i < iLengthWiener; i++)
			matrFiltTime[j][i] = vecrTempFilt[i];
	}

#if 0
#ifdef _DEBUG_
/* Save filter coefficients */
static FILE* pFile = fopen("test/wienertime.dat", "w");
for (i = 0; i < iLengthWiener; i++)
	for (j = 0; j < iNoFiltPhasTi; j++)
		fprintf(pFile, "%e\n", matrFiltTime[j][i]);
fflush(pFile);
#endif
#endif

	/* Normalize averaged MMSE */
	rMMSE /= iNoFiltPhasTi;

	return rMMSE;
}

CReal CTimeWiener::TimeOptimalFilter(CRealVector& vecrTaps, const int iTimeInt, 
									 const int iDiff, const CReal rNewSNR, 
									 const CReal rNewSigma, const CReal rTs, 
									 const int iLength)
{
	CRealVector	vecrReturn(iLength);
	CRealVector vecrRpp(iLength);
	CRealVector vecrRhp(iLength);
	CReal		rMMSE;

	int			i;
	CReal		rFactorArgExp;
	int			iCurPos;

	/* Factor for the argument of the exponetial function to generate the 
	   correlation function */
	rFactorArgExp = 
		(CReal) -2.0 * crPi * crPi * rTs * rTs * rNewSigma * rNewSigma;

	/* Doppler-spectrum for short-wave channel is Gaussian
	   (Calculation of R_hp!) */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iTimeInt + iDiff;

		vecrRhp[i] = exp(rFactorArgExp * iCurPos * iCurPos);
	}

	/* Doppler-spectrum for short-wave channel is Gaussian
	   (Calculation of R_pp!) */
	for (i = 0; i < iLength; i++)
	{
		iCurPos = i * iTimeInt;

		vecrRpp[i] = exp(rFactorArgExp * iCurPos * iCurPos);
	}

	/* Add SNR at first tap */
	vecrRpp[0] += (CReal) 1.0 / rNewSNR;

	/* Call levinson algorithm to solve matrix system for optimal solution */
	vecrTaps = Levinson(vecrRpp, vecrRhp);

	/* Calculate MMSE for the current wiener filter */
	rMMSE = (CReal) 1.0 - Sum(vecrRhp * vecrTaps);

	return rMMSE;
}

CReal CTimeWiener::ModLinRegr(CRealVector& vecrCorrEst)
{
	/* Modified linear regresseion to estimate the "sigma" of the Gaussian 
	   correlation function */
	/* Get vector length */
	int iVecLen = Size(vecrCorrEst);

	/* Init vectors and variables */
	CRealVector Tau(iVecLen);
	CRealVector Z(iVecLen);
	CRealVector W(iVecLen);
	CRealVector Wmrem(iVecLen);
	CReal		Wm, Zm;
	CReal		A1;

	/* Generate the tau vector */
	for (int i = 0; i < iVecLen; i++)
		Tau[i] = (CReal) (i * iScatPilTimeInt);

	/*
		% linearize acf equation:  y = a exp (-b x^2)
		%
		% z = ln y;   w = x^2
		%
		% -> z = a0 + a1 * w
	*/
	Z = Log(Abs(vecrCorrEst)); /* acfm can be negative due to noise */
	W = Tau * Tau;

	Wm = Mean(W);
	Zm = Mean(Z);

	Wmrem = W - Wm; /* Remove mean of W */

	A1 = Sum(Wmrem * (Z - Zm)) / Sum(Wmrem * Wmrem);

	return (CReal) 0.5 / crPi * sqrt((CReal) -2.0 * A1) / Ts;
}
