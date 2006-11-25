/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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
#include "../util/Modul.h"
#include "../util/CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"
#include "../util/Utilities.h"
#include "AudioSourceDecoderInterface.h"
#ifdef USE_FAAD2_LIBRARY
# include "neaacdec.h"
#endif

class CAudioSourceDecoder : public CReceiverModul<_BINARY, _SAMPLE>, public CAudioSourceDecoderInterface
{
public:
	CAudioSourceDecoder();

	virtual ~CAudioSourceDecoder();

	int GetNumCorDecAudio();
	void SetReverbEffect(const _BOOLEAN bNER) {bUseReverbEffect = bNER;}
	_BOOLEAN GetReverbEffect() {return bUseReverbEffect;}

protected:
	enum EInitErr {ET_ALL, ET_AUDDECODER}; /* ET: Error type */
	class CInitErr 
	{
	public:
		CInitErr(EInitErr eNewErrType) : eErrType(eNewErrType) {}
		EInitErr eErrType;
	};

	/* General */
	_BOOLEAN			DoNotProcessData;
	_BOOLEAN			DoNotProcessAudDecoder;
	int					iTotalFrameSize;
	int					iNumCorDecAudio;

	/* Text message */
	_BOOLEAN			bTextMessageUsed;
	CTextMessageDecoder	TextMessage;
	CVector<_BINARY>	vecbiTextMessBuf;

	/* Resampling */
	int					iResOutBlockSize;
	
	CAudioResample		ResampleObjL;
	CAudioResample		ResampleObjR;

	CVector<_REAL>		vecTempResBufInLeft;
	CVector<_REAL>		vecTempResBufInRight;
	CVector<_REAL>		vecTempResBufOutCurLeft;
	CVector<_REAL>		vecTempResBufOutCurRight;
	CVector<_REAL>		vecTempResBufOutOldLeft;
	CVector<_REAL>		vecTempResBufOutOldRight;

	/* Drop-out masking (reverberation) */
	_BOOLEAN			bAudioWasOK;
	_BOOLEAN			bUseReverbEffect;
	CAudioReverb		AudioRev;

	int					iLenDecOutPerChan;
	int					iNumAudioFrames;

	CParameter::EAudCod	eAudioCoding;


#ifdef USE_FAAD2_LIBRARY /* AAC decoding */
	NeAACDecHandle		HandleAACDecoder;

	CVector<_BYTE>		vecbyPrepAudioFrame;
	CVector<_BYTE>		aac_crc_bits;
	CMatrix<_BYTE>		audio_frame;

	CVector<int>		veciFrameLength;

	int					iNumBorders;
	int					iNumHigherProtectedBytes;
	int					iMaxLenOneAudFrame;

	int					iBadBlockCount;
	int					iAudioPayloadLen;
#endif


	/* CELP decoding */
	CMatrix<_BINARY>	celp_frame;
	CVector<_BYTE>		celp_crc_bits;
	int					iNumHigherProtectedBits;
	int					iNumLowerProtectedBits;

	_BOOLEAN			bCELPCRC;
	CCRC				CELPCRCObject;

#ifdef USE_CELP_DECODER
	/* TODO put here decoder specific things */
#endif
	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(AUIDOSOURCEDECODER_H__3B0BA660_CABB2B_23E7A0D31912__INCLUDED_)
