/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
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

#include "Data.h"


/* Implementation *************************************************************/
/******************************************************************************\
* MSC data																	   *
\******************************************************************************/
/* Transmitter -------------------------------------------------------------- */
void CReadData::ProcessDataInternal(CParameter& TransmParam)
{
#ifdef WRITE_TRNSM_TO_FILE
	/* Stop writing file when defined number of blocks were generated */
	iCounter++;
	if (iCounter == iNumTransBlocks)
		TransmParam.bRunThread = FALSE;
#endif

	/* Get data from sound interface */
	pSound->Read(vecsSoundBuffer);

	/* Write data to output buffer */
	for (int i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = vecsSoundBuffer[i];
}

void CReadData::InitInternal(CParameter& TransmParam)
{
	/* Define block-size for output, an audio frame always corresponds
	   to 400 ms. We use always stereo blocks */
	iOutputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
		(_REAL) 0.4 /* 400 ms */ * 2 /* stereo */);

	/* Init sound interface and intermediate buffer */
	pSound->InitRecording(iOutputBlockSize, FALSE);
	vecsSoundBuffer.Init(iOutputBlockSize);
}


/* Receiver ----------------------------------------------------------------- */
void CWriteData::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* Send data to sound interface if audio is not muted */
	if (bMuteAudio == FALSE)
	{
		if (pSound->Write((*pvecInputData)) == FALSE)
			PostWinMessage(MS_IOINTERFACE, 0); /* green light */
		else
			PostWinMessage(MS_IOINTERFACE, 1); /* yellow light */
	}

	/* Write data as wave in file */
	if (bDoWriteWaveFile == TRUE)
	{
		for (int i = 0; i < iInputBlockSize; i += 2)
			WaveFileAudio.AddStereoSample((*pvecInputData)[i] /* left */,
				(*pvecInputData)[i + 1] /* right */);
	}
}

void CWriteData::InitInternal(CParameter& ReceiverParam)
{
	/* Define block-size for input, an audio frame always corresponds to 400 ms.
	   We use always stereo blocks */
	iInputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
		(_REAL) 0.4 /* 400 ms */ * 2 /* stereo */);

	/* Init sound interface */
	pSound->InitPlayback(iInputBlockSize);
}

void CWriteData::StartWriteWaveFile(const string strFileName)
{
	/* No Lock(), Unlock() needed here */
	if (bDoWriteWaveFile == FALSE)
	{
		WaveFileAudio.Open(strFileName);
		bDoWriteWaveFile = TRUE;
	}
}

void CWriteData::StopWriteWaveFile()
{
	Lock();

	WaveFileAudio.Close();
	bDoWriteWaveFile = FALSE;

	Unlock();
}


/* Simulation --------------------------------------------------------------- */
void CGenSimData::ProcessDataInternal(CParameter& TransmParam)
{
	int			i;
	_UINT32BIT	iTempShiftRegister1;
	_BINARY		biPRBSbit;
	_UINT32BIT	iShiftRegister;
	FILE*		pFileCurPos;
	time_t		tiElTi;
	long int	lReTi;

	/* Get elapsed time since this run was started (seconds) */
	tiElTi = time(NULL) - tiStartTime;

	/* Stop simulation if stop condition is true */
	iCounter++;
	switch (eCntType)
	{
	case CT_TIME:
		try
		{
			/* Estimate remaining time */
			lReTi = (long int) (((_REAL) iNumSimBlocks - iCounter) /
				iCounter * tiElTi);

			/* Store current counter position in file */
			pFileCurPos = fopen(strFileName.c_str(), "w");
			fprintf(pFileCurPos,
				"%d / %d (%d min elapsed, estimated time remaining: %d min)",
				iCounter, iNumSimBlocks, tiElTi / 60, lReTi / 60);

			/* Add current value of BER */
			fprintf(pFileCurPos, "\n%e %e", TransmParam.rSimSNRdB,
				TransmParam.rBitErrRate);
			fclose(pFileCurPos);
		}

		catch (...)
		{
			/* Catch all file errors to avoid stopping the simulation */
		}

		if (iCounter == iNumSimBlocks)
		{
			TransmParam.bRunThread = FALSE;
			iCounter = 0;
		}
		break;

	case CT_ERRORS:
		try
		{
			if (iCounter >= iMinNumBlocks)
			{
				/* Estimate remaining time */
				lReTi = (long int)
					(((_REAL) iNumErrors - TransmParam.iNumBitErrors) /
					TransmParam.iNumBitErrors * tiElTi);

				/* Store current counter position in file */
				pFileCurPos = fopen(strFileName.c_str(), "w");
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated time remaining: %d min)",
					TransmParam.iNumBitErrors, iNumErrors,
					tiElTi / 60, lReTi / 60);

				/* Add current value of BER */
				fprintf(pFileCurPos, "\n%e %e", TransmParam.rSimSNRdB,
					TransmParam.rBitErrRate);
				fclose(pFileCurPos);
			}
			else
			{
				/* Estimate remaining time */
				lReTi = (long int) 
					(((_REAL) iMinNumBlocks - iCounter) / iCounter * tiElTi);

				/* Store current counter position in file */
				pFileCurPos = fopen(strFileName.c_str(), "w");
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated minimum"
					" time remaining: %d min)\n",
					iCounter, iMinNumBlocks, tiElTi / 60, lReTi / 60);

				lReTi = (long int)
					(((_REAL) iNumErrors - TransmParam.iNumBitErrors) /
					TransmParam.iNumBitErrors * tiElTi);
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated"
					" time remaining: %d min)",
					TransmParam.iNumBitErrors, iNumErrors, tiElTi / 60,
					lReTi / 60);

				/* Add current value of BER */
				fprintf(pFileCurPos, "\n%e %e", TransmParam.rSimSNRdB,
					TransmParam.rBitErrRate);
				fclose(pFileCurPos);
			}
		}

		catch (...)
		{
			/* Catch all file errors to avoid stopping the simulation */
		}

		if (TransmParam.iNumBitErrors >= iNumErrors)
		{
			/* A minimum simulation time must be elapsed */
			if (iCounter >= iMinNumBlocks)
			{
				TransmParam.bRunThread = FALSE;
				iCounter = 0;
			}
		}
		break;
	}

	/* Generate a pseudo-noise test-signal (PRBS) */
	/* Init shift register with an arbitrary number (Must be known at the
	   receiver AND transmitter!) */
	iShiftRegister = (_UINT32BIT) (time(NULL) + rand());
	TransmParam.RawSimDa.Add(iShiftRegister);

	for (i = 0; i < iOutputBlockSize; i++)
	{
		/* Calculate new PRBS bit */
		iTempShiftRegister1 = iShiftRegister;

		/* P(X) = X^9 + X^5 + 1,
		   in this implementation we have to shift n-1! */
		biPRBSbit = ((iTempShiftRegister1 >> 4) & 1) ^
			((iTempShiftRegister1 >> 8) & 1);

		/* Shift bits in shift register and add new bit */
		iShiftRegister <<= 1;
		iShiftRegister |= (biPRBSbit & 1);

		/* Use PRBS output */
		if (biPRBSbit == 0)
			(*pvecOutputData)[i] = FALSE;
		else
			(*pvecOutputData)[i] = TRUE;
	}
}

void CGenSimData::InitInternal(CParameter& TransmParam)
{
	/* Define output block size */
	iOutputBlockSize = TransmParam.iNumDecodedBitsMSC;

	/* Minimum simulation time depends on the selected channel */
	switch (TransmParam.iDRMChannelNum)
	{
	case 1:
		/* AWGN: No fading */
		iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
		break;

	case 2:
		/* Rice with delay: 0.1 Hz */
		iMinNumBlocks = (int) ((_REAL) 5000.0 / (_REAL) 0.4);
		break;

	case 3:
		/* US Consortium: slowest 0.1 Hz */
		iMinNumBlocks = (int) ((_REAL) 15000.0 / (_REAL) 0.4);
		break;

	case 4:
		/* CCIR Poor: 1 Hz */
		iMinNumBlocks = (int) ((_REAL) 4000.0 / (_REAL) 0.4);
		break;

	case 5:
		/* Channel no 5: 2 Hz -> 30 sec */
		iMinNumBlocks = (int) ((_REAL) 3000.0 / (_REAL) 0.4);
		break;

	case 6:
		/* Channel no 6: same as case "2" */
		iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
		break;

	default:
		/* My own channels */
		iMinNumBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
		break;
	}

	/* Prepare shift register used for storing the start values of the PRBS
	   shift register */
	TransmParam.RawSimDa.Reset();

	/* Init start time */
	tiStartTime = time(NULL);
}

void CGenSimData::SetSimTime(int iNewTi, string strNewFileName)
{
	/* One MSC frame is 400 ms long */
	iNumSimBlocks = (int) ((_REAL) iNewTi /* sec */ / (_REAL) 0.4);

	/* Set simulation count type */
	eCntType = CT_TIME;

	/* Reset counter */
	iCounter = 0;

	/* Set file name */
	strFileName =
		string("test/") + strNewFileName + "__SIMTIME" + string(".dat");
}

void CGenSimData::SetNumErrors(int iNewNE, string strNewFileName)
{
	iNumErrors = iNewNE;

	/* Set simulation count type */
	eCntType = CT_ERRORS;

	/* Reset counter, because we also use it at the beginning of a run */
	iCounter = 0;

	/* Set file name */
	strFileName =
		string("test/") + strNewFileName + "__SIMTIME" + string(".dat");
}

void CEvaSimData::ProcessDataInternal(CParameter& ReceiverParam)
{
	_UINT32BIT		iTempShiftRegister1;
	_BINARY			biPRBSbit;
	_UINT32BIT		iShiftRegister;
	int				iNumBitErrors;
	int				i;

	/* -------------------------------------------------------------------------
	   Generate a pseudo-noise test-signal (PRBS) for comparison with
	   received signal */
	/* Init shift register with an arbitrary number (Must be known at the
	   receiver AND transmitter!) */
	iShiftRegister = ReceiverParam.RawSimDa.Get();

	iNumBitErrors = 0;

	for (i = 0; i < iInputBlockSize; i++)
	{
		/* Calculate new PRBS bit */
		iTempShiftRegister1 = iShiftRegister;

		/* P(X) = X^9 + X^5 + 1,
		   in this implementation we have to shift n-1! */
		biPRBSbit = ((iTempShiftRegister1 >> 4) & 1) ^
			((iTempShiftRegister1 >> 8) & 1);

		/* Shift bits in shift register and add new bit */
		iShiftRegister <<= 1;
		iShiftRegister |= (biPRBSbit & 1);

		/* Count bit errors */
		if (biPRBSbit != (*pvecInputData)[i])
			iNumBitErrors++;
	}

	/* Save bit error rate, debar initialization blocks */
	if (iIniCnt > 0)
		iIniCnt--;
	else
	{
		rAccBitErrRate += (_REAL) iNumBitErrors / iInputBlockSize;
		iNumAccBitErrRate++;

		ReceiverParam.rBitErrRate = rAccBitErrRate / iNumAccBitErrRate;
		ReceiverParam.iNumBitErrors += iNumBitErrors;
	}
}

void CEvaSimData::InitInternal(CParameter& ReceiverParam)
{
	/* Reset bit error rate parameters */
	rAccBitErrRate = (_REAL) 0.0;
	iNumAccBitErrRate = 0;

	/* Number of blocks at the beginning we do not want to use */
	iIniCnt = 10;

	/* Init global parameters */
	ReceiverParam.rBitErrRate = (_REAL) 0.0;
	ReceiverParam.iNumBitErrors = 0;

	/* Define block-size for input */
	iInputBlockSize = ReceiverParam.iNumDecodedBitsMSC;
}


/******************************************************************************\
* FAC data																	   *
\******************************************************************************/
/* Transmitter */
void CGenerateFACData::ProcessDataInternal(CParameter& TransmParam)
{
	FACTransmit.FACParam(pvecOutputData, TransmParam);
}

void CGenerateFACData::InitInternal(CParameter& TransmParam)
{
	FACTransmit.Init(TransmParam);

	/* Define block-size for output */
	iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}

/* Receiver */
void CUtilizeFACData::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* Do not use received FAC data in case of simulation */
	if (bSyncInput == FALSE)
	{
		bCRCOk = FACReceive.FACParam(pvecInputData, ReceiverParam);

		if (bCRCOk == TRUE)
		{
			PostWinMessage(MS_FAC_CRC, 0);

			/* Set AAC in log file */
			ReceiverParam.ReceptLog.SetFAC(TRUE);
		}
		else
		{
			PostWinMessage(MS_FAC_CRC, 2);

			/* Set AAC in log file */
			ReceiverParam.ReceptLog.SetFAC(FALSE);
		}
	}

	if ((bSyncInput == TRUE) || (bCRCOk == FALSE))
	{
		/* If FAC CRC check failed we should increase the frame-counter 
		   manually. If only FAC data was corrupted, the others can still
		   decode if they have the right frame number. In case of simulation
		   no FAC data is used, we have to increase the counter here */
		ReceiverParam.iFrameIDReceiv++;

		if (ReceiverParam.iFrameIDReceiv == NUM_FRAMES_IN_SUPERFRAME)
			ReceiverParam.iFrameIDReceiv = 0;
	}
}

void CUtilizeFACData::InitInternal(CParameter& ReceiverParam)
{

// This should be in FAC class in an Init() routine which has to be defined, this
// would be cleaner code! TODO
/* Init frame ID so that a "0" comes after increasing the init value once */
ReceiverParam.iFrameIDReceiv = NUM_FRAMES_IN_SUPERFRAME - 1;

	/* Reset flag */
	bCRCOk = FALSE;

	/* Define block-size for input */
	iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;
}


/******************************************************************************\
* SDC data																	   *
\******************************************************************************/
/* Transmitter */
void CGenerateSDCData::ProcessDataInternal(CParameter& TransmParam)
{
	SDCTransmit.SDCParam(pvecOutputData, TransmParam);
}

void CGenerateSDCData::InitInternal(CParameter& TransmParam)
{
	/* Define block-size for output */
	iOutputBlockSize = TransmParam.iNumSDCBitsPerSFrame;
}

/* Receiver */
void CUtilizeSDCData::ProcessDataInternal(CParameter& ReceiverParam)
{
	_BOOLEAN bCRCOk;

	bCRCOk = SDCReceive.SDCParam(pvecInputData, ReceiverParam);

	if (bCRCOk)
		PostWinMessage(MS_SDC_CRC, 0);
	else
		PostWinMessage(MS_SDC_CRC, 2);
}

void CUtilizeSDCData::InitInternal(CParameter& ReceiverParam)
{
	/* Define block-size for input */
	iInputBlockSize = ReceiverParam.iNumSDCBitsPerSFrame;
}
