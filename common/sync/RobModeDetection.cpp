/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Robustness-mode detection
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

#include "RobModeDetection.h"


/* Implementation *************************************************************/
void CRobModDet::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i, k;
	_REAL		rGuardCorr;
	_REAL		rGuardPow;
	CReal		rResModeA;
	CReal		rResModeB;
	CReal		rResModeC;
	CReal		rResModeD;
	_REAL		rMaxValue;
	_REAL		rSecondPeak;
	ERobMode	eDetectedMode;

	if (bAquisition == TRUE)
	{
		/* Add new symbol in history (shift register) */
		vecrHistory.AddEnd((*pvecInputData), iInputBlockSize);

		/* We use the block in the middle of the buffer for investigation */
		for (i = 0; i < iInputBlockSize; i++)
		{
			/* Only every STEP_SIZE_GUARD_CORR_RMD'th value is calculated for
			   efficiency reasons */
			if (i == iTimeSyncPos)
			{
				/* Robustness mode A ---------------------------------------- */
				rGuardCorr = (_REAL) 0.0;
				rGuardPow = (_REAL) 0.0;
				for (k = 0; k < iGuardSizeA; k++)
				{
					rGuardCorr += vecrHistory[iTimeSyncPos + k] * 
						vecrHistory[iTimeSyncPos + k + RMA_FFT_SIZE_N];

					rGuardPow += vecrHistory[iTimeSyncPos + k] *
						vecrHistory[iTimeSyncPos + k] +
						vecrHistory[iTimeSyncPos + k + RMA_FFT_SIZE_N] *
						vecrHistory[iTimeSyncPos + k + RMA_FFT_SIZE_N];
				}

				/* Store result in shift register */
				for (k = 0; k < iFFTBufSize - 1; k++)
					vecrFFTBufferModA[k] = vecrFFTBufferModA[k + 1];

				vecrFFTBufferModA[iFFTBufSize - 1] =
					fabs(rGuardCorr) - rGuardPow / 2;


				/* Robustness mode B ---------------------------------------- */
				rGuardCorr = (_REAL) 0.0;
				rGuardPow = (_REAL) 0.0;
				for (k = 0; k < iGuardSizeB; k++)
				{
					rGuardCorr += vecrHistory[iTimeSyncPos + k] * 
						vecrHistory[iTimeSyncPos + k + RMB_FFT_SIZE_N];

					rGuardPow += vecrHistory[iTimeSyncPos + k] *
						vecrHistory[iTimeSyncPos + k] +
						vecrHistory[iTimeSyncPos + k + RMB_FFT_SIZE_N] *
						vecrHistory[iTimeSyncPos + k + RMB_FFT_SIZE_N];
				}

				/* Store result in shift register */
				for (k = 0; k < iFFTBufSize - 1; k++)
					vecrFFTBufferModB[k] = vecrFFTBufferModB[k + 1];

				vecrFFTBufferModB[iFFTBufSize - 1] =
					fabs(rGuardCorr) - rGuardPow / 2;


				/* Robustness mode C ---------------------------------------- */
				rGuardCorr = (_REAL) 0.0;
				rGuardPow = (_REAL) 0.0;
				for (k = 0; k < iGuardSizeC; k++)
				{
					rGuardCorr += vecrHistory[iTimeSyncPos + k] * 
						vecrHistory[iTimeSyncPos + k + RMC_FFT_SIZE_N];

					rGuardPow += vecrHistory[iTimeSyncPos + k] *
						vecrHistory[iTimeSyncPos + k] +
						vecrHistory[iTimeSyncPos + k + RMC_FFT_SIZE_N] *
						vecrHistory[iTimeSyncPos + k + RMC_FFT_SIZE_N];
				}

				/* Store result in shift register */
				for (k = 0; k < iFFTBufSize - 1; k++)
					vecrFFTBufferModC[k] = vecrFFTBufferModC[k + 1];

				vecrFFTBufferModC[iFFTBufSize - 1] =
					fabs(rGuardCorr) - rGuardPow / 2;


				/* Robustness mode D ---------------------------------------- */
				rGuardCorr = (_REAL) 0.0;
				rGuardPow = (_REAL) 0.0;
				for (k = 0; k < iGuardSizeD; k++)
				{
					rGuardCorr += vecrHistory[iTimeSyncPos + k] * 
						vecrHistory[iTimeSyncPos + k + RMD_FFT_SIZE_N];

					rGuardPow += vecrHistory[iTimeSyncPos + k] *
						vecrHistory[iTimeSyncPos + k] +
						vecrHistory[iTimeSyncPos + k + RMD_FFT_SIZE_N] *
						vecrHistory[iTimeSyncPos + k + RMD_FFT_SIZE_N];
				}

				/* Store result in shift register */
				for (k = 0; k < iFFTBufSize - 1; k++)
					vecrFFTBufferModD[k] = vecrFFTBufferModD[k + 1];

				vecrFFTBufferModD[iFFTBufSize - 1] =
					fabs(rGuardCorr) - rGuardPow / 2;


				/* Set position pointer to next step */
				iTimeSyncPos += STEP_SIZE_GUARD_CORR_RMD;
			}
		}

		/* Set position pointer back for the next block */
		iTimeSyncPos -= iInputBlockSize;


		/* Correlation with symbol rate frequency --------------------------- */
		/* Guard-interval correlation (Correlations must be normalized 
		   to be comparable! ("/ iGuardSizeX")) */
		/* Robustness mode A */
		rResModeA = Abs(Sum(vecrFFTBufferModA * vecrCosA)) / iGuardSizeA;

		/* Robustness mode B */
		rResModeB = Abs(Sum(vecrFFTBufferModB * vecrCosB)) / iGuardSizeB;

		/* Robustness mode C */
		rResModeC = Abs(Sum(vecrFFTBufferModC * vecrCosC)) / iGuardSizeC;

		/* Robustness mode D */
		rResModeD = Abs(Sum(vecrFFTBufferModD * vecrCosD)) / iGuardSizeD;


		/* Search for maximum ----------------------------------------------- */
		rMaxValue = (_REAL) 0.0;
		if (rResModeA > rMaxValue)
		{
			rMaxValue = rResModeA;
			eDetectedMode = RM_ROBUSTNESS_MODE_A;
		}
		if (rResModeB > rMaxValue)
		{
			rMaxValue = rResModeB;
			eDetectedMode = RM_ROBUSTNESS_MODE_B;
		}
		if (rResModeC > rMaxValue)
		{
			rMaxValue = rResModeC;
			eDetectedMode = RM_ROBUSTNESS_MODE_C;
		}
		if (rResModeD > rMaxValue)
		{
			rMaxValue = rResModeD;
			eDetectedMode = RM_ROBUSTNESS_MODE_D;
		}

		/* Get reliable measure (distance to next peak) */
		/* Get second highest peak */
		rSecondPeak = (_REAL) 0.0;
		if ((rResModeA > rSecondPeak) && (eDetectedMode != RM_ROBUSTNESS_MODE_A))
			rSecondPeak = rResModeA;
		
		if ((rResModeB > rSecondPeak) && (eDetectedMode != RM_ROBUSTNESS_MODE_B))
			rSecondPeak = rResModeB;
		
		if ((rResModeC > rSecondPeak) && (eDetectedMode != RM_ROBUSTNESS_MODE_C))
			rSecondPeak = rResModeC;
		
		if ((rResModeD > rSecondPeak) && (eDetectedMode != RM_ROBUSTNESS_MODE_D))
			rSecondPeak = rResModeD;

		/* If we have a reliable measurement, stop aquisition and set new 
		   mode */
		if ((rMaxValue / rSecondPeak) > THRESHOLD_RELI_MEASURE)
		{
			/* Reset aquisition flag */
			bAquisition = FALSE;

			/* Set wave mode */
			if (ReceiverParam.SetWaveMode(eDetectedMode))
			{
				/* Reset output cyclic-buffer because wave mode has changed and
				   the data written in the buffer is not valid any more */
				SetBufReset1();
			}
		}
	}

	/* Copy data from input to the output. Data is not modified in this 
	   module */
	for (i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = (*pvecInputData)[i];
}

void CRobModDet::InitInternal(CParameter& ReceiverParam)
{
	int		iMaxSymbolBlockSize;
	_REAL	rArgTemp;

	/* Length of guard-interval measured in "time-bins" */
	iGuardSizeA = RMA_FFT_SIZE_N * RMA_ENUM_TG_TU / RMA_DENOM_TG_TU;
	iGuardSizeB = RMB_FFT_SIZE_N * RMB_ENUM_TG_TU / RMB_DENOM_TG_TU;
	iGuardSizeC = RMC_FFT_SIZE_N * RMC_ENUM_TG_TU / RMC_DENOM_TG_TU;
	iGuardSizeD = RMD_FFT_SIZE_N * RMD_ENUM_TG_TU / RMD_DENOM_TG_TU;

	/* Calculate maximum symbol block size (It is Rob. Mode A) */
	iMaxSymbolBlockSize = RMA_FFT_SIZE_N + iGuardSizeA;

	/* We need at least two blocks of data for correlation */
	iTotalBufferSize = 2 * iMaxSymbolBlockSize;

	/* Size for FFT buffer */
	iFFTBufSize = (int) (NO_BLOCKS_FOR_FFT * iMaxSymbolBlockSize 
		/ STEP_SIZE_GUARD_CORR_RMD);

	/* Calculate frequency bins which has to be observed for each mode */
	/* Mode A: f_A = 1 / T_s = 1 / 26.66 ms = 37.5 Hz */
	iObservedFreqBinA = (int) (
		/* Calculate f_A */
		(_REAL) SOUNDCRD_SAMPLE_RATE / (RMA_FFT_SIZE_N * 
		(1 + (_REAL) RMA_ENUM_TG_TU / RMA_DENOM_TG_TU)) /
		/* Frequency bin for our FFT-size */
		SOUNDCRD_SAMPLE_RATE * iFFTBufSize * STEP_SIZE_GUARD_CORR_RMD);

	/* Mode B: f_B = 1 / T_s = 1 / 26.66 ms = 37.5 Hz */
	iObservedFreqBinB = (int) (
		/* Calculate f_B */
		(_REAL) SOUNDCRD_SAMPLE_RATE / (RMB_FFT_SIZE_N * 
		(1 + (_REAL) RMB_ENUM_TG_TU / RMB_DENOM_TG_TU)) /
		/* Frequency bin for our FFT-size */
		SOUNDCRD_SAMPLE_RATE * iFFTBufSize * STEP_SIZE_GUARD_CORR_RMD);

	/* Mode C: f_C = 1 / T_s = 1 / 20 ms = 50 Hz */
	iObservedFreqBinC = (int) (
		/* Calculate f_C */
		(_REAL) SOUNDCRD_SAMPLE_RATE / (RMC_FFT_SIZE_N * 
		(1 + (_REAL) RMC_ENUM_TG_TU / RMC_DENOM_TG_TU)) /
		/* Frequency bin for our FFT-size */
		SOUNDCRD_SAMPLE_RATE * iFFTBufSize * STEP_SIZE_GUARD_CORR_RMD);

	/* Mode D: f_D = 1 / T_s = 1 / 16.66 ms = 60 Hz */
	iObservedFreqBinD = (int) (
		/* Calculate f_D */
		(_REAL) SOUNDCRD_SAMPLE_RATE / (RMD_FFT_SIZE_N * 
		(1 + (_REAL) RMD_ENUM_TG_TU / RMD_DENOM_TG_TU)) /
		/* Frequency bin for our FFT-size */
		SOUNDCRD_SAMPLE_RATE * iFFTBufSize * STEP_SIZE_GUARD_CORR_RMD);

	/* Init virtual index of correlation */
	iTimeSyncPos = 0;

	/* Allocate memory for FFT input buffers and zero out */
	vecrFFTBufferModA.Init(iFFTBufSize);
	vecrFFTBufferModB.Init(iFFTBufSize);
	vecrFFTBufferModC.Init(iFFTBufSize);
	vecrFFTBufferModD.Init(iFFTBufSize);

	/* Allocate memory for sample-history and zero out */
	vecrHistory.Init(iTotalBufferSize, (_REAL) 0.0);

	/* Tables for sin and cos function for the desired frequency band */
	vecrCosA.Init(iFFTBufSize);
	vecrCosB.Init(iFFTBufSize);
	vecrCosC.Init(iFFTBufSize);
	vecrCosD.Init(iFFTBufSize);

	for (int i = 0; i < iFFTBufSize; i++)
	{
		rArgTemp = (_REAL) 2.0 * crPi / iFFTBufSize * i;

		vecrCosA[i] = cos(rArgTemp * iObservedFreqBinA);
		vecrCosB[i] = cos(rArgTemp * iObservedFreqBinB);
		vecrCosC[i] = cos(rArgTemp * iObservedFreqBinC);
		vecrCosD[i] = cos(rArgTemp * iObservedFreqBinD);
	}

	/* Define block-size for input and output */
	iInputBlockSize = ReceiverParam.iSymbolBlockSize;
	iOutputBlockSize = ReceiverParam.iSymbolBlockSize;
}

void CRobModDet::StartAcquisition() 
{
	/* Reset the buffers which are storing data for correlation */
	vecrFFTBufferModA = Zeros(iFFTBufSize);
	vecrFFTBufferModB = Zeros(iFFTBufSize);
	vecrFFTBufferModC = Zeros(iFFTBufSize);
	vecrFFTBufferModD = Zeros(iFFTBufSize);

	/* Reset data in history buffer */
	vecrHistory.Reset((_REAL) 0.0);

	/* Set flag so that the actual routine can be entered */
	bAquisition = TRUE;
}
