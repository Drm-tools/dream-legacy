/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Time synchronization
 * This module can have different amounts of input data. If two
 * possible FFT-window positions are found, the next time no new block is
 * requested.
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

#include "TimeSync.h"


/* Implementation *************************************************************/
void CTimeSync::ProcessDataInternal(CParameter& ReceiverParam)
{
	int				i, k;
	int				iMaxIndex;
	_REAL			rMaxValue;
	CVector<int>	iNewStartIndexField(5); // "5" for safety reasons. Could be "2"
	int				iNewStIndCount; 
	int				iIntDiffToCenter;
	int				iCurPos;
	_REAL			rCurCorrPowRes;
	int				iStartIndex;

	/* Write new block of data at the end of shift register */
	HistoryBuf.AddEnd((*pvecInputData), iInputBlockSize);

	if (bAquisition == TRUE)
	{
		/* Guard-interval correlation --------------------------------------- */
		/* Set position pointer back for this block */
		iTimeSyncPos -= iInputBlockSize;

		/* Init start-index count */
		iNewStIndCount = 0;

		/* We use the block in the middle of the buffer for observation */
		for (i = iSymbolBlockSize + iSymbolBlockSize - iInputBlockSize; 
			i < iSymbolBlockSize + iSymbolBlockSize; i++)
		{
			/* Only every "iStepSizeGuardCorr"'th value is calculated for
			   efficiency reasons */
			if (i == iTimeSyncPos)
			{
				/* Guard-interval correlation ------------------------------- */
				/* Speed optimized calculation of the guard-interval 
				   correlation. We devide the total block, which has to be 
				   computed, in parts of length "iStepSizeGuardCorr". The 
				   results of these blocks are stored in a vector. Now, only
				   one new part has to be calculated and one old one has to be
				   subtracted from the global result. Special care has to be
				   taken since "iGuardSize" must not be a multiple of 
				   "iStepSizeGuardCorr". Therefore the "if"-condition */
				/* First subtract correlation values shifted out */
				rGuardCorr -= vecrIntermCorrRes[iPosInIntermCResBuf];
				rGuardPow -= vecrIntermPowRes[iPosInIntermCResBuf];

				/* Calculate new block and add in memory */
				for (k = iLengthOverlap; k < iGuardSize; k++)
				{
					/* Actual correlation */
					iCurPos = iTimeSyncPos + k;
					rGuardCorrBlock += 
						HistoryBuf[iCurPos] * HistoryBuf[iCurPos + iDFTSize];

					rGuardPowBlock +=
						HistoryBuf[iCurPos] * HistoryBuf[iCurPos] + 
						HistoryBuf[iCurPos + iDFTSize] * 
						HistoryBuf[iCurPos + iDFTSize];

					/* If one complete block is ready -> store it. We need to 
					   add "1" to the k, because otherwise "iLengthOverlap" 
					   would satisfy the "if"-condition */
					if (((k + 1) % iStepSizeGuardCorr) == 0)
					{
						vecrIntermCorrRes[iPosInIntermCResBuf] = 
							rGuardCorrBlock;

						vecrIntermPowRes[iPosInIntermCResBuf] =
							rGuardPowBlock;

						/* Add the new block to the global result */
						rGuardCorr += rGuardCorrBlock;
						rGuardPow += rGuardPowBlock;

						/* Reset block result */
						rGuardCorrBlock = (_REAL) 0.0;
						rGuardPowBlock = (_REAL) 0.0;

						/* Increase position pointer and test if wrap */
						iPosInIntermCResBuf++;
						if (iPosInIntermCResBuf == iLengthIntermCRes)
							iPosInIntermCResBuf = 0;
					}
				}

				/* Add additional part to the global result to get current 
				   result (ML solution) */
				rCurCorrPowRes = fabs(rGuardCorr + rGuardCorrBlock) - 
					(rGuardPow + rGuardPowBlock) / 2;


				/* Energy of guard-interval correlation --------------------- */
				/* Optimized calculation of the guard-interval energy. We only
				   add a new value und subtract the old value from the result. 
				   We only need one addition and a history buffer */
				/* Subtract oldest value */
				rGuardEnergy -= pMovAvBuffer[iPosInMovAvBuffer];

				/* Add new value and write in memory */
				rGuardEnergy += rCurCorrPowRes;
				pMovAvBuffer[iPosInMovAvBuffer] = rCurCorrPowRes;

				/* Increase position pointer and test if wrap */
				iPosInMovAvBuffer++;
				if (iPosInMovAvBuffer == iMovAvBufSize)
					iPosInMovAvBuffer = 0;


				/* Detection buffer ----------------------------------------- */
				/* Update buffer for storing the moving average results */
				pMaxDetBuffer.AddEnd(rGuardEnergy);

				/* Search for maximum */
				iMaxIndex = 0;
				rMaxValue = (_REAL) -3.4e38; // Init value
				for (k = 0; k < iMaxDetBufSize; k++)
				{
					if (pMaxDetBuffer[k] > rMaxValue)
					{
						rMaxValue = pMaxDetBuffer[k];
						iMaxIndex = k;
					}
				}

				/* If maximum is in the middle of the interval, mark position
				   as the beginning of the FFT window */
				if (iMaxIndex == iCenterOfMaxDetBuf)
				{
					/* The optimal start position for the FFT-window is the
					   middle of the "MaxDetBuffer" (iTimeSyncPos - 
					   iSymbolBlockSize / 2) plus a correction term for our
					   method (iStepSizeGuardCorr) */
					iNewStartIndexField[iNewStIndCount] = iTimeSyncPos - 
						iSymbolBlockSize / 2 + iStepSizeGuardCorr;

					iNewStIndCount++;
				}

				/* Set position pointer to next step */
				iTimeSyncPos += iStepSizeGuardCorr;
			}
		}

		/* Use all measured FFT-window start points for determining the "real" 
		   one */	
		for (i = 0; i < iNewStIndCount; i++)
		{
			/* Check if new measurement is in range of predefined bound. This 
			   bound shall eliminate outliers for the calculation of the 
			   filtered result */
			if (((iNewStartIndexField[i] < (iCenterOfBuf + TIMING_BOUND_ABS)) &&
				(iNewStartIndexField[i] > (iCenterOfBuf - TIMING_BOUND_ABS))))
			{
				/* New measurement is in range -> use it for filtering */
				/* Low-pass filter detected start of frame */
				rStartIndex = LAMBDA_LOW_PASS_START * rStartIndex + 
					(1 - LAMBDA_LOW_PASS_START) * (_REAL) iNewStartIndexField[i];

				/* Reset counters for non-linear correction algorithm */
				iCorrCounter = 0;
				iAveCorr = 0;

				/* GUI message that timing is ok */
				PostWinMessage(MS_TIME_SYNC, 0);
			}
			else
			{
				/* Non-linear correction of the filter-output to ged rid of 
				   large differences between current measurement and 
				   filter-output */
				iCorrCounter++;

				/* Average the NO_SYM_BEFORE_RESET measurements for reset
				   rStartIndex */
				iAveCorr += iNewStartIndexField[i];

				/* If pre-defined number of outliers is exceed, correct */
				if (iCorrCounter > NO_SYM_BEFORE_RESET)
				{
					/* Correct filter-output */
					rStartIndex = (_REAL) iAveCorr / (NO_SYM_BEFORE_RESET + 1);

					/* Reset counter */
					iCorrCounter = 0;
					iAveCorr = 0;

					/* GUI message that timing was corrected (red light) */
					PostWinMessage(MS_TIME_SYNC, 2);
				}
				else
				{
					/* GUI message that timing is yet ok (yellow light) */
					PostWinMessage(MS_TIME_SYNC, 1);
				}
			}

#ifdef _DEBUG_
/* Plot estimated positions of timing */
static _REAL rTempAccuStartInd = (_REAL) 0.0;
rTempAccuStartInd -= (int) (iCenterOfBuf - (int) rStartIndex);
static FILE* pFile = fopen("test/testtime.dat", "w");
fprintf(pFile, "%e %d %d\n", rTempAccuStartInd, iNewStartIndexField[i], iInputBlockSize);
fflush(pFile);
#endif
		}

		/* Convert result to integer format for cutting out the FFT-window */
		iStartIndex = (int) rStartIndex;
	}
	else
	{
		/* Detect situation when acquisition was deactivated right now */
		if (bAcqWasActive == TRUE)
		{
			bAcqWasActive = FALSE;

			/* Reset also the tracking value since the tracking could not get right
			   parameters since the timing was not yet correct */
			ReceiverParam.rTimingOffsTrack = (_REAL) 0.0;
		}

		/* In case of tracking only, use final acquisition result "rStartIndex"
		   (which is not updated any more) and add tracking correction */
		iStartIndex = (int) rStartIndex + (int) ReceiverParam.rTimingOffsTrack;
	}


	/* -------------------------------------------------------------------------
	   Cut out the estimated optimal time window and write it to the output
	   vector. Add the acquisition and tracking offset together for the final
	   timing */

	/* Check range of "iStartIndex" to prevent from vector overwrites. It must 
	   be larger than "0" since then the input block size would be also "0" and
	   than the processing routine of the modul would not be called any more */
	if (iStartIndex <= 0)
		iStartIndex = 1;
	if (iStartIndex >= iSymbolBlockSize + iSymbolBlockSize)
		iStartIndex = iSymbolBlockSize + iSymbolBlockSize;

	for (k = iStartIndex; k < iStartIndex + iDFTSize; k++)
		(*pvecOutputData)[k - iStartIndex] = HistoryBuf[k];

	/* If synchronized DRM input stream is used, overwrite the detected
	   timing */
	if (bSyncInput == TRUE)
	{
		/* Set fixed timing position */
		iStartIndex = iSymbolBlockSize; 
		/* Cut out guard-interval at right position -> no channel estimation 
		   needed when having only one path. No delay introduced in this 
		   module  */
		for (k = iGuardSize; k < iSymbolBlockSize; k++)
			(*pvecOutputData)[k - iGuardSize] = 
				HistoryBuf[iTotalBufferSize - iInputBlockSize + k];
	}


	/* -------------------------------------------------------------------------
	   Adjust filtered measurement so that it is back in the center of the 
	   buffer */
	/* Integer part of the difference between filtered measurement and the
	   center of the buffer */
	iIntDiffToCenter = iCenterOfBuf - iStartIndex;

	/* Set input block size for next block and reset filtered measurement. This
	   is equal to a global shift of the observation window by the 
	   rearrangement of the filtered measurement */
	iInputBlockSize = iSymbolBlockSize - iIntDiffToCenter;

	/* In case of tracking the tracking offset must be reset since the 
	   acquisition result is constant after switching to tracking */
	if (bAquisition == TRUE)
		rStartIndex += (_REAL) iIntDiffToCenter;
	else
		ReceiverParam.rTimingOffsTrack += (_REAL) iIntDiffToCenter;


	/* -------------------------------------------------------------------------
	   The channel estimation needs information about timing corrections, 
	   because it is using information from the symbol memory. After a 
	   timing correction all old symbols must be corrected by a phase
	   rotation as well */
	(*pvecOutputData).GetExData().iCurTimeCorr = iIntDiffToCenter;
}

void CTimeSync::InitInternal(CParameter& ReceiverParam)
{
	/* Length of guard-interval measured in "time-bins" */
	iGuardSize = ReceiverParam.iGuardSize;
	iDFTSize = ReceiverParam.iFFTSizeN;

	/* Symbol block size is the guard-interval plus the useful part */
	iSymbolBlockSize = ReceiverParam.iSymbolBlockSize;

	/* We need at least two blocks of data for determining the timing */
	iTotalBufferSize = 3 * iSymbolBlockSize;

	/* Set step size of the guard-interval correlation */
	iStepSizeGuardCorr = STEP_SIZE_GUARD_CORR;

	/* Size for moving average buffer for guard-interval correlation */
	iMovAvBufSize = (int) (iGuardSize / iStepSizeGuardCorr);

	/* Size of buffer, storing the moving-average results for 
	   maximum detection */
	iMaxDetBufSize = (int) (iSymbolBlockSize / iStepSizeGuardCorr);

	/* Center of maximum detection buffer */
	iCenterOfMaxDetBuf = (iMaxDetBufSize - 1) / 2;

	/* Init Energy calculation after guard-interval correlation */
	rGuardEnergy = (_REAL) 0.0;
	iPosInMovAvBuffer = 0;

	/* Start position of this value must be at the end of the observation
       window because we reset it at the beginning of the loop */
	iTimeSyncPos = 2 * iSymbolBlockSize;

	/* Calculate center of buffer */
	iCenterOfBuf = iSymbolBlockSize;

	/* Init rStartIndex only if acquisition was activated */
	if (bAquisition == TRUE)
		rStartIndex = (_REAL) iCenterOfBuf;

	/* Some inits */
	/* Set correction counter to limit to get a non-linear correction 
	   the first time of a new acquisition block */
	iCorrCounter = NO_SYM_BEFORE_RESET;
	iAveCorr = 0;

	/* Allocate memory for vectors and zero out */
	HistoryBuf.Init(iTotalBufferSize, (_REAL) 0.0);
	pMovAvBuffer.Init(iMovAvBufSize, (_REAL) 0.0);
	pMaxDetBuffer.Init(iMaxDetBufSize, (_REAL) 0.0);


	/* Inits for guard-interval correlation --------------------------------- */
	rGuardCorr = (_REAL) 0.0;
	rGuardPow = (_REAL) 0.0;
	rGuardCorrBlock = (_REAL) 0.0;
	rGuardPowBlock = (_REAL) 0.0;
	iPosInIntermCResBuf = 0;

	/* Number of correlation result blocks to be stored in a vector. This is the
	   total length of the guard-interval divided by the step size. Since the
	   guard-size must not be a multiple of "iStepSizeGuardCorr", we need to 
	   cut-off the fractional part */
	iLengthIntermCRes = (int) (iGuardSize / iStepSizeGuardCorr);

	/* Intermediate correlation results vector (init, zero out) */
	vecrIntermCorrRes.Init(iLengthIntermCRes, (_REAL) 0.0);
	vecrIntermPowRes.Init(iLengthIntermCRes, (_REAL) 0.0);

	/* This length is the start point for the "for"-loop */
	iLengthOverlap = iGuardSize - iStepSizeGuardCorr;

	/* Define block-sizes for input and output */
	iInputBlockSize = iSymbolBlockSize; // For the first loop
	iOutputBlockSize = iDFTSize;
}

void CTimeSync::StartAcquisition() 
{
	/* The regular acquisition flag */
	bAquisition = TRUE;

	/* Set the init flag so that the "rStartIndex" can be initialized with the
	   center of the buffer */
	SetInitFlag();
	
	/* This second flag is to determine the moment when the acquisition just
	   finished. In this case, the tracking value must be reset */
	bAcqWasActive = TRUE;

	/* Set correction counter so that a non-linear correction is performed right
	   at the beginning */
	iCorrCounter = NO_SYM_BEFORE_RESET;
}
