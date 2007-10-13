/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Julian Cable
 *
 * Description:
 *	DRM-receiver
 * The hand over of data is done via an intermediate-buffer. The calling
 * convention is always "input-buffer, output-buffer". Additionally, the
 * DRM-parameters are fed to the function.
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include AMSS demodulation
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

#include "DrmReceiver.h"
#include "util/Settings.h"

#include "sound.h"
#include "sound/soundnull.h"
#ifdef __linux__
# include "source/shmsoundin.h"
#endif
#include "audiofilein.h"

const int
	CDRMReceiver::MAX_UNLOCKED_COUNT = 2;

/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver():
pSoundInInterface(new CSoundInNull), pSoundOutInterface(new CSoundOut),
ReceiveData(), WriteData(pSoundOutInterface),
FreqSyncAcq(),
ChannelEstimation(),
UtilizeFACData(), UtilizeSDCData(), MSCDemultiplexer(),
AudioSourceDecoder(),
upstreamRSCI(), DecodeRSIMDI(), downstreamRSCI(),
pReceiverParam(NULL), pDRMParam(NULL), pAMParam(NULL),
RSIPacketBuf(),
MSCDecBuf(MAX_NUM_STREAMS), MSCUseBuf(MAX_NUM_STREAMS),
MSCSendBuf(MAX_NUM_STREAMS), iAcquRestartCnt(0),
iAcquDetecCnt(0), iGoodSignCnt(0), eReceiverMode(RM_DRM),
eNewReceiverMode(RM_DRM), iAudioStreamID(STREAM_ID_NOT_USED),
iDataStreamID(STREAM_ID_NOT_USED), bDoInitRun(FALSE), bRestartFlag(FALSE),
rInitResampleOffset((_REAL) 0.0),
pHamlib(NULL),
iFreqkHz(0),
time_keeper(0),
pcmInput(Dummy),
demodulation(onBoard)
{
	pReceiverParam = new CParameter(this);
	downstreamRSCI.SetReceiver(this);
	PlotManager.SetReceiver(this);
}

CDRMReceiver::~CDRMReceiver()
{
	delete pSoundInInterface;
	delete pSoundOutInterface;
}

void
CDRMReceiver::SetReceiverMode(ERecMode eNewMode)
{
	if (eReceiverMode!=eNewMode || eNewReceiverMode != RM_NONE)
		eNewReceiverMode = eNewMode;


}

void
CDRMReceiver::SetEnableSMeter(_BOOLEAN bNew)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->SetEnableSMeter(bNew);
#endif
}

_BOOLEAN
CDRMReceiver::GetEnableSMeter()
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		return pHamlib->GetEnableSMeter();
#endif
	return FALSE;
}

void CDRMReceiver::SetUseHWDemod(_BOOLEAN bUse)
{
	CParameter & Parameters = *pReceiverParam;
	Parameters.bUseHWDemod = bUse;
	if(bUse)
	{
		OnboardDecoder.SetInitFlag();
	}
	else
	{
		AMDemodulation.SetInitFlag();
		AMSSPhaseDemod.SetInitFlag();
		InputResample.SetInitFlag();
		AMSSExtractBits.SetInitFlag();
		AMSSDecode.SetInitFlag();
		ReceiveData.SetInitFlag();
	}
}

_BOOLEAN CDRMReceiver::GetUseHWDemod()
{
	return pReceiverParam->bUseHWDemod;
}

void
CDRMReceiver::SetAMDemodType(EDemodType eNew)
{
	CParameter & Parameters = *pReceiverParam;

	Parameters.eDemodType = eNew;
	AMDemodulation.SetDemodTypeAndBPF(Parameters.eDemodType, Parameters.iBw[eNew]);
	UpdateRigSettings();
}

void
CDRMReceiver::SetAMFilterBW(int value)
{
	/* Store filter bandwidth for this demodulation type */
	CParameter & Parameters = *pReceiverParam;
	Parameters.iBw[Parameters.eDemodType] = value;
	AMDemodulation.SetDemodTypeAndBPF(Parameters.eDemodType, value);
}

void
CDRMReceiver::Run()
{
	_BOOLEAN bEnoughData = TRUE;
	_BOOLEAN bFrameToSend = FALSE;
	size_t i;
	/* Check for parameter changes from RSCI or GUI thread --------------- */
	/* The parameter changes are done through flags, the actual initialization
	 * is done in this (the working) thread to avoid problems with shared data */
	if (eNewReceiverMode != RM_NONE)
		InitReceiverMode();

	CParameter & ReceiverParam = *pReceiverParam;

	if(bRestartFlag) /* new acquisition requested by GUI */
	{
		bRestartFlag = FALSE;
		SetInStartMode();
	}

	/* Input - from upstream RSCI or input and demodulation from sound card / file */

	if (upstreamRSCI.GetInEnabled() == TRUE)
	{
		if (bDoInitRun == FALSE)	/* don't wait for a packet in Init mode */
		{
			RSIPacketBuf.Clear();
			upstreamRSCI.ReadData(ReceiverParam, RSIPacketBuf);
			if (RSIPacketBuf.GetFillLevel() > 0)
			{
				time_keeper = time(NULL);
				DecodeRSIMDI.ProcessData(ReceiverParam, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
				PlotManager.UpdateParamHistoriesRSIIn();
				bFrameToSend = TRUE;
			}
			else
			{
				time_t now = time(NULL);
				if ((now - time_keeper) > 2)
				{
					ReceiverParam.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
				}
			}
		}
	}
	else
	{
		if(eReceiverMode==RM_AM && ReceiverParam.bUseHWDemod)
		{
			OnboardDecoder.ReadData(ReceiverParam, AMAudioBuf);
		}
		else
		{
			ReceiveData.ReadData(ReceiverParam, RecDataBuf);

			// Split samples, one output to the demodulation, another for IQ recording
			if (SplitForIQRecord.ProcessData(ReceiverParam, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
			{
				bEnoughData = TRUE;
			}

			switch(eReceiverMode)
			{
			case RM_DRM:
					DemodulateDRM(bEnoughData);
					DecodeDRM(bEnoughData, bFrameToSend);
					break;
			case RM_AM:
					DemodulateAM(bEnoughData);
					DecodeAM(bEnoughData);
					break;
			case RM_NONE:
					break;
			}
		}
	}

	/* Split the data for downstream RSCI and local processing. TODO make this conditional */
	switch(eReceiverMode)
	{
	case RM_DRM:
		SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);

		/* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
		if (SDCDecBuf.GetFillLevel() == ReceiverParam.iNumSDCBitsPerSFrame)
		{
			SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
		}

		for (i = 0; i < MSCDecBuf.size(); i++)
		{
			SplitMSC[i].ProcessData(ReceiverParam, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
		}
		break;
	case RM_AM:
		SplitAudio.ProcessData(ReceiverParam, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
		break;
	case RM_NONE:
		break;
	}

	/* decoding */
	while (bEnoughData && ReceiverParam.bRunThread)
	{
		/* Init flag */
		bEnoughData = FALSE;

		// Write output I/Q file
		if (WriteIQFile.WriteData(ReceiverParam, IQRecordDataBuf))
		{
			bEnoughData = TRUE;
		}

		switch(eReceiverMode)
		{
		case RM_DRM:
			UtilizeDRM(bEnoughData);
			break;
		case RM_AM:
			UtilizeAM(bEnoughData);
			break;
		case RM_NONE:
			break;
		}
	}

	/* output to downstream RSCI */
	if (downstreamRSCI.GetOutEnabled())
	{
		switch(eReceiverMode)
		{
		case RM_DRM:
			if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
			{
				/* we will get one of these between each FAC block, and occasionally we */
				/* might get two, so don't start generating free-wheeling RSCI until we've. */
				/* had three in a row */
				if (FreqSyncAcq.GetUnlockedFrameBoundary())
				{
					if (iUnlockedCount < MAX_UNLOCKED_COUNT)
						iUnlockedCount++;
					else
						downstreamRSCI.SendUnlockedFrame(ReceiverParam);
				}
			}
			else if (bFrameToSend)
			{
				downstreamRSCI.SendLockedFrame(ReceiverParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
				iUnlockedCount = 0;
				bFrameToSend = FALSE;
			}
			break;
		case RM_AM:
			/* Encode audio for RSI output */
			if (AudioSourceEncoder.ProcessData(ReceiverParam, AMSoEncBuf, MSCSendBuf[0]))
				bFrameToSend = TRUE;

			if (bFrameToSend)
				downstreamRSCI.SendAMFrame(ReceiverParam, MSCSendBuf[0]);
			break;
		case RM_NONE:
			break;
		}
	}

	/* Play and/or save the audio */
	if (iAudioStreamID != STREAM_ID_NOT_USED || eReceiverMode == RM_AM)
	{
		if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
		{
			bEnoughData = TRUE;
		}
	}
}

void
CDRMReceiver::DemodulateDRM(_BOOLEAN& bEnoughData)
{
	CParameter & ReceiverParam = *pReceiverParam;

	/* Resample input DRM-stream -------------------------------- */
	if (InputResample.ProcessData(ReceiverParam, DemodDataBuf, InpResBuf))
	{
		bEnoughData = TRUE;
	}

	/* Frequency synchronization acquisition -------------------- */
	if (FreqSyncAcq.ProcessData(ReceiverParam, InpResBuf, FreqSyncAcqBuf))
	{
		bEnoughData = TRUE;
	}

	/* Time synchronization ------------------------------------- */
	if (TimeSync.ProcessData(ReceiverParam, FreqSyncAcqBuf, TimeSyncBuf))
	{
		bEnoughData = TRUE;
		/* Use count of OFDM-symbols for detecting
		 * aquisition state for acquisition detection
		 * only if no signal was decoded before */
		if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
		{
			/* Increment symbol counter and check if bound is reached */
			iAcquDetecCnt++;

			if (iAcquDetecCnt > NUM_OFDMSYM_U_ACQ_WITHOUT)
				SetInStartMode();
		}
	}

	/* OFDM-demodulation ---------------------------------------- */
	if (OFDMDemodulation.
		ProcessData(ReceiverParam, TimeSyncBuf, OFDMDemodBuf))
	{
		bEnoughData = TRUE;
	}

	/* Synchronization in the frequency domain (using pilots) --- */
	if (SyncUsingPil.
		ProcessData(ReceiverParam, OFDMDemodBuf, SyncUsingPilBuf))
	{
		bEnoughData = TRUE;
	}

	/* Channel estimation and equalisation ---------------------- */
	if (ChannelEstimation.
		ProcessData(ReceiverParam, SyncUsingPilBuf, ChanEstBuf))
	{
		bEnoughData = TRUE;

		/* If this module has finished, all synchronization units
		   have also finished their OFDM symbol based estimates.
		   Update synchronization parameters histories */
		PlotManager.UpdateParamHistories(eReceiverState);
	}

	/* Demapping of the MSC, FAC, SDC and pilots off the carriers */
	if (OFDMCellDemapping.ProcessData(ReceiverParam, ChanEstBuf,
									  MSCCarDemapBuf,
									  FACCarDemapBuf, SDCCarDemapBuf))
	{
		bEnoughData = TRUE;
	}

}

void
CDRMReceiver::DecodeDRM(_BOOLEAN& bEnoughData, _BOOLEAN& bFrameToSend)
{
	CParameter & ReceiverParam = *pReceiverParam;

	/* FAC ------------------------------------------------------ */
	if (FACMLCDecoder.ProcessData(ReceiverParam, FACCarDemapBuf, FACDecBuf))
	{
		bEnoughData = TRUE;
		bFrameToSend = TRUE;
	}

	/* SDC ------------------------------------------------------ */
	if (SDCMLCDecoder.ProcessData(ReceiverParam, SDCCarDemapBuf, SDCDecBuf))
	{
		bEnoughData = TRUE;
	}

	/* MSC ------------------------------------------------------ */

	/* Symbol de-interleaver */
	if (SymbDeinterleaver.ProcessData(ReceiverParam, MSCCarDemapBuf, DeintlBuf))
	{
		bEnoughData = TRUE;
	}

	/* MLC decoder */
	if (MSCMLCDecoder.ProcessData(ReceiverParam, DeintlBuf, MSCMLCDecBuf))
	{
		bEnoughData = TRUE;
	}

	/* MSC demultiplexer (will leave FAC & SDC alone! */
	if (MSCDemultiplexer.ProcessData(ReceiverParam, MSCMLCDecBuf, MSCDecBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::UtilizeDRM(_BOOLEAN& bEnoughData)
{
	CParameter & ReceiverParam = *pReceiverParam;

	if (UtilizeFACData.WriteData(ReceiverParam, FACUseBuf))
	{
		bEnoughData = TRUE;

		/* Use information of FAC CRC for detecting the acquisition
		   requirement */
		DetectAcquiFAC();
#if 0
		saveSDCtoFile();
#endif
	}

	if (UtilizeSDCData.WriteData(ReceiverParam, SDCUseBuf))
	{
		bEnoughData = TRUE;
	}

	/* Data decoding */
	if (iDataStreamID != STREAM_ID_NOT_USED)
	{
		if (DataDecoder.WriteData(ReceiverParam, MSCUseBuf[iDataStreamID]))
			bEnoughData = TRUE;
	}
	/* Source decoding (audio) */
	if (iAudioStreamID != STREAM_ID_NOT_USED)
	{
		if (AudioSourceDecoder.ProcessData(ReceiverParam,
										   MSCUseBuf[iAudioStreamID],
										   AudSoDecBuf))
		{
			bEnoughData = TRUE;

			/* Store the number of correctly decoded audio blocks for
			 *                            the history */
			PlotManager.SetCurrentCDAud(AudioSourceDecoder.GetNumCorDecAudio());
		}
	}
}

void
CDRMReceiver::DemodulateAM(_BOOLEAN& bEnoughData)
{
	CParameter & ReceiverParam = *pReceiverParam;

	/* The incoming samples are split 2 ways.
	   One set is passed to the existing AM demodulator.
	   The other set is passed to the new AMSS demodulator.
	   The AMSS and AM demodulators work completely independently
	 */
	if (Split.ProcessData(ReceiverParam, DemodDataBuf, AMDataBuf, AMSSDataBuf))
	{
		bEnoughData = TRUE;
	}

	/* AM demodulation ------------------------------------------ */
	if (AMDemodulation.ProcessData(ReceiverParam, AMDataBuf, AMAudioBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS phase demodulation */
	if (AMSSPhaseDemod.ProcessData(ReceiverParam, AMSSDataBuf, AMSSPhaseBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::DecodeAM(_BOOLEAN& bEnoughData)
{
	CParameter & ReceiverParam = *pReceiverParam;

	/* AMSS resampling */
	if (InputResample.ProcessData(ReceiverParam, AMSSPhaseBuf, AMSSResPhaseBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS bit extraction */
	if (AMSSExtractBits.
		ProcessData(ReceiverParam, AMSSResPhaseBuf, AMSSBitsBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS data decoding */
	if (AMSSDecode.ProcessData(ReceiverParam, AMSSBitsBuf, SDCDecBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::UtilizeAM(_BOOLEAN& bEnoughData)
{
	CParameter & ReceiverParam = *pReceiverParam;

	if (UtilizeSDCData.WriteData(ReceiverParam, SDCDecBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::DetectAcquiFAC()
{
	/* If upstreamRSCI in is enabled, do not check for acquisition state because we want
	   to stay in tracking mode all the time */
	if (upstreamRSCI.GetInEnabled() == TRUE)
		return;

	/* Acquisition switch */
	if (!UtilizeFACData.GetCRCOk())
	{
		/* Reset "good signal" count */
		iGoodSignCnt = 0;

		iAcquRestartCnt++;

		/* Check situation when receiver must be set back in start mode */
		if ((pReceiverParam->eAcquiState == AS_WITH_SIGNAL)
			&& (iAcquRestartCnt > NUM_FAC_FRA_U_ACQ_WITH))
		{
			SetInStartMode();
		}
	}
	else
	{
		/* Set the receiver state to "with signal" not until two successive FAC
		   frames are "ok", because there is only a 8-bit CRC which is not good
		   for many bit errors. But it is very unlikely that we have two
		   successive FAC blocks "ok" if no good signal is received */
		if (iGoodSignCnt > 0)
		{
			pReceiverParam->eAcquiState = AS_WITH_SIGNAL;

			/* Take care of delayed tracking mode switch */
			if (iDelayedTrackModeCnt > 0)
				iDelayedTrackModeCnt--;
			else
				SetInTrackingModeDelayed();
		}
		else
		{
			/* If one CRC was correct, reset acquisition since
			   we assume, that this was a correct detected signal */
			iAcquRestartCnt = 0;
			iAcquDetecCnt = 0;

			/* Set in tracking mode */
			SetInTrackingMode();

			iGoodSignCnt++;
		}
	}
}

void
CDRMReceiver::Init()
{
	/* Set flags so that we have only one loop in the Run() routine which is
	   enough for initializing all modues */
	bDoInitRun = TRUE;
	pReceiverParam->bRunThread = TRUE;

	/* Run once */
	Run();

	/* Reset flags */
	bDoInitRun = FALSE;
	pReceiverParam->bRunThread = FALSE;
}

void
CDRMReceiver::InitReceiverMode()
{
	switch(eNewReceiverMode)
	{
	case RM_AM:
		if (pAMParam == NULL)
		{
			/* its the first time we have been in AM mode */
			if (pDRMParam == NULL)
			{
				/* DRM Mode was never invoked so we get to claim the default parameter instance */
				pAMParam = pReceiverParam;
			}
			else
			{
				/* change from DRM to AM Mode - we better have our own copy
				 * but make sure we inherit the initial settings of the default
				 */
				pAMParam = new CParameter(*pDRMParam);
			}
		}
		else
		{
			/* we have been in AM mode before, and have our own parameters but
			 * we might need some state from the DRM mode params
			 */
			switch(eReceiverMode)
			{
			case RM_AM:
				/* AM to AM switch - re-acquisition requested - no special action */
				break;
			case RM_DRM:
				/* DRM to AM switch - grab some common stuff */
 				pAMParam->rSigStrengthCorrection = pDRMParam->rSigStrengthCorrection;
 				pAMParam->bMeasurePSD = pDRMParam->bMeasurePSD;
				pAMParam->bMeasureInterference = pDRMParam->bMeasureInterference;
 				pAMParam->FrontEndParameters = pDRMParam->FrontEndParameters;
 				pAMParam->GPSData = pDRMParam->GPSData;
				pAMParam->sSerialNumber = pDRMParam->sSerialNumber;
				pAMParam->sReceiverID  = pDRMParam->sReceiverID;
				pAMParam->sDataFilesDirectory = pDRMParam->sDataFilesDirectory;
				pAMParam->SetFrequency(pDRMParam->GetFrequency());
				//pAMParam->bUseHWDemod = pDRMParam->bUseHWDemod;
				break;
			case RM_NONE:
				/* Start from cold in AM mode - no special action */
				break;
			}
		}
		pAMParam->eReceiverMode = RM_AM;
		pReceiverParam = pAMParam;

		if (pReceiverParam == NULL)
			throw CGenErr("Something went terribly wrong in the Receiver");

		/* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);

		/* Set the receive status - this affects the RSI output */
		pAMParam->ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
		break;
	case RM_DRM:
		if (pDRMParam == NULL)
		{
			/* its the first time we have been in DRM mode */
			if (pAMParam == NULL)
			{
				/* AM Mode was never invoked so we get to claim the default parameter instance */
				pDRMParam = pReceiverParam;
			}
			else
			{
				/* change from AM to DRM Mode - we better have our own copy
				 * but make sure we inherit the initial settings of the default
				 */
				pDRMParam = new CParameter(*pAMParam);
			}
		}
		else
		{
			/* we have been in DRM mode before, and have our own parameters but
			 * we might need some state from the AM mode params
			 */
			switch(eReceiverMode)
			{
			case RM_AM:
				/* AM to DRM switch - grab some common stuff */
 				pDRMParam->rSigStrengthCorrection = pAMParam->rSigStrengthCorrection;
 				pDRMParam->bMeasurePSD = pAMParam->bMeasurePSD;
				pDRMParam->bMeasureInterference = pAMParam->bMeasureInterference;
 				pDRMParam->FrontEndParameters = pAMParam->FrontEndParameters;
 				pDRMParam->GPSData = pAMParam->GPSData;
				pDRMParam->sSerialNumber = pAMParam->sSerialNumber;
				pDRMParam->sReceiverID  = pAMParam->sReceiverID;
				pDRMParam->sDataFilesDirectory = pAMParam->sDataFilesDirectory;
				pDRMParam->SetFrequency(pAMParam->GetFrequency());
				//pDRMParam->bUseHWDemod = pAMParam->bUseHWDemod;
				break;
			case RM_DRM:
				/* DRM to DRM switch - re-acquisition requested - no special action */
				break;
			case RM_NONE:
				/* Start from cold in DRM mode - no special action */
				break;
			}
		}
		pDRMParam->eReceiverMode = RM_DRM;
		pReceiverParam = pDRMParam;

		if (pReceiverParam == NULL)
			throw CGenErr("Something went terribly wrong in the Receiver");

		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_DRM);
		break;
	case RM_NONE:
		return;
	}

	eReceiverMode = eNewReceiverMode;
	/* Reset new mode flag */
	eNewReceiverMode = RM_NONE;

	/* Init all modules */
	SetInStartMode();

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetReceiverMode(eReceiverMode);
	}
	UpdateRigSettings();
}

void
CDRMReceiver::UpdateRigSettings()
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
	{
		ERigMode eNewMode;
		CRigCaps caps;
		switch(eReceiverMode)
		{
		case RM_DRM:
			eNewMode = DRM;
			break;
		case RM_AM:
			switch (pReceiverParam->eDemodType)
			{
			case DT_AM:
				eNewMode = AM;
				break;
			case DT_LSB:
				eNewMode = LSB;
				break;
			case DT_USB:
				eNewMode = USB;
				break;
			case DT_CW:
				eNewMode = CW;
				break;
			case DT_NBFM:
				eNewMode = NBFM;
				break;
			case DT_WBFM:
				eNewMode = WBFM;
				break;
			case DT_SIZE:
				return;
			}
			break;
		case RM_NONE:
			return;
		}
		pHamlib->SetRigMode(eNewMode);
		pHamlib->GetRigCaps(caps);
		SetUseHWDemod(caps.settings[eNewMode].eOnboardDemod!=C_CANT);
	}
#endif
}

#ifdef USE_QT_GUI
void
CDRMReceiver::run()
{
#ifdef _WIN32
	/* it doesn't matter what the GUI does, we want to be normal! */
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif
	try
	{
		/* Call receiver main routine */
		Start();
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	qDebug("Working thread complete");
}
#endif

void
CDRMReceiver::Start()
{
	/* Set run flag so that the thread can work */
	pReceiverParam->bRunThread = TRUE;

	do
	{
		Run();

	}
	while (pReceiverParam->bRunThread);

	pSoundInInterface->Close();
	pSoundOutInterface->Close();
}

void
CDRMReceiver::Stop()
{
	pReceiverParam->bRunThread = FALSE;
}

void
CDRMReceiver::SetAMDemodAcq(_REAL rNewNorCen)
{
	/* Set the frequency where the AM demodulation should look for the
	   aquisition. Receiver must be in AM demodulation mode */
	if (eReceiverMode == RM_AM)
	{
		AMDemodulation.SetAcqFreq(rNewNorCen);
		AMSSPhaseDemod.SetAcqFreq(rNewNorCen);
	}
}

void
CDRMReceiver::SetInStartMode()
{
	CParameter & ReceiverParam = *pReceiverParam;

	iUnlockedCount = MAX_UNLOCKED_COUNT;

	ReceiverParam.Lock();
	/* Load start parameters for all modules */

	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	ReceiverParam.SetWaveMode(RM_ROBUSTNESS_MODE_B);
	ReceiverParam.SetSpectrumOccup(SO_3);

	/* Set initial MLC parameters */
	ReceiverParam.SetInterleaverDepth(CParameter::SI_LONG);
	ReceiverParam.SetMSCCodingScheme(CS_3_SM);
	ReceiverParam.SetSDCCodingScheme(CS_2_SM);

	/* Select the service we want to decode. Always zero, because we do not
	   know how many services are transmitted in the signal we want to
	   decode */

	/* TODO: if service 0 is not used but another service is the audio service
	 * we have a problem. We should check as soon as we have information about
	 * services if service 0 is really the audio service
	 */

	/* Set the following parameters to zero states (initial states) --------- */
	ReceiverParam.ResetServicesStreams();
	ReceiverParam.ResetCurSelAudDatServ();

	/* Protection levels */
	ReceiverParam.MSCPrLe.iPartA = 0;
	ReceiverParam.MSCPrLe.iPartB = 1;
	ReceiverParam.MSCPrLe.iHierarch = 0;

	/* Number of audio and data services */
	ReceiverParam.iNumAudioService = 0;
	ReceiverParam.iNumDataService = 0;

	/* We start with FAC ID = 0 (arbitrary) */
	ReceiverParam.iFrameIDReceiv = 0;

	/* Set synchronization parameters */
	ReceiverParam.rResampleOffset = rInitResampleOffset;	/* Initial resample offset */
	ReceiverParam.rFreqOffsetAcqui = (_REAL) 0.0;
	ReceiverParam.rFreqOffsetTrack = (_REAL) 0.0;
	ReceiverParam.iTimingOffsTrack = 0;

	ReceiverParam.Unlock();

	/* Initialization of the modules */
	InitsForAllModules();

	/* Activate acquisition */
	FreqSyncAcq.StartAcquisition();
	TimeSync.StartAcquisition();
	ChannelEstimation.GetTimeSyncTrack()->StopTracking();
	ChannelEstimation.StartSaRaOffAcq();
	ChannelEstimation.GetTimeWiener()->StopTracking();

	SyncUsingPil.StartAcquisition();
	SyncUsingPil.StopTrackPil();

	ReceiverParam.Lock();
	/* Set flag that no signal is currently received */
	ReceiverParam.eAcquiState = AS_NO_SIGNAL;

	/* Set flag for receiver state */
	eReceiverState = RS_ACQUISITION;

	/* Reset counters for acquisition decision, "good signal" and delayed
	   tracking mode counter */
	iAcquRestartCnt = 0;
	iAcquDetecCnt = 0;
	iGoodSignCnt = 0;
	iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

	/* Reset GUI lights */
	ReceiverParam.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);

	ReceiverParam.Unlock();

	/* In case upstreamRSCI is enabled, go directly to tracking mode, do not activate the
	   synchronization units */
	if (upstreamRSCI.GetInEnabled() == TRUE)
	{
		/* We want to have as low CPU usage as possible, therefore set the
		   synchronization units in a state where they do only a minimum
		   work */
		FreqSyncAcq.StopAcquisition();
		TimeSync.StopTimingAcqu();
		InputResample.SetSyncInput(TRUE);
		SyncUsingPil.SetSyncInput(TRUE);

		/* This is important so that always the same amount of module input
		   data is queried, otherwise it could be that amount of input data is
		   set to zero and the receiver gets into an infinite loop */
		TimeSync.SetSyncInput(TRUE);

		/* Always tracking mode for upstreamRSCI */
		ReceiverParam.Lock();
		ReceiverParam.eAcquiState = AS_WITH_SIGNAL;
		ReceiverParam.Unlock();

		SetInTrackingMode();
	}
}

void
CDRMReceiver::SetInTrackingMode()
{
	/* We do this with the flag "eReceiverState" to ensure that the following
	   routines are only called once when the tracking is actually started */
	if (eReceiverState == RS_ACQUISITION)
	{
		/* In case the acquisition estimation is still in progress, stop it now
		   to avoid a false estimation which could destroy synchronization */
		TimeSync.StopRMDetAcqu();

		/* Acquisition is done, deactivate it now and start tracking */
		ChannelEstimation.GetTimeWiener()->StartTracking();

		/* Reset acquisition for frame synchronization */
		SyncUsingPil.StopAcquisition();
		SyncUsingPil.StartTrackPil();

		/* Set receiver flag to tracking */
		eReceiverState = RS_TRACKING;
	}
}

void
CDRMReceiver::SetInTrackingModeDelayed()
{
	/* The timing tracking must be enabled delayed because it must wait until
	   the channel estimation has initialized its estimation */
	TimeSync.StopTimingAcqu();
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();
}

void
CDRMReceiver::InitsForAllModules()
{
	if (downstreamRSCI.GetOutEnabled())
	{
		pReceiverParam->bMeasureDelay = TRUE;
		pReceiverParam->bMeasureDoppler = TRUE;
		pReceiverParam->bMeasureInterference = TRUE;
		pReceiverParam->bMeasurePSD = TRUE;
	}
	else
	{
		pReceiverParam->bMeasureDelay = FALSE;
		pReceiverParam->bMeasureDoppler = FALSE;
		pReceiverParam->bMeasureInterference = FALSE;
		pReceiverParam->bMeasurePSD = FALSE;
	}

	/* Set init flags */
	SplitFAC.SetInitFlag();
	SplitSDC.SetInitFlag();
	for (size_t i = 0; i < MSCDecBuf.size(); i++)
	{
		SplitMSC[i].SetStream(i);
		SplitMSC[i].SetInitFlag();
		MSCDecBuf[i].Clear();
		MSCUseBuf[i].Clear();
		MSCSendBuf[i].Clear();
	}
	ReceiveData.SetSoundInterface(pSoundInInterface);
	ReceiveData.SetInitFlag();
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	TimeSync.SetInitFlag();
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	FACMLCDecoder.SetInitFlag();
	UtilizeFACData.SetInitFlag();
	SDCMLCDecoder.SetInitFlag();
	UtilizeSDCData.SetInitFlag();
	SymbDeinterleaver.SetInitFlag();
	MSCMLCDecoder.SetInitFlag();
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	AudioSourceDecoder.SetInitFlag();
	DataDecoder.SetInitFlag();
	WriteData.SetInitFlag();

	Split.SetInitFlag();
	SplitAudio.SetInitFlag();
	AudioSourceEncoder.SetInitFlag();
	AMDemodulation.SetInitFlag();
	OnboardDecoder.SetSoundInterface(pSoundInInterface);
	OnboardDecoder.SetInitFlag();

	SplitForIQRecord.SetInitFlag();
	WriteIQFile.SetInitFlag();
	/* AMSS */
	AMSSPhaseDemod.SetInitFlag();
	AMSSExtractBits.SetInitFlag();
	AMSSDecode.SetInitFlag();

	upstreamRSCI.SetInitFlag();
	//downstreamRSCI.SetInitFlag();

	/* Clear all buffers (this is especially important for the "AudSoDecBuf"
	   buffer since AM mode and DRM mode use the same buffer. When init is
	   called or modes are switched, the buffer could have some data left which
	   lead to an overrun) */
	RecDataBuf.Clear();
	AMDataBuf.Clear();

	DemodDataBuf.Clear();
	IQRecordDataBuf.Clear();

	AMSSDataBuf.Clear();
	AMSSPhaseBuf.Clear();
	AMSSResPhaseBuf.Clear();
	AMSSBitsBuf.Clear();

	InpResBuf.Clear();
	FreqSyncAcqBuf.Clear();
	TimeSyncBuf.Clear();
	OFDMDemodBuf.Clear();
	SyncUsingPilBuf.Clear();
	ChanEstBuf.Clear();
	MSCCarDemapBuf.Clear();
	FACCarDemapBuf.Clear();
	SDCCarDemapBuf.Clear();
	DeintlBuf.Clear();
	FACDecBuf.Clear();
	SDCDecBuf.Clear();
	MSCMLCDecBuf.Clear();
	RSIPacketBuf.Clear();
	AudSoDecBuf.Clear();
	AMAudioBuf.Clear();
	AMSoEncBuf.Clear();
}

/* -----------------------------------------------------------------------------
   Initialization routines for the modules. We have to look into the modules
   and decide on which parameters the modules depend on */
void
CDRMReceiver::InitsForWaveMode()
{
	/* Reset averaging of the parameter histories (needed, e.g., because the
	   number of OFDM symbols per DRM frame might have changed) */
	PlotManager.Init();

	/* After a new robustness mode was detected, give the time synchronization
	   a bit more time for its job */
	iAcquDetecCnt = 0;

	/* Set init flags */
	ReceiveData.SetSoundInterface(pSoundInInterface);
	ReceiveData.SetInitFlag();
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	Split.SetInitFlag();
	AMDemodulation.SetInitFlag();
	ReceiveData.SetSoundInterface(pSoundInInterface);
	OnboardDecoder.SetInitFlag();
	AudioSourceEncoder.SetInitFlag();

	SplitForIQRecord.SetInitFlag();
	WriteIQFile.SetInitFlag();

	AMSSPhaseDemod.SetInitFlag();
	AMSSExtractBits.SetInitFlag();
	AMSSDecode.SetInitFlag();

	TimeSync.SetInitFlag();
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	SymbDeinterleaver.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
	SDCMLCDecoder.SetInitFlag();	// Because of "iNumSDCCellsPerSFrame"
}

void
CDRMReceiver::InitsForSpectrumOccup()
{
	/* Set init flags */
	FreqSyncAcq.SetInitFlag();	// Because of bandpass filter
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	SymbDeinterleaver.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag();	// Because of "iNumUsefMSCCellsPerFrame"
	SDCMLCDecoder.SetInitFlag();	// Because of "iNumSDCCellsPerSFrame"
}

/* SDC ---------------------------------------------------------------------- */
void
CDRMReceiver::InitsForSDCCodSche()
{
	/* Set init flags */
	SDCMLCDecoder.SetInitFlag();

#ifdef USE_DD_WIENER_FILT_TIME
	ChannelEstimation.SetInitFlag();
#endif
}

void
CDRMReceiver::InitsForNoDecBitsSDC()
{
	/* Set init flag */
	SplitSDC.SetInitFlag();
	UtilizeSDCData.SetInitFlag();
}

/* MSC ---------------------------------------------------------------------- */
void
CDRMReceiver::InitsForInterlDepth()
{
	/* Can be absolutely handled seperately */
	SymbDeinterleaver.SetInitFlag();
}

void
CDRMReceiver::InitsForMSCCodSche()
{
	/* Set init flags */
	MSCMLCDecoder.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();	// Not sure if really needed, look at code! TODO

#ifdef USE_DD_WIENER_FILT_TIME
	ChannelEstimation.SetInitFlag();
#endif
}

void
CDRMReceiver::InitsForMSC()
{
	/* Set init flags */
	MSCMLCDecoder.SetInitFlag();

	InitsForMSCDemux();
}

void
CDRMReceiver::InitsForMSCDemux()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	for (size_t i = 0; i < MSCDecBuf.size(); i++)
	{
		SplitMSC[i].SetStream(i);
		SplitMSC[i].SetInitFlag();
	}
	InitsForAudParam();
	InitsForDataParam();

	/* Reset value used for the history because if an audio service was selected
	   but then only a data service is selected, the value would remain with the
	   last state */
	PlotManager.SetCurrentCDAud(0);
}

void
CDRMReceiver::InitsForAudParam()
{
	for (size_t i = 0; i < MSCDecBuf.size(); i++)
	{
		MSCDecBuf[i].Clear();
		MSCUseBuf[i].Clear();
		MSCSendBuf[i].Clear();
	}

	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int a = pReceiverParam->GetCurSelAudioService();
	iAudioStreamID = pReceiverParam->Service[a].iAudioStream;
	if(iAudioStreamID != STREAM_ID_NOT_USED)
	{
		int audiobits = pReceiverParam->GetStreamLen(iAudioStreamID) * SIZEOF__BYTE;
		pReceiverParam->SetNumAudioDecoderBits(audiobits);
	}
	AudioSourceDecoder.SetInitFlag();
}

void
CDRMReceiver::InitsForDataParam()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int d = pReceiverParam->GetCurSelDataService();
	iDataStreamID = pReceiverParam->Service[d].iDataStream;
	int databits = pReceiverParam-> GetStreamLen(iDataStreamID) * SIZEOF__BYTE;
	pReceiverParam->SetNumDataDecoderBits(databits);
	DataDecoder.SetInitFlag();
}


_BOOLEAN CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
	if (iFreqkHz == iNewFreqkHz)
		return TRUE;
	iFreqkHz = iNewFreqkHz;

	pReceiverParam->Lock();
	pReceiverParam->SetFrequency(iNewFreqkHz);
	/* clear out AMSS data and re-initialise AMSS acquisition */
	if(pReceiverParam->eReceiverMode == RM_AM)
		pReceiverParam->ResetServicesStreams();
	pReceiverParam->Unlock();

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetFrequency(iNewFreqkHz);
		return TRUE;
	}
	else
	{
		/* tell the RSCI and IQ file writer that freq has changed in case it needs to start a new file */
		if (downstreamRSCI.GetOutEnabled() == TRUE)
			downstreamRSCI.NewFrequency(*pReceiverParam);

		WriteIQFile.NewFrequency(*pReceiverParam);

#ifdef HAVE_LIBHAMLIB
		if(pHamlib)
			return pHamlib->SetFrequency(iNewFreqkHz);
#endif
		return TRUE;
	}
}

void
CDRMReceiver::SetIQRecording(_BOOLEAN bON)
{
	if(bON)
		WriteIQFile.StartRecording(*pReceiverParam);
	else
		WriteIQFile.StopRecording();
}

void
CDRMReceiver::SetRSIRecording(_BOOLEAN bOn, const char cProfile)
{
	downstreamRSCI.SetRSIRecording(*pReceiverParam, bOn, cProfile);
}

void
CDRMReceiver::SetReadPCMFromFile(const string strNFN)
{
	delete pSoundInInterface;
	CAudioFileIn *pf = new CAudioFileIn;
	pf->SetFileName(strNFN);
	pSoundInInterface = pf;
	pcmInput = File;

	_BOOLEAN bIsIQ = FALSE;
	string ext;
	size_t p = strNFN.rfind('.');
	if (p != string::npos)
		ext = strNFN.substr(p + 1);
	if (ext.substr(0, 2) == "iq")
		bIsIQ = TRUE;
	if (ext.substr(0, 2) == "IQ")
		bIsIQ = TRUE;
	if (bIsIQ)
		ReceiveData.SetInChanSel(CS_IQ_POS_ZERO);
	else
		ReceiveData.SetInChanSel(CS_MIX_CHAN);
	ReceiveData.SetSoundInterface(pSoundInInterface);
	ReceiveData.SetInitFlag();
	OnboardDecoder.SetSoundInterface(pSoundInInterface);
	OnboardDecoder.SetInitFlag();
}

void CDRMReceiver::UpdateSoundIn()
{
	if(pcmInput == File)
		return;
#ifdef HAVE_LIBHAMLIB
# ifdef __linux__
	if(pHamlib)
	{
		CRigCaps caps;
		pHamlib->GetRigCaps(caps);
		if(caps.bHamlibDoesAudio)
		{
			if(pcmInput != Shm)
			{
				delete pSoundInInterface;
    			CShmSoundIn* ShmSoundIn = new CShmSoundIn;
				string shm_path = pHamlib->GetConfig("if_path");
				ShmSoundIn->SetShmPath(shm_path);
				ShmSoundIn->SetName(caps.hamlib_caps.model_name);
				ShmSoundIn->SetShmChannels(1);
				ShmSoundIn->SetWantedChannels(2);
				pSoundInInterface = ShmSoundIn;
				pcmInput = Shm;
				pSoundInInterface->SetDev(0);
			}
		}
		else
		{
			if(pcmInput == Shm)
			{
				delete pSoundInInterface;
				pSoundInInterface = new CSoundInNull;
				pcmInput=Dummy;
			}
		}
	}
# endif
#endif
	if(pcmInput==Dummy)
	{
		int iDev = pSoundInInterface->GetDev();
		delete pSoundInInterface;
		pSoundInInterface = new CSoundIn;
		pcmInput = SoundCard;
		pSoundInInterface->SetDev(iDev);
	}
	ReceiveData.SetSoundInterface(pSoundInInterface);
	ReceiveData.SetInitFlag();
	OnboardDecoder.SetSoundInterface(pSoundInInterface);
	OnboardDecoder.SetInitFlag();
}

void CDRMReceiver::SetHamlib(CHamlib* p)
{
	pHamlib = p;
}

void CDRMReceiver::SetRigModel(int iID)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
	{
		rig_model_t	id = pHamlib->GetHamlibModelID();
		if(id!=iID)
		{
			pHamlib->SetHamlibModelID(iID);
			UpdateSoundIn();
		}
	}
#endif
}

int CDRMReceiver::GetRigModel()
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		return pHamlib->GetHamlibModelID();
#endif
	return 0;
}

void CDRMReceiver::GetRigList(map<rig_model_t,CRigCaps>& rigs)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->GetRigList(rigs);
#endif
}

void CDRMReceiver::GetRigCaps(CRigCaps& caps)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->GetRigCaps(caps);
#endif
}

void CDRMReceiver::GetComPortList(map<string,string>& ports)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->GetPortList(ports);
#endif
}

string CDRMReceiver::GetRigComPort()
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		return pHamlib->GetComPort();
#endif
	return "";
}

void CDRMReceiver::SetRigFreqOffset(int iVal)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->SetFreqOffset(iVal);
#endif
}

void CDRMReceiver::SetRigComPort(const string& s)
{
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
		pHamlib->SetComPort(s);
#endif
}

_BOOLEAN
CDRMReceiver::GetSignalStrength(_REAL& rSigStr)
{
	CParameter& Parameters = *pReceiverParam;
	Parameters.Lock();
	if(Parameters.SigStrstat.isValid())
	{
		rSigStr = Parameters.SigStrstat.getCurrent();
		Parameters.Unlock();
		return TRUE;
	}
	Parameters.Unlock();
	return FALSE;
}

/* TEST store information about alternative frequency transmitted in SDC */
void
CDRMReceiver::saveSDCtoFile()
{
	CParameter & ReceiverParam = *pReceiverParam;
	static FILE *pFile = NULL;

	if(pFile == NULL)
		pFile = fopen("test/altfreq.dat", "w");

	ReceiverParam.Lock();
	size_t inum = ReceiverParam.AltFreqSign.vecMultiplexes.size();
	for (size_t z = 0; z < inum; z++)
	{
		fprintf(pFile, "sync:%d sr:", ReceiverParam.AltFreqSign.vecMultiplexes[z].bIsSyncMultplx);

		for (int k = 0; k < 4; k++)
				fprintf(pFile, "%d", ReceiverParam.AltFreqSign.vecMultiplexes[z].  veciServRestrict[k]);
		fprintf(pFile, " fr:");

		for (size_t kk = 0; kk < ReceiverParam.AltFreqSign.vecMultiplexes[z].veciFrequencies.size(); kk++)
			fprintf(pFile, "%d ", ReceiverParam.AltFreqSign.vecMultiplexes[z].  veciFrequencies[kk]);

		fprintf(pFile, " rID:%d sID:%d   /   ",
					ReceiverParam.AltFreqSign.vecMultiplexes[z].iRegionID,
					ReceiverParam.AltFreqSign.vecMultiplexes[z].iScheduleID);
	}
	ReceiverParam.Unlock();
	fprintf(pFile, "\n");
	fflush(pFile);
}

void
CDRMReceiver::LoadSettings(CSettings& s)
{
	CParameter & Parameters = *pReceiverParam;
	size_t p;

	int i;
	/* Serial Number */
	string sValue = s.Get("Receiver", "serialnumber");
	if (sValue != "")
	{
		// Pad to a minimum of 6 characters
		while (sValue.length() < 6)
			sValue += "_";
		Parameters.sSerialNumber = sValue;
	}

	Parameters.GenerateReceiverID();

	/* Data files directory */
	string sDataFilesDirectory = s.Get(
	   "Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
	// remove trailing slash if there
	p = sDataFilesDirectory.find_last_not_of("/\\");
	if(p != string::npos)
		sDataFilesDirectory.erase(p+1);

	Parameters.sDataFilesDirectory = sDataFilesDirectory;
	s.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);

	/* Sync */
	SetFreqInt(CChannelEstimation::ETypeIntFreq(s.Get("Receiver", "frequencyinterpolation", int(CChannelEstimation::FWIENER))));
	SetTimeInt(CChannelEstimation::ETypeIntTime(s.Get("Receiver", "timeinterpolation", int(CChannelEstimation::TWIENER))));
	SetTiSyncTracType(CTimeSyncTrack::ETypeTiSyncTrac(s.Get("Receiver", "tracking", 0)));

	/* Receiver ------------------------------------------------------------- */

	/* Sound In device */
	pSoundInInterface->SetDev(s.Get("Receiver", "snddevin", 0));

	/* Sound Out device */
	pSoundOutInterface->SetDev(s.Get("Receiver", "snddevout", 0));

	string strInFile;
	string str;
	string strInFileExt;
	int n;

	/* input from file */
	str = s.Get("command", "fileio");
	p = str.rfind('.');
	if (p != string::npos)
		strInFileExt = str.substr(p + 1);

	if (strInFileExt.substr(0,2) == "RS" || strInFileExt.substr(0,2) == "rs" || strInFileExt == "pcap")
	{
		s.Put("command", "rsiin", str);
	}
	else
	{
		strInFile = str;
	}

	if(strInFile != "")
		SetReadPCMFromFile(strInFile);
	else
		UpdateSoundIn();

	/* Flip spectrum flag */
	ReceiveData.SetFlippedSpectrum(s.Get("Receiver", "flipspectrum", FALSE));

	n = s.Get("command", "inchansel", -1);
	switch (n)
	{
	case 0:
		ReceiveData.SetInChanSel(CS_LEFT_CHAN);
		break;

	case 1:
		ReceiveData.SetInChanSel(CS_RIGHT_CHAN);
		break;

	case 2:
		ReceiveData.SetInChanSel(CS_MIX_CHAN);
		break;

	case 3:
		ReceiveData.SetInChanSel(CS_IQ_POS);
		break;

	case 4:
		ReceiveData.SetInChanSel(CS_IQ_NEG);
		break;

	case 5:
		ReceiveData.SetInChanSel(CS_IQ_POS_ZERO);
		break;

	case 6:
		ReceiveData.SetInChanSel(CS_IQ_NEG_ZERO);
		break;
	default:
		break;
	}
	n = s.Get("command", "outchansel", -1);
	switch (n)
	{
	case 0:
		WriteData.SetOutChanSel(CWriteData::CS_BOTH_BOTH);
		break;

	case 1:
		WriteData.SetOutChanSel(CWriteData::CS_LEFT_LEFT);
		break;

	case 2:
		WriteData.SetOutChanSel(CWriteData::CS_RIGHT_RIGHT);
		break;

	case 3:
		WriteData.SetOutChanSel(CWriteData::CS_LEFT_MIX);
		break;

	case 4:
		WriteData.SetOutChanSel(CWriteData::CS_RIGHT_MIX);
		break;
	default:
		break;
	}

	/* demodulation */
	Parameters.eDemodType = EDemodType(s.Get("Demodulator", "modulation", DT_AM));

	/* AM Parameters */
	Parameters.bUseHWDemod = s.Get("Demodulator", "usehw", 0);

	/* AGC */
	AMDemodulation.SetAGCType((CAGC::EType)s.Get("AM Demodulation", "agc", 0));

	/* noise reduction */
	AMDemodulation.SetNoiRedType((CAMDemodulation::ENoiRedType)s.Get("AM Demodulation", "noisered", 0));

	/* pll enabled/disabled */
	AMDemodulation.EnablePLL(s.Get("AM Demodulation", "enablepll", 0));

	/* auto frequency acquisition */
	AMDemodulation.EnableAutoFreqAcq(s.Get("AM Demodulation", "autofreqacq", 0));

	Parameters.iBw[DT_AM] = s.Get("AM Demodulation", "filterbwam", 10000);
	Parameters.iBw[DT_LSB] = s.Get("AM Demodulation", "filterbwlsb", 5000);
	Parameters.iBw[DT_USB] = s.Get("AM Demodulation", "filterbwusb", 5000);
	Parameters.iBw[DT_CW] = s.Get("AM Demodulation", "filterbwcw", 150);

	/* FM Parameters */
	Parameters.iBw[DT_NBFM] = s.Get("FM Demodulation", "nbfilterbw", 6000);
	Parameters.iBw[DT_WBFM] = s.Get("FM Demodulation", "wbfilterbw", 80000);

	/* Load user's saved filter bandwidth for the demodulation type. */
	AMDemodulation.SetDemodTypeAndBPF(Parameters.eDemodType, Parameters.iBw[Parameters.eDemodType]);

	/* upstream RSCI */
	str = s.Get("command", "rsiin");
	if(str != "")
		upstreamRSCI.SetOrigin(str); // its a port

	str = s.Get("command", "rciout");
	if(str != "")
		upstreamRSCI.SetDestination(str);

	/* downstream RSCI */
	for(i = 0; i<MAX_NUM_RSI_SUBSCRIBERS; i++)
	{
		stringstream ss;
		ss << "rsiout" << i;
		str = s.Get("command", ss.str());
		if(str != "")
		{
			ss.str("");
			ss << "rsioutprofile" << i;
			string profile = s.Get("command", ss.str(), string("A"));

			// Check whether the profile has a subsampling ratio (e.g. --rsioutprofile A20)
			int iSubsamplingFactor = 1;
			if (profile.length() > 1)
			{
				iSubsamplingFactor = atoi(profile.substr(1).c_str());
			}


			ss.str("");
			ss << "rciin" << i;
			string origin = s.Get("command", ss.str());
			downstreamRSCI.AddSubscriber(str, origin, profile[0], iSubsamplingFactor);
		}
	}

	for (i=1; i<=MAX_NUM_RSI_PRESETS; i++)
	{
		// define presets in same format as --rsioutprofile
		stringstream ss;
		ss << "rsioutpreset" << i;
		str = s.Get("RSCI", ss.str());
		if(str != "")
		{
			// Check whether the preset has a subsampling ratio (e.g. A20)
			int iSubsamplingFactor = 1;
			if (str.length() > 1)
			{
				iSubsamplingFactor = atoi(str.substr(1).c_str());
			}
			downstreamRSCI.DefineRSIPreset(i, str[0], iSubsamplingFactor);
		}
	}
	/* RSCI File Recording */
	str = s.Get("command", "rsirecordprofile");
	string s2 = s.Get("command", "rsirecordtype");
	if(str != "" || s2 != "")
		downstreamRSCI.SetRSIRecording(*pReceiverParam, TRUE, str[0], s2);

	/* IQ File Recording */
	if(s.Get("command", "recordiq", false))
		WriteIQFile.StartRecording(*pReceiverParam);

	/* Mute audio flag */
	WriteData.MuteAudio(s.Get("Receiver", "muteaudio", FALSE));

	/* Output to File */
	str = s.Get("command", "writewav");
	if(str != "")
		WriteData.StartWriteWaveFile(str);

	/* Reverberation flag */
	AudioSourceDecoder.SetReverbEffect(s.Get("Receiver", "reverb", TRUE));

	/* Bandpass filter flag */
	FreqSyncAcq.SetRecFilter(s.Get("Receiver", "filter", FALSE));

	/* Set parameters for frequency acquisition search window if needed */
	 _REAL rFreqAcSeWinSize = s.Get("command", "fracwinsize", _REAL(SOUNDCRD_SAMPLE_RATE / 2));
	 _REAL rFreqAcSeWinCenter = s.Get("command", "fracwincent", _REAL(SOUNDCRD_SAMPLE_RATE / 4));
	/* Set new parameters */
	FreqSyncAcq.SetSearchWindow(rFreqAcSeWinCenter, rFreqAcSeWinSize);

	/* Modified metrics flag */
	ChannelEstimation.SetIntCons(s.Get("Receiver", "modmetric", FALSE));

	/* Number of iterations for MLC setting */
	MSCMLCDecoder.SetNumIterations(s.Get("Receiver", "mlciter", 0));

	/* Activate/Deactivate EPG decoding */
	DataDecoder.SetDecodeEPG(s.Get("EPG", "decodeepg", TRUE));

	string strMode = s.Get("GUI",	"mode");

	if (strMode == "DRMRX")
		SetReceiverMode(RM_DRM);
	else if (strMode == "AMRX")
		SetReceiverMode(RM_AM);
	//else - leave it as initialised (ie. DRM)

	/* Front-end - combine into Hamlib? */
	CFrontEndParameters& FrontEndParameters = Parameters.FrontEndParameters;

	FrontEndParameters.eSMeterCorrectionType =
		CFrontEndParameters::ESMeterCorrectionType(s.Get("FrontEnd", "smetercorrectiontype", 0));

	FrontEndParameters.rSMeterBandwidth = s.Get("FrontEnd", "smeterbandwidth", 0.0);

	FrontEndParameters.rDefaultMeasurementBandwidth = s.Get("FrontEnd", "defaultmeasurementbandwidth", 0);

	FrontEndParameters.bAutoMeasurementBandwidth = s.Get("FrontEnd", "automeasurementbandwidth", TRUE);

	FrontEndParameters.rCalFactorDRM = s.Get("FrontEnd", "calfactordrm", 0.0);

	FrontEndParameters.rCalFactorAM = s.Get("FrontEnd", "calfactoram", 0.0);

	FrontEndParameters.rIFCentreFreq = s.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);

	/* Wanted RF Frequency file */
	SetFrequency(s.Get("Receiver", "frequency", 0));
}

void
CDRMReceiver::SaveSettings(CSettings& s)
{
	CParameter & Parameters = *pReceiverParam;

	if(eReceiverMode == RM_AM)
		s.Put("GUI", "mode", "AMRX");
	else
		s.Put("GUI", "mode", "DRMRX");

	/* Receiver ------------------------------------------------------------- */

	/* Flip spectrum flag */
	s.Put("Receiver", "flipspectrum", ReceiveData.GetFlippedSpectrum());

	/* Mute audio flag */
	s.Put("Receiver", "muteaudio", WriteData.GetMuteAudio());

	/* Reverberation */
	s.Put("Receiver", "reverb", AudioSourceDecoder.GetReverbEffect());

	/* Bandpass filter flag */
	s.Put("Receiver", "filter", FreqSyncAcq.GetRecFilter());

	/* Modified metrics flag */
	s.Put("Receiver", "modmetric", ChannelEstimation.GetIntCons());

	/* Sync */
	s.Put("Receiver", "frequencyinterpolation", int(GetFreqInt()));
	s.Put("Receiver", "timeinterpolation", int(GetTimeInt()));
	s.Put("Receiver", "tracking", int(GetTiSyncTracType()));

	/* Sound In device */
	s.Put("Receiver", "snddevin", pSoundInInterface->GetDev());

	/* Sound Out device */
	s.Put("Receiver", "snddevout", pSoundOutInterface->GetDev());

	/* Number of iterations for MLC setting */
	s.Put("Receiver", "mlciter", MSCMLCDecoder.GetInitNumIterations());

	/* Tuned Frequency */
	s.Put("Receiver", "frequency", iFreqkHz);

	/* Active/Deactivate EPG decoding */
	s.Put("EPG", "decodeepg", DataDecoder.GetDecodeEPG());


	/* demodulation */
	s.Put("Demodulator", "modulation", Parameters.eDemodType);
	s.Put("Demodulator", "usehw", Parameters.bUseHWDemod);

	/* AM Parameters */

	/* AGC */
	s.Put("AM Demodulation", "agc", AMDemodulation.GetAGCType());

	/* noise reduction */
	s.Put("AM Demodulation", "noisered", AMDemodulation.GetNoiRedType());

	/* pll enabled/disabled */
	s.Put("AM Demodulation", "enablepll", AMDemodulation.PLLEnabled());

	/* auto frequency acquisition */
	s.Put("AM Demodulation", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

	s.Put("AM Demodulation", "filterbwam", Parameters.iBw[DT_AM]);
	s.Put("AM Demodulation", "filterbwlsb", Parameters.iBw[DT_LSB]);
	s.Put("AM Demodulation", "filterbwusb", Parameters.iBw[DT_USB]);
	s.Put("AM Demodulation", "filterbwcw", Parameters.iBw[DT_CW]);

	/* FM Parameters */

	s.Put("FM Demodulation", "nbfilterbw", Parameters.iBw[DT_NBFM]);
	s.Put("FM Demodulation", "wbfilterbw", Parameters.iBw[DT_WBFM]);

	/* Front-end - combine into Hamlib? */
	s.Put("FrontEnd", "smetercorrectiontype", int(Parameters.FrontEndParameters.eSMeterCorrectionType));
	s.Put("FrontEnd", "smeterbandwidth", int(Parameters.FrontEndParameters.rSMeterBandwidth));
	s.Put("FrontEnd", "defaultmeasurementbandwidth", int(Parameters.FrontEndParameters.rDefaultMeasurementBandwidth));
	s.Put("FrontEnd", "automeasurementbandwidth", Parameters.FrontEndParameters.bAutoMeasurementBandwidth);
	s.Put("FrontEnd", "calfactordrm", int(Parameters.FrontEndParameters.rCalFactorDRM));
	s.Put("FrontEnd", "calfactoram", int(Parameters.FrontEndParameters.rCalFactorAM));
	s.Put("FrontEnd", "ifcentrefrequency", int(Parameters.FrontEndParameters.rIFCentreFreq));

	/* Serial Number */
	s.Put("Receiver", "serialnumber", Parameters.sSerialNumber);
	s.Put("Receiver", "datafilesdirectory", Parameters.sDataFilesDirectory);
}
