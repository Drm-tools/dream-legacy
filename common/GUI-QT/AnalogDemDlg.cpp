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

	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot, "Click on the plot to set the demod. frequency");
	MainPlot->setMargin(1);

	/* Update controls */
	UpdateControls();


	/* Connect controls ----------------------------------------------------- */
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
	
	/* Button groups */
	connect(ButtonGroupDemodulation, SIGNAL(clicked(int)),
		this, SLOT(OnRadioDemodulation(int)));
	connect(ButtonGroupBW, SIGNAL(clicked(int)),
		this, SLOT(OnRadioBW(int)));
	connect(ButtonGroupAGC, SIGNAL(clicked(int)),
		this, SLOT(OnRadioAGC(int)));

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
	switch (DRMReceiver.GetAMDemod()->GetFilterBW())
	{
	case CAMDemodulation::BW_1KHZ:
		if (!RadioButtonBW1->isChecked())
			RadioButtonBW1->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_2KHZ:
		if (!RadioButtonBW2->isChecked())
			RadioButtonBW2->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_3KHZ:
		if (!RadioButtonBW3->isChecked())
			RadioButtonBW3->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_4KHZ:
		if (!RadioButtonBW4->isChecked())
			RadioButtonBW4->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_5KHZ:
		if (!RadioButtonBW5->isChecked())
			RadioButtonBW5->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_6KHZ:
		if (!RadioButtonBW6->isChecked())
			RadioButtonBW6->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_7KHZ:
		if (!RadioButtonBW7->isChecked())
			RadioButtonBW7->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_8KHZ:
		if (!RadioButtonBW8->isChecked())
			RadioButtonBW8->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_9KHZ:
		if (!RadioButtonBW9->isChecked())
			RadioButtonBW9->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_10KHZ:
		if (!RadioButtonBW10->isChecked())
			RadioButtonBW10->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_11KHZ:
		if (!RadioButtonBW11->isChecked())
			RadioButtonBW11->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_12KHZ:
		if (!RadioButtonBW12->isChecked())
			RadioButtonBW12->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_13KHZ:
		if (!RadioButtonBW13->isChecked())
			RadioButtonBW13->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_14KHZ:
		if (!RadioButtonBW14->isChecked())
			RadioButtonBW14->setChecked(TRUE);
		break;

	case CAMDemodulation::BW_15KHZ:
		if (!RadioButtonBW15->isChecked())
			RadioButtonBW15->setChecked(TRUE);
		break;
	}

	/* Update mute audio switch and write wave file */
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());
}

void AnalogDemDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timers when window is shown */
	TimerChart.start(GUI_CONTROL_UPDATE_TIME);

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
	DRMReceiver.GetReceiver()->GetInputSpec(vecrData, vecrScale);

	/* Prepare graph and set data */
	CReal rCenterFreq, rBW;
	DRMReceiver.GetAMDemod()->GetBWParameters(rCenterFreq, rBW);

	MainPlot->SetInpSpec(vecrData, vecrScale,
		DRMReceiver.GetParameters()->GetDCFrequency(), rCenterFreq, rBW);
}

void AnalogDemDlg::OnTimer()
{
	/* Carrier frequency of AM signal */
	TextFreqOffset->setText("Carrier Frequency: " + QString().setNum(
		DRMReceiver.GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
}

void AnalogDemDlg::OnRadioDemodulation(int iID)
{
	switch (iID)
	{
	case 0:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM);

		/* Set to default filter -> 10 kHz */
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_10KHZ);
		RadioButtonBW10->setChecked(TRUE);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_LSB);

		/* Set to default filter -> 5 kHz */
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_5KHZ);
		RadioButtonBW5->setChecked(TRUE);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_USB);

		/* Set to default filter -> 5 kHz */
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_5KHZ);
		RadioButtonBW5->setChecked(TRUE);
		break;
	}
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

void AnalogDemDlg::OnRadioBW(int iID)
{
	switch (iID)
	{
	case 0:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_1KHZ);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_2KHZ);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_3KHZ);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_4KHZ);
		break;

	case 4:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_5KHZ);
		break;

	case 5:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_6KHZ);
		break;

	case 6:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_7KHZ);
		break;

	case 7:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_8KHZ);
		break;

	case 8:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_9KHZ);
		break;

	case 9:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_10KHZ);
		break;

	case 10:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_11KHZ);
		break;

	case 11:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_12KHZ);
		break;

	case 12:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_13KHZ);
		break;

	case 13:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_14KHZ);
		break;

	case 14:
		DRMReceiver.GetAMDemod()->SetFilterBW(CAMDemodulation::BW_15KHZ);
		break;
	}
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
