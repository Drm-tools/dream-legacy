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

#include "systemevalDlg.h"


systemevalDlg::systemevalDlg( QWidget* parent, const char* name, bool modal, WFlags f )
	: systemevalDlgBase( parent, name, modal, f )
{
	MainPlot->setMargin(1);

	/* Default chart (at startup) */
	ButtonInpSpec->setOn(TRUE);
	CharType = INPUTSPECTRUM_NO_AV;


	/* Init slider control */
	SliderNoOfIterations->setRange(0, 4);
	SliderNoOfIterations->
		setValue(DRMReceiver.GetMSCMLC()->GetInitNumIterations());
	TextNoOfIterations->setText("MLC: Number of Iterations: " +
		QString().setNum(DRMReceiver.GetMSCMLC()->GetInitNumIterations()));


	/* Inits for channel estimation and time sync switches */
	switch (DRMReceiver.GetChanEst()->GetTimeInt())
	{
	case CChannelEstimation::TLINEAR:
		RadioButtonTiLinear->setChecked(TRUE);
		break;

	case CChannelEstimation::TWIENER:
		RadioButtonTiWiener->setChecked(TRUE);
		break;
	}

	switch (DRMReceiver.GetChanEst()->GetFreqInt())
	{
	case CChannelEstimation::FLINEAR:
		RadioButtonFreqLinear->setChecked(TRUE);
		break;

	case CChannelEstimation::FDFTFILTER:
		RadioButtonFreqDFT->setChecked(TRUE);
		break;

	case CChannelEstimation::FWIENER:
		RadioButtonFreqWiener->setChecked(TRUE);
		break;
	}

	switch (DRMReceiver.GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType())
	{
	case CTimeSyncTrack::TSFIRSTPEAK:
		RadioButtonTiSyncFirstPeak->setChecked(TRUE);
		break;

	case CTimeSyncTrack::TSENERGY:
		RadioButtonTiSyncEnergy->setChecked(TRUE);
		break;
	}

	/* Init settings checkbuttons */
	CheckBoxFlipSpec->setChecked(DRMReceiver.GetReceiver()->GetFlippedSpectrum());
	CheckBoxMuteAudio->setChecked(DRMReceiver.GetWriteData()->GetMuteAudio());


	/* Init progress bar for SNR */
	ThermoSNR->setRange(0.0, 30.0);
	ThermoSNR->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
	ThermoSNR->setFillColor(QColor(0, 190, 0));


	/* Update times for color LEDs */
	LEDFAC->SetUpdateTime(1500);
	LEDSDC->SetUpdateTime(1500);
	LEDMSC->SetUpdateTime(600);
	LEDFrameSync->SetUpdateTime(600);
	LEDTimeSync->SetUpdateTime(600);


	/* Connect controls */
	connect(SliderNoOfIterations, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderIterChange(int)));

	connect(RadioButtonTiLinear, SIGNAL(clicked()),
		this, SLOT(OnRadioTimeLinear()));
	connect(RadioButtonTiWiener, SIGNAL(clicked()),
		this, SLOT(OnRadioTimeWiener()));
	connect(RadioButtonFreqLinear, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyLinear()));
	connect(RadioButtonFreqDFT, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyDft()));
	connect(RadioButtonFreqWiener, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyWiener()));
	connect(RadioButtonTiSyncEnergy, SIGNAL(clicked()),
		this, SLOT(OnRadioTiSyncEnergy()));
	connect(RadioButtonTiSyncFirstPeak, SIGNAL(clicked()),
		this, SLOT(OnRadioTiSyncFirstPeak()));

	connect(ButtonAvIR, SIGNAL(clicked()),
		this, SLOT(OnButtonAvIR()));
	connect(ButtonTransFct, SIGNAL(clicked()),
		this, SLOT(OnButtonTransFct()));
	connect(ButtonFACConst, SIGNAL(clicked()),
		this, SLOT(OnButtonFACConst()));
	connect(ButtonSDCConst, SIGNAL(clicked()),
		this, SLOT(OnButtonSDCConst()));
	connect(ButtonMSCConst, SIGNAL(clicked()),
		this, SLOT(OnButtonMSCConst()));
	connect(ButtonPSD, SIGNAL(clicked()),
		this, SLOT(OnButtonPSD()));
	connect(ButtonInpSpec, SIGNAL(clicked()),
		this, SLOT(OnButtonInpSpec()));

	connect(buttonOk, SIGNAL(clicked()),
		this, SLOT(accept()));

	connect(CheckBoxFlipSpec, SIGNAL(clicked()),
		this, SLOT(OnCheckFlipSpectrum()));
	connect(CheckBoxMuteAudio, SIGNAL(clicked()),
		this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxWriteLog, SIGNAL(clicked()),
		this, SLOT(OnCheckWriteLog()));

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerChart, SIGNAL(timeout()),
		this, SLOT(OnTimerChart()));
	connect(&TimerLogFile, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFile()));

	/* Activte real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimerChart();
}

void systemevalDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timers when window is shown */
	TimerChart.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimerChart();
}

void systemevalDlg::hideEvent(QHideEvent* pEvent)
{
	/* Deactivate real-time timers when window is hide to save CPU power */
	TimerChart.stop();
}

void systemevalDlg::SetStatus(int MessID, int iMessPara)
{
	switch(MessID)
	{
	case MS_FAC_CRC:
		LEDFAC->SetLight(iMessPara);
		break;

	case MS_SDC_CRC:
		LEDSDC->SetLight(iMessPara);
		break;

	case MS_MSC_CRC:
		LEDMSC->SetLight(iMessPara);
		break;

	case MS_FRAME_SYNC:
		LEDFrameSync->SetLight(iMessPara);
		break;

	case MS_TIME_SYNC:
		LEDTimeSync->SetLight(iMessPara);
		break;

	case MS_RESET_ALL:
		LEDFAC->Reset();
		LEDSDC->Reset();
		LEDMSC->Reset();
		LEDFrameSync->Reset();
		LEDTimeSync->Reset();
		break;
	}
}

void systemevalDlg::OnTimerChart()
{
	CVector<_REAL>		vecrData;
	CVector<_COMPLEX>	veccData;
	CVector<_REAL>		vecrScale;
	_REAL				rLowerBound, rHigherBound;
	_REAL				rStartGuard, rEndGuard;
	_REAL				rPDSBegin, rPDSEnd;

	/* CHART ******************************************************************/
	switch (CharType)
	{
	case AVERAGED_IR:
		/* Get data from module */
		DRMReceiver.GetChanEst()->
			GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);

		/* Prepare graph and set data */
		MainPlot->SetAvIR(vecrData, vecrScale, rLowerBound, rHigherBound, 
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
		break;

	case TRANSFERFUNCTION:
		/* Get data from module */
		DRMReceiver.GetChanEst()->GetTransferFunction(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetTranFct(vecrData, vecrScale);
		break;

	case POWER_SPEC_DENSITY:
		/* Get data from module */
		DRMReceiver.GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetPSD(vecrData, vecrScale);
		break;

	case INPUTSPECTRUM_NO_AV:
		/* Get data from module */
		DRMReceiver.GetReceiver()->GetInputSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetInpSpec(vecrData, vecrScale,
			DRMReceiver.GetParameters()->GetDCFrequency());
		break;

	case FAC_CONSTELLATION:
		/* Get data vector */
		DRMReceiver.GetFACMLC()->GetVectorSpace(veccData);

		/* Prepare graph and set data */
		MainPlot->SetFACConst(veccData);
		break;

	case SDC_CONSTELLATION:
		/* Get data vector */
		DRMReceiver.GetSDCMLC()->GetVectorSpace(veccData);

		/* Prepare graph and set data */
		MainPlot->SetSDCConst(veccData, 
			DRMReceiver.GetParameters()->eSDCCodingScheme);
		break;

	case MSC_CONSTELLATION:
		/* Get data vector */
		DRMReceiver.GetMSCMLC()->GetVectorSpace(veccData);

		/* Prepare graph and set data */
		MainPlot->SetMSCConst(veccData, 
			DRMReceiver.GetParameters()->eMSCCodingScheme);
		break;
	}
}

void systemevalDlg::OnTimer()
{
	_REAL rSNREstimate;
	QString strTextWiener = "Doppler / Delay: \t\n";
	QString strTextFreqOffs = "Sample Frequency Offset: \t\n";

	/* Show SNR if receiver is in tracking mode */
	if (DRMReceiver.GetReceiverState() == CDRMReceiver::AS_WITH_SIGNAL)
	{
		rSNREstimate = DRMReceiver.GetChanEst()->GetSNREstdB();

		TextSNR->setText("<center>SNR<br><b>" + 
			QString().setNum(rSNREstimate, 'f', 1) + " dB</b></center>");

		/* Set SNR for log file */
		DRMReceiver.GetParameters()->ReceptLog.SetSNR(rSNREstimate);

		/* Doppler estimation (assuming Gaussian doppler spectrum) */
		TextWiener->setText(strTextWiener +
			QString().setNum(
			DRMReceiver.GetChanEst()->GetSigma(), 'f', 2) + " Hz / " +
			QString().setNum(
			DRMReceiver.GetChanEst()->GetDelay(), 'f', 2) + " ms");

		/* Sample frequency offset estimation */
		TextSampFreqOffset->setText(strTextFreqOffs + QString().
			setNum(DRMReceiver.GetParameters()->GetSampFreqEst(), 'f', 2) +	" Hz");
	}
	else
	{
		rSNREstimate = 0;

		TextSNR->setText("SNR<br><b>---</b>");

		TextWiener->setText(strTextWiener + "--- / ---");
		TextSampFreqOffset->setText(strTextFreqOffs + "---");
	}
	ThermoSNR->setValue(rSNREstimate);


#ifdef _DEBUG_
	/* Metric values */
	TextFreqOffset->setText("Metrics [dB]: \t\nMSC: " +
		QString().setNum(
		DRMReceiver.GetMSCMLC()->GetAccMetric(), 'f', 2) +	" / SDC: " +
		QString().setNum(
		DRMReceiver.GetSDCMLC()->GetAccMetric(), 'f', 2) +	" / FAC: " +
		QString().setNum(
		DRMReceiver.GetFACMLC()->GetAccMetric(), 'f', 2) + "\nDC Frequency: " +
		QString().setNum(
		DRMReceiver.GetParameters()->GetDCFrequency(), 'f', 3) + " Hz");
#else
	/* DC frequency */
	TextFreqOffset->setText("DC Frequency of DRM Signal: \t\n" +
		QString().setNum(
		DRMReceiver.GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
#endif


	/* FAC info static ------------------------------------------------------ */
	QString strFACInfo;

	/* Robustness mode #################### */
	strFACInfo = "DRM Mode / Bandwidth: \t" + GetRobModeStr() + " / " +
		GetSpecOccStr();

	FACDRMModeBW->setText(strFACInfo);

	/* Interleaver Depth #################### */
	strFACInfo = "Interleaver Depth: \t\t";
	switch (DRMReceiver.GetParameters()->eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		strFACInfo += "2 s (Long Interleaving)";
		break;

	case CParameter::SI_SHORT:
		strFACInfo += "400 ms (Short Interleaving)";
		break;
	}

	FACInterleaverDepth->setText(strFACInfo);

	/* SDC, MSC mode #################### */
	strFACInfo = "SDC / MSC Mode: \t\t";

	/* SDC */
	switch (DRMReceiver.GetParameters()->eSDCCodingScheme)
	{
	case CParameter::CS_1_SM:
		strFACInfo += "4-QAM / ";
		break;

	case CParameter::CS_2_SM:
		strFACInfo += "16-QAM / ";
		break;
	}

	/* MSC */
	switch (DRMReceiver.GetParameters()->eMSCCodingScheme)
	{
	case CParameter::CS_2_SM:
		strFACInfo += "SM 16-QAM";
		break;

	case CParameter::CS_3_SM:
		strFACInfo += "SM 64-QAM";
		break;

	case CParameter::CS_3_HMSYM:
		strFACInfo += "HMsym 64-QAM";
		break;

	case CParameter::CS_3_HMMIX:
		strFACInfo += "HMmix 64-QAM";
		break;
	}

	FACSDCMSCMode->setText(strFACInfo);

	/* Code rates #################### */
	strFACInfo = "Prot. Level (B / A): \t\t";
	strFACInfo += QString().setNum(DRMReceiver.GetParameters()->MSCPrLe.iPartB);
	strFACInfo += " / ";
	strFACInfo += QString().setNum(DRMReceiver.GetParameters()->MSCPrLe.iPartA);

	FACCodeRate->setText(strFACInfo);

	/* Number of services #################### */
	strFACInfo = "Number of Services: \t\tAudio: ";
	strFACInfo += QString().setNum(DRMReceiver.GetParameters()->iNumAudioService);
	strFACInfo += " / Data: ";
	strFACInfo +=QString().setNum(DRMReceiver.GetParameters()->iNumDataService);

	FACNumServices->setText(strFACInfo);

	/* Time, date #################### */
	strFACInfo = "Received time - date: \t\t";

	if ((DRMReceiver.GetParameters()->iUTCHour == 0) &&
		(DRMReceiver.GetParameters()->iUTCMin == 0) &&
		(DRMReceiver.GetParameters()->iDay == 0) &&
		(DRMReceiver.GetParameters()->iMonth == 0) &&
		(DRMReceiver.GetParameters()->iYear == 0))
	{
		/* No time service available */
		strFACInfo += "Service not available";
	}
	else
	{
#ifdef GUI_QT_DATE_TIME_TYPE
		/* QT type of displaying date and time */
		QDateTime DateTime;
		DateTime.setDate(QDate(DRMReceiver.GetParameters()->iYear,
			DRMReceiver.GetParameters()->iMonth,
			DRMReceiver.GetParameters()->iDay));
		DateTime.setTime(QTime(DRMReceiver.GetParameters()->iUTCHour,
			DRMReceiver.GetParameters()->iUTCMin));

		strFACInfo += DateTime.toString();
#else
		/* Set time and date */
		QString strMin;
		const int iMin = DRMReceiver.GetParameters()->iUTCMin;

		/* Add leading zero to number smaller than 10 */
		if (iMin < 10)
			strMin = "0";
		else
			strMin = "";
	
		strMin += QString().setNum(iMin);

		strFACInfo +=
			/* Time */
			QString().setNum(DRMReceiver.GetParameters()->iUTCHour) + ":" +
			strMin + "  -  " +
			/* Date */
			QString().setNum(DRMReceiver.GetParameters()->iMonth) + "/" +
			QString().setNum(DRMReceiver.GetParameters()->iDay) + "/" +
			QString().setNum(DRMReceiver.GetParameters()->iYear);
#endif
	}

	FACTimeDate->setText(strFACInfo);
}

void systemevalDlg::OnRadioTimeLinear() 
{
	if (DRMReceiver.GetChanEst()->GetTimeInt() != CChannelEstimation::TLINEAR)
		DRMReceiver.GetChanEst()->SetTimeInt(CChannelEstimation::TLINEAR);
}

void systemevalDlg::OnRadioTimeWiener() 
{
	if (DRMReceiver.GetChanEst()->GetTimeInt() != CChannelEstimation::TWIENER)
		DRMReceiver.GetChanEst()->SetTimeInt(CChannelEstimation::TWIENER);
}

void systemevalDlg::OnRadioFrequencyLinear() 
{
	if (DRMReceiver.GetChanEst()->GetFreqInt() != CChannelEstimation::FLINEAR)
		DRMReceiver.GetChanEst()->SetFreqInt(CChannelEstimation::FLINEAR);
}

void systemevalDlg::OnRadioFrequencyDft() 
{
	if (DRMReceiver.GetChanEst()->GetFreqInt() != CChannelEstimation::FDFTFILTER)
		DRMReceiver.GetChanEst()->SetFreqInt(CChannelEstimation::FDFTFILTER);
}

void systemevalDlg::OnRadioFrequencyWiener() 
{
	if (DRMReceiver.GetChanEst()->GetFreqInt() != CChannelEstimation::FWIENER)
		DRMReceiver.GetChanEst()->SetFreqInt(CChannelEstimation::FWIENER);
}

void systemevalDlg::OnRadioTiSyncFirstPeak() 
{
	if (DRMReceiver.GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType() != 
		CTimeSyncTrack::TSFIRSTPEAK)
	{
		DRMReceiver.GetChanEst()->GetTimeSyncTrack()->
			SetTiSyncTracType(CTimeSyncTrack::TSFIRSTPEAK);
	}
}

void systemevalDlg::OnRadioTiSyncEnergy() 
{
	if (DRMReceiver.GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType() != 
		CTimeSyncTrack::TSENERGY)
	{
		DRMReceiver.GetChanEst()->GetTimeSyncTrack()->
			SetTiSyncTracType(CTimeSyncTrack::TSENERGY);
	}
}

void systemevalDlg::OnSliderIterChange(int value)
{
	/* Set new value in working thread module */
	DRMReceiver.GetMSCMLC()->SetNumIterations(value);

	/* Show the new value in the label control */
	TextNoOfIterations->setText("MLC: Number of Iterations: " +
		QString().setNum(value));
}

void systemevalDlg::OnButtonAvIR()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonAvIR);

	CharType = AVERAGED_IR;
}

void systemevalDlg::OnButtonTransFct()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonTransFct);

	CharType = TRANSFERFUNCTION;
}

void systemevalDlg::OnButtonFACConst()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonFACConst);

	CharType = FAC_CONSTELLATION;
}

void systemevalDlg::OnButtonSDCConst()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonSDCConst);

	CharType = SDC_CONSTELLATION;
}

void systemevalDlg::OnButtonMSCConst()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonMSCConst);

	CharType = MSC_CONSTELLATION;
}

void systemevalDlg::OnButtonPSD()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonPSD);

	CharType = POWER_SPEC_DENSITY;
}

void systemevalDlg::OnButtonInpSpec()
{
	/* Set all other buttons up */
	OnlyThisButDown(ButtonInpSpec);

	CharType = INPUTSPECTRUM_NO_AV;
}

void systemevalDlg::OnlyThisButDown(QPushButton* pButton)
{
	/* Set all buttons of the chart group up */
	if (ButtonAvIR->isOn()) ButtonAvIR->setOn(FALSE);
	if (ButtonTransFct->isOn()) ButtonTransFct->setOn(FALSE);
	if (ButtonFACConst->isOn()) ButtonFACConst->setOn(FALSE);
	if (ButtonSDCConst->isOn()) ButtonSDCConst->setOn(FALSE);
	if (ButtonMSCConst->isOn()) ButtonMSCConst->setOn(FALSE);
	if (ButtonPSD->isOn()) ButtonPSD->setOn(FALSE);
	if (ButtonInpSpec->isOn()) ButtonInpSpec->setOn(FALSE);

	/* If button was already down, put him back */
	if (pButton->isOn() == FALSE)
		pButton->setOn(TRUE);
}

void systemevalDlg::OnCheckFlipSpectrum()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetReceiver()->
		SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
}

void systemevalDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
}

void systemevalDlg::OnCheckWriteLog()
{
	if (CheckBoxWriteLog->isChecked())
	{
		/* Activte log file timer, update time: 1 min (i.e. 60000 ms) */
		TimerLogFile.start(60000);

		/* Set frequency of front-end */
		QString strFreq = EdtFrequency->text();
		DRMReceiver.GetParameters()->ReceptLog.SetFrequency(strFreq.toUInt());

		/* Set some other information obout this receiption */
		QString strAddText = "";

		/* Check if receiver does receive a DRM signal */
		if ((DRMReceiver.GetReceiverState() == CDRMReceiver::AS_WITH_SIGNAL) &&
			(DRMReceiver.GetReceiverMode() == CDRMReceiver::RM_DRM))
		{
			/* First get current selected audio service */
			int iCurSelServ =
				DRMReceiver.GetParameters()->GetCurSelAudioService();

			/* Check whether service parameters were not transmitted yet */
			if (DRMReceiver.GetParameters()->Service[iCurSelServ].IsActive())
			{
				strAddText = "Label            ";

				/* Service label (UTF-8 encoded string -> convert) */
				strAddText += QString().fromUtf8(QCString(
					DRMReceiver.GetParameters()->Service[iCurSelServ].
					strLabel.c_str()));

				strAddText += "\nBitrate          ";

				strAddText += QString().setNum(DRMReceiver.GetParameters()->
					GetBitRate(iCurSelServ), 'f', 2) + " kbps";

				strAddText += "\nMode             " + GetRobModeStr();
				strAddText += "\nBandwidth        " + GetSpecOccStr();
			}
		}

		/* Set additional text for log file. Conversion from QString to STL
		   string is needed (done with .latin1() function of QT string) */
		string strTemp = strAddText.latin1();
		DRMReceiver.GetParameters()->ReceptLog.SetAdditText(strTemp);

		/* Activate log file */
		DRMReceiver.GetParameters()->ReceptLog.SetLog(TRUE);
	}
	else
	{
		/* Deactivate log file timer */
		TimerLogFile.stop();

		DRMReceiver.GetParameters()->ReceptLog.SetLog(FALSE);
	}
}

void systemevalDlg::OnTimerLogFile()
{
	/* Write new parameters in log file */
	DRMReceiver.GetParameters()->ReceptLog.WriteParameters();
}

QString	systemevalDlg::GetRobModeStr()
{
	switch (DRMReceiver.GetParameters()->GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		return "A";
		break;

	case RM_ROBUSTNESS_MODE_B:
		return "B";
		break;

	case RM_ROBUSTNESS_MODE_C:
		return "C";
		break;

	case RM_ROBUSTNESS_MODE_D:
		return "D";
		break;

	default:
		return "A";
	}
}

QString	systemevalDlg::GetSpecOccStr()
{
	switch (DRMReceiver.GetParameters()->GetSpectrumOccup())
	{
	case SO_0:
		return "4,5 kHz";
		break;

	case SO_1:
		return "5 kHz";
		break;

	case SO_2:
		return "9 kHz";
		break;

	case SO_3:
		return "10 kHz";
		break;

	case SO_4:
		return "18 kHz";
		break;

	case SO_5:
		return "20 kHz";
		break;

	default:
		return "10 kHz";
	}
}
