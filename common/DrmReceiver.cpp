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
#include "util/Settings.h"

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
iAcquDetecCnt(0), iGoodSignCnt(0), eReceiverMode(RM_NONE),
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
#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
	RigPoll(),
#endif
	iBwAM(10000), iBwLSB(5000), iBwUSB(5000), iBwCW(150), iBwFM(6000),
	bReadFromFile(FALSE), time_keeper(0)
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
CDRMReceiver::SetEnableSMeter(_BOOLEAN bNew)
{
	bEnableSMeter = bNew;
}

_BOOLEAN
CDRMReceiver::GetEnableSMeter()
{
	return bEnableSMeter;
}

void
CDRMReceiver::SetAMDemodType(CAMDemodulation::EDemodType eNew)
{
	AMDemodulation.SetDemodType(eNew);
	switch (eNew)
	{
	case CAMDemodulation::DT_AM:
		AMDemodulation.SetFilterBW(iBwAM);
		break;

	case CAMDemodulation::DT_LSB:
		AMDemodulation.SetFilterBW(iBwLSB);
		break;

	case CAMDemodulation::DT_USB:
		AMDemodulation.SetFilterBW(iBwUSB);
		break;

	case CAMDemodulation::DT_CW:
		AMDemodulation.SetFilterBW(iBwCW);
		break;

	case CAMDemodulation::DT_FM:
		AMDemodulation.SetFilterBW(iBwFM);
		break;
	}
}

void
CDRMReceiver::SetAMFilterBW(int value)
{
	/* Store filter bandwidth for this demodulation type */
	switch (AMDemodulation.GetDemodType())
	{
	case CAMDemodulation::DT_AM:
		iBwAM = value;
		break;

	case CAMDemodulation::DT_LSB:
		iBwLSB = value;
		break;

	case CAMDemodulation::DT_USB:
		iBwUSB = value;
		break;

	case CAMDemodulation::DT_CW:
		iBwCW = value;
		break;

	case CAMDemodulation::DT_FM:
		iBwFM = value;
		break;
	}
	AMDemodulation.SetFilterBW(value);
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
	/* Reset new mode flag */
	eNewReceiverMode = RM_NONE;

	switch(eReceiverMode)
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
				/* change from DRM to AM Mode - we better have our own parameter instance */
				pAMParam = new CParameter(this);
			}
			/* copy some important state from the DRM parameters */
			pAMParam->bRunThread = pReceiverParam->bRunThread;
		}
		pReceiverParam = pAMParam;

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
				/* change from AM to DRM Mode - we better have our own parameter instance */
				pDRMParam = new CParameter(this);
			}
			/* copy some important state from the AM parameters */
			pDRMParam->bRunThread = pReceiverParam->bRunThread;
		}
		pReceiverParam = pDRMParam;

		if (pReceiverParam == NULL)
			throw CGenErr("Something went terribly wrong in the Receiver");

		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_DRM);
		break;
	case RM_NONE:
		return;
	}

	/* Init all modules */
	SetInStartMode();

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetReceiverMode(eReceiverMode);
	}
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
	pReceiverParam->SetMSCCodingScheme(CS_3_SM);
	pReceiverParam->SetSDCCodingScheme(CS_2_SM);

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
		if (iSymbolCount == pReceiverParam->CellMappingTable.iNumSymPerFrame)
		{
			/* Apply averaged values to the history vectors */
			vecrLenIRHist.
				AddEnd((pReceiverParam->rMinDelay +
						pReceiverParam->rMaxDelay) / 2.0);

			vecrSNRHist.AddEnd(rSumSNRHist / pReceiverParam->CellMappingTable.iNumSymPerFrame);

			vecrDopplerHist.AddEnd(rSumDopplerHist /
								   pReceiverParam->CellMappingTable.iNumSymPerFrame);

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
	const _REAL rTs = (CReal) (pReceiverParam->CellMappingTable.iFFTSizeN + pReceiverParam->CellMappingTable.iGuardSize) / SOUNDCRD_SAMPLE_RATE;

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
	const _REAL rDRMFrameDur = (CReal) (pReceiverParam->CellMappingTable.iFFTSizeN + pReceiverParam->CellMappingTable.iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * pReceiverParam->CellMappingTable.iNumSymPerFrame;

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
	const _REAL rDRMFrameDur = (CReal) (pReceiverParam->CellMappingTable.iFFTSizeN + pReceiverParam->CellMappingTable.iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * pReceiverParam->CellMappingTable.iNumSymPerFrame;

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

		WriteIQFile.NewFrequency(*pReceiverParam);

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
	CHamlib & rig = *(pDRMRec->GetHamlib());
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

	size_t inum = ReceiverParam.AltFreqSign.vecAltFreq.size();
	for (size_t z = 0; z < inum; z++)
	{
		fprintf(pFile, "sync:%d sr:", ReceiverParam.AltFreqSign.vecAltFreq[z].bIsSyncMultplx);

		for (int k = 0; k < 4; k++)
				fprintf(pFile, "%d", ReceiverParam.AltFreqSign.vecAltFreq[z].  veciServRestrict[k]);
		fprintf(pFile, " fr:");

		for (size_t kk = 0; kk < ReceiverParam.AltFreqSign.vecAltFreq[z].veciFrequencies.size(); kk++)
			fprintf(pFile, "%d ", ReceiverParam.AltFreqSign.vecAltFreq[z].  veciFrequencies[kk]);

		fprintf(pFile, " rID:%d sID:%d   /   ",
					ReceiverParam.AltFreqSign.vecAltFreq[z].iRegionID,
					ReceiverParam.AltFreqSign.vecAltFreq[z].iScheduleID);
	}
	fprintf(pFile, "\n");
	fflush(pFile);
}

void
CDRMReceiver::LoadSettings(const CSettings& s)
{
	/* Receiver ------------------------------------------------------------- */
	string str;
	int n;
	/* input from file */
	str = s.Get("command", "fileio");
	if(str != "")
		SetReadDRMFromFile(str);

	/* Flip spectrum flag */
	ReceiveData.SetFlippedSpectrum(s.Get("Receiver", "flipspectrum", FALSE));

	n = s.Get("command", "inchansel", -1);
	switch (n)
	{
	case 0:
		ReceiveData.SetInChanSel(CReceiveData::CS_LEFT_CHAN);
		break;

	case 1:
		ReceiveData.SetInChanSel(CReceiveData::CS_RIGHT_CHAN);
		break;

	case 2:
		ReceiveData.SetInChanSel(CReceiveData::CS_MIX_CHAN);
		break;

	case 3:
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS);
		break;

	case 4:
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_NEG);
		break;

	case 5:
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS_ZERO);
		break;

	case 6:
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_NEG_ZERO);
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

	/* AM Parameters */

	/* AGC */
	AMDemodulation.SetAGCType((CAGC::EType)s.Get("AM Demodulation", "agc", 0)); 

	/* noise reduction */
	AMDemodulation.SetNoiRedType((CAMDemodulation::ENoiRedType)s.Get("AM Demodulation", "noisered", 0));

	/* pll enabled/disabled */
	AMDemodulation.EnablePLL(s.Get("AM Demodulation", "enablepll", 0));

	/* auto frequency acquisition */
	AMDemodulation.EnableAutoFreqAcq(s.Get("AM Demodulation", "autofreqacq", 0));

	/* demodulation */
	CAMDemodulation::EDemodType DemodType
		= (CAMDemodulation::EDemodType)s.Get("AM Demodulation", "demodulation", CAMDemodulation::DT_AM);

	AMDemodulation.SetDemodType(DemodType);

	iBwAM = s.Get("AM Demodulation", "filterbwam", 10000);
	iBwUSB = s.Get("AM Demodulation", "filterbwusb", 5000);
	iBwLSB = s.Get("AM Demodulation", "filterbwlsb", 5000);
	iBwCW = s.Get("AM Demodulation", "filterbwcw", 150);
	iBwFM = s.Get("AM Demodulation", "filterbwfm", 6000);

	/* Load user's saved filter bandwidth for the demodulation type. */
	switch (DemodType)
	{
	case CAMDemodulation::DT_AM:
		AMDemodulation.SetFilterBW(iBwAM);
		break;

	case CAMDemodulation::DT_LSB:
		AMDemodulation.SetFilterBW(iBwLSB);
		break;

	case CAMDemodulation::DT_USB:
		AMDemodulation.SetFilterBW(iBwUSB);
		break;

	case CAMDemodulation::DT_CW:
		AMDemodulation.SetFilterBW(iBwCW);
		break;

	case CAMDemodulation::DT_FM:
		AMDemodulation.SetFilterBW(iBwFM);
		break;
	}

	/* Init slider control for bandwidth setting */
	/* upstream RSCI */
	str = s.Get("command", "rsiin");
	if(str != "")
		upstreamRSCI.SetOrigin(str);
	str = s.Get("command", "rciout");
	if(str != "")
		upstreamRSCI.SetDestination(str);

	/* downstream RSCI */
	str = s.Get("command", "rciin");
	if(str != "")
		downstreamRSCI.SetOrigin(str);
	for(int i = 0; i<MAX_NUM_RSI_SUBSCRIBERS; i++)
	{
		stringstream ss;
		ss << "rsiout" << i;
		str = s.Get("command", ss.str());
		if(str != "")
		{
			downstreamRSCI.SetDestination(str);
			ss.str("rsioutprofile");
			ss << i;
			str = s.Get("command", ss.str());
			if(str != "")
				downstreamRSCI.SetProfile(str[0]);
		}
	}
	/* RSCI File Recording */
	str = s.Get("command", "rsirecordprofile");
	if(str != "")
		SetRSIRecording(TRUE, str[0]);

	/* IQ File Recording */
	if(s.Get("command", "recordiq", false))
		SetIQRecording(TRUE);

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

	/* Sound In device */
	pSoundInInterface->SetDev(s.Get("Receiver", "snddevin", 0));

	/* Sound Out device */
	pSoundOutInterface->SetDev(s.Get("Receiver", "snddevout", 0));

	/* Number of iterations for MLC setting */
	MSCMLCDecoder.SetNumIterations(s.Get("Receiver", "mlciter", 0));

	/* Wanted RF Frequency file */
	SetFrequency(s.Get("Receiver", "frequency", 0));

	/* Activate/Deactivate EPG decoding */
	DataDecoder.SetDecodeEPG(s.Get("EPG", "decodeepg", TRUE));

	/* Logfile -------------------------------------------------------------- */
	CReceptLog& ReceptLog = pReceiverParam->ReceptLog;
	/* Start log file flag */
	ReceptLog.SetLoggingEnabled(s.Get("Logfile", "enablelog", FALSE));

    /* log file flag for storing signal strength in long log */
	ReceptLog.SetRxlEnabled(s.Get("Logfile", "enablerxl", FALSE));

	/* log file flag for storing lat/long in long log */
	ReceptLog.SetPositionEnabled(s.Get("Logfile", "enablepositiondata", FALSE));

	/* logging delay value */
	ReceptLog.SetDelLogStart(s.Get("Logfile", "delay", 0));

	{
		_REAL latitude, longitude;
		/* Latitude string for log file */
		latitude = s.Get("Logfile", "latitude", 1000.0);
	/* Longitude string for log file */
		longitude = s.Get("Logfile", "longitude", 1000.0);
		if(-90.0 <= latitude && latitude <= 90.0 && -180.0 <= longitude  && longitude <= 180.0)
		{
			ReceptLog.GPSData.SetPositionAvailable(TRUE);
			ReceptLog.GPSData.SetLatLongDegrees(latitude, longitude);
		}
		else
			ReceptLog.GPSData.SetPositionAvailable(FALSE);
	}

#ifdef HAVE_LIBHAMLIB
	/* Hamlib --------------------------------------------------------------- */
	/* Hamlib Model ID */
	Hamlib.SetHamlibModelID(s.Get("Hamlib",	"hamlib-model", 0));

	/* Hamlib configuration string */
	Hamlib.SetHamlibConf(s.Get("Hamlib", "hamlib-config"));

	/* Enable DRM modified receiver flag */
	Hamlib.SetEnableModRigSettings(s.Get("Hamlib", "enmodrig", FALSE));

	/* Enable s-meter flag */
	bEnableSMeter = s.Get("Hamlib", "ensmeter", FALSE);

#endif

	/* Front-end - combine into Hamlib? */
	CFrontEndParameters& FrontEndParameters = pReceiverParam->FrontEndParameters;

	FrontEndParameters.eSMeterCorrectionType =
		CFrontEndParameters::ESMeterCorrectionType(s.Get("FrontEnd", "smetercorrectiontype", 0));

	FrontEndParameters.rSMeterBandwidth = s.Get("FrontEnd", "smeterbandwidth", 0.0);

	FrontEndParameters.rDefaultMeasurementBandwidth = s.Get("FrontEnd", "defaultmeasurementbandwidth", 0);

	FrontEndParameters.bAutoMeasurementBandwidth = s.Get("FrontEnd", "automeasurementbandwidth", TRUE);

	FrontEndParameters.rCalFactorDRM = s.Get("FrontEnd", "calfactordrm", 0.0);

	FrontEndParameters.rCalFactorAM = s.Get("FrontEnd", "calfactoram", 0.0);

	FrontEndParameters.rIFCentreFreq = s.Get("FrontEnd", "ifcentrefrequency", SOUNDCRD_SAMPLE_RATE / 4);

	/* Serial Number */
	string sValue = s.Get("Receiver", "serialnumber");
	if (sValue != "")
	{
		// Pad to a minimum of 6 characters
		while (sValue.length() < 6)
			sValue += "_";
	}
	pReceiverParam->sSerialNumber = sValue;
		
	pReceiverParam->GenerateReceiverID();

	/* Data files directory */
	string sDataFilesDirectory = s.Get("Receiver", "datafilesdirectory", string("./"));
	// add trailing slash if not there already
	string::iterator p = sDataFilesDirectory.end();
	if (p[-1] != '/' &&  p[-1] != '\\')
		sDataFilesDirectory.insert(p, '/');

	pReceiverParam->sDataFilesDirectory = sDataFilesDirectory;
}

void
CDRMReceiver::SaveSettings(CSettings& s)
{

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

	/* Logfile -------------------------------------------------------------- */
	/* log or nolog? */
	s.Put("Logfile", "enablelog", pReceiverParam->ReceptLog.GetLoggingEnabled());

	/* Start log file delayed */
	s.Put("Logfile", "delay", pReceiverParam->ReceptLog.GetDelLogStart());

	if(pReceiverParam->ReceptLog.GPSData.GetPositionAvailable())
	{
		double latitude, longitude;
		pReceiverParam->ReceptLog.GPSData.GetLatLongDegrees(latitude, longitude);
		s.Put("Logfile", "latitude", latitude);
		s.Put("Logfile", "longitude", longitude);
	}

	/* AM Parameters */

	/* AGC */
	s.Put("AM Demodulation", "agc", AMDemodulation.GetAGCType());

	/* noise reduction */
	s.Put("AM Demodulation", "noisered", AMDemodulation.GetNoiRedType());

	/* pll enabled/disabled */
	s.Put("AM Demodulation", "enablepll", AMDemodulation.PLLEnabled());

	/* auto frequency acquisition */
	s.Put("AM Demodulation", "autofreqacq", AMDemodulation.AutoFreqAcqEnabled());

	/* demodulation */
	s.Put("AM Demodulation", "demodulation", AMDemodulation.GetDemodType());

	s.Put("AM Demodulation", "filterbwam", iBwAM);
	s.Put("AM Demodulation", "filterbwusb", iBwUSB);
	s.Put("AM Demodulation", "filterbwlsb", iBwLSB);
	s.Put("AM Demodulation", "filterbwcw", iBwCW);
	s.Put("AM Demodulation", "filterbwfm", iBwFM);

#ifdef HAVE_LIBHAMLIB
	/* Hamlib --------------------------------------------------------------- */
	/* Hamlib Model ID */
	s.Put("Hamlib", "hamlib-model", Hamlib.GetHamlibModelID());

	/* Hamlib configuration string */
	s.Put("Hamlib", "hamlib-config", Hamlib.GetHamlibConf());

	/* Enable DRM modified receiver flag */
	s.Put("Hamlib", "enmodrig", Hamlib.GetEnableModRigSettings());

	/* Enable s-meter flag */
	s.Put("Hamlib", "ensmeter", bEnableSMeter);

#endif

	/* Front-end - combine into Hamlib? */
	s.Put("FrontEnd", "smetercorrectiontype", int(pReceiverParam->FrontEndParameters.eSMeterCorrectionType));

	s.Put("FrontEnd", "smeterbandwidth", int(pReceiverParam->FrontEndParameters.rSMeterBandwidth));

	s.Put("FrontEnd", "defaultmeasurementbandwidth", int(pReceiverParam->FrontEndParameters.rDefaultMeasurementBandwidth));

	s.Put("FrontEnd", "automeasurementbandwidth", pReceiverParam->FrontEndParameters.bAutoMeasurementBandwidth);

	s.Put("FrontEnd", "calfactordrm", int(pReceiverParam->FrontEndParameters.rCalFactorDRM));

	s.Put("FrontEnd", "calfactoram", int(pReceiverParam->FrontEndParameters.rCalFactorAM));

	s.Put("FrontEnd", "ifcentrefrequency", int(pReceiverParam->FrontEndParameters.rIFCentreFreq));

	/* Serial Number */
	s.Put("Receiver", "serialnumber", pReceiverParam->sSerialNumber);

	s.Put("Receiver", "datafilesdirectory", pReceiverParam->sDataFilesDirectory);

}
