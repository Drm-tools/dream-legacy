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
#ifdef WITH_SOUND
# ifdef _WIN32
#  include "../windows/Source/Sound.h"
# else
#  include "../linux/source/soundin.h"
#  include "../linux/source/soundout.h"
# endif
#else
# include "soundnull.h"
#endif
# include "audiofilein.h"

const int CDRMReceiver::MAX_UNLOCKED_COUNT=2;

// TODO don't create a CSoundIn if its not going to be used. It helps for now because of SetDev in the GUI
/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver() :
#ifdef WITH_SOUND
		pSoundInInterface(new CSoundIn), pSoundOutInterface(new CSoundOut),
#else
		pSoundInInterface(new CSoundInNull), pSoundOutInterface(new CSoundOutNull),
#endif
		ReceiveData(), WriteData(pSoundOutInterface),
		FreqSyncAcq(),
		ChannelEstimation(),
		UtilizeFACData(), UtilizeSDCData(), MSCDemultiplexer(),
		AudioSourceDecoder(),
		upstreamRSCI(), DecodeRSIMDI(), downstreamRSCI(),
		RSIPacketBuf(),
		MSCDecBuf(MAX_NUM_STREAMS), MSCUseBuf(MAX_NUM_STREAMS), MSCSendBuf(MAX_NUM_STREAMS),
		eAcquiState(AS_NO_SIGNAL),
		iAcquRestartCnt(0),
		iAcquDetecCnt(0),
		iGoodSignCnt(0),
		eReceiverMode(RM_DRM),	eNewReceiverMode(RM_NONE),
		iAudioStreamID(STREAM_ID_NOT_USED), iDataStreamID(STREAM_ID_NOT_USED),
		bDoInitRun(FALSE),
		rInitResampleOffset((_REAL) 0.0),
		vecrFreqSyncValHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrSamOffsValHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrLenIRHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrDopplerHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrSNRHist(LEN_HIST_PLOT_SYNC_PARMS),
		veciCDAudHist(LEN_HIST_PLOT_SYNC_PARMS), iAvCntParamHist(0),
		rAvLenIRHist((_REAL) 0.0), rAvDopplerHist((_REAL) 0.0),
		rAvSNRHist((_REAL) 0.0),
		iCurrentCDAud(0),
#ifdef USE_QT_GUI
		RigPoll(),
#endif
		iBwAM(10000),
		iBwLSB(5000),
		iBwUSB(5000),
		iBwCW(150),
		iBwFM(6000),
		AMDemodType(CAMDemodulation::DT_AM),
#ifdef _WIN32
		bProcessPriorityEnabled(TRUE),
#endif
		bReadFromFile(FALSE), time_keeper(0)
#if defined(USE_QT_GUI) || defined(_WIN32)
		, GeomChartWindows(0),
		iMainPlotColorStyle(0), /* default color scheme: blue-white */
		iSecondsPreview(0), iSecondsPreviewLiveSched(0), bShowAllStations(TRUE),
		SortParamAnalog(0, TRUE), /* Sort list by station name  */
		/* Sort list by transmit power (5th column), most powerful on top */
		SortParamDRM(4, FALSE),
		SortParamLiveSched(0, FALSE), /* sort by frequency */
		iSysEvalDlgPlotType(0),
		iMOTBWSRefreshTime(10),
		bAddRefreshHeader(TRUE),
		strStoragePathMMDlg(""),
		strStoragePathLiveScheduleDlg(""),
		iMainDisplayColor(0xff0000), /* Red */
		FontParamMMDlg("", 1, 0, FALSE)
#endif
{
	downstreamRSCI.SetReceiver(this);
#if defined(USE_QT_GUI)
	RigPoll.setReceiver(this);
# if defined(HAVE_LIBHAMLIB)
	if(Hamlib.GetHamlibModelID() != 0)
		RigPoll.start();
# endif
#endif
}

CDRMReceiver::~CDRMReceiver()
{
#if defined(USE_QT_GUI)
	if(RigPoll.running())
	{
		RigPoll.stop();
	}
	if(RigPoll.wait(1000)==FALSE)
		cout << "error terminating rig polling thread" << endl;;
#endif
	delete pSoundInInterface;
	delete pSoundOutInterface;
}

void CDRMReceiver::Run()
{
	_BOOLEAN bEnoughData = TRUE;
	_BOOLEAN bFrameToSend=FALSE;

		/* Check for parameter changes from RSCI or GUI thread --------------- */
		/* The parameter changes are done through flags, the actual
		   initialization is done in this (the working) thread to avoid
		   problems with shared data */
		if (eNewReceiverMode != RM_NONE)
			InitReceiverMode();

		/* Check for changes in front end selection */
#if defined(USE_QT_GUI) && defined(HAVE_LIBHAMLIB)
		if(Hamlib.GetHamlibModelID()==0)
		{
			if(RigPoll.running() )
				RigPoll.stop();
		}
		else
		{
			if(RigPoll.running() == FALSE)
				RigPoll.start();
		}
#endif

		/* Receive data ----------------------------------------------------- */

		if ((upstreamRSCI.GetInEnabled() == TRUE) && (bDoInitRun == FALSE)) /* don't wait for a packet in Init mode */
		{
			RSIPacketBuf.Clear();
			upstreamRSCI.ReadData(ReceiverParam, RSIPacketBuf);
			if(RSIPacketBuf.GetFillLevel()>0)
			{
				time_keeper = time(NULL);
				DecodeRSIMDI.ProcessData(ReceiverParam, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
				SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);
				/* if we have an SDC block, make a copy and keep it until the next frame is to be sent */
				if(SDCDecBuf.GetFillLevel()==ReceiverParam.iNumSDCBitsPerSFrame)
				{
					SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
				}
				else
				{/* is this needed, or will DecodeRSIMDI do this anyway? */
				 	SDCUseBuf.Clear();
				 	SDCSendBuf.Clear();
				}
				for(size_t i=0; i<MAX_NUM_STREAMS; i++)
				{
					SplitMSC[i].ProcessData(ReceiverParam, MSCDecBuf[i], MSCUseBuf[i], MSCSendBuf[i]);
				}

				bFrameToSend=TRUE;
			}
			else
			{
			 	time_t now = time(NULL);
			 	if((now - time_keeper)>2)
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

		if (upstreamRSCI.GetInEnabled() == FALSE)
		{
			ReceiveData.ReadData(ReceiverParam, RecDataBuf);
#ifndef USE_QT_GUI
			/* TODO - get the polling interval sensible */
			_BOOLEAN bValid;
			_REAL r;
			bValid = Hamlib.GetSMeter(r)==CHamlib::SS_VALID;
			ReceiverParam.SetSignalStrength(bValid, r);
#endif
		}

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
				/* The incoming samples are split 2 ways using a new CSplit
				   class. One set are passed to the existing AM demodulator.
				   The other set are passed to the new AMSS demodulator. The
				   AMSS and AM demodulators work completely independently */
				if (Split.ProcessData(ReceiverParam, DemodDataBuf, AMDataBuf,
					AMSSDataBuf))
				{
					bEnoughData = TRUE;
				}


				/* AM demodulation ------------------------------------------ */
				if (AMDemodulation.ProcessData(ReceiverParam, AMDataBuf,
					AudSoDecBuf))
				{
					bEnoughData = TRUE;
				}

				/* Play and/or save the audio */
				if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
					bEnoughData = TRUE;

				/* AMSS phase demodulation */
				if (AMSSPhaseDemod.ProcessData(ReceiverParam, AMSSDataBuf,
					AMSSPhaseBuf))
				{
					bEnoughData = TRUE;
				}

				
				/* AMSS resampling */
				if (InputResample.ProcessData(ReceiverParam, AMSSPhaseBuf,
					AMSSResPhaseBuf))
				{
					bEnoughData = TRUE;
				}

				/* AMSS bit extraction */
				if (AMSSExtractBits.ProcessData(ReceiverParam, AMSSResPhaseBuf,
					AMSSBitsBuf))
				{
					bEnoughData = TRUE;
				}

				/* AMSS data decoding */
				if (AMSSDecode.ProcessData(ReceiverParam, AMSSBitsBuf,
					SDCDecBuf))
				{
					bEnoughData = TRUE;
				}

				if (UtilizeSDCData.WriteData(ReceiverParam, SDCDecBuf))
					bEnoughData = TRUE;
			}

			if ((eReceiverMode == RM_DRM) || bDoInitRun)
			{

				/* Demodulator */

				if (upstreamRSCI.GetInEnabled() == FALSE)
				{

					/* Resample input DRM-stream -------------------------------- */
					if (InputResample.ProcessData(ReceiverParam, DemodDataBuf,
						InpResBuf))
					{
						bEnoughData = TRUE;
					}

					/* Frequency synchronization acquisition -------------------- */
					if (FreqSyncAcq.ProcessData(ReceiverParam, InpResBuf,
						FreqSyncAcqBuf))
					{
						bEnoughData = TRUE;
					}

					/* Time synchronization ------------------------------------- */
					if (TimeSync.ProcessData(ReceiverParam, FreqSyncAcqBuf,
						TimeSyncBuf))
					{
						bEnoughData = TRUE;

						/* Use count of OFDM-symbols for detecting aquisition
						   state */
						DetectAcquiSymbol();
					}

					/* OFDM-demodulation ---------------------------------------- */
					if (OFDMDemodulation.ProcessData(ReceiverParam, TimeSyncBuf,
						OFDMDemodBuf))
					{
						bEnoughData = TRUE;
					}

					/* Synchronization in the frequency domain (using pilots) --- */
					if (SyncUsingPil.ProcessData(ReceiverParam, OFDMDemodBuf,
						SyncUsingPilBuf))
					{
						bEnoughData = TRUE;
					}

					/* Channel estimation and equalisation ---------------------- */
					if (ChannelEstimation.ProcessData(ReceiverParam,
						SyncUsingPilBuf, ChanEstBuf))
					{
						bEnoughData = TRUE;

						/* If this module has finished, all synchronization units
						   have also finished their OFDM symbol based estimates.
						   Update synchronization parameters histories */
						UpdateParamHistories();
					}

					/* Demapping of the MSC, FAC, SDC and pilots off the carriers */
					if (OFDMCellDemapping.ProcessData(ReceiverParam, ChanEstBuf,
						MSCCarDemapBuf, FACCarDemapBuf, SDCCarDemapBuf))
					{
						bEnoughData = TRUE;
					}

					/* FAC ------------------------------------------------------ */
					if (FACMLCDecoder.ProcessData(ReceiverParam, FACCarDemapBuf,
						FACDecBuf))
					{
						bEnoughData = TRUE;
						SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);
						bFrameToSend=TRUE;
					}

					/* SDC ------------------------------------------------------ */
					if (SDCMLCDecoder.ProcessData(ReceiverParam, SDCCarDemapBuf,
						SDCDecBuf))
					{
						bEnoughData = TRUE;
						SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
					}

					/* MSC ------------------------------------------------------ */
					/* Symbol de-interleaver */
					if (SymbDeinterleaver.ProcessData(ReceiverParam, MSCCarDemapBuf,
						DeintlBuf))
					{
						bEnoughData = TRUE;
					}

					/* MLC decoder */
					if (MSCMLCDecoder.ProcessData(ReceiverParam, DeintlBuf,
						MSCMLCDecBuf))
					{
						bEnoughData = TRUE;
					}

					/* MSC demultiplexer (will leave FAC & SDC alone! */
					if (MSCDemultiplexer.ProcessData(ReceiverParam,
												MSCMLCDecBuf, MSCDecBuf))
					{
						for(size_t i=0; i<MSCDecBuf.size(); i++)
						{
							SplitMSC[i].ProcessData(ReceiverParam, MSCDecBuf[i],
								MSCUseBuf[i], MSCSendBuf[i]);
						}
						bEnoughData = TRUE;
					}

				}

				/* Decoder, common for local and RSI Input */
				if (UtilizeFACData.WriteData(ReceiverParam, FACUseBuf))
				{
					bEnoughData = TRUE;
					bFrameToSend=TRUE;

					/* Use information of FAC CRC for detecting the acquisition
					   requirement */
					DetectAcquiFAC();

#if 0
/* TEST store information about alternative frequency transmitted in SDC */
static FILE* pFile = fopen("test/altfreq.dat", "w");

int inum = ReceiverParam.AltFreqSign.vecAltFreq.Size();
for (int z = 0; z < inum; z++)
{
	fprintf(pFile, "sync:%d sr:", ReceiverParam.AltFreqSign.vecAltFreq[z].bIsSyncMultplx);

	for (int k = 0; k < 4; k++)
		fprintf(pFile, "%d", ReceiverParam.AltFreqSign.vecAltFreq[z].veciServRestrict[k]);
	fprintf(pFile, " fr:");

	for (int kk = 0; kk < ReceiverParam.AltFreqSign.vecAltFreq[z].veciFrequencies.Size(); kk++)
		fprintf(pFile, "%d ", ReceiverParam.AltFreqSign.vecAltFreq[z].veciFrequencies[kk]);

	fprintf(pFile, " rID:%d sID:%d   /   ", ReceiverParam.AltFreqSign.vecAltFreq[z].iRegionID,
		ReceiverParam.AltFreqSign.vecAltFreq[z].iScheduleID);
}
fprintf(pFile, "\n");
fflush(pFile);
#endif

				}

				if (UtilizeSDCData.WriteData(ReceiverParam, SDCUseBuf))
				{
					bEnoughData = TRUE;
				}

				/* Data decoding */
				if(iDataStreamID != STREAM_ID_NOT_USED)
				{
					if (DataDecoder.WriteData(ReceiverParam, MSCUseBuf[iDataStreamID]))
						bEnoughData = TRUE;
				}
                /* Source decoding (audio) */
				if(iAudioStreamID != STREAM_ID_NOT_USED)
				{
					if (AudioSourceDecoder.ProcessData(ReceiverParam,
						MSCUseBuf[iAudioStreamID], AudSoDecBuf))
					{
						bEnoughData = TRUE;

						/* Store the number of correctly decoded audio blocks for
						 *                            the history */
						iCurrentCDAud = AudioSourceDecoder.GetNumCorDecAudio();
					}
					/* Play and/or save the audio */
					if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
						bEnoughData = TRUE;
				}
			}
		}
		if(downstreamRSCI.GetOutEnabled())
		{
			if (eReceiverMode == RM_DRM)
			{
				if (eAcquiState == AS_NO_SIGNAL)
				{
						/* we will get one of these between each FAC block, and occasionally we */
						/* might get two, so don't start generating free-wheeling RSCI until we've. */
						/* had three in a row */
						if(FreqSyncAcq.GetUnlockedFrameBoundary())
							if (iUnlockedCount < MAX_UNLOCKED_COUNT)
								iUnlockedCount++;
							else
							{
								downstreamRSCI.SendUnlockedFrame(ReceiverParam);
							 }
				}
				else if(bFrameToSend)
				{
					downstreamRSCI.SendLockedFrame(ReceiverParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
					iUnlockedCount=0;
					bFrameToSend = FALSE;
				}
			}
			else
			{
				if (AMDemodulation.GetFrameBoundary())
				{
					downstreamRSCI.SendAMFrame(ReceiverParam);
				}
			}
		}
}

void CDRMReceiver::DetectAcquiSymbol()
{
	/* Only for aquisition detection if no signal was decoded before */
	if (eAcquiState == AS_NO_SIGNAL)
	{
		/* Increment symbol counter and check if bound is reached */
		iAcquDetecCnt++;

		if (iAcquDetecCnt > NUM_OFDMSYM_U_ACQ_WITHOUT)
			SetInStartMode();			
	}
}

void CDRMReceiver::DetectAcquiFAC()
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
		if ((eAcquiState == AS_WITH_SIGNAL) &&
			(iAcquRestartCnt > NUM_FAC_FRA_U_ACQ_WITH))
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
			eAcquiState = AS_WITH_SIGNAL;

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

void CDRMReceiver::Init()
{
	//CFileLogPrinter::Instantiate();

	/* Set flags so that we have only one loop in the Run() routine which is
	   enough for initializing all modues */
	bDoInitRun = TRUE;
	ReceiverParam.bRunThread = TRUE;

	/* Set init flags in all modules */
	InitsForAllModules();

	/* Now the actual initialization */
	/* Reset all parameters to start parameter settings */
	SetInStartMode();
	iUnlockedCount = MAX_UNLOCKED_COUNT;
	/* Run once */
	Run();

	/* Reset flags */
	bDoInitRun = FALSE;
	ReceiverParam.bRunThread = FALSE;

}

void CDRMReceiver::InitReceiverMode()
{
	eReceiverMode = eNewReceiverMode;

	/* Init all modules */
	SetInStartMode();

	if (eReceiverMode == RM_AM)
	{
		/* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);

		/* Set the receive status - this affects the RSI output */
		ReceiverParam.ReceiveStatus.SetTimeSyncStatus(NOT_PRESENT);
		ReceiverParam.ReceiveStatus.SetFrameSyncStatus(NOT_PRESENT);
		ReceiverParam.ReceiveStatus.SetFACStatus(NOT_PRESENT);
		ReceiverParam.ReceiveStatus.SetSDCStatus(NOT_PRESENT);
		ReceiverParam.ReceiveStatus.SetAudioStatus(NOT_PRESENT);
		ReceiverParam.ReceiveStatus.SetMOTStatus(NOT_PRESENT);
	}
	else
	{
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
void CDRMReceiver::run()
{
	/* Set thread priority (the working thread should have a higher priority
	   than tthe GUI) */
#ifdef _WIN32
	if(GetEnableProcessPriority())
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

void CDRMReceiver::Start()
{
	/* Set run flag so that the thread can work */
	ReceiverParam.bRunThread = TRUE;

	/* Reset all parameters to start parameter settings */
	SetInStartMode();

	do
	{
		Run();

	} while (ReceiverParam.bRunThread);

	if(upstreamRSCI.GetInEnabled() == FALSE)
		pSoundInInterface->Close();
	pSoundOutInterface->Close();
}

void CDRMReceiver::Stop()
{
	ReceiverParam.bRunThread = FALSE;
}

void CDRMReceiver::SetAMDemodAcq(_REAL rNewNorCen)
{
	/* Set the frequency where the AM demodulation should look for the
	   aquisition. Receiver must be in AM demodulation mode */
	if (eReceiverMode == RM_AM)
	{
		AMDemodulation.SetAcqFreq(rNewNorCen);
		AMSSPhaseDemod.SetAcqFreq(rNewNorCen);
	}
}

void CDRMReceiver::SetInStartMode()
{
	/* Load start parameters for all modules */
	StartParameters(ReceiverParam);

	/* Activate acquisition */
	FreqSyncAcq.StartAcquisition();
	TimeSync.StartAcquisition();
	ChannelEstimation.GetTimeSyncTrack()->StopTracking();
	ChannelEstimation.StartSaRaOffAcq();
	ChannelEstimation.GetTimeWiener()->StopTracking();

	SyncUsingPil.StartAcquisition();
	SyncUsingPil.StopTrackPil();

	/* Set flag that no signal is currently received */
	eAcquiState = AS_NO_SIGNAL;

	/* Set flag for receiver state */
	eReceiverState = RS_ACQUISITION;

	/* Reset counters for acquisition decision, "good signal" and delayed
	   tracking mode counter */
	iAcquRestartCnt = 0;
	iAcquDetecCnt = 0;
	iGoodSignCnt = 0;
	iDelayedTrackModeCnt = NUM_FAC_DEL_TRACK_SWITCH;

	/* Reset GUI lights */
	ReceiverParam.ReceiveStatus.SetInterfaceStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetTimeSyncStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetFrameSyncStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetFACStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetSDCStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetAudioStatus(NOT_PRESENT);
	ReceiverParam.ReceiveStatus.SetMOTStatus(NOT_PRESENT);

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
		eAcquiState = AS_WITH_SIGNAL;

		SetInTrackingMode();
	}
}

void CDRMReceiver::SetInTrackingMode()
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

void CDRMReceiver::SetInTrackingModeDelayed()
{
	/* The timing tracking must be enabled delayed because it must wait until
	   the channel estimation has initialized its estimation */
	TimeSync.StopTimingAcqu();
	ChannelEstimation.GetTimeSyncTrack()->StartTracking();
}

void CDRMReceiver::SetReadDRMFromFile(const string strNFN)
{
	delete pSoundInInterface;
	CAudioFileIn* pf = new CAudioFileIn;
	pf->SetFileName(strNFN);
	pSoundInInterface = pf;
	ReceiveData.SetSoundInterface(pSoundInInterface); // needed ?
	string ext;
	size_t p = strNFN.rfind('.');
	if(p != string::npos)
		ext = strNFN.substr(p+1);
	_BOOLEAN bIsIQ = FALSE;
	if(ext.substr(0,2) == "iq") bIsIQ = TRUE;
	if(ext.substr(0,2) == "IQ") bIsIQ = TRUE;
	if(bIsIQ)
		ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS_ZERO);
	else
		ReceiveData.SetInChanSel(CReceiveData::CS_MIX_CHAN);
	bReadFromFile = TRUE;
}

void CDRMReceiver::StartParameters(CParameter& Param)
{
/*
	Reset all parameters to starting values. This is done at the startup of the
	application and also when the S/N of the received signal is too low and
	no receiption is left -> Reset all parameters
*/

	/* Define with which parameters the receiver should try to decode the
	   signal. If we are correct with our assumptions, the receiver does not
	   need to reinitialize */
	Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

	/* Set initial MLC parameters */
	Param.SetInterleaverDepth(CParameter::SI_LONG);
	Param.SetMSCCodingScheme(CParameter::CS_3_SM);
	Param.SetSDCCodingScheme(CParameter::CS_2_SM);

	/* Select the service we want to decode. Always zero, because we do not
	   know how many services are transmitted in the signal we want to
	   decode */

// TODO: if service 0 is not used but another service is the audio service we
// have a problem. We should check as soon as we have information about services
// if service 0 is really the audio service


	/* Set the following parameters to zero states (initial states) --------- */
	Param.ResetServicesStreams();

	Param.ResetCurSelAudDatServ();

	/* Protection levels */
	Param.MSCPrLe.iPartA = 0;
	Param.MSCPrLe.iPartB = 1;
	Param.MSCPrLe.iHierarch = 0;

	/* Number of audio and data services */
	Param.iNumAudioService = 0;
	Param.iNumDataService = 0;

	/* We start with FAC ID = 0 (arbitrary) */
	Param.iFrameIDReceiv = 0;

	/* Set synchronization parameters */
	Param.rResampleOffset = rInitResampleOffset; /* Initial resample offset */
	Param.rFreqOffsetAcqui = (_REAL) 0.0;
	Param.rFreqOffsetTrack = (_REAL) 0.0;
	Param.iTimingOffsTrack = 0;

	/* Init reception log (log long) transmission parameters. TODO: better solution */
	Param.ReceptLog.ResetTransParams();

	/* Initialization of the modules */
	InitsForAllModules();
}

void CDRMReceiver::InitsForAllModules()
{
	if(downstreamRSCI.GetOutEnabled())
	{
		ReceiverParam.bMeasureDelay = TRUE;
		ReceiverParam.bMeasureDoppler = TRUE;
		ReceiverParam.bMeasureInterference = TRUE;
		ReceiverParam.bMeasurePSD = TRUE;
	}
	else
	{
		ReceiverParam.bMeasureDelay = FALSE;
		ReceiverParam.bMeasureDoppler = FALSE;
		ReceiverParam.bMeasureInterference = FALSE;
		ReceiverParam.bMeasurePSD = FALSE;
	}

	/* Set init flags */
	SplitFAC.SetInitFlag();
	SplitSDC.SetInitFlag();
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
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
}


/* -----------------------------------------------------------------------------
   Initialization routines for the modules. We have to look into the modules
   and decide on which parameters the modules depend on */
void CDRMReceiver::InitsForWaveMode()
{
	/* Reset averaging of the parameter histories (needed, e.g., because the
	   number of OFDM symbols per DRM frame might have changed) */
	iAvCntParamHist = 0;
	rAvLenIRHist = (_REAL) 0.0;
	rAvDopplerHist = (_REAL) 0.0;
	rAvSNRHist = (_REAL) 0.0;

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
	SymbDeinterleaver.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	SDCMLCDecoder.SetInitFlag(); // Because of "iNumSDCCellsPerSFrame"
}

void CDRMReceiver::InitsForSpectrumOccup()
{
	/* Set init flags */
	FreqSyncAcq.SetInitFlag(); // Because of bandpass filter
	OFDMDemodulation.SetInitFlag();
	SyncUsingPil.SetInitFlag();
	ChannelEstimation.SetInitFlag();
	OFDMCellDemapping.SetInitFlag();
	SymbDeinterleaver.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	MSCMLCDecoder.SetInitFlag(); // Because of "iNumUsefMSCCellsPerFrame"
	SDCMLCDecoder.SetInitFlag(); // Because of "iNumSDCCellsPerSFrame"
}


/* SDC ---------------------------------------------------------------------- */
void CDRMReceiver::InitsForSDCCodSche()
{
	/* Set init flags */
	SDCMLCDecoder.SetInitFlag();

#ifdef USE_DD_WIENER_FILT_TIME
	ChannelEstimation.SetInitFlag();
#endif
}

void CDRMReceiver::InitsForNoDecBitsSDC()
{
	/* Set init flag */
	SplitSDC.SetInitFlag();
	UtilizeSDCData.SetInitFlag();
}


/* MSC ---------------------------------------------------------------------- */
void CDRMReceiver::InitsForInterlDepth()
{
	/* Can be absolutely handled seperately */
	SymbDeinterleaver.SetInitFlag();
}

void CDRMReceiver::InitsForMSCCodSche()
{
	/* Set init flags */
	MSCMLCDecoder.SetInitFlag();
	MSCDemultiplexer.SetInitFlag(); // Not sure if really needed, look at code! TODO

#ifdef USE_DD_WIENER_FILT_TIME
	ChannelEstimation.SetInitFlag();
#endif
}

void CDRMReceiver::InitsForMSC()
{
	/* Set init flags */
	MSCMLCDecoder.SetInitFlag();

	InitsForMSCDemux();
}

void CDRMReceiver::InitsForMSCDemux()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
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

void CDRMReceiver::InitsForAudParam()
{
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		MSCDecBuf[i].Clear();
		MSCUseBuf[i].Clear();
		MSCSendBuf[i].Clear();
	}

	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int a = ReceiverParam.GetCurSelAudioService();
	iAudioStreamID = ReceiverParam.GetAudioParam(a).iStreamID;
	ReceiverParam.SetNumAudioDecoderBits(
		ReceiverParam.GetStreamLen(iAudioStreamID)*SIZEOF__BYTE);
	AudioSourceDecoder.SetInitFlag();
}

void CDRMReceiver::InitsForDataParam()
{
	/* Set init flags */
	DecodeRSIMDI.SetInitFlag();
	MSCDemultiplexer.SetInitFlag();
	int d = ReceiverParam.GetCurSelDataService();
	iDataStreamID = ReceiverParam.GetDataParam(d).iStreamID;
	ReceiverParam.SetNumDataDecoderBits(
		ReceiverParam.GetStreamLen(iDataStreamID)*SIZEOF__BYTE);
	DataDecoder.SetInitFlag();
}


/* Parameter histories for plot --------------------------------------------- */
void CDRMReceiver::UpdateParamHistories()
{
     /* update parameters in the Parameter object. This replaces 
     direct calls to the channelestimation class in the evaluation dialog
     negative dB values are used to indicate invalid values
      */
   	/* TODO: make this work with upstreamRSCI */

	if(upstreamRSCI.GetInEnabled())
	{
 	}
 	else
 	{
	    ReceiverParam.rSNREstimate = ChannelEstimation.GetSNREstdB();

     	/* MERs are updated in the RSCI part of ChannelEstimation */

	    ReceiverParam.rSigmaEstimate = ChannelEstimation.GetSigma();

   	 	ReceiverParam.rMinDelay = ChannelEstimation.GetMinDelay();
	}

   	/* TODO: do not use the shift register class, build a new
	   one which just increments a pointer in a buffer and put
	   the new value at the position of the pointer instead of
	   moving the total data all the time -> special care has
	   to be taken when reading out the data */

	/* Only update histories if the receiver is in tracking mode */
	if (eReceiverState == RS_TRACKING)
	{
		MutexHist.Lock(); /* MUTEX vvvvvvvvvv */

		/* Frequency offset tracking values */
		vecrFreqSyncValHist.AddEnd(
			ReceiverParam.rFreqOffsetTrack *
			SOUNDCRD_SAMPLE_RATE);

		/* Sample rate offset estimation */
		vecrSamOffsValHist.AddEnd(ReceiverParam.
			GetSampFreqEst());

		/* Signal to Noise ratio estimates */
		rAvSNRHist += ReceiverParam.rSNREstimate;

/* TODO - reconcile this with Ollies RSCI Doppler code in ChannelEstimation */
		/* Average Doppler and delay estimates */
		rAvLenIRHist += ChannelEstimation.GetDelay();
		rAvDopplerHist += ReceiverParam.rSigmaEstimate;

		/* Only evaluate Doppler and delay once in one DRM frame */
		iAvCntParamHist++;
		if (iAvCntParamHist == ReceiverParam.iNumSymPerFrame)
		{
			/* Apply averaged values to the history vectors */
			vecrLenIRHist.AddEnd(
				rAvLenIRHist / ReceiverParam.iNumSymPerFrame);
			vecrDopplerHist.AddEnd(
				rAvDopplerHist / ReceiverParam.iNumSymPerFrame);
			vecrSNRHist.AddEnd(
				rAvSNRHist / ReceiverParam.iNumSymPerFrame);

			/* At the same time, add number of correctly decoded audio blocks.
			   This number is updated once a DRM frame. Since the other
			   parameters like SNR is also updated once a DRM frame, the two
			   values are synchronized by one DRM frame */
			veciCDAudHist.AddEnd(iCurrentCDAud);

			/* Reset parameters used for averaging */
			iAvCntParamHist = 0;
			rAvLenIRHist = (_REAL) 0.0;
			rAvDopplerHist = (_REAL) 0.0;
			rAvSNRHist = (_REAL) 0.0;
		}

		MutexHist.Unlock(); /* MUTEX ^^^^^^^^^^ */
	}
}

void CDRMReceiver::GetFreqSamOffsHist(CVector<_REAL>& vecrFreqOffs,
									  CVector<_REAL>& vecrSamOffs,
									  CVector<_REAL>& vecrScale,
									  _REAL& rFreqAquVal)
{
	/* Init output vectors */
	vecrFreqOffs.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrSamOffs.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
	MutexHist.Lock();

	/* Simply copy history buffers in output buffers */
	vecrFreqOffs = vecrFreqSyncValHist;
	vecrSamOffs = vecrSamOffsValHist;

	/* Duration of OFDM symbol */
	const _REAL rTs = (CReal) (ReceiverParam.iFFTSizeN +
		ReceiverParam.iGuardSize) / SOUNDCRD_SAMPLE_RATE;

	/* Calculate time scale */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rTs;

	/* Value from frequency acquisition */
	rFreqAquVal = ReceiverParam.rFreqOffsetAcqui * SOUNDCRD_SAMPLE_RATE;

	/* Release resources */
	MutexHist.Unlock();
}

void CDRMReceiver::GetDopplerDelHist(CVector<_REAL>& vecrLenIR,
									 CVector<_REAL>& vecrDoppler,
									 CVector<_REAL>& vecrScale)
{
	/* Init output vectors */
	vecrLenIR.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrDoppler.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
	MutexHist.Lock();

	/* Simply copy history buffers in output buffers */
	vecrLenIR = vecrLenIRHist;
	vecrDoppler = vecrDopplerHist;

	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (ReceiverParam.iFFTSizeN +
		ReceiverParam.iGuardSize) / SOUNDCRD_SAMPLE_RATE *
		ReceiverParam.iNumSymPerFrame;

	/* Calculate time scale in minutes */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rDRMFrameDur / 60;

	/* Release resources */
	MutexHist.Unlock();
}

void CDRMReceiver::GetSNRHist(CVector<_REAL>& vecrSNR,
							  CVector<_REAL>& vecrCDAud,
							  CVector<_REAL>& vecrScale)
{
	/* Init output vectors */
	vecrSNR.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrCDAud.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);
	vecrScale.Init(LEN_HIST_PLOT_SYNC_PARMS, (_REAL) 0.0);

	/* Lock resources */
	MutexHist.Lock();

	/* Simply copy history buffer in output buffer */
	vecrSNR = vecrSNRHist;

	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (ReceiverParam.iFFTSizeN +
		ReceiverParam.iGuardSize) / SOUNDCRD_SAMPLE_RATE *
		ReceiverParam.iNumSymPerFrame;

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
	MutexHist.Unlock();
}

_BOOLEAN CDRMReceiver::SetFrequency(int iNewFreqkHz)
{
	if(iFreqkHz == iNewFreqkHz)
		return TRUE;
	iFreqkHz = iNewFreqkHz;

 	ReceiverParam.ReceptLog.SetFrequency(iNewFreqkHz);

	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		upstreamRSCI.SetFrequency(iNewFreqkHz);
		return TRUE;
	}
	else
	{
		/* tell the RSCI and IQ file writer that freq has changed in case it needs to start a new file */
		if (downstreamRSCI.GetOutEnabled() == TRUE)
			downstreamRSCI.NewFrequency(ReceiverParam);

		//WriteIQFile.NewFrequency(ReceiverParam);

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

_BOOLEAN CDRMReceiver::GetSignalStrength(_REAL& rSigStr)
{
	return ReceiverParam.GetSignalStrength(rSigStr);
}

#ifdef USE_QT_GUI
void
CDRMReceiver::CRigPoll::run()
{
	while(bQuit==FALSE)
	{
		if (pDrmRec->GetRSIIn()->GetInEnabled() == FALSE)
		{
#ifdef HAVE_LIBHAMLIB
			_BOOLEAN bValid;
			_REAL r;
			bValid = pDrmRec->GetHamlib()->GetSMeter(r)==CHamlib::SS_VALID;
			// Apply any correction
			if (bValid)
				r += pDrmRec->GetParameters()->rSigStrengthCorrection;

			pDrmRec->GetParameters()->SetSignalStrength(bValid, r);
#endif
		}
		msleep(400);
	}
}
#endif

void CDRMReceiver::SetRSIRecording(const _BOOLEAN bOn, const char cPro)
{
	downstreamRSCI.SetRSIRecording(ReceiverParam, bOn, cPro);
}

void CDRMReceiver::SetIQRecording(const _BOOLEAN bOn)
{
	if (bOn)
		WriteIQFile.StartRecording(ReceiverParam);
	else
		WriteIQFile.StopRecording();
}

