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
	/* Define block-size for input, an audio frame always corresponds to 400ms. 
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

	/* Stop simulation if stop condition is true */
	iCounter++;
	switch (eCntType)
	{
	case CT_TIME:
		if (iCounter == iNoSimBlocks)
		{
			TransmParam.bRunThread = FALSE;
			iCounter = 0;
		}
		break;

	case CT_ERRORS:
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
	iShiftRegister = 12345;

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
		/* AWGN: No fading, 30 seconds should be ok */
		iMinNoBlocks = (int) ((_REAL) 30.0 / (_REAL) 0.4);
		break;

	case 2:
		/* Rice with delay: 0.1 Hz -> ~10 sec / period -> 500 sec */
		iMinNoBlocks = (int) ((_REAL) 500.0 / (_REAL) 0.4);
		break;

	case 3:
		/* US Consortium: same as case "2" */
		iMinNoBlocks = (int) ((_REAL) 500.0 / (_REAL) 0.4);
		break;

	case 4:
		/* CCIR Poor: 1 Hz -> 60 seconds should be ok */
		iMinNoBlocks = (int) ((_REAL) 60.0 / (_REAL) 0.4);
		break;

	case 5:
		/* Channel no 5: 2 Hz -> 30 sec */
		iMinNoBlocks = (int) ((_REAL) 30.0 / (_REAL) 0.4);
		break;

	case 6:
		/* Channel no 6: same as case "2" */
		iMinNoBlocks = (int) ((_REAL) 500.0 / (_REAL) 0.4);
		break;
	}
}

void CGenSimData::SetSimTime(int iNewTi) 
{
	/* One MSC frame is 400 ms long */
	iNoSimBlocks = (int) ((_REAL) iNewTi /* sec */ / (_REAL) 0.4);

	/* Set simulation count type */
	eCntType = CT_TIME;

	/* Reset counter */
	iCounter = 0;
}

void CGenSimData::SetNoErrors(int iNewNE)
{
	iNoErrors = iNewNE;

	/* Set simulation count type */
	eCntType = CT_ERRORS;

	/* Reset counter, because we also look at the time */
	iCounter = 0;
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
	iShiftRegister = 12345;

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
	if (iIniCnt > 5)
	{
		rAccBitErrRate += (_REAL) iNoBitErrors / iInputBlockSize;
		iNoAccBitErrRate++;

		ReceiverParam.rBitErrRate = rAccBitErrRate / iNoAccBitErrRate;
		ReceiverParam.iNoBitErrors += iNoBitErrors;
	}
	else
		iIniCnt++;
}

void CEvaSimData::InitInternal(CParameter& ReceiverParam)
{
	/* Reset bit error rate parameters */
	rAccBitErrRate = (_REAL) 0.0;
	iNoAccBitErrRate = 0;
	iIniCnt = 0;

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
	bCRCOk = FACReceive.FACParam(pvecInputData, ReceiverParam);

	if (bCRCOk)
		PostWinMessage(MS_FAC_CRC, 0);
	else
		PostWinMessage(MS_FAC_CRC, 2);
}

void CUtilizeFACData::InitInternal(CParameter& ReceiverParam)
{

// This should be in FAC class in an Init() routine which has to be defined, this
// would be cleaner code! TODO
/* Init frame ID */
ReceiverParam.iFrameIDReceiv = 0;

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
void CEvalChanEst::ProcessDataInternal(CParameter& ReceiverParam)
{
	int iChanEstDelay;
	int i, j;

	/* Get delay of channel estimation (must not be longer than symbol number
	   per frame, otherwise we get wraps which are not evaluated here!) */
	iChanEstDelay = (*pvecInputData2).GetExData().iSymbolNo - 
		(*pvecInputData).GetExData().iSymbolNo + 1;
	if (iChanEstDelay < 0)
		iChanEstDelay += iNoSymPerFrame;


	/* History for OFDM demodulation output --------------------------------- */
	/* Move data in history-buffer (from iLenHistBuff - 1 towards 0) */
	for (j = 0; j < iChanEstDelay - 1; j++)
	{
		for (i = 0; i < iNoCarrier; i++)
			matcHistOFDMDem[j][i] = matcHistOFDMDem[j + 1][i];
	}

	/* Write new symbol in memory */
	for (i = 0; i < iNoCarrier; i++)
		matcHistOFDMDem[iChanEstDelay - 1][i] = (*pvecInputData2)[i];


	/* Calculation of channel tranfer function ------------------------------ */
	/*	pvecInputData: ChanEstBuf
		pvecInputData2: OFDMDemodBuf2
		pvecInputData3: DemChanInRefBuf
		pvecInputData4: DemChanRefBuf */
	for (i = 0; i < iNoCarrier; i++)
	{
		veccEstChan[i] = matcHistOFDMDem[0][i] / (*pvecInputData)[i];
		veccRefChan[i] = (*pvecInputData4)[i] / (*pvecInputData3)[i];
	}

	/* Debar DC carriers, set them to zero */
	for (i = 0; i < iNoDCCarriers; i++)
	{
		veccEstChan[i + iStartDCCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
		veccRefChan[i + iStartDCCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
	}


	/* History for channel reference ---------------------------------------- */
	/* Move data in history-buffer (from iLenHistBuff - 1 towards 0) */
	for (j = 0; j < iChanEstDelay - 1; j++)
	{
		for (i = 0; i < iNoCarrier; i++)
			matcHistRefChan[j][i] = matcHistRefChan[j + 1][i];
	}

	/* Write new symbol in memory */
	for (i = 0; i < iNoCarrier; i++)
		matcHistRefChan[iChanEstDelay - 1][i] = veccRefChan[i];


	/* Start evaluation results after exceeding the start count */
	if (iStartCnt > 10)
	{
		/* MSE for all carriers --------------------------------------------- */
		for (i = 0; i < iNoCarrier; i++)
		{
			vecrMSE[i] = abs(veccEstChan[i] - matcHistRefChan[0][i]) *
				abs(veccEstChan[i] - matcHistRefChan[0][i]);
		
			/* Average results */
			vecrMSEAverage[i] += vecrMSE[i];
		}

		/* New values have been added, increase counter for final result 
		   calculation */
		lAvCnt++;
	}
	else
		iStartCnt++;
}

void CEvalChanEst::GetResults(CVector<_REAL>& vecrResults)
{
	vecrResults.Init(iNoCarrier, (_REAL) 0.0);

	/* Copy data in return vector */
	for (int i = 0; i < iNoCarrier; i++)
		vecrResults[i] = vecrMSEAverage[i] / lAvCnt;
}

void CEvalChanEst::InitInternal(CParameter& ReceiverParam)
{
	/* Init start count (do not use first blocks) */
	iStartCnt = 0;

	/* Get local parameters */
	iNoCarrier = ReceiverParam.iNoCarrier;
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;

	/* Parameters for debaring the DC carriers from evaluation */
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

	/* Allocate memory for histories */
	matcHistOFDMDem.Init(iNoSymPerFrame, iNoCarrier);
	matcHistRefChan.Init(iNoSymPerFrame, iNoCarrier);

	/* Allocate memory for intermedia results */
	veccEstChan.Init(iNoCarrier);
	veccRefChan.Init(iNoCarrier);
	vecrMSE.Init(iNoCarrier);
	vecrMSEAverage.Init(iNoCarrier, (_REAL) 0.0); // Reset average with zeros

	/* Init average counter */
	lAvCnt = 0;

	/* Define block-sizes for inputs */
	iInputBlockSize = iNoCarrier;
	iInputBlockSize2 = iNoCarrier;
	iInputBlockSize3 = iNoCarrier;
	iInputBlockSize4 = iNoCarrier;
}

void CDataConv::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* This module is used for conversion of "CEquSig" data into "_COMPLEX"
	   data */
	for (int i = 0; i < iOutputBlockSize; i++)
		(*pvecOutputData)[i] = (*pvecInputData)[i].cSig;
}

void CDataConv::InitInternal(CParameter& ReceiverParam)
{
	iInputBlockSize = ReceiverParam.iNoCarrier;
	iOutputBlockSize = ReceiverParam.iNoCarrier;
}
