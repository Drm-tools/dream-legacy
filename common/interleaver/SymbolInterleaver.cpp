/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Symbol interleaver for MSC-symbols
 *	We have to support long and short symbol interleaving. Long interleaving
 *	spans over iD times iN_MUX interleaver blocks. To create a "block-wise 
 *	cycle-buffer" we shift the indices (stored in a table) each time a complete
 *	block was written. Thus, we dont need to copy data since we only modify
 *	the indices. 
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

#include "SymbolInterleaver.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Symbol interleaver														   *
\******************************************************************************/
void CSymbInterleaver::ProcessDataInternal(CParameter& TransmParam)
{
	int i, j;

	/* Write data in interleaver-memory (always index "0") */
	for (i = 0; i < iInputBlockSize; i++)
		veccInterlMemory[iCurIndex[0]][i] = (*pvecInputData)[i];

	/* Interleave data according the interleaver table. Use the
	   the interleaver-blocks described in the DRM-standard 
	   (Ro(i) = i (mod D)  -> "i % iD") */
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData)[i] = 
			veccInterlMemory[iCurIndex[i % iD]][veciIntTable[i]];

	/* Set new indices. Move blocks (virtually) forward */
	for (j = 0; j < iD; j++)
	{
		iCurIndex[j]--;

		if (iCurIndex[j] < 0)
			iCurIndex[j] = iD - 1;
	}
}

void CSymbInterleaver::InitInternal(CParameter& TransmParam)
{
	int i;

	/* Set internal parameters */
	iN_MUX = TransmParam.iNoUsefMSCCellsPerFrame;

	/* Allocate memory for table */
	veciIntTable.Init(iN_MUX);

	/* Make interleaver table */
	MakeTable(veciIntTable, iN_MUX, SYMB_INTERL_CONST_T_0);

	/* Set interleaver depth */
	switch (TransmParam.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		iD = D_LENGTH_LONG_INTERL;
		break;

	case CParameter::SI_SHORT:
		iD = D_LENGTH_SHORT_INTERL;
		break;
	}

	/* Always allocate memory for long interleaver case (Interleaver memory) */
	for (i = 0; i < D_LENGTH_LONG_INTERL; i++)
		veccInterlMemory[i].Init(iN_MUX);

	/* Index for addressing the buffers */
	for (i = 0; i < D_LENGTH_LONG_INTERL; i++)
		iCurIndex[i] = i;

	/* Define block-sizes for input and output */
	iInputBlockSize = iN_MUX;
	iOutputBlockSize = iN_MUX;

	/* Since the MSC logical frames must not end at the end of one symbol
	   (could be somewhere in the middle of the symbol), the output buffer must
	   accept more cells than one logical MSC frame is long */
	iMaxOutputBlockSize = 2 * iN_MUX;
}


/******************************************************************************\
* Symbol deinterleaver														   *
\******************************************************************************/
void CSymbDeinterleaver::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i, j;

	/* Deinterleave data according the deinterleaver table. Use the
	   the interleaver-blocks described in the DRM-standard 
	   (Ro(i) = i (mod D)  -> "i % iD") */
	for (i = 0; i < iInputBlockSize; i++)
		veccDeinterlMemory[iCurIndex[i % iD]][veciIntTable[i]] = 
			(*pvecInputData)[i];

	/* Read data from current block (index "iD - 1")*/
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData)[i] = veccDeinterlMemory[iCurIndex[iD - 1]][i];

	/* Set new indices. Move blocks forward (virtually) */
	for (j = 0; j < iD; j++)
	{
		iCurIndex[j]--;

		if (iCurIndex[j] < 0)
			iCurIndex[j] = iD - 1;
	}
}

void CSymbDeinterleaver::InitInternal(CParameter& ReceiverParam)
{
	int i;

	/* Set internal parameters */
	iN_MUX = ReceiverParam.iNoUsefMSCCellsPerFrame;

	/* Allocate memory for table */
	veciIntTable.Init(iN_MUX);

	/* Make interleaver table */
	MakeTable(veciIntTable, iN_MUX, SYMB_INTERL_CONST_T_0);

	/* Set interleaver depth */
	switch (ReceiverParam.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		iD = D_LENGTH_LONG_INTERL;
		break;

	case CParameter::SI_SHORT:
		iD = D_LENGTH_SHORT_INTERL;
		break;
	}

	/* Always allocate memory for long interleaver case (Interleaver memory) */
	for (i = 0; i < D_LENGTH_LONG_INTERL; i++)
		veccDeinterlMemory[i].Init(iN_MUX);

	/* Index for addressing the buffers */
	for (i = 0; i < D_LENGTH_LONG_INTERL; i++)
		iCurIndex[i] = i;

	/* Define block-sizes for input and output */
	iInputBlockSize = iN_MUX;
	iOutputBlockSize = iN_MUX;
}
