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

#ifndef AUIDOSOURCEENCODER_H
#define AUIDOSOURCEENCODER_H

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../util/Modul.h"
#include "../util/CRC.h"
#include "../TextMessage.h"
#include "../resample/Resample.h"
#include "../datadecoding/DataDecoder.h"
#include "../util/Utilities.h"
#include "AudioSourceEncoderInterface.h"
#ifdef USE_FAAC_LIBRARY
# include "faac.h"
#endif


/* Classes ********************************************************************/
class CAudioSourceEncoder : public CTransmitterModul<_SAMPLE, _BINARY>
							, public CAudioSourceEncoderInterface
{
public:
	CAudioSourceEncoder() : bUsingTextMessage(FALSE)
#ifdef USE_FAAC_LIBRARY
		, hEncoder(NULL)
#endif
		{}
	virtual ~CAudioSourceEncoder();

	void AddTextMessage(const string& strText);
	void ClearTextMessages();

	void AddPic(const string& strFileName, const string& strFormat)
		{DataEncoder.GetSliShowEnc()->AddFileName(strFileName, strFormat);}
	void ClearPics()
		{DataEncoder.GetSliShowEnc()->ClearAllFileNames();}
	_BOOLEAN GetTransStat(string& strCPi, _REAL& rCPe)
		{return DataEncoder.GetSliShowEnc()->GetTransStat(strCPi, rCPe);}

protected:
	CTextMessageEncoder		TextMessage;
	_BOOLEAN				bUsingTextMessage;
	CDataEncoder			DataEncoder;
	int						iTotPacketSize;
	_BOOLEAN				bIsDataService;
	int						iTotNumBitsForUsage;

#ifdef USE_FAAC_LIBRARY
	faacEncHandle			hEncoder;
	faacEncConfigurationPtr CurEncFormat;

	unsigned long			lNumSampEncIn;
	unsigned long			lMaxBytesEncOut;
	unsigned long			lEncSamprate;
	CVector<_BYTE>			aac_crc_bits;
	CVector<_SAMPLE>		vecsEncInData;
	CMatrix<_BYTE>			audio_frame;
	CVector<int>			veciFrameLength;
	int						iNumAACFrames;
	int						iAudioPayloadLen;
	int						iNumHigherProtectedBytes;

	CAudioResample			ResampleObj;
	CVector<_REAL>			vecTempResBufIn;
	CVector<_REAL>			vecTempResBufOut;
#endif

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

#endif
