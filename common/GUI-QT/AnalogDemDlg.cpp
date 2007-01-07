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


/* Implementation *************************************************************/
AnalogDemDlg::AnalogDemDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) : pDRMRec(pNDRMR),
	AnalogDemDlgBase(parent, name, modal, f),
	AMSSDlg(pNDRMR, parent, name, modal, f)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomAnalogDemDlg.iXPos,
		pDRMRec->GeomAnalogDemDlg.iYPos,
		pDRMRec->GeomAnalogDemDlg.iWSize,
		pDRMRec->GeomAnalogDemDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#else /* Under Linux only restore the size */
	resize(pDRMRec->GeomAnalogDemDlg.iWSize,
		pDRMRec->GeomAnalogDemDlg.iHSize);
#endif


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
		new CSoundCardSelMenu(pDRMRec->GetSoundInInterface(), pDRMRec->GetSoundOutInterface(), this));
	pSettingsMenu->insertItem(tr("&DRM (digital)"), this,
		SIGNAL(SwitchToDRM()), CTRL+Key_D);
	pSettingsMenu->insertItem(tr("New &AM Acquisition"), this,
		SLOT(OnNewAMAcquisition()), CTRL+Key_A);


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
	MainPlot->SetRecObj(pDRMRec);
	MainPlot->SetPlotStyle(pDRMRec->iMainPlotColorStyle);
	MainPlot->setMargin(1);
	MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);

	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot,
		tr("Click on the plot to set the demodulation frequency"));

	/* Set current AM demodulation type */
	pDRMRec->GetAMDemod()->SetDemodType(pDRMRec->AMDemodType);

	/* Load user's saved filter bandwidth for the demodulation type. */
	switch (pDRMRec->GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM:
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwAM);
		break;

	case CAMDemodulation::DT_LSB:
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwLSB);
		break;

	case CAMDemodulation::DT_USB:
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwUSB);
		break;

	case CAMDemodulation::DT_CW:
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwCW);
		break;

	case CAMDemodulation::DT_FM:
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwFM);
		break;
	}

	/* Init slider control for bandwidth setting */
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
		this, SIGNAL(SwitchToDRM()));
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

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
}

void AnalogDemDlg::showEvent(QShowEvent* pEvent)
{
	OnTimer();
	OnTimerPLLPhaseDial();

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);

	/* Open AMSS window */
	if (pDRMRec->GeomAMSSDlg.bVisible == TRUE)
		AMSSDlg.show();
	else
		AMSSDlg.hide();
	
	UpdateControls();
}

void AnalogDemDlg::hideEvent(QHideEvent* pEvent)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	pDRMRec->GeomAMSSDlg.bVisible = AMSSDlg.isVisible();

	/* Close AMSS window */
	AMSSDlg.hide();

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomAnalogDemDlg.iXPos = WinGeom.x();
	pDRMRec->GeomAnalogDemDlg.iYPos = WinGeom.y();
	pDRMRec->GeomAnalogDemDlg.iHSize = WinGeom.height();
	pDRMRec->GeomAnalogDemDlg.iWSize = WinGeom.width();
}

void AnalogDemDlg::closeEvent(QCloseEvent* pEvent)
{
	pDRMRec->GeomAMSSDlg.bVisible = AMSSDlg.isVisible();

	/* Close AMSS window */
	AMSSDlg.hide();

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomAnalogDemDlg.iXPos = WinGeom.x();
	pDRMRec->GeomAnalogDemDlg.iYPos = WinGeom.y();
	pDRMRec->GeomAnalogDemDlg.iHSize = WinGeom.height();
	pDRMRec->GeomAnalogDemDlg.iWSize = WinGeom.width();

	/* Tell the invisible main window to close, too */
	emit Closed();
}

void AnalogDemDlg::UpdateControls()
{
	/* Set demodulation type */
	switch (pDRMRec->GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM:
		if (!RadioButtonDemAM->isChecked())
			RadioButtonDemAM->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_LSB:
		if (!RadioButtonDemLSB->isChecked())
			RadioButtonDemLSB->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_USB:
		if (!RadioButtonDemUSB->isChecked())
			RadioButtonDemUSB->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_CW:
		if (!RadioButtonDemCW->isChecked())
			RadioButtonDemCW->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_FM:
		if (!RadioButtonDemFM->isChecked())
			RadioButtonDemFM->setChecked(TRUE);
		break;
	}

	/* Set AGC type */
	switch (pDRMRec->GetAMDemod()->GetAGCType())
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
	switch (pDRMRec->GetAMDemod()->GetNoiRedType())
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
	SliderBandwidth->setValue(pDRMRec->GetAMDemod()->GetFilterBW());
	TextLabelBandWidth->setText(QString().setNum(
		pDRMRec->GetAMDemod()->GetFilterBW()) +	tr(" Hz"));

	/* Update check boxes */
	CheckBoxMuteAudio->setChecked(pDRMRec->GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(pDRMRec->GetWriteData()->GetIsWriteWaveFile());

	CheckBoxAutoFreqAcq->
		setChecked(pDRMRec->GetAMDemod()->AutoFreqAcqEnabled());

	CheckBoxPLL->setChecked(pDRMRec->GetAMDemod()->PLLEnabled());
}

void AnalogDemDlg::UpdatePlotsStyle()
{
	/* Update main plot window */
	MainPlot->SetPlotStyle(pDRMRec->iMainPlotColorStyle);
}

void AnalogDemDlg::OnTimer()
{
	/* Carrier frequency of AM signal */
	TextFreqOffset->setText(tr("Carrier<br>Frequency:<b><br>") +
		QString().setNum(pDRMRec->GetAMDemod()->GetCurMixFreqOffs(), 'f', 2) +
		" Hz</b>");
}

void AnalogDemDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (pDRMRec->GetAMDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
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
	switch (iID)
	{
	case 0:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwAM);
		break;

	case 1:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_LSB);
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwLSB);
		break;

	case 2:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_USB);
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwUSB);
		break;

	case 3:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_CW);
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwCW);
		break;

	case 4:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_FM);
		pDRMRec->GetAMDemod()->SetFilterBW(pDRMRec->iBwFM);
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
		pDRMRec->GetAMDemod()->SetAGCType(CAGC::AT_NO_AGC);
		break;

	case 1:
		pDRMRec->GetAMDemod()->SetAGCType(CAGC::AT_SLOW);
		break;

	case 2:
		pDRMRec->GetAMDemod()->SetAGCType(CAGC::AT_MEDIUM);
		break;

	case 3:
		pDRMRec->GetAMDemod()->SetAGCType(CAGC::AT_FAST);
		break;
	}
}

void AnalogDemDlg::OnRadioNoiRed(int iID)
{
	switch (iID)
	{
	case 0:
		pDRMRec->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_OFF);
		break;

	case 1:
		pDRMRec->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_LOW);
		break;

	case 2:
		pDRMRec->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_MEDIUM);
		break;

	case 3:
		pDRMRec->GetAMDemod()->SetNoiRedType(CAMDemodulation::NR_HIGH);
		break;
	}
}

void AnalogDemDlg::OnSliderBWChange(int value)
{
	/* Set new filter in processing module */
	pDRMRec->GetAMDemod()->SetFilterBW(value);
	TextLabelBandWidth->setText(QString().setNum(value) + tr(" Hz"));

	/* Store filter bandwidth for this demodulation type */
	switch (pDRMRec->GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM:
		pDRMRec->iBwAM = value;
		break;

	case CAMDemodulation::DT_LSB:
		pDRMRec->iBwLSB = value;
		break;

	case CAMDemodulation::DT_USB:
		pDRMRec->iBwUSB = value;
		break;

	case CAMDemodulation::DT_CW:
		pDRMRec->iBwCW = value;
		break;

	case CAMDemodulation::DT_FM:
		pDRMRec->iBwFM = value;
		break;
	}

	/* Update chart */
	MainPlot->Update();
}

void AnalogDemDlg::OnCheckAutoFreqAcq()
{
	/* Set parameter in working thread module */
	pDRMRec->GetAMDemod()->EnableAutoFreqAcq(CheckBoxAutoFreqAcq->isChecked());
}

void AnalogDemDlg::OnCheckPLL()
{
	/* Set parameter in working thread module */
	pDRMRec->GetAMDemod()->EnablePLL(CheckBoxPLL->isChecked());
}

void AnalogDemDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	pDRMRec->GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
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
			pDRMRec->GetWriteData()->
				StartWriteWaveFile(strFileName.latin1());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(FALSE);
		}
	}
	else
		pDRMRec->GetWriteData()->StopWriteWaveFile();
}

void AnalogDemDlg::OnChartxAxisValSet(double dVal)
{
	/* Set new frequency in receiver module */
	pDRMRec->SetAMDemodAcq(dVal);

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
	QWhatsThis::add(RadioButtonDemFM, strDemodType);

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
CAMSSDlg::CAMSSDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) : pDRMRec(pNDRMR),
	CAMSSDlgBase(parent, name, modal, f)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomAMSSDlg.iXPos,
		pDRMRec->GeomAMSSDlg.iYPos,
		pDRMRec->GeomAMSSDlg.iWSize,
		pDRMRec->GeomAMSSDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#else /* Under Linux only restore the size */
	resize(pDRMRec->GeomAMSSDlg.iWSize,
		pDRMRec->GeomAMSSDlg.iHSize);
#endif

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
	TextAMSSInfo->setText("");

	ListBoxAMSSAFSList->setEnabled(FALSE);


	/* Connect controls ----------------------------------------------------- */
	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerPLLPhaseDial, SIGNAL(timeout()),
		this, SLOT(OnTimerPLLPhaseDial()));

	/* Activate real-time timers */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	TimerPLLPhaseDial.start(PLL_PHASE_DIAL_UPDATE_TIME);
	/* set the progress bar style */
	ProgressBarAMSS->setStyle( new QMotifStyle() );

}

void CAMSSDlg::hideEvent(QHideEvent* pEvent)
{
	/* stop real-time timers */
	Timer.stop();
	TimerPLLPhaseDial.stop();

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomAMSSDlg.iXPos = WinGeom.x();
	pDRMRec->GeomAMSSDlg.iYPos = WinGeom.y();
	pDRMRec->GeomAMSSDlg.iHSize = WinGeom.height();
	pDRMRec->GeomAMSSDlg.iWSize = WinGeom.width();
}

void CAMSSDlg::showEvent(QShowEvent* pEvent)
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

	/* Show label if available */
	if ((pDRMRec->GetParameters()->Service[0].IsActive()) &&
		(pDRMRec->GetParameters()->Service[0].strLabel != ""))
	{
		/* Service label (UTF-8 encoded string -> convert) */
		TextAMSSServiceLabel->setText(QString().fromUtf8(QCString(
			pDRMRec->GetParameters()->Service[0].
			strLabel.c_str())));
	}
	else
		TextAMSSServiceLabel->setText(tr(""));

	/* Country code */
	const string strCntryCode = pDRMRec->GetParameters()->
		Service[0].strCountryCode; /* must be of 2 lowercase chars */

	if ((pDRMRec->GetParameters()->Service[0].IsActive()) &&
	   (!strCntryCode.empty()))
	{
		TextAMSSCountryCode->
			setText(QString(GetName(strCntryCode).c_str()));
	}
	else
		TextAMSSCountryCode->setText("");

	/* Time, date */
	if ((pDRMRec->GetParameters()->iUTCHour == 0) &&
		(pDRMRec->GetParameters()->iUTCMin == 0) &&
		(pDRMRec->GetParameters()->iDay == 0) &&
		(pDRMRec->GetParameters()->iMonth == 0) &&
		(pDRMRec->GetParameters()->iYear == 0))
	{
		/* No time service available */
		TextAMSSTimeDate->setText("");
	}
	else
	{
		QDateTime DateTime;
		DateTime.setDate(QDate(pDRMRec->GetParameters()->iYear,
			pDRMRec->GetParameters()->iMonth,
			pDRMRec->GetParameters()->iDay));
		DateTime.setTime(QTime(pDRMRec->GetParameters()->iUTCHour,
			pDRMRec->GetParameters()->iUTCMin));

		TextAMSSTimeDate->setText(DateTime.toString());
	}

	/* Get number of alternative services */
	const int iNumAltServices = pDRMRec->GetParameters()->
		AltFreqOtherServicesSign.vecAltFreqOtherServices.Size();

	if (iNumAltServices != 0)
	{
		ListBoxAMSSAFSList->
			insertItem(QString().setNum((long) iNumAltServices), 10);

		ListBoxAMSSAFSList->clear();
		ListBoxAMSSAFSList->setEnabled(TRUE);

		QString freqEntry;

		for (int i = 0; i < iNumAltServices; i++)
		{
			switch (pDRMRec->GetParameters()->AltFreqOtherServicesSign.
				vecAltFreqOtherServices[i].iSystemID)
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

			const int iNumAltFreqs =
				pDRMRec->GetParameters()->AltFreqOtherServicesSign.
				vecAltFreqOtherServices[i].veciFrequencies.Size();

			const int iSystemID = pDRMRec->GetParameters()->
				AltFreqOtherServicesSign.vecAltFreqOtherServices[i].iSystemID;

			switch (iSystemID)
			{
			case 0:
			case 1:
			case 2:
				/* AM or DRM, freq in kHz */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						veciFrequencies[j], 10);
							
					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " kHz";


				if (iSystemID == 0 || iSystemID == 1)
				{
					freqEntry += " ID:0x";
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						iOtherServiceID, 16);
				}
				break;

			case 3:
			case 4:
			case 5:
				/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (87.5 + 0.1 * pDRMRec->
						GetParameters()->AltFreqOtherServicesSign.
						vecAltFreqOtherServices[i].veciFrequencies[j]), 'f', 1);

					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 3)
				{
					freqEntry += " ECC+PI:0x";
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						iOtherServiceID, 16);
				}

				if (iSystemID == 4)
				{
					freqEntry += " PI:0x";
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						iOtherServiceID, 16);
				}
				break;

			case 6:
			case 7:
			case 8:
				/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
				for (j = 0; j < iNumAltFreqs; j++)
				{
					freqEntry +=
						QString().setNum((float) (76.0 + 0.1 * pDRMRec->
						GetParameters()->AltFreqOtherServicesSign.
						vecAltFreqOtherServices[i].veciFrequencies[j]), 'f', 1);
							
					if (j != iNumAltFreqs-1)
						freqEntry += ",";
				}
				freqEntry += " MHz";

				if (iSystemID == 6)
				{
					freqEntry += " ECC+PI:0x";
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						iOtherServiceID, 16);
				}

				if (iSystemID == 7)
				{
					freqEntry += " PI:0x";
					freqEntry +=
						QString().setNum((long) pDRMRec->GetParameters()->
						AltFreqOtherServicesSign.vecAltFreqOtherServices[i].
						iOtherServiceID, 16);
				}
				break;

			default:
				freqEntry = "DAB";
				break;
			}

			if (pDRMRec->GetParameters()->AltFreqOtherServicesSign.
				vecAltFreqOtherServices[i].bSameService)
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

	if (pDRMRec->GetAMSSDecode()->GetLockStatus() == CAMSSDecode::NO_SYNC)
	{
		TextAMSSInfo->setText(tr("No AMSS detected"));
	}
	else 
	{
		TextAMSSInfo->setText(tr("Awaiting AMSS data..."));

		/* Display 'block 1' info */
		if (pDRMRec->GetAMSSDecode()->GetBlock1Status())
		{
			QString strServIDCarrierMode = strTableLanguageCode[pDRMRec->
				GetParameters()->Service[0].iLanguage].c_str();

			strServIDCarrierMode += " / ID:0x";
			strServIDCarrierMode += QString().setNum(
				(long) pDRMRec->GetParameters()->Service[0].iServiceID, 16);

			strServIDCarrierMode += " / ";
			strServIDCarrierMode += strTableAMSSCarrierMode[pDRMRec->
				GetParameters()->iAMSSCarrierMode].c_str();

			TextAMSSInfo->setText(strServIDCarrierMode);
		}
	}

	TextDataEntityGroupStatus->setText(pDRMRec->GetAMSSDecode()->
		GetDataEntityGroupStatus());

	TextCurrentBlock->setText(QString().setNum(pDRMRec->GetAMSSDecode()->
		GetCurrentBlock(), 10));

	TextBlockBits->setText(pDRMRec->GetAMSSDecode()->GetCurrentBlockBits());

	ProgressBarAMSS->setProgress(pDRMRec->GetAMSSDecode()->
		GetPercentageDataEntityGroupComplete());
}

void CAMSSDlg::OnTimerPLLPhaseDial()
{
	CReal rCurPLLPhase;

	if (pDRMRec->GetAMSSPhaseDemod()->GetPLLPhase(rCurPLLPhase) == TRUE)
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
