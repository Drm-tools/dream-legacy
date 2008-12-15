/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additional widgets for displaying AMSS information
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

#include "AnalogDemDlg.h"
#include <qmessagebox.h>
#include "ReceiverSettingsDlg.h"


/* Implementation *************************************************************/
AnalogDemDlg::AnalogDemDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	ReceiverSettingsDlg& NReceiverSettingsDlg,
	QWidget* parent, const char* name, bool modal, WFlags f):
	AnalogDemDlgBase(parent, name, modal, f),
	Receiver(NDRMR), Settings(NSettings), pReceiverSettingsDlg(&NReceiverSettingsDlg),
	AMSSDlg(NDRMR, Settings, parent, name, modal, f)
{
	CWinGeom s;
	Settings.Get("AM Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	QPopupMenu* EvalWinMenu = new QPopupMenu(this);
	CHECK_PTR(EvalWinMenu);
	EvalWinMenu->insertItem(tr("S&tations Dialog..."), this,
		SIGNAL(ViewStationsDlg()), CTRL+Key_T);
	EvalWinMenu->insertItem(tr("&Live Schedule Dialog..."), this,
		SIGNAL(ViewLiveScheduleDlg()), CTRL+Key_L);
	EvalWinMenu->insertSeparator();
	EvalWinMenu->insertItem(tr("E&xit"), this, SLOT(close()), CTRL+Key_Q);

	/* Settings menu  ------------------------------------------------------- */
	QPopupMenu* pSettingsMenu = new QPopupMenu(this);
	CHECK_PTR(pSettingsMenu);
	pSettingsMenu->insertItem(tr("&Sound Card Selection"),
		new CSoundCardSelMenu(Receiver.GetSoundInInterface(), Receiver.GetSoundOutInterface(), this));
	pSettingsMenu->insertItem(tr("&DRM (digital)"), this,
		SLOT(OnSwitchToDRM()), CTRL+Key_D);
	pSettingsMenu->insertItem(tr("New &AM Acquisition"), this,
		SLOT(OnNewAMAcquisition()), CTRL+Key_A);
	pSettingsMenu->insertItem(tr("&Receiver settings..."), this,
		SLOT(OnViewReceiverSettingsDlg()));

	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), EvalWinMenu);
	pMenu->insertItem(tr("&Settings"), pSettingsMenu);
	pMenu->insertItem(tr("&?"), new CDreamHelpMenu(this));
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	AnalogDemDlgBaseLayout->setMenuBar(pMenu);


	/* Init main plot */
	MainPlot->SetRecObj(&Receiver);
	MainPlot->SetPlotStyle(Settings.Get("System Evaluation Dialog", "plotstyle", 0));
	MainPlot->setMargin(1);
	MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);

	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot,
		tr("Click on the plot to set the demodulation frequency"));

	SliderBandwidth->setRange(0, SOUNDCRD_SAMPLE_RATE / 2);
	SliderBandwidth->setTickmarks(QSlider::Both);
	SliderBandwidth->setTickInterval(1000); /* Each kHz a tick */
	SliderBandwidth->setPageStep(1000); /* Hz */

	/* Init PLL phase dial control */
	PhaseDial->setMode(QwtDial::RotateNeedle);
	PhaseDial->setWrapping(true);
	PhaseDial->setReadOnly(true);
	PhaseDial->setScale(0, 360, 45); /* Degrees */
	PhaseDial->setOrigin(270);
	PhaseDial->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));
	PhaseDial->setFrameShadow(QwtDial::Plain);
	PhaseDial->setScaleOptions(QwtDial::ScaleTicks);


	/* Update controls */
	UpdateControls();


	/* Connect controls ----------------------------------------------------- */
	connect(ButtonDRM, SIGNAL(clicked()),
		this, SLOT(OnSwitchToDRM()));
	connect(ButtonAMSS, SIGNAL(clicked()),
		this, SLOT(OnButtonAMSS()));
	connect(ButtonWaterfall, SIGNAL(clicked()),
		this, SLOT(OnButtonWaterfall()));
	connect(MainPlot, SIGNAL(xAxisValSet(double)),
		this, SLOT(OnChartxAxisValSet(double)));

	/* Button groups */
	connect(ButtonGroupDemodulation, SIGNAL(clicked(int)),
		this, SLOT(OnRadioDemodulation(int)));
	connect(ButtonGroupAGC, SIGNAL(clicked(int)),
		this, SLOT(OnRadioAGC(int)));
	connect(ButtonGroupNoiseReduction, SIGNAL(clicked(int)),
		this, SLOT(OnRadioNoiRed(int)));

	/* Slider */
	connect(SliderBandwidth, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderBWChange(int)));

	/* Check boxes */
	connect(CheckBoxMuteAudio, SIGNAL(clicked()),
		this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()),
		this, SLOT(OnCheckSaveAudioWAV()));
	connect(CheckBoxAutoFreqAcq, SIGNAL(clicked()),
		this, SLOT(OnCheckAutoFreqAcq()));
	connect(CheckBoxPLL, SIGNAL(clicked()),
		this, SLOT(OnCheckPLL()));
	connect(CheckBoxOnBoardDemod, SIGNAL(clicked()),
		this, SLOT(OnCheckOnBoardDemod()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));

	/* Don't activate real-time timers, wait for show event */
}

void AnalogDemDlg::OnViewReceiverSettingsDlg()
{
	pReceiverSettingsDlg->show();
}

void AnalogDemDlg::showEvent(QShowEvent*)
{
	OnTimer();
	OnTimerPLLPhaseDial();

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);

	/* Open AMSS window */
	if (Settings.Get("AMSS Dialog", "visible", FALSE) == TRUE)
		AMSSDlg.show();
	else
		AMSSDlg.hide();

	UpdateControls();
}

void AnalogDemDlg::hideEvent(QHideEvent*)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Close AMSS window */
	Settings.Put("AMSS Dialog", "visible", AMSSDlg.isVisible());
	AMSSDlg.hide();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AM Dialog", s);
	Settings.Put("GUI", "mode", string("AMRX"));
}

void AnalogDemDlg::closeEvent(QCloseEvent* ce)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	Settings.Put("AMSS Dialog", "visible", AMSSDlg.isVisible());

	/* tell every other window to close too */
	emit Closed();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AM Dialog", s);

	/* request that the working thread stops */
	Receiver.Stop();
	(void)Receiver.wait(5000);
	if(!Receiver.finished())
	{
		QMessageBox::critical(this, "Dream", "Exit\n",
				"Termination of working thread failed");
	}
	ce->accept();
}

void AnalogDemDlg::UpdateControls()
{
	/* Set demodulation type */
	switch (Receiver.GetReceiverMode())
	{
	case AM:
		if (!RadioButtonDemAM->isChecked())
			RadioButtonDemAM->setChecked(TRUE);
		break;

	case LSB:
		if (!RadioButtonDemLSB->isChecked())
			RadioButtonDemLSB->setChecked(TRUE);
		break;

	case USB:
		if (!RadioButtonDemUSB->isChecked())
			RadioButtonDemUSB->setChecked(TRUE);
		break;

	case CW:
		if (!RadioButtonDemCW->isChecked())
			RadioButtonDemCW->setChecked(TRUE);
		break;

	case NBFM:
		if (!RadioButtonDemNBFM->isChecked())
			RadioButtonDemNBFM->setChecked(TRUE);
		break;

	case WBFM:
		if (!RadioButtonDemWBFM->isChecked())
			RadioButtonDemWBFM->setChecked(TRUE);
		break;

	case DRM: case NONE:
		break;
	}

	/* Set AGC type */
	switch (Receiver.GetAnalogAGCType())
	{
	case CAGC::AT_NO_AGC:
		if (!RadioButtonAGCOff->isChecked())
			RadioButtonAGCOff->setChecked(TRUE);
		break;

	case CAGC::AT_SLOW:
		if (!RadioButtonAGCSlow->isChecked())
			RadioButtonAGCSlow->setChecked(TRUE);
		break;

	case CAGC::AT_MEDIUM:
		if (!RadioButtonAGCMed->isChecked())
			RadioButtonAGCMed->setChecked(TRUE);
		break;

	case CAGC::AT_FAST:
		if (!RadioButtonAGCFast->isChecked())
			RadioButtonAGCFast->setChecked(TRUE);
		break;
	}

	/* Set noise reduction type */
	switch (Receiver.GetAnalogNoiseReductionType())
	{
	case CAMDemodulation::NR_OFF:
		if (!RadioButtonNoiRedOff->isChecked())
			RadioButtonNoiRedOff->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_LOW:
		if (!RadioButtonNoiRedLow->isChecked())
			RadioButtonNoiRedLow->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_MEDIUM:
		if (!RadioButtonNoiRedMed->isChecked())
			RadioButtonNoiRedMed->setChecked(TRUE);
		break;

	case CAMDemodulation::NR_HIGH:
		if (!RadioButtonNoiRedHigh->isChecked())
			RadioButtonNoiRedHigh->setChecked(TRUE);
		break;
	}

	/* Set filter bandwidth */
	SliderBandwidth->setValue(Receiver.GetAnalogFilterBWHz());
	TextLabelBandWidth->setText(QString().setNum(Receiver.GetAnalogFilterBWHz())+tr(" Hz"));

	/* Update check boxes */
	CheckBoxMuteAudio->setChecked(Receiver.GetMuteAudio());
	CheckBoxSaveAudioWave->setChecked(Receiver.GetIsWriteWaveFile());

	CheckBoxAutoFreqAcq->
		setChecked(Receiver.AnalogAutoFreqAcqEnabled());

	CheckBoxPLL->setChecked(Receiver.AnalogPLLEnabled());
}

void AnalogDemDlg::OnCheckOnBoardDemod()
{
	if (CheckBoxOnBoardDemod->isChecked() == TRUE)
		Receiver.SetUseAnalogHWDemod(TRUE);
	else
		Receiver.SetUseAnalogHWDemod(FALSE);
}

void AnalogDemDlg::UpdatePlotsStyle()
{
	/* Update main plot window */
	MainPlot->SetPlotStyle(Settings.Get("System Evaluation Dialog", "plotstyle", 0));
}

void AnalogDemDlg::OnSwitchToDRM()
{
	this->hide();
	SwitchToDRM();
}

void AnalogDemDlg::OnTimer()
{
	bool b;

	switch(Receiver.GetReceiverMode())
	{
	case DRM:
		OnSwitchToDRM();
		break;
	case AM: case  USB: case  LSB: case  CW: case  NBFM: case  WBFM:
		/* Carrier frequency of AM signal */
		TextFreqOffset->setText(tr("Carrier<br>Frequency:<br><b>")
		+ QString().setNum(Receiver.GetAnalogCurMixFreqOffs(), 'f', 2) + " Hz</b>");
		b = Receiver.GetUseAnalogHWDemod();
		if(b)
		{
			CheckBoxOnBoardDemod->setEnabled(true);
			CheckBoxOnBoardDemod->setChecked(true);
			//EDemodType eMode = Parameters.eDemodType;
			/* TODO enable & disable the Onboard checkbox according to the rig caps */
		}
		else
		{
			//CheckBoxOnBoardDemod->setEnabled(false);
		}
		UpdateControls();
		break;
	case NONE:
		break;
	}
}

void AnalogDemDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (Receiver.GetAnalogPLLPhase(rCurPLLPhase) == TRUE)
	{
		/* Set current PLL phase (convert radiant in degree) */
		PhaseDial->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
		if (!PhaseDial->isEnabled())
			PhaseDial->setEnabled(true);
	}
	else
	{
		/* Reset dial */
		PhaseDial->setValue((CReal) 0.0);

		/* Check if control is disabled */
		if (PhaseDial->isEnabled())
			PhaseDial->setEnabled(false);
	}
}

void AnalogDemDlg::OnRadioDemodulation(int iID)
{
	/* Receiver takes care of setting appropriate filter BW */
	switch (iID)
	{
	case 0:
		Receiver.SetReceiverMode(AM);
		break;

	case 1:
		Receiver.SetReceiverMode(LSB);
		break;

	case 2:
		Receiver.SetReceiverMode(USB);
		break;

	case 3:
		Receiver.SetReceiverMode(CW);
		break;

	case 4:
		Receiver.SetReceiverMode(NBFM);
		break;

	case 5:
		Receiver.SetReceiverMode(WBFM);
		break;
	}

	/* Update controls */
	UpdateControls();
}

void AnalogDemDlg::OnRadioAGC(int iID)
{
	switch (iID)
	{
	case 0:
		Receiver.SetAnalogAGCType(CAGC::AT_NO_AGC);
		break;

	case 1:
		Receiver.SetAnalogAGCType(CAGC::AT_SLOW);
		break;

	case 2:
		Receiver.SetAnalogAGCType(CAGC::AT_MEDIUM);
		break;

	case 3:
		Receiver.SetAnalogAGCType(CAGC::AT_FAST);
		break;
	}
}

void AnalogDemDlg::OnRadioNoiRed(int iID)
{
	switch (iID)
	{
	case 0:
		Receiver.SetAnalogNoiseReductionType(CAMDemodulation::NR_OFF);
		break;

	case 1:
		Receiver.SetAnalogNoiseReductionType(CAMDemodulation::NR_LOW);
		break;

	case 2:
		Receiver.SetAnalogNoiseReductionType(CAMDemodulation::NR_MEDIUM);
		break;

	case 3:
		Receiver.SetAnalogNoiseReductionType(CAMDemodulation::NR_HIGH);
		break;
	}
}

void AnalogDemDlg::OnSliderBWChange(int value)
{
	/* Set new filter in processing module */
	Receiver.SetAnalogFilterBWHz(value);
	TextLabelBandWidth->setText(QString().setNum(value) + tr(" Hz"));

	/* Update chart */
	MainPlot->Update();
}

void AnalogDemDlg::OnCheckAutoFreqAcq()
{
	/* Set parameter in working thread module */
	Receiver.EnableAnalogAutoFreqAcq(CheckBoxAutoFreqAcq->isChecked());
}

void AnalogDemDlg::OnCheckPLL()
{
	/* Set parameter in working thread module */
	Receiver.EnableAnalogPLL(CheckBoxPLL->isChecked());
}

void AnalogDemDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	Receiver.MuteAudio(CheckBoxMuteAudio->isChecked());
}

void AnalogDemDlg::OnCheckSaveAudioWAV()
{
/*
	This code is copied in systemevalDlg.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (CheckBoxSaveAudioWave->isChecked() == TRUE)
	{
		/* Show "save file" dialog */
		QString strFileName =
			QFileDialog::getSaveFileName("DreamOut.wav", "*.wav", this);

		/* Check if user not hit the cancel button */
		if (!strFileName.isNull())
		{
			Receiver.StartWriteWaveFile(strFileName.latin1());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(FALSE);
		}
	}
	else
		Receiver.StopWriteWaveFile();
}

void AnalogDemDlg::OnChartxAxisValSet(double dVal)
{
	/* Set new frequency in receiver module */
	Receiver.SetAnalogDemodAcq(dVal);

	/* Update chart */
	MainPlot->Update();
}

void AnalogDemDlg::OnButtonWaterfall()
{
	/* Toggle between normal spectrum plot and waterfall spectrum plot */
	if (ButtonWaterfall->state() == QButton::On)
		MainPlot->SetupChart(CDRMPlot::INP_SPEC_WATERF);
	else
		MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);
}

void AnalogDemDlg::OnButtonAMSS()
{
	/* Open AMSS window and set in foreground */
	AMSSDlg.show();
	AMSSDlg.raise();
}

void AnalogDemDlg::AddWhatsThisHelp()
{
	/* Noise Reduction */
	const QString strNoiseReduction =
		tr("<b>Noise Reduction:</b> The noise suppression is a frequency "
		"domain optimal filter design based algorithm. The noise PSD is "
		"estimated utilizing a minimum statistic. A problem of this type of "
		"algorithm is that it produces the so called \"musical tones\". The "
		"noise becomes colored and sounds a bit strange. At the same time, "
		"the useful signal (which might be speech or music) is also "
		"distorted by the algorithm. By selecting the level of noise "
		"reduction, a compromise between distortion of the useful signal "
		"and actual noise reduction can be made.");

	QWhatsThis::add(ButtonGroupNoiseReduction, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedOff, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedLow, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedMed, strNoiseReduction);
	QWhatsThis::add(RadioButtonNoiRedHigh, strNoiseReduction);

	/* Automatic Gain Control */
	const QString strAGC =
		tr("<b>AGC (Automatic Gain Control):</b> Input signals can have a "
		"large variation in power due to channel impairments. To compensate "
		"for that, an automatic gain control can be applied. The AGC has "
		"four settings: Off, Slow, Medium and Fast.");

	QWhatsThis::add(ButtonGroupAGC, strAGC);
	QWhatsThis::add(RadioButtonAGCOff, strAGC);
	QWhatsThis::add(RadioButtonAGCSlow, strAGC);
	QWhatsThis::add(RadioButtonAGCMed, strAGC);
	QWhatsThis::add(RadioButtonAGCFast, strAGC);

	/* Filter Bandwidth */
	const QString strFilterBW =
		tr("<b>Filter Bandwidth:</b> A band-pass filter is applied before "
		"the actual demodulation process. With this filter, adjacent signals "
		"are attenuated. The bandwidth of this filter can be chosen in steps "
		"of 1 Hz by using the slider bar. Clicking on the right or left side "
		"of the slider leveler will increase/decrease the bandwidth by 1 kHz. "
		"<br>The current filter bandwidth is indicated in the spectrum plot "
		"by a selection bar.");

	QWhatsThis::add(ButtonGroupBW, strFilterBW);
	QWhatsThis::add(TextLabelBandWidth, strFilterBW);
	QWhatsThis::add(SliderBandwidth, strFilterBW);

	/* Demodulation type */
	const QString strDemodType =
		tr("<b>Demodulation Type:</b> The following analog "
		"demodulation types are available:<ul>"
		"<li><b>AM:</b> This analog demodulation type is used in most of "
		"the hardware radios. The envelope of the complex base-band signal "
		"is used followed by a high-pass filter to remove the DC offset. "
		"Additionally, a low pass filter with the same bandwidth as the "
		"pass-band filter is applied to reduce the noise caused by "
		"non-linear distortions.</li>"
		"<li><b>LSB / USB:</b> These are single-side-band (SSB) demodulation "
		"types. Only one side of the spectrum is evaluated, the upper side "
		"band is used in USB and the lower side band with LSB. It is "
		"important for SSB demodulation that the DC frequency of the analog "
		"signal is known to get satisfactory results. The DC frequency is "
		"automatically estimated by starting a new acquisition or by "
		"clicking on the plot.</li>"
		"<li><b>CW:</b> This demodulation type can be used to receive "
		"CW signals. Only a narrow frequency band in a fixed distance "
		"to the mixing frequency is used. By clicking on the spectrum "
		"plot, the center position of the band pass filter can be set.</li>"
		"<li><b>FM:</b> This is a narrow band frequency demodulation.</li>"
		"</ul>");

	QWhatsThis::add(ButtonGroupDemodulation, strDemodType);
	QWhatsThis::add(RadioButtonDemAM, strDemodType);
	QWhatsThis::add(RadioButtonDemLSB, strDemodType);
	QWhatsThis::add(RadioButtonDemUSB, strDemodType);
	QWhatsThis::add(RadioButtonDemCW, strDemodType);
	QWhatsThis::add(RadioButtonDemNBFM, strDemodType);
	QWhatsThis::add(RadioButtonDemWBFM, strDemodType);

	/* Mute Audio (same as in systemevaldlg.cpp!) */
	QWhatsThis::add(CheckBoxMuteAudio,
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Save audio as wave (same as in systemevaldlg.cpp!) */
	QWhatsThis::add(CheckBoxSaveAudioWave,
		tr("<b>Save Audio as WAV:</b> Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording."));

	/* Carrier Frequency */
	QWhatsThis::add(TextFreqOffset,
		tr("<b>Carrier Frequency:</b> The (estimated) carrier frequency of the "
		"analog signal is shown. (The estimation of this parameter can be done "
		"by the Autom Frequency Acquisition which uses the estimated PSD of "
		"the input signal and applies a maximum search.)"));

	/* Phase lock loop */
	const QString strPLL =
		tr("<b>PLL:</b> The Phase-Lock-Loop (PLL) tracks the carrier of the "
		"modulated received signal. The resulting phase offset between the "
		"reference oscillator and the received carrier is displayed in "
		"a dial control. If the pointer is almost steady, the PLL is locked. "
		"If the pointer of the dial control turns quickly, the PLL is "
		"out of lock. To get the PLL locked, the frequency offset to "
		"the true carrier frequency must not exceed a few Hz.");

	QWhatsThis::add(GroupBoxPLL, strPLL);
	QWhatsThis::add(CheckBoxPLL, strPLL);
	QWhatsThis::add(PhaseDial, strPLL);
	QWhatsThis::add(TextLabelPhaseOffset, strPLL);

	/* Auto frequency acquisition */
	const QString strAutoFreqAcqu =
		tr("<b>Auto Frequency Acquisition:</b> Clicking on the "
		"input spectrum plot changes the mixing frequency for demodulation. "
		"If the Auto Frequency Acquisition is enabled, the largest peak "
		"near the curser is selected.");

	QWhatsThis::add(GroupBoxAutoFreqAcq, strAutoFreqAcqu);
	QWhatsThis::add(CheckBoxAutoFreqAcq, strAutoFreqAcqu);

	const QString strOnBoard =
		tr("<b>On Board Demod:</b> When checked, the receiver will use a hardware demodulator."
			" When clear the software demodulator will be used. Different H/W may not support"
			" one or other of the options.");
	QWhatsThis::add(CheckBoxOnBoardDemod, strOnBoard);
}


/******************************************************************************\
* AMSS controls                                                                *
\******************************************************************************/
/*
	Added by Andrew Murphy, BBC Research & Development, 2005

	Additional widgets have been added to display the AMSS service label,
	language etc. in in a similar style to that used for DRM reception.
	A display has also been added to show the status of the AMSS decoding.
	Everytime an AMSS CRC passes (for block or block 2) the 47 decoded
	bits are displayed. Note this could also include 'false' passes.

	The percentage of the current data entity group or 'SDC' is displayed
	along with which parts of the data entity group have been decoded. A
	'#' indicates that a data entity gruop segment is yet to be received
	whilst a 'c' or 'C' indicates a CRC pass for the block 2 carrying that
	particular segment.

	Added phase offset display for AMSS demodulation loop.
*/
CAMSSDlg::CAMSSDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f) :
	CAMSSDlgBase(parent, name, modal, f),
	Receiver(NDRMR),
	Settings(NSettings)
{
	CWinGeom s;
	Settings.Get("AMSS Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Init AMSS PLL phase dial control */
	PhaseDialAMSS->setMode(QwtDial::RotateNeedle);
	PhaseDialAMSS->setWrapping(true);
	PhaseDialAMSS->setReadOnly(true);
	PhaseDialAMSS->setScale(0, 360, 45); /* Degrees */
	PhaseDialAMSS->setOrigin(270);
	PhaseDialAMSS->setNeedle(new QwtDialSimpleNeedle(QwtDialSimpleNeedle::Arrow));
	PhaseDialAMSS->setFrameShadow(QwtDial::Plain);
	PhaseDialAMSS->setScaleOptions(QwtDial::ScaleTicks);

	TextAMSSServiceLabel->setText("");
	TextAMSSCountryCode->setText("");
	TextAMSSTimeDate->setText("");
	TextAMSSLanguage->setText("");
	TextAMSSServiceID->setText("");
	TextAMSSAMCarrierMode->setText("");
	TextAMSSInfo->setText("");

	ListBoxAMSSAFSList->setEnabled(FALSE);


	/* Connect controls ----------------------------------------------------- */
	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));

	/* set the progress bar style */
	ProgressBarAMSS->setStyle( new QMotifStyle() );

}

void CAMSSDlg::hideEvent(QHideEvent*)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Save window geometry data */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("AMSS Dialog", s);
}

void CAMSSDlg::showEvent(QShowEvent*)
{
	OnTimer();
	OnTimerPLLPhaseDial();

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
}

void CAMSSDlg::OnTimer()
{
	int j;

	CParameter& Parameters = *Receiver.GetParameters();
	Parameters.Lock();

	/* Show label if available */
	if ((Parameters.Service[0].IsActive()) && (Parameters.Service[0].strLabel != ""))
	{
		/* Service label (UTF-8 encoded string -> convert) */
		TextAMSSServiceLabel->setText(QString().fromUtf8(QCString(
			Parameters.Service[0].strLabel.c_str())));
	}
	else
		TextAMSSServiceLabel->setText(tr(""));

	/* Country code */
	const string strCntryCode = Parameters.Service[0].strCountryCode; /* must be of 2 lowercase chars */

	if ((Parameters.Service[0].IsActive()) && (!strCntryCode.empty()) && (strCntryCode != "--"))
	{
		TextAMSSCountryCode->
			setText(QString(GetISOCountryName(strCntryCode).c_str()));
	}
	else
		TextAMSSCountryCode->setText("");

	/* SDC Language code */

	if (Parameters.Service[0].IsActive())
	{
		const string strLangCode = Parameters.Service[0].strLanguageCode; /* must be of 3 lowercase chars */

		if ((!strLangCode.empty()) && (strLangCode != "---"))
			 TextAMSSLanguage->
				setText(QString(GetISOLanguageName(strLangCode).c_str()));
		else
			TextAMSSLanguage->setText(QString(strTableLanguageCode[Parameters.Service[0].iLanguage].c_str()));
	}
	else
		TextAMSSLanguage->setText("");

	/* Time, date */
	if ((Parameters.iUTCHour == 0) &&
		(Parameters.iUTCMin == 0) &&
		(Parameters.iDay == 0) &&
		(Parameters.iMonth == 0) &&
		(Parameters.iYear == 0))
	{
		/* No time service available */
		TextAMSSTimeDate->setText("");
	}
	else
	{
		QDateTime DateTime;
		DateTime.setDate(QDate(Parameters.iYear, Parameters.iMonth, Parameters.iDay));
		DateTime.setTime(QTime(Parameters.iUTCHour, Parameters.iUTCMin));

		TextAMSSTimeDate->setText(DateTime.toString());
	}

	/* Get number of alternative services */
	const size_t iNumAltServices = Parameters.AltFreqSign.vecOtherServices.size();

	if (iNumAltServices != 0)
	{
		ListBoxAMSSAFSList->insertItem(QString().setNum((long) iNumAltServices), 10);

		ListBoxAMSSAFSList->clear();
		ListBoxAMSSAFSList->setEnabled(TRUE);

		QString freqEntry;

		for (size_t i = 0; i < iNumAltServices; i++)
		{
			switch (Parameters.AltFreqSign.vecOtherServices[i].iSystemID)
			{
			case 0:
				freqEntry = "DRM:";
				break;

			case 1:
			case 2:
				freqEntry = "AM:   ";
				break;

			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				freqEntry = "FM:   ";
				break;

			default:
				freqEntry = "";
				break;
			}

			const int iNumAltFreqs = Parameters.AltFreqSign.vecOtherServices[i].veciFrequencies.size();

			const int iSystemID = Parameters.AltFreqSign.vecOtherServices[i].iSystemID;

			switch (iSystemID)
			{
			case 0:
			case 1:
			case 2:
				/* AM or DRM, freq in kHz */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						veciFrequencies[j], 10);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " kHz";


				if (iSystemID == 0 || iSystemID == 1)
				{
					freqEntry += " ID:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
				}
				break;

			case 3:
			case 4:
			case 5:
				/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (87.5 + 0.1 * Receiver.
						GetParameters()->AltFreqSign.
						vecOtherServices[i].veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 3)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
				}

				if (iSystemID == 4)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
				}
				break;

			case 6:
			case 7:
			case 8:
				/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (76.0 + 0.1 * Receiver.
						GetParameters()->AltFreqSign.
						vecOtherServices[i].veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 6)
				{
					freqEntry += " ECC+PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
				}

				if (iSystemID == 7)
				{
					freqEntry += " PI:";
					freqEntry +=
						QString().setNum((long) Parameters.
						AltFreqSign.vecOtherServices[i].
						iServiceID, 16).upper();
				}
				break;

			default:
				freqEntry = "DAB";
				break;
			}

			if (Parameters.AltFreqSign.
				vecOtherServices[i].bSameService)
			{
				freqEntry += " (same service)";
			}
			else
			{
				freqEntry += " (alt service)";
			}

			ListBoxAMSSAFSList->insertItem(freqEntry, 0);
		}
	}
	else
	{
		ListBoxAMSSAFSList->clear();
		ListBoxAMSSAFSList->setEnabled(FALSE);
	}

	TextAMSSServiceID->setText("");
	TextAMSSAMCarrierMode->setText("");

	if (Receiver.GetAMSSDecode()->GetLockStatus() == CAMSSDecode::NO_SYNC
	|| Parameters.Service[0].iServiceID == SERV_ID_NOT_USED
	)
	{
		TextAMSSInfo->setText(tr("No AMSS detected"));
	}
	else
	{
		TextAMSSInfo->setText(tr("Awaiting AMSS data..."));

		/* Display 'block 1' info */
		if (Receiver.GetAMSSDecode()->GetBlock1Status())
		{
			TextAMSSInfo->setText("");

			TextAMSSLanguage->setText(QString(strTableLanguageCode[Receiver.
				GetParameters()->Service[0].iLanguage].c_str()));

			TextAMSSServiceID->setText("ID:" + QString().setNum(
				(long) Parameters.Service[0].iServiceID, 16).upper());

			TextAMSSAMCarrierMode->setText(QString(strTableAMSSCarrierMode[Receiver.
				GetParameters()->iAMSSCarrierMode].c_str()));
		}
	}

	TextDataEntityGroupStatus->setText(Receiver.GetAMSSDecode()->
		GetDataEntityGroupStatus());

	TextCurrentBlock->setText(QString().setNum(Receiver.GetAMSSDecode()->
		GetCurrentBlock(), 10));

	TextBlockBits->setText(Receiver.GetAMSSDecode()->GetCurrentBlockBits());

	ProgressBarAMSS->setProgress(Receiver.GetAMSSDecode()->
		GetPercentageDataEntityGroupComplete());

	Parameters.Unlock();
}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (Receiver.GetAMSSPhaseDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
	{
		/* Set current PLL phase (convert radiant in degree) */
		PhaseDialAMSS->setValue(rCurPLLPhase * (CReal) 360.0 / (2 * crPi));

		/* Check if control is enabled */
		if (!PhaseDialAMSS->isEnabled())
			PhaseDialAMSS->setEnabled(true);
	}
	else
	{
		/* Reset dial */
		PhaseDialAMSS->setValue((CReal) 0.0);

		/* Check if control is disabled */
		if (PhaseDialAMSS->isEnabled())
			PhaseDialAMSS->setEnabled(false);
	}
}

void CAMSSDlg::AddWhatsThisHelp()
{
	// TODO
}
