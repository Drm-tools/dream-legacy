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

#include "DrmTransmitter.h"


/* Implementation *************************************************************/
void CDRMTransmitter::Start()
{
	/* Set run flag */
	TransmParam.bRunThread = TRUE;

	/* Initialization of the modules */
	Init();

	/* Start the transmitter run routine */
	Run();
}

void CDRMTransmitter::Stop()
{
	TransmParam.bRunThread = FALSE;

#ifndef WRITE_TRNSM_TO_FILE
	SoundInterface.Close();
#endif
}

void CDRMTransmitter::Run()
{
/*
 The hand over of data is done via an intermediate-buffer. The calling
 convention is always "input-buffer, output-buffer". Additional, the
 DRM-parameters are fed to the function
*/
	while (TransmParam.bRunThread)
	{
		/* MSC ****************************************************************/
		/* Read the source signal */
		ReadData.ReadData(TransmParam, DataBuf);

		/* Audio source encoder */
		AudioSourceEncoder.ProcessData(TransmParam, DataBuf, AudSrcBuf);
	
		/* MLC-encoder */
		MSCMLCEncoder.ProcessData(TransmParam, AudSrcBuf, MLCEncBuf);
	
		/* Convolutional interleaver */
		SymbInterleaver.ProcessData(TransmParam, MLCEncBuf, IntlBuf);


		/* FAC ****************************************************************/
		GenerateFACData.ReadData(TransmParam, GenFACDataBuf);
		FACMLCEncoder.ProcessData(TransmParam, GenFACDataBuf, FACMapBuf);


		/* SDC ****************************************************************/
		GenerateSDCData.ReadData(TransmParam, GenSDCDataBuf);
		SDCMLCEncoder.ProcessData(TransmParam, GenSDCDataBuf, SDCMapBuf);

	
		/* Mapping of the MSC, FAC, SDC and pilots on the carriers ************/
		OFDMCellMapping.ProcessData(TransmParam, IntlBuf, FACMapBuf, SDCMapBuf,
			CarMapBuf);
	

		/* OFDM-modulation ****************************************************/
		OFDMModulation.ProcessData(TransmParam, CarMapBuf, OFDMModBuf);
	

		/* Transmit the signal ************************************************/
		TransmitData.WriteData(TransmParam, OFDMModBuf);
	}
}

void CDRMTransmitter::Init()
{
	/* Defines number of cells, important! */
	OFDMCellMapping.Init(TransmParam, CarMapBuf);

	/* Defines number of SDC bits per super-frame */
	SDCMLCEncoder.Init(TransmParam, SDCMapBuf);
	
	MSCMLCEncoder.Init(TransmParam, MLCEncBuf);
	SymbInterleaver.Init(TransmParam, IntlBuf);
	GenerateFACData.Init(TransmParam, GenFACDataBuf);
	FACMLCEncoder.Init(TransmParam, FACMapBuf);
	GenerateSDCData.Init(TransmParam, GenSDCDataBuf);
	OFDMModulation.Init(TransmParam, OFDMModBuf);
	AudioSourceEncoder.Init(TransmParam, AudSrcBuf);
	ReadData.Init(TransmParam, DataBuf);
	TransmitData.Init(TransmParam);
}

void CDRMTransmitter::StartParameters(CParameter& Param)
{
	/* Init streams */
	Param.ResetServicesStreams();

	/* Init frame ID counter (index) */
	Param.iFrameIDTransm = 0;

	/* Date, time. TODO: use computer system time... */
	Param.iDay = 0;
	Param.iMonth = 0;
	Param.iYear = 0;
	Param.iUTCHour = 0;
	Param.iUTCMin = 0;


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
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

	/* Protection levels for MSC. Depend on the modulation scheme. Look at
	   TableMLC.h, iCodRateCombMSC16SM, iCodRateCombMSC64SM,
	   iCodRateCombMSC64HMsym, iCodRateCombMSC64HMmix for available numbers */
	Param.MSCPrLe.iPartA = 0;
	Param.MSCPrLe.iPartB = 1;
	Param.MSCPrLe.iHierarch = 0;

	/* Either one audio or one data service can be chosen */
	_BOOLEAN bIsAudio = TRUE;

	/* In the current version only one service and one stream is supported. The
	   stream IDs must be 0 in both cases */
	if (bIsAudio == TRUE)
	{
		/* Audio */
		Param.iNumAudioService = 1;
		Param.iNumDataService = 0;

		Param.Service[0].eAudDataFlag = CParameter::SF_AUDIO;
		Param.Service[0].AudioParam.iStreamID = 0;

		/* Text message */
		Param.Service[0].AudioParam.bTextflag = TRUE;

		/* Programme Type code (see TableFAC.h, "strTableProgTypCod[]") */
		Param.Service[0].iServiceDescr = 15; /* 15 -> other music */
	}
	else
	{
		/* Data */
		Param.iNumAudioService = 0;
		Param.iNumDataService = 1;

		Param.Service[0].eAudDataFlag = CParameter::SF_DATA;
		Param.Service[0].DataParam.iStreamID = 0;

		/* Init SlideShow application */
		Param.Service[0].DataParam.iPacketLen = 45; /* TEST */
		Param.Service[0].DataParam.eDataUnitInd = CParameter::DU_DATA_UNITS;
		Param.Service[0].DataParam.eAppDomain = CParameter::AD_DAB_SPEC_APP;

		/* The value 0 indicates that the application details are provided
		   solely by SDC data entity type 5 */
		Param.Service[0].iServiceDescr = 0;
	}

	/* Length of part B is set automatically (equal error protection (EEP),
	   if "= 0"). Sets the number of bytes, should not exceed total number of
	   bytes available in MSC block */
	Param.Stream[0].iLenPartA = 0;

	/* Init service parameters, 24 bit unsigned integer number */
	Param.Service[0].iServiceID = 163569;

	/* Service label data. Up to 16 bytes defining the label using UTF-8
	   coding */
	Param.Service[0].strLabel = "Dream Test";

	/* Language (see TableFAC.h, "strTableLanguageCode[]") */
	Param.Service[0].iLanguage = 5; /* 5 -> english */

	/* Interleaver mode of MSC service. Long interleaving (2 s): SI_LONG,
	   short interleaving (400 ms): SI_SHORT */
	Param.eSymbolInterlMode = CParameter::SI_LONG;

	/* MSC modulation scheme. Available modes:
	   16-QAM standard mapping (SM): CS_2_SM,
	   64-QAM standard mapping (SM): CS_3_SM,
	   64-QAM symmetrical hierarchical mapping (HMsym): CS_3_HMSYM,
	   64-QAM mixture of the previous two mappings (HMmix): CS_3_HMMIX */
	Param.eMSCCodingScheme = CParameter::CS_3_SM;

	/* SDC modulation scheme. Available modes:
	   4-QAM standard mapping (SM): CS_1_SM,
	   16-QAM standard mapping (SM): CS_2_SM */
	Param.eSDCCodingScheme = CParameter::CS_2_SM;

	/* Set desired intermedia frequency (IF) in Hertz */
	OFDMModulation.SetCarOffset(12000.0); /* Default: "VIRTUAL_INTERMED_FREQ" */

#ifdef WRITE_TRNSM_TO_FILE
	/* Set the number of MSC frames we want to generate */
	ReadData.SetNumTransBlocks(200);
#endif
}
