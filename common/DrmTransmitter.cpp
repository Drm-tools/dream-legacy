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
void CDRMTransmitter::StartTransmitter()
{
	/* Set run flag */
	TransmParam.bRunThread = TRUE;

	/* Initialization of the modules */
	InitTransmitter();

	/* Start of the simulation */
	TransmitterMainRoutine();
}

void CDRMTransmitter::TransmitterMainRoutine()
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
	
		/* MLC-encoder */
		MSCMLCEncoder.ProcessData(TransmParam, DataBuf, MLCEncBuf);
	
		/* Convolutional interleaver */
		SymbInterleaver.ProcessData(TransmParam, MLCEncBuf, IntlBuf);


		/* FAC ****************************************************************/
		GenerateFACData.ReadData(TransmParam, GenFACDataBuf);
		FACMLCEncoder.ProcessData(TransmParam, GenFACDataBuf, FACMapBuf);


		/* SDC ****************************************************************/
		GenerateSDCData.ReadData(TransmParam, GenSDCDataBuf);
		SDCMLCEncoder.ProcessData(TransmParam, GenSDCDataBuf, SDCMapBuf);

	
		/* Mapping of the MSC, FAC, SDC and pilots on the carriers ************/
		OFDMCellMapping.ProcessData(TransmParam, IntlBuf,
												 FACMapBuf,
												 SDCMapBuf,
												 CarMapBuf);
	
		/* OFDM-modulation ****************************************************/
		OFDMModulation.ProcessData(TransmParam, CarMapBuf, OFDMModBuf);
	
		/* Transmit the signal ************************************************/
		TransmitData.WriteData(TransmParam, OFDMModBuf);
	}
}

void CDRMTransmitter::InitTransmitter()
{
	/* Read parameters */
	StartParameters(TransmParam);

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
	TransmitData.Init(TransmParam);
	ReadData.Init(TransmParam, DataBuf);
}

void CDRMTransmitter::StartParameters(CParameter& Param)
{
	/* Init streams */
	Param.ResetServicesStreams();

	/* Use 6.3.6 to set these two parameters! If only one service with ID = 0
	   is present, the following parameters are correct */
	Param.FACRepetition[0] = 0;
	Param.FACNumRep = 1; /* Length of the repetition pattern table */

	/* Date, time */
	Param.iDay = 0;
	Param.iMonth = 0;
	Param.iYear = 0;
	Param.iUTCHour = 0;
	Param.iUTCMin = 0;

	/* Frame ID */
	Param.iFrameIDTransm = 0;


	/**************************************************************************/
	/* In the current version only one service and one stream is supported. The
	   IDs must be 0 in both cases */
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

	Param.MSCPrLe.iPartA = 0;
	Param.MSCPrLe.iPartB = 1;
	Param.MSCPrLe.iHierarch = 0;

	/* Either one audio or one data service can be chosen */
	_BOOLEAN bIsAudio = FALSE;

	if (bIsAudio == TRUE)
	{
		/* Audio */
		Param.iNumAudioService = 1;
		Param.iNumDataService = 0;

		Param.Service[0].eAudDataFlag = CParameter::SF_AUDIO;
		Param.Service[0].AudioParam.iStreamID = 0;
	}
	else
	{
		/* Data */
		Param.iNumAudioService = 0;
		Param.iNumDataService = 1;

		Param.Service[0].eAudDataFlag = CParameter::SF_DATA;
		Param.Service[0].DataParam.iStreamID = 0;
	}

	/* Length of part B is set automatically
	   (equal error protection, if "= 0") */
	Param.Stream[0].iLenPartA = 80;

	/* Init service parameters */
	Param.Service[0].iServiceID = 163569;
	Param.Service[0].strLabel = "Dream Test"; /* Has to be UTF-8! */
	Param.Service[0].iLanguage = 5; /* Language: 5 -> english */
	Param.Service[0].iServiceDescr = 15; /* Service description: 
											15 -> other music */

	Param.eSymbolInterlMode = CParameter::SI_SHORT;
	Param.eMSCCodingScheme = CParameter::CS_3_SM;
	Param.eSDCCodingScheme = CParameter::CS_2_SM;

	/* Set the number of MSC frames we want to generate */
	ReadData.SetNumTransBlocks(70);
}
