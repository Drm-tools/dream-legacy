/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Mapping of the symbols on the carriers
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

#include "OFDMCellMapping.h"


/* Implementation *************************************************************/
/******************************************************************************\
* OFDM cells mapping														   *
\******************************************************************************/
void COFDMCellMapping::ProcessDataInternal(CParameter& TransmParam)
{
	int iCar;
	int iMSCCounter;
	int iFACCounter;
	int iSDCCounter;
	int iDummyCellCounter;
	int iSymbolCounterAbs;

	/* Mapping of the data and pilot cells on the OFDM symbol --------------- */
	/* Set absolute symbol position */
	iSymbolCounterAbs = 
		TransmParam.iFrameIDTransm * iNoSymPerFrame + iSymbolCounter;

	/* Init temporary counter */
	iDummyCellCounter = 0;
	iMSCCounter = 0;
	iFACCounter = 0;
	iSDCCounter = 0;
	for (iCar = 0; iCar < iNoCarrier; iCar++)
	{
		/* MSC */
		if (_IsMSC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			if (iMSCCounter >= TransmParam.veciNoMSCSym[iSymbolCounterAbs])
			{
				/* Insert dummy cells */
				(*pvecOutputData)[iCar] = pcDummyCells[iDummyCellCounter];

				iDummyCellCounter++;
			}
			else
				(*pvecOutputData)[iCar] = (*pvecInputData)[iMSCCounter];
				
			iMSCCounter++;
		}

		/* FAC */
		if (_IsFAC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData)[iCar] = (*pvecInputData2)[iFACCounter];
				
			iFACCounter++;
		}

		/* SDC */
		if (_IsSDC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData)[iCar] = (*pvecInputData3)[iSDCCounter];
				
			iSDCCounter++;
		}

		/* Pilots */
		if (_IsPilot(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
			(*pvecOutputData)[iCar] = 
				TransmParam.matcPilotCells[iSymbolCounterAbs][iCar];

		/* DC carrier */
		if (_IsDC(TransmParam.matiMapTab[iSymbolCounterAbs][iCar]))
			(*pvecOutputData)[iCar] = _COMPLEX((_REAL) 0.0, (_REAL) 0.0);
	}

	/* Increase symbol-counter and wrap if needed */
	iSymbolCounter++;
	if (iSymbolCounter == iNoSymPerFrame)
	{
		iSymbolCounter = 0;

		/* Increase frame-counter (ID) (Used also in FAC.cpp) */
		TransmParam.iFrameIDTransm++;
		if (TransmParam.iFrameIDTransm == NO_FRAMES_IN_SUPERFRAME)
			TransmParam.iFrameIDTransm = 0;
	}

	/* Set absolute symbol position (for updated relative positions) */
	iSymbolCounterAbs = 
		TransmParam.iFrameIDTransm * iNoSymPerFrame + iSymbolCounter;

	/* Set input block-sizes for next symbol */
	iInputBlockSize = TransmParam.veciNoMSCSym[iSymbolCounterAbs];
	iInputBlockSize2 = TransmParam.veciNoFACSym[iSymbolCounterAbs];
	iInputBlockSize3 = TransmParam.veciNoSDCSym[iSymbolCounterAbs];
}

void COFDMCellMapping::InitInternal(CParameter& TransmParam)
{
	iNoSymPerFrame = TransmParam.iNoSymPerFrame;
	iNoCarrier = TransmParam.iNoCarrier;

	/* Init symbol-counter */
	iSymbolCounter = 0;

	/* Init frame ID */
	TransmParam.iFrameIDTransm = 0;

	/* Choose right dummy cells for MSC QAM scheme */
	switch (TransmParam.eMSCCodingScheme)
	{
	case CParameter::CS_2_SM:
		pcDummyCells = (_COMPLEX*) &cDummyCells16QAM[0];
		break;

	case CParameter::CS_3_SM:
	case CParameter::CS_3_HMSYM:
	case CParameter::CS_3_HMMIX:
		pcDummyCells = (_COMPLEX*) &cDummyCells64QAM[0];
		break;
	}

	/* Define block-sizes for input and output of the module ---------------- */
	/* MSC */
	iInputBlockSize = TransmParam.veciNoMSCSym[0];
	/* FAC */
	iInputBlockSize2 = TransmParam.veciNoFACSym[0];
	/* SDC */
	iInputBlockSize3 = TransmParam.veciNoSDCSym[0];
	/* Output */
	iOutputBlockSize = TransmParam.iNoCarrier;
}


/******************************************************************************\
* OFDM cells demapping														   *
\******************************************************************************/
void COFDMCellDemapping::ProcessDataInternal(CParameter& ReceiverParam)
{
	int iCar;
	int iMSCCounter;
	int iSDCCounter;
	int iFACCounter;
	int iSymbolCounterAbs;

	/* Set absolute symbol position */
	iSymbolCounterAbs = iCurrentFrameID * iNoSymPerFrame + iSymbolCounter;

	/* Set output block-sizes for this symbol */
	iOutputBlockSize = ReceiverParam.veciNoMSCSym[iSymbolCounterAbs];
	iOutputBlockSize2 = ReceiverParam.veciNoFACSym[iSymbolCounterAbs];
	iOutputBlockSize3 = ReceiverParam.veciNoSDCSym[iSymbolCounterAbs];

	/* Demap data from the cells */
	iMSCCounter = 0;
	iFACCounter = 0;
	iSDCCounter = 0;
	for (iCar = 0; iCar < iNoCarrier; iCar++)
	{
		/* MSC */
		if (_IsMSC(ReceiverParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			/* Ignore dummy cells */
			if (iMSCCounter < ReceiverParam.veciNoMSCSym[iSymbolCounterAbs])
			{
				(*pvecOutputData)[iMSCCounter] = (*pvecInputData)[iCar];

				iMSCCounter++; /* Local counter */
				iMSCCellCounter++; /* Super-frame counter */
			}
		}

		/* FAC */
		if (_IsFAC(ReceiverParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData2)[iFACCounter] = (*pvecInputData)[iCar];
				
			iFACCounter++; /* Local counter */
			iFACCellCounter++; /* Super-frame counter */
		}

		/* SDC */
		if (_IsSDC(ReceiverParam.matiMapTab[iSymbolCounterAbs][iCar]))
		{
			(*pvecOutputData3)[iSDCCounter] = (*pvecInputData)[iCar];
				
			iSDCCounter++; /* Local counter */
			iSDCCellCounter++; /* Super-frame counter */
		}
	}

	/* Get symbol-counter for next symbol and adjust frame-ID. Use the extended
	   data, shipped with the input vector */
	iSymbolCounter = (*pvecInputData).GetExData().iSymbolNo + 1;

	/* Check range (iSymbolCounter must be in {0, ... , iNoSymPerFrame} */
	while (iSymbolCounter >= iNoSymPerFrame)
		iSymbolCounter -= iNoSymPerFrame;
	while (iSymbolCounter < 0)
		iSymbolCounter += iNoSymPerFrame;

	/* If frame bound is reached, update frame ID from FAC stream */
	if (iSymbolCounter == 0)
	{
		/* Check, if number of FAC cells is correct. If not, reset 
		   output cyclic-buffer. An incorrect number of FAC cells can be if
		   the "iSymbolCounterAbs" was changed, e.g. by the synchronization
		   units */
		if (iFACCellCounter != NO_FAC_CELLS)
			SetBufReset2(); /* FAC: buffer number 2 */

		/* Reset FAC cell counter */
		iFACCellCounter = 0;

		/* Frame ID of this FAC block stands for the "current" block. We need
		   the ID of the next block, therefore we have to add "1" */
		iCurrentFrameID = ReceiverParam.iFrameIDReceiv + 1;

		if (iCurrentFrameID == NO_FRAMES_IN_SUPERFRAME)
			iCurrentFrameID = 0;

		if (iCurrentFrameID == 0)
		{
			/* Super-frame bound reached, test cell-counters (same as with the
			   FAC cells, see above) */
			if (iMSCCellCounter != iNoUsefMSCCellsPerFrame * 
				NO_FRAMES_IN_SUPERFRAME)
			{
				SetBufReset1(); /* MSC: buffer number 1 */
			}
			if (iSDCCellCounter != iNoSDCCellsPerSFrame)
				SetBufReset3(); /* SDC: buffer number 3 */

			/* Reset counters */
			iMSCCellCounter = 0;
			iSDCCellCounter = 0;
		}
	}
}

void COFDMCellDemapping::InitInternal(CParameter& ReceiverParam)
{
	iNoSymPerFrame = ReceiverParam.iNoSymPerFrame;
	iNoCarrier = ReceiverParam.iNoCarrier;
	iNoUsefMSCCellsPerFrame = ReceiverParam.iNoUsefMSCCellsPerFrame;
	iNoSDCCellsPerSFrame = ReceiverParam.iNoSDCCellsPerSFrame;

	/* Init symbol-counter and internal frame counter */
	iSymbolCounter = 0;
	iCurrentFrameID = 0;

	/* Init cell-counter */
	iSDCCellCounter = 0;
	iMSCCellCounter = 0;
	iFACCellCounter = 0;

	/* Define block-sizes for input and output of the module ---------------- */
	/* Input */
	iInputBlockSize = iNoCarrier;

	/* Define space for output cyclic buffers. We must consider enough headroom
	   otherwise the buffers could overrun */
	/* FAC, one block is exactly finished when last symbol with FAC cells is 
	   processed */
	iMaxOutputBlockSize2 = NO_FAC_BITS_PER_BLOCK;
	/* SDC, one block is exactly finished when last symbol with SDC cells is 
	   processed */
	iMaxOutputBlockSize3 = ReceiverParam.iNoSDCCellsPerSFrame;
	/* MSC, since the MSC logical frames must not end at the end of one symbol
	   (could be somewhere in the middle of the symbol), the output buffer must
	   accept more cells than one logical MSC frame is long. The worst case is
	   that the block ends right at the beginning of one symbol; in this case we
	   have an overhang of approximately one symbol of MSC cells */
	iMaxOutputBlockSize = ReceiverParam.iNoUsefMSCCellsPerFrame + 
		ReceiverParam.iMaxNoMSCSym;
}
