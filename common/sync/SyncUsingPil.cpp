/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Frame synchronization (Using time-pilots), frequency sync tracking
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

#include "SyncUsingPil.h"


/* Implementation *************************************************************/
void CSyncUsingPil::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i;
	_REAL		rTimePilotCorr;
	int			iMinIndex;
	_REAL		rMinValue;
	_REAL		rSampFreqOffsetEst;
	_COMPLEX	cOldFreqPilCorr;
	_COMPLEX	cFreqOffsetEstTemp;
	_REAL		rFreqOffsetEst;


	/**************************************************************************\
	* Frame synchronization detection										   *
	\**************************************************************************/
	if ((bSyncInput == FALSE) && (bAquisition == TRUE))
	{
		/* We use a differential demodulation of the time-pilots, because there
		   is no channel-estimation done, yet. The differential factors are pre-
		   calculated in the Init-routine. Additionally, the index of the first
		   pilot in the pair is stored in ".iNoCarrier". We calculate and 
		   averaging the Euclidean-norm of the resulting complex values */
		rTimePilotCorr = (_REAL) 0.0;
		for (i = 0; i < iNoDiffFact; i++)
		{
			_COMPLEX cErrVec = ((*pvecInputData)[vecDiffFact[i].iNoCarrier] *
				vecDiffFact[i].cDiff - 
				(*pvecInputData)[vecDiffFact[i].iNoCarrier + 1]);

			/* Add squard magnitude of error vector */
			rTimePilotCorr += SqMag(cErrVec);
		}

		/* Store correlation results in a shift register for finding the peak */
		vecrCorrHistory.AddEnd(rTimePilotCorr);

		/* Search for minimum distance. Init value with a high number */
		iMinIndex = 0;
		rMinValue = (_REAL) HI_VALUE_FOR_MIN_SEARCH; 
		for (i = 0; i < iNoSymPerFrame; i++)
		{
			if (vecrCorrHistory[i] < rMinValue)
			{
				rMinValue = vecrCorrHistory[i];
				iMinIndex = i;
			}
		}

		/* If minimum is in the middle of the interval -> check frame sync */
		if (iMinIndex == iMiddleOfInterval)
		{
			if (iSymbCntFraSy == iNoSymPerFrame - iMiddleOfInterval - 1)
			{
				/* Reset flag */
				bBadFrameSync = FALSE;

				/* Post Message for GUI (Good frame sync) */
				PostWinMessage(MS_FRAME_SYNC, 0);
			}
			else
			{
				if (bBadFrameSync == TRUE)
				{
					/* Reset symbol counter according to received data */
					iSymbCntFraSy = iNoSymPerFrame - iMiddleOfInterval - 1;

					/* Reset flag */
					bBadFrameSync = FALSE;

					/* Post Message for GUI for bad frame sync (red light) */
					PostWinMessage(MS_FRAME_SYNC, 2);
				}
				else
				{
					/* One false detected frame sync should not reset the actual
					   frame sync because the measurement could be wrong. 
					   Sometimes the frame sync detection gets false results. If
					   the next time the frame sync is still unequal to the 
					   measurement, then correct it */
					bBadFrameSync = TRUE;

					/* Post Message that frame sync was wrong but was not yet
					   corrected (yellow light) */
					PostWinMessage(MS_FRAME_SYNC, 1);
				}
			}
		}
	}

	/* Set current symbol number in extended data of output vector */
	(*pvecOutputData).GetExData().iSymbolNo = iSymbCntFraSy;

	/* Increase symbol counter and take care of wrap around */
	iSymbCntFraSy++;
	if (iSymbCntFraSy >= iNoSymPerFrame)
		iSymbCntFraSy = 0;


	/**************************************************************************\
	* Using Frequency pilot information										   *
	\**************************************************************************/
	if ((bSyncInput == FALSE) && (bTrackPil == TRUE))
	{
		cFreqOffsetEstTemp = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
		for (i = 0; i < NO_FREQ_PILOTS; i++)
		{
			/* The old pilots must be rotated due to timing corrections */
			cOldFreqPilCorr = Rotate(cOldFreqPil[i], iPosFreqPil[i], 
				-(*pvecInputData).GetExData().iCurTimeCorr);

			/* Get phase difference */
			rFreqPilotPhDiff[i] = 
				arg((*pvecInputData)[iPosFreqPil[i]] * conj(cOldFreqPilCorr));

			/* Calculate estimation of frequency offset */
			cFreqOffsetEstTemp += 
				(*pvecInputData)[iPosFreqPil[i]] * conj(cOldFreqPilCorr);

			/* Save "old" frequency pilots for next symbol. Special treatment
			   for robustness mode D (carriers 7 and 21) necessary 
			   (See 8.4.2.2) */
			if ((ReceiverParam.GetWaveMode() == RM_ROBUSTNESS_MODE_D) && 
				(i < 2))
			{
				cOldFreqPil[i] = -(*pvecInputData)[iPosFreqPil[i]];
			}
			else
				cOldFreqPil[i] = (*pvecInputData)[iPosFreqPil[i]];
		}

		/* Calculate frequency offset */
		rFreqOffsetEst = arg(cFreqOffsetEstTemp) * rNormConstFOE;

		/* Calculate estimation of sample frequency offset. We use the different
		   frequency offset estimations of the frequency pilots. We normalize 
		   them with the distance between them and average the result (/ 2.0) */
		rSampFreqOffsetEst = ((rFreqPilotPhDiff[1] - rFreqPilotPhDiff[0]) / 
			(iPosFreqPil[1] - iPosFreqPil[0]) +	
			(rFreqPilotPhDiff[2] - rFreqPilotPhDiff[0]) / 
			(iPosFreqPil[2] - iPosFreqPil[0])) / (_REAL) 2.0;


		/* Using estimated parameters for controlling ----------------------- */
		/* Integrate the result for controling the frequency offset */
		ReceiverParam.rFreqOffsetTrack += 
			CONTR_FREQ_OFF_INTEGRATION * rFreqOffsetEst;

		/* Integrate the result for controling the resampling */
		ReceiverParam.rResampleOffset += 
			CONTR_SAMP_OFF_INTEGRATION * rSampFreqOffsetEst;
	}


	/* If synchronized DRM input stream is used, overwrite the detected
	   frequency offest estimate by "0", because we know this value */
	if (bSyncInput == TRUE)
		ReceiverParam.rFreqOffsetTrack = (_REAL) 0.0;	


	/* Copy data from input to the output. Data is not modified in this 
	   module */
	for (i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = (*pvecInputData)[i];
}

void CSyncUsingPil::InitInternal(CParameter& ReceiverParam)
{
	int			i;
	int			iTotalNoUsefCarr;
	_COMPLEX	cPhaseCorTermDivi;
	_REAL		rArgumentTemp;

	/* Init base class for modifying the pilots (rotation) */
	CPilotModiClass::InitRot(ReceiverParam);

	/* Init internal parameters from global struct */
	iTotalNoUsefCarr = ReceiverParam.iNoCarrier;

	/* Check if symbol number per frame has changed. If yes, reset the 
	   symbol counter */
	if (iNoSymPerFrame != ReceiverParam.iNoSymPerFrame)
	{
		/* Init internal counter for symbol number */
		iSymbCntFraSy = 0;

		/* Refresh parameter */
		iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;
	}

	/* After an initialization the frame sync must be adjusted */
	bBadFrameSync = TRUE;

	/* Allocate memory for storing differential complex factors. Since we do 
	   not know the resulting "iNoDiffFact" we allocate memory for the 
	   worst case, i.e. "iTotalNoUsefCarr" */
	vecDiffFact.Init(iTotalNoUsefCarr);

	/* Calculate differential complex factors for time-synchronization pilots.
	   Use only first symbol of "matcPilotCells", because there are the pilots
	   for Frame-synchronization */
	iNoDiffFact = 0;
	for (i = 0; i < iTotalNoUsefCarr - 1; i++)
	{
		/* Only successive pilots (in frequency direction) are used */
		if (_IsPilot(ReceiverParam.matiMapTab[0][i]) &&
			_IsPilot(ReceiverParam.matiMapTab[0][i + 1]))
		{
			/* Store index of first pilot of the couple */
			vecDiffFact[iNoDiffFact].iNoCarrier = i;

			/* Calculate phase correction term. This term is needed, because the
			   desired position of the main peak (line of sight) is the middle
			   of the guard-interval */
			rArgumentTemp = (_REAL) 2.0 * crPi / ReceiverParam.iFFTSizeN *
				ReceiverParam.iGuardSize / 2;
			cPhaseCorTermDivi = 
				_COMPLEX(cos(rArgumentTemp), -sin(rArgumentTemp));

			/* Calculate differential factor */
			vecDiffFact[iNoDiffFact].cDiff = 
				ReceiverParam.matcPilotCells[0][i + 1] / 
				ReceiverParam.matcPilotCells[0][i] * cPhaseCorTermDivi;

			iNoDiffFact++;
		}
	}

	/* Set middle of observation interval */
	iMiddleOfInterval = iNoSymPerFrame / 2;

	/* Get position of frequency pilots */
	int iFreqPilCount = 0;
	for (i = 0; i < iTotalNoUsefCarr - 1; i++)
	{
		if (_IsFreqPil(ReceiverParam.matiMapTab[0][i]))
		{
			iPosFreqPil[iFreqPilCount] = i;
			iFreqPilCount++;
		}
	}

	/* Init memory for "old" frequency pilots and actual phase differences */
	for (i = 0; i < NO_FREQ_PILOTS; i++)
	{
		cOldFreqPil[i] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
		rFreqPilotPhDiff[i] = (_REAL) 0.0;
	}
	
	/* Nomalization constant for frequency offset estimation */
	rNormConstFOE = 
		(_REAL) 1.0 / ((_REAL) 2.0 * crPi * ReceiverParam.iSymbolBlockSize);

	/* Allocate memory for histories. Init history with large values, because
	   we search for minimum! */
	vecrCorrHistory.Init(iNoSymPerFrame, (_REAL) HI_VALUE_FOR_MIN_SEARCH);

	/* Define block-sizes for input and output */
	iInputBlockSize = iTotalNoUsefCarr;
	iOutputBlockSize = iTotalNoUsefCarr;
}

void CSyncUsingPil::StartAcquisition() 
{
	/* Init internal counter for symbol number */
	iSymbCntFraSy = 0;

	bAquisition = TRUE;
}
