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


AnalogDemDlg::AnalogDemDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) : pDRMRec(pNDRMR),
	AnalogDemDlgBase(parent, name, modal, f)
{
#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomAnalogDemDlg.iXPos,
		pDRMRec->GeomAnalogDemDlg.iYPos,
		pDRMRec->GeomAnalogDemDlg.iWSize,
		pDRMRec->GeomAnalogDemDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#endif

	/* Init main plot */
	MainPlot->SetRecObj(pDRMRec);
	MainPlot->SetPlotStyle(pDRMRec->iMainPlotColorStyle);
	MainPlot->setMargin(1);
	MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD_ANALOG);

	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot,
		tr("Click on the plot to set the demod. frequency"));

	/* Set default settings -> AM: 10 kHz; SSB: 5 kHz; medium AGC */
	iBwLSB = 5000; /* Hz */
	iBwUSB = 5000; /* Hz */
	iBwFM = 6000; /* Hz */
	iBwAM = 10000; /* Hz */
	pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);
	pDRMRec->GetAMDemod()->SetFilterBW(iBwAM);
	pDRMRec->GetAMDemod()->SetAGCType(CAMDemodulation::AT_MEDIUM);

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

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));

	/* Activte real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

AnalogDemDlg::~AnalogDemDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomAnalogDemDlg.iXPos = WinGeom.x();
	pDRMRec->GeomAnalogDemDlg.iYPos = WinGeom.y();
	pDRMRec->GeomAnalogDemDlg.iHSize = WinGeom.height();
	pDRMRec->GeomAnalogDemDlg.iWSize = WinGeom.width();
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

	case CAMDemodulation::DT_FM:
		if (!RadioButtonDemFM->isChecked())
			RadioButtonDemFM->setChecked(TRUE);
		break;
	}

	/* Set AGC type */
	switch (pDRMRec->GetAMDemod()->GetAGCType())
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
}

void AnalogDemDlg::showEvent(QShowEvent* pEvent)
{
	/* Update controls */
	UpdateControls();
}

void AnalogDemDlg::OnTimer()
{
	/* Carrier frequency of AM signal */
	TextFreqOffset->setText(tr("Carrier Frequency: ") + QString().setNum(
		pDRMRec->GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
}

void AnalogDemDlg::OnRadioDemodulation(int iID)
{
	switch (iID)
	{
	case 0:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);
		pDRMRec->GetAMDemod()->SetFilterBW(iBwAM);
		break;

	case 1:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_LSB);
		pDRMRec->GetAMDemod()->SetFilterBW(iBwLSB);
		break;

	case 2:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_USB);
		pDRMRec->GetAMDemod()->SetFilterBW(iBwUSB);
		break;

	case 3:
		pDRMRec->GetAMDemod()->SetDemodType(CAMDemodulation::DT_FM);
		pDRMRec->GetAMDemod()->SetFilterBW(iBwFM);
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
		pDRMRec->GetAMDemod()->SetAGCType(CAMDemodulation::AT_NO_AGC);
		break;

	case 1:
		pDRMRec->GetAMDemod()->SetAGCType(CAMDemodulation::AT_SLOW);
		break;

	case 2:
		pDRMRec->GetAMDemod()->SetAGCType(CAMDemodulation::AT_MEDIUM);
		break;

	case 3:
		pDRMRec->GetAMDemod()->SetAGCType(CAMDemodulation::AT_FAST);
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
	MainPlot->Update();
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
