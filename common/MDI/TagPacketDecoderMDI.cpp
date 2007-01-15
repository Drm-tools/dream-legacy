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


#include "TagPacketDecoderMDI.h"

CTagPacketDecoderMDI::CTagPacketDecoderMDI()
:	TagItemDecoderProTy()
,	TagItemDecoderLoFrCnt()
,	TagItemDecoderFAC()
,	TagItemDecoderSDC()
,	TagItemDecoderRobMod()
,	TagItemDecoderStr()
,	TagItemDecoderSDCChanInf()
,	TagItemDecoderInfo()
{

	// Add the tag item decoders to the base class list of decoders
	// This defines the vocabulary of this particular decoder
	AddTagItemDecoder(&TagItemDecoderProTy);
	AddTagItemDecoder(&TagItemDecoderLoFrCnt);
	AddTagItemDecoder(&TagItemDecoderFAC);
	AddTagItemDecoder(&TagItemDecoderSDC);
	AddTagItemDecoder(&TagItemDecoderRobMod);
	TagItemDecoderStr.resize(MAX_NUM_STREAMS);
	for(int i=0; i<MAX_NUM_STREAMS; i++)
	{
		TagItemDecoderStr[i].iStreamNumber = i;
        AddTagItemDecoder(&TagItemDecoderStr[i]);
	}
	AddTagItemDecoder(&TagItemDecoderSDCChanInf);
	AddTagItemDecoder(&TagItemDecoderInfo);
}
