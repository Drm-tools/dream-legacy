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

	/* Defines no of cells, important! */
	OFDMCellMapping.Init(TransmParam, CarMapBuf);

	/* Defines No of SDC bits per super-frame */
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

	/* These parameters are not yet used by this application */
	Param.iAFSIndex = 1;
	Param.iReConfigIndex = 0;
	Param.eAFSFlag = CParameter::AS_VALID;
	Param.eBaseEnhFlag = CParameter::BE_BASE_LAYER;

	/* Use 6.3.6 to set this two parameters! */
	Param.FACRepitition[0] = 0;
	Param.FACNoRep = 1;

	/* Date, time */
	Param.iDay = 0;
	Param.iMonth = 0;
	Param.iYear = 0;
	Param.iUTCHour = 0;
	Param.iUTCMin = 0;

	/* Frame ID */
	Param.iFrameIDTransm = 0;


	/**************************************************************************/
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);
	Param.iNoAudioService = 1;
	Param.iNoDataService = 0;
	Param.Service[0].AudioParam.iStreamID = 0;

	Param.MSCPrLe.iPartA = 0;
	Param.MSCPrLe.iPartB = 1;
	Param.MSCPrLe.iHierarch = 0;

	Param.Stream[0].iLenPartA = 84; /* More important than partB */
	Param.Stream[0].iLenPartB = 1040;
	Param.Service[0].iServiceID = 163569;
	Param.Service[0].strLabel = "Dream Test"; /* Not transmitted yet */
	Param.eSymbolInterlMode = CParameter::SI_SHORT;
	Param.eMSCCodingScheme = CParameter::CS_3_SM;
	Param.eSDCCodingScheme = CParameter::CS_2_SM;

	/* Set the number of MSC frames we want to generate */
	ReadData.SetNoTransBlocks(20);
}
