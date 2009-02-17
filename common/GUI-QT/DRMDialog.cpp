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

#include <iostream>
#include "DRMDialog.h"
#include "ReceiverSettingsDlg.h"
#include <qlabel.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qstring.h>
#include <qmenubar.h>
#include <q3popupmenu.h>
#include <qwt_thermo.h>
#include <qevent.h>
#include <q3cstring.h>
#include <qlayout.h>
#include <q3whatsthis.h>
#include <qpalette.h>
#include <qcolordialog.h>
//Added by qt3to4:
#include <QHideEvent>
#include <QShowEvent>
#include <QCloseEvent>
#include <fstream>

/* Implementation *************************************************************/
DRMDialog::DRMDialog(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f)
	: QDialog(parent,name,modal,f), Ui_DRMDialog(),
	DRMReceiver(NDRMR),
	Settings(NSettings),
	loghelper(NDRMR, NSettings),
	eReceiverMode(NONE)
{
	setupUi(this);
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("DRM Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	Q3PopupMenu* EvalWinMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(EvalWinMenu);
	EvalWinMenu->insertItem(tr("&Evaluation Dialog..."), this,
		SLOT(OnViewEvalDlg()), Qt::CTRL+Qt::Key_E, 0);
	EvalWinMenu->insertItem(tr("M&ultimedia Dialog..."), this,
		SLOT(OnViewMultiMediaDlg()), Qt::CTRL+Qt::Key_U, 1);
	EvalWinMenu->insertItem(tr("S&tations Dialog..."), this,
		SLOT(OnViewStationsDlg()), Qt::CTRL+Qt::Key_T, 2);
	EvalWinMenu->insertItem(tr("&Live Schedule Dialog..."), this,
		SLOT(OnViewLiveScheduleDlg()), Qt::CTRL+Qt::Key_L, 3);
	EvalWinMenu->insertItem(tr("&Programme Guide..."), this,
		SLOT(OnViewEPGDlg()), Qt::CTRL+Qt::Key_P, 4);
	EvalWinMenu->insertSeparator();
	EvalWinMenu->insertItem(tr("E&xit"), this, SLOT(close()), Qt::CTRL+Qt::Key_Q, 5);

	/* Settings menu  ------------------------------------------------------- */
	pSettingsMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(DRMReceiver.GetSoundInInterface(),
		DRMReceiver.GetSoundOutInterface(), this));

	pSettingsMenu->insertItem(tr("&AM (analog)"), this,
		SLOT(OnSwitchToAM()), Qt::CTRL+Qt::Key_A);
	pSettingsMenu->insertItem(tr("New &DRM Acquisition"), this,
		SLOT(OnNewDRMAcquisition()), Qt::CTRL+Qt::Key_D);
	pSettingsMenu->insertSeparator();
	pSettingsMenu->insertItem(tr("Set D&isplay Color..."), this,
		SLOT(OnMenuSetDisplayColor()));

	/* Plot style settings */
	pPlotStyleMenu = new Q3PopupMenu(this);
	pPlotStyleMenu->insertItem(tr("&Blue / White"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 0);
	pPlotStyleMenu->insertItem(tr("&Green / Black"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 1);
	pPlotStyleMenu->insertItem(tr("B&lack / Grey"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 2);
	pSettingsMenu->insertItem(tr("&Plot Style"), pPlotStyleMenu);

	/* Set check */
	pPlotStyleMenu->setItemChecked(Settings.Get("System Evaluation Dialog", "plotstyle", 0), true);

	/* multimedia settings */
	pSettingsMenu->insertSeparator();
	pSettingsMenu->insertItem(tr("&Multimedia settings..."), this,
		SLOT(OnViewMultSettingsDlg()));

	pSettingsMenu->insertItem(tr("&Receiver settings..."), this,
		SLOT(OnViewReceiverSettingsDlg()));

	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	Q_CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), EvalWinMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	vboxLayout->setMenuBar(pMenu);

	/* Digi controls */
	/* Set display color */
	SetDisplayColor(CRGBConversion::int2RGB(Settings.Get("DRM Dialog", "colorscheme", 0xff0000)));

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

	/* Stations window */
	pStationsDlg = new StationsDlg(DRMReceiver, Settings, this, "", false, Qt::WStyle_MinMax);
	bStationsDlgWasVis = Settings.Get("Stations Dialog", "visible", false);

	SetDialogCaption(pStationsDlg, tr("Stations"));

	/* Live Schedule window */
	pLiveScheduleDlg = new LiveScheduleDlg(DRMReceiver, this, "", false, Qt::WStyle_MinMax);
	bLiveSchedDlgWasVis = Settings.Get("Live Schedule Dialog", "visible", false);
	pLiveScheduleDlg->LoadSettings(Settings);

	SetDialogCaption(pLiveScheduleDlg, tr("Live Schedule"));

	/* Programme Guide Window */
	pEPGDlg = new EPGDlg(DRMReceiver, Settings, this, "", false, Qt::WStyle_MinMax);
	bEPGDlgWasVis = Settings.Get("EPG Dialog", "visible", false);

	SetDialogCaption(pEPGDlg, tr("Programme Guide"));


	/* Evaluation window */
	pSysEvalDlg = new systemevalDlg(DRMReceiver, Settings, this, "", false, Qt::WStyle_MinMax);
	bSysEvalDlgWasVis = Settings.Get("System Evaluation Dialog", "visible", false);

	SetDialogCaption(pSysEvalDlg, tr("System Evaluation"));

	/* Multimedia window */
	pMultiMediaDlg = new MultimediaDlg(DRMReceiver, this, "", false, Qt::WStyle_MinMax);
	bMultMedDlgWasVis = Settings.Get("Multimedia Dialog", "visible", false);

	SetDialogCaption(pMultiMediaDlg, tr("Multimedia"));
	pMultiMediaDlg->LoadSettings(Settings);

	/* receiver settings window */
	CParameter& Parameters = *DRMReceiver.GetParameters();
	pReceiverSettingsDlg = new ReceiverSettingsDlg(DRMReceiver, Settings, this, "", true, Qt::WType_Dialog);
	SetDialogCaption(pReceiverSettingsDlg, tr("Receiver settings"));

	/* Analog demodulation window */
	pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings, *pReceiverSettingsDlg,
						NULL, "Analog Demodulation", false, Qt::WStyle_MinMax);

	Parameters.Lock();

	/* Enable multimedia */
	Parameters.EnableMultimedia(true);

	/* Init current selected service */
	Parameters.ResetCurSelAudDatServ();

	Parameters.Unlock();

	iCurSelServiceGUI = 0;
	iOldNoServicesGUI = 0;

	PushButtonService1->setOn(true);
	PushButtonService1->setEnabled(false);
	PushButtonService2->setEnabled(false);
	PushButtonService3->setEnabled(false);
	PushButtonService4->setEnabled(false);

	/* Update times for color LEDs */
	CLED_FAC->SetUpdateTime(1500);
	CLED_SDC->SetUpdateTime(1500);
	CLED_MSC->SetUpdateTime(600);

	/* Connect buttons */
	connect(PushButtonService1, SIGNAL(clicked()),
		this, SLOT(OnButtonService1()));
	connect(PushButtonService2, SIGNAL(clicked()),
		this, SLOT(OnButtonService2()));
	connect(PushButtonService3, SIGNAL(clicked()),
		this, SLOT(OnButtonService3()));
	connect(PushButtonService4, SIGNAL(clicked()),
		this, SLOT(OnButtonService4()));

	connect(pAnalogDemDlg, SIGNAL(SwitchToDRM()), this, SLOT(OnSwitchToDRM()));
	connect(pAnalogDemDlg, SIGNAL(ViewStationsDlg()),
		this, SLOT(OnViewStationsDlg()));
	connect(pAnalogDemDlg, SIGNAL(ViewLiveScheduleDlg()),
		this, SLOT(OnViewLiveScheduleDlg()));
	connect(pAnalogDemDlg, SIGNAL(Closed()),
		this, SLOT(close()));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	connect(pReceiverSettingsDlg, SIGNAL(StartStopGPS(bool)), pSysEvalDlg, SLOT(EnableGPS(bool)));
	connect(pReceiverSettingsDlg, SIGNAL(ShowHideGPS(bool)), pSysEvalDlg, SLOT(ShowGPS(bool)));

	Loghelper *ploghelper = &loghelper;

	connect(pReceiverSettingsDlg, SIGNAL(StartStopLog(bool)), ploghelper, SLOT(EnableLog(bool)));
	connect(pReceiverSettingsDlg, SIGNAL(SetLogStartDelay(long)), ploghelper, SLOT(LogStartDel(long)));
	connect(pReceiverSettingsDlg, SIGNAL(LogPosition(bool)), ploghelper, SLOT(LogPosition(bool)));
	connect(pReceiverSettingsDlg, SIGNAL(LogSigStr(bool)), ploghelper, SLOT(LogSigStr(bool)));

	/* Disable text message label */
	TextTextMessage->setText("");
	TextTextMessage->setEnabled(false);

	/* Activate real-time timers */
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

DRMDialog::~DRMDialog()
{
}

void DRMDialog::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
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

void DRMDialog::OnTimer()
{
	EDemodulationType eNewReceiverMode = DRMReceiver.GetReceiverMode();
	switch(eNewReceiverMode)
	{
	case DRM:
		if(eReceiverMode != DRM)
			ChangeGUIModeToDRM();
		{
			CParameter& Parameters = *DRMReceiver.GetParameters();
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
			if(DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
				UpdateDisplay();
			else
				ClearDisplay();
		}
		break;
	case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		/* stopping the timer is normally done by the hide signal, but at startup we
		 * are already hidden and the hide signal doesn't hit the slot
		 */
		if(eReceiverMode != AM) // AM means any analog mode
		{
			Timer.stop();
			ChangeGUIModeToAM();
		} /* otherwise we might still be changing to DRM from AM */
		break;
	case NONE: // wait until working thread starts operating
		break;
	}
    time_t t = time(NULL);
    if((t % 5) == 0)
    {
        ofstream f("p.txt", ios::app | ios::out);
        f << "// Next Sample" << endl;
        DRMReceiver.GetParameters()->dump(f);
        f.close();
    }
}

void DRMDialog::UpdateDisplay()
{
	CParameter& Parameters = *(DRMReceiver.GetParameters());

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

		/* Text message of current selected audio service
		   (UTF-8 decoding) */
		Q3CString utf8Message = Parameters.AudioParam[iAudioStream].strTextMessage.c_str();
		QString textMessage = QString().fromUtf8(utf8Message);
		QString formattedMessage = "";
		for (size_t i = 0; i < textMessage.length(); i++)
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
		LabelServiceLabel->setText(QString().fromUtf8(Q3CString(
			Parameters.Service[iCurSelAudioServ].
			strLabel.c_str())));

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
			Q3CString(Parameters.Service[i].strLabel.c_str()));

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

				if (dataParam.iUserAppIdent == AT_MOTEPG)
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

void DRMDialog::ClearDisplay()
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

/* change mode is only called when the mode REALLY has changed
 * so no conditionals are needed in this routine
 */

void DRMDialog::ChangeGUIModeToDRM()
{
	show();

	/* Set correct schedule */
	pStationsDlg->SetCurrentSchedule(CDRMSchedule::SM_DRM);

	if (bStationsDlgWasVis == true)
		pStationsDlg->show();

	if (bLiveSchedDlgWasVis == true)
		pLiveScheduleDlg->show();

	if (bEPGDlgWasVis == true)
		pEPGDlg->show();

	if (bSysEvalDlgWasVis == true)
		pSysEvalDlg->show();

	if (bMultMedDlgWasVis == true)
		pMultiMediaDlg->show();

	if (pStationsDlg->isVisible())
		pStationsDlg->LoadSchedule(CDRMSchedule::SM_DRM);

	eReceiverMode = DRM;
}

/* Main window is not needed, hide it.
 * If DRM only windows are open, hide them.
 * Make sure analog demodulation dialog is visible
 */

void DRMDialog::ChangeGUIModeToAM()
{
	/* Store visibility state */
	if (eReceiverMode != NONE)
	{
		bSysEvalDlgWasVis = pSysEvalDlg->isVisible();
		bMultMedDlgWasVis = pMultiMediaDlg->isVisible();
		bEPGDlgWasVis = pEPGDlg->isVisible();
	}

	pSysEvalDlg->hide();
	pMultiMediaDlg->hide();
	pEPGDlg->hide();

	loghelper.EnableLog(false);

	Timer.stop();

	this->hide();

	eReceiverMode = AM; // euphemism for NOT DRM

	pAnalogDemDlg->show();

	/* Set correct schedule */
	pStationsDlg->SetCurrentSchedule(CDRMSchedule::SM_ANALOG);

	if (bStationsDlgWasVis == true)
		pStationsDlg->show();

	if (bLiveSchedDlgWasVis == true)
		pLiveScheduleDlg->show();

	if (pStationsDlg->isVisible())
		pStationsDlg->LoadSchedule(CDRMSchedule::SM_ANALOG);

}

void DRMDialog::showEvent(QShowEvent*)
{
	/* Set timer for real-time controls */
	OnTimer();
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void DRMDialog::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer */
	Timer.stop();
}

void DRMDialog::OnSwitchToDRM()
{
	bStationsDlgWasVis = pStationsDlg->isVisible();
	bLiveSchedDlgWasVis = pLiveScheduleDlg->isVisible();

	DRMReceiver.SetReceiverMode(DRM);
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void DRMDialog::OnNewDRMAcquisition()
{
	DRMReceiver.RequestNewAcquisition();
}

void DRMDialog::OnSwitchToAM()
{
	bStationsDlgWasVis = pStationsDlg->isVisible();
	bLiveSchedDlgWasVis = pLiveScheduleDlg->isVisible();
	DRMReceiver.SetReceiverMode(AM); // User must then select if wants FM, etc.
}

void DRMDialog::OnButtonService1()
{
	if (PushButtonService1->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService2->isOn()) PushButtonService2->setOn(false);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(false);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(false);

		SetService(0);
	}
	else
		PushButtonService1->setOn(true);
}

void DRMDialog::OnButtonService2()
{
	if (PushButtonService2->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(false);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(false);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(false);

		SetService(1);
	}
	else
		PushButtonService2->setOn(true);

}

void DRMDialog::OnButtonService3()
{
	if (PushButtonService3->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(false);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(false);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(false);

		SetService(2);
	}
	else
		PushButtonService3->setOn(true);
}

void DRMDialog::OnButtonService4()
{
	if (PushButtonService4->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(false);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(false);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(false);

		SetService(3);
	}
	else
		PushButtonService4->setOn(true);
}

void DRMDialog::SetService(int iNewServiceID)
{
	CParameter& Parameters = *DRMReceiver.GetParameters();

	Parameters.Lock();

	Parameters.SetCurSelAudioService(iNewServiceID);
	Parameters.SetCurSelDataService(iNewServiceID);
	iCurSelServiceGUI = iNewServiceID;

	/* If service is only data service or has a multimedia content
	   , activate multimedia window */
	EStreamType eAudDataFlag = Parameters.Service[iNewServiceID].eAudDataFlag;
	Parameters.Unlock();

	if (eAudDataFlag == SF_DATA)
	{
		Parameters.Lock();
		int iStreamID = Parameters.Service[iNewServiceID].iDataStream;
		int iPacketID = Parameters.Service[iNewServiceID].iPacketID;
		CDataParam& dataParam = Parameters.DataParam[iStreamID][iPacketID];
		int iAppIdent = dataParam.iUserAppIdent;
		Parameters.Unlock();

        switch(iAppIdent)
        {
            case AT_MOTEPG:
                OnViewEPGDlg();
                break;
            case AT_MOTBROADCASTWEBSITE:
            case AT_JOURNALINE:
            case AT_MOTSLISHOW:
                OnViewMultiMediaDlg();
                break;
            default:
                QMessageBox::information(this, "Dream", tr("unsupported data application"));
        }
	}
}

void DRMDialog::OnViewEvalDlg()
{
	if(DRMReceiver.GetReceiverMode() == DRM)
	{
		/* Show evaluation window in DRM mode */
		pSysEvalDlg->show();
	}
	else
	{
		/* Show AM demodulation window in AM mode */
		pAnalogDemDlg->show();
	}
}

void DRMDialog::OnViewMultiMediaDlg()
{
	/* Show Multimedia window */
	pMultiMediaDlg->show();
}

void DRMDialog::OnViewStationsDlg()
{
	/* Show stations window */
	pStationsDlg->show();
}

void DRMDialog::OnViewLiveScheduleDlg()
{
	/* Show live schedule window */
	pLiveScheduleDlg->show();
}

void DRMDialog::OnViewMultSettingsDlg()
{

	/* Show multimedia settings window */
	MultSettingsDlg* pMultSettingsDlg = new MultSettingsDlg(Settings, this, "", true, Qt::WStyle_Dialog);

	SetDialogCaption(pMultSettingsDlg, tr("Multimedia settings"));

	pMultSettingsDlg->show();

}

void DRMDialog::OnViewReceiverSettingsDlg()
{
	pReceiverSettingsDlg->show();
}

void DRMDialog::OnViewEPGDlg()
{
	/* Show programme guide window */
	pEPGDlg->show();
}

void DRMDialog::OnMenuSetDisplayColor()
{
    const QColor color = CRGBConversion::int2RGB(Settings.Get("DRM Dialog", "colorscheme", 0xff0000));
    const QColor newColor = QColorDialog::getColor( color, this);
    if (newColor.isValid())
	{
		/* Store new color and update display */
		SetDisplayColor(newColor);
    	Settings.Put("DRM Dialog", "colorscheme", CRGBConversion::RGB2int(newColor));
	}
}

void DRMDialog::OnMenuPlotStyle(int value)
{
	/* Save new style in global variable */
	Settings.Put("System Evaluation Dialog", "plotstyle", value);

	/* Set new plot style in other dialogs */
	pSysEvalDlg->UpdatePlotsStyle();
	pAnalogDemDlg->UpdatePlotsStyle();

	/* Taking care of the checks */
	for (int i = 0; i < NUM_AVL_COLOR_SCHEMES_PLOT; i++)
		pPlotStyleMenu->setItemChecked(i, i == value);
}

void DRMDialog::closeEvent(QCloseEvent* ce)
{
	/* the close event has been actioned and we want to shut
	 * down, but the main window should be the last thing to
	 * close so that the user knows the program has completed
	 * when the window closes
	 */

	/* this can be called in two situations:
	 * DRM Mode:
	 * 	this window is visible and this routine is responsible
	 * 	for storing state, hiding other windows, stopping the working
	 * 	thread and remaining open until the rest of the system is shut
	 * 	down
	 * AM Mode:
	 *  this window is hidden, the AnalogDemDlg has stored state,
	 *  stayed open until the system is cleared down and then emitted
	 *  our signal
	 */
	if(eReceiverMode == DRM)
	{
		Settings.Put("GUI", "mode", string("DRMRX"));
		/* remember the state of the windows */
		Settings.Put("DRM Dialog", "visible", true);
		Settings.Put("AM Dialog", "visible", false);
		Settings.Put("Stations Dialog", "visible", pStationsDlg->isVisible());
		Settings.Put("Live Schedule Dialog", "visible", pLiveScheduleDlg->isVisible());
		Settings.Put("System Evaluation Dialog", "visible", pSysEvalDlg->isVisible());
		Settings.Put("Multimedia Dialog", "visible", pMultiMediaDlg->isVisible());
		Settings.Put("EPG Dialog", "visible", pEPGDlg->isVisible());

		/* stop any asynchronous GUI actions */
		loghelper.EnableLog(false);
		Timer.stop();

		/* now close all the windows except the main window */

		pSysEvalDlg->hide();
		pMultiMediaDlg->hide();
		pLiveScheduleDlg->hide();
		pEPGDlg->hide();
		pStationsDlg->hide();
		pAnalogDemDlg->hide();

		/* request that the working thread stops
		 * TODO move this to main and pass a close routine to here and
		* AnalogDemDlg to cover gps and anything else
		* or possible have a new ALWAYS hidden main dialogue box
		* that manages startup and close-down */
		DRMReceiver.Stop();
		(void)DRMReceiver.wait(5000);
		if(!DRMReceiver.isFinished())
		{
			QMessageBox::critical(this, "Dream", "Exit\n",
				"Termination of working thread failed");
		}
	}
	else
	{
		Settings.Put("GUI", "mode", string("AMRX"));
		Settings.Put("DRM Dialog", "visible", false);
		Settings.Put("AM Dialog", "visible", true);
		Settings.Put("Stations Dialog", "visible", pStationsDlg->isVisible());

		if (pStationsDlg->isVisible())
			pStationsDlg->hide();

		Settings.Put("Live Schedule Dialog", "visible", pLiveScheduleDlg->isVisible());

		if (pLiveScheduleDlg->isVisible())
			pLiveScheduleDlg->hide();

		/* we saved these when we were in DRM Mode */
		Settings.Put("System Evaluation Dialog", "visible", bSysEvalDlgWasVis);
		Settings.Put("Multimedia Dialog", "visible", bMultMedDlgWasVis);
		Settings.Put("EPG Dialog", "visible",  bEPGDlgWasVis);
	}

	pMultiMediaDlg->SaveSettings(Settings);
	pLiveScheduleDlg->SaveSettings(Settings);

	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("DRM Dialog", s);

	/* now let QT close us */
	ce->accept();
}

QString DRMDialog::GetCodecString(const int iServiceID)
{

	CParameter& Parameters = *DRMReceiver.GetParameters();


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

QString DRMDialog::GetTypeString(const int iServiceID)
{
	QString strReturn;

	CParameter& Parameters = *DRMReceiver.GetParameters();
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
				switch (dataParam.iUserAppIdent)
				{
				case 1:
					strReturn = "Dynamic labels";
					break;

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

void DRMDialog::SetDisplayColor(const QColor newColor)
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

void DRMDialog::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* Text Message */
	Q3WhatsThis::add(TextTextMessage,
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

	Q3WhatsThis::add(TextLabelInputLevel, strInputLevel);
	Q3WhatsThis::add(ProgrInputLevel, strInputLevel);

	/* Status LEDs */
	const QString strStatusLEDS =
		tr("<b>Status LEDs:</b> The three status LEDs show "
		"the current CRC status of the three logical channels of a DRM stream. "
		"These LEDs are the same as the top LEDs on the Evaluation Dialog.");

	Q3WhatsThis::add(CLED_MSC, strStatusLEDS);
	Q3WhatsThis::add(CLED_SDC, strStatusLEDS);
	Q3WhatsThis::add(CLED_FAC, strStatusLEDS);

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

	Q3WhatsThis::add(LabelBitrate, strStationLabelOther);
	Q3WhatsThis::add(LabelCodec, strStationLabelOther);
	Q3WhatsThis::add(LabelStereoMono, strStationLabelOther);
	Q3WhatsThis::add(LabelServiceLabel, strStationLabelOther);
	Q3WhatsThis::add(LabelProgrType, strStationLabelOther);
	Q3WhatsThis::add(LabelServiceID, strStationLabelOther);
	Q3WhatsThis::add(LabelLanguage, strStationLabelOther);
	Q3WhatsThis::add(LabelCountryCode, strStationLabelOther);
	Q3WhatsThis::add(FrameAudioDataParams, strStationLabelOther);

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

	Q3WhatsThis::add(PushButtonService1, strServiceSel);
	Q3WhatsThis::add(PushButtonService2, strServiceSel);
	Q3WhatsThis::add(PushButtonService3, strServiceSel);
	Q3WhatsThis::add(PushButtonService4, strServiceSel);
	Q3WhatsThis::add(TextMiniService1, strServiceSel);
	Q3WhatsThis::add(TextMiniService2, strServiceSel);
	Q3WhatsThis::add(TextMiniService3, strServiceSel);
	Q3WhatsThis::add(TextMiniService4, strServiceSel);
}
