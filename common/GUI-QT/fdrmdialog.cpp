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


/* Implementation *************************************************************/
/* About dialog */
CAboutDlg::CAboutDlg(QWidget* parent, const char* name, bool modal, WFlags f)
	: CAboutDlgBase(parent, name, modal, f)
{
	/* Set the text for the about dialog html text control */
	TextViewCredits->setText(
		"<p>" /* General description of Dream software */
		"<big><b>Dream</b> is a software implementation of a Digital Radio "
		"Mondiale (DRM) receiver. All what is needed to receive DRM "
		"transmissions is a PC with a sound card and a modified analog "
		"short-wave (MW, LW) receiver.</big>"
		"</p><br>"
		"<p><font face=\"courier\">" /* GPL header text */
		"This program is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version.<br>This program is distributed in "
		"the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
		"even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
		"PARTICULAR PURPOSE. See the GNU General Public License for more "
		"details.<br>You should have received a copy of the GNU General Public "
		"License along with his program; if not, write to the Free Software "
		"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
		"USA"
		"</font></p><br>" /* Our warning text */
		"<p><font color=\"#ff0000\" face=\"courier\">"
		"Although this software is going to be "
		"distributed as free software under the terms of the GPL this does not "
		"mean that its use is free of rights of others. The use may infringe "
		"third party IP and thus may not be legal in some countries."
		"</font></p><br>"
		"<p>" /* Libraries used by this compilation of Dream */
		"<b>This compilation of Dream uses the following libraries:</b>"
		"</p>"
		"<ul>"
		"<li><b>FFTW</b> <i>http://www.fftw.org</i></li>"
#ifdef USE_FAAD2_LIBRARY
		"<li><b>FAAD2</b> <i>http://faac.sourceforge.net</i></li>"
#endif
#ifdef USE_FAAC_LIBRARY
		"<li><b>FAAC</b> <i>http://faac.sourceforge.net</i></li>"
#endif
#ifdef USE_QT_GUI /* QWT */
		"<li><b>QWT</b> <i>Dream is based in part on the work of the Qwt "
		"project (http://qwt.sf.net).</i></li>"
#endif
#ifdef HAVE_LIBHAMLIB
		"<li><b>Hamlib</b> <i>http://hamlib.sourceforge.net</i></li>"
#endif
#ifdef HAVE_JOURNALINE
		"<li><b>FhG IIS Journaline Decoder</b> <i>Features NewsService "
		"Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
		"Germany. For more information visit http://www.iis.fhg.de/dab</i></li>"
#endif
#ifdef HAVE_LIBFREEIMAGE
		"<li><b>FreeImage</b> <i>This software uses the FreeImage open source "
		"image library. See http://freeimage.sourceforge.net for details. "
		"FreeImage is used under the GNU GPL.</i></li>"
#endif
		"</ul>");

	/* Set version number in about dialog */
	QString strVersionText;
	strVersionText = "<center><b>Dream, Version ";
	strVersionText += VERSION;
	strVersionText += "</b><br> Open-Source Software Implementation of a "
		"DRM-Receiver<br>";
	strVersionText += "Under the GNU General Public License (GPL)</center>";
	TextLabelVersion->setText(strVersionText);
}


/* Main dialog */
FDRMDialog::FDRMDialog(QWidget* parent, const char* name, bool modal, WFlags f)
	: FDRMDialogBase(parent, name, modal, f)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(DRMReceiver.GeomFdrmdialog.iXPos,
		DRMReceiver.GeomFdrmdialog.iYPos,
		DRMReceiver.GeomFdrmdialog.iWSize,
		DRMReceiver.GeomFdrmdialog.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#endif


	/* Set Menu ***************************************************************/
	/* Help menu ------------------------------------------------------------ */
	QPopupMenu* HelpMenu = new QPopupMenu(this);
	CHECK_PTR(HelpMenu);
    HelpMenu->insertItem("What's &This", this ,
		SLOT(OnHelpWhatsThis()), SHIFT+Key_F1);
	HelpMenu->insertSeparator();
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

	/* Insert "Wave mapper". "iNumSoundDev" is no valid ID for a device, use
	   this for wave-mapper */
	pSoundInMenu->insertSeparator();
	pSoundInMenu->insertItem("Wave &Mapper Recording", this,
		SLOT(OnSoundInDevice(int)), 0, iNumSoundDev);
	pSoundOutMenu->insertSeparator();
	pSoundOutMenu->insertItem("Wave &Mapper Playback", this,
		SLOT(OnSoundOutDevice(int)), 0, iNumSoundDev);

	/* Set default device. If no valid device was selected, select
	   "Wave mapper" */
	int iDefaultInDev = DRMReceiver.GetSoundInterface()->GetInDev();
	if ((iDefaultInDev > iNumSoundDev) || (iDefaultInDev < 0))
		iDefaultInDev = iNumSoundDev;

	int iDefaultOutDev = DRMReceiver.GetSoundInterface()->GetOutDev();
	if ((iDefaultOutDev > iNumSoundDev) || (iDefaultOutDev < 0))
		iDefaultOutDev = iNumSoundDev;

	pSoundInMenu->setItemChecked(iDefaultInDev, TRUE);
	pSoundOutMenu->setItemChecked(iDefaultOutDev, TRUE);

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

	/* Check "visible" settings flag if values are possible */
	if ((DRMReceiver.GeomAnalogDemDlg.bVisible == TRUE) &&
		(DRMReceiver.GeomSystemEvalDlg.bVisible == TRUE))
	{
		/* It makes no sense that both windows are shown at the same time ->
		   disable both windows */
		DRMReceiver.GeomAnalogDemDlg.bVisible = FALSE;
		DRMReceiver.GeomSystemEvalDlg.bVisible = FALSE;
	}

	/* Analog demodulation window */
	pAnalogDemDlg = new AnalogDemDlg(this, "Analog Demodulation", FALSE,
		Qt::WStyle_MinMax);

	if (DRMReceiver.GeomAnalogDemDlg.bVisible == TRUE)
		pAnalogDemDlg->show();
	else
		pAnalogDemDlg->hide();

	/* Stations window */
	pStationsDlg = new StationsDlg(this, "Stations", FALSE, Qt::WStyle_MinMax);
	if (DRMReceiver.GeomStationsDlg.bVisible == TRUE)
		pStationsDlg->show();
	else
		pStationsDlg->hide();

	/* Evaluation window */
	pSysEvalDlg = new systemevalDlg(this, "System Evaluation", FALSE,
		Qt::WStyle_MinMax);

	if (DRMReceiver.GeomSystemEvalDlg.bVisible == TRUE)
		pSysEvalDlg->show();
	else
		pSysEvalDlg->hide();

	/* Multimedia window */
	pMultiMediaDlg = new MultimediaDlg(this, "Multimedia", FALSE,
		Qt::WStyle_MinMax);

	if (DRMReceiver.GeomMultimediaDlg.bVisible == TRUE)
		pMultiMediaDlg->show();
	else
		pMultiMediaDlg->hide();

	/* Enable multimedia */
	DRMReceiver.GetParameters()->EnableMultimedia(TRUE);

	/* Init current selected service */
	DRMReceiver.GetParameters()->ResetCurSelAudDatServ();
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

FDRMDialog::~FDRMDialog()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	DRMReceiver.GeomFdrmdialog.iXPos = WinGeom.x();
	DRMReceiver.GeomFdrmdialog.iYPos = WinGeom.y();
	DRMReceiver.GeomFdrmdialog.iHSize = WinGeom.height();
	DRMReceiver.GeomFdrmdialog.iWSize = WinGeom.width();

	/* Set "visible" flags for settings */
	if (pAnalogDemDlg->isVisible())
		DRMReceiver.GeomAnalogDemDlg.bVisible = TRUE;
	else
		DRMReceiver.GeomAnalogDemDlg.bVisible = FALSE;

	if (pStationsDlg->isVisible())
		DRMReceiver.GeomStationsDlg.bVisible = TRUE;
	else
		DRMReceiver.GeomStationsDlg.bVisible = FALSE;

	if (pSysEvalDlg->isVisible())
		DRMReceiver.GeomSystemEvalDlg.bVisible = TRUE;
	else
		DRMReceiver.GeomSystemEvalDlg.bVisible = FALSE;

	if (pMultiMediaDlg->isVisible())
		DRMReceiver.GeomMultimediaDlg.bVisible = TRUE;
	else
		DRMReceiver.GeomMultimediaDlg.bVisible = FALSE;
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

	/* Check the receiver mode for showing the correct evaluation window */
	if (DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_DRM)
	{
		/* DRM: 0, AM: 1 */
		if (pReceiverModeMenu->isItemChecked(1) == TRUE)
		{
			pReceiverModeMenu->setItemChecked(1, FALSE);
			pReceiverModeMenu->setItemChecked(0, TRUE);
		}

		if (pAnalogDemDlg->isVisible())
		{
			pAnalogDemDlg->hide();
			pSysEvalDlg->show();
		}
	}

	if (DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_AM)
	{
		/* DRM: 0, AM: 1 */
		if (pReceiverModeMenu->isItemChecked(0) == TRUE)
		{
			pReceiverModeMenu->setItemChecked(0, FALSE);
			pReceiverModeMenu->setItemChecked(1, TRUE);
		}

		if (pSysEvalDlg->isVisible())
		{
			pSysEvalDlg->hide();
			pAnalogDemDlg->show();
		}
	}
}

void FDRMDialog::OnReceiverMode(int id)
{
	switch (id)
	{
	case 0:
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);
		if (pAnalogDemDlg->isVisible())
		{
			pAnalogDemDlg->hide();
			pSysEvalDlg->show();
		}
		break;

	case 1:
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_AM);
		if (pSysEvalDlg->isVisible())
		{
			pSysEvalDlg->hide();
			pAnalogDemDlg->show();
		}
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

// TODO: use QActionGroup instead

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
	if (DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_DRM)
	{
		/* Show evauation window in DRM mode */
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
	/* Show evaluation window */
	pMultiMediaDlg->show();
}

void FDRMDialog::OnViewStationsDlg()
{
	/* Show evauation window */
	pStationsDlg->show();
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

				case 0x44A: /* Journaline */
					strReturn += "NewsService Journaline";
					break;
				}
			}
			else
				strReturn += "Unknown Service";
		}
		else
			strReturn += "Unknown Service";
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

void FDRMDialog::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* Text Message */
	QWhatsThis::add(TextTextMessage,
		"<b>Text Message:</b> On the top right the text message label is "
		"shown. This label only appears when an actual text message is "
		"transmitted. If the current service does not transmit a text "
		"message, the label will be invisible.");

	/* Input Level */
	const QString strInputLevel =
		"<b>Input Level:</b> The input level meter shows the relative input "
		"signal peak level in dB. If the level is too high, the meter turns "
		"from green to red. The red region should be avoided since overload "
		"causes distortions which degrade the reception performance. Too low "
		"levels should be avoided too, since in this case the Signal-to-Noise "
		"Ratio (SNR) degrades.";

	QWhatsThis::add(TextLabelInputLevel, strInputLevel);
	QWhatsThis::add(ProgrInputLevel, strInputLevel);

	/* Status LEDs */
	const QString strStatusLEDS =
		"<b>Status LEDs:</b> The three status LEDs show the current CRC status "
		"of the three logical channels of a DRM stream. These LEDs are the same "
		"as the top LEDs on the Evaluation Dialog.";

	QWhatsThis::add(TextLabelStatusLEDs, strStatusLEDS);
	QWhatsThis::add(CLED_MSC, strStatusLEDS);
	QWhatsThis::add(CLED_SDC, strStatusLEDS);
	QWhatsThis::add(CLED_FAC, strStatusLEDS);

	/* Station Label and Info Display */
	const QString strStationLabelOther =
		"<b>Station Label and Info Display:</b> In the big label with the "
		"black background the station label and some other information about "
		"the current selected service is displayed. The red text on the top "
		"shows the audio compression format (e.g. AAC), the sample rate of the "
		"core coder without SBR (e.g. 24 kHz), if SBR is used and what audio "
		"mode is used (mono, stereo, P-stereo -> low-complexity or parametric "
		"stereo). In case SBR is used, the actual sample rate is twice the "
		"sample rate of the core AAC decoder. The next two types of "
		"information are the language and the program type of the service "
		"(e.g. German / News).<br>The big turquoise text in the middle is "
		"the station label. This label may appear later than the red text "
		"since this information is transmitted in a different logical channel "
		"of DRM.<br>The turquoise text on the bottom shows the gross bit-rate "
		"in kbits per second of the current selected service. The abbreviation "
		"EEP and UEP stands for Equal Error Protection and Unequal Error "
		"Protection. UEP is a feature of DRM for a graceful degradation of "
		"the decoded audio signal in case of a bad reception situation. UEP "
		"means that some parts of the audio is higher protected and some parts "
		"are lower protected. On the right the ID number connected with this "
		"service is shown.";

	QWhatsThis::add(TextServiceAudio, strStationLabelOther);
	QWhatsThis::add(TextServiceLabel, strStationLabelOther);
	QWhatsThis::add(TextServiceIDRate, strStationLabelOther);

	/* Service Selectors */
	const QString strServiceSel =
		"<b>Service Selectors:</b> In a DRM stream up to four services can be "
		"carried. The service type can either be audio, data or audio and "
		"data. If a data service is selected, the Multimedia Dialog will "
		"automatically show up. On the right of each service selection button "
		"a short description of the service is shown. If a service is an audio "
		"and data service, a \"+ MM\" is added to this text. If a service is "
		"an audio and data service and this service is selected, by opening "
		"the Multimedia Dialog, the data can be viewed while the audio is "
		"still playing.";

	QWhatsThis::add(PushButtonService1, strServiceSel);
	QWhatsThis::add(PushButtonService2, strServiceSel);
	QWhatsThis::add(PushButtonService3, strServiceSel);
	QWhatsThis::add(PushButtonService4, strServiceSel);
	QWhatsThis::add(TextMiniService1, strServiceSel);
	QWhatsThis::add(TextMiniService2, strServiceSel);
	QWhatsThis::add(TextMiniService3, strServiceSel);
	QWhatsThis::add(TextMiniService4, strServiceSel);

	/* Dream Logo */
	QWhatsThis::add(PixmapLabelDreamLogo,
		"<b>Dream Logo:</b> This is the official logo of the Dream software.");
}
