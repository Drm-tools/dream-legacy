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
#include "../Modul.h"
#include "../CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"

#ifdef USE_FAAD2_LIBRARY
# include "faad.h"
#endif


/* Definitions ****************************************************************/
/* Forgetting factor for audio blocks in case CRC was wrong */
#define FORFACT_AUD_BL_BAD_CRC			((_REAL) 0.6)


/* Classes ********************************************************************/
class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>
{
public:
	CAudioSourceDecoder();
	virtual ~CAudioSourceDecoder();

protected:
	enum EInitErr {ET_ALL, ET_AAC}; /* ET: Error type */
	class CInitErr 
	{
	public:
		CInitErr(EInitErr eNewErrType) : eErrType(eNewErrType) {}
		EInitErr eErrType;
	};

	/* General */
	_BOOLEAN			DoNotProcessData;
	_BOOLEAN			DoNotProcessAAC;

	/* Text message */
	_BOOLEAN			bTextMessageUsed;
	CTextMessageDecoder	TextMessage;
	CVector<_BINARY>	vecbiTextMessBuf;

	int					iTotalFrameSize;


#ifdef USE_FAAD2_LIBRARY
	/* AAC decoding */
	faacDecHandle		HandleAACDecoder;

	CAudioResample		ResampleObjL;
	CAudioResample		ResampleObjR;
		

	CVector<_REAL>		vecTempResBufInLeft;
	CVector<_REAL>		vecTempResBufInRight;
	CVector<_REAL>		vecTempResBufOutLeft;
	CVector<_REAL>		vecTempResBufOutRight;

	CVector<_BYTE>		vecbyPrepAudioFrame;
	CVector<_BYTE>		aac_crc_bits;
	CMatrix<_BYTE>		audio_frame;

	CVector<int>		veciFrameLength;

	int					iNumAACFrames;
	int					iNumBorders;
	int					iResOutBlockSize;
	int					iNumHigherProtectedBytes;
	int					iMaxLenOneAudFrame;
	int					iNumChannelsAAC;
	int					iLenDecOutPerChan;
	int					iBadBlockCount;
	int					iAudioPayloadLen;
	int					iLenAudLow;

	_BOOLEAN			bAudioWasOK;
#endif


	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
