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

#include "AnalogDemDlg.h"


AnalogDemDlg::AnalogDemDlg(QWidget* parent, const char* name, bool modal, WFlags f)
	: AnalogDemDlgBase(parent, name, modal, f)
{
#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(DRMReceiver.GeomAnalogDemDlg.iXPos,
		DRMReceiver.GeomAnalogDemDlg.iYPos,
		DRMReceiver.GeomAnalogDemDlg.iWSize,
		DRMReceiver.GeomAnalogDemDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#endif

	/* Init main plot */
	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot,
		tr("Click on the plot to set the demod. frequency"));
	MainPlot->SetPlotStyle(DRMReceiver.iMainPlotColorStyle);
	MainPlot->setMargin(1);

	/* Set default settings -> AM: 10 kHz; SSB: 5 kHz; medium AGC */
	iBwLSB = 5000; /* Hz */
	iBwUSB = 5000; /* Hz */
	iBwFM = 6000; /* Hz */
	iBwAM = 10000; /* Hz */
	DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);
	DRMReceiver.GetAMDemod()->SetFilterBW(iBwAM);
	DRMReceiver.GetAMDemod()->SetAGCType(CAMDemodulation::AT_MEDIUM);

	/* Init slider control for bandwidth setting */
	SliderBandwidth->setRange(0, SOUNDCRD_SAMPLE_RATE / 2);
	SliderBandwidth->setTickmarks(QSlider::Both);
	SliderBandwidth->setTickInterval(1000); /* Each kHz a tick */
	SliderBandwidth->setPageStep(1000); /* Hz */

	/* Update controls */
	UpdateControls();


	/* Connect controls ----------------------------------------------------- */
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
	connect(MainPlot, SIGNAL(xAxisValSet(double)),
		this, SLOT(OnChartxAxisValSet(double)));
	
	/* Button groups */
	connect(ButtonGroupDemodulation, SIGNAL(clicked(int)),
		this, SLOT(OnRadioDemodulation(int)));
	connect(ButtonGroupBW, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBW(int)));
	connect(ButtonGroupAGC, SIGNAL(clicked(int)),
		this, SLOT(OnRadioAGC(int)));

	/* Slider */
	connect(SliderBandwidth, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderBWChange(int)));

	/* Check boxes */
	connect(CheckBoxMuteAudio, SIGNAL(clicked()),
		this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()),
		this, SLOT(OnCheckSaveAudioWAV()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerChart, SIGNAL(timeout()),
		this, SLOT(OnTimerChart()));

	/* Activte real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimerChart();
}

AnalogDemDlg::~AnalogDemDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	DRMReceiver.GeomAnalogDemDlg.iXPos = WinGeom.x();
	DRMReceiver.GeomAnalogDemDlg.iYPos = WinGeom.y();
	DRMReceiver.GeomAnalogDemDlg.iHSize = WinGeom.height();
	DRMReceiver.GeomAnalogDemDlg.iWSize = WinGeom.width();
}

void AnalogDemDlg::UpdateControls()
{
	/* Set demodulation type */
	switch (DRMReceiver.GetAMDemod()->GetDemodType())
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

	case CAMDemodulation::DT_FM:
		if (!RadioButtonDemFM->isChecked())
			RadioButtonDemFM->setChecked(TRUE);
		break;
	}

	/* Set AGC type */
	switch (DRMReceiver.GetAMDemod()->GetAGCType())
	{
	case CAMDemodulation::AT_NO_AGC:
		if (!RadioButtonAGCOff->isChecked())
			RadioButtonAGCOff->setChecked(TRUE);
		break;

	case CAMDemodulation::AT_SLOW:
		if (!RadioButtonAGCSlow->isChecked())
			RadioButtonAGCSlow->setChecked(TRUE);
		break;

	case CAMDemodulation::AT_MEDIUM:
		if (!RadioButtonAGCMed->isChecked())
			RadioButtonAGCMed->setChecked(TRUE);
		break;

	case CAMDemodulation::AT_FAST:
		if (!RadioButtonAGCFast->isChecked())
			RadioButtonAGCFast->setChecked(TRUE);
		break;
	}

	/* Set filter bandwidth */
	SliderBandwidth->setValue(DRMReceiver.GetAMDemod()->GetFilterBW());
	TextLabelBandWidth->setText(tr("Bandwidth: ") +
		QString().setNum(DRMReceiver.GetAMDemod()->GetFilterBW()) +
		tr(" Hz"));

	/* Update mute audio switch and write wave file */
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());
}

void AnalogDemDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timers when window is shown */
	TimerChart.start(GUI_CONTROL_UPDATE_TIME_FAST);

	/* Update window */
	OnTimerChart();

	/* Update controls */
	UpdateControls();
}

void AnalogDemDlg::hideEvent(QHideEvent* pEvent)
{
	/* Deactivate real-time timers when window is hide to save CPU power */
	TimerChart.stop();
}

void AnalogDemDlg::OnTimerChart()
{
	CVector<_REAL>	vecrData;
	CVector<_REAL>	vecrScale;

	/* Get data from module */
	DRMReceiver.GetReceiver()->GetInputPSD(vecrData, vecrScale);

	/* Prepare graph and set data */
	CReal rCenterFreq, rBW;
	DRMReceiver.GetAMDemod()->GetBWParameters(rCenterFreq, rBW);

	MainPlot->SetInpPSD(vecrData, vecrScale,
		DRMReceiver.GetParameters()->GetDCFrequency(), rCenterFreq, rBW);
}

void AnalogDemDlg::OnTimer()
{
	/* Carrier frequency of AM signal */
	TextFreqOffset->setText(tr("Carrier Frequency: ") + QString().setNum(
		DRMReceiver.GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
}

void AnalogDemDlg::OnRadioDemodulation(int iID)
{
	switch (iID)
	{
	case 0:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);
		DRMReceiver.GetAMDemod()->SetFilterBW(iBwAM);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_LSB);
		DRMReceiver.GetAMDemod()->SetFilterBW(iBwLSB);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_USB);
		DRMReceiver.GetAMDemod()->SetFilterBW(iBwUSB);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_FM);
		DRMReceiver.GetAMDemod()->SetFilterBW(iBwFM);
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
		DRMReceiver.GetAMDemod()->SetAGCType(CAMDemodulation::AT_NO_AGC);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetAGCType(CAMDemodulation::AT_SLOW);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetAGCType(CAMDemodulation::AT_MEDIUM);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetAGCType(CAMDemodulation::AT_FAST);
		break;
	}
}

void AnalogDemDlg::OnSliderBWChange(int value)
{
	/* Set new filter in processing module */
	DRMReceiver.GetAMDemod()->SetFilterBW(value);
	TextLabelBandWidth->setText(tr("Bandwidth: ") +
		QString().setNum(value) + tr(" Hz"));

	/* Store filter bandwidth for this demodulation type */
	switch (DRMReceiver.GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM:
		iBwAM = value;
		break;

	case CAMDemodulation::DT_LSB:
		iBwLSB = value;
		break;

	case CAMDemodulation::DT_USB:
		iBwUSB = value;
		break;

	case CAMDemodulation::DT_FM:
		iBwFM = value;
		break;
	}

	/* Update chart */
	OnTimerChart();
}

void AnalogDemDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
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
			DRMReceiver.GetWriteData()->
				StartWriteWaveFile(strFileName.latin1());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(FALSE);
		}
	}
	else
		DRMReceiver.GetWriteData()->StopWriteWaveFile();
}

void AnalogDemDlg::OnChartxAxisValSet(double dVal)
{
	/* Set new frequency in receiver module */
	DRMReceiver.SetAMDemodAcq(dVal);
}
