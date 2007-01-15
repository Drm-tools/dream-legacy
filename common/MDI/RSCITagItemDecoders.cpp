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
 *  items defined in the control part of RSCI.
 *  Decoded commands are generally sent straight to the CDRMReceiver object which
 *	they hold a pointer to.
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


#include "RSCITagItemDecoders.h"
#include "../DrmReceiver.h"

/* RX_STAT Items */

_REAL CTagItemDecoderRSI::decodeDb(CVector<_BINARY>& vecbiTag)
{
 	  int8_t n = (int8_t)vecbiTag.Separate(8);
 	  uint8_t m = (uint8_t)vecbiTag.Separate(8);
 	  return _REAL(n)+_REAL(m)/256.0;
}

void CTagItemDecoderRdbv::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->SetSignalStrength(TRUE, decodeDb(vecbiTag));
}

void CTagItemDecoderRsta::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;
 	uint8_t sync = (uint8_t)vecbiTag.Separate(8);
 	uint8_t fac = (uint8_t)vecbiTag.Separate(8);
 	uint8_t sdc = (uint8_t)vecbiTag.Separate(8);
 	uint8_t audio = (uint8_t)vecbiTag.Separate(8);
 	if(sync==0)
		pParameter->ReceiveStatus.SetTimeSyncStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SetTimeSyncStatus(CRC_ERROR);
 	if(fac==0)
		pParameter->ReceiveStatus.SetFACStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SetFACStatus(CRC_ERROR);
 	if(sdc==0)
		pParameter->ReceiveStatus.SetSDCStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SetSDCStatus(CRC_ERROR);
 	if(audio==0)
		pParameter->ReceiveStatus.SetAudioStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SetAudioStatus(CRC_ERROR);
}

void CTagItemDecoderRwmf::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rWMERFAC = decodeDb(vecbiTag);
}

void CTagItemDecoderRwmm::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rWMERMSC = decodeDb(vecbiTag);
}

void CTagItemDecoderRmer::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rMER = decodeDb(vecbiTag);
}
/* RX_CTRL Items */

void CTagItemDecoderCact::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	const int iNewState = vecbiTag.Separate(8) - '0';

	// TODO pDRMReceiver->SetState(iNewState);
	(void)iNewState;

}

void CTagItemDecoderCfre::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	if (pDRMReceiver == NULL)
		return;

	const int iNewFrequency = vecbiTag.Separate(32);

	pDRMReceiver->SetFrequency(iNewFrequency/1000);

}

void CTagItemDecoderCdmo::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	string s = "";
	for (int i = 0; i < iLen / SIZEOF__BYTE; i++)
		s += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);
	if(s == "drm_")
		pDRMReceiver->SetReceiverMode(RM_DRM);
	if(s == "am__")
		pDRMReceiver->SetReceiverMode(RM_AM);
}

/* TODO: other control tag items */
