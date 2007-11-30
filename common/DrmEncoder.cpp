/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DRM-Encoder
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

#include <iostream>
#include "util/Settings.h"
#include "Parameter.h"
#include "DrmEncoder.h"

#include "sound.h"
#include "sound/soundfile.h"

/* Implementation *************************************************************/
CDRMEncoder::CDRMEncoder():
	DataBuf(), pSoundInInterface(NULL),
	AudioSourceEncoder(), DataEncoder(), GenerateFACData(), GenerateSDCData(),
	pReadData(NULL), strInputFileName(),
	vecstrTexts(), vecstrPics(), vecstrPicTypes(),
	iSoundInDev(-1), bUseUEP(FALSE)
{
}

void
CDRMEncoder::GetSoundInChoices(vector<string>& v)
{
	CSoundIn s;
	s.Enumerate(v);
}

void
CDRMEncoder::SetSoundInInterface(int i)
{
	iSoundInDev = i;
}

void
CDRMEncoder::AddTextMessage(const string& strText)
{
	vecstrTexts.push_back(strText);
}

void
CDRMEncoder::ClearTextMessages()
{
	vecstrTexts.clear();
}

void
CDRMEncoder::AddPic(const string& strFileName, const string& strFormat)
{
	vecstrPics.push_back(strFileName);
	vecstrPicTypes.push_back(strFormat);
}

void
CDRMEncoder::ClearPics()
{
	vecstrPics.clear();
	vecstrPicTypes.clear();
}

void
CDRMEncoder::GetTextMessages(vector<string>&)
{
}

void
CDRMEncoder::GetPics(map<string,string>& m)
{
	for(size_t i=0; i<vecstrPics.size(); i++)
		m[vecstrPics[i]] = vecstrPicTypes[i];
}

_BOOLEAN
CDRMEncoder::GetTransStat(string& strCPi, _REAL& rCPe)
{
	return DataEncoder.GetSliShowEnc()->GetTransStat(strCPi, rCPe);
}

void
CDRMEncoder::SetReadFromFile(const string & strNFN)
{
	strInputFileName = strNFN;
}

void CDRMEncoder::Init(CParameter& Parameters,
			CBuffer<_BINARY>& FACBuf, 
			CBuffer<_BINARY>& SDCBuf, 
			vector< CSingleBuffer<_BINARY> >& MSCBuf)
{
	GenerateFACData.Init(Parameters, FACBuf);
	GenerateSDCData.Init(Parameters, SDCBuf);

	if(strInputFileName=="")
	{
		pSoundInInterface = new CSoundIn;
		pSoundInInterface->SetDev(iSoundInDev);
	}
	else
	{
		CSoundFileIn *pf = new CSoundFileIn;
		pf->SetFileName(strInputFileName);
		pSoundInInterface = pf;
	}
	pReadData = new CReadData(pSoundInInterface);
	pReadData->Init(Parameters, DataBuf);

	AudioSourceEncoder.ClearTextMessage();
	size_t i;
	for(i=0; i<vecstrTexts.size(); i++)
		AudioSourceEncoder.SetTextMessage(vecstrTexts[i]);
	AudioSourceEncoder.Init(Parameters, MSCBuf[0]);

	DataEncoder.GetSliShowEnc()->ClearAllFileNames();
	for(i=0; i<vecstrPics.size(); i++)
		DataEncoder.GetSliShowEnc()->AddFileName(vecstrPics[i], vecstrPicTypes[i]);
	DataEncoder.Init(Parameters);
	SignalLevelMeter.Init(0);
}

void
CDRMEncoder::ProcessData(CParameter& Parameters,
			CBuffer<_BINARY>& FACBuf, 
			CBuffer<_BINARY>& SDCBuf, 
			vector< CSingleBuffer<_BINARY> >& MSCBuf)
{
	/* MSC *********************************************************** */
	/* Read the source signal */
	pReadData->ReadData(Parameters, DataBuf);

	SignalLevelMeter.Update(*DataBuf.QueryWriteBuffer());

	/* Audio source encoder */
	AudioSourceEncoder.ProcessData(Parameters, DataBuf, MSCBuf[0]);

#if 0
	if(false)
	{
		/* Write data packets in stream */
		CVector < _BINARY > vecbiData;
		const int iNumPack = iOutputBlockSize / iTotPacketSize;
		int iPos = 0;

		for (int j = 0; j < iNumPack; j++)
		{
			/* Get new packet */
			DataEncoder.GeneratePacket(vecbiData);

			/* Put it on stream */
			for (i = 0; i < iTotPacketSize; i++)
			{
				(*pvecOutputData)[iPos] = vecbiData[i];
				iPos++;
			}
		}
	}
#endif

	/* FAC *********************************************************** */
	GenerateFACData.ReadData(Parameters, FACBuf);

	/* SDC *********************************************************** */
	GenerateSDCData.ReadData(Parameters, SDCBuf);

}

void
CDRMEncoder::Cleanup(CParameter&)
{
	delete pReadData;
	if(pSoundInInterface)
		delete pSoundInInterface;
}

void
CDRMEncoder::LoadSettings(CSettings& s, CParameter& Parameters)
{
	/* Either one audio or one data service can be chosen */

	Parameters.Service.resize(1);

	_BOOLEAN bIsAudio = s.Get("Encoder", "audioservice", 1);

	/* In the current version only one service and one stream is supported. The
	   stream IDs must be 0 in both cases */
	if (bIsAudio == TRUE)
	{
		/* Audio */
		Parameters.iNumAudioService = 1;
		Parameters.iNumDataService = 0;

		Parameters.Service[0].eAudDataFlag = SF_AUDIO;
		Parameters.Service[0].iAudioStream = 0;

		/* Text message */
		Parameters.AudioParam[0].bTextflag = s.Get("Encoder", "textmessages", 1);

		/* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
		Parameters.Service[0].iServiceDescr = s.Get("Encoder", "genre", 15); /* 15 -> other music */
	}
	else
	{
		/* Data */
		Parameters.iNumAudioService = 0;
		Parameters.iNumDataService = 1;

		Parameters.Service[0].eAudDataFlag = SF_DATA;
		Parameters.Service[0].iDataStream = 0;
		Parameters.Service[0].iPacketID = 0;

		/* Init SlideShow application */
		Parameters.DataParam[0][0].eDataUnitInd = CDataParam::DU_DATA_UNITS;
		Parameters.DataParam[0][0].eAppDomain = CDataParam::AD_DAB_SPEC_APP;

		/* The value 0 indicates that the application details are provided
		   solely by SDC data entity type 5 */
		Parameters.Service[0].iServiceDescr = 0;
	}

	/* Init service parameters, 24 bit unsigned integer number */
	Parameters.Service[0].iServiceID = s.Get("Encoder", "sid", 0);

	/* Service label data. Up to 16 bytes defining the label using UTF-8 coding */
	Parameters.Service[0].strLabel = s.Get("Encoder", "label", string("Dream Test"));

	/* FAC Language (see TableFAC.h, "strTableLanguageCode[]") */
	Parameters.Service[0].iLanguage = s.Get("Encoder", "language", 5);	/* 5 -> english */

	/* SDC Language and Country */
	Parameters.Service[0].strLanguageCode = s.Get("Encoder", "ISOlanguage", string("eng"));
	Parameters.Service[0].strCountryCode = s.Get("Encoder", "ISOCountry", string("gb"));

	/**************************************************************************/
	/* Robustness mode and spectrum occupancy. Available transmission modes:
	   RM_ROBUSTNESS_MODE_A: Gaussian channels, with minor fading,
	   RM_ROBUSTNESS_MODE_B: Time and frequency selective channels, with longer
	   delay spread,
	   RM_ROBUSTNESS_MODE_C: As robustness mode B, but with higher Doppler
	   spread,
	   RM_ROBUSTNESS_MODE_D: As robustness mode B, but with severe delay and
	   Doppler spread.
	   Available bandwidths:
	   SO_0: 4.5 kHz, SO_1: 5 kHz, SO_2: 9 kHz, SO_3: 10 kHz, SO_4: 18 kHz,
	   SO_5: 20 kHz */
	Parameters.SetWaveMode(ERobMode(s.Get("Encoder", "robm", RM_ROBUSTNESS_MODE_B)));
	Parameters.SetSpectrumOccup(ESpecOcc(s.Get("Encoder", "spectrum_occupancy", SO_3)));

	/* Protection levels for MSC. Depend on the modulation scheme. Look at
	   TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
	   iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
	Parameters.MSCPrLe.iPartA = s.Get("Encoder", "PartAProt", 0);
	Parameters.MSCPrLe.iPartB = s.Get("Encoder", "PartBProt", 1);
	Parameters.MSCPrLe.iHierarch = s.Get("Encoder", "HierarchicalProt", 0);

	/* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
	   short interleaving (400 ms): SI_SHORT */
	Parameters.eSymbolInterlMode
		= CParameter::ESymIntMod(s.Get("Encoder", "interleaving", CParameter::SI_LONG));

	/* MSC modulation scheme. Available modes:
	   16-QAM standard mapping (SM): CS_2_SM,
	   64-QAM standard mapping (SM): CS_3_SM,
	   64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
	   64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
	Parameters.eMSCCodingScheme = ECodScheme(s.Get("Encoder", "mscmod", CS_3_SM));

	/* SDC modulation scheme. Available modes:
	   4-QAM standard mapping (SM): CS_1_SM,
	   16-QAM standard mapping (SM): CS_2_SM */
	Parameters.eSDCCodingScheme = ECodScheme(s.Get("Encoder", "sdcmod", CS_2_SM));

	iSoundInDev = s.Get("Encoder", "snddevin", -1);
	strInputFileName = s.Get("Encoder", "inputfile", string(""));

	/* streams - do when know MSC Capacity */
	Parameters.Stream.resize(1);
	int iA = s.Get("Encoder", "s0PartALen", 0);
	int iB = s.Get("Encoder", "s0PartBLen", -1);
	Parameters.SetStreamLen(0, iA, iB);
	if (bIsAudio == TRUE)
	{
		Parameters.Stream[0].eAudDataFlag = SF_AUDIO;
	}
	else
	{
		Parameters.Stream[0].eAudDataFlag = SF_DATA;
		Parameters.Stream[0].ePacketModInd = PM_PACKET_MODE;
		Parameters.Stream[0].iPacketLen = 45;	/* TEST */
	}
}

void
CDRMEncoder::SaveSettings(CSettings& s, CParameter& Parameters)
{
	s.Put("Encoder", "audioservice", (Parameters.Service[0].eAudDataFlag == SF_AUDIO)?1:0);
	s.Put("Encoder", "textmessages", Parameters.AudioParam[0].bTextflag);
	s.Put("Encoder", "genre", Parameters.Service[0].iServiceDescr);
	s.Put("Encoder", "sid", int(Parameters.Service[0].iServiceID));
	s.Put("Encoder", "label", Parameters.Service[0].strLabel);
	s.Put("Encoder", "language", Parameters.Service[0].iLanguage);
	s.Put("Encoder", "ISOlanguage", Parameters.Service[0].strLanguageCode);
	s.Put("Encoder", "ISOCountry", Parameters.Service[0].strCountryCode);

	s.Put("Encoder", "robm", Parameters.GetWaveMode());
	s.Put("Encoder", "spectrum_occupancy", Parameters.GetSpectrumOccup());
	s.Put("Encoder", "PartAProt", Parameters.MSCPrLe.iPartA);
	s.Put("Encoder", "PartBProt", Parameters.MSCPrLe.iPartB);
	s.Put("Encoder", "HierarchicalProt", Parameters.MSCPrLe.iHierarch);
	s.Put("Encoder", "interleaving", Parameters.eSymbolInterlMode);
	s.Put("Encoder", "mscmod", Parameters.eMSCCodingScheme);
	s.Put("Encoder", "sdcmod", Parameters.eSDCCodingScheme);
	s.Put("Encoder", "snddevin", iSoundInDev);
	s.Put("Encoder", "inputfile", strInputFileName);
	s.Put("Encoder", "s0PartALen", Parameters.Stream[0].iLenPartA);
	s.Put("Encoder", "s0PartBLen", Parameters.Stream[0].iLenPartB);
}
