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

const size_t fac_ = 0;
const size_t sdc_ = 1;
const size_t str = 2;

void CDecodeRSIMDI::ProcessData(CParameter& Parameters,
				CInputStruct<_BINARY>* inputs,
				COutputStruct<_BINARY>* outputs)
{
cerr << "CDecodeRSIMDI::ProcessData" << endl;
	// pass receiver parameter structure to all the decoders that need it
	TagPacketDecoder.SetParameterPtr(&Parameters);

	CTagPacketDecoder::Error err = TagPacketDecoder.DecodeAFPacket(*inputs[0].pvecData);

	Parameters.Lock(); 

	if(err == CTagPacketDecoder::E_OK)
	{
	cerr << "got AF packet" << endl;
		Parameters.ReceiveStatus.Interface.SetStatus(RX_OK);
	}
	else
	{
	cerr << "AF packet CRC error" << endl;
		Parameters.ReceiveStatus.Interface.SetStatus(CRC_ERROR);
		Parameters.Unlock(); 
		return;
	}

	if (TagPacketDecoder.TagItemDecoderRobMod.IsReady())
	{
		Parameters.SetWaveMode(TagPacketDecoder.TagItemDecoderRobMod.eRobMode);
	}

	Parameters.Unlock(); 

	CVector<_BINARY>& vecbiFACData = TagPacketDecoder.TagItemDecoderFAC.vecbidata;
	CVector<_BINARY>& vecbiSDCData = TagPacketDecoder.TagItemDecoderSDC.vecbidata;
	outputs[fac_].pvecData->Reset(0);
	if (TagPacketDecoder.TagItemDecoderFAC.IsReady() && vecbiFACData.Size() > 0)
	{
		/* Copy incoming FAC data */
		outputs[fac_].pvecData->ResetBitAccess();
		vecbiFACData.ResetBitAccess();

		if(outputs[fac_].pvecData->Size() != 72)
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
			outputs[fac_].pvecData->Enqueue(vecbiFACData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
		outputs[fac_].iBlockSize = NUM_FAC_BITS_PER_BLOCK;
	}
	else
	{
		outputs[fac_].iBlockSize = 0;
		Parameters.Lock();
		Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
		Parameters.Unlock();
	}

	if (TagPacketDecoder.TagItemDecoderSDCChanInf.IsReady())
	{
		CVector<_BINARY>& vecbisdciData = TagPacketDecoder.TagItemDecoderSDCChanInf.vecbidata;
		// sdci not decoded later - will allow decoding/modulation before first SDC received
		CSDCReceive sdci;
		sdci.SDCIParam(&vecbisdciData, Parameters);
	}

	outputs[sdc_].pvecData->Reset(0);
	const int iLenBitsMDISDCdata = vecbiSDCData.Size();
	if (TagPacketDecoder.TagItemDecoderSDC.IsReady() && iLenBitsMDISDCdata > 0)
	{
		/* If receiver is correctly initialized, the input vector should be
		   large enough for the SDC data */
		const int iLenSDCDataBits = outputs[sdc_].pvecData->Size();
		Parameters.SetNumDecodedBitsSDC(iLenBitsMDISDCdata);

		if (iLenSDCDataBits >= iLenBitsMDISDCdata)
		{
			/* Copy incoming MDI SDC data */
			outputs[sdc_].pvecData->ResetBitAccess();
            vecbiSDCData.ResetBitAccess();

			/* We have to copy bits instead of bytes since the length of SDC
			   data is usually not a multiple of 8 */
			for (int i = 0; i < iLenBitsMDISDCdata; i++)
				outputs[sdc_].pvecData->Enqueue(vecbiSDCData.Separate(1), 1);

			outputs[sdc_].iBlockSize = iLenBitsMDISDCdata;
		}
		else
		{
			cout << "SDC not properly initialised ready for " << iLenSDCDataBits << " bits, got " << iLenBitsMDISDCdata << "bits" << endl;
		}
		iFramesSinceSDC = 0;
	}
	else
	{
		outputs[sdc_].pvecData->Reset(0);
		outputs[sdc_].iBlockSize = 0;
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
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		CVector<_BINARY>& vecbiStr = TagPacketDecoder.TagItemDecoderStr[i].vecbidata;
		CVector<_BINARY>* pvecOutputData = outputs[str+i].pvecData;
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
			outputs[str+i].iBlockSize = iStreamLen;
		}
		else
		{
			cout << "MSC str" << i << " not properly initialised can accept " << iLen << " got " << iStreamLen << endl;
		}
    }

	if (TagPacketDecoder.TagItemDecoderRxDemodMode.IsReady() &&
		TagPacketDecoder.TagItemDecoderAMAudio.IsReady() &&
		TagPacketDecoder.TagItemDecoderRxDemodMode.eMode == RM_AM)
	{
		CVector<_BINARY>& vecbiAMAudio = TagPacketDecoder.TagItemDecoderAMAudio.vecbidata;
		CVector<_BINARY>* pvecOutputData = outputs[str].pvecData;
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
			outputs[str].iBlockSize = iStreamLen;
		}

		/* Get the audio parameters for decoding the coded AM */
		CAudioParam AudioParam = TagPacketDecoder.TagItemDecoderAMAudio.AudioParams;
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

void CDecodeRSIMDI::Init(CParameter&)
{
	iFramesSinceSDC = 3;
}

void CDecodeRSI::InitInternal(CParameter& Parameters)
{
	Decoder.Init(Parameters);
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
	veciOutputBlockSize.resize(numstreams);
	for(size_t i=0; i<numstreams; i++)
	{
		int streamlen = Parameters.GetStreamLen(i);
		if(streamlen == 0)
			streamlen = 2048;
		veciOutputBlockSize[i] = streamlen*SIZEOF__BYTE;
	}

	Parameters.Unlock(); 
}

void CDecodeRSI::ProcessDataInternal(CParameter& Parameters)
{
	CInputStruct<_BINARY> inputs;
	COutputStruct<_BINARY> outputs[2+MAX_NUM_STREAMS];
	inputs.iBlockSize = iInputBlockSize;
	inputs.pvecData = pvecInputData;
	outputs[fac_].iBlockSize = iOutputBlockSize;
	outputs[fac_].iMaxBlockSize = iMaxOutputBlockSize;
	outputs[fac_].pvecData = pvecOutputData;
	outputs[sdc_].iBlockSize = iOutputBlockSize2;
	outputs[sdc_].iMaxBlockSize = iMaxOutputBlockSize2;
	outputs[sdc_].pvecData = pvecOutputData2;
	for(size_t i=0; i<vecpvecOutputData.size(); i++)
	{
		outputs[str+i].iBlockSize = veciOutputBlockSize[i];
		outputs[str+i].iMaxBlockSize = veciMaxOutputBlockSize[i];
		outputs[str+i].pvecData = vecpvecOutputData[i];
	}
	Decoder.ProcessData(Parameters, &inputs, outputs);
}

void CDecodeMDI::InitInternal(CParameter& Parameters)
{
cerr << "CDecodeMDI::InitInternal" << endl;
	Decoder.Init(Parameters);
	Parameters.Lock(); 

	outputs[fac_].iBlockSize = NUM_FAC_BITS_PER_BLOCK;
	outputs[sdc_].iBlockSize = Parameters.iNumSDCBitsPerSFrame;
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		int streamlen = Parameters.GetStreamLen(i);
		outputs[str+i].iBlockSize = streamlen*SIZEOF__BYTE;
	}
	Parameters.Unlock(); 
	inputs[0].iBlockSize = 1; // to try and get some input
}

void CDecodeMDI::ProcessDataInternal(CParameter& Parameters)
{
cerr << "CDecodeMDI::ProcessDataInternal" << endl;
	iConsumed[0] = inputs[0].pvecData->Size() - inputs[0].iBlockSize;
	Decoder.ProcessData(Parameters, inputs, outputs);
}
