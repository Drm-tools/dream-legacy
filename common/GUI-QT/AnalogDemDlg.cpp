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
	/* Add tool tip to show the user the possibility of choosing the AM IF */
	QToolTip::add(MainPlot, "Click on the plot to set the demod. frequency");
	MainPlot->setMargin(1);


	/* Set default demodulation type */
	switch (DRMReceiver.GetAMDemod()->GetDemodType())
	{
	case CAMDemodulation::DT_AM_10:
		RadioButtonDemAM10->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_AM_5:
		RadioButtonDemAM5->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_LSB:
		RadioButtonDemLSB->setChecked(TRUE);
		break;

	case CAMDemodulation::DT_USB:
		RadioButtonDemUSB->setChecked(TRUE);
		break;
	}


	/* Init settings checkbuttons */
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());


	/* Connect controls ----------------------------------------------------- */
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
	
	/* Button groups */
	connect(ButtonGroupDemodulation, SIGNAL(clicked(int)),
		this, SLOT(OnRadioDemodulation(int)));

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

void AnalogDemDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timers when window is shown */
	TimerChart.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimerChart();

	/* Update mute audio switch and write wave file, these can be changed
	   by other windows */
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());
	CheckBoxSaveAudioWave->
		setChecked(DRMReceiver.GetWriteData()->GetIsWriteWaveFile());
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
	MainPlot->SetInpSpec(vecrData, vecrScale,
		DRMReceiver.GetParameters()->GetDCFrequency());
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
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM_10);
		break;

	case 1:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_AM_5);
		break;

	case 2:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_LSB);
		break;

	case 3:
		DRMReceiver.GetAMDemod()->SetDemodType(CAMDemodulation::DT_USB);
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
