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
	SliderNoOfIterations->setValue(DRMReceiver.GetMSCMLC()->GetNoIterations());
	TextNoOfIterations->setText("MLC: Number of Iterations: " + 
		QString().setNum(DRMReceiver.GetMSCMLC()->GetNoIterations()));


	/* Inits for channel estimation switches */
	switch (DRMReceiver.
		GetChanEst()->GetTimeInt())
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

	/* Init progress bar for SNR */
	ThermoSNR->setRange(0.0, 30.0);
	ThermoSNR->setOrientation(QwtThermo::Vertical, QwtThermo::Left);
	ThermoSNR->setFillColor(QColor(0, 190, 0));


	/* Update times for color LEDs */
	LEDFAC->SetUpdateTime(600);
	LEDSDC->SetUpdateTime(1500);
	LEDMSC->SetUpdateTime(600);
	LEDFrameSync->SetUpdateTime(600);
	LEDTimeSync->SetUpdateTime(600);


	/* Set timer for real-time controls */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
	connect(&Timer, SIGNAL(timeout()), 
		this, SLOT(OnTimer()));

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
	}
}

void systemevalDlg::OnTimer()
{
	int					v;
	CVector<_REAL>		vecrData;
	CVector<_REAL>		vecrScale;
	CVector<_COMPLEX>*	pveccData;
	_REAL				rLowerBound, rHigherBound;
	_REAL				rStartGuard, rEndGuard;
	_REAL				rSNREstimate;

	/* SNR estimate */
	rSNREstimate = Min(DRMReceiver.GetChanEst()->GetSNREstdB(), 
		DRMReceiver.GetOFDMDemod()->GetSNREstdB());

	ThermoSNR->setValue(rSNREstimate);
	TextSNR->setText("<center>SNR<br><b>" + 
		QString().setNum(rSNREstimate, 'f', 1) + " dB</b></center>");


#ifdef _DEBUG_
	/* Metric values */
	TextFreqOffset->setText("Metrics [dB]: \t\nMSC: " + 
		QString().setNum(
		DRMReceiver.GetMSCMLC()->GetAccMetric(), 'f', 2) +	" / SDC: "+ 
		QString().setNum(
		DRMReceiver.GetSDCMLC()->GetAccMetric(), 'f', 2) +	" / FAC: "+ 
		QString().setNum(
		DRMReceiver.GetFACMLC()->GetAccMetric(), 'f', 2));
#else
	/* DC frequency */
	TextFreqOffset->setText("DC Frequency of DRM Signal: \t\n" + 
		QString().setNum(
		DRMReceiver.GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
#endif


	/* Doppler estimation (assuming Gaussian doppler spectrum) */
	TextWiener->setText("Doppler: \t\n" + 
		QString().setNum(
		DRMReceiver.GetChanEst()->GetSigma(), 'f', 2) + " Hz");


	/* Sample frequency offset estimation */
	TextSampFreqOffset->setText("Sample Frequency Offset: \t\n" + QString().
		setNum(DRMReceiver.GetParameters()->GetSampFreqEst(), 'f', 2) +	" Hz");


	/* Time, date */
	if ((DRMReceiver.GetParameters()->iUTCHour == 0) &&
		(DRMReceiver.GetParameters()->iUTCMin == 0) &&
		(DRMReceiver.GetParameters()->iDay == 0) &&
		(DRMReceiver.GetParameters()->iMonth == 0) &&
		(DRMReceiver.GetParameters()->iYear == 0))
	{
		/* No time service available */
		TextTimeService->setText("No time service available");
		TextDateService->setText("No date service available");
	}
	else
	{
		/* Set time and date */
		TextTimeService->setText("Received UTC-Time: " +
			QString().setNum(DRMReceiver.GetParameters()->iUTCHour) + ":" +
			QString().setNum(DRMReceiver.GetParameters()->iUTCMin));
		TextDateService->setText("Received Date: " +
			QString().setNum(DRMReceiver.GetParameters()->iMonth) + "/" +
			QString().setNum(DRMReceiver.GetParameters()->iDay) + "/" +
			QString().setNum(DRMReceiver.GetParameters()->iYear));
	}


	/* CHART ******************************************************************/
	switch (CharType)
	{
	case AVERAGED_IR:
		/* Get data from module */
		DRMReceiver.GetChanEst()->GetTimeSyncTrack()->
			GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
			rStartGuard, rEndGuard);

		/* Prepare graph and set data */
		MainPlot->SetAvIR(vecrData, vecrScale, rLowerBound, rHigherBound, 
			rStartGuard, rEndGuard);
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
		MainPlot->SetInpSpec(vecrData, vecrScale);
		break;

	case FAC_CONSTELLATION:
		/* Get data vector */
		pveccData = DRMReceiver.GetFACMLC()->GetVectorSpace();

		/* Prepare graph and set data */
		MainPlot->SetFACConst(*pveccData);
		break;

	case SDC_CONSTELLATION:
		/* Get data vector */
		pveccData = DRMReceiver.GetSDCMLC()->GetVectorSpace();

		/* Prepare graph and set data */
		MainPlot->SetSDCConst(*pveccData, 
			DRMReceiver.GetParameters()->eSDCCodingScheme);
		break;

	case MSC_CONSTELLATION:
		/* Get data vector */
		pveccData = DRMReceiver.GetMSCMLC()->GetVectorSpace();

		/* Prepare graph and set data */
		MainPlot->SetMSCConst(*pveccData, 
			DRMReceiver.GetParameters()->eMSCCodingScheme);
		break;
	}


	/* FAC info static ********************************************************/
	QString m_StaticFACInfo = " "; 

	/* Robustness mode */
	m_StaticFACInfo += "Robustness Mode: \t\t";
	switch (DRMReceiver.GetParameters()->GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		m_StaticFACInfo += "A";
		break;

	case RM_ROBUSTNESS_MODE_B:
		m_StaticFACInfo += "B";
		break;
	
	case RM_ROBUSTNESS_MODE_C:
		m_StaticFACInfo += "C";
		break;
	
	case RM_ROBUSTNESS_MODE_D:
		m_StaticFACInfo += "D";
		break;
	}

	m_StaticFACInfo += "\r\n "; 

	/* Spectrum occupancy */
	m_StaticFACInfo += "Spectrum Occupancy: \t\t";
	switch (DRMReceiver.GetParameters()->GetSpectrumOccup())
	{
	case SO_0:
		m_StaticFACInfo += "4,5 kHz";
		break;

	case SO_1:
		m_StaticFACInfo += "5 kHz";
		break;

	case SO_2:
		m_StaticFACInfo += "9 kHz";
		break;

	case SO_3:
		m_StaticFACInfo += "10 kHz";
		break;

	case SO_4:
		m_StaticFACInfo += "18 kHz";
		break;

	case SO_5:
		m_StaticFACInfo += "20 kHz";
		break;
	}

	m_StaticFACInfo += "\r\n "; 

	/* Interleaver Depth */
	m_StaticFACInfo += "Interleaver Depth: \t\t";
	switch (DRMReceiver.GetParameters()->eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		m_StaticFACInfo += "2 s (Long Interleaving)";
		break;

	case CParameter::SI_SHORT:
		m_StaticFACInfo += "400 ms (Short Interleaving)";
		break;
	}

	m_StaticFACInfo += "\r\n "; 

	/* MSC mode */
	m_StaticFACInfo += "MSC Mode: \t\t\t";
	switch (DRMReceiver.GetParameters()->eMSCCodingScheme)
	{
	case CParameter::CS_2_SM:
		m_StaticFACInfo += "16-QAM, No Hierarchical";
		break;

	case CParameter::CS_3_SM:
		m_StaticFACInfo += "64-QAM, No Hierarchical";
		break;

	case CParameter::CS_3_HMSYM:
		m_StaticFACInfo += "64-QAM, Hierarchical on I&Q";
		break;

	case CParameter::CS_3_HMMIX:
		m_StaticFACInfo += "64-QAM, Hierarchical on I";
		break;
	}

	m_StaticFACInfo += "\r\n "; 

	/* SDC mode */
	m_StaticFACInfo += "SDC Mode: \t\t\t";
	switch (DRMReceiver.GetParameters()->eSDCCodingScheme)
	{
	case CParameter::CS_1_SM:
		m_StaticFACInfo += "4-QAM";
		break;

	case CParameter::CS_2_SM:
		m_StaticFACInfo += "16-QAM";
		break;
	}

	m_StaticFACInfo += "\r\n "; 

	/* Number of services */
	QString strTemp = "Number of Services: \t\t";
	strTemp += QString().setNum(DRMReceiver.GetParameters()->iNoAudioService +
		DRMReceiver.GetParameters()->iNoDataService);
	m_StaticFACInfo += strTemp;

	/* Finally, set the string to the label */
	TextFACInfo->setText(m_StaticFACInfo);
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

void systemevalDlg::OnSliderIterChange(int value)
{
	/* Set new value in working thread module */
	DRMReceiver.GetMSCMLC()->SetNoIterations(value);

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
	DRMReceiver.GetReceiver()->SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
}

void systemevalDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	DRMReceiver.GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
}

