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
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QCloseEvent>
#include "DialogUtil.h"
#include "../DrmTransmitterInterface.h"
#include "../Parameter.h"
#include <sstream>
#include <fstream>

TransmitterMainWindow::TransmitterMainWindow(CDRMTransmitterInterface& tx, CSettings& NSettings,
	QWidget* parent, const char* name, Qt::WFlags f
	):
	QMainWindow(parent, name, f),
	Ui_TransmitterMainWindow(),
	DRMTransmitter(tx), Settings(NSettings),
    Timer(), bIsStarted(false),
    channelEditor(*this),
    streamEditor(*this),
    audioComponentEditor(*this),
    dataComponentEditor(*this),
    servicesEditor(*this),
    cofdmEditor(*this),
    mdiInputEditor(*this),
    mdiOutputEditor(*this)
{
    setupUi(this);
    channelEditor.setupUi();
    streamEditor.setupUi();
    audioComponentEditor.setupUi();
    dataComponentEditor.setupUi();
    servicesEditor.setupUi();
    cofdmEditor.setupUi();
    mdiInputEditor.setupUi();
    mdiOutputEditor.setupUi();

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Transmit Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

    setCaption(tr("Dream DRM Transmitter"));

	/* Init controls with default settings */
	ButtonStartStop->setText(tr("&Start"));

	/* Init progress bar for input signal level */
	ProgrInputLevel->setRange(-50.0, 0.0);
	ProgrInputLevel->setOrientation(Qt::Horizontal, QwtThermo::BottomScale);
	ProgrInputLevel->setFillColor(QColor(0, 190, 0));
	ProgrInputLevel->setAlarmLevel(-5.0);
	ProgrInputLevel->setAlarmColor(QColor(255, 0, 0));

	/* Init progress bar for current transmitted picture */
	ProgressBarCurPict->setRange(0, 100);
	ProgressBarCurPict->setValue(0);
	TextLabelCurPict->setText("");

	/* Enable all controls */
	EnableAllControlsForSet();

	/* Connections ---------------------------------------------------------- */

    /* File Menu */
	connect(action_Exit, SIGNAL(triggered()), this, SLOT(OnButtonClose()));

    /* Help Menu */
    CAboutDlg* pAboutDlg = new CAboutDlg(this);
	connect(actionWhats_This, SIGNAL(triggered()), this, SLOT(OnHelpWhatsThis()));
	connect(actionAbout, SIGNAL(triggered()), pAboutDlg, SLOT(show()));

	/* General */
	connect(ButtonStartStop, SIGNAL(clicked()), this, SLOT(OnButtonStartStop()));
	connect(buttonClose, SIGNAL(clicked()), this, SLOT(OnButtonClose()));

    QButtonGroup* modeGroup = new QButtonGroup(this);
    modeGroup->addButton(RadioButtonTransmitter, CDRMTransmitterInterface::T_TX);
    modeGroup->addButton(RadioButtonEncoder, CDRMTransmitterInterface::T_ENC);
    modeGroup->addButton(RadioButtonModulator, CDRMTransmitterInterface::T_MOD);

	connect(modeGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioMode(int)));

	connect(&channelEditor, SIGNAL(MSCCapacityChanged()), this, SLOT(OnMSCCapacityChanged()));
	connect(this, SIGNAL(MSCCapacityChanged(int)), &channelEditor, SLOT(OnMSCCapacityChanged(int)));
	connect(this, SIGNAL(MSCCapacityChanged(int)), &streamEditor, SLOT(OnMSCCapacityChanged(int)));

	connect(&channelEditor, SIGNAL(SDCCapacityChanged()), this, SLOT(OnSDCCapacityChanged()));
	connect(this, SIGNAL(SDCCapacityChanged(int)), &channelEditor, SLOT(OnSDCCapacityChanged(int)));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	GetFromTransmitter();
	OnRadioMode(0);

	Timer.stop();
}

TransmitterMainWindow::~TransmitterMainWindow()
{
}

void
TransmitterMainWindow::closeEvent(QCloseEvent* ce)
{
	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("Transmit Dialog", s);

	/* Stop transmitter if needed */
	if (bIsStarted == true)
		OnButtonStartStop();
	else
		SetTransmitter(); // so Transmitter save settings has the latest info
	ce->accept();
}

void TransmitterMainWindow::OnMSCCapacityChanged()
{
    channelEditor.PutTo(DRMTransmitter);
    DRMTransmitter.CalculateChannelCapacities();
    emit MSCCapacityChanged(DRMTransmitter.GetParameters()->iNumDecodedBitsMSC);
}

void TransmitterMainWindow::OnSDCCapacityChanged()
{
    channelEditor.PutTo(DRMTransmitter);
    DRMTransmitter.CalculateChannelCapacities();
    emit SDCCapacityChanged(DRMTransmitter.GetParameters()->iNumSDCBitsPerSuperFrame);
}

void TransmitterMainWindow::OnButtonClose()
{
    this->close(false);
}

void TransmitterMainWindow::OnTimer()
{
    ProgrInputLevel->setValue(DRMTransmitter.GetLevelMeter());
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
        ProgressBarCurPict->setValue(int(rCPercent * 100)); /* % */
    }
    else
    {
        /* Disable controls */
        ProgressBarCurPict->setEnabled(false);
        TextLabelCurPict->setEnabled(false);
    }
}

void
TransmitterMainWindow::GetFromTransmitter()
{
	switch(DRMTransmitter.GetOperatingMode())
	{
	case CDRMTransmitterInterface::T_ENC:
		channelEditor.GetFrom(DRMTransmitter);
		streamEditor.GetFrom(DRMTransmitter);
        audioComponentEditor.GetFrom(DRMTransmitter);
		dataComponentEditor.GetFrom(DRMTransmitter);
		servicesEditor.GetFrom(DRMTransmitter);
		mdiOutputEditor.GetFrom(DRMTransmitter);
		RadioButtonEncoder->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_MOD:
		mdiInputEditor.GetFrom(DRMTransmitter);
		cofdmEditor.GetFrom(DRMTransmitter);
		RadioButtonModulator->setChecked(true);
		break;
	case CDRMTransmitterInterface::T_TX:
		channelEditor.GetFrom(DRMTransmitter);
		streamEditor.GetFrom(DRMTransmitter);
        audioComponentEditor.GetFrom(DRMTransmitter);
		dataComponentEditor.GetFrom(DRMTransmitter);
		servicesEditor.GetFrom(DRMTransmitter);
		cofdmEditor.GetFrom(DRMTransmitter);
		RadioButtonTransmitter->setChecked(true);
		break;
	}
}

void
TransmitterMainWindow::SetTransmitter()
{
	CDRMTransmitterInterface::ETxOpMode eMod = CDRMTransmitterInterface::T_TX;

	if(RadioButtonTransmitter->isChecked())
        eMod = CDRMTransmitterInterface::T_TX;

	if(RadioButtonEncoder->isChecked())
        eMod = CDRMTransmitterInterface::T_ENC;

	if(RadioButtonModulator->isChecked())
        eMod = CDRMTransmitterInterface::T_MOD;

	DRMTransmitter.SaveSettings(Settings);
	DRMTransmitter.SetOperatingMode(eMod);
	DRMTransmitter.LoadSettings(Settings);

    switch(eMod)
	{
    case CDRMTransmitterInterface::T_ENC:
		channelEditor.PutTo(DRMTransmitter);
		streamEditor.PutTo(DRMTransmitter);
		audioComponentEditor.PutTo(DRMTransmitter);
		dataComponentEditor.PutTo(DRMTransmitter);
		servicesEditor.PutTo(DRMTransmitter);
		mdiOutputEditor.PutTo(DRMTransmitter);
		Indicators->show();
		break;
    case CDRMTransmitterInterface::T_TX:
		channelEditor.PutTo(DRMTransmitter);
		streamEditor.PutTo(DRMTransmitter);
		audioComponentEditor.PutTo(DRMTransmitter);
		dataComponentEditor.PutTo(DRMTransmitter);
		servicesEditor.PutTo(DRMTransmitter);
		Indicators->show();
		cofdmEditor.PutTo(DRMTransmitter);
		break;
    case CDRMTransmitterInterface::T_MOD:
		mdiInputEditor.PutTo(DRMTransmitter);
		cofdmEditor.PutTo(DRMTransmitter);
		Indicators->hide();
	}
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

void TransmitterMainWindow::OnButtonStartStop()
{
	if (bIsStarted == true)
	{
	    Timer.stop();
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

        if(RadioButtonModulator->isChecked()==false)
            Timer.start(GUI_CONTROL_UPDATE_TIME);
	}
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

	Indicators->setEnabled(true); /* For run-mode */
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

	Indicators->setEnabled(false); /* For run-mode */

	/* Reset status bars */
	ProgrInputLevel->setValue(RET_VAL_LOG_0);
	ProgressBarCurPict->setValue(0);
	TextLabelCurPict->setText("");
}

void TransmitterMainWindow::OnHelpWhatsThis()
{
	QWhatsThis::enterWhatsThisMode();
}

ChannelEditor::ChannelEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void ChannelEditor::setupUi()
{
	/* MSC interleaver mode */
	ui.ComboBoxMSCInterleaver->insertItem(0, tr("2 s (Long Interleaving)"), QVariant(SI_LONG));
	ui.ComboBoxMSCInterleaver->insertItem(0, tr("400 ms (Short Interleaving)"), QVariant(SI_SHORT));

	/* MSC Constellation Scheme */
	ui.ComboBoxMSCConstellation->insertItem(0, tr("SM 16-QAM"), QVariant(CS_2_SM));
	ui.ComboBoxMSCConstellation->insertItem(1, tr("SM 64-QAM"), QVariant(CS_3_SM));

// These modes should not be used right now, TODO
//	ui.ComboBoxMSCConstellation->insertItem(2, tr("HMsym 64-QAM"), QVariant(CS_3_HMSYM));
//	ui.ComboBoxMSCConstellation->insertItem(3, tr("HMmix 64-QAM"), QVariant(CS_3_HMMIX));

	/* SDC Constellation Scheme */
	ui.ComboBoxSDCConstellation->insertItem(0, tr("4-QAM"), QVariant(CS_1_SM));
	ui.ComboBoxSDCConstellation->insertItem(1, tr("16-QAM"), QVariant(CS_2_SM));

	/* PutTo button group IDs */

    soGroup = new QButtonGroup(this);
    soGroup->addButton(ui.RadioButtonBandwidth45, SO_0);
    soGroup->addButton(ui.RadioButtonBandwidth5, SO_1);
    soGroup->addButton(ui.RadioButtonBandwidth9, SO_2);
    soGroup->addButton(ui.RadioButtonBandwidth10, SO_3);
    soGroup->addButton(ui.RadioButtonBandwidth18, SO_4);
    soGroup->addButton(ui.RadioButtonBandwidth20, SO_5);
	connect(soGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioBandwidth(int)));

    robustnessGroup = new QButtonGroup(this);
    robustnessGroup->addButton(ui.RadioButtonRMA, RM_ROBUSTNESS_MODE_A);
    robustnessGroup->addButton(ui.RadioButtonRMB, RM_ROBUSTNESS_MODE_B);
    robustnessGroup->addButton(ui.RadioButtonRMC, RM_ROBUSTNESS_MODE_C);
    robustnessGroup->addButton(ui.RadioButtonRMD, RM_ROBUSTNESS_MODE_D);
	connect(robustnessGroup, SIGNAL(buttonClicked(int)), this, SLOT(OnRadioRobustnessMode(int)));

	connect(ui.ComboBoxMSCInterleaver, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCInterleaverActivated(int)));
	connect(ui.ComboBoxMSCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCConstellationActivated(int)));
	connect(ui.ComboBoxMSCProtLev, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxMSCProtLevActivated(int)));
	connect(ui.ComboBoxSDCConstellation, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSDCConstellationActivated(int)));
}

void
ChannelEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
    const CChannel& channel = DRMTransmitter.GetParameters()->Channel;

    robustnessGroup->button(channel.eRobustness)->setChecked(true);
	soGroup->button(channel.eSpectrumOccupancy)->setChecked(true);

	switch (channel.eInterleaverDepth)
	{
	case SI_LONG:
		ui.ComboBoxMSCInterleaver->setCurrentItem(0);
		break;

	case SI_SHORT:
		ui.ComboBoxMSCInterleaver->setCurrentItem(1);
		break;
	}

	switch (channel.eMSCmode)
	{
	case CS_1_SM:
		break;

	case CS_2_SM:
		ui.ComboBoxMSCConstellation->setCurrentItem(0);
		break;

	case CS_3_SM:
		ui.ComboBoxMSCConstellation->setCurrentItem(1);
		break;

	case CS_3_HMSYM:
//		ui.ComboBoxMSCConstellation->setCurrentItem(2);
		break;

	case CS_3_HMMIX:
//		ui.ComboBoxMSCConstellation->setCurrentItem(3);
		break;
	}

	/* MSC Protection Level */
	UpdateMSCProtLevCombo(); /* options depend on MSC Constellation */
	ui.ComboBoxMSCProtLev->setCurrentItem(DRMTransmitter.GetParameters()->MSCParameters.ProtectionLevel.iPartB);

	switch (channel.eSDCmode)
	{
	case CS_1_SM:
		ui.ComboBoxSDCConstellation->setCurrentItem(0);
		break;

	case CS_2_SM:
		ui.ComboBoxSDCConstellation->setCurrentItem(1);
		break;
	default:
		;
	}
	emit MSCCapacityChanged();
	emit SDCCapacityChanged();
}

void
ChannelEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
    CParameter& Parameters = *DRMTransmitter.GetParameters();

	/* Spectrum Occupancy */
    Parameters.Channel.eSpectrumOccupancy = ESpecOcc(soGroup->checkedId());

	/* MSC Protection Level */
	CMSCProtLev MSCPrLe;
	MSCPrLe.iPartB = ui.ComboBoxMSCProtLev->currentItem();
	Parameters.MSCParameters.ProtectionLevel = MSCPrLe;

	Parameters.Channel.eMSCmode
        = ECodScheme(ui.ComboBoxMSCConstellation->itemData(
            ui.ComboBoxMSCConstellation->currentItem()
            ).toInt());

    Parameters.Channel.eInterleaverDepth
        = ESymIntMod(ui.ComboBoxMSCInterleaver->itemData(
            ui.ComboBoxMSCInterleaver->currentItem()
    ).toInt());

    Parameters.Channel.eSDCmode
        = ECodScheme(ui.ComboBoxSDCConstellation->itemData(
            ui.ComboBoxSDCConstellation->currentItem()
    ).toInt());

	/* Robustness Mode */
    Parameters.Channel.eRobustness = ERobMode(robustnessGroup->checkedId());

    Parameters.CellMappingTable.MakeTable(
        Parameters.Channel.eRobustness,
        Parameters.Channel.eSpectrumOccupancy
    );
}

void ChannelEditor::OnComboBoxMSCInterleaverActivated(int)
{
}

void ChannelEditor::OnComboBoxSDCConstellationActivated(int)
{
	emit SDCCapacityChanged();
}

void ChannelEditor::OnComboBoxMSCConstellationActivated(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::OnComboBoxMSCProtLevActivated(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::UpdateMSCProtLevCombo()
{
	if(ui.ComboBoxMSCConstellation->currentItem()==0)
	{
		/* Only two protection levels possible in 16 QAM mode */
		ui.ComboBoxMSCProtLev->clear();
		ui.ComboBoxMSCProtLev->insertItem("0", 0);
		ui.ComboBoxMSCProtLev->insertItem("1", 1);
	}
	else
	{
		/* Four protection levels defined */
		ui.ComboBoxMSCProtLev->clear();
		ui.ComboBoxMSCProtLev->insertItem("0", 0);
		ui.ComboBoxMSCProtLev->insertItem("1", 1);
		ui.ComboBoxMSCProtLev->insertItem("2", 2);
		ui.ComboBoxMSCProtLev->insertItem("3", 3);
	}

	/* PutTo protection level to 1 */
	ui.ComboBoxMSCProtLev->setCurrentItem(1);
}

void ChannelEditor::OnRadioBandwidth(int)
{
	emit MSCCapacityChanged();
}

void ChannelEditor::OnRadioRobustnessMode(int iID)
{
	/* Check, which bandwidth's are possible with this robustness mode */
	switch (iID)
	{
	case 0:
		/* All bandwidth modes are possible */
		ui.RadioButtonBandwidth45->setEnabled(true);
		ui.RadioButtonBandwidth5->setEnabled(true);
		ui.RadioButtonBandwidth9->setEnabled(true);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(true);
		ui.RadioButtonBandwidth20->setEnabled(true);
		break;

	case 1:
		/* All bandwidth modes are possible */
		ui.RadioButtonBandwidth45->setEnabled(true);
		ui.RadioButtonBandwidth5->setEnabled(true);
		ui.RadioButtonBandwidth9->setEnabled(true);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(true);
		ui.RadioButtonBandwidth20->setEnabled(true);
		break;

	case 2:
		/* Only 10 and 20 kHz possible in robustness mode C */
		ui.RadioButtonBandwidth45->setEnabled(false);
		ui.RadioButtonBandwidth5->setEnabled(false);
		ui.RadioButtonBandwidth9->setEnabled(false);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(false);
		ui.RadioButtonBandwidth20->setEnabled(true);

		/* PutTo check on a default value to be sure we are "in range" */
		ui.RadioButtonBandwidth10->setChecked(true);
		break;

	case 3:
		/* Only 10 and 20 kHz possible in robustness mode D */
		ui.RadioButtonBandwidth45->setEnabled(false);
		ui.RadioButtonBandwidth5->setEnabled(false);
		ui.RadioButtonBandwidth9->setEnabled(false);
		ui.RadioButtonBandwidth10->setEnabled(true);
		ui.RadioButtonBandwidth18->setEnabled(false);
		ui.RadioButtonBandwidth20->setEnabled(true);

		/* PutTo check on a default value to be sure we are "in range" */
		ui.RadioButtonBandwidth10->setChecked(true);
		break;
	}
	emit MSCCapacityChanged();
	emit SDCCapacityChanged();
}

void
ChannelEditor::OnMSCCapacityChanged(int iBitsMSC)
{
	ui.TextLabelMSCCapBits->setText(QString::number(iBitsMSC));
	ui.TextLabelMSCCapBytes->setText(QString::number(iBitsMSC/8));
}

void
ChannelEditor::OnSDCCapacityChanged(int iBitsSDC)
{
	ui.TextLabelSDCCapBits->setText(QString::number(iBitsSDC));
	ui.TextLabelSDCCapBytes->setText(QString::number(iBitsSDC/8));
}

StreamEditor::StreamEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void StreamEditor::setupUi()
{
	connect(ui.ButtonAddStream, SIGNAL(clicked()),
		this, SLOT(OnButtonAddStream()));
	connect(ui.ButtonDeleteStream, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteStream()));
	connect(ui.ComboBoxStreamType, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxStreamTypeActivated(int)));
	connect(ui.ComboBoxPacketsPerFrame, SIGNAL(activated(const QString&)),
		this, SLOT(OnComboBoxPacketsPerFrameActivated(const QString&)));
	connect(ui.LineEditPacketLen, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPacketLenChanged(const QString&)));
	connect(ui.tableWidgetStreams, SIGNAL(itemSelectionChanged()),
		this, SLOT(OnItemSelectionChanged()));

    ui.ButtonDeleteStream->setEnabled(false);
}

void
StreamEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
    CParameter& Parameters = *DRMTransmitter.GetParameters();

    size_t n = Parameters.MSCParameters.Stream.size();

    ui.tableWidgetStreams->clearContents();

    if(n==0) // default to 1 audio stream filling MSC
    {
        ui.ComboBoxStreamType->setCurrentItem(0);
        ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesTotal->text());
        ui.LineEditPacketLen->setText("");
		OnButtonAddStream();
        return;
    }

	for(size_t i=0; i<n; i++)
	{
		const CStream& stream = Parameters.MSCParameters.Stream[i];
        int bytes = stream.iLenPartB; // EEP Only
        if(stream.iLenPartA != 0)
        {
            QMessageBox::information(NULL, "Dream", tr("UEP stream read from transmitter - ignored"),
            QMessageBox::Ok);
        }
		ui.ComboBoxStream->setCurrentItem(i);
		if(stream.eAudDataFlag == SF_AUDIO)
		{
			ui.ComboBoxStreamType->setCurrentItem(0);
			ui.LineEditPacketLen->setText("");
		}
		else
		{
			if(stream.ePacketModInd == PM_PACKET_MODE)
			{
				ui.ComboBoxStreamType->setCurrentItem(1);
				ui.LineEditPacketLen->setText(QString::number(stream.iPacketLen));
			}
			else
			{
				ui.ComboBoxStreamType->setCurrentItem(2);
				ui.LineEditPacketLen->setText("");
				ui.ComboBoxPacketsPerFrame->clear();
			}
		}
		ui.LineEditBytesPerFrame->setText(QString::number(bytes));
		OnButtonAddStream();
	}
}

void
StreamEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
    CMSCParameters& MSCParameters = DRMTransmitter.GetParameters()->MSCParameters;
    int n = ui.tableWidgetStreams->rowCount();
	MSCParameters.Stream.resize(n);
	for (int i=0; i<n; i++)
	{
	    int iStreamID = ui.tableWidgetStreams->item(i, 0)->text().toInt();
	    QString type = ui.tableWidgetStreams->item(i, 1)->text();
	    int bytes = ui.tableWidgetStreams->item(i, 2)->text().toInt();
        CStream& stream = MSCParameters.Stream[iStreamID];
        switch(ui.ComboBoxStreamType->findText(type))
        {
            case 0:
                stream.eAudDataFlag = SF_AUDIO;
                break;
            case 1:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_PACKET_MODE;
                stream.iPacketLen = ui.tableWidgetStreams->item(i, 3)->text().toInt();
                break;
            case 2:
                stream.eAudDataFlag = SF_DATA;
                stream.ePacketModInd = PM_SYNCHRON_STR_MODE;
        }
        stream.iLenPartA = 0; // EEP
        stream.iLenPartB = bytes;
	}
}

void
StreamEditor::OnLineEditPacketLenChanged(const QString& str)
{
	ui.ComboBoxPacketsPerFrame->clear();
	if(str=="")
		return;
	size_t packet_len = str.toInt();
	if(packet_len>=3) // minumum size of a packet
	{
        size_t bytes = ui.TextLabelMSCBytesAvailable->text().toInt();
        size_t max_packets = bytes/(packet_len);
        for(size_t i=1; i<=max_packets; i++)
            ui.ComboBoxPacketsPerFrame->insertItem(QString::number(i));
        ui.ComboBoxPacketsPerFrame->setCurrentItem(0);
	}
}

void
StreamEditor::OnComboBoxPacketsPerFrameActivated(const QString& str)
{
	if(ui.ComboBoxPacketsPerFrame->currentText()=="-")
		return;
	size_t packet_len = ui.LineEditPacketLen->text().toInt();
	size_t packets = str.toInt();
	ui.LineEditBytesPerFrame->setText(QString::number(packets*packet_len));
}

void
StreamEditor::OnComboBoxStreamTypeActivated(int item)
{
	size_t bytes = ui.TextLabelMSCBytesAvailable->text().toInt();
	switch(item)
	{
	case 0: // audio
	case 2: // data_stream
		ui.LineEditPacketLen->setText("");
		ui.LineEditPacketLen->setEnabled(false);
		ui.ComboBoxPacketsPerFrame->clear();
		ui.ComboBoxPacketsPerFrame->setEnabled(false);
		ui.LineEditBytesPerFrame->setEnabled(true);
		break;
	case 1: // data_packet
		ui.LineEditPacketLen->setText(QString::number(int(bytes)));
		ui.LineEditPacketLen->setEnabled(true);
		ui.ComboBoxPacketsPerFrame->setEnabled(true);
		ui.LineEditBytesPerFrame->setEnabled(false);
		break;
	}
    ui.LineEditBytesPerFrame->setText(QString::number(bytes));
}

void
StreamEditor::OnItemSelectionChanged()
{
    int row = ui.tableWidgetStreams->currentRow();
    if(row == -1) // nothing selected
        return;
    QString type =  ui.tableWidgetStreams->item(row, 1)->text();
	ui.LineEditBytesPerFrame->setText(ui.tableWidgetStreams->item(row, 2)->text());
    int iType = ui.ComboBoxStreamType->findText(type);
    ui.ComboBoxStreamType->setCurrentIndex(iType);
    if(iType == 1) // data packet
    {
        ui.LineEditPacketLen->setText(ui.tableWidgetStreams->item(row, 3)->text());
        int f = ui.ComboBoxPacketsPerFrame->findText(ui.tableWidgetStreams->item(row, 4)->text());
        ui.ComboBoxPacketsPerFrame->setCurrentIndex(f);
    }
    ui.tableWidgetStreams->setCurrentCell(row, 0, QItemSelectionModel::Rows|QItemSelectionModel::ClearAndSelect);
}

void
StreamEditor::OnButtonAddStream()
{
	int bytes=0;
	QString plen, packets;
	switch(ui.ComboBoxStreamType->currentItem())
	{
	case 0: // audio
	case 2: // data_stream
        bytes = ui.LineEditBytesPerFrame->text().toInt();
		break;
	case 1: // data_packet
        plen = ui.LineEditPacketLen->text();
        packets = ui.ComboBoxPacketsPerFrame->currentText();
        bytes = plen.toInt()*packets.toInt();
	}

    int row = ui.tableWidgetStreams->rowCount();
    cerr << row << endl;
    ui.tableWidgetStreams->setRowCount(row+1);
    ui.tableWidgetStreams->setItem(row, 0, new QTableWidgetItem(ui.ComboBoxStream->currentText()));
    ui.tableWidgetStreams->setItem(row, 1, new QTableWidgetItem(ui.ComboBoxStreamType->currentText()));
    ui.tableWidgetStreams->setItem(row, 2, new QTableWidgetItem(QString::number(bytes)));
    ui.tableWidgetStreams->setItem(row, 3, new QTableWidgetItem(plen));
    ui.tableWidgetStreams->setItem(row, 4, new QTableWidgetItem(packets));

	ui.ComboBoxStream->removeItem(ui.ComboBoxStream->currentItem());
	ui.ComboBoxStream->setCurrentItem(0);
	ui.ComboBoxStreamType->setCurrentItem(0);
	int availbytes = ui.TextLabelMSCBytesAvailable->text().toInt();
	ui.TextLabelMSCBytesAvailable->setText(QString::number(availbytes-bytes));
    ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesAvailable->text());
    ui.ButtonDeleteStream->setEnabled(true);
    ui.tableWidgetStreams->setCurrentCell(row, 0, QItemSelectionModel::Rows|QItemSelectionModel::ClearAndSelect);
}

void
StreamEditor::OnButtonDeleteStream()
{
    //if(ui.tableWidgetStreams->rowCount()<2) // one row of header
    //    return;

    int row = ui.tableWidgetStreams->currentRow();

    QString stream = ui.tableWidgetStreams->item(row, 0)->text();
    int bytes = ui.tableWidgetStreams->item(row, 2)->text().toInt();
    ui.tableWidgetStreams->removeRow(row);
    ui.ComboBoxStream->insertItem(stream);
    int availbytes = ui.TextLabelMSCBytesAvailable->text().toInt();
    ui.TextLabelMSCBytesAvailable->setText(QString::number(availbytes+bytes));
    ui.LineEditBytesPerFrame->setText(ui.TextLabelMSCBytesAvailable->text());
    if(ui.tableWidgetStreams->rowCount()<1)
        ui.ButtonDeleteStream->setEnabled(false);
    else // select a row near the deleted row
    {
        if(row>0)
            row--;
        else
            row++;
        ui.tableWidgetStreams->setCurrentCell(row, 0, QItemSelectionModel::Rows|QItemSelectionModel::ClearAndSelect);
    }
}

void
StreamEditor::OnMSCCapacityChanged(int iBitsMSC)
{
	ui.TextLabelMSCBytesTotal->setText(QString::number(iBitsMSC/8));
	int used = 0;
	int n = ui.tableWidgetStreams->rowCount();
	for (int i=0; i<n; i++)
	{
	    used +=  ui.tableWidgetStreams->item(i, 2)->text().toInt();
	}
	ui.TextLabelMSCBytesAvailable->setText(QString::number(iBitsMSC/8 - used));
	ui.ComboBoxStreamType->setCurrentIndex(0);
}

AudioComponentEditor::AudioComponentEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void AudioComponentEditor::setupUi()
{
    ui.ComboBoxAudioStreamNo->clear();
	ui.ComboBoxAudioStreamNo->insertItem("-");
	ui.ComboBoxAudioStreamNo->insertItem("0");
	ui.ComboBoxAudioStreamNo->insertItem("1");
	ui.ComboBoxAudioStreamNo->insertItem("2");
	ui.ComboBoxAudioStreamNo->insertItem("3");

	ui.ListViewTextMessages->clear();
	ui.ListViewTextMessages->setAllColumnsShowFocus(true);
	/* NOT implemented yet */
    ui.ComboBoxCodec->setEnabled(false);
    ui.ComboBoxAudioMode->setEnabled(false);
    ui.ComboBoxAudioBitrate->setEnabled(false);
    ui.CheckBoxSBR->setEnabled(false);

	connect(ui.ComboBoxAudioSource, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxSourceActivated(int)));
	connect(ui.PushButtonAudioSourceFileBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonFileBrowse()));
	connect(ui.CheckBoxAudioSourceIsFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxSourceIsFile(bool)));
	connect(ui.LineEditAudioSourceFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));

	connect(ui.PushButtonAddText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddText()));
	connect(ui.PushButtonDeleteText, SIGNAL(clicked()),
		this, SLOT(OnPushButtonDeleteText()));
	connect(ui.PushButtonClearAllText, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllText()));
	connect(ui.CheckBoxEnableTextMessage, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxEnableTextMessage(bool)));
}

void
AudioComponentEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
	vector<string> vecAudioDevices;
	DRMTransmitter.GetSoundInChoices(vecAudioDevices);
	ui.ComboBoxAudioSource->clear();
	for (size_t t = 0; t < vecAudioDevices.size(); t++)
	{
		ui.ComboBoxAudioSource->insertItem(QString(vecAudioDevices[t].c_str()), t);
	}
	ui.ComboBoxAudioSource->setCurrentItem(0);
    string fn = DRMTransmitter.GetReadFromFile();
    ui.LineEditAudioSourceFile->setText(fn.c_str());
    if(fn == "")
    {
        int iAudSrc = DRMTransmitter.GetSoundInInterface();
        if((iAudSrc>=0) && (iAudSrc<ui.ComboBoxAudioSource->count()))
        {
            ui.ComboBoxAudioSource->setCurrentItem(iAudSrc);
        }
        else
        {
            int n = ui.ComboBoxAudioSource->count();
            if(n>0)
                ui.ComboBoxAudioSource->setCurrentItem(n-1);
        }
    }

    CParameter& Parameters = *DRMTransmitter.GetParameters();
    for(map<int,CAudioParam>::const_iterator i=Parameters.AudioParam.begin();
                                            i!=Parameters.AudioParam.end(); i++)
    {
        ui.ComboBoxAudioStreamNo->setCurrentIndex(ui.ComboBoxAudioStreamNo->findText(QString::number(i->first)));
        if(i->second.bTextflag == true)
        {
            /* Activate text message */
            EnableTextMessage(true);
            vector<string> msg;
            DRMTransmitter.GetTextMessages(msg);
            for(size_t j=0; j<msg.size(); j++)
                ui.ListViewTextMessages->insertItem(new Q3ListViewItem(ui.ListViewTextMessages, msg[j].c_str()));
        }
        else
        {
            EnableTextMessage(false);
        }
    }
}

void
AudioComponentEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
    int iStreamNo = ui.ComboBoxAudioStreamNo->currentText().toInt();
	CAudioParam& AudioParam = DRMTransmitter.GetParameters()->AudioParam[iStreamNo];

	if(ui.CheckBoxAudioSourceIsFile->isChecked())
	{
		DRMTransmitter.SetReadFromFile(ui.LineEditAudioSourceFile->text().latin1());
	}
	else
	{
		int iAudSrc = ui.ComboBoxAudioSource->currentItem();
		DRMTransmitter.SetSoundInInterface(iAudSrc);
	}

	AudioParam.bTextflag = ui.CheckBoxEnableTextMessage->isChecked();

	if(AudioParam.bTextflag)
	{
		DRMTransmitter.ClearTextMessages();
        Q3ListViewItemIterator it(ui.ListViewTextMessages);
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

void AudioComponentEditor::OnComboBoxSourceActivated(int)
{
}

void AudioComponentEditor::OnButtonFileBrowse()
{
	QString s( QFileDialog::getOpenFileName(
		QString::null, "Wave Files (*.wav)", NULL ) );
	if ( s.isEmpty() )
		return;
    ui.LineEditAudioSourceFile->setText(s);
}

void AudioComponentEditor::OnLineEditFileChanged(const QString&)
{
}

void AudioComponentEditor::OnToggleCheckBoxSourceIsFile(bool)
{
}

void AudioComponentEditor::OnToggleCheckBoxEnableTextMessage(bool bState)
{
	EnableTextMessage(bState);
}

void AudioComponentEditor::EnableTextMessage(const bool bFlag)
{
    ui.CheckBoxEnableTextMessage->setChecked(bFlag);
    ui.PushButtonAddText->setEnabled(bFlag);
    ui.PushButtonDeleteText->setEnabled(bFlag);
    ui.PushButtonClearAllText->setEnabled(bFlag);
}

void AudioComponentEditor::OnPushButtonAddText()
{
	QString msg = ui.LineEditTextMessage->text();
	if(msg != "")
		(void) new Q3ListViewItem(ui.ListViewTextMessages, msg);
}

void AudioComponentEditor::OnPushButtonDeleteText()
{
	Q3ListViewItem* p = ui.ListViewTextMessages->selectedItem();
	if(p)
		delete p;
}

void AudioComponentEditor::OnButtonClearAllText()
{
    ui.ListViewTextMessages->clear();
}

DataComponentEditor::DataComponentEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void DataComponentEditor::setupUi()
{
    ui.ComboBoxDataStreamNo->clear();
	ui.ComboBoxDataStreamNo->insertItem("-");
	ui.ComboBoxDataStreamNo->insertItem("0");
	ui.ComboBoxDataStreamNo->insertItem("1");
	ui.ComboBoxDataStreamNo->insertItem("2");
	ui.ComboBoxDataStreamNo->insertItem("3");

    ui.ComboBoxDataPacketId->clear();
	ui.ComboBoxDataPacketId->insertItem("-");
	ui.ComboBoxDataPacketId->insertItem("0");
	ui.ComboBoxDataPacketId->insertItem("1");
	ui.ComboBoxDataPacketId->insertItem("2");
	ui.ComboBoxDataPacketId->insertItem("3");

	/* Clear list box for file names and set up columns */
	ui.ListViewFileNames->setAllColumnsShowFocus(true);

	ui.ListViewFileNames->clear();

	/* We assume that one column is already there */
	ui.ListViewFileNames->setColumnText(0, "File Name");
	ui.ListViewFileNames->addColumn("Size [KB]");
	ui.ListViewFileNames->addColumn("Full Path");

	connect(ui.PushButtonAddFile, SIGNAL(clicked()),
		this, SLOT(OnPushButtonAddFileName()));
	connect(ui.PushButtonClearAllFileNames, SIGNAL(clicked()),
		this, SLOT(OnButtonClearAllFileNames()));
}

void
DataComponentEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
	CParameter& Parameters = *DRMTransmitter.GetParameters();

	for(map<int, map<int, CDataParam> >::const_iterator i=Parameters.DataParam.begin();
                                                        i!=Parameters.DataParam.end(); i++)
	{
	    int stream = i->first;
	    for(map<int, CDataParam>::const_iterator j=i->second.begin(); j!=i->second.end(); j++)
	    {
	        int packetId = j->first;
            if(j->second.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
            {
                // we only do slideshow!
                ui.ComboBoxDataStreamNo->setCurrentIndex(ui.ComboBoxDataStreamNo->findText(QString::number(stream)));
                ui.ComboBoxDataPacketId->setCurrentIndex(ui.ComboBoxDataPacketId->findText(QString::number(packetId)));
                /* file names for data application */
                map<string,string> m;
                DRMTransmitter.GetPics(m);

                ui.ListViewFileNames->clear();
                for (map<string,string>::const_iterator p=m.begin(); p!=m.end(); p++)
                {
                    AddSlide(p->first.c_str());
                }
                break; // one is enough!
            }
	    }
	}
}

void
DataComponentEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
	CParameter& Parameters = *DRMTransmitter.GetParameters();

	Parameters.DataParam.clear();

    if(ui.ComboBoxDataStreamNo->currentText() == "-")
        return;

    if(ui.ComboBoxDataPacketId->currentText() == "-")
        return;

    int iStreamNo = ui.ComboBoxDataStreamNo->currentText().toInt();
    int iPacketId = ui.ComboBoxDataPacketId->currentText().toInt();

	Parameters.DataParam.clear();

	/* Init SlideShow application */
	Parameters.DataParam[iStreamNo][iPacketId].eAppDomain = CDataParam::AD_DAB_SPEC_APP;
	Parameters.DataParam[iStreamNo][iPacketId].ePacketModInd = PM_PACKET_MODE;
	Parameters.DataParam[iStreamNo][iPacketId].eDataUnitInd = CDataParam::DU_DATA_UNITS;

	/* file names for data application */
	DRMTransmitter.ClearPics();

	Q3ListViewItemIterator it(ui.ListViewFileNames);

	for (; it.current(); it++)
	{
		/* Complete file path is in third column */
		const QString strFileName = "";//it.current()->text(2); TODO

		/* Extract format string */
		QFileInfo FileInfo(strFileName);
		const QString strFormat = FileInfo.extension(false);

		DRMTransmitter.AddPic(strFileName.latin1(), strFormat.latin1());
	}
}

void DataComponentEditor::AddSlide(const QString& path)
{
    QFileInfo FileInfo(path);

    /* Insert list view item. The object which is created here will be
       automatically destroyed by QT when the parent
       ("ListViewFileNames") is destroyed */
    ui.ListViewFileNames->insertItem(
        new Q3ListViewItem(ui.ListViewFileNames, FileInfo.fileName(),
        QString().setNum((float) FileInfo.size() / 1000.0, 'f', 2),
        FileInfo.filePath()));
}

void DataComponentEditor::OnPushButtonAddFileName()
{
	/* Show "open file" dialog. Let the user select more than one file */
	QStringList list = QFileDialog::getOpenFileNames(
		tr("Image Files (*.png *.jpg *.jpeg *.jfif)"), NULL);

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

void DataComponentEditor::OnButtonClearAllFileNames()
{
	/* Clear list box for file names */
	ui.ListViewFileNames->clear();
}

void DataComponentEditor::EnableData(const bool bFlag)
{
	if (bFlag == true)
	{
		/* Enable data controls */
		ui.ListViewFileNames->setEnabled(true);
		ui.PushButtonClearAllFileNames->setEnabled(true);
		ui.PushButtonAddFile->setEnabled(true);
	}
	else
	{
		/* Disable data controls */
		ui.ListViewFileNames->setEnabled(false);
		ui.PushButtonClearAllFileNames->setEnabled(false);
		ui.PushButtonAddFile->setEnabled(false);
	}
}

ServicesEditor::ServicesEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void ServicesEditor::setupUi()
{
	/* Language */
	for (int i = 0; i < LEN_TABLE_LANGUAGE_CODE; i++)
		ui.ComboBoxFACLanguage->insertItem(strTableLanguageCode[i].c_str(), i);

	/* Program type */
	for (int i = 0; i < LEN_TABLE_PROG_TYPE_CODE; i++)
		ui.ComboBoxProgramType->insertItem(strTableProgTypCod[i].c_str(), i);

	/* services */
	ui.ComboBoxStreamType->insertItem(tr("audio"));
	ui.ComboBoxStreamType->insertItem(tr("data packet"));
	ui.ComboBoxStreamType->insertItem(tr("data stream"));
	ui.ComboBoxStream->insertItem("0");
	ui.ComboBoxStream->insertItem("1");
	ui.ComboBoxStream->insertItem("2");
	ui.ComboBoxStream->insertItem("3");

	ui.ListViewServices->setAllColumnsShowFocus(true);
	ui.ListViewServices->clear();
	ui.ComboBoxShortID->insertItem("0");
	ui.ComboBoxShortID->insertItem("1");
	ui.ComboBoxShortID->insertItem("2");
	ui.ComboBoxShortID->insertItem("3");

    ui.ComboBoxServiceAudioStream->clear();
	ui.ComboBoxServiceAudioStream->insertItem("0");
	ui.ComboBoxServiceAudioStream->insertItem("1");
	ui.ComboBoxServiceAudioStream->insertItem("2");
	ui.ComboBoxServiceAudioStream->insertItem("3");

    ui.ComboBoxServiceDataStream->clear();
	ui.ComboBoxServiceDataStream->insertItem("0");
	ui.ComboBoxServiceDataStream->insertItem("1");
	ui.ComboBoxServiceDataStream->insertItem("2");
	ui.ComboBoxServiceDataStream->insertItem("3");

    ui.ComboBoxServicePacketID->clear();
	ui.ComboBoxServicePacketID->insertItem("0");
	ui.ComboBoxServicePacketID->insertItem("1");
	ui.ComboBoxServicePacketID->insertItem("2");
	ui.ComboBoxServicePacketID->insertItem("3");

    ui.ComboBoxAppType->clear();
    ui.ComboBoxAppType->insertItem(tr("Normal"));
    ui.ComboBoxAppType->insertItem(tr("Engineering Test"));
	for (int t = 1; t < 31; t++)
	{
	    QString reserved = QString(tr("Reserved"))+" (%1)";
		ui.ComboBoxAppType->insertItem(reserved.arg(t));
	}

	connect(ui.PushButtonAddService, SIGNAL(clicked()),
		this, SLOT(OnButtonAdd()));
	connect(ui.PushButtonDeleteService, SIGNAL(clicked()),
		this, SLOT(OnButtonDelete()));
	connect(ui.LineEditServiceLabel, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedLabel(const QString&)));
	connect(ui.LineEditServiceID, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedServiceID(const QString&)));
	connect(ui.ListViewServices, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnListItemClicked(Q3ListViewItem*)));
}

void
ServicesEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
	const vector<CService>& Service = DRMTransmitter.GetParameters()->Service;
	for(size_t i=0; i<Service.size(); i++)
	{
		Q3ListViewItem* v = new Q3ListViewItem
		(
            ui.ListViewServices,
            QString::number(i),
            Service[i].strLabel.c_str(),
            QString::number(ulong(Service[i].iServiceID), 16),
            ui.ComboBoxFACLanguage->text(Service[i].iLanguage),
            Service[i].strLanguageCode.c_str(),
            Service[i].strCountryCode.c_str()
		);
		if(Service[i].iDataStream != STREAM_ID_NOT_USED)
		{
            v->setText(6, ui.ComboBoxAppType->text(Service[i].iServiceDescr));
			v->setText(8, QString::number(Service[i].iDataStream));
            v->setText(9, QString::number(Service[i].iPacketID));
		}
		if(Service[i].iAudioStream!=STREAM_ID_NOT_USED)
		{
		    /* audio overrides data */
            v->setText(6, ui.ComboBoxProgramType->text(Service[i].iServiceDescr));
			v->setText(7, QString::number(Service[i].iAudioStream));
		}
	}
	ui.ListViewServices->setSelected(ui.ListViewServices->firstChild(), true);
}

void
ServicesEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
	DRMTransmitter.GetParameters()->FACParameters.iNumDataServices=0;
	DRMTransmitter.GetParameters()->FACParameters.iNumAudioServices=0;

	Q3ListViewItemIterator sit(ui.ListViewServices);
	for (; sit.current(); sit++)
	{
		int iShortID = sit.current()->text(0).toUInt();
		CService Service;
		Service.strLabel = sit.current()->text(1).toUtf8().constData();
		Service.iServiceID = sit.current()->text(2).toULong(NULL, 16);
		QString lang = sit.current()->text(3);
		for(int i=0; i<ui.ComboBoxFACLanguage->count(); i++)
            if(ui.ComboBoxFACLanguage->text(i)==lang)
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
            for(int i=0; i<ui.ComboBoxAppType->count(); i++)
                if(ui.ComboBoxAppType->text(i)==type)
                    Service.iServiceDescr = i;
		}
		else
		{
			Service.iAudioStream = sA.toUInt();
			DRMTransmitter.GetParameters()->FACParameters.iNumAudioServices++;
			Service.eAudDataFlag = SF_AUDIO;
            for(int i=0; i<ui.ComboBoxProgramType->count(); i++)
                if(ui.ComboBoxProgramType->text(i)==type)
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

void ServicesEditor::OnTextChangedServiceID(const QString& strID)
{
	(void)strID; // TODO
}

void ServicesEditor::OnTextChangedLabel(const QString& strLabel)
{
	(void)strLabel; // TODO
}

void ServicesEditor::OnButtonAdd()
{
	Q3ListViewItem* v = new Q3ListViewItem(ui.ListViewServices,
        ui.ComboBoxShortID->currentText(),
		ui.LineEditServiceLabel->text(),
		ui.LineEditServiceID->text(),
		ui.ComboBoxFACLanguage->currentText(),
		ui.LineEditSDCLanguage->text(),
		ui.LineEditCountry->text()
	);
    if(ui.CheckBoxDataComp->isChecked())
    {
        v->setText(6, ui.ComboBoxAppType->currentText());
        v->setText(8, ui.ComboBoxServiceDataStream->currentText());
        v->setText(9, ui.ComboBoxServicePacketID->currentText());
    }
    if(ui.CheckBoxAudioComp->isChecked())
    {
        /* audio overrides data */
        v->setText(6, ui.ComboBoxProgramType->currentText());
        v->setText(7, ui.ComboBoxServiceAudioStream->currentText());
    }
	ui.ComboBoxShortID->setCurrentItem(0);
}

void ServicesEditor::OnButtonDelete()
{
	Q3ListViewItem* p = ui.ListViewServices->selectedItem();
	if(p)
	{
		QStringList s(p->text(0));
		for(int i=0; i<ui.ComboBoxShortID->count(); i++)
			s.append(ui.ComboBoxShortID->text(i));
		s.sort();
		ui.ComboBoxShortID->clear();
		ui.ComboBoxShortID->insertStringList(s);
		delete p;
	}
	ui.ComboBoxShortID->setCurrentItem(0);
}

void ServicesEditor::OnListItemClicked(Q3ListViewItem* item)
{
    ui.ComboBoxShortID->setCurrentIndex(ui.ComboBoxShortID->findText(item->text(0)));
	ui.LineEditServiceLabel->setText(item->text(1));
	ui.LineEditServiceID->setText(item->text(2));
    ui.ComboBoxFACLanguage->setCurrentIndex(ui.ComboBoxFACLanguage->findText(item->text(3)));
	ui.LineEditSDCLanguage->setText(item->text(4));
	ui.LineEditCountry->setText(item->text(5));
	if(item->text(8) != "")
	{
	    ui.CheckBoxDataComp->setChecked(true);
        ui.ComboBoxAppType->setCurrentIndex(ui.ComboBoxAppType->findText(item->text(6)));
        ui.ComboBoxServiceDataStream->setCurrentIndex(ui.ComboBoxServiceDataStream->findText(item->text(7)));
        ui.ComboBoxServicePacketID->setCurrentIndex(ui.ComboBoxServicePacketID->findText(item->text(8)));
	}
	else
	{
	    ui.CheckBoxDataComp->setChecked(false);
	}
	if(item->text(7) != "")
	{
	    ui.CheckBoxAudioComp->setChecked(true);
        ui.ComboBoxProgramType->setCurrentIndex(ui.ComboBoxProgramType->findText(item->text(6)));
        ui.ComboBoxServiceAudioStream->setCurrentIndex(ui.ComboBoxServiceAudioStream->findText(item->text(7)));
	}
	else
	{
	    ui.CheckBoxAudioComp->setChecked(false);
	}
}

COFDMEditor::COFDMEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u)
{
}

void COFDMEditor::setupUi()
{
    outputGroup = new QButtonGroup(this);
    outputGroup->addButton(ui.RadioButtonOutReal, OF_REAL_VAL);
    outputGroup->addButton(ui.RadioButtonOutIQPos, OF_IQ_POS);
    outputGroup->addButton(ui.RadioButtonOutIQNeg, OF_IQ_NEG);
    outputGroup->addButton(ui.RadioButtonOutEP, OF_EP);

	ui.ListViewCOFDM->setAllColumnsShowFocus(true);
	ui.ListViewCOFDM->setColumnWidthMode(0, Q3ListView::Maximum);

	connect(ui.PushButtonCOFDMAddAudio, SIGNAL(clicked()),
		this, SLOT(OnButtonAddAudio()));
	connect(ui.PushButtonCOFDMAddFile, SIGNAL(clicked()),
		this, SLOT(OnButtonAddFile()));
	connect(ui.PushButtonCOFDMDeleteSelected, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteSelected()));
	connect(ui.PushButtonCOFDMBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonBrowse()));
	connect(ui.ComboBoxCOFDMdest, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxDestActivated(int)));
	connect(ui.LineEditSndCrdIF, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnTextChangedSndCrdIF(const QString&)));
	connect(ui.LineEditCOFDMOutputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));
	connect(ui.ListViewCOFDM, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnListItemClicked(Q3ListViewItem*)));
}

void
COFDMEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
    CParameter& Parameters = *DRMTransmitter.GetParameters();

	/* Output mode (real valued, I / Q or E / P) */
    outputGroup->button(Parameters.eOutputFormat)->setChecked(true);

	/* Sound card IF */
	ui.LineEditSndCrdIF->setText(QString().number(Parameters.rCarOffset, 'f', 2));

	/* Get sound device names */
	vector<string> vecAudioDevices;
	DRMTransmitter.GetSoundOutChoices(vecAudioDevices);
	for (size_t t = 0; t < vecAudioDevices.size(); t++)
	{
		ui.ComboBoxCOFDMdest->insertItem(QString(vecAudioDevices[t].c_str()));
	}
	ui.ComboBoxCOFDMdest->setCurrentItem(0);
	ui.ListViewCOFDM->clear();
	vector<string> COFDMOutputs;
	DRMTransmitter.GetCOFDMOutputs(COFDMOutputs);
	for(size_t i=0; i<COFDMOutputs.size(); i++)
		(void) new Q3ListViewItem(ui.ListViewCOFDM, COFDMOutputs[i].c_str());
}

void
COFDMEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
    CParameter& Parameters = *DRMTransmitter.GetParameters();

	Parameters.rCarOffset = ui.LineEditSndCrdIF->text().toFloat();

    Parameters.eOutputFormat = EOutFormat(outputGroup->checkedId());

	vector<string> COFDMOutputs;
	Q3ListViewItemIterator item(ui.ListViewCOFDM);
	for (; item.current(); item++)
	{
		COFDMOutputs.push_back(item.current()->text(0).latin1());
	}
	DRMTransmitter.SetCOFDMOutputs(COFDMOutputs);
}

void COFDMEditor::OnComboBoxDestActivated(int)
{
}

void COFDMEditor::OnLineEditFileChanged(const QString&)
{
}

void COFDMEditor::OnComboBoxFileDestActivated(int)
{
}

void COFDMEditor::OnTextChangedSndCrdIF(const QString&)
{
}

void
COFDMEditor::OnButtonAddAudio()
{
	(void) new Q3ListViewItem(ui.ListViewCOFDM, ui.ComboBoxCOFDMdest->currentText());
}

void COFDMEditor::OnButtonAddFile()
{
	QString file = ui.LineEditCOFDMOutputFile->text();
	if(file != "")
		(void) new Q3ListViewItem(ui.ListViewCOFDM, file);
}

void COFDMEditor::OnButtonDeleteSelected()
{
	Q3ListViewItem* p = ui.ListViewCOFDM->selectedItem();
	if(p)
		delete p;
}

void COFDMEditor::OnListItemClicked(Q3ListViewItem*)
{
	Q3ListViewItem* p = ui.ListViewCOFDM->selectedItem();
	if(p)
	{
		QString s = p->text(0);
		bool found = false;
		for(int i=0; i<ui.ComboBoxCOFDMdest->count(); i++)
		{
			if(ui.ComboBoxCOFDMdest->text(i) == s)
			{
				ui.ComboBoxCOFDMdest->setCurrentItem(i);
				found = true;
				break;
			}
		}
		if(found==false)
			ui.LineEditCOFDMOutputFile->setText(s);
	}
}

void COFDMEditor::OnButtonBrowse()
{
	QString s( QFileDialog::getSaveFileName(
		"cofdm.wav", "Wave Files (*.wav)", NULL ) );
	if ( s.isEmpty() )
		return;
	ui.LineEditCOFDMOutputFile->setText(s);
}

MDIInputEditor::MDIInputEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u),vecIpIf()
{
}

void MDIInputEditor::setupUi()
{
	ui.LineEditMDIinGroup->setEnabled(false);
	ui.LineEditMDIinGroup->setInputMask("000.000.000.000;_");
	ui.LineEditMDIOutAddr->setInputMask("000.000.000.000;_");
	ui.LineEditMDIinPort->setInputMask("00009;_");
	ui.LineEditMDIoutPort->setInputMask("00009;_");

    GetNetworkInterfaces(vecIpIf);
    for(size_t i=0; i<vecIpIf.size(); i++)
    {
        ui.ComboBoxMDIoutInterface->insertItem(vecIpIf[i].name.c_str());
    }

	connect(ui.PushButtonMDIInBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonBrowse()));
	connect(ui.CheckBoxMDIinMcast, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxMcast(bool)));
	connect(ui.CheckBoxReadMDIFile, SIGNAL(toggled(bool)),
		this, SLOT(OnToggleCheckBoxReadFile(bool)));
	connect(ui.ComboBoxMDIinInterface, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxInterfaceActivated(int)));
	connect(ui.LineEditMDIinPort, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPortChanged(const QString&)));
	connect(ui.LineEditMDIinGroup, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditGroupChanged(const QString&)));
	connect(ui.LineEditMDIInputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));
}

void
MDIInputEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
{
	QString addr = DRMTransmitter.GetMDIIn().c_str();
	QFileInfo f(addr);

    ui.CheckBoxMDIinMcast->setChecked(false);

	if(f.exists())
	{
		ui.CheckBoxReadMDIFile->setChecked(true);
		ui.LineEditMDIInputFile->setText(addr);
		ui.LineEditMDIinPort->setText("");
        ui.ComboBoxMDIinInterface->setCurrentIndex(ui.ComboBoxMDIinInterface->findText("any"));
		ui.LineEditMDIinGroup->setText("");
	}
	else
	{
		ui.CheckBoxReadMDIFile->setChecked(false);
		ui.LineEditMDIInputFile->setText("");
		QString port,group,ifname="any";

		QStringList parts = QStringList::split(":", addr, true);
		switch(parts.count())
		{
		case 1:
			port = parts[0];
			break;
		case 2:
			group = parts[0];
			port = parts[1];
            ui.CheckBoxMDIinMcast->setChecked(true); // TODO - check address type
			break;
		case 3:
            for(size_t i=0; i<vecIpIf.size(); i++)
            {
                if(parts[0].toUInt()==vecIpIf[i].addr)
                {
                    ifname = vecIpIf[i].name.c_str();
                }
            }
			group = parts[1];
			port = parts[2];
            ui.CheckBoxMDIinMcast->setChecked(true);
			break;
		}
        ui.LineEditMDIinGroup->setText(group);
        ui.LineEditMDIinPort->setText(port);
        int item = ui.ComboBoxMDIinInterface->findText(ifname);
        ui.ComboBoxMDIinInterface->setCurrentItem(item);
	}
}

void
MDIInputEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
	if(ui.CheckBoxReadMDIFile->isChecked())
	{
		DRMTransmitter.SetMDIIn(ui.LineEditMDIInputFile->text().latin1());
	}
	else
	{
		QString port = ui.LineEditMDIinPort->text();
		QString group = ui.LineEditMDIinGroup->text();
		QString iface = ui.ComboBoxMDIinInterface->currentText();
		QString addr=port;
		if(ui.CheckBoxMDIinMcast->isChecked())
			addr = group+":"+addr;
		if(iface!="any")
		{
			if(!ui.CheckBoxMDIinMcast->isChecked())
				addr = ":"+addr;
			addr = iface+":"+addr;
		}
		DRMTransmitter.SetMDIIn(addr.latin1());
	}
}


void
MDIInputEditor::OnLineEditPortChanged(const QString&)
{
}

void
MDIInputEditor::OnToggleCheckBoxMcast(bool)
{
}

void
MDIInputEditor::OnLineEditGroupChanged(const QString&)
{
}

void
MDIInputEditor::OnComboBoxInterfaceActivated(int)
{
}

void
MDIInputEditor::OnLineEditFileChanged(const QString&)
{
}

void
MDIInputEditor::OnButtonBrowse()
{
	QString s( QFileDialog::getOpenFileName(
		"out.pcap", "Capture Files (*.pcap)", NULL ) );
	if ( s.isEmpty() )
		return;
    ui.LineEditMDIInputFile->setText(s);
}

void
MDIInputEditor::OnToggleCheckBoxReadFile(bool)
{
}

MDIOutputEditor::MDIOutputEditor(Ui_TransmitterMainWindow& u)
:QObject(),ui(u), vecIpIf()
{
}

void MDIOutputEditor::setupUi()
{
    GetNetworkInterfaces(vecIpIf);
    for(size_t i=0; i<vecIpIf.size(); i++)
    {
        ui.ComboBoxMDIoutInterface->insertItem(vecIpIf[i].name.c_str());
    }

	connect(ui.PushButtonAddMDIDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddDest()));
	connect(ui.PushButtonAddMDIFileDest, SIGNAL(clicked()),
		this, SLOT(OnButtonAddFileDest()));
	connect(ui.PushButtonDeleteMDIOutput, SIGNAL(clicked()),
		this, SLOT(OnButtonDeleteOutput()));
	connect(ui.PushButtonMDIOutBrowse, SIGNAL(clicked()),
		this, SLOT(OnButtonBrowse()));
	connect(ui.ComboBoxMDIoutInterface, SIGNAL(activated(int)),
		this, SLOT(OnComboBoxInterfaceActivated(int)));
	connect(ui.LineEditMDIOutAddr, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditAddrChanged(const QString&)));
	connect(ui.LineEditMDIOutputFile, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditFileChanged(const QString&)));
	connect(ui.LineEditMDIoutPort, SIGNAL(textChanged(const QString&)),
		this, SLOT(OnLineEditPortChanged(const QString&)));
	connect(ui.ListViewMDIOutputs, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnListItemClicked(Q3ListViewItem*)));
}

void
MDIOutputEditor::GetFrom(CDRMTransmitterInterface& DRMTransmitter)
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
            (void)new Q3ListViewItem(ui.ListViewMDIOutputs, parts[0]);
            break;
        case 1:
            (void)new Q3ListViewItem(ui.ListViewMDIOutputs, parts[0], "", "any");
            break;
        case 2:
            (void)new Q3ListViewItem(ui.ListViewMDIOutputs, parts[1], parts[0], "any");
            break;
        case 3:
            {
                QString name;
                for(size_t j=0; j<vecIpIf.size(); j++)
                {
                    if(parts[0].toUInt()==vecIpIf[j].addr)
                        name = vecIpIf[j].name.c_str();
                }
                (void)new Q3ListViewItem(ui.ListViewMDIOutputs, parts[2], parts[1], name);
            }
        }
    }
}

void
MDIOutputEditor::PutTo(CDRMTransmitterInterface& DRMTransmitter)
{
	vector<string> MDIoutAddr;
	Q3ListViewItemIterator it(ui.ListViewMDIOutputs);
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
			addr = iface+":"+dest+":"+port;
		}
		MDIoutAddr.push_back(string(addr.utf8()));
	}
	DRMTransmitter.SetMDIOut(MDIoutAddr);
}

void
MDIOutputEditor::OnButtonAddDest()
{
	QString dest = ui.LineEditMDIOutAddr->text();
	if(dest == "...")
		dest = "";
    (void) new Q3ListViewItem(ui.ListViewMDIOutputs,
        ui.LineEditMDIoutPort->text(),
        dest,
        ui.ComboBoxMDIoutInterface->currentText()
    );
}

void
MDIOutputEditor::OnButtonAddFileDest()
{
	QString file = ui.LineEditMDIOutputFile->text();
	if(file != "")
		(void) new Q3ListViewItem(ui.ListViewMDIOutputs, file);
}

void
MDIOutputEditor::OnButtonDeleteOutput()
{
	Q3ListViewItem* p = ui.ListViewMDIOutputs->selectedItem();
	if(p)
		delete p;
}

void MDIOutputEditor::OnButtonBrowse()
{
    QString s( QFileDialog::getSaveFileName(
			"out.pcap", "Capture Files (*.pcap)", NULL ) );
    if ( s.isEmpty() )
        return;
    ui.LineEditMDIOutputFile->setText(s);
}

void MDIOutputEditor::OnComboBoxInterfaceActivated(int)
{
}

void MDIOutputEditor::OnLineEditAddrChanged(const QString&)
{
}

void MDIOutputEditor::OnLineEditFileChanged(const QString&)
{
}

void MDIOutputEditor::OnLineEditPortChanged(const QString&)
{
}

void MDIOutputEditor::OnListItemClicked(Q3ListViewItem* item)
{
	QString dest = item->text(1);
	if(dest == "")
	{
    	ui.LineEditMDIOutputFile->setText(item->text(0));
        ui.LineEditMDIoutPort->setText("");
		ui.LineEditMDIOutAddr->setText("");
        ui.ComboBoxMDIoutInterface->setCurrentIndex(ui.ComboBoxMDIoutInterface->findText("any"));
	}
	else
	{
    	ui.LineEditMDIOutputFile->setText("");
        ui.LineEditMDIoutPort->setText(item->text(0));
		ui.LineEditMDIOutAddr->setText(dest);
        ui.ComboBoxMDIoutInterface->setCurrentIndex(ui.ComboBoxMDIoutInterface->findText(item->text(2)));
	}
}
