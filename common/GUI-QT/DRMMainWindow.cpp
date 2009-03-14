/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
 *
 * Description:
 *
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

#include "DRMMainWindow.h"
#include "ReceiverSettingsDlg.h"
#include "LiveScheduleDlg.h"
#include "StationsDlg.h"
#include "SystemEvalDlg.h"
#include "EPGDlg.h"
#include "MultSettingsDlg.h"
#include "JLViewer.h"
#include "BWSViewer.h"
#include "SlideShowViewer.h"
#include <QColorDialog>
#include <QButtonGroup>
#include <iostream>

/* Implementation *************************************************************/
DRMMainWindow::DRMMainWindow(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, Qt::WFlags f)
	:QMainWindow(parent, name, f),  Ui_DRMMainWindow(),
	Receiver(NDRMR), Settings(NSettings),
    jlViewer(NULL), bwsViewer(NULL), slideShowViewer(NULL),
	sysEvalDlg(NULL), stationsDlg(NULL), liveScheduleDlg(NULL),
	epgDlg(NULL), receiverSettingsDlg(NULL), multSettingsDlg(NULL),
	loghelper(NDRMR, NSettings), iCurSelServiceGUI(-1), iOldNoServicesGUI(0),
	Timer(), eReceiverMode(NONE), quitWanted(true)
{
    setupUi(this);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Evaluation window */
	sysEvalDlg = new SystemEvalDlg(Receiver, Settings, this, "", false, Qt::WStyle_MinMax);

	/* Stations window */
	stationsDlg = new StationsDlg(Receiver, Settings, this, "", false, Qt::WStyle_MinMax);

	/* Live Schedule window */
	liveScheduleDlg = new LiveScheduleDlg(Receiver, Settings, this, "", false, Qt::WStyle_MinMax);

	/* Programme Guide Window */
	epgDlg = new EPGDlg(Receiver, Settings, this, "", false, Qt::WStyle_MinMax);

    /* receiver settings window */
	receiverSettingsDlg = new ReceiverSettingsDlg(Receiver, Settings, this, "", true, Qt::WType_Dialog);

	multSettingsDlg = new MultSettingsDlg(Settings, this, "", true, Qt::WStyle_Dialog);

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	connect(actionDetails, SIGNAL(triggered()), sysEvalDlg, SLOT(show()));
	connect(actionStations, SIGNAL(triggered()), stationsDlg, SLOT(show()));
	connect(actionAFS, SIGNAL(triggered()), liveScheduleDlg, SLOT(show()));
	connect(actionEPG, SIGNAL(triggered()), epgDlg, SLOT(show()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));

    CSoundCardSelMenu* pSoundInMenu = new CSoundCardSelMenu(Receiver.GetSoundInInterface(), menuSound_Card_Selection);
    CSoundCardSelMenu* pSoundOutMenu = new CSoundCardSelMenu(Receiver.GetSoundOutInterface(), menuSound_Card_Selection);
    pSoundInMenu->setTitle(tr("Sound &In"));
    pSoundOutMenu->setTitle(tr("Sound &Out"));

	/* Settings menu  ------------------------------------------------------- */
	connect(actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAnalog()));
	connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnNewDRMAcquisition()));
	connect(actionSet_Display_Colour, SIGNAL(triggered()), this, SLOT(OnMenuSetDisplayColor()));
	connect(actionSettings, SIGNAL(triggered()), receiverSettingsDlg, SLOT(show()));

	/* Digi controls */
	/* Set display color */
	SetDisplayColor(CRGBConversion::int2RGB(Settings.Get("DRMGUI", "colorscheme", 0xff0000)));

	/* Reset text */
	LabelBitrate->setText("");
	LabelCodec->setText("");
	LabelStereoMono->setText("");
	LabelServiceLabel->setText("");
	LabelProgrType->setText("");
	LabelLanguage->setText("");
	LabelCountryCode->setText("");
	LabelServiceID->setText("");

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(Qt::Vertical, QwtThermo::LeftScale);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-12.5);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));

	CParameter& Parameters = *Receiver.GetParameters();

	Parameters.Lock();

	/* Enable multimedia */
	Parameters.EnableMultimedia(true);

	/* Init current selected service */
	Parameters.ResetCurSelAudDatServ();

	Parameters.Unlock();

	/* Update times for color LEDs */
	CLED_FAC->SetUpdateTime(1500);
	CLED_SDC->SetUpdateTime(1500);
	CLED_MSC->SetUpdateTime(600);

	iCurSelServiceGUI = 0;
	iOldNoServicesGUI = 0;

	PushButtonService1->setEnabled(false);
	PushButtonService2->setEnabled(false);
	PushButtonService3->setEnabled(false);
	PushButtonService4->setEnabled(false);

    QButtonGroup* serviceGroup = new QButtonGroup(this);
    serviceGroup->addButton(PushButtonService1, 0);
    serviceGroup->addButton(PushButtonService2, 1);
    serviceGroup->addButton(PushButtonService3, 2);
    serviceGroup->addButton(PushButtonService4, 3);

	/* Connect buttons */
	connect(serviceGroup, SIGNAL(buttonClicked(int)), this, SLOT(SetService(int)));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	Loghelper *ploghelper = &loghelper;

	connect(receiverSettingsDlg, SIGNAL(StartStopGPS(bool)), sysEvalDlg, SLOT(EnableGPS(bool)));
	connect(receiverSettingsDlg, SIGNAL(ShowHideGPS(bool)), sysEvalDlg, SLOT(ShowGPS(bool)));

	connect(receiverSettingsDlg, SIGNAL(StartStopLog(bool)), ploghelper, SLOT(EnableLog(bool)));
	connect(receiverSettingsDlg, SIGNAL(SetLogStartDelay(long)), ploghelper, SLOT(LogStartDel(long)));
	connect(receiverSettingsDlg, SIGNAL(LogPosition(bool)), ploghelper, SLOT(LogPosition(bool)));
	connect(receiverSettingsDlg, SIGNAL(LogSigStr(bool)), ploghelper, SLOT(LogSigStr(bool)));

	/* Disable text message label */
	TextTextMessage->setText("");
	TextTextMessage->setEnabled(false);

}

DRMMainWindow::~DRMMainWindow()
{
}

void DRMMainWindow::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
	switch(state)
	{
	case NOT_PRESENT:
		LED->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LED->SetLight(2); /* RED */
		break;

	case DATA_ERROR:
		LED->SetLight(1); /* YELLOW */
		break;

	case RX_OK:
		LED->SetLight(0); /* GREEN */
		break;
	}
}

void DRMMainWindow::OnTimer()
{
	EDemodulationType eNewReceiverMode = Receiver.GetReceiverMode();
	switch(eNewReceiverMode)
	{
	case DRM:
		{
			CParameter& Parameters = *Receiver.GetParameters();
			Parameters.Lock();

			/* Input level meter */
			ProgrInputLevel->setValue(Parameters.GetIFSignalLevel());

            int iCurSelAudioServ = Parameters.GetCurSelAudioService();
            if(Parameters.Service[iCurSelAudioServ].eAudDataFlag == SF_DATA)
                SetStatus(CLED_MSC, Parameters.ReceiveStatus.MOT.GetStatus());
            else
                SetStatus(CLED_MSC, Parameters.ReceiveStatus.Audio.GetStatus());
			SetStatus(CLED_SDC, Parameters.ReceiveStatus.SDC.GetStatus());
			SetStatus(CLED_FAC, Parameters.ReceiveStatus.FAC.GetStatus());

			Parameters.Unlock();

			/* Check if receiver does receive a signal */
			if(Receiver.GetAcquiState() == AS_WITH_SIGNAL)
				UpdateDisplay();
			else
				ClearDisplay();
		}
		break;
	case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
        quitWanted = false;
        close();
		break;
	case NONE: // wait until working thread starts operating
		break;
	}
}

void DRMMainWindow::UpdateDisplay()
{
	CParameter& Parameters = *(Receiver.GetParameters());

	Parameters.Lock();

	/* Receiver does receive a DRM signal ------------------------------- */
	/* First get current selected services */
	int iCurSelAudioServ = Parameters.GetCurSelAudioService();

	/* If the current audio service is not active or is an only data service
	   select the first audio service available */

	if (!Parameters.Service[iCurSelAudioServ].IsActive() ||
	    Parameters.Service[iCurSelAudioServ].iAudioStream == STREAM_ID_NOT_USED ||
	    Parameters.Service[iCurSelAudioServ].eAudDataFlag == SF_DATA)
	{
		int i = 0;
		bool bStop = false;

		while ((bStop == false) && (i < MAX_NUM_SERVICES))
		{
			if (Parameters.Service[i].IsActive() &&
			    Parameters.Service[i].iAudioStream != STREAM_ID_NOT_USED &&
			    Parameters.Service[i].eAudDataFlag == SF_AUDIO)
			{
				iCurSelAudioServ = i;
				bStop = true;
			}
			else
				i++;
		}
	}

	int iAudioStream = Parameters.Service[iCurSelAudioServ].iAudioStream;

	/* If selected service is audio and text message is true */
	if (
		(Parameters.Service[iCurSelAudioServ].eAudDataFlag == SF_AUDIO)
	&&	(iAudioStream != STREAM_ID_NOT_USED)
	&&	(Parameters.AudioParam[iAudioStream].bTextflag == true)
	)
	{
		/* Activate text window */
		TextTextMessage->setEnabled(true);

		/* Text message of current selected audio service (UTF-8 decoding) */
		QString textMessage = QString().fromUtf8(
            Parameters.AudioParam[iAudioStream].strTextMessage.c_str()
        );
		QString formattedMessage = "";
		for (int i = 0; i < textMessage.length(); i++)
		{
			switch (textMessage.at(i).unicode())
			{
			case 0x0A:
				/* Code 0x0A may be inserted to indicate a preferred
				   line break */
			case 0x1F:
				/* Code 0x1F (hex) may be inserted to indicate a
				   preferred word break. This code may be used to
					   display long words comprehensibly */
				formattedMessage += "<br>";
				break;

			case 0x0B:
				/* End of a headline */
				formattedMessage = "<b><u>"
                                    + formattedMessage
                                    + "</u></b></center><br><center>";
				break;

			case '<':
				formattedMessage += "&lt;";
				break;

			case '>':
				formattedMessage += "&gt;";
				break;

			default:
				formattedMessage += textMessage[int(i)];
			}
		}
		formattedMessage = "<center>" + formattedMessage + "</center>";
		TextTextMessage->setText(formattedMessage);
	}
	else
	{
		/* Deactivate text window */
		TextTextMessage->setEnabled(false);

		/* Clear Text */
		TextTextMessage->setText("");
	}

	/* Check whether service parameters were not transmitted yet */
	if (Parameters.Service[iCurSelAudioServ].IsActive())
	{
		/* Service label (UTF-8 encoded string -> convert) */
		LabelServiceLabel->setText(QString().fromUtf8(
			Parameters.Service[iCurSelAudioServ].
			strLabel.c_str()));

		/* Bit-rate */
		QString strBitrate = QString().setNum(Parameters.
			GetBitRateKbps(iCurSelAudioServ, false), 'f', 2) +
			tr(" kbps");

		/* Equal or unequal error protection */
		const _REAL rPartABLenRat =
			Parameters.PartABLenRatio(iCurSelAudioServ);

		if (rPartABLenRat != (_REAL) 0.0)
		{
			/* Print out the percentage of part A length to total length */
			strBitrate += " UEP (" +
				QString().setNum(rPartABLenRat * 100, 'f', 1) + " %)";
		}
		else
		{
			/* If part A is zero, equal error protection (EEP) is used */
			strBitrate += " EEP";
		}
		LabelBitrate->setText(strBitrate);

		/* Service ID (plot number in hexadecimal format) */
		const long iServiceID = (long) Parameters.
			Service[iCurSelAudioServ].iServiceID;

		if (iServiceID != 0)
		{
			LabelServiceID->setText("ID:" +
				QString().setNum(iServiceID, 16).upper());
		}
		else
			LabelServiceID->setText("");

		/* Codec label */
		LabelCodec->setText(GetCodecString(iCurSelAudioServ));

		/* Type (Mono / Stereo) label */
		LabelStereoMono->setText(GetTypeString(iCurSelAudioServ));

		/* Language and program type labels (only for audio service) */
		if (Parameters.Service[iCurSelAudioServ].eAudDataFlag == SF_AUDIO)
		{
		/* SDC Language */
		const string strLangCode = Parameters.
			Service[iCurSelAudioServ].strLanguageCode;

		if ((!strLangCode.empty()) && (strLangCode != "---"))
		{
			 LabelLanguage->
				setText(QString(GetISOLanguageName(strLangCode).c_str()));
		}
		else
		{
			/* FAC Language */
			const int iLanguageID = Parameters.
				Service[iCurSelAudioServ].iLanguage;

			if ((iLanguageID > 0) &&
				(iLanguageID < LEN_TABLE_LANGUAGE_CODE))
			{
				LabelLanguage->setText(
					strTableLanguageCode[iLanguageID].c_str());
			}
			else
				LabelLanguage->setText("");
		}

			/* Program type */
			const int iProgrammTypeID = Parameters.
				Service[iCurSelAudioServ].iServiceDescr;

			if ((iProgrammTypeID > 0) &&
				(iProgrammTypeID < LEN_TABLE_PROG_TYPE_CODE))
			{
				LabelProgrType->setText(
					strTableProgTypCod[iProgrammTypeID].c_str());
			}
			else
				LabelProgrType->setText("");
		}

		/* Country code */
		const string strCntryCode = Parameters.
			Service[iCurSelAudioServ].strCountryCode;

		if ((!strCntryCode.empty()) && (strCntryCode != "--"))
		{
			LabelCountryCode->
				setText(QString(GetISOCountryName(strCntryCode).c_str()));
		}
		else
			LabelCountryCode->setText("");
	}
	else
	{
		LabelServiceLabel->setText(tr("No Service"));

		LabelBitrate->setText("");
		LabelCodec->setText("");
		LabelStereoMono->setText("");
		LabelProgrType->setText("");
		LabelLanguage->setText("");
		LabelCountryCode->setText("");
		LabelServiceID->setText("");
	}


	/* Update service selector ------------------------------------------ */
	/* Make sure a possible service was selected. If not, correct. Make sure
	   an audio service is selected. If we have a data only service, we do
	   not want to have the button pressed */
	if (((!Parameters.Service[iCurSelServiceGUI].IsActive()) ||
		(iCurSelServiceGUI != iCurSelAudioServ) &&
		Parameters.Service[iCurSelAudioServ].IsActive()) &&
		/* Make sure current selected audio service is not a data only
		   service */
		(Parameters.Service[iCurSelAudioServ].IsActive() &&
		(Parameters.Service[iCurSelAudioServ].eAudDataFlag != SF_DATA)))
	{
		/* Reset checks */
		PushButtonService1->setOn(false);
		PushButtonService2->setOn(false);
		PushButtonService3->setOn(false);
		PushButtonService4->setOn(false);

		/* Set right flag */
		switch (iCurSelAudioServ)
		{
		case 0:
			PushButtonService1->setOn(true);
			iCurSelServiceGUI = 0;
			break;

		case 1:
			PushButtonService2->setOn(true);
			iCurSelServiceGUI = 1;
			break;

		case 2:
			PushButtonService3->setOn(true);
			iCurSelServiceGUI = 2;
			break;

		case 3:
			PushButtonService4->setOn(true);
			iCurSelServiceGUI = 3;
			break;
		}
	}
	else if (Parameters.Service[iCurSelServiceGUI].eAudDataFlag == SF_DATA)
	{
		/* In case we only have data services, reset checks */
		PushButtonService1->setOn(false);
		PushButtonService2->setOn(false);
		PushButtonService3->setOn(false);
		PushButtonService4->setOn(false);
	}

	/* Service selector ------------------------------------------------- */
	/* Enable only so many number of channel switches as present in the stream */
	const int iNumServices = Parameters.GetTotNumServices();

	QString m_StaticService[MAX_NUM_SERVICES] = {"", "", "", ""};

	/* Reset all buttons only if number of services has changed */
	if (iOldNoServicesGUI != iNumServices)
	{
		PushButtonService1->setEnabled(false);
		PushButtonService2->setEnabled(false);
		PushButtonService3->setEnabled(false);
		PushButtonService4->setEnabled(false);
	}
	iOldNoServicesGUI = iNumServices;

	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		/* Check, if service is used */
		if (Parameters.Service[i].IsActive())
		{
			/* Do UTF-8 to string conversion with the label strings */
			QString strLabel = QString().fromUtf8(
			Parameters.Service[i].strLabel.c_str());

			/* Label for service selection button (service label, codec
			   and Mono / Stereo information) */
			m_StaticService[i] = strLabel + "  |   ";
			m_StaticService[i] += GetCodecString(i) + " ";
			m_StaticService[i] += GetTypeString(i);

			/* Bit-rate (only show if greater than 0) */
			const _REAL rBitRate =
				Parameters.GetBitRateKbps(i, false);

			if (rBitRate > (_REAL) 0.0)
			{
				m_StaticService[i] += " (" +
					QString().setNum(rBitRate, 'f', 2) + " kbps)";
			}

			/* Show, if a multimedia stream is connected to this service */
			if ((Parameters.Service[i].eAudDataFlag ==SF_AUDIO) &&
				(Parameters.Service[i].iDataStream != STREAM_ID_NOT_USED))
			{
				int iStreamID = Parameters.Service[i].iDataStream;
				int iPacketID = Parameters.Service[i].iPacketID;
				CDataParam& dataParam = Parameters.DataParam[iStreamID][iPacketID];

				if (dataParam.eUserAppIdent == AT_MOTEPG)
				{
					m_StaticService[i] += tr(" + EPG"); /* EPG service */
				}
				else
					m_StaticService[i] += tr(" + MM"); /* other multimedia service */

				/* Bit-rate of connected data stream */
				m_StaticService[i] += " (" + QString().setNum(
				Parameters.GetBitRateKbps(i, true), 'f', 2) +
					" kbps)";
			}

			switch (i)
			{
			case 0:
				PushButtonService1->setEnabled(true);
				break;

			case 1:
				PushButtonService2->setEnabled(true);
				break;

			case 2:
				PushButtonService3->setEnabled(true);
				break;

			case 3:
				PushButtonService4->setEnabled(true);
				break;
			}
		}
	}

	/* detect if AFS informations are available */
	if ((Parameters.AltFreqSign.vecMultiplexes.size() > 0) || (Parameters.AltFreqSign.vecOtherServices.size() > 0))
	{
		/* show AFS label */
		if (Parameters.Service[0].eAudDataFlag == SF_AUDIO) m_StaticService[0] += tr(" + AFS");
	}

	Parameters.Unlock();

	/* Set texts */
	TextMiniService1->setText(m_StaticService[0]);
	TextMiniService2->setText(m_StaticService[1]);
	TextMiniService3->setText(m_StaticService[2]);
	TextMiniService4->setText(m_StaticService[3]);
}

void DRMMainWindow::ClearDisplay()
{
	/* No signal is currently received ---------------------------------- */
	/* Disable service buttons and associated labels */
	PushButtonService1->setEnabled(false);
	PushButtonService2->setEnabled(false);
	PushButtonService3->setEnabled(false);
	PushButtonService4->setEnabled(false);
	TextMiniService1->setText("");
	TextMiniService2->setText("");
	TextMiniService3->setText("");
	TextMiniService4->setText("");

	/* Main text labels */
	LabelBitrate->setText("");
	LabelCodec->setText("");
	LabelStereoMono->setText("");
	LabelProgrType->setText("");
	LabelLanguage->setText("");
	LabelCountryCode->setText("");
	LabelServiceID->setText("");

	/* Hide text message label */
	TextTextMessage->setEnabled(false);
	TextTextMessage->setText("");

	LabelServiceLabel->setText(tr("Scanning..."));
}

void DRMMainWindow::OnNewDRMAcquisition()
{
	Receiver.RequestNewAcquisition();
}

void DRMMainWindow::OnSwitchToAnalog()
{
    // User must then select if wants FM, etc. TODO - allow direct mode changes
    Receiver.SetReceiverMode(AM);
}

void DRMMainWindow::SetService(int iNewServiceID)
{
    if(iCurSelServiceGUI == iNewServiceID)
        return;

	CParameter& Parameters = *Receiver.GetParameters();

	Parameters.Lock();

	Parameters.SetCurSelAudioService(iNewServiceID);
	Parameters.SetCurSelDataService(iNewServiceID);
	iCurSelServiceGUI = iNewServiceID;

	EStreamType eAudDataFlag = Parameters.Service[iNewServiceID].eAudDataFlag;
	Parameters.Unlock();

	/* If service is only data service or has a multimedia content
	   , activate multimedia window */
	if (eAudDataFlag == SF_DATA)
	{
		Parameters.Lock();
		int iStreamID = Parameters.Service[iNewServiceID].iDataStream;
		int iPacketID = Parameters.Service[iNewServiceID].iPacketID;
		CDataParam& dataParam = Parameters.DataParam[iStreamID][iPacketID];
		EAppType eAppIdent = dataParam.eUserAppIdent;
		Parameters.Unlock();

        switch(eAppIdent)
        {
            case AT_MOTEPG:
                epgDlg->show();
                break;
            case AT_MOTBROADCASTWEBSITE:
                if(bwsViewer==NULL)
                    bwsViewer = new BWSViewer(Receiver, Settings, this, "", Qt::WStyle_MinMax);
                bwsViewer->show();
                break;
            case AT_JOURNALINE:
                if(jlViewer==NULL)
                    jlViewer = new BWSViewer(Receiver, Settings, this, "", Qt::WStyle_MinMax);
                jlViewer->show();
                break;
            case AT_MOTSLISHOW:
                if(slideShowViewer==NULL)
                    slideShowViewer = new BWSViewer(Receiver, Settings, this, "", Qt::WStyle_MinMax);
                slideShowViewer->show();
                break;
            default:
                QMessageBox::information(this, "Dream", tr("unsupported data application"));
        }
	}
}

void DRMMainWindow::OnMenuSetDisplayColor()
{
    const QColor color = CRGBConversion::int2RGB(Settings.Get("DRMGUI", "colorscheme", 0xff0000));
    const QColor newColor = QColorDialog::getColor( color, this);
    if (newColor.isValid())
	{
		/* Store new color and update display */
		SetDisplayColor(newColor);
    	Settings.Put("DRMGUI", "colorscheme", CRGBConversion::RGB2int(newColor));
	}
}

void DRMMainWindow::showEvent(QShowEvent* pEvent)
{
 	/* default close action is to exit */
    quitWanted = true;
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("DRMGUI", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);
	if(Settings.Get("DRMGUI", "SEDvisible", false))
        sysEvalDlg->show();
	if(Settings.Get("DRMGUI", "Stationsvisible", false))
        stationsDlg->show();
	if(Settings.Get("DRMGUI", "AFSvisible", false))
        liveScheduleDlg->show();
	if(Settings.Get("DRMGUI", "EPGvisible", false))
        epgDlg->show();

	/* Activate real-time timers */
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void DRMMainWindow::hideEvent(QHideEvent* pEvent)
{
    /* remember the state of the windows */
    Settings.Put("DRMGUI", "AFSvisible", liveScheduleDlg->isVisible());
    Settings.Put("DRMGUI", "SEDvisible", sysEvalDlg->isVisible());
    Settings.Put("DRMGUI", "Stationsvisible", stationsDlg->isVisible());
    Settings.Put("DRMGUI", "EPGvisible", epgDlg->isVisible());
    /* stop any asynchronous GUI actions */
    loghelper.EnableLog(false);
    Timer.stop();

    liveScheduleDlg->hide();
    sysEvalDlg->hide();
    epgDlg->hide();
    stationsDlg->hide();
    if(jlViewer)
    {
        Settings.Put("DRMGUI", "JLvisible", jlViewer->isVisible());
        jlViewer->close();
    }
    if(bwsViewer)
    {
        Settings.Put("DRMGUI", "BWSvisible", bwsViewer->isVisible());
        bwsViewer->close();
    }
    if(slideShowViewer)
    {
        Settings.Put("DRMGUI", "SSvisible", slideShowViewer->isVisible());
        slideShowViewer->close();
    }

	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("DRMGUI", s);
}

void DRMMainWindow::closeEvent(QCloseEvent* ce)
{
	/* the close event has been actioned and we want to shut
	 * down, but the main window should be the last thing to
	 * close so that the user knows the program has completed
	 * when the window closes
	 */

    if(quitWanted)
    {
        /* request that the working thread stops */
        Receiver.Stop();
        (void)Receiver.wait(5000);
        if(!Receiver.isFinished())
        {
            QMessageBox::critical(this, "Dream", "Exit\n",
                "Termination of working thread failed");
        }
        qApp->quit();
    }

	/* now let QT close us */
	ce->accept();
}

QString DRMMainWindow::GetCodecString(const int iServiceID)
{

	CParameter& Parameters = *Receiver.GetParameters();


	/* First check if it is audio or data service */
	if (Parameters.Service[iServiceID].eAudDataFlag == SF_AUDIO)
	{
		/* Audio service */
		int iAudioStream = Parameters.Service[iServiceID].iAudioStream;

        if (iAudioStream == STREAM_ID_NOT_USED)
        {
           return QString("Waiting for stream info");
        }

		CAudioParam& audioParam = Parameters.AudioParam[iAudioStream];

		QString strReturn;

		const CAudioParam::EAudSamRat eSamRate = audioParam.eAudioSamplRate;

		/* Audio coding */
		switch (audioParam.eAudioCoding)
		{
		case CAudioParam::AC_AAC:
			/* Only 12 and 24 kHz sample rates are supported for AAC encoding */
			if (eSamRate == CAudioParam::AS_12KHZ)
				strReturn = "aac";
			else
				strReturn = "AAC";
			break;

		case CAudioParam::AC_CELP:
			/* Only 8 and 16 kHz sample rates are supported for CELP encoding */
			if (eSamRate == CAudioParam::AS_8_KHZ)
				strReturn = "celp";
			else
				strReturn = "CELP";
			break;

		case CAudioParam::AC_HVXC:
			strReturn = "HVXC";
			break;
		}

		/* SBR */
		if (audioParam.eSBRFlag == CAudioParam::SB_USED)
		{
			strReturn += "+";
		}

		return strReturn;
	}
	else
	{
		/* Data service */
		return QString("Data");
	}
}

QString DRMMainWindow::GetTypeString(const int iServiceID)
{
	QString strReturn;

	CParameter& Parameters = *Receiver.GetParameters();
	/* First check if it is audio or data service */
	if (Parameters.Service[iServiceID].eAudDataFlag == SF_AUDIO)
	{
        if (Parameters.Service[iServiceID].iAudioStream == STREAM_ID_NOT_USED)
        {
           return QString("Waiting for stream info");
        }

		/* Audio service */
		CAudioParam& audioParam = Parameters.AudioParam[Parameters.Service[iServiceID].iAudioStream];
		/* Mono-Stereo */
		switch (audioParam.eAudioMode)
		{
			case CAudioParam::AM_MONO:
				strReturn = "Mono";
				break;

			case CAudioParam::AM_P_STEREO:
				strReturn = "P-Stereo";
				break;

			case CAudioParam::AM_STEREO:
				strReturn = "Stereo";
				break;
		}
	}
	else
	{
        if (Parameters.Service[iServiceID].iDataStream == STREAM_ID_NOT_USED)
        {
           return QString("Waiting for stream info");
        }
		/* Data service */
		CDataParam& dataParam = Parameters.DataParam[
				Parameters.Service[iServiceID].iDataStream]
				[Parameters.Service[iServiceID].iPacketID];
		if (dataParam.ePacketModInd == PM_PACKET_MODE)
		{
			if (dataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
			{
				switch (dataParam.eUserAppIdent)
				{
				case AT_MOTSLISHOW:
					strReturn = "MOT Slideshow";
					break;

				case AT_MOTBROADCASTWEBSITE:
					strReturn = "MOT WebSite";
					break;

				case 4:
					strReturn = "TPEG";
					break;

				case 5:
					strReturn = "DGPS";
					break;

				case 6:
					strReturn = "TMC";
					break;

				case AT_MOTEPG:
					strReturn = "EPG - Electronic Programme Guide";
					break;

				case 8:
					strReturn = "Java";
					break;

				case AT_JOURNALINE: /* Journaline */
					strReturn = "Journaline";
					break;
                default:
                    strReturn = "Unknown Service";
				}
			}
			else
				strReturn = "Unknown Service";
		}
		else
			strReturn = "Unknown Service";
	}

	return strReturn;
}

void DRMMainWindow::SetDisplayColor(const QColor newColor)
{
	/* Collect pointer to the desired controls in a vector */
	vector<QWidget*> vecpWidgets;
	vecpWidgets.push_back(TextTextMessage);
	vecpWidgets.push_back(LabelBitrate);
	vecpWidgets.push_back(LabelCodec);
	vecpWidgets.push_back(LabelStereoMono);
	vecpWidgets.push_back(FrameAudioDataParams);
	vecpWidgets.push_back(LabelProgrType);
	vecpWidgets.push_back(LabelLanguage);
	vecpWidgets.push_back(LabelCountryCode);
	vecpWidgets.push_back(LabelServiceID);
	vecpWidgets.push_back(TextLabelInputLevel);
	vecpWidgets.push_back(ProgrInputLevel);
	vecpWidgets.push_back(CLED_FAC);
	vecpWidgets.push_back(CLED_SDC);
	vecpWidgets.push_back(CLED_MSC);
	vecpWidgets.push_back(FrameMainDisplay);

	for (size_t i = 0; i < vecpWidgets.size(); i++)
	{
		/* Request old palette */
		QPalette CurPal(vecpWidgets[i]->palette());

		/* Change colors */
		CurPal.setColor(QPalette::Active, QColorGroup::Foreground, newColor);
		CurPal.setColor(QPalette::Active, QColorGroup::Button, newColor);
		CurPal.setColor(QPalette::Active, QColorGroup::Text, newColor);
		CurPal.setColor(QPalette::Active, QColorGroup::Light, newColor);
		CurPal.setColor(QPalette::Active, QColorGroup::Dark, newColor);

		CurPal.setColor(QPalette::Inactive, QColorGroup::Foreground, newColor);
		CurPal.setColor(QPalette::Inactive, QColorGroup::Button, newColor);
		CurPal.setColor(QPalette::Inactive, QColorGroup::Text, newColor);
		CurPal.setColor(QPalette::Inactive, QColorGroup::Light, newColor);
		CurPal.setColor(QPalette::Inactive, QColorGroup::Dark, newColor);

		/* Special treatment for text message window. This should always be
		   black color of the text */
		if (vecpWidgets[i] == TextTextMessage)
		{
			CurPal.setColor(QPalette::Active, QColorGroup::Text, Qt::black);
			CurPal.setColor(QPalette::Active, QColorGroup::Foreground, Qt::black);
			CurPal.setColor(QPalette::Inactive, QColorGroup::Text, Qt::black);
			CurPal.setColor(QPalette::Inactive, QColorGroup::Foreground, Qt::black);

			/* We need to specify special color for disabled */
			CurPal.setColor(QPalette::Disabled, QColorGroup::Light, Qt::black);
			CurPal.setColor(QPalette::Disabled, QColorGroup::Dark, Qt::black);
		}

		/* Set new palette */
		vecpWidgets[i]->setPalette(CurPal);
	}
}

void DRMMainWindow::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* Text Message */
	TextTextMessage->setWhatsThis(
		tr("<b>Text Message:</b> On the top right the text "
		"message label is shown. This label only appears when an actual text "
		"message is transmitted. If the current service does not transmit a "
		"text message, the label will be disabled."));

	/* Input Level */
	const QString strInputLevel =
		tr("<b>Input Level:</b> The input level meter shows "
		"the relative input signal peak level in dB. If the level is too high, "
		"the meter turns from green to red. The red region should be avoided "
		"since overload causes distortions which degrade the reception "
		"performance. Too low levels should be avoided too, since in this case "
		"the Signal-to-Noise Ratio (SNR) degrades.");

	TextLabelInputLevel->setWhatsThis( strInputLevel);
	ProgrInputLevel->setWhatsThis( strInputLevel);

	/* Status LEDs */
	const QString strStatusLEDS =
		tr("<b>Status LEDs:</b> The three status LEDs show "
		"the current CRC status of the three logical channels of a DRM stream. "
		"These LEDs are the same as the top LEDs on the Evaluation Dialog.");

	CLED_MSC->setWhatsThis( strStatusLEDS);
	CLED_SDC->setWhatsThis( strStatusLEDS);
	CLED_FAC->setWhatsThis( strStatusLEDS);

	/* Station Label and Info Display */
	const QString strStationLabelOther =
		tr("<b>Station Label and Info Display:</b> In the "
		"big label with the black background the station label and some other "
		"information about the current selected service is displayed. The "
		"magenta text on the top shows the bit-rate of the current selected "
		"service (The abbreviations EEP and "
		"UEP stand for Equal Error Protection and Unequal Error Protection. "
		"UEP is a feature of DRM for a graceful degradation of the decoded "
		"audio signal in case of a bad reception situation. UEP means that "
		"some parts of the audio is higher protected and some parts are lower "
		"protected (the ratio of higher protected part length to total length "
		"is shown in the brackets)), the audio compression format "
		"(e.g. AAC), if SBR is used and what audio mode is used (Mono, Stereo, "
		"P-Stereo -> low-complexity or parametric stereo). In case SBR is "
		"used, the actual sample rate is twice the sample rate of the core AAC "
		"decoder. The next two types of information are the language and the "
		"program type of the service (e.g. German / News).<br>The big "
		"turquoise text in the middle is the station label. This label may "
		"appear later than the magenta text since this information is "
		"transmitted in a different logical channel of a DRM stream. On the "
		"right, the ID number connected with this service is shown.");

	LabelBitrate->setWhatsThis( strStationLabelOther);
	LabelCodec->setWhatsThis( strStationLabelOther);
	LabelStereoMono->setWhatsThis( strStationLabelOther);
	LabelServiceLabel->setWhatsThis( strStationLabelOther);
	LabelProgrType->setWhatsThis( strStationLabelOther);
	LabelServiceID->setWhatsThis( strStationLabelOther);
	LabelLanguage->setWhatsThis( strStationLabelOther);
	LabelCountryCode->setWhatsThis( strStationLabelOther);
	FrameAudioDataParams->setWhatsThis( strStationLabelOther);

	/* Service Selectors */
	const QString strServiceSel =
		tr("<b>Service Selectors:</b> In a DRM stream up to "
		"four services can be carried. The service can be an audio service, "
		"a data service or an audio service with data. "
		"Audio services can have associated text messages, in addition to any data component. "
		"If a Multimedia data service is selected, the Multimedia Dialog will automatically show up. "
		"On the right of each service selection button a short description of the service is shown. "
		"If an audio service has associated Multimedia data, \"+ MM\" is added to this text. "
		"If such a service is selected, opening the Multimedia Dialog will allow the data to be viewed "
		"while the audio is still playing. If the data component of a service is not Multimedia, "
		"but an EPG (Electronic Programme Guide) \"+ EPG\" is added to the description. "
		"The accumulated Programme Guides for all stations can be viewed by opening the Programme Guide Dialog. "
		"The selected channel in the Programme Guide Dialog defaults to the station being received. "
		"If Alternative Frequency Signalling is available, \"+ AFS\" is added to the description. "
		"In this case the alternative frequencies can be viewed by opening the Live Schedule Dialog."
	);

	PushButtonService1->setWhatsThis( strServiceSel);
	PushButtonService2->setWhatsThis( strServiceSel);
	PushButtonService3->setWhatsThis( strServiceSel);
	PushButtonService4->setWhatsThis( strServiceSel);
	TextMiniService1->setWhatsThis( strServiceSel);
	TextMiniService2->setWhatsThis( strServiceSel);
	TextMiniService3->setWhatsThis( strServiceSel);
	TextMiniService4->setWhatsThis( strServiceSel);
}
