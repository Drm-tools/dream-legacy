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

#include "MDIDecode.h"
#include <iostream>

void CDecodeRSIMDI::ProcessDataInternal(CParameter& ReceiverParam)
{
	CTagPacketDecoder::Error err = 
							TagPacketDecoderMDI.DecodeAFPacket(*pvecInputData);
	if(err == CTagPacketDecoder::E_OK)
	{
		ReceiverParam.ReceiveStatus.SetInterfaceStatus(RX_OK);
	}
	else
	{
		ReceiverParam.ReceiveStatus.SetInterfaceStatus(CRC_ERROR);
		return;
	}
	ReceiverParam.SetWaveMode(TagPacketDecoderMDI.TagItemDecoderRobMod.eRobMode);
	CVector<_BINARY>& vecbiFACData = TagPacketDecoderMDI.TagItemDecoderFAC.vecbidata;
	CVector<_BINARY>& vecbiSDCData = TagPacketDecoderMDI.TagItemDecoderSDC.vecbidata;
	pvecOutputData->Reset(0);
	if (vecbiFACData.Size() > 0)
	{
		/* Copy incoming MDI FAC data */
		pvecOutputData->ResetBitAccess();
		vecbiFACData.ResetBitAccess();

		/* FAC data is always 72 bits long which is 9 bytes, copy data
		   byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE; i++)
		{
			pvecOutputData->Enqueue(vecbiFACData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
		iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	}

	pvecOutputData2->Reset(0);
	const int iLenBitsMDISDCdata = vecbiSDCData.Size();
	if (iLenBitsMDISDCdata > 0)
	{
		/* If receiver is correctly initialized, the input vector should be
		   large enough for the SDC data */
		const int iLenSDCDataBits = pvecOutputData2->Size();
		ReceiverParam.SetNumDecodedBitsSDC(iLenBitsMDISDCdata);

		if (iLenSDCDataBits >= iLenBitsMDISDCdata)
		{
			/* Copy incoming MDI SDC data */
			pvecOutputData2->ResetBitAccess();
            vecbiSDCData.ResetBitAccess();

			/* We have to copy bits instead of bytes since the length of SDC
			   data is usually not a multiple of 8 */
			for (int i = 0; i < iLenBitsMDISDCdata; i++)
				pvecOutputData2->Enqueue(vecbiSDCData.Separate(1), 1);

			iOutputBlockSize2 = iLenBitsMDISDCdata;
		}
	}
	else
	{
		pvecOutputData2->Reset(0);
    }

	/* Get stream data from received RSCI / MDI packets */
	for(size_t i=0; i<vecpvecOutputData.size(); i++)
	{
		CVector<_BINARY>& vecbiStr = TagPacketDecoderMDI.TagItemDecoderStr[i].vecbidata;
		CVector<_BINARY>* pvecOutputData = vecpvecOutputData[i];
		/* Now check length of data vector */
		const int iLen = pvecOutputData->Size();
		const int iStreamLen = vecbiStr.Size();
		if (iLen >= iStreamLen)
		{
			/* Copy data */
			vecbiStr.ResetBitAccess();
			pvecOutputData->ResetBitAccess();

			/* Data is always a multiple of 8 -> copy bytes */
			for (int j = 0; j < iStreamLen / SIZEOF__BYTE; j++)
			{
				pvecOutputData->Enqueue(
					vecbiStr.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
			}
			veciOutputBlockSize[i] = iStreamLen;
		}
    }

	// TODO RSCI Data Items, MER, etc.
}

void CDecodeRSIMDI::InitInternal(CParameter& ReceiverParam)
{
		iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
		//iOutputBlockSize2 = ReceiverParam.iNumSDCBitsPerSFrame;
		iMaxOutputBlockSize2 = 1024;
		size_t numstreams = ReceiverParam.Stream.Size();
		//vecpvecOutputData.resize(numstreams);
		for(size_t i=0; i<numstreams; i++)
		{
			int streamlen = ReceiverParam.Stream[i].iLenPartA;
			streamlen += ReceiverParam.Stream[i].iLenPartB;
			//veciMaxOutputBlockSize[i] = 16384;
			veciOutputBlockSize[i] = streamlen*SIZEOF__BYTE;
		}
}
