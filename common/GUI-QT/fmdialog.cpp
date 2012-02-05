/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2010
 *
 * Author(s):
 *	Julian Cable
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

#include "fmdialog.h"
#include "DialogUtil.h"
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qwt_thermo.h>
#if QT_VERSION < 0x040000
# include <qwhatsthis.h>
# define Q3WhatsThis QWhatsThis
#else
# include <q3whatsthis.h>
# include <q3cstring.h>
# include <QShowEvent>
# include <QHideEvent>
# include <QCustomEvent>
# include <QCloseEvent>
# include <QEvent>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
#endif

/* Implementation *************************************************************/
FMDialog::FMDialog(CDRMReceiver& NDRMR, CSettings& NSettings, CRig& rig,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f):
	FMDialogBase(parent, name, modal, f),
	DRMReceiver(NDRMR), Settings(NSettings), eReceiverMode(RM_NONE),
	alarmBrush(QColor(255, 0, 0))
{
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("FM Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

#if QT_VERSION < 0x040000
        /* Set Menu ***************************************************************/
        /* View menu ------------------------------------------------------------ */
        Q3PopupMenu* ViewMenu = new Q3PopupMenu(this);
        CHECK_PTR(ViewMenu);
        ViewMenu->insertItem(tr("&Tune"), this, SLOT(OnTune()), Qt::CTRL+Qt::Key_T);
        ViewMenu->insertSeparator();
        ViewMenu->insertItem(tr("E&xit"), this, SLOT(close()), Qt::CTRL+Qt::Key_Q, 5);

        /* Settings menu  ------------------------------------------------------- */
        pSettingsMenu = new Q3PopupMenu(this);
        CHECK_PTR(pSettingsMenu);
        pSettingsMenu->insertItem(tr("&Sound Card Selection"),
                new CSoundCardSelMenu(DRMReceiver.GetSoundInInterface(),
                DRMReceiver.GetSoundOutInterface(), this));

        pSettingsMenu->insertItem(tr("&DRM (digital)"), this,
                SLOT(OnSwitchToDRM()), Qt::CTRL+Qt::Key_D);
        pSettingsMenu->insertItem(tr("&AM (analog)"), this,
                SLOT(OnSwitchToAM()), Qt::CTRL+Qt::Key_A);
        pSettingsMenu->insertSeparator();

        /* Remote menu  --------------------------------------------------------- */
        RemoteMenu* pRemoteMenu = new RemoteMenu(this, rig);
        pSettingsMenu->insertItem(tr("Set &Rig..."), pRemoteMenu->menu(), Qt::CTRL+Qt::Key_R);

        pSettingsMenu->insertItem(tr("Set D&isplay Color..."), this,
                SLOT(OnMenuSetDisplayColor()));
        /* Main menu bar -------------------------------------------------------- */
        pMenu = new QMenuBar(this);
        CHECK_PTR(pMenu);
        pMenu->insertItem(tr("&View"), ViewMenu);
        pMenu->insertItem(tr("&Settings"), pSettingsMenu);
        pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
        pMenu->setSeparator(QMenuBar::InWindowsStyle);

        /* Now tell the layout about the menu */
        FMDialogBaseLayout->setMenuBar(pMenu);
#else
	connect(actionTune, SIGNAL(triggered()), this, SLOT(OnTune()));
	connect(actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(actionAM, SIGNAL(triggered()), this, SLOT(OnSwitchToAM()));
	connect(actionDRM, SIGNAL(triggered()), this, SLOT(OnSwitchToDRM()));
	connect(actionDisplayColor, SIGNAL(triggered()), this, SLOT(OnMenuSetDisplayColor()));

	menu_Settings->addMenu(
		new CSoundCardSelMenu(
			DRMReceiver.GetSoundInInterface(), DRMReceiver.GetSoundOutInterface(), this
		)
	);
	//menu_Settings->addMenu(pRemoteMenu->menu());
	menubar->addMenu(new CDreamHelpMenu(this));
#endif

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
#if QT_VERSION < 0x040000
	ProgrInputLevel->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
#else
	ProgrInputLevel->setOrientation(Qt::Vertical, QwtThermo::LeftScale);
#endif
        QBrush fillBrush(QColor(0, 190, 0));
	ProgrInputLevel->setFillBrush(fillBrush);
	ProgrInputLevel->setAlarmLevel(-12.5);
	ProgrInputLevel->setAlarmBrush(alarmBrush);

	/* Update times for color LEDs */
	CLED_FAC->SetUpdateTime(1500);
	CLED_SDC->SetUpdateTime(1500);
	CLED_MSC->SetUpdateTime(600);

	/* Connect buttons */

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));

	/* Activate real-time timers */
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

FMDialog::~FMDialog()
{
}

void FMDialog::OnSwitchToDRM()
{
	emit SwitchMode(RM_DRM);
}

void FMDialog::OnSwitchToAM()
{
	emit SwitchMode(RM_AM);
}

void FMDialog::OnTune()
{
	bool ok;
	double freq = double(DRMReceiver.GetFrequency())/1000.0;
	double f = QInputDialog::getDouble(tr("Dream FM"),
					tr("Frequency (MHz):"), freq, 86.0, 110.0, 2, &ok);
	if (ok)
	{
		DRMReceiver.SetFrequency(int(1000.0*f));
	}
}

void FMDialog::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
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

void FMDialog::OnTimer()
{
	ERecMode eNewReceiverMode = DRMReceiver.GetReceiverMode();
	switch(eNewReceiverMode)
	{
	case RM_DRM:
		this->hide();
		break;
	case RM_AM:
		this->hide();
		break;
	case RM_FM:
		{
			CParameter& Parameters = *DRMReceiver.GetParameters();
			Parameters.Lock();

			/* Input level meter */
			ProgrInputLevel->setValue(Parameters.GetIFSignalLevel());

			SetStatus(CLED_MSC, Parameters.ReceiveStatus.Audio.GetStatus());
			SetStatus(CLED_SDC, Parameters.ReceiveStatus.SDC.GetStatus());
			SetStatus(CLED_FAC, Parameters.ReceiveStatus.FAC.GetStatus());

			int freq = DRMReceiver.GetFrequency();
			QString fs = QString("%1 MHz").arg(double(freq)/1000.0, 5, 'f', 2);

			LabelServiceLabel->setText(fs);

			Parameters.Unlock();

			/* Check if receiver does receive a signal */
			if(DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
				UpdateDisplay();
			else
				ClearDisplay();
		}
		break;
	default: // wait until working thread starts operating
		break;
	}
}

void FMDialog::UpdateDisplay()
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

	/* Check whether service parameters were not transmitted yet */
	if (Parameters.Service[iCurSelAudioServ].IsActive())
	{
		/* Service label (UTF-8 encoded string -> convert)
		LabelServiceLabel->setText(QString().fromUtf8(QCString(
			Parameters.Service[iCurSelAudioServ].
			strLabel.c_str())));
*/
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
		//LabelServiceLabel->setText(tr("No Service"));

		LabelBitrate->setText("");
		LabelCodec->setText("");
		LabelStereoMono->setText("");
		LabelProgrType->setText("");
		LabelLanguage->setText("");
		LabelCountryCode->setText("");
		LabelServiceID->setText("");
	}


	/* Service selector ------------------------------------------------- */
	/* Enable only so many number of channel switches as present in the stream */
	const int iNumServices = Parameters.GetTotNumServices();

	QString m_StaticService[MAX_NUM_SERVICES] = {"", "", "", ""};

	/* detect if AFS informations are available */
	if ((Parameters.AltFreqSign.vecMultiplexes.size() > 0) || (Parameters.AltFreqSign.vecOtherServices.size() > 0))
	{
		/* show AFS label */
		if (Parameters.Service[0].eAudDataFlag
				== CService::SF_AUDIO) m_StaticService[0] += tr(" + AFS");
	}

	/* set data service to be decoded to EPG until user selects something else */
	int iEPGServiceID=-1;

	Parameters.Unlock();
}

void FMDialog::ClearDisplay()
{
	/* Main text labels */
	LabelBitrate->setText("");
	LabelCodec->setText("");
	LabelStereoMono->setText("");
	LabelProgrType->setText("");
	LabelLanguage->setText("");
	LabelCountryCode->setText("");
	LabelServiceID->setText("");
	//LabelServiceLabel->setText(tr("Scanning..."));
}

void FMDialog::showEvent(QShowEvent*)
{
	/* Set timer for real-time controls */
	OnTimer();
 	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void FMDialog::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer */
	Timer.stop();
}

void FMDialog::SetService(int iNewServiceID)
{
	CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock();
	Parameters.SetCurSelAudioService(iNewServiceID);
	Parameters.Unlock();
}

void FMDialog::OnMenuSetDisplayColor()
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

void FMDialog::closeEvent(QCloseEvent* ce)
{
	/* stop real-time timers */
	Timer.stop();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("FM Dialog", s);

	/* tell every other window to close too */
	emit Closed();
	// stay open until working thread is done
	if(DRMReceiver.GetParameters()->eRunState==CParameter::STOPPED)
		ce->accept();
	else
		ce->ignore();
}

void FMDialog::customEvent(QCustomEvent* Event)
{
	if (Event->type() == QEvent::User + 11)
	{
		int iMessType = ((DRMEvent*) Event)->iMessType;
		int iStatus = ((DRMEvent*) Event)->iStatus;

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

QString FMDialog::GetCodecString(const int iServiceID)
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

QString FMDialog::GetTypeString(const int iServiceID)
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

	return strReturn;
}

void FMDialog::SetDisplayColor(const QColor newColor)
{
	/* Collect pointer to the desired controls in a vector */
	vector<QWidget*> vecpWidgets;
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

		/* Set new palette */
		vecpWidgets[i]->setPalette(CurPal);
	}
}

void FMDialog::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
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

}
