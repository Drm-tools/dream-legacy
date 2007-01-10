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

const int CDRMReceiver::MAX_UNLOCKED_COUNT=2;

/* Implementation *************************************************************/
CDRMReceiver::CDRMReceiver(CSettings& Settings)
	:	iAcquRestartCnt(0), iGoodSignCnt(0),
		eNewReceiverMode(RM_NONE),
		ReceiveData(&SoundInInterface), WriteData(&SoundOutInterface),
		rInitResampleOffset((_REAL) 0.0), iAcquDetecCnt(0),
		vecrFreqSyncValHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrSamOffsValHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrLenIRHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrDopplerHist(LEN_HIST_PLOT_SYNC_PARMS),
		vecrSNRHist(LEN_HIST_PLOT_SYNC_PARMS),
		veciCDAudHist(LEN_HIST_PLOT_SYNC_PARMS), iAvCntParamHist(0),
		rAvLenIRHist((_REAL) 0.0), rAvDopplerHist((_REAL) 0.0),
		rAvSNRHist((_REAL) 0.0), iCurrentCDAud(0),
		UtilizeFACData(), UtilizeSDCData(), MSCDemultiplexer(),
		iAudioStreamID(STREAM_ID_NOT_USED), iDataStreamID(STREAM_ID_NOT_USED),
		upstreamRSCI(), DecodeRSIMDI(), downstreamRSCI(), RSIPacketBuf(),
		MSCDecBuf(MAX_NUM_STREAMS), MSCUseBuf(MAX_NUM_STREAMS), MSCSendBuf(MAX_NUM_STREAMS),
		ChannelEstimation(), AudioSourceDecoder(), FreqSyncAcq()
#if defined(USE_QT_GUI) || defined(_WIN32)
		, iMainPlotColorStyle(0), /* default color scheme: blue-white */
		iSecondsPreview(0), iSecondsPreviewLiveSched(0), bShowAllStations(TRUE),
		GeomChartWindows(0), bEnableSMeter(TRUE),
		iSysEvalDlgPlotType(0), strStoragePathMMDlg(""),
		strStoragePathLiveScheduleDlg(""),
		bAddRefreshHeader(TRUE),
		iMOTBWSRefreshTime(10),
		SortParamAnalog(0, TRUE), /* Sort list by station name  */
		/* Sort list by transmit power (5th column), most powerful on top */
		SortParamDRM(4, FALSE),
		SortParamLiveSched(0, FALSE), /* sort by frequency */
		iMainDisplayColor(0xff0000), /* Red */
		FontParamMMDlg("", 1, 0, FALSE),
		iBwAM(10000),
		iBwLSB(5000),
		iBwUSB(5000),
		iBwCW(150),
		iBwFM(6000),
		AMDemodType(CAMDemodulation::DT_AM)
#endif
#ifdef _WIN32
		, bProcessPriorityEnabled(TRUE)
#endif
		, bReadFromFile(FALSE), time_keeper(0)

{
	downstreamRSCI.SetReceiver(this);

	string sValue;

	/* Flip spectrum flag */
	sValue = Settings.Get("Receiver", "flipspectrum");
	if (sValue == "1")
		ReceiveData.SetFlippedSpectrum(1);

	/* read from file */
	sValue = Settings.Get("Global", "filein");
	if (sValue == "1")
	{
		/* If DRM data is read from file instead of using the sound card, the sound
		   output must be a blocking function otherwise we cannot achieve a
		   synchronized stream */
		ReceiveData.SetReadFromFile(sValue);
		WriteData.SetSoundBlocking(TRUE);
		bReadFromFile = TRUE;
	}

	/* write to wav file */
	sValue = Settings.Get("Global", "writewav");
	if (sValue == "1")
		WriteData.StartWriteWaveFile(sValue);

	/* Mute audio flag */
	sValue = Settings.Get("Receiver", "muteaudio");
	if (sValue == "1")
		WriteData.MuteAudio(1);

	/* Reverberation flag */
	sValue = Settings.Get("Receiver", "reverb");
	if (sValue == "1")
		AudioSourceDecoder.SetReverbEffect(1);

	/* Bandpass filter flag */
	sValue = Settings.Get("Receiver", "filter");
	if (sValue == "1")
		FreqSyncAcq.SetRecFilter(1);

	/* Modified metrics flag */
	sValue = Settings.Get("Receiver", "modmetric");
	if (sValue == "1")
		ChannelEstimation.SetIntCons(1);

	/* Sound In device */
	sValue = Settings.Get("Global", "snddevin");
	SoundInInterface.SetDev(atoi(sValue.c_str()));

	/* Sound Out device */
	sValue = Settings.Get("Global", "snddevout");
	SoundOutInterface.SetDev(atoi(sValue.c_str()));

	/* Number of iterations for MLC setting */
	sValue = Settings.Get("Receiver", "mlciter");
	MSCMLCDecoder.SetNumIterations(atoi(sValue.c_str()));

	/* Wanted RF Frequency file */
	sValue = Settings.Get("Receiver", "frequency");
	SetFrequency(atoi(sValue.c_str()));
	
	/* Sample rate offset start value ----------------------------------- */
	sValue = Settings.Get("Receiver", "sampleoff");
	SetInitResOff(atof(sValue.c_str()));

	/* Set parameters for frequency acquisition search window if needed */
	sValue = Settings.Get("Receiver", "fracwinsize");
	if (sValue != "")
	{
		_REAL rFreqAcSeWinCenter, rFreqAcSeWinSize;
		
		rFreqAcSeWinSize = atof(sValue.c_str());

		sValue = Settings.Get("Receiver", "fracwincent");
		if (sValue != "")
		{
			rFreqAcSeWinCenter = atof(sValue.c_str());
		}
		else
		{
			/* If no center was specified, set default parameter (in the
			   middle of the available spectrum) */
			rFreqAcSeWinCenter = (_REAL) SOUNDCRD_SAMPLE_RATE / 4;
		}

		/* Set new parameters */
		FreqSyncAcq.SetSearchWindow(rFreqAcSeWinCenter, rFreqAcSeWinSize);
	}

	/* Input channel selection ------------------------------------------ */
	sValue = Settings.Get("Receiver", "inchansel");
	if (sValue != "")
	{
		switch (sValue[0])
		{
		case '0':
			ReceiveData.SetInChanSel(CReceiveData::CS_LEFT_CHAN);
			break;

		case '1':
			ReceiveData.SetInChanSel(CReceiveData::CS_RIGHT_CHAN);
			break;

		case '2':
			ReceiveData.SetInChanSel(CReceiveData::CS_MIX_CHAN);
			break;

		case '3':
			ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS);
			break;

		case '4':
			ReceiveData.SetInChanSel(CReceiveData::CS_IQ_NEG);
			break;

		case '5':
			ReceiveData.SetInChanSel(CReceiveData::CS_IQ_POS_ZERO);
			break;

		case '6':
			ReceiveData.SetInChanSel(CReceiveData::CS_IQ_NEG_ZERO);
			break;
		}
	}

	/* Output channel selection ----------------------------------------- */
	sValue = Settings.Get("Receiver", "outchansel");
	if (sValue != "")
	{
		switch (sValue[0])
		{
		case '0':
			WriteData.SetOutChanSel(CWriteData::CS_BOTH_BOTH);
			break;

		case '1':
			WriteData.SetOutChanSel(CWriteData::CS_LEFT_LEFT);
			break;

		case '2':
			WriteData.SetOutChanSel(CWriteData::CS_RIGHT_RIGHT);
			break;

		case '3':
			WriteData.SetOutChanSel(CWriteData::CS_LEFT_MIX);
			break;

		case '4':
			WriteData.SetOutChanSel(CWriteData::CS_RIGHT_MIX);
			break;
		}
	}


#ifdef WIN32
	/* Enable/Disable process priority flag */
	sValue = Settings.Get("Receiver", "processpriority");
	if(sValue=="0")
		bProcessPriorityEnabled = FALSE;
	if(sValue=="1")
		bProcessPriorityEnabled = TRUE;
#endif

	/* Activate/Deactivate EPG decoding */
	sValue = Settings.Get("EPG", "decodeepg");
	DataDecoder.SetDecodeEPG(atoi(sValue.c_str()));

#ifdef USE_QT_GUI
	/* Logfile -------------------------------------------------------------- */
	/* Start log file flag */
	sValue = Settings.Get("Logfile", "enablelog");
	ReceiverParam.ReceptLog.SetLoggingEnabled(atoi(sValue.c_str()));

	/* logging delay value */
	sValue = Settings.Get("Logfile", "delay");
	ReceiverParam.ReceptLog.SetDelLogStart(atoi(sValue.c_str()));

	/* Latitude string for log file */
	ReceiverParam.ReceptLog.SetLatitude(Settings.Get("Logfile", "latitude"));

	/* Longitude string for log file */
	ReceiverParam.ReceptLog.SetLongitude(Settings.Get("Logfile", "longitude"));

	/* Storage path for files saved from Multimedia dialog */
	strStoragePathMMDlg = Settings.Get("Multimedia dialog", "storagepath");

	/* MOT BWS refresh time for pages saved from Multimedia dialog */
	sValue = Settings.Get("Multimedia dialog", "motbwsrefresh");
	iMOTBWSRefreshTime = atoi(sValue.c_str());

	/* MOT BWS add refresh header for pages saved from Multimedia dialog */
	sValue = Settings.Get("Multimedia dialog", "addrefresh");
	bAddRefreshHeader = atoi(sValue.c_str());

	/* Store font saved from Multimedia dialog */
	FontParamMMDlg.strFamily = Settings.Get("Multimedia dialog", "fontfamily");

	sValue = Settings.Get("Multimedia dialog", "fontpointsize");
	FontParamMMDlg.intPointSize = atoi(sValue.c_str());

	sValue = Settings.Get("Multimedia dialog", "fontweight");
	FontParamMMDlg.intWeight = atoi(sValue.c_str());

	sValue = Settings.Get("Multimedia dialog", "fontitalic");
	FontParamMMDlg.bItalic = (sValue=="1")?TRUE:FALSE;

	/* Seconds for preview into Stations Dialog if zero then inactive */
	sValue = Settings.Get("Stations dialog", "preview");
	iSecondsPreview = atoi(sValue.c_str());

	/* Sort order and column for DRM schedule and analog schedule */
	sValue = Settings.Get("Stations dialog", "sortcolumndrm");
	SortParamDRM.iColumn = atoi(sValue.c_str());
	sValue = Settings.Get("Stations dialog", "sortcolumnanalog");
	SortParamAnalog.iColumn = atoi(sValue.c_str());

	sValue = Settings.Get("Stations dialog", "sortascendingdrm");
	SortParamDRM.bAscending = (sValue=="1")?TRUE:FALSE;
	sValue = Settings.Get("Stations dialog", "sortascendinganalog");
	SortParamAnalog.bAscending = (sValue=="1")?TRUE:FALSE;


	/* Window geometry ------------------------------------------------------ */
	/* Main window */
	sValue = Settings.Get("Window geometry", "mainxpos");
	GeomFdrmdialog.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "mainypos");
	GeomFdrmdialog.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "mainhsize");
	GeomFdrmdialog.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "mainwsize");
	GeomFdrmdialog.iWSize = atoi(sValue.c_str());

	/* System evaluation window */
	sValue = Settings.Get("Window geometry", "sysevxpos");
	GeomSystemEvalDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "sysevypos");
	GeomSystemEvalDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "sysevhsize");
	GeomSystemEvalDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "sysevwsize");
	GeomSystemEvalDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "sysevvis");
	GeomSystemEvalDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* Multimedia window */
	sValue = Settings.Get("Window geometry", "multdlgxpos");
	GeomMultimediaDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "multdlgypos");
	GeomMultimediaDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "multdlghsize");
	GeomMultimediaDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "multdlgwsize");
	GeomMultimediaDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "multdlgvis");
	GeomMultimediaDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* Stations dialog */
	sValue = Settings.Get("Window geometry", "statdlgxpos");
	GeomStationsDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "statdlgypos");
	GeomStationsDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "statdlghsize");
	GeomStationsDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "statdlgwsize");
	GeomStationsDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "statdlgvis");
	GeomStationsDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* EPG dialog */
	sValue = Settings.Get("Window geometry", "epgdlgxpos");
	GeomEPGDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "epgdlgypos");
	GeomEPGDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "epgdlghsize");
	GeomEPGDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "epgdlgwsize");
	GeomEPGDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "epgdlgvis");
	GeomEPGDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* Live schedule dialog */
	sValue = Settings.Get("Window geometry", "scheddlgxpos");
	GeomLiveScheduleDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "scheddlgypos");
	GeomLiveScheduleDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "scheddlghsize");
	GeomLiveScheduleDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "scheddlgwsize");
	GeomLiveScheduleDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "scheddlgvis");
	GeomLiveScheduleDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* Sort order and column for schedule */
	sValue = Settings.Get("Live schedule dialog", "sortcolumn");
	SortParamLiveSched.iColumn = atoi(sValue.c_str());
	sValue = Settings.Get("Live schedule dialog", "sortascending");
	SortParamLiveSched.bAscending = (sValue=="1")?TRUE:FALSE;

	/* Seconds for preview into live schedule dialog if zero then inactive */
	sValue = Settings.Get("Live schedule dialog", "preview");
	iSecondsPreviewLiveSched = atoi(sValue.c_str());

	sValue = Settings.Get("Live schedule dialog", "showall");
	bShowAllStations = (sValue=="1")?TRUE:FALSE;

	/* Storage path for files saved from live schedule dialog */
	strStoragePathLiveScheduleDlg = Settings.Get("Live schedule dialog", "storagepath");

	/* Analog demodulation dialog */
	sValue = Settings.Get("Window geometry", "analdemxpos");
	GeomAnalogDemDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "analdemypos");
	GeomAnalogDemDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "analdemhsize");
	GeomAnalogDemDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "analdemwsize");
	GeomAnalogDemDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "analdemvis");
	GeomAnalogDemDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* filter bandwidth (max = SOUNDCRD_SAMPLE_RATE / 2 = typically 24000 Hz = 24 kHz) */

	/* AM filter bandwidth */
	sValue = Settings.Get("Analog demodulation dialog", "filterbwam");
	iBwAM = atoi(sValue.c_str());

	/* USB filter bandwidth */
	sValue = Settings.Get("Analog demodulation dialog", "filterbwusb");
	iBwUSB = atoi(sValue.c_str());

	/* LSB filter bandwidth */
	sValue = Settings.Get("Analog demodulation dialog", "filterbwlsb");
	iBwLSB = atoi(sValue.c_str());

	/* CW filter bandwidth */
	sValue = Settings.Get("Analog demodulation dialog", "filterbwcw");
	iBwCW = atoi(sValue.c_str());

	/* FM filter bandwidth */
	sValue = Settings.Get("Analog demodulation dialog", "filterbwfm");
	iBwFM = atoi(sValue.c_str());
	
	/* demodulation */
	sValue = Settings.Get("Analog demodulation dialog", "demodulation");
	AMDemodType = (CAMDemodulation::EDemodType) atoi(sValue.c_str());

	/* AGC */
	sValue = Settings.Get("Analog demodulation dialog", "agc");
	AMDemodulation.SetAGCType((CAGC::EType) atoi(sValue.c_str()));

	/* noise reduction */
	sValue = Settings.Get("Analog demodulation dialog", "noisered");
	AMDemodulation.SetNoiRedType((CAMDemodulation::ENoiRedType) atoi(sValue.c_str()));

	/* pll enabled/disabled */
	sValue = Settings.Get("Analog demodulation dialog", "enablepll");
	AMDemodulation.EnablePLL((sValue=="1")?TRUE:FALSE);

	/* auto frequency acquisition */
	sValue = Settings.Get("Analog demodulation dialog", "autofreqacq");
	AMDemodulation.EnableAutoFreqAcq((sValue=="1")?TRUE:FALSE);


	/* AMSS dialog */
	sValue = Settings.Get("Window geometry", "amssxpos");
	GeomAMSSDlg.iXPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "amssypos");
	GeomAMSSDlg.iYPos = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "amsshsize");
	GeomAMSSDlg.iHSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "amsswsize");
	GeomAMSSDlg.iWSize = atoi(sValue.c_str());
	sValue = Settings.Get("Window geometry", "amssvis");
	GeomAMSSDlg.bVisible = (sValue=="1")?TRUE:FALSE;

	/* Chart windows */
	int iNumChartWin = 0;
	sValue = Settings.Get("Window geometry", "numchartwin");
	iNumChartWin = atoi(sValue.c_str());

	GeomChartWindows.Init(iNumChartWin);
	for (int i = 0; i < iNumChartWin; i++)
	{
		/* Convert number to string */
		char chNumTmpLong[256];

		sprintf(chNumTmpLong, "chwin%dxpos", i);
		sValue = Settings.Get("Window geometry", chNumTmpLong);
		GeomChartWindows[i].iXPos = atoi(sValue.c_str());

		sprintf(chNumTmpLong, "chwin%dypos", i);
		sValue = Settings.Get("Window geometry", chNumTmpLong);
		GeomChartWindows[i].iYPos = atoi(sValue.c_str());

		sprintf(chNumTmpLong, "chwin%dhsize", i);
		sValue = Settings.Get("Window geometry", chNumTmpLong);
		GeomChartWindows[i].iHSize = atoi(sValue.c_str());

		sprintf(chNumTmpLong, "chwin%dwsize", i);
		sValue = Settings.Get("Window geometry", chNumTmpLong);
			GeomChartWindows[i].iWSize = atoi(sValue.c_str());

		sprintf(chNumTmpLong, "chwin%dtype", i);
		sValue = Settings.Get("Window geometry", chNumTmpLong);
		GeomChartWindows[i].iType = atoi(sValue.c_str());
	}


	/* Other ---------------------------------------------------------------- */
	/* Color scheme main plot */
	sValue = Settings.Get("GUI", "colorscheme");
	iMainPlotColorStyle = atoi(sValue.c_str());

	/* System evaluation dialog plot type. Maximum value is the last element
	   in the plot type enum! */
	sValue = Settings.Get("GUI", "sysevplottype");
	iSysEvalDlgPlotType = atoi(sValue.c_str());

	/* Main window display color */
	sValue = Settings.Get("GUI", "maindispcolor");
	iMainDisplayColor = atoi(sValue.c_str());
#endif

	sValue = Settings.Get("Receiver", "rsioutprofile");
	if (sValue != "")
		downstreamRSCI.SetProfile(sValue[0]);

	sValue = Settings.Get("Receiver", "rsiout");
	if (sValue != "")
		downstreamRSCI.SetOutAddr(sValue);

	sValue = Settings.Get("Receiver", "rsiin");
	if (sValue != "")
		downstreamRSCI.SetInAddr(sValue);

	sValue = Settings.Get("Receiver", "rciout");
	if (sValue != "")
		upstreamRSCI.SetOutAddr(sValue);

	sValue = Settings.Get("Receiver", "rciin");
	if (sValue != "")
		downstreamRSCI.SetInAddr(sValue);

#ifdef HAVE_LIBHAMLIB
	/* Hamlib --------------------------------------------------------------- */
	/* Hamlib Model ID */
	sValue = Settings.Get("Hamlib", "hamlib-model");
	Hamlib.SetHamlibModelID(atoi(sValue.c_str()));

	/* Hamlib configuration string */
	Hamlib.SetHamlibConf(Settings.Get("Hamlib", "hamlib-config"));

# ifdef USE_QT_GUI
	/* Enable s-meter flag */
	sValue = Settings.Get("Hamlib", "ensmeter");
	bEnableSMeter = (sValue=="1")?TRUE:FALSE;
# endif

	/* Enable DRM modified receiver flag */
	sValue = Settings.Get("Hamlib", "enmodrig");
	Hamlib.SetEnableModRigSettings((sValue=="1")?TRUE:FALSE);
#endif
}

void CDRMReceiver::Run(_BOOLEAN bRunOnce)
{
	_BOOLEAN bEnoughData = TRUE;
	_BOOLEAN bFrameToSend=FALSE;

		/* Check for parameter changes from GUI thread ---------------------- */
		/* The parameter changes are done through flags, the actual
		   initialization is done in this (the working) thread to avoid
		   problems with shared data */
		if (eNewReceiverMode != RM_NONE)
			InitReceiverMode();

		/* Receive data ----------------------------------------------------- */

		if ((upstreamRSCI.GetInEnabled() == TRUE) && (bRunOnce == FALSE)) /* don't wait for a packet in Init mode */
		{
			RSIPacketBuf.Clear();
			upstreamRSCI.ReadData(ReceiverParam, RSIPacketBuf);
			if(RSIPacketBuf.GetFillLevel()>0)
			{
				time_keeper = time(NULL);
				DecodeRSIMDI.ProcessData(ReceiverParam, RSIPacketBuf, FACDecBuf, SDCDecBuf, MSCDecBuf);
				CheckInitsNeeded();
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
			ReceiveData.ReadData(ReceiverParam, RecDataBuf);

		while (bEnoughData && ReceiverParam.bRunThread)
		{
			/* Init flag */
			bEnoughData = FALSE;

			if ((ReceiverParam.GetReceiverMode() == RM_AM) || bRunOnce)
			{
				/* The incoming samples are split 2 ways using a new CSplit
				   class. One set are passed to the existing AM demodulator.
				   The other set are passed to the new AMSS demodulator. The
				   AMSS and AM demodulators work completely independently */
				if (Split.ProcessData(ReceiverParam, RecDataBuf, AMDataBuf,
					AMSSDataBuf))
				{
					bEnoughData = TRUE;
				}


				/* AM demodulation ------------------------------------------ */
				if (AMDemodulation.ProcessData(ReceiverParam, AMDataBuf,
					AudSoDecBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				/* Play and/or save the audio */
				if (WriteData.WriteData(ReceiverParam, AudSoDecBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				/* AMSS phase demodulation */
				if (AMSSPhaseDemod.ProcessData(ReceiverParam, AMSSDataBuf,
					AMSSPhaseBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				
				/* AMSS resampling */
				if (InputResample.ProcessData(ReceiverParam, AMSSPhaseBuf,
					AMSSResPhaseBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				/* AMSS bit extraction */
				if (AMSSExtractBits.ProcessData(ReceiverParam, AMSSResPhaseBuf,
					AMSSBitsBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				/* AMSS data decoding */
				if (AMSSDecode.ProcessData(ReceiverParam, AMSSBitsBuf,
					SDCDecBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				if (UtilizeSDCData.WriteData(ReceiverParam, SDCDecBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}
			}

			if ((ReceiverParam.GetReceiverMode() == RM_DRM) || bRunOnce)
			{

				/* Demodulator */

				if (upstreamRSCI.GetInEnabled() == FALSE)
				{

					/* Resample input DRM-stream -------------------------------- */
					if (InputResample.ProcessData(ReceiverParam, RecDataBuf,
						InpResBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* Frequency synchronization acquisition -------------------- */
					if (FreqSyncAcq.ProcessData(ReceiverParam, InpResBuf,
						FreqSyncAcqBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* Time synchronization ------------------------------------- */
					if (TimeSync.ProcessData(ReceiverParam, FreqSyncAcqBuf,
						TimeSyncBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;

						/* Use count of OFDM-symbols for detecting aquisition
						   state */
						DetectAcquiSymbol();
					}

					/* OFDM-demodulation ---------------------------------------- */
					if (OFDMDemodulation.ProcessData(ReceiverParam, TimeSyncBuf,
						OFDMDemodBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* Synchronization in the frequency domain (using pilots) --- */
					if (SyncUsingPil.ProcessData(ReceiverParam, OFDMDemodBuf,
						SyncUsingPilBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* Channel estimation and equalisation ---------------------- */
					if (ChannelEstimation.ProcessData(ReceiverParam,
						SyncUsingPilBuf, ChanEstBuf))
					{
						CheckInitsNeeded();
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
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* FAC ------------------------------------------------------ */
					if (FACMLCDecoder.ProcessData(ReceiverParam, FACCarDemapBuf,
						FACDecBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
						SplitFAC.ProcessData(ReceiverParam, FACDecBuf, FACUseBuf, FACSendBuf);
						bFrameToSend=TRUE;
					}

					/* SDC ------------------------------------------------------ */
					if (SDCMLCDecoder.ProcessData(ReceiverParam, SDCCarDemapBuf,
						SDCDecBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
						SplitSDC.ProcessData(ReceiverParam, SDCDecBuf, SDCUseBuf, SDCSendBuf);
					}

					/* MSC ------------------------------------------------------ */
					/* Symbol de-interleaver */
					if (SymbDeinterleaver.ProcessData(ReceiverParam, MSCCarDemapBuf,
						DeintlBuf))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

					/* MLC decoder */
					if (MSCMLCDecoder.ProcessData(ReceiverParam, DeintlBuf,
						MSCMLCDecBuf))
					{
						CheckInitsNeeded();
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
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}

				}

				/* Decoder, common for local and RSI Input */
				if (UtilizeFACData.WriteData(ReceiverParam, FACUseBuf))
				{
					CheckInitsNeeded();
					bEnoughData = TRUE;
					bFrameToSend=TRUE;

					/* Use information of FAC CRC for detecting the acquisition
					   requirement */
					DetectAcquiFAC();
					CheckInitsNeeded();

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
					CheckInitsNeeded();
					bEnoughData = TRUE;
				}

				/* Data decoding */
				if(iDataStreamID != STREAM_ID_NOT_USED)
				{
					if (DataDecoder.WriteData(ReceiverParam, MSCUseBuf[iDataStreamID]))
					{
						CheckInitsNeeded();
						bEnoughData = TRUE;
					}
				}
                /* Source decoding (audio) */
				if(iAudioStreamID != STREAM_ID_NOT_USED)
				{
					if (AudioSourceDecoder.ProcessData(ReceiverParam,
						MSCUseBuf[iAudioStreamID], AudSoDecBuf))
					{
						CheckInitsNeeded();
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
			if(bRunOnce)
				return;
		}
		if(downstreamRSCI.GetOutEnabled())
		{
			if (ReceiverParam.GetReceiverMode() == RM_DRM)
			{
				if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
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
				downstreamRSCI.SendAMFrame(ReceiverParam);
			}
		}
}

void CDRMReceiver::DetectAcquiSymbol()
{
	/* Only for aquisition detection if no signal was decoded before */
	if (ReceiverParam.eAcquiState == AS_NO_SIGNAL)
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
		if ((ReceiverParam.eAcquiState == AS_WITH_SIGNAL) &&
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
			ReceiverParam.eAcquiState = AS_WITH_SIGNAL;

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
	/* Set flags so that we have only one loop in the Run() routine which is
	   enough for initializing all modules */
	ReceiverParam.bRunThread = TRUE;

	/* Set init flags in all modules */
	InitsForAllModules();

	/* Now the actual initialization */
	/* Reset all parameters to start parameter settings */
	SetInStartMode();
	iUnlockedCount = MAX_UNLOCKED_COUNT;
	/* Run once */
	Run(TRUE);

	/* Reset flags */
	ReceiverParam.bRunThread = FALSE;
}

void CDRMReceiver::InitReceiverMode()
{
	ReceiverParam.SetReceiverMode(eNewReceiverMode);

	/* Init all modules */
	SetInStartMode();

	if (ReceiverParam.GetReceiverMode() == RM_AM)
	{
		/* Tell the SDC decoder that it's AMSS to decode (no AFS index) */
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_AMSS);
	}
	else
		UtilizeSDCData.GetSDCReceive()->SetSDCType(CSDCReceive::SDC_DRM);

	/* Reset new mode flag */
	eNewReceiverMode = RM_NONE;
}

#ifdef USE_QT_GUI
void CDRMReceiver::run()
{
	/* Set thread priority (the working thread should have a higher priority
	   than the GUI) */
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
		Run(FALSE); /* run continuously */

	} while (ReceiverParam.bRunThread);

	if( (upstreamRSCI.GetInEnabled() == FALSE) && (bReadFromFile == FALSE) )
		SoundInInterface.Close();
	SoundOutInterface.Close();
}

void CDRMReceiver::Stop()
{
	ReceiverParam.bRunThread = FALSE;
}

void CDRMReceiver::SetAMDemodAcq(_REAL rNewNorCen)
{
	/* Set the frequency where the AM demodulation should look for the
	   aquisition. Receiver must be in AM demodulation mode */
	if (ReceiverParam.GetReceiverMode() == RM_AM)
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
		ReceiverParam.eAcquiState = AS_WITH_SIGNAL;

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
	Param.SetMSCCodingScheme(CS_3_SM);
	Param.SetSDCCodingScheme(CS_2_SM);

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
	}
	{
		ReceiverParam.bMeasureDelay = FALSE;
		ReceiverParam.bMeasureDoppler = FALSE;
		ReceiverParam.bMeasureInterference = FALSE;
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
	ReceiveData.SetInitFlag();
	InputResample.SetInitFlag();
	FreqSyncAcq.SetInitFlag();
	Split.SetInitFlag();
	AMDemodulation.SetInitFlag();
	
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
		if (ReceiverParam.eAcquiState == AS_WITH_SIGNAL)
		{
			ReceiverParam.ReceptLog.SetDelay(ReceiverParam.rMinDelay);
			ReceiverParam.ReceptLog.SetDelay(ReceiverParam.rSigmaEstimate);
		}
		else
		{
			ReceiverParam.ReceptLog.SetDelay(0.0);
			ReceiverParam.ReceptLog.SetDelay(0.0);
		}
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
	if (upstreamRSCI.GetOutEnabled() == TRUE)
	{
		return upstreamRSCI.SetFrequency(iNewFreqkHz);
	}
	else
	{
 		ReceiverParam.ReceptLog.SetFrequency(iNewFreqkHz);
#ifdef HAVE_LIBHAMLIB
		return Hamlib.SetFrequency(iNewFreqkHz);
#else
		return FALSE;
#endif
	}
}

_BOOLEAN CDRMReceiver::GetSignalStrength(_REAL& rSigStr)
{
	if (upstreamRSCI.GetInEnabled() == TRUE)
	{
	}
	else
	{
#ifdef HAVE_LIBHAMLIB
		return Hamlib.GetSMeter(rSigStr)==CHamlib::SS_VALID;
#else
		return FALSE;
#endif
	}
}

void CDRMReceiver::save(CSettings& Settings)
{
	/* Flip spectrum flag */
	Settings.Put("Receiver", "flipspectrum",	ReceiveData.GetFlippedSpectrum());

	/* Mute audio flag */
	Settings.Put("Receiver", "muteaudio", WriteData.GetMuteAudio());

	/* Reverberation */
	Settings.Put("Receiver", "reverb", AudioSourceDecoder.GetReverbEffect());

	/* Bandpass filter flag */
	Settings.Put("Receiver", "filter", FreqSyncAcq.GetRecFilter());

	/* Modified metrics flag */
	Settings.Put("Receiver", "modmetric", ChannelEstimation.GetIntCons());

	/* Sound In device */
	Settings.Put("Global", "snddevin", SoundInInterface.GetDev());

	/* Sound Out device */
	Settings.Put("Global", "snddevout", SoundOutInterface.GetDev());

	/* Number of iterations for MLC setting */
	Settings.Put("Receiver", "mlciter", MSCMLCDecoder.GetInitNumIterations());

	/* Active/Deactivate EPG decoding */
	Settings.Put("EPG", "decodeepg", DataDecoder.GetDecodeEPG());

#ifdef USE_QT_GUI
	/* Logfile -------------------------------------------------------------- */
	/* log or nolog? */
	Settings.Put("Logfile", "enablelog", ReceiverParam.ReceptLog.GetLoggingEnabled());

	/* Start log file delayed */
	Settings.Put("Logfile", "delay", ReceiverParam.ReceptLog.GetDelLogStart());

	/* Frequency for log file */
	Settings.Put("Logfile", "frequency", ReceiverParam.ReceptLog.GetFrequency());

#ifdef WIN32
	/* Enable/Disable process priority flag */
	Settings.Put("Receiver", "processpriority", bProcessPriorityEnabled);
#endif

	/* Latitude string for log file */
	Settings.Put("Logfile", "latitude", ReceiverParam.ReceptLog.GetLatitude());

	/* Longitude string for log file */
	Settings.Put("Logfile", "longitude", ReceiverParam.ReceptLog.GetLongitude());

	/* Storage path for files saved from Multimedia dialog */
	Settings.Put("Multimedia dialog", "storagepath", strStoragePathMMDlg);

	/* MOT BWS refresh time for pages saved from Multimedia dialog */
	Settings.Put("Multimedia dialog", "motbwsrefresh", iMOTBWSRefreshTime);

	/* MOT BWS add refresh header for pages saved from Multimedia dialog */
	Settings.Put("Multimedia dialog", "addrefresh", bAddRefreshHeader);

	/* Store font saved from Multimedia dialog */
	Settings.Put("Multimedia dialog", "fontfamily", FontParamMMDlg.strFamily);

	Settings.Put("Multimedia dialog", "fontpointsize", FontParamMMDlg.intPointSize);

	Settings.Put("Multimedia dialog", "fontweight", FontParamMMDlg.intWeight);

	Settings.Put("Multimedia dialog", "fontitalic", FontParamMMDlg.bItalic);

	/* Seconds for preview into Stations Dialog if zero then inactive */
	Settings.Put("Stations dialog", "preview", iSecondsPreview);

	/* Sort order and column for DRM schedule and analog schedule */
	Settings.Put("Stations dialog", "sortcolumndrm", SortParamDRM.iColumn);
	Settings.Put("Stations dialog", "sortcolumnanalog", SortParamAnalog.iColumn);
	Settings.Put("Stations dialog", "sortascendingdrm", SortParamDRM.bAscending);
	Settings.Put("Stations dialog", "sortascendinganalog", SortParamAnalog.bAscending);

	/* Window geometry ------------------------------------------------------ */
	/* Main window */
	Settings.Put("Window geometry", "mainxpos", GeomFdrmdialog.iXPos);
	Settings.Put("Window geometry", "mainypos", GeomFdrmdialog.iYPos);
	Settings.Put("Window geometry", "mainhsize", GeomFdrmdialog.iHSize);
	Settings.Put("Window geometry", "mainwsize", GeomFdrmdialog.iWSize);

	/* System evaluation window */
	Settings.Put("Window geometry", "sysevxpos", GeomSystemEvalDlg.iXPos);
	Settings.Put("Window geometry", "sysevypos", GeomSystemEvalDlg.iYPos);
	Settings.Put("Window geometry", "sysevhsize", GeomSystemEvalDlg.iHSize);
	Settings.Put("Window geometry", "sysevwsize", GeomSystemEvalDlg.iWSize);
	Settings.Put("Window geometry", "sysevvis", GeomSystemEvalDlg.bVisible);

	/* Multimedia window */
	Settings.Put("Window geometry", "multdlgxpos", GeomMultimediaDlg.iXPos);
	Settings.Put("Window geometry", "multdlgypos", GeomMultimediaDlg.iYPos);
	Settings.Put("Window geometry", "multdlghsize", GeomMultimediaDlg.iHSize);
	Settings.Put("Window geometry", "multdlgwsize", GeomMultimediaDlg.iWSize);
	Settings.Put("Window geometry", "multdlgvis", GeomMultimediaDlg.bVisible);

	/* Stations dialog */
	Settings.Put("Window geometry", "statdlgxpos", GeomStationsDlg.iXPos);
	Settings.Put("Window geometry", "statdlgypos", GeomStationsDlg.iYPos);
	Settings.Put("Window geometry", "statdlghsize", GeomStationsDlg.iHSize);
	Settings.Put("Window geometry", "statdlgwsize", GeomStationsDlg.iWSize);
	Settings.Put("Window geometry", "statdlgvis", GeomStationsDlg.bVisible);

	/* EPG dialog */
	Settings.Put("Window geometry", "epgdlgxpos", GeomEPGDlg.iXPos);
	Settings.Put("Window geometry", "epgdlgypos", GeomEPGDlg.iYPos);
	Settings.Put("Window geometry", "epgdlghsize", GeomEPGDlg.iHSize);
	Settings.Put("Window geometry", "epgdlgwsize", GeomEPGDlg.iWSize);
	Settings.Put("Window geometry", "epgdlgvis", GeomEPGDlg.bVisible);

	/* Live schedule dialog */
	Settings.Put("Window geometry", "scheddlgxpos", GeomLiveScheduleDlg.iXPos);
	Settings.Put("Window geometry", "scheddlgypos", GeomLiveScheduleDlg.iYPos);
	Settings.Put("Window geometry", "scheddlghsize", GeomLiveScheduleDlg.iHSize);
	Settings.Put("Window geometry", "scheddlgwsize", GeomLiveScheduleDlg.iWSize);
	Settings.Put("Window geometry", "scheddlgvis", GeomLiveScheduleDlg.bVisible);

	/* Sort order and column for schedule */
	Settings.Put("Live schedule dialog", "sortcolumn", SortParamLiveSched.iColumn);
	Settings.Put("Live schedule dialog", "sortascending", SortParamLiveSched.bAscending);

	/* Seconds for preview into live schedule dialog if zero then inactive */
	Settings.Put("Live schedule dialog", "preview", iSecondsPreviewLiveSched);

	Settings.Put("Live schedule dialog", "showall", bShowAllStations);

	/* Storage path for files saved from live schedule dialog */
	Settings.Put("Live schedule dialog", "storagepath", strStoragePathLiveScheduleDlg);

	/* Analog demodulation dialog */
	Settings.Put("Window geometry", "analdemxpos", GeomAnalogDemDlg.iXPos);
	Settings.Put("Window geometry", "analdemypos", GeomAnalogDemDlg.iYPos);
	Settings.Put("Window geometry", "analdemhsize", GeomAnalogDemDlg.iHSize);
	Settings.Put("Window geometry", "analdemwsize", GeomAnalogDemDlg.iWSize);
	Settings.Put("Window geometry", "analdemvis", GeomAnalogDemDlg.bVisible);

	/* filter bandwidth */
	
	/* AM filter bandwidth */
	Settings.Put("Analog demodulation dialog", "filterbwam", iBwAM);

	/* USB filter bandwidth */
	Settings.Put("Analog demodulation dialog", "filterbwusb", iBwUSB);

	/* LSB filter bandwidth */
	Settings.Put("Analog demodulation dialog", "filterbwlsb", iBwLSB);

	/* CW filter bandwidth */
	Settings.Put("Analog demodulation dialog", "filterbwcw", iBwCW);

	/* FM filter bandwidth */
	Settings.Put("Analog demodulation dialog", "filterbwfm", iBwFM);

	/* demodulation */
	Settings.Put("Analog demodulation dialog", "demodulation",
		AMDemodulation.GetDemodType());

	/* AGC */
	Settings.Put("Analog demodulation dialog", "agc", AMDemodulation.GetAGCType());

	/* noise reduction */
	Settings.Put("Analog demodulation dialog", "noisered",
		AMDemodulation.GetNoiRedType());

	/* pll enabled/disabled */
	Settings.Put("Analog demodulation dialog", "enablepll",
		AMDemodulation.PLLEnabled());

	/* auto frequency acquisition */
	Settings.Put("Analog demodulation dialog", "autofreqacq",
		AMDemodulation.AutoFreqAcqEnabled());

	/* AMSS dialog */
	Settings.Put("Window geometry", "amssxpos", GeomAMSSDlg.iXPos);
	Settings.Put("Window geometry", "amssypos", GeomAMSSDlg.iYPos);
	Settings.Put("Window geometry", "amsshsize", GeomAMSSDlg.iHSize);
	Settings.Put("Window geometry", "amsswsize", GeomAMSSDlg.iWSize);
	Settings.Put("Window geometry", "amssvis", GeomAMSSDlg.bVisible);

	/* Chart windows */
	const int iNumChartWin = GeomChartWindows.Size();
	Settings.Put("Window geometry", "numchartwin", iNumChartWin);

	for (int i = 0; i < iNumChartWin; i++)
	{
		/* Convert number to string */
		char chNumTmpLong[256];

		sprintf(chNumTmpLong, "chwin%dxpos", i);
		Settings.Put("Window geometry", chNumTmpLong, GeomChartWindows[i].iXPos);

		sprintf(chNumTmpLong, "chwin%dypos", i);
		Settings.Put("Window geometry", chNumTmpLong, GeomChartWindows[i].iYPos);

		sprintf(chNumTmpLong, "chwin%dhsize", i);
		Settings.Put("Window geometry", chNumTmpLong, GeomChartWindows[i].iHSize);

		sprintf(chNumTmpLong, "chwin%dwsize", i);
		Settings.Put("Window geometry", chNumTmpLong, GeomChartWindows[i].iWSize);

		sprintf(chNumTmpLong, "chwin%dtype", i);
		Settings.Put("Window geometry", chNumTmpLong, GeomChartWindows[i].iType);
	}


	/* Other ---------------------------------------------------------------- */
	/* Color scheme main plot */
	Settings.Put("GUI", "colorscheme", iMainPlotColorStyle);

	/* System evaluation dialog plot type */
	Settings.Put("GUI", "sysevplottype", iSysEvalDlgPlotType);

	/* Main window display color */
	Settings.Put("GUI", "maindispcolor", iMainDisplayColor);
#endif


#ifdef HAVE_LIBHAMLIB
	/* Hamlib --------------------------------------------------------------- */
	/* Hamlib Model ID */
	Settings.Put("Hamlib", "hamlib-model",
		Hamlib.GetHamlibModelID());

	/* Hamlib configuration string */
	Settings.Put("Hamlib", "hamlib-config", Hamlib.GetHamlibConf());

# ifdef USE_QT_GUI
	/* Enable s-meter flag */
	Settings.Put("Hamlib", "ensmeter", bEnableSMeter);
# endif

	/* Enable DRM modified receiver flag */
	Settings.Put("Hamlib", "enmodrig", Hamlib.GetEnableModRigSettings());
#endif

}

void CDRMReceiver::CheckInitsNeeded()
{
	if(ReceiverParam.TestInitFlag(CParameter::I_ROBUSTNESS_MODE))
	{
		InitsForWaveMode();
		ReceiverParam.ClearInitFlags(CParameter::I_ROBUSTNESS_MODE);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_SPECTRUM_OCCUPANCY))
	{
		InitsForSpectrumOccup();
		ReceiverParam.ClearInitFlags(CParameter::I_SPECTRUM_OCCUPANCY);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_INTERLEAVER))
	{
		InitsForInterlDepth();
		ReceiverParam.ClearInitFlags(CParameter::I_INTERLEAVER);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_MSC_CODE))
	{
		InitsForMSCCodSche();
		ReceiverParam.ClearInitFlags(CParameter::I_MSC_CODE);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_SDC_CODE))
	{
		InitsForSDCCodSche();
		ReceiverParam.ClearInitFlags(CParameter::I_SDC_CODE);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_SDC))
	{
		InitsForNoDecBitsSDC();
		ReceiverParam.ClearInitFlags(CParameter::I_SDC);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_MSC))
	{
		InitsForMSC();
		ReceiverParam.ClearInitFlags(CParameter::I_MSC);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_MSC_DEMUX))
	{
		InitsForMSCDemux();
		ReceiverParam.ClearInitFlags(CParameter::I_MSC_DEMUX);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_AUDIO))
	{
		InitsForAudParam();
		ReceiverParam.ClearInitFlags(CParameter::I_AUDIO);
	}
	if(ReceiverParam.TestInitFlag(CParameter::I_DATA))
	{
		InitsForDataParam();
		ReceiverParam.ClearInitFlags(CParameter::I_DATA);
	}
}
