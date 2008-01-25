/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden, Julian Cable
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
#include "../SDC/SDC.h"
#include <iostream>

void CDecodeRSIMDI::ProcessDataInternal(CParameter& Parameters)
{
	// pass receiver parameter structure to all the decoders that need it
	TagPacketDecoderMDI.SetParameterPtr(&Parameters);

	CTagPacketDecoder::Error err = TagPacketDecoderMDI.DecodeAFPacket(*pvecInputData);

	Parameters.Lock(); 

	if(err == CTagPacketDecoder::E_OK)
	{
		Parameters.ReceiveStatus.Interface.SetStatus(RX_OK);
	}
	else
	{
		Parameters.ReceiveStatus.Interface.SetStatus(CRC_ERROR);
		Parameters.Unlock(); 
		return;
	}

	if (TagPacketDecoderMDI.TagItemDecoderRobMod.IsReady())
		Parameters.SetWaveMode(TagPacketDecoderMDI.TagItemDecoderRobMod.eRobMode);

	Parameters.Unlock(); 

	CVector<_BINARY>& vecbiFACData = TagPacketDecoderMDI.TagItemDecoderFAC.vecbidata;
	CVector<_BINARY>& vecbiSDCData = TagPacketDecoderMDI.TagItemDecoderSDC.vecbidata;
	pvecOutputData->Reset(0);
	if (TagPacketDecoderMDI.TagItemDecoderFAC.IsReady() && vecbiFACData.Size() > 0)
	{
		/* Copy incoming MDI FAC data */
		pvecOutputData->ResetBitAccess();
		vecbiFACData.ResetBitAccess();

		if(pvecOutputData->Size() != 72)
		{
			cout << "FAC not initialised?" << endl;
/*
			return;
*/
		}

		/* FAC data is always 72 bits long which is 9 bytes, copy data
		   byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE; i++)
		{
			pvecOutputData->Enqueue(vecbiFACData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
		iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	}
	else
	{
		iOutputBlockSize = 0;
		Parameters.Lock();
		Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
		Parameters.Unlock();
	}

	if (TagPacketDecoderMDI.TagItemDecoderSDCChanInf.IsReady())
	{
		CVector<_BINARY>& vecbisdciData = TagPacketDecoderMDI.TagItemDecoderSDCChanInf.vecbidata;
		// sdci not decoded later - will allow decoding/modulation before first SDC received
		CSDCReceive sdci;
		sdci.SDCIParam(&vecbisdciData, Parameters);
	}

	pvecOutputData2->Reset(0);
	const int iLenBitsMDISDCdata = vecbiSDCData.Size();
	if (TagPacketDecoderMDI.TagItemDecoderSDC.IsReady() && iLenBitsMDISDCdata > 0)
	{
		/* If receiver is correctly initialized, the input vector should be
		   large enough for the SDC data */
		const int iLenSDCDataBits = pvecOutputData2->Size();
		Parameters.SetNumDecodedBitsSDC(iLenBitsMDISDCdata);

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
		else
		{
			cout << "SDC not properly initialised ready for " << iLenSDCDataBits << " bits, got " << iLenBitsMDISDCdata << "bits" << endl;
		}
		iFramesSinceSDC = 0;
	}
	else
	{
		pvecOutputData2->Reset(0);
		iOutputBlockSize2 = 0;
		if(iFramesSinceSDC>2)
		{
			Parameters.Lock();
			Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
			Parameters.Unlock();
		}
		else
			iFramesSinceSDC++;
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
		else
		{
			cout << "MSC str" << i << " not properly initialised can accept " << iLen << " got " << iStreamLen << endl;
		}
    }

	if (TagPacketDecoderMDI.TagItemDecoderRxDemodMode.IsReady() &&
		TagPacketDecoderMDI.TagItemDecoderAMAudio.IsReady() &&
		TagPacketDecoderMDI.TagItemDecoderRxDemodMode.eMode == RM_AM)
	{
		CVector<_BINARY>& vecbiAMAudio = TagPacketDecoderMDI.TagItemDecoderAMAudio.vecbidata;
		CVector<_BINARY>* pvecOutputData = vecpvecOutputData[0];
		// Now check length of data vector
		const int iLen = pvecOutputData->Size();
		const int iStreamLen = vecbiAMAudio.Size();
		if (iLen >= iStreamLen)
		{
			// Copy data
			vecbiAMAudio.ResetBitAccess();
			pvecOutputData->ResetBitAccess();
			// Data is always a multiple of 8 -> copy bytes
			for (int j = 0; j < iStreamLen / SIZEOF__BYTE; j++)
			{
				pvecOutputData->Enqueue(
				vecbiAMAudio.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
			}
			veciOutputBlockSize[0] = iStreamLen;
		}

		/* Get the audio parameters for decoding the coded AM */
		CAudioParam AudioParam = TagPacketDecoderMDI.TagItemDecoderAMAudio.AudioParams;
		/* Write the audio settings into the parameter object
		 * CParameter takes care of keeping separate data for AM and DRM
		 */
		Parameters.Lock();
		Parameters.SetAudioParam(0, AudioParam);
			
		Parameters.SetStreamLen(0, 0, iStreamLen/SIZEOF__BYTE);
		Parameters.SetNumOfServices(1,0);
		Parameters.Service[0].iAudioStream = 0;
		Parameters.SetCurSelAudioService(0);
		Parameters.SetNumDecodedBitsMSC(iStreamLen); // is this necessary?

		Parameters.Service[0].strLabel = "";
		Parameters.Service[0].strCountryCode = "";
		Parameters.Service[0].iLanguage = 0;
		Parameters.Service[0].strLanguageCode = "";
		Parameters.Service[0].iServiceDescr = 0;
		Parameters.Service[0].iServiceID = 0;

		Parameters.Unlock();
	}

	// TODO RSCI Data Items, MER, etc.

}

void CDecodeRSIMDI::InitInternal(CParameter& Parameters)
{
	Parameters.Lock(); 

	/* set sensible values if Parameters not properly initialised */
	iOutputBlockSize = NUM_FAC_BITS_PER_BLOCK;
	iOutputBlockSize2 = Parameters.iNumSDCBitsPerSFrame;
	if(iOutputBlockSize2 == 0)
		iMaxOutputBlockSize2 = 1024;
	size_t numstreams = Parameters.Stream.size();
	if(numstreams == 0)
		numstreams = 4;
	vecpvecOutputData.resize(numstreams);
	for(size_t i=0; i<numstreams; i++)
	{
		int streamlen = Parameters.Stream[i].iLenPartA;
		streamlen += Parameters.Stream[i].iLenPartB;
		if(streamlen == 0)
			streamlen = 2048;
		veciOutputBlockSize[i] = streamlen*SIZEOF__BYTE;
	}
	iFramesSinceSDC = 3;

	Parameters.Unlock(); 
}
