/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-simulation
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

#include "DrmSimulation.h"


/* Implementation *************************************************************/
void CDRMSimulation::Run()
{
/*
 The hand over of data is done via an intermediate-buffer. The calling
 convention is always "input-buffer, output-buffer". Additional, the
 DRM-parameters are fed to the function
*/

	/* Initialization of the modules */
	Init();

	/* Set run flag */
	Param.bRunThread = TRUE;

	while (Param.bRunThread)
	{
		/**********************************************************************\
		* Transmitter														   *
		\**********************************************************************/
		/* MSC -------------------------------------------------------------- */
		/* Read the source signal */
		GenSimData.ReadData(Param, DataBuf);

		/* MLC-encoder */
		MSCMLCEncoder.ProcessData(Param, DataBuf, MLCEncBuf);

		/* Convolutional interleaver */
		SymbInterleaver.ProcessData(Param, MLCEncBuf, IntlBuf);


		/* FAC -------------------------------------------------------------- */
		GenerateFACData.ReadData(Param, GenFACDataBuf);
		FACMLCEncoder.ProcessData(Param, GenFACDataBuf, FACMapBuf);


		/* SDC -------------------------------------------------------------- */
		GenerateSDCData.ReadData(Param, GenSDCDataBuf);
		SDCMLCEncoder.ProcessData(Param, GenSDCDataBuf, SDCMapBuf);


		/* Mapping of the MSC, FAC, SDC and pilots on the carriers */
		OFDMCellMapping.ProcessMultipleData(Param, IntlBuf, FACMapBuf,
			SDCMapBuf, CarMapBuf);

		/* OFDM-modulation */
		OFDMModulation.ProcessData(Param, CarMapBuf, OFDMModBuf);



switch (eSimType)
{
case ST_MSECHANEST:
case ST_BER_IDEALCHAN:
		/**********************************************************************\
		* MSE of channel estimation, ideal channel estimation				   *
		\**********************************************************************/
		/* DRM channel simulation ------------------------------------------- */
		DRMChannel.TransferData(Param, OFDMModBuf, RecDataBuf, ChanInRefBuf,
			ChanRefBuf);


		/* Receiver --------------------------------------------------------- */
		/* Special OFDM demodulation for channel estimation tests (with guard-
		   interval removal) */
		OFDMDemodSimulation.ProcessMultipleData(Param,
			/* Input buffers */
			RecDataBuf, ChanInRefBuf, ChanRefBuf,
			/* Output buffers */
			OFDMDemodBuf, OFDMDemodBuf2, DemChanInRefBuf, DemChanRefBuf);
	
		/* Channel estimation and equalisation */
		ChannelEstimation.ProcessData(Param, OFDMDemodBuf, ChanEstBuf);

		/* This module converts the "CEquSig" data type of "ChanEstBuf" to the
		   "_COMPLEX" data type, because a module can only have ONE type of
		   input buffers (even in ProcessMultipleData() case) */
		DataConv.ProcessData(Param, ChanEstBuf, ChanEstBufForSim);

		/* Ideal channel estimation (with MSE calculation) */
		IdealChanEst.ProcessMultipleData(Param,
			/* Input buffers */
			ChanEstBufForSim, OFDMDemodBuf2, DemChanInRefBuf, DemChanRefBuf,
			/* Output buffer */
			ChanEstBuf);
	break;


case ST_BITERROR:
		/**********************************************************************\
		* Bit error rate (we can use all synchronization units here!)		   *
		\**********************************************************************/
		/* DRM channel simulation ------------------------------------------- */
		DRMChannel.TransferData(Param, OFDMModBuf, RecDataBuf);


		/* Receiver --------------------------------------------------------- */
		/* Resample input DRM-stream */
		InputResample.ProcessData(Param, RecDataBuf, InpResBuf);

		/* Frequency synchronization acquisition */
		FreqSyncAcq.ProcessData(Param, InpResBuf, FreqSyncAcqBuf);

		/* Time synchronization */
		TimeSync.ProcessData(Param, FreqSyncAcqBuf, TimeSyncBuf);

		/* OFDM-demodulation */
		OFDMDemodulation.ProcessData(Param, TimeSyncBuf, OFDMDemodBuf);

		/* Synchronization in the frequency domain (using pilots) */
		SyncUsingPil.ProcessData(Param, OFDMDemodBuf, SyncUsingPilBuf);

		/* Channel estimation and equalization */
		ChannelEstimation.ProcessData(Param, SyncUsingPilBuf, ChanEstBuf);
	break;
}

		/* Demapping of the MSC, FAC, SDC and pilots from the carriers */
		OFDMCellDemapping.ProcessMultipleData(Param, ChanEstBuf,
			MSCCarDemapBuf, FACCarDemapBuf, SDCCarDemapBuf);

		/* FAC */
		FACMLCDecoder.ProcessData(Param, FACCarDemapBuf, FACDecBuf);
		UtilizeFACData.WriteData(Param, FACDecBuf);


		/* SDC */
		SDCMLCDecoder.ProcessData(Param, SDCCarDemapBuf, SDCDecBuf);
		UtilizeSDCData.WriteData(Param, SDCDecBuf);


		/* MSC */
		/* Symbol de-interleaver */
		SymbDeinterleaver.ProcessData(Param, MSCCarDemapBuf, DeintlBuf);

		/* MLC-decoder */
		MSCMLCDecoder.ProcessData(Param, DeintlBuf, MSCMLCDecBuf);

		/* Evaluate simulation data */
		EvaSimData.WriteData(Param, MSCMLCDecBuf);
	}
}

void CDRMSimulation::Init()
{
	/* Defines no of cells, important! */
	OFDMCellMapping.Init(Param, CarMapBuf);

	/* Defines No of SDC bits per super-frame */
	SDCMLCEncoder.Init(Param, SDCMapBuf);
	
	MSCMLCEncoder.Init(Param, MLCEncBuf);
	SymbInterleaver.Init(Param, IntlBuf);
	GenerateFACData.Init(Param, GenFACDataBuf);
	FACMLCEncoder.Init(Param, FACMapBuf);
	GenerateSDCData.Init(Param, GenSDCDataBuf);
	OFDMModulation.Init(Param, OFDMModBuf);
	GenSimData.Init(Param, DataBuf);


	/* Receiver modules */
	/* The order of modules are important! */
	InputResample.Init(Param, InpResBuf);
	FreqSyncAcq.Init(Param, FreqSyncAcqBuf);
	TimeSync.Init(Param, TimeSyncBuf);
	SyncUsingPil.Init(Param, SyncUsingPilBuf);

	/* Channel estimation init must be called before OFDMDemodSimulation
	   module, because the delay is set here which the other modules use! */
	ChannelEstimation.Init(Param, ChanEstBuf);
	
	OFDMCellDemapping.Init(Param, MSCCarDemapBuf, FACCarDemapBuf, 
		SDCCarDemapBuf);
	FACMLCDecoder.Init(Param, FACDecBuf);
	UtilizeFACData.Init(Param);
	SDCMLCDecoder.Init(Param, SDCDecBuf);
	UtilizeSDCData.Init(Param);
	SymbDeinterleaver.Init(Param, DeintlBuf);
	MSCMLCDecoder.Init(Param, MSCMLCDecBuf);


	/* Special module for simulation */
	EvaSimData.Init(Param);

	/* Mode dependent initializations */
	switch (eSimType)
	{
	case ST_MSECHANEST:
	case ST_BER_IDEALCHAN:
		/* Init channel */
		DRMChannel.Init(Param, RecDataBuf, ChanInRefBuf, ChanRefBuf);

		OFDMDemodSimulation.Init(Param, OFDMDemodBuf, OFDMDemodBuf2,
			DemChanInRefBuf, DemChanRefBuf);
		
		DataConv.Init(Param, ChanEstBufForSim);

		IdealChanEst.Init(Param, ChanEstBuf);
		break;

	case ST_BITERROR:
		/* Init channel */
		DRMChannel.Init(Param, RecDataBuf);

		OFDMDemodulation.Init(Param, OFDMDemodBuf);
		break;
	}

	/* We only want to simulate tracking performance ------------------------ */
	TimeSync.StopTimingAcqu();
	TimeSync.StopRMDetAcqu(); /* Robustness mode detection */
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();

	/* Disable FAC evaluation to make sure that no mistakenly correct CRC
	   sets false parameters which can cause run-time errors */
	UtilizeFACData.SetSyncInput(TRUE);

	/* We have to first start aquisition and then stop it right after it to set
	   internal parameters */
	SyncUsingPil.StartAcquisition();
	SyncUsingPil.StopAcquisition();

	SyncUsingPil.StartTrackPil();
}

CDRMSimulation::CDRMSimulation()
{
	/* Set all parameters to meaningful value for startup state. If we want to
	   make a simulation we just have to specify the important values */
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

	/* Frame IDs */
	Param.iFrameIDTransm = 0;
	Param.iFrameIDReceiv = 0;

	/* Initialize synchronization parameters */
	Param.rResampleOffset = (_REAL) 0.0;
	Param.rFreqOffsetAcqui = (_REAL) 0.0;
	Param.rFreqOffsetTrack = (_REAL) 0.0;
	Param.rTimingOffsTrack = (_REAL) 0.0;

	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);
	Param.iNoAudioService = 1;
	Param.iNoDataService = 0;
	Param.Service[0].AudioParam.iStreamID = 0;

	Param.MSCPrLe.iPartA = 1;
	Param.MSCPrLe.iPartB = 1;
	Param.MSCPrLe.iHierarch = 0;

	Param.Stream[0].iLenPartA = 0; // EEP, if "= 0"
	Param.eSymbolInterlMode = CParameter::SI_SHORT;
	Param.eMSCCodingScheme = CParameter::CS_3_SM;
	Param.eSDCCodingScheme = CParameter::CS_2_SM;

	/* DRM channel parameters */
	Param.iDRMChannelNo = 1;
	Param.rSimSNRdB = 25;

	/* Length of simulation */
	GenSimData.SetSimTime(300);
}
