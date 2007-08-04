/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-transmitter
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

#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "DrmTransmitter.h"
#include "MDI/MDIDecode.h"
#include "util/Buffer.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include <iostream>

#include "sound.h"

/* cloned from DrmReceiver - TODO better solution */

class CSplitFAC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;}
};

class CSplitSDC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.iNumSDCBitsPerSFrame;}
};

class CSplitMSC : public CSplitModul<_BINARY>
{
public:
	void SetStream(int iID) {iStreamID = iID;}

protected:
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.GetStreamLen(iStreamID) * SIZEOF__BYTE;}

	int iStreamID;
};

/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter(CSettings& Settings):
	TransmParam(NULL),
	strMDIinAddr(), strMDIoutAddr(),
	pReadData(NULL), AudioSourceEncoder(), strInputFileName(),
	strOutputFileName(), strOutputFileType(),
	vecstrTexts(), vecstrPics(), vecstrPicTypes(),
	iSoundInDev(-1), iSoundOutDev(-1),
	bCOFDMout(FALSE), bUseUEP(FALSE)
{
	/* Init streams */
	TransmParam.ResetServicesStreams();

	/* Init frame ID counter (index) */
	TransmParam.iFrameIDTransm = 0;

	/* Date, time. TODO: use computer system time... */
	TransmParam.iDay = 0;
	TransmParam.iMonth = 0;
	TransmParam.iYear = 0;
	TransmParam.iUTCHour = 0;
	TransmParam.iUTCMin = 0;

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
	TransmParam.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

	/* Protection levels for MSC. Depend on the modulation scheme. Look at
	   TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
	   iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
	TransmParam.MSCPrLe.iPartA = 0;
	TransmParam.MSCPrLe.iPartB = 1;
	TransmParam.MSCPrLe.iHierarch = 0;

	/* Either one audio or one data service can be chosen */
	_BOOLEAN bIsAudio = TRUE;

	/* In the current version only one service and one stream is supported. The
	   stream IDs must be 0 in both cases */
	if (bIsAudio == TRUE)
	{
		/* Audio */
		TransmParam.iNumAudioService = 1;
		TransmParam.iNumDataService = 0;

		TransmParam.Service[0].eAudDataFlag = CService::SF_AUDIO;
		TransmParam.Service[0].AudioParam.iStreamID = 0;

		/* Text message */
		TransmParam.Service[0].AudioParam.bTextflag = TRUE;

		/* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
		TransmParam.Service[0].iServiceDescr = 15;	/* 15 -> other music */
	}
	else
	{
		/* Data */
		TransmParam.iNumAudioService = 0;
		TransmParam.iNumDataService = 1;

		TransmParam.Service[0].eAudDataFlag = CService::SF_DATA;
		TransmParam.Service[0].DataParam.iStreamID = 0;

		/* Init SlideShow application */
		TransmParam.Service[0].DataParam.iPacketLen = 45;	/* TEST */
		TransmParam.Service[0].DataParam.eDataUnitInd =
			CDataParam::DU_DATA_UNITS;
		TransmParam.Service[0].DataParam.eAppDomain =
			CDataParam::AD_DAB_SPEC_APP;

		/* The value 0 indicates that the application details are provided
		   solely by SDC data entity type 5 */
		TransmParam.Service[0].iServiceDescr = 0;
	}

	/* Init service parameters, 24 bit unsigned integer number */
	TransmParam.Service[0].iServiceID = 0;

	/* Service label data. Up to 16 bytes defining the label using UTF-8
	   coding */
	TransmParam.Service[0].strLabel = "Dream Test";

	/* Language (see TableFAC.h, "strTableLanguageCode[]") */
	TransmParam.Service[0].iLanguage = 5;	/* 5 -> english */

	/* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
	   short interleaving (400 ms): SI_SHORT */
	TransmParam.eSymbolInterlMode = CParameter::SI_LONG;

	/* MSC modulation scheme. Available modes:
	   16-QAM standard mapping (SM): CS_2_SM,
	   64-QAM standard mapping (SM): CS_3_SM,
	   64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
	   64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
	TransmParam.eMSCCodingScheme = CS_3_SM;

	/* SDC modulation scheme. Available modes:
	   4-QAM standard mapping (SM): CS_1_SM,
	   16-QAM standard mapping (SM): CS_2_SM */
	TransmParam.eSDCCodingScheme = CS_2_SM;

	/* Set desired intermediate frequency (IF) in Hertz */
	/* Set desired intermediate frequency (IF) in Hertz */
	TransmParam.rCarOffset=12000.0;		/* Default: "VIRTUAL_INTERMED_FREQ" */

	/* default output format - REAL */
	TransmParam.eOutputFormat = OF_REAL_VAL;

	if (bUseUEP == TRUE)
	{
		// TEST
		TransmParam.SetStreamLen(0, 80, 0);
	}
	else
	{
		/* Length of part B is set automatically (equal error protection (EEP),
		   if "= 0"). Sets the number of bytes, should not exceed total number
		   of bytes available in MSC block */
		TransmParam.SetStreamLen(0, 0, 0);
	}
	TransmParam.eOutputFormat = OF_REAL_VAL;
}

void
CDRMTransmitter::
GetSoundInChoices(vector<string>& v)
{
	CSoundIn s;
	s.Enumerate(v);
}

void
CDRMTransmitter::
GetSoundOutChoices(vector<string>& v)
{
	CSoundOut s;
	s.Enumerate(v);
}

void
CDRMTransmitter::
SetSoundInInterface(int i)
{
	iSoundInDev = i;
}

void
CDRMTransmitter::
SetSoundOutInterface(int i)
{
	iSoundOutDev = i;
	strOutputFileName = "";
	strOutputFileType = "";
	bCOFDMout = TRUE;
}

_REAL CDRMTransmitter::GetLevelMeter()
{
	if(pReadData==NULL)
		return 0.0;
	return pReadData->GetLevelMeter();
}

void
CDRMTransmitter::AddTextMessage(const string& strText)
{
	vecstrTexts.push_back(strText);
}

void
CDRMTransmitter::ClearTextMessages()
{
	vecstrTexts.clear();
}

void
CDRMTransmitter::AddPic(const string& strFileName, const string& strFormat)
{
	vecstrPics.push_back(strFileName);
	vecstrPicTypes.push_back(strFormat);
}

void
CDRMTransmitter::ClearPics()
{
	vecstrPics.clear();
	vecstrPicTypes.clear();
}

_BOOLEAN
CDRMTransmitter::GetTransStat(string& strCPi, _REAL& rCPe)
{
	return AudioSourceEncoder.GetTransStat(strCPi, rCPe);
}

void
CDRMTransmitter::SetReadFromFile(const string & strNFN)
{
	strInputFileName = strNFN;
}

void
CDRMTransmitter::SetWriteToFile(const string & strNFN, const string & strType)
{
	strOutputFileName = strNFN;
	strOutputFileType = strType;
	cout << "CDRMTransmitter::SetWriteToFile(" << strOutputFileName << "," << strOutputFileType << ")" << endl;
	bCOFDMout = TRUE;
}

void
CDRMTransmitter::Stop()
{
	TransmParam.bRunThread = FALSE;
}

void CDRMTransmitter::Start()
{
	/* Buffers */
	CSingleBuffer<_SAMPLE>	DataBuf;

	vector<CSingleBuffer<_BINARY> >	MSCBuf(MAX_NUM_STREAMS);
	vector<CSingleBuffer<_BINARY> >	MSCTxBuf(MAX_NUM_STREAMS);
	vector<CSingleBuffer<_BINARY> >	MSCSendBuf(MAX_NUM_STREAMS);
	CSingleBuffer<_BINARY>			MDIPacketBuf;

	CSingleBuffer<_COMPLEX>	MLCEncBuf;
	CCyclicBuffer<_COMPLEX>	IntlBuf;

	CSingleBuffer<_BINARY>	FACBuf;
	CSingleBuffer<_BINARY>	FACTxBuf;
	CSingleBuffer<_BINARY>	FACSendBuf;
	CCyclicBuffer<_COMPLEX>	FACMapBuf;

	CSingleBuffer<_BINARY>	SDCBuf;
	CSingleBuffer<_BINARY>	SDCTxBuf;
	CSingleBuffer<_BINARY>	SDCSendBuf;
	CCyclicBuffer<_COMPLEX>	SDCMapBuf;

	CSingleBuffer<_COMPLEX>	CarMapBuf;
	CSingleBuffer<_COMPLEX>	OFDMModBuf;

	/* Modules */
	CSoundInInterface*		pSoundInInterface = NULL;
	CSoundOutInterface*		pSoundOutInterface = NULL;
	CTransmitData*			pTransmitData = NULL;

	CSplitFAC				SplitFAC;
	CAudioSourceEncoder		AudioSourceEncoder;
	CMSCMLCEncoder			MSCMLCEncoder;
	CSymbInterleaver		SymbInterleaver;
	CGenerateFACData		GenerateFACData;
	CFACMLCEncoder			FACMLCEncoder;
	CGenerateSDCData		GenerateSDCData;
	CSDCMLCEncoder			SDCMLCEncoder;
	COFDMCellMapping		OFDMCellMapping;
	COFDMModulation			OFDMModulation;
	CSplitSDC				SplitSDC;
	CSplitMSC				SplitMSC[MAX_NUM_STREAMS];
	CUpstreamDI				MDIIn;
	CDecodeRSIMDI			DecodeMDI;
	CDownstreamDI			MDIOut;

	if(strInputFileName=="")
	{
		pSoundInInterface = new CSoundIn;
		pReadData = new CReadData(pSoundInInterface);
	}
	else
	{
		pReadData = new CReadData(NULL);
	}


	/* Initialization of the modules
	 * we have to do the MLC first because this initialises the MSC parameters.
	 * TODO problem of initialising things from the MDI input !!!!!!!!!!
	 */

	if(bCOFDMout)
	{
		/* Defines number of cells, important! */
		OFDMCellMapping.Init(TransmParam, CarMapBuf);

		/* Defines number of SDC bits per super-frame */
		SDCMLCEncoder.Init(TransmParam, SDCMapBuf);

		MSCMLCEncoder.Init(TransmParam, MLCEncBuf);
		SymbInterleaver.Init(TransmParam, IntlBuf);
		FACMLCEncoder.Init(TransmParam, FACMapBuf);
		OFDMModulation.Init(TransmParam, OFDMModBuf);

		if(strOutputFileName=="")
		{
			pSoundOutInterface = new CSoundOut;
			pTransmitData = new CTransmitData(pSoundOutInterface);
		}
		else
		{
			pTransmitData = new CTransmitData(NULL);
			cout << "write to file" << endl;
			pTransmitData->SetWriteToFile(strOutputFileName, strOutputFileType);
		}

		pTransmitData->Init(TransmParam);
	}

	if(strMDIinAddr != "")
	{
		/* set the input address*/
		MDIIn.SetOrigin(strMDIinAddr);
		MDIIn.SetInitFlag();
		DecodeMDI.SetInitFlag();
		//strMDIinAddr="";
	}
	else
	{
		GenerateFACData.Init(TransmParam, FACBuf);
		GenerateSDCData.Init(TransmParam, SDCBuf);

		if(strInputFileName != "")
			pReadData->SetReadFromFile(strInputFileName);
		pReadData->Init(TransmParam, DataBuf);

		AudioSourceEncoder.ClearTextMessage();
		size_t i;
		for(i=0; i<vecstrTexts.size(); i++)
			AudioSourceEncoder.SetTextMessage(vecstrTexts[i]);

		AudioSourceEncoder.ClearPicFileNames();
		for(i=0; i<vecstrPics.size(); i++)
			AudioSourceEncoder.SetPicFileName(vecstrPics[i], vecstrPicTypes[i]);

		AudioSourceEncoder.Init(TransmParam, MSCBuf[0]);
	}

	if(strMDIoutAddr!="")
	{
		/* set the output address */
		MDIOut.AddSubscriber(strMDIoutAddr, "", 'M');
		//MDIOut.SetInitFlag();
		//strMDIoutAddr="";
	}

	SplitFAC.SetInitFlag();
	FACBuf.Clear();
	FACTxBuf.Clear();
	FACSendBuf.Clear();
	SplitSDC.SetInitFlag();
	SDCBuf.Clear();
	SDCTxBuf.Clear();
	SDCSendBuf.Clear();
	FACBuf.Init(72);
	SDCBuf.Init(10000);
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		SplitMSC[i].SetStream(i);
		SplitMSC[i].SetInitFlag();
		MSCBuf[i].Clear();
		MSCTxBuf[i].Clear();
		MSCSendBuf[i].Clear();
		MSCBuf[i].Init(10000);
	}

	/* Set run flag */
	TransmParam.bRunThread = TRUE;

	try
	{
/*
	The hand over of data is done via an intermediate-buffer. The calling
	convention is always "input-buffer, output-buffer". Additional, the
	DRM-parameters are fed to the function
*/
		while (TransmParam.bRunThread)
		{
			if(MDIIn.GetInEnabled())
			{
				MDIPacketBuf.Init(16384);
				//MDIPacketBuf.Clear();
				MDIIn.ReadData(TransmParam, MDIPacketBuf);
				if(MDIPacketBuf.GetFillLevel()>0)
				{
					DecodeMDI.ProcessData(TransmParam, MDIPacketBuf, FACBuf, SDCBuf, MSCBuf);
				}
			}
			else
			{
				/* MSC *********************************************************** */
				/* Read the source signal */
				pReadData->ReadData(TransmParam, DataBuf);

				/* Audio source encoder */
				AudioSourceEncoder.ProcessData(TransmParam, DataBuf, MSCBuf[0]);

				/* FAC *********************************************************** */
				GenerateFACData.ReadData(TransmParam, FACBuf);

				/* SDC *********************************************************** */
				GenerateSDCData.ReadData(TransmParam, SDCBuf);
			}

			/* TODO optimise - split only if needed */

			SplitFAC.ProcessData(TransmParam, FACBuf, FACTxBuf, FACSendBuf);

			if(SDCBuf.GetFillLevel()==TransmParam.iNumSDCBitsPerSFrame)
			{
				SplitSDC.ProcessData(TransmParam, SDCBuf, SDCTxBuf, SDCSendBuf);
			}

			for(size_t i=0; i<MAX_NUM_STREAMS; i++)
			{
				SplitMSC[i].ProcessData(TransmParam, MSCBuf[i], MSCTxBuf[i], MSCSendBuf[i]);
			}

			if(bCOFDMout)
			{
				/* MLC-encoder */
				MSCMLCEncoder.ProcessData(TransmParam, MSCBuf[0], MLCEncBuf);

				/* Convolutional interleaver */
				SymbInterleaver.ProcessData(TransmParam, MLCEncBuf, IntlBuf);

				/* FAC *************************************************************** */
				FACMLCEncoder.ProcessData(TransmParam, FACBuf, FACMapBuf);

				/* SDC *************************************************************** */
				SDCMLCEncoder.ProcessData(TransmParam, SDCBuf, SDCMapBuf);

				/* Mapping of the MSC, FAC, SDC and pilots on the carriers *********** */
				OFDMCellMapping.ProcessData(TransmParam, IntlBuf, FACMapBuf, SDCMapBuf, CarMapBuf);

				/* OFDM-modulation *************************************************** */
				OFDMModulation.ProcessData(TransmParam, CarMapBuf, OFDMModBuf);

				/* Transmit the signal *********************************************** */
				pTransmitData->WriteData(TransmParam, OFDMModBuf);
			}

			if(MDIOut.GetOutEnabled())
			{
				MDIOut.SendLockedFrame(TransmParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
			}
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}

	delete pReadData;
	if(pSoundInInterface)
		delete pSoundInInterface;

	if(bCOFDMout)
	{
		delete pTransmitData;
		if(pSoundOutInterface)
			delete pSoundOutInterface;
	}
}
