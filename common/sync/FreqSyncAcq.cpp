
/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Frequency synchronization acquisition (FFT-based)
 *
 * The input data is not modified by this module, it is just a measurement
 * of the frequency offset. The data is fed through this module.
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

#include "FreqSyncAcq.h"


/* Implementation *************************************************************/
void CFreqSyncAcq::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i, j;
	int			iMaxIndex;
	fftw_real	rMaxValue;
	int			iNoDetPeaks;
	int			iDiffTemp;
	CReal		rLevDiff;
	_BOOLEAN	bNoPeaksLeft;
	CRealVector vecrPSDPilPoin(3);

	if (bAquisition == TRUE)
	{
		/* Add new symbol in history (shift register) */
		vecrFFTHistory.AddEnd((*pvecInputData), iSymbolBlockSize);


		/* Start algorithm when history memory is filled -------------------- */
		/* Wait until history memory is filled for the first time */
		if (iAquisitionCounter > 0)
		{
			/* Decrease counter */
			iAquisitionCounter--;
		}
		else
		{
			/* Real-valued FFTW */
			rfftw_one(RFFTWPlan, &vecrFFTHistory[0], &vecrFFTOutput[0]);

			/* Introduce a time-out for the averaging in case of no detected
			   peak in a certain time interval */
			iAverTimeOutCnt++;
			if (iAverTimeOutCnt > AVERAGE_TIME_OUT_NUMBER)
			{
				/* Reset counter and average vector */
				iAverTimeOutCnt = 0;
				iAverageCounter = NO_BLOCKS_BEFORE_US_AV;
				vecrPSD = Zeros(iHalfBuffer);
			}

			/* Calculate power spectrum (X = real(F)^2 + imag(F)^2)*/
			for (i = 1; i < iHalfBuffer; i++)
				vecrPSD[i] += vecrFFTOutput[i] * vecrFFTOutput[i] + 
					vecrFFTOutput[iTotalBufferSize - i] * 
					vecrFFTOutput[iTotalBufferSize - i];

			/* Correlate known frequency-pilot structure with power spectrum and
			   average results by adding them together */
			for (i = 0; i < iSearchWinSize; i++)
				vecrPSDPilCor[i] = 
					vecrPSD[i + piTableFreqPilots[0]] +
					vecrPSD[i + piTableFreqPilots[1]] +
					vecrPSD[i + piTableFreqPilots[2]];

			/* Wait until we have sufficiant data averaged */
			if (iAverageCounter > 0)
			{
				/* Decrease counter */
				iAverageCounter--;
			}
			else
			{
				/* -------------------------------------------------------------
				   Low pass filtering over frequency axis. We do the filtering 
				   from both sides, once from right to left and then from left 
				   to the right side. Afterwards, these results are averaged */
				const CReal rLambdaF = 0.9;
				/* From left to right */
				vecrFiltResLR[0] = vecrPSDPilCor[0];
				for (i = 1; i < iSearchWinSize; i++)
					vecrFiltResLR[i] = rLambdaF * (vecrFiltResLR[i - 1] - 
						vecrPSDPilCor[i]) + vecrPSDPilCor[i];

				/* From right to left */
				vecrFiltResRL[iSearchWinSize - 1] = 
					vecrPSDPilCor[iSearchWinSize - 1];
				for (i = iSearchWinSize - 2; i >= 0; i--)
					vecrFiltResRL[i] = rLambdaF * (vecrFiltResRL[i + 1] - 
						vecrPSDPilCor[i]) + vecrPSDPilCor[i];

				/* Average RL and LR filter outputs */
				vecrFiltRes = (vecrFiltResLR + vecrFiltResRL) / 2;
		

				/* Detect peaks by the distance to the filtered curve ------- */
				/* Get peak indices of detected peaks */
				iNoDetPeaks = 0;
				for (i = 0; i < iSearchWinSize; i++)
				{
					if (vecrPSDPilCor[i] / vecrFiltRes[i] > 
						PEAK_BOUND_FILT2SIGNAL)
					{
						veciPeakIndex[iNoDetPeaks] = i;
						iNoDetPeaks++;
					}
				}

				/* Check, if at least one peak was detected */
				if (iNoDetPeaks > 0)
				{
					/* ---------------------------------------------------------
					   The following test shall exclude sinusoid interferers in 
					   the received spectrum */
					CVector<int> vecbFlagVec(iNoDetPeaks, 1);

					/* Check all detected peaks in the "PSD-domain" if there are
					   at least two peaks with approx the same power at the
					   right places (positions of the desired pilots) */
					for (i = 0; i < iNoDetPeaks; i++)
					{
						/* Fill the vector with the values at the desired 
						   pilot positions */
						vecrPSDPilPoin[0] = 
							vecrPSD[veciPeakIndex[i] + piTableFreqPilots[0]];
						vecrPSDPilPoin[1] = 
							vecrPSD[veciPeakIndex[i] + piTableFreqPilots[1]];
						vecrPSDPilPoin[2] = 
							vecrPSD[veciPeakIndex[i] + piTableFreqPilots[2]];

						/* Sort, to extract the higesht and second highest
						   peak */
						vecrPSDPilPoin = Sort(vecrPSDPilPoin);

						/* Debar peak, if it is much higher than second highest
						   peak (most probably a sinusoid interferer) */
						if (vecrPSDPilPoin[2] / vecrPSDPilPoin[1] > 
							MAX_RAT_PEAKS_AT_PIL_POS)
						{
							/* Reset "good-flag" */
							vecbFlagVec[i] = 0;
						}
					}


					/* Get maximum ------------------------------------------ */
					/* First, get the first valid peak entry and init the 
					   maximum with this value. We also detect, if a peak is 
					   left */
					bNoPeaksLeft = TRUE;
					for (i = 0; i < iNoDetPeaks; i++)
					{
						if (vecbFlagVec[i] == 1)
						{
							/* At least one peak is left */
							bNoPeaksLeft = FALSE;

							/* Init max value */
							iMaxIndex = veciPeakIndex[i];
							rMaxValue = vecrPSDPilCor[veciPeakIndex[i]];
						}
					}

					if (bNoPeaksLeft == FALSE)
					{
						/* Actual maximum detection, take the remaining peak
						   which has the highest value */
						for (i = 0; i < iNoDetPeaks; i++)
						{
							if ((vecbFlagVec[i] == 1) &&
								(vecrPSDPilCor[veciPeakIndex[i]] > 
								rMaxValue))
							{
								iMaxIndex = veciPeakIndex[i];
								rMaxValue = vecrPSDPilCor[veciPeakIndex[i]];
							}
						}



#ifdef _DEBUG_
// TEST
FILE* pFile1 = fopen("test/freqacq.dat", "w");
int iPeakCnt = 0;
for (i = 1; i < iSearchWinSize; i++)
{
	_REAL rPeakMarker;
	_REAL rFinPM;
	if (iPeakCnt < iNoDetPeaks)
	{
		if (i == veciPeakIndex[iPeakCnt])
		{
			rPeakMarker = vecrPSDPilCor[i];
			if (vecbFlagVec[iPeakCnt] == 1)
				rFinPM = vecrPSDPilCor[i];
			else
				rFinPM = 0;
			iPeakCnt++;
		}
		else
		{
			rPeakMarker = 0;
			rFinPM = 0;
		}
	}
	else
	{
		rPeakMarker = 0;
		rFinPM = 0;
	}
	

	fprintf(pFile1, "%e %e %e %e\n", vecrPSDPilCor[i], vecrFiltRes[i], rPeakMarker, rFinPM);
}
fclose(pFile1);
// close all;load freqacq.dat;semilogy(freqacq(:,1:2));hold;plot(freqacq(:,3),'*y');plot(freqacq(:,4),'*k');
#endif



						/* -----------------------------------------------------
						   An acquisition frequency offest estimation was 
						   found */
						/* Calculate frequency offset and set global parameter
						   for offset */
						_REAL rNewOffsetNorm = 
							(_REAL) iMaxIndex / iTotalBufferSize;

						ReceiverParam.rFreqOffsetAcqui = 
							rNewOffsetNorm - rDesFreqPosNorm;

						/* Calculate IIR filter taps for 4.5 kHz filter with the
						   new detected frequency offset */
						GetIIRTaps(rvecB, rvecA, rNewOffsetNorm * 2);

						/* Init state vector for filtering with zeros */
						rvecZ.Init(Size(rvecB));
						rvecZ = Zeros(Size(rvecB));

						/* Reset acquisition flag */
						bAquisition = FALSE;
					}
				}
			}
		}

		/* Do not transfer any data to the next block if no frequency 
		   acquisition was successfully done */
		iOutputBlockSize = 0;
	}
	else
	{
		/* Use the same block size as input block size */
		iOutputBlockSize = iInputBlockSize;

		if (bIIRFiltEnabled == TRUE)
		{
			/* -----------------------------------------------------------------
			   Data must be band-pass-filtered before applying the algorithms,
			   because we do not know which mode is active when we synchronize
			   the timing and we must assume the worst-case, therefore use only
			   from DC to 4.5 kHz */
			/* Copy CVector data in CMatlibVector */
			for (i = 0; i < iInputBlockSize; i++)
				rvecInpTmp[i] = (*pvecInputData)[i];

			/* Actual filter routine */
			rvecOutTmp = Filter(rvecB, rvecA, rvecInpTmp, rvecZ);
	
			/* Use filtered data for copying to output vector */
			for (i = 0; i < iInputBlockSize; i++)
				(*pvecOutputData)[i] = rvecOutTmp[i];
		}
		else
		{
			/* Copy data from input to the output. Data is not modified in this
			   module */
			for (i = 0; i < iOutputBlockSize; i++)
				(*pvecOutputData)[i] = (*pvecInputData)[i];
		}
	}

	/* If synchronized DRM input stream is used, overwrite the detected
	   frequency offest estimate by "0", because we know this value */
	if (bSyncInput == TRUE)
		ReceiverParam.rFreqOffsetAcqui = (_REAL) 0.0;
}

void CFreqSyncAcq::InitInternal(CParameter& ReceiverParam)
{
	_REAL	rNormDesPos;
	_REAL	rNormWinSize;

	/* Get frequency pilot table (750 Hz, 2250 Hz, and 3000 Hz) */
	switch (ReceiverParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		/* The number stored in table "iTableFreqPilRobModA" must be
		   adjusted to the new fft-window-size */
		piTableFreqPilots[0] = (int) ((_REAL) iTableFreqPilRobModA[0][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[1] = (int) ((_REAL) iTableFreqPilRobModA[1][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[2] = (int) ((_REAL) iTableFreqPilRobModA[2][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		break;

	case RM_ROBUSTNESS_MODE_B:
		piTableFreqPilots[0] = (int) ((_REAL) iTableFreqPilRobModB[0][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[1] = (int) ((_REAL) iTableFreqPilRobModB[1][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[2] = (int) ((_REAL) iTableFreqPilRobModB[2][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		break;

	case RM_ROBUSTNESS_MODE_C:
		piTableFreqPilots[0] = (int) ((_REAL) iTableFreqPilRobModC[0][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[1] = (int) ((_REAL) iTableFreqPilRobModC[1][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[2] = (int) ((_REAL) iTableFreqPilRobModC[2][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		break;

	case RM_ROBUSTNESS_MODE_D:
		piTableFreqPilots[0] = (int) ((_REAL) iTableFreqPilRobModD[0][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[1] = (int) ((_REAL) iTableFreqPilRobModD[1][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		piTableFreqPilots[2] = (int) ((_REAL) iTableFreqPilRobModD[2][0] * 
			NO_BLOCKS_4_FREQ_ACQU * ((_REAL) ReceiverParam.RatioTgTu.iEnum / 
			ReceiverParam.RatioTgTu.iDenom + 1));
		break;
	}

	/* Needed for calculating offset in Hertz */
	iFFTSize = ReceiverParam.iFFTSizeN;
	
	/* Symbol block size is the guard-interval plus the useful part */
	iSymbolBlockSize = ReceiverParam.iSymbolBlockSize;

	/* Calculate the desired frequency position (normalized) */
	rDesFreqPosNorm = 
		(_REAL) ReceiverParam.iIndexDCFreq / ReceiverParam.iFFTSizeN;

	/* Total buffer size */
	iTotalBufferSize = NO_BLOCKS_4_FREQ_ACQU * iSymbolBlockSize;


	/* -------------------------------------------------------------------------
	   Set start- and endpoint of search window for DC carrier after the
	   correlation with the known pilot structure */
	/* Normalize the desired position and window size which are in Hertz */
	rNormDesPos = rCenterFreq / SOUNDCRD_SAMPLE_RATE;
	rNormWinSize = rWinSize / SOUNDCRD_SAMPLE_RATE;

	/* Center of maximum possible search window */
	iHalfBuffer = (iTotalBufferSize + 1) / 2;

	/* Search window is smaller than haft-buffer size because of correlation
	   with pilot positions */
	iSearchWinSize = iHalfBuffer - piTableFreqPilots[2];

	iStartDCSearch = (int) ((rNormDesPos - rWinSize / 2) * iHalfBuffer);
	iEndDCSearch = (int) ((rNormDesPos + rWinSize / 2) * iHalfBuffer);

	/* Check range. If out of range -> correct */
	if (!((iStartDCSearch > 0) && (iStartDCSearch < iHalfBuffer)))
		iStartDCSearch = 0;

	if (!((iEndDCSearch > 0) && (iEndDCSearch < iHalfBuffer)))
		iEndDCSearch = iHalfBuffer;


	/* Allocate memory for FFT-histories and init with zeros */
	vecrFFTHistory.Init(iTotalBufferSize, (_REAL) 0.0);
	vecrFFTOutput.Init(iTotalBufferSize, (_REAL) 0.0);

	vecrPSD.Init(iHalfBuffer);

	/* Allocate memory for PSD after pilot correlation */
	vecrPSDPilCor.Init(iHalfBuffer);

	/* Init vectors for filtering in frequency direction */
	vecrFiltResLR.Init(iHalfBuffer);
	vecrFiltResRL.Init(iHalfBuffer);
	vecrFiltRes.Init(iHalfBuffer);

	/* Index memory for detected peaks (assume worst case with the size) */
	veciPeakIndex.Init(iHalfBuffer);

	/* Allocate memory for IIR filter intermediate buffers */
	rvecInpTmp.Init(iSymbolBlockSize);
	rvecOutTmp.Init(iSymbolBlockSize);

	/* Create plan for rfftw */
	if (RFFTWPlan != NULL)
		fftw_destroy_plan(RFFTWPlan);
	RFFTWPlan = rfftw_create_plan(iTotalBufferSize, 
		FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);

	/* Define block-sizes for input (The output block size is set inside
	   the processing routine) */
	iInputBlockSize = iSymbolBlockSize;
	iMaxOutputBlockSize = iSymbolBlockSize; // Because output can be 0 sometimes
}

CFreqSyncAcq::~CFreqSyncAcq()
{
	/* Destroy FFTW plan */
	if (RFFTWPlan != NULL)
		fftw_destroy_plan(RFFTWPlan);
}

void CFreqSyncAcq::SetSearchWindow(_REAL rNewCenterFreq, _REAL rNewWinSize)
{
	/* Set internal parameters */
	rCenterFreq = rNewCenterFreq;
	rWinSize = rNewWinSize;

	/* Set flag to initialize the module to the new parameters */
	SetInitFlag();
}

void CFreqSyncAcq::StartAcquisition() 
{
	/* Set flag so that the actual acquisition routine is entered */
	bAquisition = TRUE; 

	/* If frequency acquisition was started, after that the data has to be
	   filtered -> set flag */
	bIIRFiltEnabled = TRUE;
	
	/* Reset (or init) counters */
	iAquisitionCounter = NO_BLOCKS_4_FREQ_ACQU;
	iAverageCounter = NO_BLOCKS_BEFORE_US_AV;
	iAverTimeOutCnt = 0;

	/* Reset vector for the averaged spectrum */
	vecrPSD = Zeros(iHalfBuffer);

	/* Reset FFT-history */
	vecrFFTHistory.Reset((_REAL) 0.0);
}

