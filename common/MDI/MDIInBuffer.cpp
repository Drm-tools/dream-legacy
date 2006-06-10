/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)  
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *	This module implements a buffer for decoded Digital Radio Mondiale (DRM) 
 *  Multiplex Distribution Interface (MDI) packets at the receiver input.
 *	
 *	see ETSI TS 102 820 and ETSI TS 102 821.
 *
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

#include "MDIInBuffer.h"

_BOOLEAN CMDIInBuffer::Put(const CMDIInPkt& nMDIData)
{
	/* Buffer is full, return error */
	if (vecMDIDataBuf.GetFillLevel() == MDI_IN_BUF_LEN)
	{
		Reset();
		return FALSE;
	}

	/* Put data in three steps (as required by cyclic buffer) */
	CVectorEx<CMDIInPkt>* pInData = vecMDIDataBuf.QueryWriteBuffer();
	(*pInData)[0] = nMDIData;
	vecMDIDataBuf.Put(1);

	return TRUE;
}

_BOOLEAN CMDIInBuffer::Get(CMDIInPkt& nMDIData)
{
	/* Buffer is empty, return error */
	if (vecMDIDataBuf.GetFillLevel() == 0)
	{
		/* Reset return data and buffer */
		nMDIData.Reset();
		Reset();

		return FALSE;
	}

	/* Cyclic buffer always returns data in a vector */
	CVectorEx<CMDIInPkt>* pInData = vecMDIDataBuf.Get(1);
	nMDIData = (*pInData)[0];

	return TRUE;
}

void CMDIInBuffer::Reset()
{
	vecMDIDataBuf.Clear();
	
	/* Set the buffer pointers maximum far apart */
	for (int i = 0; i < MDI_IN_BUF_LEN / 2; i++)
	{
		CVectorEx<CMDIInPkt>* pInData = vecMDIDataBuf.QueryWriteBuffer();
		(*pInData)[0].Reset(); /* Empty object */
		vecMDIDataBuf.Put(1);
	}
}


/* MDI in buffer implementation ***********************************************/
CMDIInPkt& CMDIInPkt::operator=(const CMDIInPkt& nMDI)
{
	/* First initialize the vectors with the correct size, then copy the data */
	vecbiFACData.Init(nMDI.vecbiFACData.Size());
	vecbiFACData = nMDI.vecbiFACData;

	vecbiSDCData.Init(nMDI.vecbiSDCData.Size());
	vecbiSDCData = nMDI.vecbiSDCData;

	vecbiStr.Init(MAX_NUM_STREAMS);
	for (int i = 0; i < MAX_NUM_STREAMS; i++)
	{
		vecbiStr[i].Init(nMDI.vecbiStr[i].Size());
		vecbiStr[i] = nMDI.vecbiStr[i];
	}

	eRobMode = nMDI.eRobMode;

	return *this;
}

void CMDIInPkt::Reset()
{
	vecbiFACData.Init(0);
	vecbiSDCData.Init(0);
	eRobMode = RM_ROBUSTNESS_MODE_B;

	for (int i = 0; i < MAX_NUM_STREAMS; i++)
		vecbiStr[i].Init(0);
}
