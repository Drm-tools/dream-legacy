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
 * convention is always "input-buffer, output-buffer". Additional, the
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

#include "util/LogPrint.h"
#include "sound.h"
#include "audiofilein.h"

const int
	CDRMReceiver::MAX_UNLOCKED_COUNT = 2;

// TODO don't create a CSoundIn if its not going to be used. It helps for now because of SetDev in the GUI
/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver():
pSoundInInterface(new CSoundIn), pSoundOutInterface(new CSoundOut),
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
eNewReceiverMode(RM_NONE), iAudioStreamID(STREAM_ID_NOT_USED),
iDataStreamID(STREAM_ID_NOT_USED), bDoInitRun(FALSE),
rInitResampleOffset((_REAL) 0.0),
vecrFreqSyncValHist(LEN_HIST_PLOT_SYNC_PARMS),
vecrSamOffsValHist(LEN_HIST_PLOT_SYNC_PARMS),
vecrLenIRHist(LEN_HIST_PLOT_SYNC_PARMS),
vecrDopplerHist(LEN_HIST_PLOT_SYNC_PARMS),
vecrSNRHist(LEN_HIST_PLOT_SYNC_PARMS),
veciCDAudHist(LEN_HIST_PLOT_SYNC_PARMS), iSymbolCount(0),
rSumDopplerHist((_REAL) 0.0), rSumSNRHist((_REAL) 0.0), iCurrentCDAud(0),
#ifdef USE_QT_GUI
	RigPoll(),
#endif
	iBwAM(10000), iBwLSB(5000), iBwUSB(5000), iBwCW(150), iBwFM(6000),
AMDemodType(CAMDemodulation::DT_AM),
#ifdef _WIN32
	bProcessPriorityEnabled(TRUE),
#endif
	bReadFromFile(FALSE), time_keeper(0)
#if defined(USE_QT_GUI) || defined(_WIN32)
	, GeomChartWindows(0), iMainPlotColorStyle(0),	/* default color scheme: blue-white */
	iSecondsPreview(0), iSecondsPreviewLiveSched(0), bShowAllStations(TRUE), SortParamAnalog(0, TRUE),	/* Sort list by station name  */
	/* Sort list by transmit power (5th column), most powerful on top */
	SortParamDRM(4, FALSE), SortParamLiveSched(0, FALSE),	/* sort by frequency */
	iSysEvalDlgPlotType(0), iMOTBWSRefreshTime(10), bAddRefreshHeader(TRUE), strStoragePathMMDlg(""), strStoragePathLiveScheduleDlg(""), iMainDisplayColor(0xff0000),	/* Red */
	FontParamMMDlg("", 1, 0, FALSE)
#endif
{
	pReceiverParam = new CParameter(this);
	downstreamRSCI.SetReceiver(this);
#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
	RigPoll.SetReceiver(this);
#endif
}

CDRMReceiver::~CDRMReceiver()
{
#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
	if (RigPoll.running())
		RigPoll.stop();
	if (RigPoll.wait(1000) == FALSE)
		cout << "error terminating rig polling thread" << endl;
#endif
	delete pSoundInInterface;
	delete pSoundOutInterface;
}



void
CDRMReceiver::Run()
{
	_BOOLEAN bEnoughData = TRUE;
	_BOOLEAN bFrameToSend = FALSE;

	/* Check for parameter changes from RSCI or GUI thread --------------- */
	/* The parameter changes are done through flags, the actual initialization
	 * is done in this (the working) thread to avoid problems with shared data */
	if (eNewReceiverMode != RM_NONE)
		InitReceiverMode();

	CParameter & ReceiverParam = *pReceiverParam;

	/* Check for changes in front end selection */
#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
	if (bEnableSMeter && upstreamRSCI.GetInEnabled() == FALSE)
	{
		if (RigPoll.running() == FALSE)
			RigPoll.start();
	}
	else
	{
		if (RigPoll.running())
			RigPoll.stop();
	}
#endif

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
				bFrameToSend = TRUE;
			}
			else
			{
				time_t now = time(NULL);
				if ((now - time_keeper) > 2)
				{
					ReceiverParam.ReceiveStatus.SetInterfaceStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetTimeSyncStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetFrameSyncStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetFACStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetSDCStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetAudioStatus(NOT_PRESENT);
					ReceiverParam.ReceiveStatus.SetMOTStatus(NOT_PRESENT);
				}
			}
		}
	}
	else
	{
#if defined(HAVE_LIBHAMLIB) && !defined(USE_QT_GUI)
		/* TODO - get the polling interval sensible */
		_BOOLEAN bValid;
		_REAL r;
		bValid = Hamlib.GetSMeter(r) == CHamlib::SS_VALID;
		ReceiverParam.SetSignalStrength(bValid, r);
#endif
		ReceiveData.ReadData(ReceiverParam, RecDataBuf);

		if ((eReceiverMode == RM_AM) || bDoInitRun)
		{
				DemodulateAM(bEnoughData);
				DecodeAM(bEnoughData);
		}

		if ((eReceiverMode == RM_DRM) || bDoInitRun)
		{
				DemodulateDRM(bEnoughData);
				DecodeDRM(bEnoughData, bFrameToSend);
		}
	}

	/* Split the data for downstream RSCI and local processing. TODO make this conditional */
	if (eReceiverMode == RM_AM)
	{
		SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);

		/* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
		if (SDCDecBuf.GetFillLevel() == ReceiverParam.iNumSDCBitsPerSFrame)
		{
			SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
		}

		for (size_t i = 0; i < MSCDecBuf.size(); i++)
		{
			SplitMSC[i].ProcessData(ReceiverParam, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
		}
	}
	else
	{
		SplitAudio.ProcessData(ReceiverParam, AMAudioBuf, AudSoDecBuf, AMSoEncBuf);
	}

	/* decoding */
	while (bEnoughData && ReceiverParam.bRunThread)
	{
		/* Init flag */
		bEnoughData = FALSE;

		// Split samples, one output to the demodulation, another for IQ recording
		if (SplitForIQRecord.ProcessData(ReceiverParam, RecDataBuf, DemodDataBuf, IQRecordDataBuf))
		{
			bEnoughData = TRUE;
		}

		// Write output I/Q file 
		if (WriteIQFile.WriteData(ReceiverParam, IQRecordDataBuf))
		{
			bEnoughData = TRUE;
		}

		if ((eReceiverMode == RM_AM) || bDoInitRun)
		{
			UtilizeAM(bEnoughData);
		}

		if ((eReceiverMode == RM_DRM) || bDoInitRun)
		{
			UtilizeDRM(bEnoughData);
		}
	}

	/* output to downstream RSCI */
	if (downstreamRSCI.GetOutEnabled())
	{
		if (eReceiverMode == RM_DRM)
		{
			if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
			{
				/* we will get one of these between each FAC block, and occasionally we */
				/* might get two, so don't start generating free-wheeling RSCI until we've. */
				/* had three in a row */
				if (FreqSyncAcq.GetUnlockedFrameBoundary())
					if (iUnlockedCount < MAX_UNLOCKED_COUNT)
						iUnlockedCount++;
					else
						downstreamRSCI.SendUnlockedFrame(ReceiverParam);
			}
			else if (bFrameToSend)
			{
				downstreamRSCI.SendLockedFrame(ReceiverParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
				iUnlockedCount = 0;
				bFrameToSend = FALSE;
			}
		}
		else
		{

			/* Encode audio for RSI output */
			if (AudioSourceEncoder.ProcessData(ReceiverParam, AMSoEncBuf, MSCSendBuf[0]))
			{
				bFrameToSend = TRUE;
			}

			if (bFrameToSend)
			{
				downstreamRSCI.SendAMFrame(ReceiverParam, MSCSendBuf[0]);
			}

		}
	}

	/* Play and/or save the audio */
	if (iAudioStreamID != STREAM_ID_NOT_USED)
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
		if (pReceiverParam->eAcquiState == AS_NO_SIGNAL)
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
		UpdateParamHistories();
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
			iCurrentCDAud = AudioSourceDecoder.GetNumCorDecAudio();
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
	eReceiverMode = eNewReceiverMode;

	/* Init all modules */
	SetInStartMode();

	if (eReceiverMode == RM_AM)
	{
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
				/* change from DRM to AM Mode - we better have our own parameter instance */
				pReceiverParam = pAMParam = new CParameter(this);
			}
		}
		else
		{
			/* been in AM mode before, there should be a parameter instance for us. */
			pReceiverParam = pAMParam;
		}

		if (pReceiverParam == NULL)
			throw CGenErr("Something went terribly wrong in the Receiver");

		/* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);

		/* Set the receive status - this affects the RSI output */
		pAMParam->ReceiveStatus.SetTimeSyncStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SetFrameSyncStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SetFACStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SetSDCStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SetAudioStatus(NOT_PRESENT);
		pAMParam->ReceiveStatus.SetMOTStatus(NOT_PRESENT);
	}
	else
	{
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
				/* change from AM to DRM Mode - we better have our own parameter instance */
				pReceiverParam = pDRMParam = new CParameter(this);
			}
		}
		else
		{
			/* been in DRM mode before, there should be a parameter instance for us. */
			pReceiverParam = pDRMParam;
		}

		if (pReceiverParam == NULL)
			throw CGenErr("Something went terribly wrong in the Receiver");

		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_DRM);
	}

	/* Reset new mode flag */
	eNewReceiverMode = RM_NONE;

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetReceiverMode(eReceiverMode);
	}
}

#ifdef USE_QT_GUI
void
CDRMReceiver::run()
{
	/* Set thread priority (the working thread should have a higher priority
	   than tthe GUI) */
#ifdef _WIN32
	if (GetEnableProcessPriority())
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

	/* Reset all parameters to start parameter settings */
	SetInStartMode();

	do
	{
		Run();

	}
	while (pReceiverParam->bRunThread);

	if (upstreamRSCI.GetInEnabled() == FALSE)
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
	iUnlockedCount = MAX_UNLOCKED_COUNT;

	/* Load start parameters for all modules */

	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	pReceiverParam->InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

	/* Set initial MLC parameters */
	pReceiverParam->SetInterleaverDepth(CParameter::SI_LONG);
	pReceiverParam->SetMSCCodingScheme(CParameter::CS_3_SM);
	pReceiverParam->SetSDCCodingScheme(CParameter::CS_2_SM);

	/* Select the service we want to decode. Always zero, because we do not
	   know how many services are transmitted in the signal we want to
	   decode */

	/* TODO: if service 0 is not used but another service is the audio service
	 * we have a problem. We should check as soon as we have information about
	 * services if service 0 is really the audio service
	 */

	/* Set the following parameters to zero states (initial states) --------- */
	pReceiverParam->ResetServicesStreams();

	pReceiverParam->ResetCurSelAudDatServ();

	/* Protection levels */
	pReceiverParam->MSCPrLe.iPartA = 0;
	pReceiverParam->MSCPrLe.iPartB = 1;
	pReceiverParam->MSCPrLe.iHierarch = 0;

	/* Number of audio and data services */
	pReceiverParam->iNumAudioService = 0;
	pReceiverParam->iNumDataService = 0;

	/* We start with FAC ID = 0 (arbitrary) */
	pReceiverParam->iFrameIDReceiv = 0;

	/* Set synchronization parameters */
	pReceiverParam->rResampleOffset = rInitResampleOffset;	/* Initial resample offset */
	pReceiverParam->rFreqOffsetAcqui = (_REAL) 0.0;
	pReceiverParam->rFreqOffsetTrack = (_REAL) 0.0;
	pReceiverParam->iTimingOffsTrack = 0;

	/* Init reception log (log long) transmission parameters. TODO: better solution */
	pReceiverParam->ReceptLog.ResetTransParams();

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

	/* Set flag that no signal is currently received */
	pReceiverParam->eAcquiState = AS_NO_SIGNAL;

	/* Set flag for receiver state */
	eReceiverState = RS_ACQUISITION;

	/* Reset counters for acquisition decision, "good signal" and delayed
	   tracking mode counter */
	iAcquRestartCnt = 0;
	iAcquDetecCnt = 0;
	iGoodSignCnt = 0;
	iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

	/* Reset GUI lights */
	pReceiverParam->ReceiveStatus.SetInterfaceStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetTimeSyncStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetFrameSyncStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetFACStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetSDCStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetAudioStatus(NOT_PRESENT);
	pReceiverParam->ReceiveStatus.SetMOTStatus(NOT_PRESENT);

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
		pReceiverParam->eAcquiState = AS_WITH_SIGNAL;

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
CDRMReceiver::SetReadDRMFromFile(const string strNFN)
{
	delete pSoundInInterface;
	CAudioFileIn *pf = new CAudioFileIn;
	pf->SetFileName(strNFN);
	pSoundInInterface = pf;
	ReceiveData.SetSoundInterface(pSoundInInterface);	// needed ?
	string ext;
	size_t p = strNFN.rfind('.');
	if (p != string::npos)
		ext = strNFN.substr(p + 1);
	_BOOLEAN bIsIQ = FALSE;
	if (ext.substr(0, 2) == "iq")
		bIsIQ = TRUE;
	if (ext.substr(0, 2) == "IQ")
		bIsIQ = TRUE;
	if (bIsIQ)
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS_ZERO);
	else
		ReceiveData.SetInChanSel(CReceiveData::CS_MIX_CHAN);
	bReadFromFile = TRUE;
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
	iSymbolCount = 0;
	rSumDopplerHist = (_REAL) 0.0;
	rSumSNRHist = (_REAL) 0.0;

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
	iCurrentCDAud = 0;
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
	iAudioStreamID = pReceiverParam->GetAudioParam(a).iStreamID;
	pReceiverParam->SetNumAudioDecoderBits(pReceiverParam->
										   GetStreamLen(iAudioStreamID) *
										   SIZEOF__BYTE);
	AudioSourceDecoder.SetInitFlag();
}

void
CDRMReceiver::InitsForDataParam()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int d = pReceiverParam->GetCurSelDataService();
	iDataStreamID = pReceiverParam->GetDataParam(d).iStreamID;
	pReceiverParam->SetNumDataDecoderBits(pReceiverParam->
										  GetStreamLen(iDataStreamID) *
										  SIZEOF__BYTE);
	DataDecoder.SetInitFlag();
}

/* Parameter histories for plot --------------------------------------------- */
void
CDRMReceiver::UpdateParamHistories()
{

	/* TODO: do not use the shift register class, build a new
	   one which just increments a pointer in a buffer and put
	   the new value at the position of the pointer instead of
	   moving the total data all the time -> special care has
	   to be taken when reading out the data */

	/* Only update histories if the receiver is in tracking mode */
	if (eReceiverState == RS_TRACKING)
	{
#ifdef USE_QT_GUI
		MutexHist.lock();
#endif

		/* Frequency offset tracking values */
		vecrFreqSyncValHist.AddEnd(pReceiverParam->rFreqOffsetTrack *
								   SOUNDCRD_SAMPLE_RATE);

		/* Sample rate offset estimation */
		vecrSamOffsValHist.AddEnd(pReceiverParam->rResampleOffset);
		/* Signal to Noise ratio estimates */
		rSumSNRHist += pReceiverParam->rSNREstimate;

/* TODO - reconcile this with Ollies RSCI Doppler code in ChannelEstimation */
		/* Average Doppler estimate */
		rSumDopplerHist += pReceiverParam->rSigmaEstimate;

		/* Only evaluate Doppler and delay once in one DRM frame */
		iSymbolCount++;
		if (iSymbolCount == pReceiverParam->iNumSymPerFrame)
		{
			/* Apply averaged values to the history vectors */
			vecrLenIRHist.
				AddEnd((pReceiverParam->rMinDelay +
						pReceiverParam->rMaxDelay) / 2.0);

			vecrSNRHist.AddEnd(rSumSNRHist / pReceiverParam->iNumSymPerFrame);

			vecrDopplerHist.AddEnd(rSumDopplerHist /
								   pReceiverParam->iNumSymPerFrame);

			/* At the same time, add number of correctly decoded audio blocks.
			   This number is updated once a DRM frame. Since the other
			   parameters like SNR is also updated once a DRM frame, the two
			   values are synchronized by one DRM frame */
			veciCDAudHist.AddEnd(iCurrentCDAud);

			/* Reset parameters used for averaging */
			iSymbolCount = 0;
			rSumDopplerHist = (_REAL) 0.0;
			rSumSNRHist = (_REAL) 0.0;
		}

#ifdef USE_QT_GUI
		MutexHist.unlock();
#endif
	}
}

void
CDRMReceiver::GetFreqSamOffsHist(CVector < _REAL > &vecrFreqOffs,
								 CVector < _REAL > &vecrSamOffs,
								 CVector < _REAL > &vecrScale,
								 _REAL & rFreqAquVal)
{
	/* Init output vectors */
	vecrFreqOffs.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrSamOffs.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffers in output buffers */
	vecrFreqOffs = vecrFreqSyncValHist;
	vecrSamOffs = vecrSamOffsValHist;

	/* Duration of OFDM symbol */
	const _REAL rTs = (CReal) (pReceiverParam->iFFTSizeN +
							   pReceiverParam->iGuardSize) /
		SOUNDCRD_SAMPLE_RATE;

	/* Calculate time scale */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rTs;

	/* Value from frequency acquisition */
	rFreqAquVal = pReceiverParam->rFreqOffsetAcqui * SOUNDCRD_SAMPLE_RATE;

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CDRMReceiver::GetDopplerDelHist(CVector < _REAL > &vecrLenIR,
								CVector < _REAL > &vecrDoppler,
								CVector < _REAL > &vecrScale)
{
	/* Init output vectors */
	vecrLenIR.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrDoppler.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffers in output buffers */
	vecrLenIR = vecrLenIRHist;
	vecrDoppler = vecrDopplerHist;

	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (pReceiverParam->iFFTSizeN +
										pReceiverParam->iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * pReceiverParam->iNumSymPerFrame;

	/* Calculate time scale in minutes */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rDRMFrameDur / 60;

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CDRMReceiver::GetSNRHist(CVector < _REAL > &vecrSNR,
						 CVector < _REAL > &vecrCDAud,
						 CVector < _REAL > &vecrScale)
{
	/* Init output vectors */
	vecrSNR.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrCDAud.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffer in output buffer */
	vecrSNR = vecrSNRHist;

	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (pReceiverParam->iFFTSizeN +
										pReceiverParam->iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * pReceiverParam->iNumSymPerFrame;

	/* Calculate time scale. Copy correctly decoded audio blocks history (must
	   be transformed from "int" to "real", therefore we need a for-loop */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
	{
		/* Scale in minutes */
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rDRMFrameDur / 60;

		/* Correctly decoded audio blocks */
		vecrCDAud[i] = (_REAL) veciCDAudHist[i];
	}

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

_BOOLEAN CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
	if (iFreqkHz == iNewFreqkHz)
		return TRUE;
	iFreqkHz = iNewFreqkHz;

	pReceiverParam->ReceptLog.SetFrequency(iNewFreqkHz);

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

		//WriteIQFile.NewFrequency(*pReceiverParam);

#ifdef HAVE_LIBHAMLIB
		return Hamlib.SetFrequency(iNewFreqkHz);
#else
		return TRUE;
#endif
	}
}

/* if we have QT, use a thread to poll the signal strength
 * otherwise do it when needed
 */

_BOOLEAN CDRMReceiver::GetSignalStrength(_REAL & rSigStr)
{
	return pReceiverParam->GetSignalStrength(rSigStr);
}

#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
void
CDRMReceiver::CRigPoll::run()
{
	CHamlib & rig = *pDRMRec->GetHamlib();
	while (bQuit == FALSE)
	{
		_REAL
			r;
		if (rig.GetSMeter(r) == CHamlib::SS_VALID)
		{
			// Apply any correction
			r += pDRMRec->GetParameters()->rSigStrengthCorrection;
			pDRMRec->GetParameters()->SetSignalStrength(TRUE, r);
		}
		else
			pDRMRec->GetParameters()->SetSignalStrength(FALSE, 0.0);
		msleep(400);
	}
}
#endif

void
CDRMReceiver::SetRSIRecording(const _BOOLEAN bOn, const char cPro)
{
	downstreamRSCI.SetRSIRecording(*pReceiverParam, bOn, cPro);
}

void
CDRMReceiver::SetIQRecording(const _BOOLEAN bOn)
{
	if (bOn)
		WriteIQFile.StartRecording(*pReceiverParam);
	else
		WriteIQFile.StopRecording();
}

/* TEST store information about alternative frequency transmitted in SDC */
void
CDRMReceiver::saveSDCtoFile()
{
	CParameter & ReceiverParam = *pReceiverParam;
	static FILE *pFile = NULL;

	if(pFile == NULL)
		pFile = fopen("test/altfreq.dat", "w");

	int inum = ReceiverParam.AltFreqSign.vecAltFreq.Size();
	for (int z = 0; z < inum; z++)
	{
		fprintf(pFile, "sync:%d sr:", ReceiverParam.AltFreqSign.vecAltFreq[z].bIsSyncMultplx);

		for (int k = 0; k < 4; k++)
				fprintf(pFile, "%d", ReceiverParam.AltFreqSign.vecAltFreq[z].  veciServRestrict[k]);
		fprintf(pFile, " fr:");

		for (int kk = 0; kk < ReceiverParam.AltFreqSign.vecAltFreq[z].veciFrequencies.Size(); kk++)
			fprintf(pFile, "%d ", ReceiverParam.AltFreqSign.vecAltFreq[z].  veciFrequencies[kk]);

		fprintf(pFile, " rID:%d sID:%d   /   ",
					ReceiverParam.AltFreqSign.vecAltFreq[z].iRegionID,
					ReceiverParam.AltFreqSign.vecAltFreq[z].iScheduleID);
	}
	fprintf(pFile, "\n");
	fflush(pFile);
}
