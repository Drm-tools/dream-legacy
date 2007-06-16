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

#include "fdrmdialog.h"
#include <iostream>

/* Implementation *************************************************************/
FDRMDialog::FDRMDialog(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f)
	: FDRMDialogBase(parent, name, modal, f),
	DRMReceiver(NDRMR),
	Settings(NSettings),
	eReceiverMode(RM_NONE)
{
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
	QPopupMenu* EvalWinMenu = new QPopupMenu(this);
	CHECK_PTR(EvalWinMenu);
	EvalWinMenu->insertItem(tr("&Evaluation Dialog..."), this,
		SLOT(OnViewEvalDlg()), CTRL+Key_E, 0);
	EvalWinMenu->insertItem(tr("M&ultimedia Dialog..."), this,
		SLOT(OnViewMultiMediaDlg()), CTRL+Key_U, 1);
	EvalWinMenu->insertItem(tr("S&tations Dialog..."), this,
		SLOT(OnViewStationsDlg()), CTRL+Key_T, 2);
	EvalWinMenu->insertItem(tr("&Live Schedule Dialog..."), this,
		SLOT(OnViewLiveScheduleDlg()), CTRL+Key_L, 3);
	EvalWinMenu->insertItem(tr("&Programme Guide..."), this,
		SLOT(OnViewEPGDlg()), CTRL+Key_P, 4);
	EvalWinMenu->insertSeparator();
	EvalWinMenu->insertItem(tr("E&xit"), this, SLOT(close()), CTRL+Key_Q, 5);

	/* Settings menu  ------------------------------------------------------- */
	pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(DRMReceiver.GetSoundInInterface(),
		DRMReceiver.GetSoundOutInterface(), this));

	pSettingsMenu->insertItem(tr("&AM (analog)"), this,
		SLOT(OnSwitchToAM()), CTRL+Key_A);
	pSettingsMenu->insertItem(tr("New &DRM Acquisition"), this,
		SLOT(OnNewDRMAcquisition()), CTRL+Key_D);
	pSettingsMenu->insertSeparator();
	pSettingsMenu->insertItem(tr("Set D&isplay Color..."), this,
		SLOT(OnMenuSetDisplayColor()));

	/* Plot style settings */
	pPlotStyleMenu = new QPopupMenu(this);
	pPlotStyleMenu->insertItem(tr("&Blue / White"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 0);
	pPlotStyleMenu->insertItem(tr("&Green / Black"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 1);
	pPlotStyleMenu->insertItem(tr("B&lack / Grey"), this,
		SLOT(OnMenuPlotStyle(int)), 0, 2);
	pSettingsMenu->insertItem(tr("&Plot Style"), pPlotStyleMenu);

	/* Set check */
	pPlotStyleMenu->setItemChecked(Settings.Get("DRM Dialog", "plotcolor", 0), TRUE);

	/* multimedia settings */
	pSettingsMenu->insertSeparator();
	pSettingsMenu->insertItem(tr("&Multimedia settings..."), this,
		SLOT(OnViewMultSettingsDlg()));

	pSettingsMenu->insertItem(tr("&General settings..."), this,
		SLOT(OnViewGeneralSettingsDlg()));

	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), EvalWinMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	FDRMDialogBaseLayout->setMenuBar(pMenu);


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
	ProgrInputLevel->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-12.5);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));

	/* Stations window */
	pStationsDlg = new StationsDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
	bStationsDlgWasVis = Settings.Get("Stations Dialog", "visible", FALSE);

	SetDialogCaption(pStationsDlg, tr("Stations"));

	/* Live Schedule window */
	pLiveScheduleDlg = new LiveScheduleDlg(DRMReceiver, this, "", FALSE, Qt::WStyle_MinMax);
	bLiveSchedDlgWasVis = Settings.Get("Live Schedule Dialog", "visible", FALSE);
	pLiveScheduleDlg->LoadSettings(Settings);

	SetDialogCaption(pLiveScheduleDlg, tr("Live Schedule"));

	/* Programme Guide Window */
	pEPGDlg = new EPGDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
	bEPGDlgWasVis = Settings.Get("EPG Dialog", "visible", FALSE);

	SetDialogCaption(pEPGDlg, tr("Programme Guide"));


	/* Evaluation window */
	pSysEvalDlg = new systemevalDlg(DRMReceiver, Settings, this, "", FALSE, Qt::WStyle_MinMax);
	bSysEvalDlgWasVis = Settings.Get("System Evaluation Dialog", "visible", FALSE);

	SetDialogCaption(pSysEvalDlg, tr("System Evaluation"));

	/* Multimedia window */
	pMultiMediaDlg = new MultimediaDlg(DRMReceiver, this, "", FALSE, Qt::WStyle_MinMax);
	bMultMedDlgWasVis = Settings.Get("Multimedia Dialog", "visible", FALSE);

	SetDialogCaption(pMultiMediaDlg, tr("Multimedia"));
	pMultiMediaDlg->LoadSettings(Settings);

	/* Analog demodulation window */
	pAnalogDemDlg = new AnalogDemDlg(DRMReceiver, Settings, NULL, "Analog Demodulation", FALSE, Qt::WStyle_MinMax);

	/* general settings window */
	CParameter& Parameters = *DRMReceiver.GetParameters();
	pGeneralSettingsDlg = new GeneralSettingsDlg(Parameters, Settings, this, "", TRUE, Qt::WStyle_Dialog);
	SetDialogCaption(pGeneralSettingsDlg, tr("General settings"));

	Parameters.Lock();

	/* Enable multimedia */
	Parameters.EnableMultimedia(TRUE);

	/* Init current selected service */
	Parameters.ResetCurSelAudDatServ();

	Parameters.Unlock();

	iCurSelServiceGUI = 0;
	iOldNoServicesGUI = 0;

	PushButtonService1->setOn(TRUE);
	PushButtonService1->setEnabled(FALSE);
	PushButtonService2->setEnabled(FALSE);
	PushButtonService3->setEnabled(FALSE);
	PushButtonService4->setEnabled(FALSE);

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

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));

	connect(pGeneralSettingsDlg, SIGNAL(StartGPS()), pSysEvalDlg, SLOT(EnableGPS()));
	connect(pGeneralSettingsDlg, SIGNAL(StopGPS()), pSysEvalDlg, SLOT(DisableGPS()));

	/* Disable text message label */
	TextTextMessage->setText("");
	TextTextMessage->setEnabled(FALSE);

	/* Activate real-time timers */
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

FDRMDialog::~FDRMDialog()
{
}

void FDRMDialog::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
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

void FDRMDialog::OnTimer()
{
	ERecMode eNewReceiverMode = DRMReceiver.GetReceiverMode();
	switch(eNewReceiverMode)
	{
	case RM_DRM:
		if(eReceiverMode != RM_DRM)
			ChangeGUIModeToDRM();
		{
			CParameter& Parameters = *DRMReceiver.GetParameters();
			Parameters.Lock();

			/* Input level meter */
			ProgrInputLevel->setValue(Parameters.GetIFSignalLevel());

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
	case RM_AM:
		/* stopping the timer is normally done by the hide signal, but at startup we
		 * are already hidden and the hide signal doesn't hit the slot
		 */
		if(eReceiverMode != RM_AM)
		{
			Timer.stop();
			ChangeGUIModeToAM();
		} /* otherwise we might still be changing to DRM from AM */
		break;
	case RM_NONE: // wait until working thread starts operating
		break;
	}
}

void FDRMDialog::UpdateDisplay()
{
	CParameter& Parameters = *(DRMReceiver.GetParameters());

	Parameters.Lock();

	/* Receiver does receive a DRM signal ------------------------------- */
	/* First get current selected services */
	int iCurSelAudioServ = Parameters.GetCurSelAudioService();

	/* If the current audio service is not active or is an only data service
	   select the first audio service available */

	if (!Parameters.Service[iCurSelAudioServ].IsActive() ||
	    Parameters.Service[iCurSelAudioServ].AudioParam.iStreamID == STREAM_ID_NOT_USED ||
	    Parameters.Service[iCurSelAudioServ].eAudDataFlag == CService::SF_DATA)
	{
		int i = 0;
		_BOOLEAN bStop = FALSE;

		while ((bStop == FALSE) && (i < MAX_NUM_SERVICES))
		{
			if (Parameters.Service[i].IsActive() &&
			    Parameters.Service[i].AudioParam.iStreamID != STREAM_ID_NOT_USED &&
			    Parameters.Service[i].eAudDataFlag == CService::SF_AUDIO)
			{
				iCurSelAudioServ = i;
				bStop = TRUE;
			}
			else
				i++;
		}
	}

	//const int iCurSelDataServ = Parameters.GetCurSelDataService();

	/* If selected service is audio and text message is true */
	if ((Parameters.Service[iCurSelAudioServ].
		eAudDataFlag == CService::SF_AUDIO) &&
		(Parameters.Service[iCurSelAudioServ].
		AudioParam.bTextflag == TRUE))
	{
		/* Activate text window */
		TextTextMessage->setEnabled(TRUE);

		/* Text message of current selected audio service
		   (UTF-8 decoding) */
		QCString utf8Message =
			Parameters.Service[iCurSelAudioServ]
				.AudioParam.strTextMessage.c_str();
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
		TextTextMessage->setEnabled(FALSE);

		/* Clear Text */
		TextTextMessage->setText("");
	}

	/* Check whether service parameters were not transmitted yet */
	if (Parameters.Service[iCurSelAudioServ].IsActive())
	{
		/* Service label (UTF-8 encoded string -> convert) */
		LabelServiceLabel->setText(QString().fromUtf8(QCString(
			Parameters.Service[iCurSelAudioServ].
			strLabel.c_str())));

		/* Bit-rate */
		QString strBitrate = QString().setNum(Parameters.
			GetBitRateKbps(iCurSelAudioServ, FALSE), 'f', 2) +
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
		if (Parameters.Service[iCurSelAudioServ].
			eAudDataFlag == CService::SF_AUDIO)
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
		(Parameters.Service[iCurSelAudioServ].eAudDataFlag !=
		CService::SF_DATA)))
	{
		/* Reset checks */
		PushButtonService1->setOn(FALSE);
		PushButtonService2->setOn(FALSE);
		PushButtonService3->setOn(FALSE);
		PushButtonService4->setOn(FALSE);

		/* Set right flag */
		switch (iCurSelAudioServ)
		{
		case 0:
			PushButtonService1->setOn(TRUE);
			iCurSelServiceGUI = 0;
			break;

		case 1:
			PushButtonService2->setOn(TRUE);
			iCurSelServiceGUI = 1;
			break;

		case 2:
			PushButtonService3->setOn(TRUE);
			iCurSelServiceGUI = 2;
			break;

		case 3:
			PushButtonService4->setOn(TRUE);
			iCurSelServiceGUI = 3;
			break;
		}
	}
	else if (Parameters.Service[iCurSelServiceGUI].
		eAudDataFlag ==	CService::SF_DATA)
	{
		/* In case we only have data services, reset checks */
		PushButtonService1->setOn(FALSE);
		PushButtonService2->setOn(FALSE);
		PushButtonService3->setOn(FALSE);
		PushButtonService4->setOn(FALSE);
	}

	/* Service selector ------------------------------------------------- */
	/* Enable only so many number of channel switches as present in the stream */
	const int iNumServices = Parameters.GetTotNumServices();

	QString m_StaticService[MAX_NUM_SERVICES] = {"", "", "", ""};

	/* Reset all buttons only if number of services has changed */
	if (iOldNoServicesGUI != iNumServices)
	{
		PushButtonService1->setEnabled(FALSE);
		PushButtonService2->setEnabled(FALSE);
		PushButtonService3->setEnabled(FALSE);
		PushButtonService4->setEnabled(FALSE);
	}
	iOldNoServicesGUI = iNumServices;

	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		/* Check, if service is used */
		if (Parameters.Service[i].IsActive())
		{
			/* Do UTF-8 to string conversion with the label strings */
			QString strLabel = QString().fromUtf8(
			QCString(Parameters.Service[i].strLabel.c_str()));

			/* Label for service selection button (service label, codec
			   and Mono / Stereo information) */
			m_StaticService[i] = strLabel + "  |   ";
			m_StaticService[i] += GetCodecString(i) + " ";
			m_StaticService[i] += GetTypeString(i);

			/* Bit-rate (only show if greater than 0) */
			const _REAL rBitRate =
				Parameters.GetBitRateKbps(i, FALSE);

			if (rBitRate > (_REAL) 0.0)
			{
				m_StaticService[i] += " (" +
					QString().setNum(rBitRate, 'f', 2) + " kbps)";
			}

			/* Show, if a multimedia stream is connected to this service */
			if ((Parameters.Service[i].
				eAudDataFlag == CService::SF_AUDIO) &&
				(Parameters.Service[i].
				DataParam.iStreamID != STREAM_ID_NOT_USED))
			{

				if (Parameters.Service[i].
					DataParam.iUserAppIdent == AT_MOTEPG)
				{
					m_StaticService[i] += tr(" + EPG"); /* EPG service */
				}
				else
					m_StaticService[i] += tr(" + MM"); /* other multimedia service */

				/* Bit-rate of connected data stream */
				m_StaticService[i] += " (" + QString().setNum(
				Parameters.GetBitRateKbps(i, TRUE), 'f', 2) +
					" kbps)";
			}

			switch (i)
			{
			case 0:
				PushButtonService1->setEnabled(TRUE);
				break;

			case 1:
				PushButtonService2->setEnabled(TRUE);
				break;

			case 2:
				PushButtonService3->setEnabled(TRUE);
				break;

			case 3:
				PushButtonService4->setEnabled(TRUE);
				break;
			}
		}
	}

	/* detect if AFS informations are available */
	if ((Parameters.AltFreqSign.vecMultiplexes.size() > 0) || (Parameters.AltFreqSign.vecOtherServices.size() > 0))
	{
		/* show AFS label */
		if (Parameters.Service[0].eAudDataFlag
				== CService::SF_AUDIO) m_StaticService[0] += tr(" + AFS");
	}

	Parameters.Unlock();

	/* Set texts */
	TextMiniService1->setText(m_StaticService[0]);
	TextMiniService2->setText(m_StaticService[1]);
	TextMiniService3->setText(m_StaticService[2]);
	TextMiniService4->setText(m_StaticService[3]);
}

void FDRMDialog::ClearDisplay()
{
	/* No signal is currently received ---------------------------------- */
	/* Disable service buttons and associated labels */
	PushButtonService1->setEnabled(FALSE);
	PushButtonService2->setEnabled(FALSE);
	PushButtonService3->setEnabled(FALSE);
	PushButtonService4->setEnabled(FALSE);
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
	TextTextMessage->setEnabled(FALSE);
	TextTextMessage->setText("");

	LabelServiceLabel->setText(tr("Scanning..."));
}

/* change mode is only called when the mode REALLY has changed
 * so no conditionals are needed in this routine
 */

void FDRMDialog::ChangeGUIModeToDRM()
{
	show();

	/* Set correct schedule */
	pStationsDlg->SetCurrentSchedule(CDRMSchedule::SM_DRM);

	if (bStationsDlgWasVis == TRUE)
		pStationsDlg->show();

	if (bLiveSchedDlgWasVis == TRUE)
		pLiveScheduleDlg->show();

	if (bEPGDlgWasVis == TRUE)
		pEPGDlg->show();

	if (bSysEvalDlgWasVis == TRUE)
		pSysEvalDlg->show();

	if (bMultMedDlgWasVis == TRUE)
		pMultiMediaDlg->show();

	if (pStationsDlg->isVisible())
		pStationsDlg->LoadSchedule(CDRMSchedule::SM_DRM);

	eReceiverMode = RM_DRM;
}

/* Main window is not needed, hide it.
 * If DRM only windows are open, hide them.
 * Make sure analog demodulation dialog is visible
 */

void FDRMDialog::ChangeGUIModeToAM()
{
	/* Store visibility state */
	if (eReceiverMode != RM_NONE)
	{
		bSysEvalDlgWasVis = pSysEvalDlg->isVisible();
		bMultMedDlgWasVis = pMultiMediaDlg->isVisible();
		bEPGDlgWasVis = pEPGDlg->isVisible();
	}

	pSysEvalDlg->hide();
	pMultiMediaDlg->hide();
	pEPGDlg->hide();

	pSysEvalDlg->StopLogTimers();

	Timer.stop();

	this->hide();

	pAnalogDemDlg->show();

	/* Set correct schedule */
	pStationsDlg->SetCurrentSchedule(CDRMSchedule::SM_ANALOG);

	if (bStationsDlgWasVis == TRUE)
		pStationsDlg->show();

	if (bLiveSchedDlgWasVis == TRUE)
		pLiveScheduleDlg->show();

	if (pStationsDlg->isVisible())
		pStationsDlg->LoadSchedule(CDRMSchedule::SM_ANALOG);

	eReceiverMode = RM_AM;
}

void FDRMDialog::showEvent(QShowEvent*)
{
	/* Set timer for real-time controls */
	OnTimer();
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer */
	Timer.stop();
}

void FDRMDialog::OnSwitchToDRM()
{
	bStationsDlgWasVis = pStationsDlg->isVisible();
	bLiveSchedDlgWasVis = pLiveScheduleDlg->isVisible();

	DRMReceiver.SetReceiverMode(RM_DRM);
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FDRMDialog::OnNewDRMAcquisition()
{
	DRMReceiver.RequestNewAcquisition();
}

void FDRMDialog::OnSwitchToAM()
{
	bStationsDlgWasVis = pStationsDlg->isVisible();
	bLiveSchedDlgWasVis = pLiveScheduleDlg->isVisible();
	DRMReceiver.SetReceiverMode(RM_AM);
}

void FDRMDialog::OnButtonService1()
{
	if (PushButtonService1->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(0);
	}
	else
		PushButtonService1->setOn(TRUE);
}

void FDRMDialog::OnButtonService2()
{
	if (PushButtonService2->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(1);
	}
	else
		PushButtonService2->setOn(TRUE);

}

void FDRMDialog::OnButtonService3()
{
	if (PushButtonService3->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(2);
	}
	else
		PushButtonService3->setOn(TRUE);
}

void FDRMDialog::OnButtonService4()
{
	if (PushButtonService4->isOn())
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);

		SetService(3);
	}
	else
		PushButtonService4->setOn(TRUE);
}

void FDRMDialog::SetService(int iNewServiceID)
{
	CParameter& Parameters = *DRMReceiver.GetParameters();

	Parameters.Lock();

	Parameters.SetCurSelAudioService(iNewServiceID);
	Parameters.SetCurSelDataService(iNewServiceID);
	iCurSelServiceGUI = iNewServiceID;


	/* Eventually activate multimedia window */
	int iAppIdent = Parameters.Service[iNewServiceID].DataParam.iUserAppIdent;

	/* If service is only data service or has a multimedia content
	   , activate multimedia window */
	CService::ETyOServ eAudDataFlag = Parameters.Service[iNewServiceID].eAudDataFlag;
	Parameters.Unlock();
	if ((eAudDataFlag == CService::SF_DATA)
		|| (iAppIdent == AT_MOTSLISHOW)
		|| (iAppIdent == AT_JOURNALINE)
		|| (iAppIdent == AT_MOTBROADCASTWEBSITE))
	{
		OnViewMultiMediaDlg();
	}
}

void FDRMDialog::OnViewEvalDlg()
{
	if (DRMReceiver.GetReceiverMode() == RM_DRM)
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

void FDRMDialog::OnViewMultiMediaDlg()
{
	/* Show Multimedia window */
	pMultiMediaDlg->show();
}

void FDRMDialog::OnViewStationsDlg()
{
	/* Show stations window */
	pStationsDlg->show();
}

void FDRMDialog::OnViewLiveScheduleDlg()
{
	/* Show live schedule window */
	pLiveScheduleDlg->show();
}

void FDRMDialog::OnViewMultSettingsDlg()
{

	/* Show multimedia settings window */
	MultSettingsDlg* pMultSettingsDlg = new MultSettingsDlg(Settings, this, "", TRUE, Qt::WStyle_Dialog);

	SetDialogCaption(pMultSettingsDlg, tr("Multimedia settings"));

	pMultSettingsDlg->show();

}

void FDRMDialog::OnViewGeneralSettingsDlg()
{
	pGeneralSettingsDlg->show();
}

void FDRMDialog::OnViewEPGDlg()
{
	/* Show programme guide window */
	pEPGDlg->show();
}

void FDRMDialog::OnMenuSetDisplayColor()
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

void FDRMDialog::OnMenuPlotStyle(int value)
{
	/* Save new style in global variable */
	Settings.Put("DRM Dialog", "plotcolor", value);

	/* Set new plot style in other dialogs */
	pSysEvalDlg->UpdatePlotsStyle();
	pAnalogDemDlg->UpdatePlotsStyle();

	/* Taking care of the checks */
	for (int i = 0; i < NUM_AVL_COLOR_SCHEMES_PLOT; i++)
		pPlotStyleMenu->setItemChecked(i, i == value);
}

void FDRMDialog::closeEvent(QCloseEvent* ce)
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
	if(eReceiverMode == RM_DRM)
	{
		Settings.Put("GUI", "mode", string("DRMRX"));
		/* remember the state of the windows */
		Settings.Put("DRM Dialog", "visible", TRUE);
		Settings.Put("AM Dialog", "visible", FALSE);
		Settings.Put("Stations Dialog", "visible", pStationsDlg->isVisible());
		Settings.Put("Live Schedule Dialog", "visible", pLiveScheduleDlg->isVisible());
		Settings.Put("System Evaluation Dialog", "visible", pSysEvalDlg->isVisible());
		Settings.Put("Multimedia Dialog", "visible", pMultiMediaDlg->isVisible());
		Settings.Put("EPG Dialog", "visible", pEPGDlg->isVisible());

		/* stop any asynchronous GUI actions */
		pSysEvalDlg->StopLogTimers();
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
		if(!DRMReceiver.finished())
		{
			QMessageBox::critical(this, "Dream", "Exit\n",
				"Termination of working thread failed");
		}
	}
	else
	{
		Settings.Put("GUI", "mode", string("AMRX"));
		Settings.Put("DRM Dialog", "visible", FALSE);
		Settings.Put("AM Dialog", "visible", TRUE);
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

void FDRMDialog::customEvent(QCustomEvent* Event)
{
	if (Event->type() == QEvent::User + 11)
	{
		int iMessType = ((DRMEvent*) Event)->iMessType;
		int iStatus = ((DRMEvent*) Event)->iStatus;

		if (iMessType == MS_MOT_OBJ_STAT)
			pMultiMediaDlg->SetStatus(iMessType, iStatus);
		else
		{
			pSysEvalDlg->SetStatus(iMessType, iStatus);

			switch(iMessType)
			{
			case MS_FAC_CRC:
				CLED_FAC->SetLight(iStatus);
				break;

			case MS_SDC_CRC:
				CLED_SDC->SetLight(iStatus);
				break;

			case MS_MSC_CRC:
				CLED_MSC->SetLight(iStatus);
				break;

			case MS_RESET_ALL:
				CLED_FAC->Reset();
				CLED_SDC->Reset();
				CLED_MSC->Reset();
				break;
			}
		}
	}
}

QString FDRMDialog::GetCodecString(const int iServiceID)
{
	QString strReturn;

	CParameter& Parameters = *DRMReceiver.GetParameters();

	/* First check if it is audio or data service */
	if (Parameters.Service[iServiceID].eAudDataFlag == CService::SF_AUDIO)
	{
		/* Audio service */
		const CAudioParam::EAudSamRat eSamRate = Parameters.
			Service[iServiceID].AudioParam.eAudioSamplRate;

		/* Audio coding */
		switch (Parameters.Service[iServiceID].
			AudioParam.eAudioCoding)
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
		if (Parameters.Service[iServiceID].
			AudioParam.eSBRFlag == CAudioParam::SB_USED)
		{
			strReturn += "+";
		}
	}
	else
	{
		/* Data service */
		strReturn = "Data:";
	}

	return strReturn;
}

QString FDRMDialog::GetTypeString(const int iServiceID)
{
	QString strReturn;

	CParameter& Parameters = *DRMReceiver.GetParameters();

	/* First check if it is audio or data service */
	if (Parameters.Service[iServiceID].
		eAudDataFlag == CService::SF_AUDIO)
	{
		/* Audio service */
		/* Mono-Stereo */
		switch (Parameters.
			Service[iServiceID].AudioParam.eAudioMode)
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
		/* Data service */
		if (Parameters.Service[iServiceID].DataParam.
			ePacketModInd == CDataParam::PM_PACKET_MODE)
		{
			if (Parameters.Service[iServiceID].DataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
			{
				switch (Parameters.Service[iServiceID].DataParam.iUserAppIdent)
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

void FDRMDialog::SetDisplayColor(const QColor newColor)
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
			CurPal.setColor(QPalette::Active, QColorGroup::Text, black);
			CurPal.setColor(QPalette::Active, QColorGroup::Foreground, black);
			CurPal.setColor(QPalette::Inactive, QColorGroup::Text, black);
			CurPal.setColor(QPalette::Inactive, QColorGroup::Foreground, black);

			/* We need to specify special color for disabled */
			CurPal.setColor(QPalette::Disabled, QColorGroup::Light, black);
			CurPal.setColor(QPalette::Disabled, QColorGroup::Dark, black);
		}

		/* Set new palette */
		vecpWidgets[i]->setPalette(CurPal);
	}
}

void FDRMDialog::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* Text Message */
	QWhatsThis::add(TextTextMessage,
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

	QWhatsThis::add(TextLabelInputLevel, strInputLevel);
	QWhatsThis::add(ProgrInputLevel, strInputLevel);

	/* Status LEDs */
	const QString strStatusLEDS =
		tr("<b>Status LEDs:</b> The three status LEDs show "
		"the current CRC status of the three logical channels of a DRM stream. "
		"These LEDs are the same as the top LEDs on the Evaluation Dialog.");

	QWhatsThis::add(CLED_MSC, strStatusLEDS);
	QWhatsThis::add(CLED_SDC, strStatusLEDS);
	QWhatsThis::add(CLED_FAC, strStatusLEDS);

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

	QWhatsThis::add(LabelBitrate, strStationLabelOther);
	QWhatsThis::add(LabelCodec, strStationLabelOther);
	QWhatsThis::add(LabelStereoMono, strStationLabelOther);
	QWhatsThis::add(LabelServiceLabel, strStationLabelOther);
	QWhatsThis::add(LabelProgrType, strStationLabelOther);
	QWhatsThis::add(LabelServiceID, strStationLabelOther);
	QWhatsThis::add(LabelLanguage, strStationLabelOther);
	QWhatsThis::add(LabelCountryCode, strStationLabelOther);
	QWhatsThis::add(FrameAudioDataParams, strStationLabelOther);

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

	QWhatsThis::add(PushButtonService1, strServiceSel);
	QWhatsThis::add(PushButtonService2, strServiceSel);
	QWhatsThis::add(PushButtonService3, strServiceSel);
	QWhatsThis::add(PushButtonService4, strServiceSel);
	QWhatsThis::add(TextMiniService1, strServiceSel);
	QWhatsThis::add(TextMiniService2, strServiceSel);
	QWhatsThis::add(TextMiniService3, strServiceSel);
	QWhatsThis::add(TextMiniService4, strServiceSel);
}
