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
/* Transmitter */
void CReadData::ProcessDataInternal(CParameter& TransmParam)
{
	static int	iCounter = 0;

	/* Stop simulation if defined number of blocks are generated */
	iCounter++;
	if (iCounter == iNoTransBlocks)
		TransmParam.bRunThread = FALSE;

	/* Write your data in (*pvecOutputData)[i],
	   where i = 0..iOutputBlockSize - 1*/
	for (int i = 0; i < iOutputBlockSize; i++)
	{
		/* TEST: "TRUE" -> To be filled with meaningful values */
		(*pvecOutputData)[i] = TRUE;
	}
}

void CReadData::InitInternal(CParameter& TransmParam)
{
	/* Define output block size */
	iOutputBlockSize = TransmParam.iNoDecodedBitsMSC;
}

/* Receiver */
void CWriteData::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* Send data to sound interface if audio is not muted */
	if (bMuteAudio == FALSE)
		Sound.Write((*pvecInputData));
}

void CWriteData::InitInternal(CParameter& ReceiverParam)
{
	/* Define block-size for input, an audio frame always corresponds to 400 ms.
	   We use always stereo blocks */
	iInputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
		(_REAL) 0.4 /* 400ms */ * 2 /* stereo */);

	/* Init sound interface */
	Sound.InitPlayback(iInputBlockSize);
}

/* Simulation */
void CGenSimData::ProcessDataInternal(CParameter& TransmParam)
{
	int			i;
	_UINT32BIT	iTempShiftRegister1;
	_BINARY		biPRBSbit;
	_UINT32BIT	iShiftRegister;
	FILE*		pFileCurPos;
	time_t		tiElTi;
	long int	lReTi;

	/* Get elapsed time since this run was started (in minutes) */
	tiElTi = time(NULL) - tiStartTime;

	/* Stop simulation if stop condition is true */
	iCounter++;
	switch (eCntType)
	{
	case CT_TIME:
		try
		{
			/* Estimate remaining time */
			lReTi = (long int) (((_REAL) iNoSimBlocks - iCounter) /
				iCounter * tiElTi);

			/* Store current counter position in file */
			pFileCurPos = fopen(strFileName.c_str(), "w");
			fprintf(pFileCurPos,
				"%d / %d (%d min elapsed, estimated time remaining: %d min)",
				iCounter, iNoSimBlocks, tiElTi / 60, lReTi / 60);

			/* Add current value of BER */
			fprintf(pFileCurPos, "\n%e %e", TransmParam.rSimSNRdB,
				TransmParam.rBitErrRate);
			fclose(pFileCurPos);
		}

		catch (...)
		{
			/* Catch all file errors to avoid stopping the simulation */
		}

		if (iCounter == iNoSimBlocks)
		{
			TransmParam.bRunThread = FALSE;
			iCounter = 0;
		}
		break;

	case CT_ERRORS:
		try
		{
			if (iCounter >= iMinNoBlocks)
			{
				/* Estimate remaining time */
				lReTi =
					(long int) (((_REAL) TransmParam.iNoBitErrors - iNoErrors) /
					iNoErrors * tiElTi);

				/* Store current counter position in file */
				pFileCurPos = fopen(strFileName.c_str(), "w");
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated time remaining: %d min)",
					TransmParam.iNoBitErrors, iNoErrors, tiElTi / 60, lReTi / 60);

				/* Add current value of BER */
				fprintf(pFileCurPos, "\n%e %e", TransmParam.rSimSNRdB,
					TransmParam.rBitErrRate);
				fclose(pFileCurPos);
			}
			else
			{
				/* Estimate remaining time */
				lReTi = (long int) 
					(((_REAL) iMinNoBlocks - iCounter) / iCounter * tiElTi);

				/* Store current counter position in file */
				pFileCurPos = fopen(strFileName.c_str(), "w");
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated minimum time remaining: %d min)\n",
					iCounter, iMinNoBlocks, tiElTi / 60, lReTi / 60);

				lReTi = (long int) (((_REAL) TransmParam.iNoBitErrors - iNoErrors) /
					iNoErrors * tiElTi);
				fprintf(pFileCurPos,
					"%d / %d (%d min elapsed, estimated time remaining: %d min)", 
					TransmParam.iNoBitErrors, iNoErrors, tiElTi / 60, lReTi / 60);

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

		if (TransmParam.iNoBitErrors >= iNoErrors)
		{
			/* A minimum simulation time must be elapsed */
			if (iCounter >= iMinNoBlocks)
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
	iOutputBlockSize = TransmParam.iNoDecodedBitsMSC;

	/* Minimum simulation time depends on the selected channel */
	switch (TransmParam.iDRMChannelNo)
	{
	case 1:
		/* AWGN: No fading */
		iMinNoBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
		break;

	case 2:
		/* Rice with delay: 0.1 Hz */
		iMinNoBlocks = (int) ((_REAL) 5000.0 / (_REAL) 0.4);
		break;

	case 3:
		/* US Consortium: slowest 0.1 Hz */
		iMinNoBlocks = (int) ((_REAL) 5000.0 / (_REAL) 0.4);
		break;

	case 4:
		/* CCIR Poor: 1 Hz */
		iMinNoBlocks = (int) ((_REAL) 2000.0 / (_REAL) 0.4);
		break;

	case 5:
		/* Channel no 5: 2 Hz -> 30 sec */
		iMinNoBlocks = (int) ((_REAL) 1000.0 / (_REAL) 0.4);
		break;

	case 6:
		/* Channel no 6: same as case "2" */
		iMinNoBlocks = (int) ((_REAL) 1000.0 / (_REAL) 0.4);
		break;

	default:
		/* My own channels */
		iMinNoBlocks = (int) ((_REAL) 1000.0 / (_REAL) 0.4);
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
	iNoSimBlocks = (int) ((_REAL) iNewTi /* sec */ / (_REAL) 0.4);

	/* Set simulation count type */
	eCntType = CT_TIME;

	/* Reset counter */
	iCounter = 0;

	/* Set file name */
	strFileName =
		string("test/") + strNewFileName + "__SIMTIME" + string(".dat");
}

void CGenSimData::SetNoErrors(int iNewNE, string strNewFileName)
{
	iNoErrors = iNewNE;

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
	int				iNoBitErrors;
	int				i;

	/* -------------------------------------------------------------------------
	   Generate a pseudo-noise test-signal (PRBS) for comparison with
	   received signal */
	/* Init shift register with an arbitrary number (Must be known at the
	   receiver AND transmitter!) */
	iShiftRegister = ReceiverParam.RawSimDa.Get();

	iNoBitErrors = 0;

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
			iNoBitErrors++;
	}

	/* Save bit error rate, debar initialization blocks */
	if (iIniCnt > 0)
		iIniCnt--;
	else
	{
		rAccBitErrRate += (_REAL) iNoBitErrors / iInputBlockSize;
		iNoAccBitErrRate++;

		ReceiverParam.rBitErrRate = rAccBitErrRate / iNoAccBitErrRate;
		ReceiverParam.iNoBitErrors += iNoBitErrors;
	}
}

void CEvaSimData::InitInternal(CParameter& ReceiverParam)
{
	/* Reset bit error rate parameters */
	rAccBitErrRate = (_REAL) 0.0;
	iNoAccBitErrRate = 0;

	/* Number of blocks at the beginning we do not want to use */
	iIniCnt = 10;

	/* Init global parameters */
	ReceiverParam.rBitErrRate = (_REAL) 0.0;
	ReceiverParam.iNoBitErrors = 0;

	/* Define block-size for input */
	iInputBlockSize = ReceiverParam.iNoDecodedBitsMSC;
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
	/* Define block-size for output */
	iOutputBlockSize = NO_FAC_BITS_PER_BLOCK;
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
		   decoder if they have the right frame number. In case of simulation
		   no FAC data is used, we have to increase the counter here */
		ReceiverParam.iFrameIDReceiv++;

		if (ReceiverParam.iFrameIDReceiv == NO_FRAMES_IN_SUPERFRAME)
			ReceiverParam.iFrameIDReceiv = 0;
	}
}

void CUtilizeFACData::InitInternal(CParameter& ReceiverParam)
{

// This should be in FAC class in an Init() routine which has to be defined, this
// would be cleaner code! TODO
/* Init frame ID so that a "0" comes after increasing the init value once */
ReceiverParam.iFrameIDReceiv = NO_FRAMES_IN_SUPERFRAME - 1;

	/* Define block-size for input */
	iInputBlockSize = NO_FAC_BITS_PER_BLOCK;
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
	iOutputBlockSize = TransmParam.iNoSDCBitsPerSFrame;
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
	iInputBlockSize = ReceiverParam.iNoSDCBitsPerSFrame;
}


/******************************************************************************\
* Simulation for channel estimation											   *
\******************************************************************************/
void CIdealChanEst::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i, j;

	/* Calculation of channel tranfer function ------------------------------ */
	/*	pvecInputData.cSig		equalized signal \hat{s}(t)
		pvecInputData2.tOut		received signal r(t)
		pvecInputData2.tIn		transmitted signal s(t)
		pvecInputData2.tRef		received signal without noise (channel
									reference signal) */
	for (i = 0; i < iNoCarrier; i++)
	{
		veccEstChan[i] = (*pvecInputData2)[i].tOut / (*pvecInputData)[i].cSig;
		veccRefChan[i] = (*pvecInputData2)[i].tRef / (*pvecInputData2)[i].tIn;
	}

	/* Debar DC carriers, set them to zero */
	for (i = 0; i < iNoDCCarriers; i++)
	{
		veccEstChan[i + iStartDCCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
		veccRefChan[i + iStartDCCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
	}

	/* Start evaluation results after exceeding the start count */
	if (iStartCnt > 0)
		iStartCnt--;
	else
	{
		/* MSE for all carriers */
		for (i = 0; i < iNoCarrier; i++)
			vecrMSEAverage[i] += SqMag(veccEstChan[i] - veccRefChan[i]);

		/* New values have been added, increase counter for final result
		   calculation */
		lAvCnt++;
	}


	/* Equalize the output vector ------------------------------------------- */
	/* Write to output vector. Also, ship the channel state at a certain cell */
	for (i = 0; i < iNoCarrier; i++)
	{
		(*pvecOutputData)[i].cSig = (*pvecInputData2)[i].tOut / veccRefChan[i];
		(*pvecOutputData)[i].rChan = SqMag(veccRefChan[i]);
	}

	/* Set symbol number for output vector */
	(*pvecOutputData).GetExData().iSymbolNo = 
		(*pvecInputData).GetExData().iSymbolNo;
}

void CIdealChanEst::InitInternal(CParameter& ReceiverParam)
{
	/* Init base class for modifying the pilots (rotation) */
	CPilotModiClass::InitRot(ReceiverParam);

	/* Get local parameters */
	iNoCarrier = ReceiverParam.iNoCarrier;
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;

	iNumTapsChan = ReceiverParam.iNumTaps;

	/* Init and calculate rotation matrix */
	matcRot.Init(iNoCarrier, iNumTapsChan);
	for (int i = 0; i < iNoCarrier; i++)
		for (int j = 0; j < iNumTapsChan; j++)
			matcRot[i][j] =
				Rotate((CReal) 1.0, i, ReceiverParam.iPathDelay[j] +
				ReceiverParam.iOffUsfExtr);


	/* Parameters for debaring the DC carriers from evaluation. First check if
	   we have only useful part on the right side of the DC carrier */
	if (ReceiverParam.iCarrierKmin > 0)
	{
		/* In this case, no DC carriers are in the useful spectrum */
		iNoDCCarriers = 0;
		iStartDCCar = 0;
	}
	else
	{
		if (ReceiverParam.GetWaveMode() == RM_ROBUSTNESS_MODE_A)
		{
			iNoDCCarriers = 3;
			iStartDCCar = abs(ReceiverParam.iCarrierKmin) - 1;
		}
		else
		{
			iNoDCCarriers = 1;
			iStartDCCar = abs(ReceiverParam.iCarrierKmin);
		}
	}

	/* Init average counter */
	lAvCnt = 0;

	/* Init start count (debar initialization of channel estimation) */
	iStartCnt = 20;

	/* Additional delay from long interleaving has to be considered */
	if (ReceiverParam.GetInterleaverDepth() == CParameter::SI_LONG)
		iStartCnt += ReceiverParam.iNoSymPerFrame * D_LENGTH_LONG_INTERL;


	/* Allocate memory for intermedia results */
	veccEstChan.Init(iNoCarrier);
	veccRefChan.Init(iNoCarrier);
	vecrMSEAverage.Init(iNoCarrier, (_REAL) 0.0); /* Reset average with zeros */

	/* Define block-sizes for inputs and output */
	iInputBlockSize = iNoCarrier;
	iInputBlockSize2 = iNoCarrier;
	iOutputBlockSize = iNoCarrier;
}

void CIdealChanEst::GetResults(CVector<_REAL>& vecrResults)
{
	vecrResults.Init(iNoCarrier, (_REAL) 0.0);

	/* Copy data in return vector */
	for (int i = 0; i < iNoCarrier; i++)
		vecrResults[i] = vecrMSEAverage[i] / lAvCnt;
}
