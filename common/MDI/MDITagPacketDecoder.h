/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	see MDITagPacketDecoder.cpp
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

#ifndef MDI_TAG_PACKET_DECODER_H_INCLUDED
#define MDI_TAG_PACKET_DECODER_H_INCLUDED

#include "TagPacketDecoder.h"
#include "MDITagItemDecoders.h"
#include "MDIInBuffer.h"

class CTagPacketDecoderMDI : public CTagPacketDecoder
{
public:
	// constructor: adds all of the decoders in the vocabulary to the list
	CTagPacketDecoderMDI(CMDIInBuffer *pMDIBuffer);

	// Override tag packet decode routine to do extra things afterwards
	virtual void DecodeTagPacket(CVector<_BINARY>& vecbiAFPkt, const int iPayloadLen); 

private:
	CMDIInBuffer *pMDIInBuffer;

	// Packet which all of the tag decoders will write their data to.
	CMDIInPkt MDIInPkt;

	// Decoders for each of the tag items in the vocabulary
	CTagItemDecoderProTy		TagItemDecoderProTy;
	CTagItemDecoderLoFrCnt		TagItemDecoderLoFrCnt;
	CTagItemDecoderFAC			TagItemDecoderFAC;
	CTagItemDecoderSDC			TagItemDecoderSDC;
	CTagItemDecoderRobMod		TagItemDecoderRobMod;
	CTagItemDecoderStr			TagItemDecoderStr0;
	CTagItemDecoderStr			TagItemDecoderStr1;
	CTagItemDecoderStr			TagItemDecoderStr2;
	CTagItemDecoderStr			TagItemDecoderStr3;
	CTagItemDecoderSDCChanInf	TagItemDecoderSDCChanInf;
	CTagItemDecoderInfo			TagItemDecoderInfo;
};

#endif
