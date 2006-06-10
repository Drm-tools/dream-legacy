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
 *  This module derives, from the CTagItemDecoder base class, tag item decoders specialised to decode each of the tag
 *  items defined in MDI. 
 *  They generally write the decoded data into the CMDIPacket object which they hold a
 *  pointer to.
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


#include "MDITagItemDecoders.h"
#include "MDIInBuffer.h"
#include "../Parameter.h"

string CTagItemDecoderProTy::GetTagName(void) {return "*ptr";}
void CTagItemDecoderProTy::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
/*
	Changes to the protocol which will allow existing decoders to still function
	will be represented by an increment of the minor version number only. Any
	new features added by the change will obviously not need to be supported by
	older modulators. Existing TAG Items will not be altered except for the
	definition of bits previously declared Rfu. New TAG Items may be added.

	Changes to the protocol which will render previous implementations unable to
	correctly process the new format will be represented by an increment of the
	major version number. Older implementations should not attempt to decode
	such MDI packets. Changes may include modification to or removal of existing
	TAG Item definitions.
*/

	/* Protocol type and revision (*ptr) always 8 bytes long */
	if (iLen != 64)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* Decode protocol type (32 bits = 4 bytes) */
	string strProtType = "";
	for (int i = 0; i < 4 /* bytes */; i++)
		strProtType += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);

	if (strProtType.compare("DMDI") != 0)
	{
		// TODO
	}

	/* Get major and minor revision of protocol */
	const int iMdiMajRev = (int) vecbiTag.Separate(16);
	const int iMdiMinRev = (int) vecbiTag.Separate(16);
}

string CTagItemDecoderLoFrCnt::GetTagName(void) {return "dlfc";}
void CTagItemDecoderLoFrCnt::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* DRM logical frame count (dlfc) always 4 bytes long */
	if (iLen != 32)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	int iLogFraCnt = (int) vecbiTag.Separate(32);

// TODO:
/* Do something with the count */
}



string CTagItemDecoderFAC::GetTagName(void) {return "fac_";}
void CTagItemDecoderFAC::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Fast access channel (fac_) always 9 bytes long */
	if (iLen != NUM_FAC_BITS_PER_BLOCK)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* Copy incoming FAC data */
	pMDIInPkt->vecbiFACData.Init(NUM_FAC_BITS_PER_BLOCK);
	pMDIInPkt->vecbiFACData.ResetBitAccess();

	for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE; i++)
	{
		pMDIInPkt->vecbiFACData.
			Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
	}
}


string CTagItemDecoderSDC::GetTagName(void) {return "sdc_";}
void CTagItemDecoderSDC::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Check that this is not a dummy packet with zero length */
	if (iLen == 0)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* Rfu */
	vecbiTag.Separate(4);

	/* Copy incoming SDC data */
	const int iSDCDataSize = iLen - 4;

	pMDIInPkt->vecbiSDCData.Init(iSDCDataSize);
	pMDIInPkt->vecbiSDCData.ResetBitAccess();

	/* We have to copy bits instead of bytes since the length of SDC data is
	   usually not a multiple of 8 */
	for (int i = 0; i < iSDCDataSize; i++)
		pMDIInPkt->vecbiSDCData.Enqueue(vecbiTag.Separate(1), 1);

}



string CTagItemDecoderRobMod::GetTagName(void) {return "robm";}
void CTagItemDecoderRobMod::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Robustness mode (robm) always one byte long */
	if (iLen != 8)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	switch (vecbiTag.Separate(8))
	{
	case 0:
		/* Robustness mode A */
		pMDIInPkt->eRobMode = RM_ROBUSTNESS_MODE_A;
		break;

	case 1:
		/* Robustness mode B */
		pMDIInPkt->eRobMode = RM_ROBUSTNESS_MODE_B;
		break;

	case 2:
		/* Robustness mode C */
		pMDIInPkt->eRobMode = RM_ROBUSTNESS_MODE_C;
		break;

	case 3:
		/* Robustness mode D */
		pMDIInPkt->eRobMode = RM_ROBUSTNESS_MODE_D;
		break;
	}
}


string CTagItemDecoderStr::GetTagName(void) 
{
	switch (iStreamNumber)
	{
	case 0: return "str0";
	case 1: return "str1";
	case 2: return "str2";
	case 3: return "str3";
	default: return "str?";
	}
}

void CTagItemDecoderStr::SetStreamNumber(const int iNumber)
{
	iStreamNumber = iNumber;
}

void CTagItemDecoderStr::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Copy stream data */
	pMDIInPkt->vecbiStr[iStreamNumber].Init(iLen);
	pMDIInPkt->vecbiStr[iStreamNumber].ResetBitAccess();

	for (int i = 0; i < iLen / SIZEOF__BYTE; i++)
	{
		pMDIInPkt->vecbiStr[iStreamNumber].
			Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
	}
}



string CTagItemDecoderSDCChanInf::GetTagName(void) {return "sdci";}

void CTagItemDecoderSDCChanInf::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Get the number of streams */
	const int iNumStreams = (iLen - 8) / 3 / SIZEOF__BYTE;

	/* Get protection levels */
	/* Rfu */
	vecbiTag.Separate(4);

	/* Protection level for part A */ // TODO
	vecbiTag.Separate(2);

	/* Protection level for part B */ // TODO
	vecbiTag.Separate(2);

	/* Get stream parameters */

	/* Determine if hierarchical modulation is used */ // TODO
_BOOLEAN bHierarchical = FALSE;

	for (int i = 0; i < iNumStreams; i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hierarchical data */
		if ((i == 0) && (bHierarchical = TRUE))
		{
			/* Protection level for hierarchical */ // TODO
			vecbiTag.Separate(2);

			/* rfu */
			vecbiTag.Separate(10);

			/* Data length for hierarchical */ // TODO
			vecbiTag.Separate(12);
		}
		else
		{
			/* Data length for part A */ // TODO
			vecbiTag.Separate(12);

			/* Data length for part B */ // TODO
			vecbiTag.Separate(12);
		}
	}
}


string CTagItemDecoderInfo::GetTagName(void) {return "info";}

void CTagItemDecoderInfo::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
// TODO use text somehow
	/* Decode info string */
	string strInfo = "";
	for (int i = 0; i < iLen / SIZEOF__BYTE; i++)
		strInfo += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);
}
