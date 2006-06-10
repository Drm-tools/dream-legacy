/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)  
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *	This is a class derived from CTagPacketDecoder, specialised for the MDI application.
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


#include "MDITagPacketDecoder.h"

CTagPacketDecoderMDI::CTagPacketDecoderMDI(CMDIInBuffer *pMDIBuffer)
:	pMDIInBuffer(pMDIBuffer)
,	TagItemDecoderProTy(&MDIInPkt)
,	TagItemDecoderLoFrCnt(&MDIInPkt)
,	TagItemDecoderFAC(&MDIInPkt)
,	TagItemDecoderSDC(&MDIInPkt)
,	TagItemDecoderRobMod(&MDIInPkt)
,	TagItemDecoderStr0(&MDIInPkt, 0)
,	TagItemDecoderStr1(&MDIInPkt, 1)
,	TagItemDecoderStr2(&MDIInPkt, 2)
,	TagItemDecoderStr3(&MDIInPkt, 3)
,	TagItemDecoderSDCChanInf(&MDIInPkt)
,	TagItemDecoderInfo(&MDIInPkt)
{

	// Add the tag item decoders to the base class list of decoders
	// This defines the vocabulary of this particular decoder
	AddTagItemDecoder(&TagItemDecoderProTy);
	AddTagItemDecoder(&TagItemDecoderLoFrCnt);
	AddTagItemDecoder(&TagItemDecoderFAC);
	AddTagItemDecoder(&TagItemDecoderSDC);
	AddTagItemDecoder(&TagItemDecoderRobMod);
	AddTagItemDecoder(&TagItemDecoderStr0);
	AddTagItemDecoder(&TagItemDecoderStr1);
	AddTagItemDecoder(&TagItemDecoderStr2);
	AddTagItemDecoder(&TagItemDecoderStr3);
	AddTagItemDecoder(&TagItemDecoderSDCChanInf);
	AddTagItemDecoder(&TagItemDecoderInfo);
}

/* overridden decode function - calls the base class decoder and then puts the resulting 
 * packet into the input buffer */
void CTagPacketDecoderMDI::DecodeTagPacket(CVector<_BINARY>& vecbiPkt, const int iPayloadLen)
{

	CTagPacketDecoder::DecodeTagPacket(vecbiPkt, iPayloadLen);

	pMDIInBuffer->Put(MDIInPkt);
}
