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
void CTimeWiener::Estimate(CVectorEx<_COMPLEX>* pvecInputData, 
						   CComplexVector& veccOutputData, 
						   CVector<int>& veciMapTab, 
						   CVector<_COMPLEX>& veccPilotCells)
{
	int			j, i;
	int			iPiHiIndex;
	int			iCurrFiltPhase;
	int			iTimeDiffNew;
	_COMPLEX	cNewPilot;

	/* Timing correction history -------------------------------------------- */
	/* Shift old vaules and add a "0" at the beginning of the vector */
	vecTiCorrHist.AddBegin(0);

	/* Add new one to all history values except of the current one */
	for (i = 1; i < iLenTiCorrHist; i++)
		vecTiCorrHist[i] += (*pvecInputData).GetExData().iCurTimeCorr;


	/* Update histories for channel estimates at the pilot positions -------- */
	for (i = 0; i < iNoCarrier; i++)
	{
		/* Identify and calculate transfer function at the pilot positions */
		if (veciMapTab[i] & CM_SCAT_PI)
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


#ifdef DO_WIENER_TIME_FILT_UPDATE
			/* Estimation of the channel correlation function --------------- */
			for (j = 0; j < iLengthWiener; j++)
			{
				/* Correct pilot information for phase rotation */
				iTimeDiffNew = -vecTiCorrHist[iScatPilTimeInt * j];
				cNewPilot = 
					Rotate(matcChanAtPilPos[j][iPiHiIndex], i, iTimeDiffNew);

				/* Simply add all results together and increment count */
				vecrTiCorrEst[j] += 
					real(conj(matcChanAtPilPos[0][iPiHiIndex]) * cNewPilot);

				iAvCntSigmaEst++;
			}
#endif
		}
	}


#ifdef DO_WIENER_TIME_FILT_UPDATE
	/* Update filter coefficiants if estimation is ready -------------------- */
	if (iAvCntSigmaEst >= NO_SIGMA_AVER_UNTIL_USE)
	{
		/* Use a threshold to exclude noisy estimates */
		for (i = 1; i < iLengthWiener; i++)
			if (vecrTiCorrEst[i] < vecrTiCorrEst[0] / 10)
				vecrTiCorrEst[i] = (_REAL) 0.0;
	
		/* Update the wiener filter */
		UpdateFilterCoef(rSNR, ModLinRegr(vecrTiCorrEst));

		/* Reset counter */
		iAvCntSigmaEst = 0;

		/* Reset estimation vector */
		vecrTiCorrEst.Reset((_REAL) 0.0);
	}
#endif


	/* Wiener interpolation, filtering and prediction ----------------------- */
	for (i = 0; i < iNoCarrier; i += iScatPilFreqInt)
	{
		/* This check is for robustness mode D sinc "iScatPilFreqInt" is "1"
		   in this case which would include the DC carrier in the for-loop */
		if (!(veciMapTab[i] & CM_DC))
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
	int i, j;
	int iNoPiFreqDirAll;
	int iSymDelyChanEst;
	int iNoTapsFilterPhase;

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
		iNoTapsFilterPhase = 20;
		rSigma = (_REAL) 2.0 / 2;
		break;

	case RM_ROBUSTNESS_MODE_B:
		iNoTapsFilterPhase = 15;
		rSigma = (_REAL) 3.36 / 2;
		break;
	
	case RM_ROBUSTNESS_MODE_C:
		iNoTapsFilterPhase = 9;
		rSigma = (_REAL) 6.73 / 2;
		break;
	
	case RM_ROBUSTNESS_MODE_D:
		iNoTapsFilterPhase = 9;
		rSigma = (_REAL) 5.38 / 2;
		break;
	}

	/* Set delay of this channel estimation type. The longer the delay is, the
	   more "acausal" pilots can be used for interpolation */
	iSymDelyChanEst = 2 * iScatPilTimeInt - 1;

	/* Length of wiener filter */
	iLengthWiener = iNoTapsFilterPhase;

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

	/* Init vector for estimation the correlation function in time direction */
	vecrTiCorrEst.Init(iLengthWiener, (_REAL) 0.0);

	/* Init average counter for sigma estimation */
	iAvCntSigmaEst = 0;

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
	while (!(ReceiverParam.matiMapTab[iFirstSymbWithPi][0] & CM_SCAT_PI))
		iFirstSymbWithPi++;


	/* Calculate optimal filter --------------------------------------------- */
	/* Significant parameters for Gaussian power density spectrum */
	const _REAL rSNRdB = (_REAL) 25.0;
	rSNR = pow(10, rSNRdB / 10);

	/* Update filter coefficiants */
	UpdateFilterCoef(rSNR, rSigma);

	/* Return delay of channel equalization */
	return iLenHistBuff;
}

void CTimeWiener::UpdateFilterCoef(_REAL rNewSNR, _REAL rNewSigma)
{
	int i, j;
	int iCurrDiffPhase;

	/* Vector for intermedia result */
	CRealVector vecrTempFilt(iLengthWiener);

	/* One filter for all possible filter phases */
	for (j = 0; j < iNoFiltPhasTi; j++)
	{
		/* We have to define the dependency between the difference between the
		   current pilot to the observed symbol in the history buffer and the
		   indizes of the FiltTime array. Definition:
		   Largest distance = index zero, index increases to smaller 
		   distances */
		iCurrDiffPhase = -(iLenHistBuff - j - 1);
			
		/* Calculate filter phase */
		vecrTempFilt = TimeOptimalFilter(iScatPilTimeInt, iCurrDiffPhase,
			rNewSNR, rNewSigma, Ts, iLengthWiener);

		/* Copy data from Matlib vector in regular vector */
		for (i = 0; i < iLengthWiener; i++)
			matrFiltTime[j][i] = vecrTempFilt[i];
	}
}

CRealVector CTimeWiener::TimeOptimalFilter(int iTimeInt, int iDiff, 
										   _REAL rNewSNR, _REAL rNewSigma, 
										   _REAL rTs, int iLength)
{
	CRealVector	vecrReturn(iLength);
	CRealVector vecrRpp(iLength);
	CRealVector vecrRhp(iLength);

	int			i;
	_REAL		rFactorArgExp;
	int			iCurPos;

	/* Factor for the argument of the exponetial function to generate the 
	   correlation function */
	rFactorArgExp = 
		(_REAL) -2.0 * crPi * crPi * rTs * rTs * rNewSigma * rNewSigma;

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
	vecrRpp[0] += (_REAL) 1.0 / rNewSNR;

	/* Call levinson algorithm to solve matrix system for optimal solution */
	vecrReturn = Levinson(vecrRpp, vecrRhp);

	return vecrReturn;
}

_REAL CTimeWiener::ModLinRegr(CVector<_REAL>& vecrCorrEst)
{
	/* Modified linear regresseion to estimate the "sigma" of the Gaussian 
	   correlation function */
	int		i, iVecLen;
	_REAL	rXAv, rYAv;
	_REAL	rSumEnum, rSumDenom;

	/* Get vector length */
	iVecLen = vecrCorrEst.Size();

	/* Average of x */
	rXAv = (_REAL) 0.0;
	for (i = 0; i < iVecLen * iScatPilTimeInt; i += iScatPilTimeInt)
		rXAv += i * i;
	rXAv /= iVecLen;

	/* Average of y */
	rYAv = (_REAL) 0.0;
	for (i = 0; i < iVecLen; i++)
		if (vecrCorrEst[i] > (_REAL) 0.0)
			rYAv += log(fabs(vecrCorrEst[i]));
	rYAv /= iVecLen;

	/* Modified linear regression */
	rSumEnum = (_REAL) 0.0;
	rSumDenom = (_REAL) 0.0;
	for (i = 0; i < iVecLen; i++)
	{
		if (vecrCorrEst[i] > (_REAL) 0.0)
		{
			int iSquaredTiIn = i * i * iScatPilTimeInt * iScatPilTimeInt;

			rSumEnum += 
				(iSquaredTiIn - rXAv) * (log(fabs(vecrCorrEst[i])) - rYAv);
			rSumDenom += 
				(iSquaredTiIn - rXAv) * (iSquaredTiIn - rXAv);
		}
	}

	/* Normalize result */
	if (rSumEnum / rSumDenom > 0)
		return (_REAL) 0.0;
	else
		return (_REAL) 0.5 / crPi * 
			sqrt((_REAL) -2.0 * rSumEnum / rSumDenom) / Ts;
}
