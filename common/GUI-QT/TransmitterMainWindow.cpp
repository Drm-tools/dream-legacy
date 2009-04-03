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

#include "TransmitterMainWindow.h"
#include <QHostAddress>
#include <q3filedialog.h>
#include <QMessageBox>
#include <QFileInfo>
#include <QCloseEvent>
#include "DialogUtil.h"
#include "../DrmTransmitter.h"
#include <sstream>
#include <fstream>

TransmitterMainWindow::TransmitterMainWindow(CDRMTransmitterInterface& tx, CSettings& NSettings,
	QWidget* parent, const char* name, Qt::WFlags f
	):
	QMainWindow(parent, name, f),
	Ui_TransmitterMainWindow(),
	pMenu(NULL), pSettingsMenu(NULL), Timer(),
	DRMTransmitter(tx), Settings(NSettings),
	bIsStarted(false),vecIpIf()
{
	int i;
	size_t t;
	vector<string> vecAudioDevices;

    setupUi(this);

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

    setCaption(tr("Dream DRM Transmitter"));

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(Qt::Horizontal, QwtThermo::BottomScale);
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
		DRMTransmitter.GetParameters()->rCarOffset, 'f', 2));

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
	vecAudioDevices.clear();
	DRMTransmitter.GetSoundOutChoices(vecAudioDevices);
	for (t = 0; t < vecAudioDevices.size(); t++)
	{
		ComboBoxCOFDMdest->insertItem(QString(vecAudioDevices[t].c_str()));
	}
	ComboBoxCOFDMdest->setCurrentItem(0);
	ListViewCOFDM->setAllColumnsShowFocus(true);
	ListViewCOFDM->setColumnWidthMode(0, Q3ListView::Maximum);

	/* Clear list box for file names and set up columns */
	ListViewFileNames->setAllColumnsShowFocus(true);

	ListViewFileNames->clear();

	/* We assume that one column is already there */
	ListViewFileNames->setColumnText(0, "File Name");
	ListViewFileNames->addColumn("Size [KB]");
	ListViewFileNames->addColumn("Full Path");

	/* Set audio enable check box */
	CheckBoxSBR->setChecked(true);

	/* enable services for initialization */
	EnableAudio(true);
	EnableData(true);

	/* Fill Audio source selection */
	/* Get sound device names */
	vecAudioDevices.clear();
	DRMTransmitter.GetSoundInChoices(vecAudioDevices);
	for (t = 0; t < vecAudioDevices.size(); t++)
	{
		ComboBoxAudioSource->insertItem(QString(vecAudioDevices[t].c_str()), t);
	}
	ComboBoxAudioSource->setCurrentItem(0);

	ListViewTextMessages->clear();
	ListViewTextMessages->setAllColumnsShowFocus(true);

	LineEditMDIinGroup->setEnabled(false);
	LineEditMDIinGroup->setInputMask("000.000.000.000;_");
	LineEditMDIOutAddr->setInputMask("000.000.000.000;_");
	LineEditMDIinPort->setInputMask("00009;_");
	LineEditMDIoutPort->setInputMask("00009;_");

	/* Enable all controls */
	EnableAllControlsForSet();


	/* Set Menu ***************************************************************/

	/* Main menu bar */
	pMenu = new QMenuBar(this);
	Q_CHECK_PTR(pMenu);
	//pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	setMenuBar(pMenu);

	/* NOT implemented yet */
    ComboBoxCodec->setEnabled(false);
    ComboBoxAudioMode->setEnabled(false);
    ComboBoxAudioBitrate->setEnabled(false);
    CheckBoxSBR->setEnabled(false);

	/* Connections ---------------------------------------------------------- */

	/* General */
	connect(ButtonStartStop, SIGNAL(clicked()),
        this, SLOT(OnButtonStartStop()));
	connect(buttonClose, SIGNAL(clicked()),
        this, SLOT(OnButtonClose()));
	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	/* channel */
	connect(ComboBoxMSCInterleaver, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCInterleaverActivated(int)));
	connect(ComboBoxMSCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCConstellationActivated(int)));
	connect(ComboBoxMSCProtLev, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCProtLevActivated(int)));
	connect(ComboBoxSDCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSDCConstellationActivated(int)));
	connect(ButtonGroupMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioMode(int)));
	connect(ButtonGroupRobustnessMode, SIGNAL(clicked(int)),
		this, SLOT(OnRadioRobustnessMode(int)));
	connect(ButtonGroupBandwidth, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBandwidth(int)));

	/* Audio (and text messages) */
	connect(ComboBoxAudioSource, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxAudioSourceActivated(int)));
	connect(PushButtonAudioSourceFileBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonAudioSourceFileBrowse()));
	connect(CheckBoxAudioSourceIsFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxAudioSourceIsFile(bool)));
	connect(LineEditAudioSourceFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditAudioSourceFileChanged(const QString&)));

	connect(PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(PushButtonDeleteText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonDeleteText()));
	connect(PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));

	/* Data */
	connect(PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));

	/* streams */
	connect(ButtonAddStream, SIGNAL(clicked()),
		this, SLOT(OnButtonAddStream()));
	connect(ButtonDeleteStream, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteStream()));
	connect(ComboBoxStreamType, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxStreamTypeActivated(int)));
	connect(ComboBoxPacketsPerFrame, SIGNAL(activated(const QString&)),
		this, SLOT(OnComboBoxPacketsPerFrameActivated(const QString&)));
	connect(LineEditPacketLen, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPacketLenChanged(const QString&)));
	connect(ListViewStreams, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnStreamsListItemClicked(Q3ListViewItem*)));

	/* services */
	connect(PushButtonAddService, SIGNAL(clicked()),
		this, SLOT(OnButtonAddService()));
	connect(PushButtonDeleteService, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteService()));
	connect(LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceLabel(const QString&)));
	connect(LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(ListViewServices, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnServicesListItemClicked(Q3ListViewItem*)));

    /* MDI Input */
	connect(PushButtonMDIInBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonMDIInBrowse()));
	connect(CheckBoxMDIinMcast, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxMDIinMcast(bool)));
	connect(CheckBoxReadMDIFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxReadMDIFile(bool)));
	connect(ComboBoxMDIinInterface, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMDIinInterfaceActivated(int)));
	connect(LineEditMDIinPort, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIinPortChanged(const QString&)));
	connect(LineEditMDIinGroup, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIinGroupChanged(const QString&)));
	connect(LineEditMDIInputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIInputFileChanged(const QString&)));

    /* MDI Output */
	connect(PushButtonAddMDIDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddMDIDest()));
	connect(PushButtonAddMDIFileDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddMDIFileDest()));
	connect(PushButtonDeleteMDIOutput, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteMDIOutput()));
	connect(PushButtonMDIOutBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonMDIOutBrowse()));
	connect(ComboBoxMDIoutInterface, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMDIoutInterfaceActivated(int)));
	connect(LineEditMDIOutAddr, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIOutAddrChanged(const QString&)));
	connect(LineEditMDIOutputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIOutputFileChanged(const QString&)));
	connect(LineEditMDIoutPort, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditMDIoutPortChanged(const QString&)));
	connect(ListViewMDIOutputs, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnMDIOutListItemClicked(Q3ListViewItem*)));

    /* COFDM */
	connect(PushButtonCOFDMAddAudio, SIGNAL(clicked()),
		this, SLOT(OnButtonCOFDMAddAudio()));
	connect(PushButtonCOFDMAddFile, SIGNAL(clicked()),
		this, SLOT(OnButtonCOFDMAddFile()));
	connect(PushButtonCOFDMDeleteSelected, SIGNAL(clicked()),
		this, SLOT(OnButtonCOFDMDeleteSelected()));
	connect(PushButtonCOFDMBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonCOFDMBrowse()));
	connect(ComboBoxCOFDMdest, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxCOFDMDestActivated(int)));
	connect(LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));
	connect(LineEditCOFDMOutputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditCOFDMOutputFileChanged(const QString&)));
	connect(ListViewCOFDM, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnCOFDMOutListItemClicked(Q3ListViewItem*)));

	ListViewStreams->setAllColumnsShowFocus(true);
	ListViewStreams->clear();
	ComboBoxStreamType->insertItem(tr("audio"));
	ComboBoxStreamType->insertItem(tr("data packet"));
	ComboBoxStreamType->insertItem(tr("data stream"));
	ComboBoxStream->insertItem("0");
	ComboBoxStream->insertItem("1");
	ComboBoxStream->insertItem("2");
	ComboBoxStream->insertItem("3");

	ListViewServices->setAllColumnsShowFocus(true);
	ListViewServices->clear();
	ComboBoxShortID->insertItem("0");
	ComboBoxShortID->insertItem("1");
	ComboBoxShortID->insertItem("2");
	ComboBoxShortID->insertItem("3");

    ComboBoxServiceAudioStream->clear();
	ComboBoxServiceAudioStream->insertItem("0");
	ComboBoxServiceAudioStream->insertItem("1");
	ComboBoxServiceAudioStream->insertItem("2");
	ComboBoxServiceAudioStream->insertItem("3");

    ComboBoxServiceDataStream->clear();
	ComboBoxServiceDataStream->insertItem("0");
	ComboBoxServiceDataStream->insertItem("1");
	ComboBoxServiceDataStream->insertItem("2");
	ComboBoxServiceDataStream->insertItem("3");

    ComboBoxServicePacketID->clear();
	ComboBoxServicePacketID->insertItem("0");
	ComboBoxServicePacketID->insertItem("1");
	ComboBoxServicePacketID->insertItem("2");
	ComboBoxServicePacketID->insertItem("3");

    ComboBoxAudioStreamNo->clear();
	ComboBoxAudioStreamNo->insertItem("0");
	ComboBoxAudioStreamNo->insertItem("1");
	ComboBoxAudioStreamNo->insertItem("2");
	ComboBoxAudioStreamNo->insertItem("3");

    ComboBoxDataStreamNo->clear();
	ComboBoxDataStreamNo->insertItem("0");
	ComboBoxDataStreamNo->insertItem("1");
	ComboBoxDataStreamNo->insertItem("2");
	ComboBoxDataStreamNo->insertItem("3");

    ComboBoxDataPacketId->clear();
	ComboBoxDataPacketId->insertItem("0");
	ComboBoxDataPacketId->insertItem("1");
	ComboBoxDataPacketId->insertItem("2");
	ComboBoxDataPacketId->insertItem("3");

    ComboBoxAppType->clear();
    ComboBoxAppType->insertItem(tr("Normal"));
    ComboBoxAppType->insertItem(tr("Engineering Test"));
	for (t = 1; t < 31; t++)
	{
	    QString reserved = QString(tr("Reserved"))+" (%1)";
		ComboBoxAppType->insertItem(reserved.arg(t));
	}

	GetFromTransmitter();
	OnRadioMode(0);

	LineEditMDIinPort->setCursorPosition(0);

	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

TransmitterMainWindow::~TransmitterMainWindow()
{
}

void
TransmitterMainWindow::closeEvent(QCloseEvent* ce)
{
	/* Stop transmitter if needed */
	if (bIsStarted == true)
		OnButtonStartStop();
	else
		SetTransmitter(); // so Transmitter save settings has the latest info
	ce->accept();
}

void TransmitterMainWindow::OnButtonClose()
{
    this->close(false);
}

void TransmitterMainWindow::OnTimer()
{
	/* Set value for input level meter (only in "start" mode) */
	if (bIsStarted == true)
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
			ProgressBarCurPict->setEnabled(true);
			TextLabelCurPict->setEnabled(true);

			/* We want to file name, not the complete path -> "QFileInfo" */
			QFileInfo FileInfo(strCPictureName.c_str());

			/* Show current file name and percentage */
			TextLabelCurPict->setText(FileInfo.fileName());
			ProgressBarCurPict->setProgress((int) (rCPercent * 100)); /* % */
		}
		else
		{
			/* Disable controls */
			ProgressBarCurPict->setEnabled(false);
			TextLabelCurPict->setEnabled(false);
		}
		time_t t = time(NULL);
		if((t % 5) == 0)
		{
		    ofstream f("p.txt", ios::app | ios::out);
		    DRMTransmitter.GetParameters()->dump(f);
		    f.close();
		}
	}
}

void
TransmitterMainWindow::GetFromTransmitter()
{
	switch(DRMTransmitter.GetOperatingMode())
	{
	case CDRMTransmitterInterface::T_ENC:
		GetChannel();
		GetStreams();
		GetAudio(ComboBoxAudioStreamNo->currentItem());
		GetData(
            ComboBoxDataStreamNo->currentItem(),
            ComboBoxDataPacketId->currentItem()
        );
		GetServices();
		GetMDIOut();
		RadioButtonEncoder->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_MOD:
		GetMDIIn();
		GetCOFDM();
		RadioButtonModulator->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_TX:
		GetChannel();
		GetStreams();
		GetAudio(ComboBoxAudioStreamNo->currentItem());
		GetData(
            ComboBoxDataStreamNo->currentItem(),
            ComboBoxDataPacketId->currentItem()
        );
		GetServices();
		GetCOFDM();
		RadioButtonTransmitter->setChecked(true);
		break;
	}
}

void
TransmitterMainWindow::GetChannel()
{
	/* Robustness mode */
	switch (DRMTransmitter.GetParameters()->Channel.eRobustness)
	{
	case RM_ROBUSTNESS_MODE_A:
		RadioButtonRMA->setChecked(true);
		break;

	case RM_ROBUSTNESS_MODE_B:
		RadioButtonRMB->setChecked(true);
		break;

	case RM_ROBUSTNESS_MODE_C:
		RadioButtonRMC->setChecked(true);
		break;

	case RM_ROBUSTNESS_MODE_D:
		RadioButtonRMD->setChecked(true);
		break;

	case RM_NO_MODE_DETECTED:
		;
	}

	/* Bandwidth */
	switch (DRMTransmitter.GetParameters()->Channel.eSpectrumOccupancy)
	{
	case SO_0:
		RadioButtonBandwidth45->setChecked(true);
		break;

	case SO_1:
		RadioButtonBandwidth5->setChecked(true);
		break;

	case SO_2:
		RadioButtonBandwidth9->setChecked(true);
		break;

	case SO_3:
		RadioButtonBandwidth10->setChecked(true);
		break;

	case SO_4:
		RadioButtonBandwidth18->setChecked(true);
		break;

	case SO_5:
		RadioButtonBandwidth20->setChecked(true);
		break;
	}

	switch (DRMTransmitter.GetParameters()->Channel.eInterleaverDepth)
	{
	case SI_LONG:
		ComboBoxMSCInterleaver->setCurrentItem(0);
		break;

	case SI_SHORT:
		ComboBoxMSCInterleaver->setCurrentItem(1);
		break;
	}

	switch (DRMTransmitter.GetParameters()->Channel.eMSCmode)
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
	ComboBoxMSCProtLev->setCurrentItem(DRMTransmitter.GetParameters()->MSCParameters.ProtectionLevel.iPartB);

	switch (DRMTransmitter.GetParameters()->Channel.eSDCmode)
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
	UpdateCapacities();
}

void
TransmitterMainWindow::SetChannel()
{
    CParameter& Parameters = *DRMTransmitter.GetParameters();
	/* Spectrum Occupancy */
	if(RadioButtonBandwidth45->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_0;
	if(RadioButtonBandwidth5->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_1;
	if(RadioButtonBandwidth9->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_2;
	if(RadioButtonBandwidth10->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_3;
	if(RadioButtonBandwidth18->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_4;
	if(RadioButtonBandwidth20->isChecked())
		Parameters.Channel.eSpectrumOccupancy = SO_5;

	/* MSC Protection Level */
	CMSCProtLev MSCPrLe;
	MSCPrLe.iPartB = ComboBoxMSCProtLev->currentItem();
	Parameters.MSCParameters.ProtectionLevel = MSCPrLe;

	/* MSC Constellation Scheme */
	switch(ComboBoxMSCConstellation->currentItem())
	{
	case 0:
		Parameters.Channel.eMSCmode = CS_2_SM;
		break;

	case 1:
		Parameters.Channel.eMSCmode = CS_3_SM;
		break;

	case 2:
		Parameters.Channel.eMSCmode = CS_3_HMSYM;
		break;

	case 3:
		Parameters.Channel.eMSCmode = CS_3_HMMIX;
		break;
	}

	/* MSC interleaver mode */
	switch(ComboBoxMSCInterleaver->currentItem())
	{
	case 0:
		Parameters.Channel.eInterleaverDepth = SI_LONG;
		break;
	case 1:
		Parameters.Channel.eInterleaverDepth = SI_SHORT;
		break;
	}


	/* SDC */
	switch(ComboBoxSDCConstellation->currentItem())
	{
	case 0:
		Parameters.Channel.eSDCmode = CS_1_SM;
		break;

	case 1:
		Parameters.Channel.eSDCmode = CS_2_SM;
		break;
	default:
		;
	}

	/* Robustness Mode */
	if(RadioButtonRMA->isChecked())
		Parameters.Channel.eRobustness = RM_ROBUSTNESS_MODE_A;
	if(RadioButtonRMB->isChecked())
		Parameters.Channel.eRobustness = RM_ROBUSTNESS_MODE_B;
	if(RadioButtonRMC->isChecked())
		Parameters.Channel.eRobustness = RM_ROBUSTNESS_MODE_C;
	if(RadioButtonRMD->isChecked())
		Parameters.Channel.eRobustness = RM_ROBUSTNESS_MODE_D;

	UpdateCapacities();
}

void
TransmitterMainWindow::UpdateCapacities()
{
	DRMTransmitter.CalculateChannelCapacities();

	TextLabelMSCCapBits->setText(QString::number(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC));
	TextLabelMSCCapBytes->setText(QString::number(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC/8));
	TextLabelMSCBytesTotal->setText(QString::number(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC/8));

	TextLabelSDCCapBits->setText(QString::number(DRMTransmitter.GetParameters()->iNumSDCBitsPerSuperFrame));
	TextLabelSDCCapBytes->setText(QString::number(DRMTransmitter.GetParameters()->iNumSDCBitsPerSuperFrame/8));

	TextLabelMSCBytesAvailable->setText(TextLabelMSCCapBytes->text());
}

void
TransmitterMainWindow::GetStreams()
{
	for(size_t i=0; i<DRMTransmitter.GetParameters()->MSCParameters.Stream.size(); i++)
	{
		const CStream& stream = DRMTransmitter.GetParameters()->MSCParameters.Stream[i];
        int bytes = stream.iLenPartB; // EEP Only
        if(stream.iLenPartA != 0)
        {
            QMessageBox::information(this, "Dream", tr("UEP stream read from transmitter - ignored"),
            QMessageBox::Ok);
        }
		ComboBoxStream->setCurrentItem(i);
		if(stream.eAudDataFlag == SF_AUDIO)
		{
			ComboBoxStreamType->setCurrentItem(0);
			LineEditPacketLen->setText("-");
			ComboBoxPacketsPerFrame->clear();
			ComboBoxPacketsPerFrame->insertItem("-");
			// currently, only a single audio stream is supported.
			// set the audio tab to this stream
			ComboBoxAudioStreamNo->setCurrentItem(i);
		}
		else
		{
			if(stream.ePacketModInd == PM_PACKET_MODE)
			{
				ComboBoxStreamType->setCurrentItem(1);
				LineEditPacketLen->setText(QString::number(stream.iPacketLen));
				ComboBoxPacketsPerFrame->clear();
				ComboBoxPacketsPerFrame->insertItem(QString::number(bytes/stream.iPacketLen));
			}
			else
			{
				ComboBoxStreamType->setCurrentItem(2);
				LineEditPacketLen->setText("-");
				ComboBoxPacketsPerFrame->clear();
				ComboBoxPacketsPerFrame->insertItem("-");
			}
			// currently, only a single data stream is supported.
			// set the data tab to this stream
			ComboBoxDataStreamNo->setCurrentItem(i);
		}
		LineEditBitsPerFrame->setText(QString::number(8*bytes));
		OnButtonAddStream();
	}
}

void
TransmitterMainWindow::SetStreams()
{
	DRMTransmitter.GetParameters()->MSCParameters.Stream.resize(ListViewStreams->childCount());
	Q3ListViewItemIterator it(ListViewStreams);
	for (; it.current(); it++)
	{
	    int iStreamID = it.current()->text(0).toInt();
        CStream& stream = DRMTransmitter.GetParameters()->MSCParameters.Stream[iStreamID];
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
        // iLen in bytes = column 5, not bits
        stream.iLenPartA = 0; // EEP
        stream.iLenPartB = it.current()->text(5).toInt();
	}
}

void
TransmitterMainWindow::GetAudio(int iStreamNo)
{
    string fn = DRMTransmitter.GetReadFromFile();
    LineEditAudioSourceFile->setText(fn.c_str());
    if(fn == "")
    {
        int iAudSrc = DRMTransmitter.GetSoundInInterface();
        if(iAudSrc == -1 || iAudSrc>=ComboBoxAudioSource->count())
            ComboBoxAudioSource->setCurrentItem(ComboBoxAudioSource->count()-1);
        else
            ComboBoxAudioSource->setCurrentItem(iAudSrc);
    }

    if(DRMTransmitter.GetParameters()->AudioParam[iStreamNo].bTextflag == true)
    {
        /* Activate text message */
        EnableTextMessage(true);
        CheckBoxEnableTextMessage->setChecked(true);
        vector<string> msg;
        DRMTransmitter.GetTextMessages(msg);
        for(size_t i=0; i<msg.size(); i++)
            ListViewTextMessages->insertItem(new Q3ListViewItem(ListViewTextMessages, msg[i].c_str()));
    }
    else
    {
        EnableTextMessage(false);
        CheckBoxEnableTextMessage->setChecked(false);
    }
}

void
TransmitterMainWindow::SetAudio(int iStreamNo)
{
	CAudioParam& AudioParam = DRMTransmitter.GetParameters()->AudioParam[iStreamNo];

	if(CheckBoxAudioSourceIsFile->isChecked())
	{
		DRMTransmitter.SetReadFromFile(LineEditAudioSourceFile->text().latin1());
	}
	else
	{
		int iAudSrc = ComboBoxAudioSource->currentItem();
		DRMTransmitter.SetSoundInInterface(iAudSrc);
	}

	AudioParam.bTextflag = CheckBoxEnableTextMessage->isChecked();

	if(AudioParam.bTextflag)
	{
		DRMTransmitter.ClearTextMessages();
        Q3ListViewItemIterator it(ListViewTextMessages);
        for (; it.current(); it++)
        {
			DRMTransmitter.AddTextMessage(it.current()->text(0).toUtf8().constData());
        }
	}

	/* TODO - let the user choose */
	if (DRMTransmitter.GetParameters()->GetStreamLen(0) > 7000)
		AudioParam.eAudioSamplRate = CAudioParam::AS_24KHZ;
	else
		AudioParam.eAudioSamplRate = CAudioParam::AS_12KHZ;
}

void TransmitterMainWindow::OnComboBoxAudioSourceActivated(int iID)
{
}

void TransmitterMainWindow::OnButtonAudioSourceFileBrowse()
{
	QString s( Q3FileDialog::getOpenFileName(
		QString::null, "Wave Files (*.wav)", this ) );
	if ( s.isEmpty() )
		return;
    LineEditAudioSourceFile->setText(s);
}

void TransmitterMainWindow::OnLineEditAudioSourceFileChanged(const QString&)
{
}

void TransmitterMainWindow::OnToggleCheckBoxAudioSourceIsFile(bool)
{
}

void TransmitterMainWindow::OnToggleCheckBoxEnableTextMessage(bool bState)
{
	EnableTextMessage(bState);
}

void TransmitterMainWindow::EnableTextMessage(const bool bFlag)
{
	if (bFlag == true)
	{
		/* Enable text message controls */
		PushButtonAddText->setEnabled(true);
		PushButtonDeleteText->setEnabled(true);
		PushButtonClearAllText->setEnabled(true);
	}
	else
	{
		/* Disable text message controls */
		PushButtonAddText->setEnabled(false);
		PushButtonDeleteText->setEnabled(false);
		PushButtonClearAllText->setEnabled(false);
	}
}

void
TransmitterMainWindow:: GetData(int iStreamNo, int iPacketId)
{
	CDataParam& DataParam = DRMTransmitter.GetParameters()->DataParam[iStreamNo][iPacketId];

	if(DataParam.eAppDomain != CDataParam::AD_DAB_SPEC_APP)
        return; // we only do slideshow!

	/* Set file names for data application */
	map<string,string> m;
    DRMTransmitter.GetPics(m);

	ListViewFileNames->clear();
	for (map<string,string>::const_iterator i=m.begin(); i!=m.end(); i++)
	{
	    AddSlide(i->first.c_str());
	}
}

void
TransmitterMainWindow::SetData(int iStreamNo, int iPacketId)
{
	CParameter& Parameters = *DRMTransmitter.GetParameters();

	Parameters.SetCurSelDataService(0); // TODO
	CDataParam& DataParam = Parameters.DataParam[iStreamNo][iPacketId];

	/* Init SlideShow application */
	DataParam.eAppDomain = CDataParam::AD_DAB_SPEC_APP;
	DataParam.ePacketModInd = PM_PACKET_MODE;
	DataParam.eDataUnitInd = CDataParam::DU_DATA_UNITS;

	/* Set file names for data application */
	DRMTransmitter.ClearPics();

	Q3ListViewItemIterator it(ListViewFileNames);

	for (; it.current(); it++)
	{
		/* Complete file path is in third column */
		const QString strFileName = it.current()->text(2);

		/* Extract format string */
		QFileInfo FileInfo(strFileName);
		const QString strFormat = FileInfo.extension(false);

		DRMTransmitter.AddPic(strFileName.latin1(), strFormat.latin1());
	}
}

void
TransmitterMainWindow::GetServices()
{
	const vector<CService>& Service = DRMTransmitter.GetParameters()->Service;
	for(size_t i=0; i<Service.size(); i++)
	{
		Q3ListViewItem* v = new Q3ListViewItem(ListViewServices, QString::number(i),
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
TransmitterMainWindow::SetServices()
{
	DRMTransmitter.GetParameters()->FACParameters.iNumDataServices=0;
	DRMTransmitter.GetParameters()->FACParameters.iNumAudioServices=0;

	Q3ListViewItemIterator sit(ListViewServices);
	for (; sit.current(); sit++)
	{
		int iShortID = sit.current()->text(0).toUInt();
		CService Service;
		Service.strLabel = sit.current()->text(1).toUtf8().constData();
		Service.iServiceID = sit.current()->text(2).toULong(NULL, 16);
		QString lang = sit.current()->text(3);
		for(int i=0; i<ComboBoxFACLanguage->count(); i++)
            if(ComboBoxFACLanguage->text(i)==lang)
                Service.iLanguage = i;
		Service.strLanguageCode = sit.current()->text(4).toUtf8().constData();
		Service.strCountryCode = sit.current()->text(5).toUtf8().constData();
        QString type = sit.current()->text(6);
		QString sA = sit.current()->text(7);
		QString sD = sit.current()->text(8);
		QString sP = sit.current()->text(9);
		if(sA=="-")
		{
			Service.iAudioStream = STREAM_ID_NOT_USED;
			DRMTransmitter.GetParameters()->FACParameters.iNumDataServices++;
			Service.eAudDataFlag = SF_DATA;
            for(int i=0; i<ComboBoxAppType->count(); i++)
                if(ComboBoxAppType->text(i)==type)
                    Service.iServiceDescr = i;
		}
		else
		{
			Service.iAudioStream = sA.toUInt();
			DRMTransmitter.GetParameters()->FACParameters.iNumAudioServices++;
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
		DRMTransmitter.GetParameters()->Service[iShortID] = Service;
	}
}

void
TransmitterMainWindow::SetTransmitter()
{
	CDRMTransmitterInterface::ETxOpMode eMod = CDRMTransmitterInterface::T_TX;

	if(RadioButtonTransmitter->isChecked() || RadioButtonEncoder->isChecked())
	{
		SetChannel();
		SetStreams();
		SetAudio(ComboBoxAudioStreamNo->currentItem());
		// TODO - which service(s) is the data associated to ?
		SetData(
            ComboBoxDataStreamNo->currentItem(),
            ComboBoxDataPacketId->currentItem()
        );
		SetServices();
	}
	if(RadioButtonTransmitter->isChecked() || RadioButtonModulator->isChecked())
	{
		SetCOFDM();
	}
	if(RadioButtonEncoder->isChecked())
	{
		SetMDIOut();
		eMod = CDRMTransmitterInterface::T_ENC;
	}
	if(RadioButtonModulator->isChecked())
	{
		SetMDIIn();
		eMod = CDRMTransmitterInterface::T_MOD;
	}
	DRMTransmitter.SetOperatingMode(eMod);
}

/* MDI Input */

void
TransmitterMainWindow::GetMDIIn()
{
	QString addr = DRMTransmitter.GetMDIIn().c_str();
	QFileInfo f(addr);
	if(f.exists())
	{
		CheckBoxReadMDIFile->setChecked(true);
		LineEditMDIInputFile->setText(addr);
		LineEditMDIinPort->setText("");
		choseComboBoxItem(ComboBoxMDIinInterface, "any");
		LineEditMDIinGroup->setText("");
		CheckBoxMDIinMcast->setChecked(false);
	}
	else
	{
		CheckBoxReadMDIFile->setChecked(false);
		LineEditMDIInputFile->setText("");

		QStringList parts = QStringList::split(":", addr, true);
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
}

void
TransmitterMainWindow::SetMDIIn()
{
	if(CheckBoxReadMDIFile->isChecked())
	{
		DRMTransmitter.SetMDIIn(LineEditMDIInputFile->text().latin1());
	}
	else
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
		DRMTransmitter.SetMDIIn(addr.latin1());
	}
}


void
TransmitterMainWindow::OnLineEditMDIinPortChanged(const QString& str)
{
}

void
TransmitterMainWindow::OnToggleCheckBoxMDIinMcast(bool bState)
{
}

void
TransmitterMainWindow::OnLineEditMDIinGroupChanged(const QString& str)
{
}

void
TransmitterMainWindow::OnComboBoxMDIinInterfaceActivated(int iID)
{
}

void
TransmitterMainWindow::OnLineEditMDIInputFileChanged(const QString& str)
{
}

void
TransmitterMainWindow::OnButtonMDIInBrowse()
{
	QString s( Q3FileDialog::getOpenFileName(
		"out.pcap", "Capture Files (*.pcap)", this ) );
	if ( s.isEmpty() )
		return;
    LineEditMDIInputFile->setText(s);
}

void
TransmitterMainWindow::OnToggleCheckBoxReadMDIFile(bool)
{
}

/* MDI Output */

void
TransmitterMainWindow::GetMDIOut()
{
	vector<string> MDIoutAddr;
	DRMTransmitter.GetMDIOut(MDIoutAddr);
    for(size_t i=0; i<MDIoutAddr.size(); i++)
    {
        QString addr = MDIoutAddr[i].c_str();
        QStringList parts = QStringList::split(":", addr, true);
        switch(parts.count())
        {
        case 0:
            (void)new Q3ListViewItem(ListViewMDIOutputs, parts[0]);
            break;
        case 1:
            (void)new Q3ListViewItem(ListViewMDIOutputs, parts[0], "", "any");
            break;
        case 2:
            (void)new Q3ListViewItem(ListViewMDIOutputs, parts[1], parts[0], "any");
            break;
        case 3:
            {
                QString name;
                for(size_t j=0; j<vecIpIf.size(); j++)
                {
                    if(parts[0].toUInt()==vecIpIf[j].addr)
                        name = vecIpIf[j].name.c_str();
                }
                (void)new Q3ListViewItem(ListViewMDIOutputs, parts[2], parts[1], name);
            }
        }
    }
}

void
TransmitterMainWindow::SetMDIOut()
{
	vector<string> MDIoutAddr;
	Q3ListViewItemIterator it(ListViewMDIOutputs);
	for (; it.current(); it++)
	{
		QString port = it.current()->text(0);
		QString dest = it.current()->text(1);
		QString iface = it.current()->text(2);
		QString addr;
		if(dest == "")
		{
		    addr = port;
		}
		else if(iface=="any")
		{
			addr = dest+":"+port;
		}
		else
		{
			uint32_t iInterface=0;
			for(size_t i=0; i<vecIpIf.size(); i++)
				if(vecIpIf[i].name.c_str()==iface)
					iInterface = vecIpIf[i].addr;
			addr = QHostAddress(iInterface).toString()+":"+dest+":"+port;
		}
		MDIoutAddr.push_back(string(addr.utf8()));
	}
	DRMTransmitter.SetMDIOut(MDIoutAddr);
}

void
TransmitterMainWindow::OnButtonAddMDIDest()
{
	QString dest = LineEditMDIOutAddr->text();
	if(dest == "...")
		dest = "";
    (void) new Q3ListViewItem(ListViewMDIOutputs,
        LineEditMDIoutPort->text(),
        dest,
        ComboBoxMDIoutInterface->currentText()
    );
}

void
TransmitterMainWindow::OnButtonAddMDIFileDest()
{
	QString file = LineEditMDIOutputFile->text();
	if(file != "")
		(void) new Q3ListViewItem(ListViewMDIOutputs, file);
}

void
TransmitterMainWindow::OnButtonDeleteMDIOutput()
{
	Q3ListViewItem* p = ListViewMDIOutputs->selectedItem();
	if(p)
		delete p;
}

void TransmitterMainWindow::OnButtonMDIOutBrowse()
{
    QString s( Q3FileDialog::getSaveFileName(
			"out.pcap", "Capture Files (*.pcap)", this ) );
    if ( s.isEmpty() )
        return;
    LineEditMDIOutputFile->setText(s);
}

void TransmitterMainWindow::OnComboBoxMDIoutInterfaceActivated(int)
{
}

void TransmitterMainWindow::OnLineEditMDIOutAddrChanged(const QString& str)
{
}

void TransmitterMainWindow::OnLineEditMDIOutputFileChanged(const QString& str)
{
}

void TransmitterMainWindow::OnLineEditMDIoutPortChanged(const QString& str)
{
}

void TransmitterMainWindow::OnMDIOutListItemClicked(Q3ListViewItem* item)
{
	QString dest = item->text(1);
	if(dest == "")
	{
    	LineEditMDIOutputFile->setText(item->text(0));
        LineEditMDIoutPort->setText("");
		LineEditMDIOutAddr->setText("");
    	choseComboBoxItem(ComboBoxMDIoutInterface, "any");
	}
	else
	{
    	LineEditMDIOutputFile->setText("");
        LineEditMDIoutPort->setText(item->text(0));
		LineEditMDIOutAddr->setText(dest);
    	choseComboBoxItem(ComboBoxMDIoutInterface, item->text(2));
	}
}

/* COFDM */

void
TransmitterMainWindow::GetCOFDM()
{
	/* Output mode (real valued, I / Q or E / P) */
	switch (DRMTransmitter.GetParameters()->eOutputFormat)
	{
	case OF_REAL_VAL:
		RadioButtonOutReal->setChecked(true);
		break;

	case OF_IQ_POS:
		RadioButtonOutIQPos->setChecked(true);
		break;

	case OF_IQ_NEG:
		RadioButtonOutIQNeg->setChecked(true);
		break;

	case OF_EP:
		RadioButtonOutEP->setChecked(true);
		break;
	}
	LineEditSndCrdIF->setText(QString::number(DRMTransmitter.GetParameters()->rCarOffset));

	ListViewCOFDM->clear();
	vector<string> COFDMOutputs;
	DRMTransmitter.GetCOFDMOutputs(COFDMOutputs);
	for(size_t i=0; i<COFDMOutputs.size(); i++)
		(void) new Q3ListViewItem(ListViewCOFDM, COFDMOutputs[i].c_str());
}

void
TransmitterMainWindow::SetCOFDM()
{
	DRMTransmitter.GetParameters()->rCarOffset = LineEditSndCrdIF->text().toFloat();

	if(RadioButtonOutReal->isChecked())
		DRMTransmitter.GetParameters()->eOutputFormat=OF_REAL_VAL;
	if(RadioButtonOutIQPos->isChecked())
		DRMTransmitter.GetParameters()->eOutputFormat=OF_IQ_POS;
	if(RadioButtonOutIQNeg->isChecked())
		DRMTransmitter.GetParameters()->eOutputFormat=OF_IQ_NEG;
	if(RadioButtonOutEP->isChecked())
		DRMTransmitter.GetParameters()->eOutputFormat=OF_EP;

	vector<string> COFDMOutputs;
	Q3ListViewItemIterator item(ListViewCOFDM);
	for (; item.current(); item++)
	{
		COFDMOutputs.push_back(item.current()->text(0).latin1());
	}
	DRMTransmitter.SetCOFDMOutputs(COFDMOutputs);
}

void TransmitterMainWindow::OnComboBoxCOFDMDestActivated(int)
{
}

void TransmitterMainWindow::OnLineEditCOFDMOutputFileChanged(const QString&)
{
}

void TransmitterMainWindow::OnComboBoxCOFDMdestActivated(int)
{
}

void TransmitterMainWindow::OnTextChangedSndCrdIF(const QString&)
{
}

void
TransmitterMainWindow::OnButtonCOFDMAddAudio()
{
	(void) new Q3ListViewItem(ListViewCOFDM, ComboBoxCOFDMdest->currentText());
}

void TransmitterMainWindow::OnButtonCOFDMAddFile()
{
	QString file = LineEditCOFDMOutputFile->text();
	if(file != "")
		(void) new Q3ListViewItem(ListViewCOFDM, file);
}

void TransmitterMainWindow::OnButtonCOFDMDeleteSelected()
{
	Q3ListViewItem* p = ListViewCOFDM->selectedItem();
	if(p)
		delete p;
}

void TransmitterMainWindow::OnCOFDMOutListItemClicked(Q3ListViewItem* item)
{
	Q3ListViewItem* p = ListViewCOFDM->selectedItem();
	if(p)
	{
		QString s = p->text(0);
		bool found = false;
		for(int i=0; i<ComboBoxCOFDMdest->count(); i++)
		{
			if(ComboBoxCOFDMdest->text(i) == s)
			{
				ComboBoxCOFDMdest->setCurrentItem(i);
				found = true;
				break;
			}
		}
		if(found==false)
			LineEditCOFDMOutputFile->setText(s);
	}
}

void TransmitterMainWindow::OnButtonCOFDMBrowse()
{
	QString s( Q3FileDialog::getSaveFileName(
		"cofdm.wav", "Wave Files (*.wav)", this ) );
	if ( s.isEmpty() )
		return;
	LineEditCOFDMOutputFile->setText(s);
}

void
TransmitterMainWindow::OnRadioMode(int)
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
		TabWidgetConfigure->addTab(Channel, tr("Channel"));
		TabWidgetConfigure->addTab(Streams, tr("Streams"));
		TabWidgetConfigure->addTab(Audio, tr("Audio"));
		TabWidgetConfigure->addTab(Data, tr("Data"));
		TabWidgetConfigure->addTab(Services, tr("Services"));
		TabWidgetConfigure->addTab(COFDM, tr("COFDM"));
		TabWidgetConfigure->showPage(Channel);
	}
	if(RadioButtonEncoder->isChecked())
	{
		TabWidgetConfigure->addTab(Channel, tr("Channel"));
		TabWidgetConfigure->addTab(Streams, tr("Streams"));
		TabWidgetConfigure->addTab(Audio, tr("Audio"));
		TabWidgetConfigure->addTab(Data, tr("Data"));
		TabWidgetConfigure->addTab(Services, tr("Services"));
		TabWidgetConfigure->addTab(MDIOut, tr("MDI Output"));
		TabWidgetConfigure->showPage(Channel);
	}
	if(RadioButtonModulator->isChecked())
	{
		TabWidgetConfigure->addTab(MDIIn, tr("MDI Input"));
		TabWidgetConfigure->addTab(COFDM, tr("COFDM"));
		TabWidgetConfigure->showPage(COFDM);
	}
}

void
TransmitterMainWindow::OnLineEditPacketLenChanged(const QString& str)
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
TransmitterMainWindow::OnComboBoxPacketsPerFrameActivated(const QString& str)
{
	if(ComboBoxPacketsPerFrame->currentText()=="-")
		return;
	size_t packet_len = LineEditPacketLen->text().toInt();
	size_t packets = str.toInt();
	LineEditBitsPerFrame->setText(QString::number(8*packets*packet_len));
}

void
TransmitterMainWindow::OnComboBoxStreamTypeActivated(int item)
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

void TransmitterMainWindow::OnStreamsListItemClicked(Q3ListViewItem* item)
{
	LineEditBitsPerFrame->setText(item->text(4));
    QString type =  item->text(1);
    choseComboBoxItem(ComboBoxStreamType, item->text(1));
}

void
TransmitterMainWindow::OnButtonAddStream()
{
	int bits = LineEditBitsPerFrame->text().toInt();
	(void) new Q3ListViewItem(ListViewStreams,
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
TransmitterMainWindow::OnButtonDeleteStream()
{
	Q3ListViewItem* p = ListViewStreams->selectedItem();
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
        OnComboBoxStreamTypeActivated(0);
	}
}

void TransmitterMainWindow::OnButtonStartStop()
{
	if (bIsStarted == true)
	{
		/* Stop transmitter */
		DRMTransmitter.Stop();
		(void)DRMTransmitter.wait(5000);
		if(!DRMTransmitter.isFinished())
		{
			QMessageBox::critical(this, "Dream", tr("Exit"),
				tr("Termination of working thread failed"));
		}

		ButtonStartStop->setText(tr("&Start"));

		EnableAllControlsForSet();

		bIsStarted = false;
	}
	else
	{
		SetTransmitter();
		DRMTransmitter.start();
		ButtonStartStop->setText(tr("&Stop"));
		DisableAllControlsForSet();
		bIsStarted = true;
	}
}

void TransmitterMainWindow::EnableAudio(const bool bFlag)
{
	if (bFlag == true)
	{
		ComboBoxAudioSource->setEnabled(true);
	}
	else
	{
		/* Disable audio controls */
		GroupBoxTextMessage->setEnabled(false);
		ComboBoxAudioSource->setEnabled(false);
	}
}

void TransmitterMainWindow::EnableData(const bool bFlag)
{
	if (bFlag == true)
	{
		/* Enable data controls */
		ListViewFileNames->setEnabled(true);
		PushButtonClearAllFileNames->setEnabled(true);
		PushButtonAddFile->setEnabled(true);
	}
	else
	{
		/* Disable data controls */
		ListViewFileNames->setEnabled(false);
		PushButtonClearAllFileNames->setEnabled(false);
		PushButtonAddFile->setEnabled(false);
	}
}

void TransmitterMainWindow::OnPushButtonAddText()
{
	QString msg = LineEditTextMessage->text();
	if(msg != "")
		(void) new Q3ListViewItem(ListViewTextMessages, msg);
}

void TransmitterMainWindow::OnPushButtonDeleteText()
{
	Q3ListViewItem* p = ListViewTextMessages->selectedItem();
	if(p)
		delete p;
}

void TransmitterMainWindow::OnButtonClearAllText()
{
    ListViewTextMessages->clear();
}

void TransmitterMainWindow::AddSlide(const QString& path)
{
    QFileInfo FileInfo(path);

    /* Insert list view item. The object which is created here will be
       automatically destroyed by QT when the parent
       ("ListViewFileNames") is destroyed */
    ListViewFileNames->insertItem(
        new Q3ListViewItem(ListViewFileNames, FileInfo.fileName(),
        QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2),
        FileInfo.filePath()));
}

void TransmitterMainWindow::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
	QStringList list = Q3FileDialog::getOpenFileNames(
		tr("Image Files (*.png *.jpg *.jpeg *.jfif)"), NULL, this);

	/* Check if user not hit the cancel button */
	if (!list.isEmpty())
	{
		/* Insert all selected file names */
		for (QStringList::Iterator it = list.begin(); it != list.end(); it++)
		{
		    AddSlide(*it);
		}
	}
}

void TransmitterMainWindow::OnButtonClearAllFileNames()
{
	/* Clear list box for file names */
	ListViewFileNames->clear();
}

void TransmitterMainWindow::OnTextChangedServiceID(const QString& strID)
{
	(void)strID; // TODO
}

void TransmitterMainWindow::OnTextChangedServiceLabel(const QString& strLabel)
{
	(void)strLabel; // TODO
}

void TransmitterMainWindow::OnButtonAddService()
{
	Q3ListViewItem* v = new Q3ListViewItem(ListViewServices,
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

void TransmitterMainWindow::OnButtonDeleteService()
{
	Q3ListViewItem* p = ListViewServices->selectedItem();
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

void TransmitterMainWindow::OnServicesListItemClicked(Q3ListViewItem* item)
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

void TransmitterMainWindow::OnComboBoxMSCInterleaverActivated(int)
{
	SetChannel();
}

void TransmitterMainWindow::OnComboBoxSDCConstellationActivated(int)
{
	SetChannel();
}

void TransmitterMainWindow::OnComboBoxMSCConstellationActivated(int)
{
	/* Protection level must be re-adjusted when
	 * constellation mode was changed */
	UpdateMSCProtLevCombo();
	SetChannel();
}

void TransmitterMainWindow::OnComboBoxMSCProtLevActivated(int)
{
	SetChannel();
}

void TransmitterMainWindow::UpdateMSCProtLevCombo()
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

void TransmitterMainWindow::OnRadioBandwidth(int)
{
	SetChannel();
}

void TransmitterMainWindow::OnRadioRobustnessMode(int iID)
{
	/* Check, which bandwidth's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(true);
		RadioButtonBandwidth5->setEnabled(true);
		RadioButtonBandwidth9->setEnabled(true);
		RadioButtonBandwidth10->setEnabled(true);
		RadioButtonBandwidth18->setEnabled(true);
		RadioButtonBandwidth20->setEnabled(true);
		break;

	case 1:
		/* All bandwidth modes are possible */
		RadioButtonBandwidth45->setEnabled(true);
		RadioButtonBandwidth5->setEnabled(true);
		RadioButtonBandwidth9->setEnabled(true);
		RadioButtonBandwidth10->setEnabled(true);
		RadioButtonBandwidth18->setEnabled(true);
		RadioButtonBandwidth20->setEnabled(true);
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		RadioButtonBandwidth45->setEnabled(false);
		RadioButtonBandwidth5->setEnabled(false);
		RadioButtonBandwidth9->setEnabled(false);
		RadioButtonBandwidth10->setEnabled(true);
		RadioButtonBandwidth18->setEnabled(false);
		RadioButtonBandwidth20->setEnabled(true);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(true);
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		RadioButtonBandwidth45->setEnabled(false);
		RadioButtonBandwidth5->setEnabled(false);
		RadioButtonBandwidth9->setEnabled(false);
		RadioButtonBandwidth10->setEnabled(true);
		RadioButtonBandwidth18->setEnabled(false);
		RadioButtonBandwidth20->setEnabled(true);

		/* Set check on a default value to be sure we are "in range" */
		RadioButtonBandwidth10->setChecked(true);
		break;
	}
	SetChannel();
}

void TransmitterMainWindow::DisableAllControlsForSet()
{
	Channel->setEnabled(false);
	Streams->setEnabled(false);
	Audio->setEnabled(false);
	Data->setEnabled(false);
	Services->setEnabled(false);
	MDIOut->setEnabled(false);
	MDIIn->setEnabled(false);
	COFDM->setEnabled(false);

	GroupInput->setEnabled(true); /* For run-mode */
}

void TransmitterMainWindow::EnableAllControlsForSet()
{
	Channel->setEnabled(true);
	Streams->setEnabled(true);
	Audio->setEnabled(true);
	Data->setEnabled(true);
	Services->setEnabled(true);
	MDIOut->setEnabled(true);
	MDIIn->setEnabled(true);
	COFDM->setEnabled(true);

	GroupInput->setEnabled(false); /* For run-mode */

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
	ProgressBarCurPict->setProgress(0);
	TextLabelCurPict->setText("");
}

void TransmitterMainWindow::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

void TransmitterMainWindow::AddWhatsThisHelp()
{
	/* Dream Logo */
	logo->setWhatsThis(
		tr("<b>Dream Logo:</b> This is the official logo of "
		"the Dream software."));

	/* Input Level */
	ProgrInputLevel->setWhatsThis(
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

	RadioButtonRMA->setWhatsThis( strRobustnessMode);
	RadioButtonRMB->setWhatsThis( strRobustnessMode);
	RadioButtonRMC->setWhatsThis( strRobustnessMode);
	RadioButtonRMD->setWhatsThis( strRobustnessMode);

	/* Bandwidth */
	const QString strBandwidth =
		tr("<b>DRM Bandwidth:</b> The bandwith is the gross "
		"bandwidth of the generated DRM signal. Not all DRM robustness mode / "
		"bandwidth constellations are possible, e.g., DRM robustness mode D "
		"and C are only defined for the bandwidths 10 kHz and 20 kHz.");

	RadioButtonBandwidth45->setWhatsThis( strBandwidth);
	RadioButtonBandwidth5->setWhatsThis( strBandwidth);
	RadioButtonBandwidth9->setWhatsThis( strBandwidth);
	RadioButtonBandwidth10->setWhatsThis( strBandwidth);
	RadioButtonBandwidth18->setWhatsThis( strBandwidth);
	RadioButtonBandwidth20->setWhatsThis( strBandwidth);

	/* Output intermediate frequency of DRM signal */
	const QString strOutputIF =
		tr("<b>Output intermediate frequency of DRM signal:</b> "
		"Set the output intermediate frequency (IF) of generated DRM signal "
		"in the 'sound-card pass-band'. In some DRM modes, the IF is located "
		"at the edge of the DRM signal, in other modes it is centered. The IF "
		"should be chosen that the DRM signal lies entirely inside the "
		"sound-card bandwidth.");

	TextLabelIF->setWhatsThis( strOutputIF);
	LineEditSndCrdIF->setWhatsThis( strOutputIF);
	TextLabelIFUnit->setWhatsThis( strOutputIF);

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

	RadioButtonOutReal->setWhatsThis( strOutputFormat);
	RadioButtonOutIQPos->setWhatsThis( strOutputFormat);
	RadioButtonOutIQNeg->setWhatsThis( strOutputFormat);
	RadioButtonOutEP->setWhatsThis( strOutputFormat);

	/* MSC interleaver mode */
	const QString strInterleaver =
		tr("<b>MSC interleaver mode:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the longer "
		"the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	TextLabelInterleaver->setWhatsThis( strInterleaver);
	ComboBoxMSCInterleaver->setWhatsThis( strInterleaver);
}

void
TransmitterMainWindow::choseComboBoxItem(QComboBox* box, const QString& text)
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
