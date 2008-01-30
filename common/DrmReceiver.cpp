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
#include "sound/soundfile.h"

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
DRMParameters(), AMParameters(), Parameters(DRMParameters),
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
demodulation(inSoftware)
#ifdef HAVE_LIBHAMLIB
,rigmodelrequest(), bRigUpdateNeeded(FALSE)
#endif
{
	AMParameters.SetReceiver(this);
	DRMParameters.SetReceiver(this);
	downstreamRSCI.SetReceiver(this);
	PlotManager.SetReceiver(this);
}

CDRMReceiver::~CDRMReceiver()
{
	delete pSoundInInterface;
	delete pSoundOutInterface;
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

void CDRMReceiver::SetUseAnalogHWDemod(_BOOLEAN bUse)
{
	demodulation = bUse?onBoard:inSoftware;
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

_BOOLEAN CDRMReceiver::GetUseAnalogHWDemod()
{
	return (demodulation==onBoard)?true:false;
}

void
CDRMReceiver::SetAnalogDemodType(EDemodType eNew)
{
	AMDemodulation.SetDemodType(eNew);
#ifdef HAVE_LIBHAMLIB
	bRigUpdateNeeded = TRUE;
#endif
}

int
CDRMReceiver::GetAnalogFilterBWHz()
{
	return AMDemodulation.GetFilterBWHz();
}

void
CDRMReceiver::SetAnalogFilterBWHz(int iNew)
{
	AMDemodulation.SetFilterBWHz(iNew);
}

void
CDRMReceiver::GetAnalogBWParameters(CReal& rCenterFreq, CReal& rBW)
{
	AMDemodulation.GetBWParameters(rCenterFreq, rBW);
}

void
CDRMReceiver::SetAnalogDemodAcq(_REAL rNewNorCen)
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
CDRMReceiver::EnableAnalogAutoFreqAcq(const _BOOLEAN bNewEn)
{
	AMDemodulation.EnableAutoFreqAcq(bNewEn);
}

_BOOLEAN
CDRMReceiver::AnalogAutoFreqAcqEnabled()
{
	return AMDemodulation.AutoFreqAcqEnabled();
}


void
CDRMReceiver::EnableAnalogPLL(const _BOOLEAN bNewEn)
{
	AMDemodulation.EnablePLL(bNewEn);
}

_BOOLEAN
CDRMReceiver::AnalogPLLEnabled()
{
	return AMDemodulation.PLLEnabled();
}

_BOOLEAN
CDRMReceiver::GetAnalogPLLPhase(CReal& rPhaseOut)
{
	return AMDemodulation.GetPLLPhase(rPhaseOut);
}


void
CDRMReceiver::SetAnalogAGCType(const CAGC::EType eNewType)
{
	AMDemodulation.SetAGCType(eNewType);
}

CAGC::EType
CDRMReceiver::GetAnalogAGCType()
{
	return AMDemodulation.GetAGCType();
}

void
CDRMReceiver::SetAnalogNoiseReductionType(const CAMDemodulation::ENoiRedType eNewType)
{
	AMDemodulation.SetNoiRedType(eNewType);
}

CAMDemodulation::ENoiRedType
CDRMReceiver::GetAnalogNoiseReductionType()
{
	return AMDemodulation.GetNoiRedType();
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
#ifdef HAVE_LIBHAMLIB
	if(rigmodelrequest.is_pending())
		SetRigModelWT(rigmodelrequest.value());
	if (bRigUpdateNeeded)
	   UpdateRigSettings();

     bRigUpdateNeeded=FALSE;
#endif

	if(pcmInput==Dummy)
		UpdateSoundIn();

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
			upstreamRSCI.ReadData(Parameters, RSIPacketBuf);
			if (RSIPacketBuf.GetFillLevel() > 0)
			{
				time_keeper = time(NULL);
				DecodeRSIMDI.ProcessData(Parameters, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
				PlotManager.UpdateParamHistoriesRSIIn();
				bFrameToSend = TRUE;
			}
			else
			{
				time_t now = time(NULL);
				if ((now - time_keeper) > 2)
				{
					Parameters.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
					Parameters.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
				}
			}
		}
	}
	else
	{
		if(eReceiverMode==RM_AM && demodulation==onBoard)
		{
			OnboardDecoder.ReadData(Parameters, AMAudioBuf);
		}
		else
		{
			ReceiveData.ReadData(Parameters, RecDataBuf);

			// Split samples, one output to the demodulation, another for IQ recording
			if (SplitForIQRecord.ProcessData(Parameters, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
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
		SplitFAC.ProcessData(Parameters, FACDecBuf, FACUseBuf, FACSendBuf);

		/* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
		if (SDCDecBuf.GetFillLevel() == Parameters.iNumSDCBitsPerSFrame)
		{
			SplitSDC.ProcessData(Parameters, SDCDecBuf, SDCUseBuf, SDCSendBuf);
		}

		for (i = 0; i < MSCDecBuf.size(); i++)
		{
			SplitMSC[i].ProcessData(Parameters, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
		}
		break;
	case RM_AM:
		SplitAudio.ProcessData(Parameters, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
		break;
	case RM_NONE:
		break;
	}

	/* decoding */
	while (bEnoughData && Parameters.bRunThread)
	{
		/* Init flag */
		bEnoughData = FALSE;

		// Write output I/Q file
		if (WriteIQFile.WriteData(Parameters, IQRecordDataBuf))
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
			if (Parameters.eAcquiState == AS_NO_SIGNAL)
			{
				/* we will get one of these between each FAC block, and occasionally we */
				/* might get two, so don't start generating free-wheeling RSCI until we've. */
				/* had three in a row */
				if (FreqSyncAcq.GetUnlockedFrameBoundary())
				{
					if (iUnlockedCount < MAX_UNLOCKED_COUNT)
						iUnlockedCount++;
					else
						downstreamRSCI.SendUnlockedFrame(Parameters);
				}
			}
			else if (bFrameToSend)
			{
				downstreamRSCI.SendLockedFrame(Parameters, FACSendBuf, SDCSendBuf, MSCSendBuf);
				iUnlockedCount = 0;
				bFrameToSend = FALSE;
			}
			break;
		case RM_AM:
			/* Encode audio for RSI output */
			if (AudioSourceEncoder.ProcessData(Parameters, AMSoEncBuf, MSCSendBuf[0]))
				bFrameToSend = TRUE;

			if (bFrameToSend)
				downstreamRSCI.SendAMFrame(Parameters, MSCSendBuf[0]);
			break;
		case RM_NONE:
			break;
		}
	}

	/* Play and/or save the audio */
	if (iAudioStreamID != STREAM_ID_NOT_USED || eReceiverMode == RM_AM)
	{
		if (WriteData.WriteData(Parameters, AudSoDecBuf))
		{
			bEnoughData = TRUE;
		}
	}
}

void
CDRMReceiver::DemodulateDRM(_BOOLEAN& bEnoughData)
{
	/* Resample input DRM-stream -------------------------------- */
	if (InputResample.ProcessData(Parameters, DemodDataBuf, InpResBuf))
	{
		bEnoughData = TRUE;
	}

	/* Frequency synchronization acquisition -------------------- */
	if (FreqSyncAcq.ProcessData(Parameters, InpResBuf, FreqSyncAcqBuf))
	{
		bEnoughData = TRUE;
	}

	/* Time synchronization ------------------------------------- */
	if (TimeSync.ProcessData(Parameters, FreqSyncAcqBuf, TimeSyncBuf))
	{
		bEnoughData = TRUE;
		/* Use count of OFDM-symbols for detecting
		 * aquisition state for acquisition detection
		 * only if no signal was decoded before */
		if (Parameters.eAcquiState == AS_NO_SIGNAL)
		{
			/* Increment symbol counter and check if bound is reached */
			iAcquDetecCnt++;

			if (iAcquDetecCnt > NUM_OFDMSYM_U_ACQ_WITHOUT)
				SetInStartMode();
		}
	}

	/* OFDM Demodulation ---------------------------------------- */
	if (OFDMDemodulation.
		ProcessData(Parameters, TimeSyncBuf, OFDMDemodBuf))
	{
		bEnoughData = TRUE;
	}

	/* Synchronization in the frequency domain (using pilots) --- */
	if (SyncUsingPil.
		ProcessData(Parameters, OFDMDemodBuf, SyncUsingPilBuf))
	{
		bEnoughData = TRUE;
	}

	/* Channel estimation and equalisation ---------------------- */
	if (ChannelEstimation.
		ProcessData(Parameters, SyncUsingPilBuf, ChanEstBuf))
	{
		bEnoughData = TRUE;

		/* If this module has finished, all synchronization units
		   have also finished their OFDM symbol based estimates.
		   Update synchronization parameters histories */
		PlotManager.UpdateParamHistories(eReceiverState);
	}

	/* Demapping of the MSC, FAC, SDC and pilots off the carriers */
	if (OFDMCellDemapping.ProcessData(Parameters, ChanEstBuf,
									  MSCCarDemapBuf,
									  FACCarDemapBuf, SDCCarDemapBuf))
	{
		bEnoughData = TRUE;
	}

}

void
CDRMReceiver::DecodeDRM(_BOOLEAN& bEnoughData, _BOOLEAN& bFrameToSend)
{
	/* FAC ------------------------------------------------------ */
	if (FACMLCDecoder.ProcessData(Parameters, FACCarDemapBuf, FACDecBuf))
	{
		bEnoughData = TRUE;
		bFrameToSend = TRUE;
	}

	/* SDC ------------------------------------------------------ */
	if (SDCMLCDecoder.ProcessData(Parameters, SDCCarDemapBuf, SDCDecBuf))
	{
		bEnoughData = TRUE;
	}

	/* MSC ------------------------------------------------------ */

	/* Symbol de-interleaver */
	if (SymbDeinterleaver.ProcessData(Parameters, MSCCarDemapBuf, DeintlBuf))
	{
		bEnoughData = TRUE;
	}

	/* MLC decoder */
	if (MSCMLCDecoder.ProcessData(Parameters, DeintlBuf, MSCMLCDecBuf))
	{
		bEnoughData = TRUE;
	}

	/* MSC demultiplexer (will leave FAC & SDC alone! */
	if (MSCDemultiplexer.ProcessData(Parameters, MSCMLCDecBuf, MSCDecBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::UtilizeDRM(_BOOLEAN& bEnoughData)
{
	if (UtilizeFACData.WriteData(Parameters, FACUseBuf))
	{
		bEnoughData = TRUE;

		/* Use information of FAC CRC for detecting the acquisition
		   requirement */
		DetectAcquiFAC();
#if 0
		saveSDCtoFile();
#endif
	}

	if (UtilizeSDCData.WriteData(Parameters, SDCUseBuf))
	{
		bEnoughData = TRUE;
	}

	/* Data decoding */
	if (iDataStreamID != STREAM_ID_NOT_USED)
	{
		if (DataDecoder.WriteData(Parameters, MSCUseBuf[iDataStreamID]))
			bEnoughData = TRUE;
	}

	/* Source decoding (audio) */
	if (iAudioStreamID != STREAM_ID_NOT_USED)
	{
		if (AudioSourceDecoder.ProcessData(Parameters,
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
	/* The incoming samples are split 2 ways.
	   One set is passed to the existing AM demodulator.
	   The other set is passed to the new AMSS demodulator.
	   The AMSS and AM demodulators work completely independently
	 */
	if (Split.ProcessData(Parameters, DemodDataBuf, AMDataBuf, AMSSDataBuf))
	{
		bEnoughData = TRUE;
	}

	/* AM demodulation ------------------------------------------ */
	if (AMDemodulation.ProcessData(Parameters, AMDataBuf, AMAudioBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS phase demodulation */
	if (AMSSPhaseDemod.ProcessData(Parameters, AMSSDataBuf, AMSSPhaseBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::DecodeAM(_BOOLEAN& bEnoughData)
{
	/* AMSS resampling */
	if (InputResample.ProcessData(Parameters, AMSSPhaseBuf, AMSSResPhaseBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS bit extraction */
	if (AMSSExtractBits.
		ProcessData(Parameters, AMSSResPhaseBuf, AMSSBitsBuf))
	{
		bEnoughData = TRUE;
	}

	/* AMSS data decoding */
	if (AMSSDecode.ProcessData(Parameters, AMSSBitsBuf, SDCDecBuf))
	{
		bEnoughData = TRUE;
	}
}

void
CDRMReceiver::UtilizeAM(_BOOLEAN& bEnoughData)
{
	if (UtilizeSDCData.WriteData(Parameters, SDCDecBuf))
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
		if ((Parameters.eAcquiState == AS_WITH_SIGNAL)
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
			Parameters.eAcquiState = AS_WITH_SIGNAL;

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
	Parameters.bRunThread = TRUE;

	/* Run once */
	Run();

	/* Reset flags */
	bDoInitRun = FALSE;
	Parameters.bRunThread = FALSE;
}

void
CDRMReceiver::SetReceiverMode(ERecMode eNewMode)
{
	if (eReceiverMode!=eNewMode || eNewReceiverMode != RM_NONE)
		eNewReceiverMode = eNewMode;
}

void
CDRMReceiver::InitReceiverMode()
{
	switch(eNewReceiverMode)
	{
	case RM_AM:
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
			AMParameters.bRunThread = Parameters.bRunThread;
 			AMParameters.rSigStrengthCorrection = Parameters.rSigStrengthCorrection;
 			AMParameters.bMeasurePSD = Parameters.bMeasurePSD;
			AMParameters.bMeasureInterference = Parameters.bMeasureInterference;
 			AMParameters.FrontEndParameters = Parameters.FrontEndParameters;
 			AMParameters.GPSData = Parameters.GPSData;
			AMParameters.sSerialNumber = Parameters.sSerialNumber;
			AMParameters.sReceiverID  = Parameters.sReceiverID;
			AMParameters.sDataFilesDirectory = Parameters.sDataFilesDirectory;
			AMParameters.SetFrequency(Parameters.GetFrequency());
			break;
		case RM_NONE:
			/* Start from cold in AM mode - no special action */
			break;
		}
		Parameters = AMParameters;

		/* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);

		/* Set the receive status - this affects the RSI output */
		AMParameters.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
		AMParameters.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
		AMParameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
		AMParameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
		AMParameters.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
		AMParameters.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);
		break;
	case RM_DRM:
		/* we have been in DRM mode before, and have our own parameters but
		 * we might need some state from the AM mode params
		 */
		switch(eReceiverMode)
		{
		case RM_AM:
			/* AM to DRM switch - grab some common stuff */
 			DRMParameters.rSigStrengthCorrection = Parameters.rSigStrengthCorrection;
 			DRMParameters.bMeasurePSD = Parameters.bMeasurePSD;
			DRMParameters.bMeasureInterference = Parameters.bMeasureInterference;
 			DRMParameters.FrontEndParameters = Parameters.FrontEndParameters;
 			DRMParameters.GPSData = Parameters.GPSData;
			DRMParameters.sSerialNumber = Parameters.sSerialNumber;
			DRMParameters.sReceiverID  = Parameters.sReceiverID;
			DRMParameters.sDataFilesDirectory = Parameters.sDataFilesDirectory;
			DRMParameters.SetFrequency(Parameters.GetFrequency());
			break;
		case RM_DRM:
			/* DRM to DRM switch - re-acquisition requested - no special action */
			break;
		case RM_NONE:
			/* Start from cold in DRM mode - no special action */
			break;
		}
		Parameters = DRMParameters;

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
#ifdef HAVE_LIBHAMLIB
	UpdateRigSettings();
#endif
}

#ifdef HAVE_LIBHAMLIB
void
CDRMReceiver::UpdateRigSettings()
{
	if(pHamlib)
	{
		ERigMode eNewMode;
		ERigMode eMode;
		switch(eReceiverMode)
		{
		case RM_DRM:
			eNewMode = DRM;
			break;
		case RM_AM:
			switch (AMDemodulation.GetDemodType())
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
		eMode = pHamlib->GetRigMode();
		if(eMode != eNewMode)
		{
			CRigCaps caps;
			pHamlib->SetRigMode(eNewMode);
			pHamlib->GetRigCaps(caps);
			if(caps.settings[eNewMode].eOnboardDemod==C_MUST)
				SetUseAnalogHWDemod(true);
			else
				SetUseAnalogHWDemod(false);
		}
	}
}
#endif

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
	Parameters.bRunThread = TRUE;

	do
	{
		Run();

	}
	while (Parameters.bRunThread);

	pSoundInInterface->Close();
	pSoundOutInterface->Close();
}

void
CDRMReceiver::Stop()
{
	Parameters.bRunThread = FALSE;
}

void
CDRMReceiver::SetInStartMode()
{
	iUnlockedCount = MAX_UNLOCKED_COUNT;

	Parameters.Lock();
	/* Load start parameters for all modules */

	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	Parameters.SetWaveMode(RM_ROBUSTNESS_MODE_B);
	Parameters.SetSpectrumOccup(SO_3);

	/* Set initial MLC parameters */
	Parameters.SetInterleaverDepth(CParameter::SI_LONG);
	Parameters.SetMSCCodingScheme(CS_3_SM);
	Parameters.SetSDCCodingScheme(CS_2_SM);

	/* Select the service we want to decode. Always zero, because we do not
	   know how many services are transmitted in the signal we want to
	   decode */

	/* TODO: if service 0 is not used but another service is the audio service
	 * we have a problem. We should check as soon as we have information about
	 * services if service 0 is really the audio service
	 */

	/* Set the following parameters to zero states (initial states) --------- */
	Parameters.ResetServicesStreams();
	Parameters.ResetCurSelAudDatServ();

	/* Protection levels */
	Parameters.MSCPrLe.iPartA = 0;
	Parameters.MSCPrLe.iPartB = 1;
	Parameters.MSCPrLe.iHierarch = 0;

	/* Number of audio and data services */
	Parameters.iNumAudioService = 0;
	Parameters.iNumDataService = 0;

	/* We start with FAC ID = 0 (arbitrary) */
	Parameters.iFrameIDReceiv = 0;

	/* Set synchronization parameters */
	Parameters.rResampleOffset = rInitResampleOffset;	/* Initial resample offset */
	Parameters.rFreqOffsetAcqui = (_REAL) 0.0;
	Parameters.rFreqOffsetTrack = (_REAL) 0.0;
	Parameters.iTimingOffsTrack = 0;

	Parameters.Unlock();

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

	Parameters.Lock();
	/* Set flag that no signal is currently received */
	Parameters.eAcquiState = AS_NO_SIGNAL;

	/* Set flag for receiver state */
	eReceiverState = RS_ACQUISITION;

	/* Reset counters for acquisition decision, "good signal" and delayed
	   tracking mode counter */
	iAcquRestartCnt = 0;
	iAcquDetecCnt = 0;
	iGoodSignCnt = 0;
	iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

	/* Reset GUI lights */
	Parameters.ReceiveStatus.Interface.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.TSync.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.FSync.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.FAC.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.SDC.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.Audio.SetStatus(NOT_PRESENT);
	Parameters.ReceiveStatus.MOT.SetStatus(NOT_PRESENT);

	Parameters.Unlock();

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
		Parameters.Lock();
		Parameters.eAcquiState = AS_WITH_SIGNAL;
		Parameters.Unlock();

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
		Parameters.bMeasureDelay = TRUE;
		Parameters.bMeasureDoppler = TRUE;
		Parameters.bMeasureInterference = TRUE;
		Parameters.bMeasurePSD = TRUE;
	}
	else
	{
		Parameters.bMeasureDelay = FALSE;
		Parameters.bMeasureDoppler = FALSE;
		Parameters.bMeasureInterference = FALSE;
		Parameters.bMeasurePSD = FALSE;
	}

	if (Parameters.FrontEndParameters.eSMeterCorrectionType != 
                CFrontEndParameters::S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY)
        Parameters.bMeasurePSD = TRUE;
	

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
	int a = Parameters.GetCurSelAudioService();
	iAudioStreamID = Parameters.Service[a].iAudioStream;
	if(iAudioStreamID != STREAM_ID_NOT_USED)
	{
		int audiobits = Parameters.GetStreamLen(iAudioStreamID) * SIZEOF__BYTE;
		Parameters.SetNumAudioDecoderBits(audiobits);
	}
	AudioSourceDecoder.SetInitFlag();
}

void
CDRMReceiver::InitsForDataParam()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int d = Parameters.GetCurSelDataService();
	iDataStreamID = Parameters.Service[d].iDataStream;
	int databits = Parameters. GetStreamLen(iDataStreamID) * SIZEOF__BYTE;
	Parameters.SetNumDataDecoderBits(databits);
	DataDecoder.SetInitFlag();
}


_BOOLEAN CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
	if (iFreqkHz == iNewFreqkHz)
		return TRUE;
	iFreqkHz = iNewFreqkHz;

	Parameters.Lock();
	Parameters.SetFrequency(iNewFreqkHz);
	/* clear out AMSS data and re-initialise AMSS acquisition */
	if(eReceiverMode == RM_AM)
		Parameters.ResetServicesStreams();
	Parameters.Unlock();

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetFrequency(iNewFreqkHz);
		return TRUE;
	}
	else
	{
		/* tell the RSCI and IQ file writer that freq has changed in case it needs to start a new file */
		if (downstreamRSCI.GetOutEnabled() == TRUE)
			downstreamRSCI.NewFrequency(Parameters);

		WriteIQFile.NewFrequency(Parameters);

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
		WriteIQFile.StartRecording(Parameters);
	else
		WriteIQFile.StopRecording();
}

void
CDRMReceiver::SetRSIRecording(_BOOLEAN bOn, const char cProfile)
{
	downstreamRSCI.SetRSIRecording(Parameters, bOn, cProfile);
}

void
CDRMReceiver::SetReadPCMFromFile(const string strNFN)
{
	delete pSoundInInterface;
	CSoundFileIn *pf = new CSoundFileIn;
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
#ifdef HAVE_LIBHAMLIB
	if(pHamlib)
	{
		CRigCaps caps;
		pHamlib->GetRigCaps(caps);
		if(caps.bHamlibDoesAudio)
		{
# ifdef __linux__
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
# endif
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

#ifdef HAVE_LIBHAMLIB
void CDRMReceiver::SetHamlib(CHamlib* p)
{
	pHamlib = p;
	if(pHamlib)
	{
		SetRigModel(pHamlib->GetHamlibModelID());
	}
}

void CDRMReceiver::SetRigModel(int iID)
{
	rigmodelrequest.request(iID);
}

void CDRMReceiver::SetRigModelWT(int iID)
{
	if(pHamlib)
	{
		rig_model_t	id = pHamlib->GetHamlibModelID();
		if(id!=iID)
		{
			if(pcmInput!=Dummy)
			{
				CRigCaps caps;
				pHamlib->GetRigCaps(caps);
				if(caps.bHamlibDoesAudio) // next rig might want real sound!
				{
					delete pSoundInInterface;
					pSoundInInterface = new CSoundInNull;
					pcmInput=Dummy;
				}
			}
			pHamlib->SetHamlibModelID(iID);
			SetFrequency(iFreqkHz);
		}
	}
}

int CDRMReceiver::GetRigModel()
{
	if(pHamlib)
		return pHamlib->GetHamlibModelID();
	return 0;
}

void CDRMReceiver::GetRigList(map<rig_model_t,CRigCaps>& rigs)
{
	if(pHamlib)
		pHamlib->GetRigList(rigs);
}

void CDRMReceiver::GetRigCaps(CRigCaps& caps)
{
	if(pHamlib)
		pHamlib->GetRigCaps(caps);
}

void CDRMReceiver::GetComPortList(map<string,string>& ports)
{
	if(pHamlib)
		pHamlib->GetPortList(ports);
}

string CDRMReceiver::GetRigComPort()
{
	if(pHamlib)
		return pHamlib->GetComPort();
	return "";
}

void CDRMReceiver::SetRigFreqOffset(int iVal)
{
	if(pHamlib)
		pHamlib->SetFreqOffset(iVal);
}

void CDRMReceiver::SetRigComPort(const string& s)
{
	if(pHamlib)
		pHamlib->SetComPort(s);
}
#endif

_BOOLEAN
CDRMReceiver::GetSignalStrength(_REAL& rSigStr)
{
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
	static FILE *pFile = NULL;

	if(pFile == NULL)
		pFile = fopen("test/altfreq.dat", "w");

	Parameters.Lock();
	size_t inum = Parameters.AltFreqSign.vecMultiplexes.size();
	for (size_t z = 0; z < inum; z++)
	{
		fprintf(pFile, "sync:%d sr:", Parameters.AltFreqSign.vecMultiplexes[z].bIsSyncMultplx);

		for (int k = 0; k < 4; k++)
				fprintf(pFile, "%d", Parameters.AltFreqSign.vecMultiplexes[z].  veciServRestrict[k]);
		fprintf(pFile, " fr:");

		for (size_t kk = 0; kk < Parameters.AltFreqSign.vecMultiplexes[z].veciFrequencies.size(); kk++)
			fprintf(pFile, "%d ", Parameters.AltFreqSign.vecMultiplexes[z].  veciFrequencies[kk]);

		fprintf(pFile, " rID:%d sID:%d   /   ",
					Parameters.AltFreqSign.vecMultiplexes[z].iRegionID,
					Parameters.AltFreqSign.vecMultiplexes[z].iScheduleID);
	}
	Parameters.Unlock();
	fprintf(pFile, "\n");
	fflush(pFile);
}

void
CDRMReceiver::LoadSettings(CSettings& s)
{
	string strMode = s.Get("0", "mode", string("DRMRX"));

	if (strMode == "DRMRX")
	{
		eNewReceiverMode = RM_DRM;
		Parameters = DRMParameters;
	}
	else if (strMode == "AMRX")
	{
		eNewReceiverMode = RM_AM;
		Parameters = AMParameters;
	}

	//else - leave it as initialised (ie. DRM)

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
	EDemodType eDemodType = EDemodType(s.Get("Demodulator", "modulation", DT_AM));

	if(s.Get("Demodulator", "usehw", 0))
		demodulation = onBoard;
	else
		demodulation = inSoftware;

	/* AM Parameters */

	/* AGC */
	AMDemodulation.SetAGCType((CAGC::EType)s.Get("AM Demodulation", "agc", 0));

	/* noise reduction */
	AMDemodulation.SetNoiRedType((CAMDemodulation::ENoiRedType)s.Get("AM Demodulation", "noisered", 0));

	/* pll enabled/disabled */
	AMDemodulation.EnablePLL(s.Get("AM Demodulation", "enablepll", 0));

	/* auto frequency acquisition */
	AMDemodulation.EnableAutoFreqAcq(s.Get("AM Demodulation", "autofreqacq", 0));

	AMDemodulation.SetDemodType(DT_AM);
	AMDemodulation.SetFilterBWHz(s.Get("AM Demodulation", "filterbwam", 10000));
	AMDemodulation.SetDemodType(DT_LSB);
	AMDemodulation.SetFilterBWHz(s.Get("AM Demodulation", "filterbwlsb", 5000));
	AMDemodulation.SetDemodType(DT_USB);
	AMDemodulation.SetFilterBWHz(s.Get("AM Demodulation", "filterbwusb", 5000));
	AMDemodulation.SetDemodType(DT_CW);
	AMDemodulation.SetFilterBWHz(s.Get("AM Demodulation", "filterbwcw", 150));

	/* FM Parameters */
	AMDemodulation.SetDemodType(DT_NBFM);
	AMDemodulation.SetFilterBWHz(s.Get("FM Demodulation", "nbfilterbw", 6000));
	AMDemodulation.SetDemodType(DT_WBFM);
	AMDemodulation.SetFilterBWHz(s.Get("FM Demodulation", "wbfilterbw", 80000));

	AMDemodulation.SetDemodType(eDemodType);

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
		downstreamRSCI.SetRSIRecording(Parameters, TRUE, str[0], s2);

	/* IQ File Recording */
	if(s.Get("command", "recordiq", false))
		WriteIQFile.StartRecording(Parameters);

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

	/* Wanted RF Frequency */
	SetFrequency(s.Get("Receiver", "frequency", 0));
}

void
CDRMReceiver::SaveSettings(CSettings& s)
{
	if(eReceiverMode == RM_AM)
		s.Put("0", "mode", string("AMRX"));
	else
		s.Put("0", "mode", string("DRMRX"));

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
	s.Put("Demodulator", "modulation", AMDemodulation.GetDemodType());
	s.Put("Demodulator", "usehw", (demodulation==onBoard)?1:0);

	/* AM Parameters */

	/* AGC */
	s.Put("AM Demodulation", "agc", AMDemodulation.GetAGCType());

	/* noise reduction */
	s.Put("AM Demodulation", "noisered", AMDemodulation.GetNoiRedType());

	/* pll enabled/disabled */
	s.Put("AM Demodulation", "enablepll", AMDemodulation.PLLEnabled());

	/* auto frequency acquisition */
	s.Put("AM Demodulation", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

	s.Put("AM Demodulation", "filterbwam", AMDemodulation.GetFilterBWHz(DT_AM));
	s.Put("AM Demodulation", "filterbwlsb", AMDemodulation.GetFilterBWHz(DT_LSB));
	s.Put("AM Demodulation", "filterbwusb", AMDemodulation.GetFilterBWHz(DT_USB));
	s.Put("AM Demodulation", "filterbwcw", AMDemodulation.GetFilterBWHz(DT_CW));

	/* FM Parameters */

	s.Put("FM Demodulation", "nbfilterbw", AMDemodulation.GetFilterBWHz(DT_NBFM));
	s.Put("FM Demodulation", "wbfilterbw", AMDemodulation.GetFilterBWHz(DT_WBFM));

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
