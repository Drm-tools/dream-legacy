/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See AudioSourceDecoder.cpp
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

#if !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
#define AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "faad.h"
#include "../Modul.h"
#include "../CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"


/* Definitions ****************************************************************/
#define NO_BAD_BL_UNTIL_ZERO			2


/* Classes ********************************************************************/
class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
	CAudioSourceDecoder();
	virtual ~CAudioSourceDecoder();

protected:
	_BOOLEAN			bTextMessageUsed;
	int					iNumAACFrames;
	int					iNoHeaderBytes;
	int					iLenAudLow;
	int					iLenAudHigh;

	CTextMessage		TextMessage;

	faacDecHandle		HandleAACDecoder;

	CVector<_BYTE>		vecbyPrepAudioFrame;

	CResample			ResampleObjLeft;
	CResample			ResampleObjRight;
	CVector<_REAL>		vecTempResBufInLeft;
	CVector<_REAL>		vecTempResBufInRight;
	CVector<_REAL>		vecTempResBufOutLeft;
	CVector<_REAL>		vecTempResBufOutRight;
	int					iAudioSamplRate;

	int					iNumHigherProtectedBytes;

	CVector<_BYTE>		aac_crc_bits;
	CMatrix<_BYTE>		audio_frame;
	CVector<int>		veciFrameLength;

	int					iMaxLenOneAudFrame;

	int					iNoChannelsAAC;

	int					iBadBlockCount;
	int					iTotalFrameSize;
	int					iAudioPayloadLen;
	int					iNoBorders;

	CVector<_BINARY>	vecbiTextMessBuf;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
