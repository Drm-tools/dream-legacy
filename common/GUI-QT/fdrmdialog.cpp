/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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


FDRMDialog::FDRMDialog(QWidget* parent, const char* name, bool modal, WFlags f)
	: FDRMDialogBase(parent, name, modal, f), AboutDlg(parent, 0, TRUE)
{
	/* Set version number in about dialog */
	QString strVersionText;
	strVersionText = "<center><b>Dream, Version ";
	strVersionText += DREAM_VERSION_NUMBER;
	strVersionText += "</b><br> Open-Source Software Implementation of a "
		"DRM-Receiver<br>";
	strVersionText += "Under the GNU General Public License (GPL)</center>";
	AboutDlg.TextLabelVersion->setText(strVersionText);


	/* Set Menu ***************************************************************/
	/* Help menu ------------------------------------------------------------ */
	QPopupMenu* HelpMenu = new QPopupMenu(this);
	CHECK_PTR(HelpMenu);
	HelpMenu->insertItem("&About...", this, SLOT(OnHelpAbout()));


	/* View menu ------------------------------------------------------------ */
	QPopupMenu* EvalWinMenu = new QPopupMenu(this);
	CHECK_PTR(EvalWinMenu);
	EvalWinMenu->insertItem("&Evaluation Dialog...", this,
		SLOT(OnViewEvalDlg()), CTRL+Key_E);
	EvalWinMenu->insertItem("M&ultimedia Dialog...", this,
		SLOT(OnViewMultiMediaDlg()), CTRL+Key_U);
	EvalWinMenu->insertItem("S&tations Dialog...", this,
		SLOT(OnViewStationsDlg()), CTRL+Key_T);
	EvalWinMenu->insertSeparator();
	EvalWinMenu->insertItem("E&xit", this, SLOT(close()), CTRL+Key_Q);


	/* Settings menu  ------------------------------------------------------- */
	pSoundInMenu = new QPopupMenu(this);
	CHECK_PTR(pSoundInMenu);
	pSoundOutMenu = new QPopupMenu(this);
	CHECK_PTR(pSoundOutMenu);
	pReceiverModeMenu = new QPopupMenu(this);
	CHECK_PTR(pReceiverModeMenu);

	/* Get sound device names */
	iNumSoundDev = DRMReceiver.GetSoundInterface()->GetNumDev();
	for (int i = 0; i < iNumSoundDev; i++)
	{
		string strName = DRMReceiver.GetSoundInterface()->GetDeviceName(i);
		pSoundInMenu->insertItem(QString(strName.c_str()), this,
			SLOT(OnSoundInDevice(int)), 0, i);
		pSoundOutMenu->insertItem(QString(strName.c_str()), this,
			SLOT(OnSoundOutDevice(int)), 0, i);
	}

	/* Set wave mapper as default device. "iNumSoundDev" is no
	   valid ID for a device, use this for wave-mapper */
	pSoundInMenu->insertSeparator();
	pSoundInMenu->insertItem("Wave &Mapper Recording", this,
		SLOT(OnSoundInDevice(int)), 0, iNumSoundDev);
	pSoundOutMenu->insertSeparator();
	pSoundOutMenu->insertItem("Wave &Mapper Playback", this,
		SLOT(OnSoundOutDevice(int)), 0, iNumSoundDev);

	pSoundInMenu->setItemChecked(iNumSoundDev, TRUE);
	pSoundOutMenu->setItemChecked(iNumSoundDev, TRUE);
	DRMReceiver.GetSoundInterface()->SetInDev(iNumSoundDev);
	DRMReceiver.GetSoundInterface()->SetOutDev(iNumSoundDev);

	/* Reiceiver mode menu */
	pReceiverModeMenu->insertItem("DRM (digital)", this,
		SLOT(OnReceiverMode(int)), CTRL+Key_D, 0);
	pReceiverModeMenu->insertItem("AM (analog)", this,
		SLOT(OnReceiverMode(int)), CTRL+Key_A, 1);

	/* Default is DRM mode */
	pReceiverModeMenu->setItemChecked(0, 1);
	DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);


	pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem("Sound &In", pSoundInMenu);
	pSettingsMenu->insertItem("Sound &Out", pSoundOutMenu);
	pSettingsMenu->insertSeparator();
	pSettingsMenu->insertItem("&Receiver Mode", pReceiverModeMenu);


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&View", EvalWinMenu);
	pMenu->insertItem("&Settings", pSettingsMenu);
	pMenu->insertItem("&?", HelpMenu);
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	FDRMDialogBaseLayout->setMenuBar(pMenu);


	/* Digi controls */
	/* Reset text */
	TextServiceIDRate->setText("");
	TextServiceLabel->setText("");
	TextServiceAudio->setText("");
	

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-12.5);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));


	/* Stations window */
	pStationsDlg = new StationsDlg(this, "Stations", FALSE, 
		Qt::WGroupLeader | Qt::WStyle_MinMax);
	pStationsDlg->hide();

	/* Evaluation window ("WGroupLeader" flag enabels that in both windows 
	   controls can be clicked) */
	pSysEvalDlg = new systemevalDlg(this, "System Evaluation", FALSE, 
		Qt::WGroupLeader | Qt::WStyle_MinMax);
	pSysEvalDlg->hide();

	/* Multimedia window */
	pMultiMediaDlg = new MultimediaDlg(this, "MOT Slide Show", FALSE, 
			Qt::WGroupLeader | Qt::WStyle_MinMax);
	pMultiMediaDlg->hide();

	/* Enable multimedia */
	DRMReceiver.GetParameters()->EnableMultimedia(TRUE);

	/* Init current selected service */
	DRMReceiver.GetParameters()->SetCurSelAudioService(0);
	DRMReceiver.GetParameters()->SetCurSelDataService(0);
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

	connect(&Timer, SIGNAL(timeout()), 
		this, SLOT(OnTimer()));

	/* Disable text message label */
	TextTextMessage->setText("");
	TextTextMessage->hide();

	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimer();

#ifdef _DEBUG_
OnViewEvalDlg();
#endif
}

void FDRMDialog::OnTimer()
{
	int iCurSelServ;

	/* Input level meter */
	ProgrInputLevel->setValue(DRMReceiver.GetReceiver()->GetLevelMeter());

	/* Check if receiver does receive a DRM signal */
	if ((DRMReceiver.GetReceiverState() == CDRMReceiver::AS_WITH_SIGNAL) &&
		(DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_DRM))
	{
		/* Receiver does receive a DRM signal ------------------------------- */
		/* First get current selected service */
		iCurSelServ = DRMReceiver.GetParameters()->GetCurSelAudioService();

		/* If selected service is audio and text message is true */
		if ((DRMReceiver.GetParameters()->Service[iCurSelServ].
			eAudDataFlag == CParameter::SF_AUDIO) &&
			(DRMReceiver.GetParameters()->Service[iCurSelServ].
			AudioParam.bTextflag == TRUE))
		{
			/* Activate text window */
			TextTextMessage->show();
			
			/* Text message of current selected audio service 
			   (UTF-8 decoding) */
			TextTextMessage->setText(QString().fromUtf8(QCString(
				DRMReceiver.GetParameters()->Service[iCurSelServ].AudioParam.
				strTextMessage.c_str())));
		}
		else
		{
			/* Deactivate text window */
			TextTextMessage->hide();

			/* Clear Text */
			TextTextMessage->setText("");
		}

		/* Check whether service parameters were not transmitted yet */
		if (DRMReceiver.GetParameters()->Service[iCurSelServ].IsActive())
		{
			/* Service label (UTF-8 encoded string -> convert) */
			TextServiceLabel->setText(QString().fromUtf8(QCString(
				DRMReceiver.GetParameters()->Service[iCurSelServ].
				strLabel.c_str())));

			TextServiceIDRate->setText(SetBitrIDStr(iCurSelServ));

			/* Audio informations digi-string */
			TextServiceAudio->setText(SetServParamStr(iCurSelServ));
		}
		else
		{
			TextServiceLabel->setText(QString("No Service"));

			TextServiceIDRate->setText("");
			TextServiceAudio->setText("");
		}


		/* Update service selector ------------------------------------------ */
		if (iCurSelServiceGUI != iCurSelServ)
		{
			/* Reset checks */
			PushButtonService1->setOn(FALSE);
			PushButtonService2->setOn(FALSE);
			PushButtonService3->setOn(FALSE);
			PushButtonService4->setOn(FALSE);

			/* Set right flag */
			switch (iCurSelServ)
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


		/* Service selector ------------------------------------------------- */
		QString strSpace = "   |   ";

		/* Enable only so many number of channel switches as present in the 
		   stream */
		int iNumServices = DRMReceiver.GetParameters()->GetTotNumServices();

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
			if (DRMReceiver.GetParameters()->Service[i].IsActive())
			{
				/* Do UTF-8 to string converion with the label strings */
				QString strLabel = QString().fromUtf8(QCString(DRMReceiver.
					GetParameters()->Service[i].strLabel.c_str()));

				/* Print out label in bold letters (rich text). Problem, if 
				   html tags are used in the label: FIXME */
				m_StaticService[i] = "<b>" + strLabel + 
					"</b>" + strSpace + SetServParamStr(i);

				/* Show, if a multimedia stream is connected to this service */
				if ((DRMReceiver.GetParameters()->Service[i].
					eAudDataFlag == CParameter::SF_AUDIO) && 
					(DRMReceiver.GetParameters()->Service[i].
					DataParam.iStreamID != STREAM_ID_NOT_USED))
				{
					m_StaticService[i] += " + MM";
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

		/* Set texts */
		TextMiniService1->setText(m_StaticService[0]);
		TextMiniService2->setText(m_StaticService[1]);
		TextMiniService3->setText(m_StaticService[2]);
		TextMiniService4->setText(m_StaticService[3]);
	}
	else
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
		TextServiceAudio->setText("");
		TextServiceIDRate->setText("");

		/* Hide text message label */
		TextTextMessage->hide();
		TextTextMessage->setText("");

		if (DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_DRM)
			TextServiceLabel->setText(QString("Scanning..."));
		else
		{
			TextServiceLabel->setText(QString("Analog AM Mode"));
			TextServiceIDRate->setText("Press Ctrl+A for new Acquisition, "
				"Ctrl+D for DRM");
		}
	}
}

void FDRMDialog::OnReceiverMode(int id)
{
	switch (id)
	{
	case 0:
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);
		break;

	case 1:
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_AM);
		break;
	}

	/* Taking care of checks in the menu */
	pReceiverModeMenu->setItemChecked(0, 0 == id);
	pReceiverModeMenu->setItemChecked(1, 1 == id);
}

void FDRMDialog::OnSoundInDevice(int id)
{
	DRMReceiver.GetSoundInterface()->SetInDev(id);

	/* Taking care of checks in the menu. "+ 1" because of wave mapper entry */
	for (int i = 0; i < iNumSoundDev + 1; i++)
		pSoundInMenu->setItemChecked(i, i == id);
}

void FDRMDialog::OnSoundOutDevice(int id)
{
	DRMReceiver.GetSoundInterface()->SetOutDev(id);

	/* Taking care of checks in the menu. "+ 1" because of wave mapper entry */
	for (int i = 0; i < iNumSoundDev + 1; i++)
		pSoundOutMenu->setItemChecked(i, i == id);
}

void FDRMDialog::OnButtonService1()
{
	/* If button was already down */
	if (DRMReceiver.GetParameters()->GetCurSelAudioService() == 0)
		PushButtonService1->setOn(TRUE);
	else
	{
		/* Set all other buttons up */
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(0);
	}
}

void FDRMDialog::OnButtonService2()
{
	/* If button was already down */
	if (DRMReceiver.GetParameters()->GetCurSelAudioService() == 1)
		PushButtonService2->setOn(TRUE);
	else
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(1);
	}
}

void FDRMDialog::OnButtonService3()
{
	/* If button was already down */
	if (DRMReceiver.GetParameters()->GetCurSelAudioService() == 2)
		PushButtonService3->setOn(TRUE);
	else
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService4->isOn()) PushButtonService4->setOn(FALSE);

		SetService(2);
	}
}

void FDRMDialog::OnButtonService4()
{
	/* If button was already down */
	if (DRMReceiver.GetParameters()->GetCurSelAudioService() == 3)
		PushButtonService4->setOn(TRUE);
	else
	{
		/* Set all other buttons up */
		if (PushButtonService1->isOn()) PushButtonService1->setOn(FALSE);
		if (PushButtonService2->isOn()) PushButtonService2->setOn(FALSE);
		if (PushButtonService3->isOn()) PushButtonService3->setOn(FALSE);

		SetService(3);
	}
}

void FDRMDialog::SetService(int iNewServiceID)
{
	DRMReceiver.GetParameters()->SetCurSelAudioService(iNewServiceID);
	DRMReceiver.GetParameters()->SetCurSelDataService(iNewServiceID);
	iCurSelServiceGUI = iNewServiceID;

	/* If service is only data service, activate multimedia window */
	if (DRMReceiver.GetParameters()->Service[iNewServiceID].eAudDataFlag ==
		CParameter::SF_DATA)
	{
		OnViewMultiMediaDlg();
	}
}

void FDRMDialog::OnViewEvalDlg()
{
	/* Show evauation window */
	pSysEvalDlg->show();
}

void FDRMDialog::OnViewMultiMediaDlg()
{
	/* Show evaluation window */
	pMultiMediaDlg->show();
}

void FDRMDialog::OnViewStationsDlg()
{
	/* Show evauation window */
	pStationsDlg->show();
}

void FDRMDialog::OnHelpAbout()
{
	AboutDlg.exec();
}

QString	FDRMDialog::SetServParamStr(int iServiceID)
{
	QString strReturn;

	if (DRMReceiver.GetParameters()->Service[iServiceID].
		eAudDataFlag == CParameter::SF_AUDIO)
	{
		/* Audio service ---------------------------------------------------- */
		/* Audio coding */
		switch (DRMReceiver.GetParameters()->Service[iServiceID].
			AudioParam.eAudioCoding)
		{
		case CParameter::AC_AAC:	
			strReturn = "AAC(";
			break;

		case CParameter::AC_CELP:
			strReturn = "Celp(";
			break;

		case CParameter::AC_HVXC:
			strReturn = "HVXC(";
			break;
		}

		/* Sample rate */
		switch (DRMReceiver.GetParameters()->Service[iServiceID].
			AudioParam.eAudioSamplRate)
		{
		case CParameter::AS_8_KHZ:	
			strReturn += "8 kHz)";
			break;

		case CParameter::AS_12KHZ:	
			strReturn += "12 kHz)";
			break;

		case CParameter::AS_16KHZ:	
			strReturn += "16 kHz)";
			break;

		case CParameter::AS_24KHZ:	
			strReturn += "24 kHz)";
			break;
		}

		/* SBR */
		if (DRMReceiver.GetParameters()->Service[iServiceID].
			AudioParam.eSBRFlag == CParameter::SB_USED)
		{
			strReturn += "+SBR";
		}

		/* Mono-Stereo */
		switch (DRMReceiver.GetParameters()->
			Service[iServiceID].AudioParam.eAudioMode)
		{
			case CParameter::AM_MONO:
				strReturn += " Mono";
				break;

			case CParameter::AM_P_STEREO:
				strReturn += " P-Stereo";
				break;

			case CParameter::AM_STEREO:
				strReturn += " Stereo";
				break;
		}

		/* Language */
		strReturn += " / ";
		strReturn += 
			strTableLanguageCode[DRMReceiver.GetParameters()->Service[
			iServiceID].iLanguage].c_str();

		/* Program type */
		strReturn += " / ";
		strReturn += 
			strTableProgTypCod[DRMReceiver.GetParameters()->Service[
			iServiceID].iServiceDescr].c_str();
	}
	else
	{
		/* Data service ----------------------------------------------------- */
		strReturn = "Data Service: ";

		if (DRMReceiver.GetParameters()->Service[iServiceID].DataParam.
			ePacketModInd == CParameter::PM_PACKET_MODE)
		{
			if (DRMReceiver.GetParameters()->Service[iServiceID].DataParam.
				eAppDomain == CParameter::AD_DAB_SPEC_APP)
			{
				switch (DRMReceiver.GetParameters()->Service[iServiceID].
					DataParam.iUserAppIdent)
				{
				case 1:
					strReturn += "Dynamic labels";
					break;

				case 2:
					strReturn += "MOT Slideshow";
					break;

				case 3:
					strReturn += "MOT Broadcast Web Site";
					break;

				case 4:
					strReturn += "TPEG";
					break;

				case 5:
					strReturn += "DGPS";
					break;
				}
			}
			else
			{
				strReturn += "Packet Mode";
				
				switch (DRMReceiver.GetParameters()->Service[iServiceID].
					DataParam.eDataUnitInd)
				{
				case CParameter::DU_SINGLE_PACKETS:
					strReturn += " (Single Packets)";
					break;

				case CParameter::DU_DATA_UNITS:
					strReturn += " (Data Units)";
					break;
				}

				QString strTemp;

				strTemp.setNum(DRMReceiver.GetParameters()->
					Service[iServiceID].DataParam.iPacketID);
				strReturn += " / ID: " + strTemp;

				strTemp.setNum(DRMReceiver.GetParameters()->
					Service[iServiceID].DataParam.iPacketLen);
				strReturn += " / Len: " + strTemp;
			}
		}
		else
			strReturn += " Synchronous Stream Mode";
	}

	return strReturn;
}

QString	FDRMDialog::SetBitrIDStr(int iServiceID)
{
	/* Bit-rate */
	QString strServIDBitrate = "Bit Rate:" + QString().setNum(
		DRMReceiver.GetParameters()->GetBitRate(iServiceID), 'f', 2) + " kbps";

	/* Equal or unequal error protection */
	if (DRMReceiver.GetParameters()->IsEEP(iServiceID) == TRUE)
		strServIDBitrate += " EEP";
	else
		strServIDBitrate += " UEP";

	/* Service ID */
	strServIDBitrate += " / ID:";
	strServIDBitrate += 
		QString().setNum((long) DRMReceiver.GetParameters()->
		Service[iServiceID].iServiceID);

	return strServIDBitrate;
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
