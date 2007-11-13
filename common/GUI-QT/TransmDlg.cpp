/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
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

#include "TransmDlg.h"
#include <qpushbutton.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qcombobox.h>
#include <qstring.h>
#include <qbuttongroup.h>
#include <qmultilineedit.h>
#include <qlistview.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qthread.h>
#include <qwt/qwt_thermo.h>
#include <qwhatsthis.h>
#include <qprogressbar.h>
#include <qmessagebox.h>
#include <qhostaddress.h>

#include "DialogUtil.h"
#include "../DrmTransmitter.h"
#include "../Parameter.h"
#include "../util/Settings.h"
#include <sstream>

TransmDialog::TransmDialog(CDRMTransmitter& tx, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f
	):
	TransmDlgBase(parent, name, modal, f),
	pMenu(NULL), pSettingsMenu(NULL), Timer(),
	DRMTransmitter(tx), Settings(NSettings),
	bIsStarted(FALSE),
	vecstrTextMessage(1) /* 1 for new text */, iIDCurrentText(0),
	vecIpIf()
{
	int i;
	size_t t;
	vector<string> vecAudioSources;

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	SetDialogCaption(this, tr("Dream DRM Transmitter"));

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set controls to custom behavior */
	MultiLineEditTextMessage->setWordWrap(QMultiLineEdit::WidgetWidth);
	MultiLineEditTextMessage->setEdited(FALSE);
	ComboBoxTextMessage->insertItem(tr("new"), 0);

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(QwtThermo::Horizontal, QwtThermo::Bottom);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-5.0);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));

	/* Init progress bar for current transmitted picture */
	ProgressBarCurPict->setTotalSteps(100);
	ProgressBarCurPict->setProgress(0);
	TextLabelCurPict->setText("");

	/* MSC interleaver mode */
	ComboBoxMSCInterleaver->insertItem(tr("2 s (Long Interleaving)"), 0);
	ComboBoxMSCInterleaver->insertItem(tr("400 ms (Short Interleaving)"), 1);

	/* MSC Constellation Scheme */
	ComboBoxMSCConstellation->insertItem(tr("SM 16-QAM"), 0);
	ComboBoxMSCConstellation->insertItem(tr("SM 64-QAM"), 1);

// These modes should not be used right now, TODO
//	ComboBoxMSCConstellation->insertItem(tr("HMsym 64-QAM"), 2);
//	ComboBoxMSCConstellation->insertItem(tr("HMmix 64-QAM"), 3);

	/* SDC Constellation Scheme */
	ComboBoxSDCConstellation->insertItem(tr("4-QAM"), 0);
	ComboBoxSDCConstellation->insertItem(tr("16-QAM"), 1);

	/* Set button group IDs */
	ButtonGroupBandwidth->insert(RadioButtonBandwidth45, 0);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth5, 1);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth9, 2);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth10, 3);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth18, 4);
	ButtonGroupBandwidth->insert(RadioButtonBandwidth20, 5);

	/* Service parameters --------------------------------------------------- */

	/* Language */
	for (i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
		ComboBoxFACLanguage->insertItem(strTableLanguageCode[i].c_str(), i);

	/* Program type */
	for (i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
		ComboBoxProgramType->insertItem(strTableProgTypCod[i].c_str(), i);

	/* Sound card IF */
	LineEditSndCrdIF->setText(QString().number(
		DRMTransmitter.TransmParam.rCarOffset, 'f', 2));

	/* Fill MDI Source/Dest selection */
	GetNetworkInterfaces(vecIpIf);
	for(t=0; t<vecIpIf.size(); t++)
	{
		ComboBoxMDIinInterface->insertItem(vecIpIf[t].name.c_str());
		ComboBoxMDIoutInterface->insertItem(vecIpIf[t].name.c_str());
	}
	ListViewMDIOutputs->clear();
	ListViewMDIOutputs->setAllColumnsShowFocus(true);

	/* Fill COFDM Dest selection */
	/* Get sound device names */
	vecAudioSources.clear();
	DRMTransmitter.GetSoundOutChoices(vecAudioSources);
	for (t = 0; t < vecAudioSources.size(); t++)
	{
		ComboBoxCOFDMdest->insertItem(QString(vecAudioSources[t].c_str()));
	}
	ComboBoxCOFDMdest->insertItem("wav file");
	ComboBoxCOFDMdest->setCurrentItem(0);

	/* Clear list box for file names and set up columns */
	ListViewFileNames->setAllColumnsShowFocus(true);

	ListViewFileNames->clear();

	/* We assume that one column is already there */
	ListViewFileNames->setColumnText(0, "File Name");
	ListViewFileNames->addColumn("Size [KB]");
	ListViewFileNames->addColumn("Full Path");

	/* Set audio enable check box */
	CheckBoxSBR->setChecked(TRUE);

	/* enable services for initialization */
	EnableAudio(true);
	EnableData(true);

	/* Fill Audio source selection */
	/* Get sound device names */
	vecAudioSources.clear();
	DRMTransmitter.GetSoundInChoices(vecAudioSources);
	vecAudioSources.push_back("wav file");
	for (t = 0; t < vecAudioSources.size(); t++)
	{
		ComboBoxAudioSource->insertItem(QString(vecAudioSources[t].c_str()), t);
	}
	ComboBoxAudioSource->setCurrentItem(0);

	LineEditMDIinGroup->setEnabled(FALSE);
#if QT_VERSION >= 0x030000
	LineEditMDIinGroup->setInputMask("000.000.000.000;_");
	LineEditMDIoutDest->setInputMask("000.000.000.000;_");
	LineEditMDIinPort->setInputMask("00009;_");
	LineEditMDIoutPort->setInputMask("00009;_");
#endif

	/* Enable all controls */
	EnableAllControlsForSet();


	/* Set Menu ***************************************************************/

	/* Main menu bar */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	//pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	TransmDlgBaseLayout->setMenuBar(pMenu);

	/* NOT implemented yet */
    ComboBoxCodec->setEnabled(FALSE);
    ComboBoxAudioMode->setEnabled(FALSE);
    ComboBoxAudioBitrate->setEnabled(FALSE);
    CheckBoxSBR->setEnabled(FALSE);

	/* Connections ---------------------------------------------------------- */
	connect(ButtonStartStop, SIGNAL(clicked()),
		this, SLOT(OnButtonStartStop()));
	connect(PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));
	connect(CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
	connect(ButtonAddStream, SIGNAL(clicked()),
		this, SLOT(OnButtonAddStream()));
	connect(ButtonDeleteStream, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteStream()));

	connect(PushButtonAddService, SIGNAL(clicked()),
		this, SLOT(OnButtonAddService()));
	connect(PushButtonDeleteService, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteService()));

	connect(PushButtonAddMDIDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddMDIDest()));
	connect(PushButtonAddMDIFileDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddMDIFileDest()));
	connect(PushButtonDeleteMDIOutput, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteMDIDest()));

	/* Combo boxes */
	connect(ComboBoxCOFDMdest, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxCOFDMDestHighlighted(int)));
	connect(ComboBoxAudioSource, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxAudioSourceHighlighted(int)));
	connect(ComboBoxMSCInterleaver, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCInterleaverActivated(int)));
	connect(ComboBoxMSCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCConstellationActivated(int)));
	connect(ComboBoxTextMessage, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxTextMessageHighlighted(int)));
	connect(ComboBoxMSCProtLev, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCProtLevActivated(int)));
	connect(ComboBoxSDCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSDCConstellationActivated(int)));
	connect(ComboBoxStreamType, SIGNAL(highlighted(int)),
		this, SLOT(OnComboBoxStreamTypeHighlighted(int)));
	connect(ComboBoxPacketsPerFrame, SIGNAL(highlighted(const QString&)),
		this, SLOT(OnComboBoxPacketsPerFrameHighlighted(const QString&)));

	/* Button groups */
	connect(ButtonGroupMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioMode(int)));
	connect(ButtonGroupRobustnessMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioRobustnessMode(int)));
	connect(ButtonGroupBandwidth, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBandwidth(int)));

	/* Line edits */
	connect(LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceLabel(const QString&)));
	connect(LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));
	connect(LineEditPacketLen, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPacketLenChanged(const QString&)));

	connect(ListViewStreams, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnStreamsListItemClicked(QListViewItem*)));
	connect(ListViewServices, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnServicesListItemClicked(QListViewItem*)));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	ListViewStreams->setAllColumnsShowFocus(true);
	ListViewStreams->clear();
	ComboBoxStreamType->insertItem(tr("audio"));
	ComboBoxStreamType->insertItem(tr("data packet"));
	ComboBoxStreamType->insertItem(tr("data stream"));
	ComboBoxStream->insertItem("0");
	ComboBoxStream->insertItem("1");
	ComboBoxStream->insertItem("2");
	ComboBoxStream->insertItem("3");

	ComboBoxStream->setCurrentItem(0);

	ListViewServices->setAllColumnsShowFocus(true);
	ListViewServices->clear();
	ComboBoxShortID->insertItem("0");
	ComboBoxShortID->insertItem("1");
	ComboBoxShortID->insertItem("2");
	ComboBoxShortID->insertItem("3");
	ComboBoxShortID->setCurrentItem(0);

    ComboBoxServiceAudioStream->clear();
	ComboBoxServiceAudioStream->insertItem("0");
	ComboBoxServiceAudioStream->insertItem("1");
	ComboBoxServiceAudioStream->insertItem("2");
	ComboBoxServiceAudioStream->insertItem("3");
	ComboBoxServiceAudioStream->setCurrentItem(0);

    ComboBoxServiceDataStream->clear();
	ComboBoxServiceDataStream->insertItem("0");
	ComboBoxServiceDataStream->insertItem("1");
	ComboBoxServiceDataStream->insertItem("2");
	ComboBoxServiceDataStream->insertItem("3");
	ComboBoxServiceDataStream->setCurrentItem(0);

    ComboBoxServicePacketID->clear();
	ComboBoxServicePacketID->insertItem("0");
	ComboBoxServicePacketID->insertItem("1");
	ComboBoxServicePacketID->insertItem("2");
	ComboBoxServicePacketID->insertItem("3");
	ComboBoxServicePacketID->setCurrentItem(0);

    ComboBoxAppType->clear();
    ComboBoxAppType->insertItem("Normal");
    ComboBoxAppType->insertItem("Engineering Test");
	for (t = 1; t < 31; t++)
	{
		ComboBoxAppType->insertItem(QString("Reserved (%1)").arg(t));
	}
	ComboBoxAppType->setCurrentItem(0);

	GetFromTransmitter();
	OnRadioMode(0);

	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

TransmDialog::~TransmDialog()
{
}

void
TransmDialog::closeEvent(QCloseEvent* ce)
{
	/* Stop transmitter */
	if (bIsStarted == TRUE)
		DRMTransmitter.Stop();
	else
		SetTransmitter(); // so Transmitter save settings has the latest info
	ce->accept();
}

void TransmDialog::OnTimer()
{
	/* Set value for input level meter (only in "start" mode) */
	if (bIsStarted == TRUE)
	{
		ProgrInputLevel->
			setValue(DRMTransmitter.GetLevelMeter());

		string strCPictureName;
		_REAL rCPercent;

		/* Activate progress bar for slide show pictures only if current state
		   can be queried */
		if (DRMTransmitter.GetTransStat(strCPictureName, rCPercent))
		{
			/* Enable controls */
			ProgressBarCurPict->setEnabled(TRUE);
			TextLabelCurPict->setEnabled(TRUE);

			/* We want to file name, not the complete path -> "QFileInfo" */
			QFileInfo FileInfo(strCPictureName.c_str());

			/* Show current file name and percentage */
			TextLabelCurPict->setText(FileInfo.fileName());
			ProgressBarCurPict->setProgress((int) (rCPercent * 100)); /* % */
		}
		else
		{
			/* Disable controls */
			ProgressBarCurPict->setEnabled(FALSE);
			TextLabelCurPict->setEnabled(FALSE);
		}
	}
}

void
TransmDialog::GetFromTransmitter()
{
	switch(DRMTransmitter.GetOperatingMode())
	{
	case CDRMTransmitter::T_ENC:
		GetChannel();
		GetStreams();
		GetAudio();
		GetData();
		GetServices();
		GetMDIOut();
		RadioButtonEncoder->setChecked(TRUE);
		break;
	case CDRMTransmitter::T_MOD:
		GetMDIIn();
		GetCOFDM();
		RadioButtonModulator->setChecked(TRUE);
		break;
	case CDRMTransmitter::T_TX:
		GetChannel();
		GetStreams();
		GetAudio();
		GetData();
		GetServices();
		GetCOFDM();
		RadioButtonTransmitter->setChecked(TRUE);
		break;
	}
}

void
TransmDialog::GetChannel()
{
	/* Robustness mode */
	switch (DRMTransmitter.TransmParam.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		RadioButtonRMA->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_B:
		RadioButtonRMB->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_C:
		RadioButtonRMC->setChecked(TRUE);
		break;

	case RM_ROBUSTNESS_MODE_D:
		RadioButtonRMD->setChecked(TRUE);
		break;

	case RM_NO_MODE_DETECTED:
		;
	}

	/* Bandwidth */
	switch (DRMTransmitter.TransmParam.GetSpectrumOccup())
	{
	case SO_0:
		RadioButtonBandwidth45->setChecked(TRUE);
		break;

	case SO_1:
		RadioButtonBandwidth5->setChecked(TRUE);
		break;

	case SO_2:
		RadioButtonBandwidth9->setChecked(TRUE);
		break;

	case SO_3:
		RadioButtonBandwidth10->setChecked(TRUE);
		break;

	case SO_4:
		RadioButtonBandwidth18->setChecked(TRUE);
		break;

	case SO_5:
		RadioButtonBandwidth20->setChecked(TRUE);
		break;
	}

	switch (DRMTransmitter.TransmParam.eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		ComboBoxMSCInterleaver->setCurrentItem(0);
		break;

	case CParameter::SI_SHORT:
		ComboBoxMSCInterleaver->setCurrentItem(1);
		break;
	}

	switch (DRMTransmitter.TransmParam.eMSCCodingScheme)
	{
	case CS_1_SM:
		break;

	case CS_2_SM:
		ComboBoxMSCConstellation->setCurrentItem(0);
		break;

	case CS_3_SM:
		ComboBoxMSCConstellation->setCurrentItem(1);
		break;

	case CS_3_HMSYM:
//		ComboBoxMSCConstellation->setCurrentItem(2);
		break;

	case CS_3_HMMIX:
//		ComboBoxMSCConstellation->setCurrentItem(3);
		break;
	}

	/* MSC Protection Level */
	UpdateMSCProtLevCombo(); /* options depend on MSC Constellation */
	ComboBoxMSCProtLev->setCurrentItem(DRMTransmitter.TransmParam.MSCPrLe.iPartB);

	switch (DRMTransmitter.TransmParam.eSDCCodingScheme)
	{
	case CS_1_SM:
		ComboBoxSDCConstellation->setCurrentItem(0);
		break;

	case CS_2_SM:
		ComboBoxSDCConstellation->setCurrentItem(1);
		break;
	default:
		;
	}

	DRMTransmitter.CalculateChannelCapacities(DRMTransmitter.TransmParam);

	TextLabelMSCCapBits->setText(QString::number(DRMTransmitter.TransmParam.iNumDecodedBitsMSC));
	TextLabelMSCCapBytes->setText(QString::number(DRMTransmitter.TransmParam.iNumDecodedBitsMSC/8));
	TextLabelMSCBytesTotal->setText(QString::number(DRMTransmitter.TransmParam.iNumDecodedBitsMSC/8));

	TextLabelSDCCapBits->setText(QString::number(DRMTransmitter.TransmParam.iNumSDCBitsPerSFrame));
	TextLabelSDCCapBytes->setText(QString::number(DRMTransmitter.TransmParam.iNumSDCBitsPerSFrame/8));

	TextLabelMSCBytesAvailable->setText(TextLabelMSCCapBytes->text());
}

void
TransmDialog::SetChannel()
{
	/* Spectrum Occupancy */
	if(RadioButtonBandwidth45->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_0);
	if(RadioButtonBandwidth5->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_1);
	if(RadioButtonBandwidth9->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_2);
	if(RadioButtonBandwidth10->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_3);
	if(RadioButtonBandwidth18->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_4);
	if(RadioButtonBandwidth20->isChecked())
		DRMTransmitter.TransmParam.SetSpectrumOccup(SO_5);

	/* MSC Protection Level */
	DRMTransmitter.TransmParam.MSCPrLe.iPartB = ComboBoxMSCProtLev->currentItem();

	/* MSC Constellation Scheme */
	switch(ComboBoxMSCConstellation->currentItem())
	{
	case 0:
		DRMTransmitter.TransmParam.eMSCCodingScheme = CS_2_SM;
		break;

	case 1:
		DRMTransmitter.TransmParam.eMSCCodingScheme = CS_3_SM;
		break;

	case 2:
		DRMTransmitter.TransmParam.eMSCCodingScheme = CS_3_HMSYM;
		break;

	case 3:
		DRMTransmitter.TransmParam.eMSCCodingScheme = CS_3_HMMIX;
		break;
	}

	/* MSC interleaver mode */
	switch(ComboBoxMSCInterleaver->currentItem())
	{
	case 0:
		DRMTransmitter.TransmParam.eSymbolInterlMode = CParameter::SI_LONG;
		break;
	case 1:
		DRMTransmitter.TransmParam.eSymbolInterlMode = CParameter::SI_SHORT;
		break;
	}


	/* SDC */
	switch(ComboBoxSDCConstellation->currentItem())
	{
	case 0:
		DRMTransmitter.TransmParam.eSDCCodingScheme = CS_1_SM;
		break;

	case 1:
		DRMTransmitter.TransmParam.eSDCCodingScheme = CS_2_SM;
		break;
	default:
		;
	}

	/* Robustness Mode */
	if(RadioButtonRMA->isChecked())
		DRMTransmitter.TransmParam.SetWaveMode(RM_ROBUSTNESS_MODE_A);
	if(RadioButtonRMB->isChecked())
		DRMTransmitter.TransmParam.SetWaveMode(RM_ROBUSTNESS_MODE_B);
	if(RadioButtonRMC->isChecked())
		DRMTransmitter.TransmParam.SetWaveMode(RM_ROBUSTNESS_MODE_C);
	if(RadioButtonRMD->isChecked())
		DRMTransmitter.TransmParam.SetWaveMode(RM_ROBUSTNESS_MODE_D);

}

void
TransmDialog::GetStreams()
{
	for(size_t i=0; i<DRMTransmitter.TransmParam.Stream.size(); i++)
	{
		const CStream& stream = DRMTransmitter.TransmParam.Stream[i];
		size_t bits;
		if(stream.iLenPartB == -1)
		{
			bits = DRMTransmitter.TransmParam.iNumDecodedBitsMSC;
		}
		else
		{
			bits = stream.iLenPartA+stream.iLenPartB;
		}
		ComboBoxStream->setCurrentItem(i);
		if(stream.eAudDataFlag == SF_AUDIO)
		{
			ComboBoxStreamType->setCurrentItem(0);
			LineEditPacketLen->setText("-");
			ComboBoxPacketsPerFrame->clear();
			ComboBoxPacketsPerFrame->insertItem("-");
		}
		else
		{
			if(stream.ePacketModInd == PM_PACKET_MODE)
			{
				ComboBoxStreamType->setCurrentItem(1);
				LineEditPacketLen->setText(QString::number(stream.iPacketLen));
				ComboBoxPacketsPerFrame->clear();
				ComboBoxPacketsPerFrame->insertItem(QString::number(bits/8/stream.iPacketLen));
			}
			else
			{
				ComboBoxStreamType->setCurrentItem(2);
				LineEditPacketLen->setText("-");
				ComboBoxPacketsPerFrame->clear();
				ComboBoxPacketsPerFrame->insertItem("-");
			}
		}
		LineEditBitsPerFrame->setText(QString::number(bits));
		OnButtonAddStream();
	}
}

void
TransmDialog::SetStreams()
{
	DRMTransmitter.TransmParam.Stream.resize(ListViewStreams->childCount());
	QListViewItemIterator it(ListViewStreams);
	for (; it.current(); it++)
	{
	    int iStreamID = it.current()->text(0).toInt();
        CStream& stream = DRMTransmitter.TransmParam.Stream[iStreamID];
	    QString type =  it.current()->text(1);
	    int iType = 0;
        for(int i=0; i<ComboBoxStreamType->count(); i++)
            if(ComboBoxStreamType->text(i)==type)
                iType = i;
        switch(iType)
        {
            case 0:
                stream.eAudDataFlag = SF_AUDIO;
                break;
            case 1:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_PACKET_MODE;
            case 2:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_SYNCHRON_STR_MODE;
        }
	    QString plen =  it.current()->text(2);
        if(plen!="-")
            stream.iPacketLen = plen.toInt();
        stream.iLenPartA = 0; /* EEP only */
        stream.iLenPartB = it.current()->text(5).toInt();
	}
}

void
TransmDialog::GetAudio()
{
	for(size_t i=0; i<DRMTransmitter.TransmParam.AudioParam.size(); i++)
	{
		if(DRMTransmitter.TransmParam.AudioParam[i].bTextflag == TRUE)
		{
			/* Activate text message */
			EnableTextMessage(TRUE);
			CheckBoxEnableTextMessage->setChecked(TRUE);
		}
		else
		{
			EnableTextMessage(FALSE);
			CheckBoxEnableTextMessage->setChecked(FALSE);
		}
	}
}

void
TransmDialog::SetAudio()
{

	CAudioParam& AudioParam = DRMTransmitter.TransmParam.AudioParam[0];

	int iAudSrc = ComboBoxAudioSource->currentItem();
	if(iAudSrc==ComboBoxAudioSource->count()-1)
		DRMTransmitter.SetReadFromFile(ComboBoxAudioSource->currentText().latin1());
	else
		DRMTransmitter.SetSoundInInterface(iAudSrc);

	AudioParam.bTextflag = CheckBoxEnableTextMessage->isChecked();

	if(AudioParam.bTextflag)
	{
		DRMTransmitter.ClearTextMessages();

		for (size_t i = 1; i < vecstrTextMessage.size(); i++)
			DRMTransmitter.AddTextMessage(vecstrTextMessage[i]);
	}

	/* TODO */
	if (DRMTransmitter.TransmParam.GetStreamLen(0) > 7000)
		AudioParam.eAudioSamplRate = CAudioParam::AS_24KHZ;
	else
		AudioParam.eAudioSamplRate = CAudioParam::AS_12KHZ;
}

void
TransmDialog:: GetData()
{
}

void
TransmDialog::SetData()
{
		/* Init SlideShow application */
		DRMTransmitter.TransmParam.DataParam[0][0].eDataUnitInd = CDataParam::DU_DATA_UNITS;
		DRMTransmitter.TransmParam.DataParam[0][0].eAppDomain = CDataParam::AD_DAB_SPEC_APP;

		/* Set file names for data application */
		DRMTransmitter.ClearPics();

		QListViewItemIterator it(ListViewFileNames);

		for (; it.current(); it++)
		{
			/* Complete file path is in third column */
			const QString strFileName = it.current()->text(2);

			/* Extract format string */
			QFileInfo FileInfo(strFileName);
			const QString strFormat = FileInfo.extension(FALSE);

			DRMTransmitter.AddPic(strFileName.latin1(), strFormat.latin1());
		}
}

void
TransmDialog::GetServices()
{
	const vector<CService>& Service = DRMTransmitter.TransmParam.Service;
	for(size_t i=0; i<Service.size(); i++)
	{
		QListViewItem* v = new QListViewItem(ListViewServices, QString::number(i),
		Service[i].strLabel.c_str(),
		QString::number(ulong(Service[i].iServiceID), 16),
		ComboBoxFACLanguage->text(Service[i].iLanguage),
		Service[i].strLanguageCode.c_str(),
		Service[i].strCountryCode.c_str()
		);
		if(Service[i].iDataStream != STREAM_ID_NOT_USED)
		{
            v->setText(6, ComboBoxAppType->text(Service[i].iServiceDescr));
			v->setText(8, QString::number(Service[i].iDataStream));
            v->setText(9, QString::number(Service[i].iPacketID));
		}
		if(Service[i].iAudioStream!=STREAM_ID_NOT_USED)
		{
		    /* audio overrides data */
            v->setText(6, ComboBoxProgramType->text(Service[i].iServiceDescr));
			v->setText(7, QString::number(Service[i].iAudioStream));
		}
	}
	ListViewServices->setSelected(ListViewServices->firstChild(), true);
}

void
TransmDialog::SetServices()
{
	DRMTransmitter.TransmParam.iNumDataService=0;
	DRMTransmitter.TransmParam.iNumAudioService=0;

	QListViewItemIterator sit(ListViewServices);
	for (; sit.current(); sit++)
	{
		int iShortID = sit.current()->text(0).toUInt();
		CService Service;
		Service.strLabel = sit.current()->text(1).utf8();
		Service.iServiceID = sit.current()->text(2).toULong(NULL, 16);
		QString lang = sit.current()->text(3);
		for(int i=0; i<ComboBoxFACLanguage->count(); i++)
            if(ComboBoxFACLanguage->text(i)==lang)
                Service.iLanguage = i;
		Service.strLanguageCode = sit.current()->text(4).utf8();
		Service.strCountryCode = sit.current()->text(5).utf8();
        QString type = sit.current()->text(6);
		QString sA = sit.current()->text(7);
		QString sD = sit.current()->text(8);
		QString sP = sit.current()->text(9);
		if(sA=="-")
		{
			Service.iAudioStream = STREAM_ID_NOT_USED;
			DRMTransmitter.TransmParam.iNumDataService++;
			Service.eAudDataFlag = SF_DATA;
            for(int i=0; i<ComboBoxAppType->count(); i++)
                if(ComboBoxAppType->text(i)==type)
                    Service.iServiceDescr = i;
		}
		else
		{
			Service.iAudioStream = sA.toUInt();
			DRMTransmitter.TransmParam.iNumAudioService++;
			Service.eAudDataFlag = SF_AUDIO;
            for(int i=0; i<ComboBoxProgramType->count(); i++)
                if(ComboBoxProgramType->text(i)==type)
                    Service.iServiceDescr = i;
		}
		if(sD=="-")
			Service.iDataStream = STREAM_ID_NOT_USED;
		else
			Service.iDataStream = sD.toUInt();
		if(sP=="-")
			Service.iPacketID = 4;
		else
			Service.iPacketID = sP.toUInt();
		DRMTransmitter.TransmParam.SetServiceParameters(iShortID, Service);
	}
}

void
TransmDialog::SetTransmitter()
{
	DRMTransmitter.strMDIinAddr = "";
	DRMTransmitter.MDIoutAddr.clear();
	CDRMTransmitter::ETxOpMode eMod = CDRMTransmitter::T_TX;

	if(RadioButtonTransmitter->isChecked() || RadioButtonEncoder->isChecked())
	{
		SetChannel();
		SetStreams();
		SetAudio();
		SetData();
		SetServices();
	}
	if(RadioButtonTransmitter->isChecked() || RadioButtonModulator->isChecked())
	{
		SetCOFDM();
	}
	if(RadioButtonEncoder->isChecked())
	{
		DRMTransmitter.DisableCOFDM();
		SetMDIOut();
		eMod = CDRMTransmitter::T_ENC;
	}
	if(RadioButtonModulator->isChecked())
	{
		SetMDIIn();
		eMod = CDRMTransmitter::T_MOD;
	}
	DRMTransmitter.SetOperatingMode(eMod);
}

void
TransmDialog::GetMDIIn()
{
	QString addr = DRMTransmitter.strMDIinAddr.c_str();
	QStringList parts = QStringList::split(":", addr, TRUE);
	switch(parts.count())
	{
	case 1:
		LineEditMDIinPort->setText(parts[0]);
		choseComboBoxItem(ComboBoxMDIinInterface, "any");
		LineEditMDIinGroup->setText("");
		CheckBoxMDIinMcast->setChecked(false);
		break;
	case 2:
		choseComboBoxItem(ComboBoxMDIinInterface, parts[0]);
		LineEditMDIinPort->setText(parts[1]);
		LineEditMDIinGroup->setText("");
		CheckBoxMDIinMcast->setChecked(false);
		break;
	case 3:
		choseComboBoxItem(ComboBoxMDIinInterface, parts[0]);
		LineEditMDIinGroup->setText(parts[1]);
		LineEditMDIinPort->setText(parts[2]);
		CheckBoxMDIinMcast->setChecked(true);
		break;
	}
}

void
TransmDialog::SetMDIIn()
{
	QString port = LineEditMDIinPort->text();
	QString group = LineEditMDIinGroup->text();
	QString addr=port;
	if(CheckBoxMDIinMcast->isChecked())
		addr = group+":"+addr;
	int iInterface = ComboBoxMDIinInterface->currentItem();
	if(vecIpIf[iInterface].name!="any")
	{
		if(!CheckBoxMDIinMcast->isChecked())
			addr = ":"+addr;
		addr = QHostAddress(vecIpIf[iInterface].addr).toString()+":"+addr;
	}
	DRMTransmitter.strMDIinAddr = addr.latin1();
}

void
TransmDialog::GetMDIOut()
{
    for(size_t i=0; i<DRMTransmitter.MDIoutAddr.size(); i++)
    {
        QString addr = DRMTransmitter.MDIoutAddr[i].c_str();
        QStringList parts = QStringList::split(":", addr, TRUE);
        switch(parts.count())
        {
        case 0:
            (void)new QListViewItem(ListViewMDIOutputs, parts[0]);
            break;
        case 1:
            (void)new QListViewItem(ListViewMDIOutputs, parts[0], "", "any");
            break;
        case 2:
            (void)new QListViewItem(ListViewMDIOutputs, parts[1], parts[0], "any");
            break;
        case 3:
            {
                QString name;
                for(size_t j=0; j<vecIpIf.size(); j++)
                {
                    if(parts[0].toUInt()==vecIpIf[j].addr)
                        name = vecIpIf[j].name.c_str();
                }
                (void)new QListViewItem(ListViewMDIOutputs, parts[2], parts[1], name);
            }
        }
    }
}

void
TransmDialog::OnButtonAddMDIDest()
{
	QString dest = LineEditMDIoutDest->text();
	if(dest == "...")
		dest = "";
    (void) new QListViewItem(ListViewMDIOutputs,
        LineEditMDIoutPort->text(),
        dest,
        ComboBoxMDIoutInterface->currentText()
    );
}

void
TransmDialog::OnButtonAddMDIFileDest()
{
    QString s( QFileDialog::getSaveFileName(
			"out.pcap", "Capture Files (*.pcap)", this ) );
    if ( s.isEmpty() )
        return;
    LineEditMDIOutputFile->setText(s);
   (void) new QListViewItem(ListViewMDIOutputs, s);
}

void
TransmDialog::OnButtonDeleteMDIDest()
{
	QListViewItem* p = ListViewMDIOutputs->selectedItem();
	if(p)
	{
		delete p;
	}
}

void
TransmDialog::SetMDIOut()
{
    DRMTransmitter.MDIoutAddr.clear();
	QListViewItemIterator it(ListViewMDIOutputs);
	for (; it.current(); it++)
	{
		QString port = it.current()->text(0);
		QString dest = it.current()->text(1);
		QString iface = it.current()->text(2);
		QString addr;
		if(dest == NULL)
		{
		    addr = port;
		}
		else if(iface=="any")
		{
			addr = dest+":"+port;
		}
		else
		{
			uint32_t iInterface;
			for(size_t i=0; i<vecIpIf.size(); i++)
				if(vecIpIf[i].name==iface)
					iInterface = vecIpIf[i].addr;
			addr = QHostAddress(iInterface).toString()+":"+dest+":"+port;
		}
        DRMTransmitter.MDIoutAddr.push_back(string(addr.utf8()));
	}
}

void
TransmDialog::GetCOFDM()
{
	/* Output mode (real valued, I / Q or E / P) */
	switch (DRMTransmitter.TransmParam.eOutputFormat)
	{
	case OF_REAL_VAL:
		RadioButtonOutReal->setChecked(TRUE);
		break;

	case OF_IQ_POS:
		RadioButtonOutIQPos->setChecked(TRUE);
		break;

	case OF_IQ_NEG:
		RadioButtonOutIQNeg->setChecked(TRUE);
		break;

	case OF_EP:
		RadioButtonOutEP->setChecked(TRUE);
		break;
	}
}

void
TransmDialog::SetCOFDM()
{
	DRMTransmitter.TransmParam.rCarOffset = LineEditSndCrdIF->text().toFloat();

	if(RadioButtonOutReal->isChecked())
		DRMTransmitter.TransmParam.eOutputFormat=OF_REAL_VAL;
	if(RadioButtonOutIQPos->isChecked())
		DRMTransmitter.TransmParam.eOutputFormat=OF_IQ_POS;
	if(RadioButtonOutIQNeg->isChecked())
		DRMTransmitter.TransmParam.eOutputFormat=OF_IQ_NEG;
	if(RadioButtonOutEP->isChecked())
		DRMTransmitter.TransmParam.eOutputFormat=OF_EP;

	int iDest = ComboBoxCOFDMdest->currentItem();
	if(iDest == ComboBoxCOFDMdest->count()-1)
	{
		string s = ComboBoxCOFDMdest->currentText().latin1();
		DRMTransmitter.SetWriteToFile(s, "wav");
	}
	else
	{
		DRMTransmitter.SetSoundOutInterface(iDest);
	}
}

void
TransmDialog::OnRadioMode(int)
{
	TabWidgetConfigure->removePage(Channel);
	TabWidgetConfigure->removePage(Streams);
	TabWidgetConfigure->removePage(Audio);
	TabWidgetConfigure->removePage(Data);
	TabWidgetConfigure->removePage(Services);
	TabWidgetConfigure->removePage(MDIOut);
	TabWidgetConfigure->removePage(MDIIn);
	TabWidgetConfigure->removePage(COFDM);

	if(RadioButtonTransmitter->isChecked())
	{
		TabWidgetConfigure->addTab(Channel, "Channel");
		TabWidgetConfigure->addTab(Streams, "Streams");
		TabWidgetConfigure->addTab(Audio, "Audio");
		TabWidgetConfigure->addTab(Data, "Data");
		TabWidgetConfigure->addTab(Services, "Services");
		TabWidgetConfigure->addTab(COFDM, "COFDM");
		TabWidgetConfigure->showPage(Channel);
	}
	if(RadioButtonEncoder->isChecked())
	{
		TabWidgetConfigure->addTab(Channel, "Channel");
		TabWidgetConfigure->addTab(Streams, "Streams");
		TabWidgetConfigure->addTab(Audio, "Audio");
		TabWidgetConfigure->addTab(Data, "Data");
		TabWidgetConfigure->addTab(Services, "Services");
		TabWidgetConfigure->addTab(MDIOut, "MDI Output");
		TabWidgetConfigure->showPage(Channel);
	}
	if(RadioButtonModulator->isChecked())
	{
		TabWidgetConfigure->addTab(MDIIn, "MDI Input");
		TabWidgetConfigure->addTab(COFDM, "COFDM");
		TabWidgetConfigure->showPage(COFDM);
	}
}

void
TransmDialog::OnLineEditPacketLenChanged(const QString& str)
{
	if(str=="-")
		return;
	size_t bits = 8*TextLabelMSCBytesAvailable->text().toInt();
	size_t packet_len = str.toInt();
	if(packet_len==0)
		packet_len = 3;
	size_t max_packets = bits/(8*packet_len);
	ComboBoxPacketsPerFrame->clear();
	for(size_t i=0; i<=max_packets; i++)
		ComboBoxPacketsPerFrame->insertItem(QString::number(i));
	if(max_packets>0)
		ComboBoxPacketsPerFrame->setCurrentItem(1);
	else
		ComboBoxPacketsPerFrame->setCurrentItem(0);
}

void
TransmDialog::OnComboBoxPacketsPerFrameHighlighted(const QString& str)
{
	if(ComboBoxPacketsPerFrame->currentText()=="-")
		return;
	size_t packet_len = LineEditPacketLen->text().toInt();
	size_t packets = str.toInt();
	LineEditBitsPerFrame->setText(QString::number(8*packets*packet_len));
}

void
TransmDialog::OnComboBoxStreamTypeHighlighted(int item)
{
	size_t bits = 8*TextLabelMSCBytesAvailable->text().toInt();
	switch(item)
	{
	case 0: // audio
	case 2: // data_stream
		LineEditPacketLen->setText("-");
		LineEditPacketLen->setEnabled(false);
		ComboBoxPacketsPerFrame->clear();
		ComboBoxPacketsPerFrame->insertItem("-");
		ComboBoxPacketsPerFrame->setEnabled(false);
		ComboBoxPacketsPerFrame->setCurrentItem(0);
		LineEditBitsPerFrame->setEnabled(true);
		LineEditBitsPerFrame->setText(QString::number(bits));
		break;
	case 1: // data_packet
		LineEditPacketLen->setText(QString::number(int(bits/8)));
		LineEditPacketLen->setEnabled(true);
		ComboBoxPacketsPerFrame->setEnabled(true);
		LineEditBitsPerFrame->setEnabled(false);
		break;
	}
}

void TransmDialog::OnStreamsListItemClicked(QListViewItem* item)
{
	LineEditBitsPerFrame->setText(item->text(4));
    QString type =  item->text(1);
    choseComboBoxItem(ComboBoxStreamType, item->text(1));
}

void
TransmDialog::OnButtonAddStream()
{
	int bits = LineEditBitsPerFrame->text().toInt();
	(void) new QListViewItem(ListViewStreams,
		ComboBoxStream->currentText(),
		ComboBoxStreamType->currentText(),
		LineEditPacketLen->text(),
		ComboBoxPacketsPerFrame->currentText(),
		QString::number(bits),
		QString::number(bits/8)
	);

	ComboBoxStream->removeItem(ComboBoxStream->currentItem());
	ComboBoxStream->setCurrentItem(0);
	ComboBoxStreamType->setCurrentItem(0);
	int availbytes = TextLabelMSCBytesAvailable->text().toInt();
	TextLabelMSCBytesAvailable->setText(QString::number(availbytes-bits/8));
}

void
TransmDialog::OnButtonDeleteStream()
{
	QListViewItem* p = ListViewStreams->selectedItem();
	if(p)
	{
        int bytes = p->text(5).toInt();
		QStringList s(p->text(0));
		for(int i=0; i<ComboBoxStream->count(); i++)
			s.append(ComboBoxStream->text(i));
		s.sort();
		ComboBoxStream->clear();
		ComboBoxStream->insertStringList(s);
		delete p;
        int availbytes = TextLabelMSCBytesAvailable->text().toInt();
        TextLabelMSCBytesAvailable->setText(QString::number(availbytes+bytes));
	}
	ComboBoxStream->setCurrentItem(0);
}

void TransmDialog::OnButtonStartStop()
{
	if (bIsStarted == TRUE)
	{
		/* Stop transmitter */
		DRMTransmitter.Stop();
		(void)DRMTransmitter.wait(5000);
		if(!DRMTransmitter.finished())
		{
			QMessageBox::critical(this, "Dream", "Exit\n",
				"Termination of working thread failed");
		}

		ButtonStartStop->setText(tr("&Start"));

		EnableAllControlsForSet();

		bIsStarted = FALSE;
	}
	else
	{
		SetTransmitter();
		DRMTransmitter.start();
		ButtonStartStop->setText(tr("&Stop"));
		DisableAllControlsForSet();
		bIsStarted = TRUE;
	}
}

void TransmDialog::OnComboBoxAudioSourceHighlighted(int iID)
{
	if(iID==ComboBoxAudioSource->count()-1)
	{
		QString s( QFileDialog::getOpenFileName(
			QString::null, "Wave Files (*.wav)", this ) );
		if ( s.isEmpty() )
			return;
		ComboBoxAudioSource->changeItem(s, iID);
	}
}

void TransmDialog::OnToggleCheckBoxEnableTextMessage(bool bState)
{
	EnableTextMessage(bState);
}

void TransmDialog::EnableTextMessage(const _BOOLEAN bFlag)
{
	if (bFlag == TRUE)
	{
		/* Enable text message controls */
		ComboBoxTextMessage->setEnabled(TRUE);
		MultiLineEditTextMessage->setEnabled(TRUE);
		PushButtonAddText->setEnabled(TRUE);
		PushButtonClearAllText->setEnabled(TRUE);
	}
	else
	{
		/* Disable text message controls */
		ComboBoxTextMessage->setEnabled(FALSE);
		MultiLineEditTextMessage->setEnabled(FALSE);
		PushButtonAddText->setEnabled(FALSE);
		PushButtonClearAllText->setEnabled(FALSE);
	}
}

void TransmDialog::EnableAudio(const _BOOLEAN bFlag)
{
	if (bFlag == TRUE)
	{
		ComboBoxAudioSource->setEnabled(TRUE);
	}
	else
	{
		/* Disable audio controls */
		GroupBoxTextMessage->setEnabled(FALSE);
		ComboBoxAudioSource->setEnabled(FALSE);
	}
}

void TransmDialog::EnableData(const _BOOLEAN bFlag)
{
	if (bFlag == TRUE)
	{
		/* Enable data controls */
		ListViewFileNames->setEnabled(TRUE);
		PushButtonClearAllFileNames->setEnabled(TRUE);
		PushButtonAddFile->setEnabled(TRUE);
	}
	else
	{
		/* Disable data controls */
		ListViewFileNames->setEnabled(FALSE);
		PushButtonClearAllFileNames->setEnabled(FALSE);
		PushButtonAddFile->setEnabled(FALSE);
	}
}

void TransmDialog::OnPushButtonAddText()
{
	string str;

	/* Check if text control is not empty */
	if (MultiLineEditTextMessage->edited())
	{
		stringstream ss;

		/* First line */
		ss << MultiLineEditTextMessage->textLine(0).utf8();

		/* Other lines */
		const int iNumLines = MultiLineEditTextMessage->numLines();

		for (int i = 1; i < iNumLines; i++)
		{
			/* Insert line break */
			ss << endl;

			/* Insert text of next line */
			ss << MultiLineEditTextMessage->textLine(i).utf8();
		}
		str = ss.str();
	}
	else
		return;

	/* Check size of container. If not enough space, enlarge */
	if (iIDCurrentText == vecstrTextMessage.size())
			vecstrTextMessage.resize(iIDCurrentText+1);

	if (iIDCurrentText == 0)
	{
		/* Add new message */
		iIDCurrentText = vecstrTextMessage.size() - 1;
		ComboBoxTextMessage->insertItem(QString().setNum(iIDCurrentText), iIDCurrentText);

	}
	vecstrTextMessage[iIDCurrentText] = str;

	/* Clear added text */
	MultiLineEditTextMessage->clear();
	MultiLineEditTextMessage->setEdited(FALSE);
}

void TransmDialog::OnButtonClearAllText()
{
	/* Clear container */
	vecstrTextMessage.clear();
	vecstrTextMessage.resize(1);
	iIDCurrentText = 0;

	/* Clear combo box */
	ComboBoxTextMessage->clear();
	ComboBoxTextMessage->insertItem(tr("new"), 0);

	/* Clear multi line edit */
	MultiLineEditTextMessage->clear();
	MultiLineEditTextMessage->setEdited(FALSE);
}

void TransmDialog::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
	QStringList list = QFileDialog::getOpenFileNames(
		tr("Image Files (*.png *.jpg *.jpeg *.jfif)"), NULL, this);

	/* Check if user not hit the cancel button */
	if (!list.isEmpty())
	{
		/* Insert all selected file names */
		for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		{
			QFileInfo FileInfo((*it));

			/* Insert list view item. The object which is created here will be
			   automatically destroyed by QT when the parent
			   ("ListViewFileNames") is destroyed */
			ListViewFileNames->insertItem(
				new QListViewItem(ListViewFileNames, FileInfo.fileName(),
				QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2),
				FileInfo.filePath()));
		}
	}
}

void TransmDialog::OnButtonClearAllFileNames()
{
	/* Clear list box for file names */
	ListViewFileNames->clear();
}

void TransmDialog::OnComboBoxTextMessageHighlighted(int iID)
{
	iIDCurrentText = iID;

	/* Set text control with selected message */
	MultiLineEditTextMessage->clear();
	MultiLineEditTextMessage->setEdited(FALSE);

	if (iID != 0)
	{
		/* Write stored text in multi line edit control */
		MultiLineEditTextMessage->insertLine(vecstrTextMessage[iID].c_str());
	}
}

void TransmDialog::OnTextChangedServiceID(const QString& strID)
{
	(void)strID; // TODO
}

void TransmDialog::OnTextChangedServiceLabel(const QString& strLabel)
{
	(void)strLabel; // TODO
}

void TransmDialog::OnComboBoxMSCInterleaverActivated(int iID)
{
	(void)iID; // TODO
}

void TransmDialog::OnButtonAddService()
{
	QListViewItem* v = new QListViewItem(ListViewServices,
        ComboBoxShortID->currentText(),
		LineEditServiceLabel->text(),
		LineEditServiceID->text(),
		ComboBoxFACLanguage->currentText(),
		LineEditSDCLanguage->text(),
		LineEditCountry->text(),
		"-", "-"
	);
    if(CheckBoxDataComp->isEnabled())
    {
        v->setText(6, ComboBoxAppType->currentText());
        v->setText(8, ComboBoxServiceDataStream->currentText());
        v->setText(9, ComboBoxServicePacketID->currentText());
    }
    else
    {
        v->setText(8, "-");
        v->setText(9, "-");
    }
    if(CheckBoxAudioComp->isEnabled())
    {
        /* audio overrides data */
        v->setText(6, ComboBoxProgramType->currentText());
        v->setText(7, ComboBoxServiceAudioStream->currentText());
    }
	ComboBoxShortID->setCurrentItem(0);
}

void TransmDialog::OnButtonDeleteService()
{
	QListViewItem* p = ListViewServices->selectedItem();
	if(p)
	{
		QStringList s(p->text(0));
		for(int i=0; i<ComboBoxShortID->count(); i++)
			s.append(ComboBoxShortID->text(i));
		s.sort();
		ComboBoxShortID->clear();
		ComboBoxShortID->insertStringList(s);
		delete p;
	}
	ComboBoxShortID->setCurrentItem(0);
}

void TransmDialog::OnServicesListItemClicked(QListViewItem* item)
{
	choseComboBoxItem(ComboBoxShortID, item->text(0));
	LineEditServiceLabel->setText(item->text(1));
	LineEditServiceID->setText(item->text(2));
	choseComboBoxItem(ComboBoxFACLanguage, item->text(3));
	LineEditSDCLanguage->setText(item->text(4));
	LineEditCountry->setText(item->text(5));
	if(item->text(8) != "-")
	{
	    CheckBoxDataComp->setEnabled(true);
        choseComboBoxItem(ComboBoxAppType, item->text(6));
        choseComboBoxItem(ComboBoxServiceDataStream, item->text(8));
        choseComboBoxItem(ComboBoxServicePacketID, item->text(9));
	}
	else
	{
	    CheckBoxDataComp->setEnabled(false);
	}
	if(item->text(7) != "-")
	{
	    CheckBoxAudioComp->setEnabled(true);
        choseComboBoxItem(ComboBoxProgramType, item->text(6));
        choseComboBoxItem(ComboBoxServiceAudioStream, item->text(7));
	}
	else
	{
	    CheckBoxAudioComp->setEnabled(false);
	}
}

void TransmDialog::OnComboBoxSDCConstellationActivated(int)
{
}

void TransmDialog::OnComboBoxMSCConstellationActivated(int)
{
	/* Protection level must be re-adjusted when
	 * constellation mode was changed */
	UpdateMSCProtLevCombo();
}

void TransmDialog::OnComboBoxMSCProtLevActivated(int)
{
}

void TransmDialog::UpdateMSCProtLevCombo()
{
	if(ComboBoxMSCConstellation->currentItem()==0)
	{
		/* Only two protection levels possible in 16 QAM mode */
		ComboBoxMSCProtLev->clear();
		ComboBoxMSCProtLev->insertItem("0", 0);
		ComboBoxMSCProtLev->insertItem("1", 1);
	}
	else
	{
		/* Four protection levels defined */
		ComboBoxMSCProtLev->clear();
		ComboBoxMSCProtLev->insertItem("0", 0);
		ComboBoxMSCProtLev->insertItem("1", 1);
		ComboBoxMSCProtLev->insertItem("2", 2);
		ComboBoxMSCProtLev->insertItem("3", 3);
	}

	/* Set protection level to 1 */
	ComboBoxMSCProtLev->setCurrentItem(1);
}

void TransmDialog::OnComboBoxCOFDMDestHighlighted(int iID)
{
	if(iID==ComboBoxCOFDMdest->count()-1)
	{
		QString s( QFileDialog::getSaveFileName(
			"cofdm.wav", "Wave Files (*.wav)", this ) );
		if ( s.isEmpty() )
			return;
		ComboBoxCOFDMdest->changeItem(s, iID);
	}
}

void TransmDialog::OnTextChangedSndCrdIF(const QString& strIF)
{
	(void)strIF; // TODO
}

void TransmDialog::OnRadioBandwidth(int)
{
}

void TransmDialog::OnRadioRobustnessMode(int iID)
{
	/* Check, which bandwidth's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		RadioButtonBandwidth20->setEnabled(TRUE);
		break;

	case 1:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(TRUE);
		RadioButtonBandwidth5->setEnabled(TRUE);
		RadioButtonBandwidth9->setEnabled(TRUE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(TRUE);
		RadioButtonBandwidth20->setEnabled(TRUE);
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonBandwidth20->setEnabled(TRUE);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(TRUE);
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		RadioButtonBandwidth45->setEnabled(FALSE);
		RadioButtonBandwidth5->setEnabled(FALSE);
		RadioButtonBandwidth9->setEnabled(FALSE);
		RadioButtonBandwidth10->setEnabled(TRUE);
		RadioButtonBandwidth18->setEnabled(FALSE);
		RadioButtonBandwidth20->setEnabled(TRUE);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(TRUE);
		break;
	}
}

void TransmDialog::DisableAllControlsForSet()
{
	GroupBoxAudioConfig->setEnabled(FALSE);
	GroupBoxTextMessage->setEnabled(FALSE);
	GroupBoxMOSlideshowApplication->setEnabled(FALSE);
	ButtonGroupBandwidth->setEnabled(FALSE);
	GroupBoxMSC->setEnabled(FALSE);
	GroupBoxSDC->setEnabled(FALSE);
	ButtonGroupRobustnessMode->setEnabled(FALSE);
	ButtonGroupOutput->setEnabled(FALSE);
	LineEditSndCrdIF->setEnabled(FALSE);
	ComboBoxCOFDMdest->setEnabled(FALSE);

	GroupInput->setEnabled(TRUE); /* For run-mode */
}

void TransmDialog::EnableAllControlsForSet()
{
	GroupBoxAudioConfig->setEnabled(TRUE);
	GroupBoxTextMessage->setEnabled(TRUE);
	GroupBoxMOSlideshowApplication->setEnabled(TRUE);
	ButtonGroupBandwidth->setEnabled(TRUE);
	GroupBoxMSC->setEnabled(TRUE);
	GroupBoxSDC->setEnabled(TRUE);
	ButtonGroupRobustnessMode->setEnabled(TRUE);
	ButtonGroupOutput->setEnabled(TRUE);
	LineEditSndCrdIF->setEnabled(TRUE);
	ComboBoxCOFDMdest->setEnabled(TRUE);

	GroupInput->setEnabled(FALSE); /* For run-mode */

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
	ProgressBarCurPict->setProgress(0);
	TextLabelCurPict->setText("");
}

void TransmDialog::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

void TransmDialog::AddWhatsThisHelp()
{
	/* Dream Logo */
	QWhatsThis::add(PixmapLabelDreamLogo,
		tr("<b>Dream Logo:</b> This is the official logo of "
		"the Dream software."));

	/* Input Level */
	QWhatsThis::add(ProgrInputLevel,
		tr("<b>Input Level:</b> The input level meter shows "
		"the relative input signal peak level in dB. If the level is too high, "
		"the meter turns from green to red."));

	/* DRM Robustness Mode */
	const QString strRobustnessMode =
		tr("<b>DRM Robustness Mode:</b> In a DRM system, "
		"four possible robustness modes are defined to adapt the system to "
		"different channel conditions. According to the DRM standard:"
		"<ul><li><i>Mode A:</i> Gaussian channels, with "
		"minor fading</li><li><i>Mode B:</i> Time "
		"and frequency selective channels, with longer delay spread"
		"</li><li><i>Mode C:</i> As robustness mode B, "
		"but with higher Doppler spread</li><li><i>Mode D:"
		"</i> As robustness mode B, but with severe delay and "
		"Doppler spread</li></ul>");

	QWhatsThis::add(RadioButtonRMA, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMB, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMC, strRobustnessMode);
	QWhatsThis::add(RadioButtonRMD, strRobustnessMode);

	/* Bandwidth */
	const QString strBandwidth =
		tr("<b>DRM Bandwidth:</b> The bandwith is the gross "
		"bandwidth of the generated DRM signal. Not all DRM robustness mode / "
		"bandwidth constellations are possible, e.g., DRM robustness mode D "
		"and C are only defined for the bandwidths 10 kHz and 20 kHz.");

	QWhatsThis::add(RadioButtonBandwidth45, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth5, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth9, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth10, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth18, strBandwidth);
	QWhatsThis::add(RadioButtonBandwidth20, strBandwidth);

	/* Output intermediate frequency of DRM signal */
	const QString strOutputIF =
		tr("<b>Output intermediate frequency of DRM signal:</b> "
		"Set the output intermediate frequency (IF) of generated DRM signal "
		"in the 'sound-card pass-band'. In some DRM modes, the IF is located "
		"at the edge of the DRM signal, in other modes it is centered. The IF "
		"should be chosen that the DRM signal lies entirely inside the "
		"sound-card bandwidth.");

	QWhatsThis::add(TextLabelIF, strOutputIF);
	QWhatsThis::add(LineEditSndCrdIF, strOutputIF);
	QWhatsThis::add(TextLabelIFUnit, strOutputIF);

	/* Output format */
	const QString strOutputFormat =
		tr("<b>Output format:</b> Since the sound-card "
		"outputs signals in stereo format, it is possible to output the DRM "
		"signal in three formats:<ul><li><b>real valued"
		"</b> output on both, left and right, sound-card "
		"channels</li><li><b>I / Q</b> output "
		"which is the in-phase and quadrature component of the complex "
		"base-band signal at the desired IF. In-phase is output on the "
		"left channel and quadrature on the right channel."
		"</li><li><b>E / P</b> output which is the "
		"envelope and phase on separate channels. This output type cannot "
		"be used if the Dream transmitter is regularly compiled with a "
		"sound-card sample rate of 48 kHz since the spectrum of these "
		"components exceed the bandwidth of 20 kHz.<br>The envelope signal "
		"is output on the left channel and the phase is output on the right "
		"channel.</li></ul>");

	QWhatsThis::add(RadioButtonOutReal, strOutputFormat);
	QWhatsThis::add(RadioButtonOutIQPos, strOutputFormat);
	QWhatsThis::add(RadioButtonOutIQNeg, strOutputFormat);
	QWhatsThis::add(RadioButtonOutEP, strOutputFormat);

	/* MSC interleaver mode */
	const QString strInterleaver =
		tr("<b>MSC interleaver mode:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the longer "
		"the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	QWhatsThis::add(TextLabelInterleaver, strInterleaver);
	QWhatsThis::add(ComboBoxMSCInterleaver, strInterleaver);
}

void
TransmDialog::choseComboBoxItem(QComboBox* box, const QString& text)
{
	int i=0;
	for(i=0; i<box->count(); i++)
	{
		if(box->text(i) == text)
		{
			box->setCurrentItem(i);
			return;
		}
	}
	box->insertItem(text);
	box->setCurrentItem(box->count()-1);
}
