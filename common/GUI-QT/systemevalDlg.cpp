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


/* Implementation *************************************************************/
systemevalDlg::systemevalDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) :
	systemevalDlgBase(parent, name, modal, f), pDRMRec(pNDRMR),
	bOnTimerCharMutexFlag(FALSE)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomSystemEvalDlg.iXPos,
		pDRMRec->GeomSystemEvalDlg.iYPos,
		pDRMRec->GeomSystemEvalDlg.iWSize,
		pDRMRec->GeomSystemEvalDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#endif


	/* Init controls -------------------------------------------------------- */
	/* Init main plot */
	MainPlot->SetPlotStyle(pDRMRec->iMainPlotColorStyle);
	MainPlot->setMargin(1);

	/* Init slider control */
	SliderNoOfIterations->setRange(0, 4);
	SliderNoOfIterations->
		setValue(pDRMRec->GetMSCMLC()->GetInitNumIterations());
	TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
		QString().setNum(pDRMRec->GetMSCMLC()->GetInitNumIterations()));

	/* Update times for color LEDs */
	LEDFAC->SetUpdateTime(1500);
	LEDSDC->SetUpdateTime(1500);
	LEDMSC->SetUpdateTime(600);
	LEDFrameSync->SetUpdateTime(600);
	LEDTimeSync->SetUpdateTime(600);
	LEDIOInterface->SetUpdateTime(2000); /* extra long -> red light stays long */

	/* Init parameter for frequency edit for log file */
	iCurFrequency = 0;

	/* Update controls */
	UpdateControls();


	/* Init chart selector list view ---------------------------------------- */
	ListViewCharSel->clear();

	/* No sorting of items */
	ListViewCharSel->setSorting(-1);

	/* Insert parent list view items. Parent list view items should not be
	   selectable */
	CCharSelItem* pHistoryLiViIt =
		new CCharSelItem(ListViewCharSel, tr("History"), NONE_OLD, FALSE);

	CCharSelItem* pConstellationLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Constellation"), NONE_OLD, FALSE);

	CCharSelItem* pChannelLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Channel"), NONE_OLD, FALSE);

	CCharSelItem* pSpectrumLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Spectrum"), NONE_OLD, FALSE);
 
	/* Inser actual items. The list is not sorted -> items which are inserted
	   first show up at the end of the list */
	/* Spectrum */
	new CCharSelItem(pSpectrumLiViIt, tr("SNR Spectrum"), SNR_SPECTRUM);
	CCharSelItem* pListItAudSpec = new CCharSelItem(pSpectrumLiViIt,
		tr("Audio Spectrum"), AUDIO_SPECTRUM);
	new CCharSelItem(pSpectrumLiViIt, tr("Shifted PSD"), POWER_SPEC_DENSITY);
	new CCharSelItem(pSpectrumLiViIt, tr("Input Spectrum"),
		INPUTSPECTRUM_NO_AV);
	CCharSelItem* pListItInpPSD = new CCharSelItem(pSpectrumLiViIt,
		tr("Input PSD"), INPUT_SIG_PSD);

	/* Constellation */
	new CCharSelItem(pConstellationLiViIt, tr("MSC"), MSC_CONSTELLATION);
	new CCharSelItem(pConstellationLiViIt, tr("SDC"), SDC_CONSTELLATION);
	new CCharSelItem(pConstellationLiViIt, tr("FAC"), FAC_CONSTELLATION);
	new CCharSelItem(pConstellationLiViIt,
		tr("FAC / SDC / MSC"), ALL_CONSTELLATION);

	/* History */
	new CCharSelItem(pHistoryLiViIt,
		"Frequency / Sample Rate", FREQ_SAM_OFFS_HIST);
	new CCharSelItem(pHistoryLiViIt, tr("Delay / Doppler"), DOPPLER_DELAY_HIST);
	new CCharSelItem(pHistoryLiViIt, tr("SNR / Audio"), SNR_AUDIO_HIST);

	/* Channel */
	new CCharSelItem(pChannelLiViIt, tr("Transfer Function"), TRANSFERFUNCTION);
	new CCharSelItem(pChannelLiViIt, tr("Impulse Response"), AVERAGED_IR);

/* _WIN32 fix because in Visual c++ the GUI files are always compiled even
   if USE_QT_GUI is set or not (problem with MDI in DRMReceiver) */
#ifdef USE_QT_GUI
	/* If MDI in is enabled, disable some of the controls and use different
	   initialization for the chart and chart selector */
	if (pDRMRec->GetMDI()->GetMDIInEnabled() == TRUE)
	{
		ListViewCharSel->setEnabled(FALSE);
		SliderNoOfIterations->setEnabled(FALSE);

		ButtonGroupChanEstFreqInt->setEnabled(FALSE);
		ButtonGroupChanEstTimeInt->setEnabled(FALSE);
		ButtonGroupTimeSyncTrack->setEnabled(FALSE);
		CheckBoxFlipSpec->setEnabled(FALSE);
		CheckBoxWriteLog->setEnabled(FALSE);
		EdtFrequency->setEnabled(FALSE);
		GroupBoxInterfRej->setEnabled(FALSE);

		/* Only audio spectrum makes sence for MDI in */
		ListViewCharSel->setSelected(pListItAudSpec, TRUE);
		ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
		SetupChart(AUDIO_SPECTRUM);
	}
	else
	{
		/* Default chart (at startup) */
		ListViewCharSel->setSelected(pListItInpPSD, TRUE);
		ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
		SetupChart(INPUT_SIG_PSD);
	}
#endif


	/* Connect controls ----------------------------------------------------- */
	connect(SliderNoOfIterations, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderIterChange(int)));

	/* Radio buttons */
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

	/* Char selector list view */
	connect(ListViewCharSel, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListSelChanged(QListViewItem*)));

	/* Buttons */
	connect(buttonOk, SIGNAL(clicked()),
		this, SLOT(accept()));

	/* Check boxes */
	connect(CheckBoxFlipSpec, SIGNAL(clicked()),
		this, SLOT(OnCheckFlipSpectrum()));
	connect(CheckBoxMuteAudio, SIGNAL(clicked()),
		this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxWriteLog, SIGNAL(clicked()),
		this, SLOT(OnCheckWriteLog()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()),
		this, SLOT(OnCheckSaveAudioWAV()));
	connect(CheckBoxRecFilter, SIGNAL(clicked()),
		this, SLOT(OnCheckRecFilter()));
	connect(CheckBoxModiMetric, SIGNAL(clicked()),
		this, SLOT(OnCheckModiMetric()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerChart, SIGNAL(timeout()),
		this, SLOT(OnTimerChart()));
	connect(&TimerLogFileLong, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileLong()));
	connect(&TimerLogFileShort, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileShort()));
	connect(&TimerLogFileStart, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileStart()));

	/* Activte real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Activate delayed log file start if necessary (timer is set to shot
	   only once) */
	if (pDRMRec->GetParameters()->ReceptLog.IsDelLogStart() == TRUE)
	{
		/* One shot timer */
		TimerLogFileStart.start(pDRMRec->GetParameters()->
			ReceptLog.GetDelLogStart() * 1000 /* ms */, TRUE);
	}
}

systemevalDlg::~systemevalDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomSystemEvalDlg.iXPos = WinGeom.x();
	pDRMRec->GeomSystemEvalDlg.iYPos = WinGeom.y();
	pDRMRec->GeomSystemEvalDlg.iHSize = WinGeom.height();
	pDRMRec->GeomSystemEvalDlg.iWSize = WinGeom.width();
}

void systemevalDlg::UpdateControls()
{
	/* Slider for MLC number of iterations */
	const int iNumIt = pDRMRec->GetMSCMLC()->GetInitNumIterations();
	if (SliderNoOfIterations->value() != iNumIt)
	{
		/* Update slider and label */
		SliderNoOfIterations->setValue(iNumIt);
		TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
			QString().setNum(iNumIt));
	}

	/* Update for channel estimation and time sync switches */
	switch (pDRMRec->GetChanEst()->GetTimeInt())
	{
	case CChannelEstimation::TLINEAR:
		if (!RadioButtonTiLinear->isChecked())
			RadioButtonTiLinear->setChecked(TRUE);
		break;

	case CChannelEstimation::TWIENER:
		if (!RadioButtonTiWiener->isChecked())
			RadioButtonTiWiener->setChecked(TRUE);
		break;
	}

	switch (pDRMRec->GetChanEst()->GetFreqInt())
	{
	case CChannelEstimation::FLINEAR:
		if (!RadioButtonFreqLinear->isChecked())
			RadioButtonFreqLinear->setChecked(TRUE);
		break;

	case CChannelEstimation::FDFTFILTER:
		if (!RadioButtonFreqDFT->isChecked())
			RadioButtonFreqDFT->setChecked(TRUE);
		break;

	case CChannelEstimation::FWIENER:
		if (!RadioButtonFreqWiener->isChecked())
			RadioButtonFreqWiener->setChecked(TRUE);
		break;
	}

	switch (pDRMRec->GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType())
	{
	case CTimeSyncTrack::TSFIRSTPEAK:
		if (!RadioButtonTiSyncFirstPeak->isChecked())
			RadioButtonTiSyncFirstPeak->setChecked(TRUE);
		break;

	case CTimeSyncTrack::TSENERGY:
		if (!RadioButtonTiSyncEnergy->isChecked())
			RadioButtonTiSyncEnergy->setChecked(TRUE);
		break;
	}

	/* Update settings checkbuttons */
	CheckBoxMuteAudio->setChecked(pDRMRec->GetWriteData()->GetMuteAudio());
	CheckBoxFlipSpec->
		setChecked(pDRMRec->GetReceiver()->GetFlippedSpectrum());
	CheckBoxSaveAudioWave->
		setChecked(pDRMRec->GetWriteData()->GetIsWriteWaveFile());

	CheckBoxRecFilter->setChecked(pDRMRec->GetOFDMDemod()->GetRecFilter());
	CheckBoxModiMetric->setChecked(pDRMRec->GetChanEst()->GetIntCons());

	/* Update frequency edit control (frequency could be changed by
	   schedule dialog */
	QString strFreq = EdtFrequency->text();
	const int iCurLogFreq =
		pDRMRec->GetParameters()->ReceptLog.GetFrequency();

	if (iCurLogFreq != iCurFrequency)
	{
		EdtFrequency->setText(QString().setNum(iCurLogFreq));
		iCurFrequency = iCurLogFreq;
	}
}

void systemevalDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timers when window is shown */
	SetupChart(CharType);

	/* Update window */
	OnTimerChart();

	/* Update controls */
	UpdateControls();
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

	case MS_IOINTERFACE:
		LEDIOInterface->SetLight(iMessPara);
		break;

	case MS_RESET_ALL:
		LEDFAC->Reset();
		LEDSDC->Reset();
		LEDMSC->Reset();
		LEDFrameSync->Reset();
		LEDTimeSync->Reset();
		LEDIOInterface->Reset();
		break;
	}
}

void systemevalDlg::OnTimerChart()
{
	/* In some cases, if the user moves the mouse very fast over the chart
	   selection list view, this function is called by two different threads.
	   Somehow, using QMtuex does not help. Therefore we introduce a flag for
	   doing this job. This solution is a work-around. TODO: better solution */
	if (bOnTimerCharMutexFlag == TRUE)
		return;

	bOnTimerCharMutexFlag = TRUE;


	/* CHART ******************************************************************/
	CVector<_REAL>		vecrData;
	CVector<_REAL>		vecrData2;
	CVector<_COMPLEX>	veccData1, veccData2, veccData3;
	CVector<_REAL>		vecrScale;
	_REAL				rLowerBound, rHigherBound;
	_REAL				rStartGuard, rEndGuard;
	_REAL				rPDSBegin, rPDSEnd;
	_REAL				rFreqAcquVal;

	switch (CharType)
	{
	case AVERAGED_IR:
		/* Get data from module */
		pDRMRec->GetChanEst()->
			GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);

		/* Prepare graph and set data */
		MainPlot->SetAvIR(vecrData, vecrScale, rLowerBound, rHigherBound, 
			rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
		break;

	case TRANSFERFUNCTION:
		/* Get data from module */
		pDRMRec->GetChanEst()->
			GetTransferFunction(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetTranFct(vecrData, vecrData2, vecrScale);
		break;

	case POWER_SPEC_DENSITY:
		/* Get data from module */
		pDRMRec->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetPSD(vecrData, vecrScale);
		break;

	case SNR_SPECTRUM:
		/* Get data from module */
		pDRMRec->GetChanEst()->GetSNRProfile(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetSNRSpectrum(vecrData, vecrScale);
		break;

	case INPUTSPECTRUM_NO_AV:
		/* Get data from module */
		pDRMRec->GetReceiver()->GetInputSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetInpSpec(vecrData, vecrScale,
			pDRMRec->GetParameters()->GetDCFrequency());
		break;

	case INPUT_SIG_PSD:
		/* Get data from module */
		pDRMRec->GetReceiver()->GetInputPSD(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetInpPSD(vecrData, vecrScale,
			pDRMRec->GetParameters()->GetDCFrequency());
		break;

	case AUDIO_SPECTRUM:
		/* Get data from module */
		pDRMRec->GetWriteData()->GetAudioSpec(vecrData, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetAudioSpec(vecrData, vecrScale);
		break;

	case FREQ_SAM_OFFS_HIST:
		/* Get data from module */
		pDRMRec->GetFreqSamOffsHist(vecrData, vecrData2, vecrScale,
			rFreqAcquVal);

		/* Prepare graph and set data */
		MainPlot->SetFreqSamOffsHist(vecrData, vecrData2, vecrScale,
			rFreqAcquVal);
		break;

	case DOPPLER_DELAY_HIST:
		/* Get data from module */
		pDRMRec->GetDopplerDelHist(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetDopplerDelayHist(vecrData, vecrData2, vecrScale);
		break;

	case SNR_AUDIO_HIST:
		/* Get data from module */
		pDRMRec->GetSNRHist(vecrData, vecrData2, vecrScale);

		/* Prepare graph and set data */
		MainPlot->SetSNRAudHist(vecrData, vecrData2, vecrScale);
		break;

	case FAC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetFACMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		MainPlot->SetFACConst(veccData1);
		break;

	case SDC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetSDCMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		MainPlot->SetSDCConst(veccData1, 
			pDRMRec->GetParameters()->eSDCCodingScheme);
		break;

	case MSC_CONSTELLATION:
		/* Get data vector */
		pDRMRec->GetMSCMLC()->GetVectorSpace(veccData1);

		/* Prepare graph and set data */
		MainPlot->SetMSCConst(veccData1, 
			pDRMRec->GetParameters()->eMSCCodingScheme);
		break;

	case ALL_CONSTELLATION:
		/* Get data vectors */
		pDRMRec->GetMSCMLC()->GetVectorSpace(veccData1);
		pDRMRec->GetSDCMLC()->GetVectorSpace(veccData2);
		pDRMRec->GetFACMLC()->GetVectorSpace(veccData3);

		/* Prepare graph and set data */
		MainPlot->SetAllConst(veccData1, veccData2, veccData3);
		break;
	}

	/* "Unlock" mutex flag */
	bOnTimerCharMutexFlag = FALSE;
}

void systemevalDlg::SetupChart(const ECharType eNewType)
{
	if (eNewType != NONE_OLD)
	{
		/* Set internal variable */
		CharType = eNewType;

		/* Update help text connected with the plot widget */
		AddWhatsThisHelpChar(eNewType);

		/* Update chart */
		OnTimerChart();

		/* Set up timer */
		switch (eNewType)
		{
		case AVERAGED_IR:
		case TRANSFERFUNCTION:
		case POWER_SPEC_DENSITY:
		case INPUT_SIG_PSD:
		case SNR_SPECTRUM:
			/* Fast update */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME_FAST);
			break;

		case FAC_CONSTELLATION:
		case SDC_CONSTELLATION:
		case MSC_CONSTELLATION:
		case ALL_CONSTELLATION:
		case INPUTSPECTRUM_NO_AV:
		case AUDIO_SPECTRUM:
		case FREQ_SAM_OFFS_HIST:
		case DOPPLER_DELAY_HIST:
		case SNR_AUDIO_HIST:
			/* Slow update of plot */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME);
			break;
		}
	}
}

void systemevalDlg::OnTimer()
{
	_REAL rSNREstimate;
	_REAL rSigmaEst;

	/* Show SNR if receiver is in tracking mode */
	if (pDRMRec->GetReceiverState() == CDRMReceiver::AS_WITH_SIGNAL)
	{
		/* Get SNR value and use it if available and valid */
		if (pDRMRec->GetChanEst()->GetSNREstdB(rSNREstimate))
		{
			ValueSNR->setText("<b>" +
				QString().setNum(rSNREstimate, 'f', 1) + " dB</b>");

			/* Set SNR for log file */
			pDRMRec->GetParameters()->ReceptLog.SetSNR(rSNREstimate);
		}
		else
			ValueSNR->setText("<b>---</b>");

		/* Doppler estimation (assuming Gaussian doppler spectrum) */
		if (pDRMRec->GetChanEst()->GetSigma(rSigmaEst))
		{
			/* Plot delay and Doppler values */
			ValueWiener->setText(
				QString().setNum(rSigmaEst, 'f', 2) + " Hz / " +
				QString().setNum(
				pDRMRec->GetChanEst()->GetMinDelay(), 'f', 2) + " ms");
		}
		else
		{
			/* Plot only delay, Doppler not available */
			ValueWiener->setText("--- / " + QString().setNum(
				pDRMRec->GetChanEst()->GetMinDelay(), 'f', 2) + " ms");
		}

		/* Sample frequency offset estimation */
		ValueSampFreqOffset->setText(
			QString().setNum(pDRMRec->GetParameters()->
			GetSampFreqEst(), 'f', 2) +	" Hz");
	}
	else
	{
		ValueSNR->setText("<b>---</b>");
		ValueWiener->setText("--- / ---");
		ValueSampFreqOffset->setText("---");
	}

#ifdef _DEBUG_
	TextFreqOffset->setText("DC: " +
		QString().setNum(pDRMRec->GetParameters()->
		GetDCFrequency(), 'f', 3) + " Hz ");

	/* Metric values */
	ValueFreqOffset->setText(tr("Metrics [dB]: MSC: ") +
		QString().setNum(
		pDRMRec->GetMSCMLC()->GetAccMetric(), 'f', 2) +	"\nSDC: " +
		QString().setNum(
		pDRMRec->GetSDCMLC()->GetAccMetric(), 'f', 2) +	" / FAC: " +
		QString().setNum(
		pDRMRec->GetFACMLC()->GetAccMetric(), 'f', 2));
#else
	/* DC frequency */
	ValueFreqOffset->setText(QString().setNum(
		pDRMRec->GetParameters()->GetDCFrequency(), 'f', 2) + " Hz");
#endif

/* _WIN32 fix because in Visual c++ the GUI files are always compiled even
   if USE_QT_GUI is set or not (problem with MDI in DRMReceiver) */
#ifdef USE_QT_GUI
	/* If MDI in is enabled, do not show any synchronization parameter */
	if (pDRMRec->GetMDI()->GetMDIInEnabled() == TRUE)
	{
		ValueSNR->setText("<b>---</b>");
		ValueWiener->setText("--- / ---");
		ValueSampFreqOffset->setText("---");
		ValueFreqOffset->setText("---");
	}
#endif


	/* FAC info static ------------------------------------------------------ */
	QString strFACInfo;

	/* Robustness mode #################### */
	strFACInfo = GetRobModeStr() + " / " + GetSpecOccStr();

	FACDRMModeBWL->setText("DRM Mode / Bandwidth:"); /* Label */
	FACDRMModeBWV->setText(strFACInfo); /* Value */


	/* Interleaver Depth #################### */
	switch (pDRMRec->GetParameters()->eSymbolInterlMode)
	{
	case CParameter::SI_LONG:
		strFACInfo = tr("2 s (Long Interleaving)");
		break;

	case CParameter::SI_SHORT:
		strFACInfo = tr("400 ms (Short Interleaving)");
		break;
	}

	FACInterleaverDepthL->setText(tr("Interleaver Depth:")); /* Label */
	FACInterleaverDepthV->setText(strFACInfo); /* Value */


	/* SDC, MSC mode #################### */
	/* SDC */
	switch (pDRMRec->GetParameters()->eSDCCodingScheme)
	{
	case CParameter::CS_1_SM:
		strFACInfo = "4-QAM / ";
		break;

	case CParameter::CS_2_SM:
		strFACInfo = "16-QAM / ";
		break;
	}

	/* MSC */
	switch (pDRMRec->GetParameters()->eMSCCodingScheme)
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

	FACSDCMSCModeL->setText(tr("SDC / MSC Mode:")); /* Label */
	FACSDCMSCModeV->setText(strFACInfo); /* Value */


	/* Code rates #################### */
	strFACInfo = QString().setNum(pDRMRec->GetParameters()->MSCPrLe.iPartB);
	strFACInfo += " / ";
	strFACInfo += QString().setNum(pDRMRec->GetParameters()->MSCPrLe.iPartA);

	FACCodeRateL->setText(tr("Prot. Level (B / A):")); /* Label */
	FACCodeRateV->setText(strFACInfo); /* Value */


	/* Number of services #################### */
	strFACInfo = tr("Audio: ");
	strFACInfo += QString().setNum(pDRMRec->GetParameters()->iNumAudioService);
	strFACInfo += tr(" / Data: ");
	strFACInfo +=QString().setNum(pDRMRec->GetParameters()->iNumDataService);

	FACNumServicesL->setText(tr("Number of Services:")); /* Label */
	FACNumServicesV->setText(strFACInfo); /* Value */


	/* Time, date #################### */
	if ((pDRMRec->GetParameters()->iUTCHour == 0) &&
		(pDRMRec->GetParameters()->iUTCMin == 0) &&
		(pDRMRec->GetParameters()->iDay == 0) &&
		(pDRMRec->GetParameters()->iMonth == 0) &&
		(pDRMRec->GetParameters()->iYear == 0))
	{
		/* No time service available */
		strFACInfo = tr("Service not available");
	}
	else
	{
#ifdef GUI_QT_DATE_TIME_TYPE
		/* QT type of displaying date and time */
		QDateTime DateTime;
		DateTime.setDate(QDate(pDRMRec->GetParameters()->iYear,
			pDRMRec->GetParameters()->iMonth,
			pDRMRec->GetParameters()->iDay));
		DateTime.setTime(QTime(pDRMRec->GetParameters()->iUTCHour,
			pDRMRec->GetParameters()->iUTCMin));

		strFACInfo = DateTime.toString();
#else
		/* Set time and date */
		QString strMin;
		const int iMin = pDRMRec->GetParameters()->iUTCMin;

		/* Add leading zero to number smaller than 10 */
		if (iMin < 10)
			strMin = "0";
		else
			strMin = "";

		strMin += QString().setNum(iMin);

		strFACInfo =
			/* Time */
			QString().setNum(pDRMRec->GetParameters()->iUTCHour) + ":" +
			strMin + "  -  " +
			/* Date */
			QString().setNum(pDRMRec->GetParameters()->iMonth) + "/" +
			QString().setNum(pDRMRec->GetParameters()->iDay) + "/" +
			QString().setNum(pDRMRec->GetParameters()->iYear);
#endif
	}

	FACTimeDateL->setText(tr("Received time - date:")); /* Label */
	FACTimeDateV->setText(strFACInfo); /* Value */


	/* Update controls */
	UpdateControls();
}

void systemevalDlg::OnRadioTimeLinear() 
{
	if (pDRMRec->GetChanEst()->GetTimeInt() != CChannelEstimation::TLINEAR)
		pDRMRec->GetChanEst()->SetTimeInt(CChannelEstimation::TLINEAR);
}

void systemevalDlg::OnRadioTimeWiener() 
{
	if (pDRMRec->GetChanEst()->GetTimeInt() != CChannelEstimation::TWIENER)
		pDRMRec->GetChanEst()->SetTimeInt(CChannelEstimation::TWIENER);
}

void systemevalDlg::OnRadioFrequencyLinear() 
{
	if (pDRMRec->GetChanEst()->GetFreqInt() != CChannelEstimation::FLINEAR)
		pDRMRec->GetChanEst()->SetFreqInt(CChannelEstimation::FLINEAR);
}

void systemevalDlg::OnRadioFrequencyDft() 
{
	if (pDRMRec->GetChanEst()->GetFreqInt() != CChannelEstimation::FDFTFILTER)
		pDRMRec->GetChanEst()->SetFreqInt(CChannelEstimation::FDFTFILTER);
}

void systemevalDlg::OnRadioFrequencyWiener() 
{
	if (pDRMRec->GetChanEst()->GetFreqInt() != CChannelEstimation::FWIENER)
		pDRMRec->GetChanEst()->SetFreqInt(CChannelEstimation::FWIENER);
}

void systemevalDlg::OnRadioTiSyncFirstPeak() 
{
	if (pDRMRec->GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType() != 
		CTimeSyncTrack::TSFIRSTPEAK)
	{
		pDRMRec->GetChanEst()->GetTimeSyncTrack()->
			SetTiSyncTracType(CTimeSyncTrack::TSFIRSTPEAK);
	}
}

void systemevalDlg::OnRadioTiSyncEnergy() 
{
	if (pDRMRec->GetChanEst()->GetTimeSyncTrack()->GetTiSyncTracType() != 
		CTimeSyncTrack::TSENERGY)
	{
		pDRMRec->GetChanEst()->GetTimeSyncTrack()->
			SetTiSyncTracType(CTimeSyncTrack::TSENERGY);
	}
}

void systemevalDlg::OnSliderIterChange(int value)
{
	/* Set new value in working thread module */
	pDRMRec->GetMSCMLC()->SetNumIterations(value);

	/* Show the new value in the label control */
	TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
		QString().setNum(value));
}

void systemevalDlg::OnListSelChanged(QListViewItem* NewSelIt)
{
	/* Get char type from selected item and setup chart */
	SetupChart(((CCharSelItem*) NewSelIt)->GetCharType());
}

void systemevalDlg::OnCheckFlipSpectrum()
{
	/* Set parameter in working thread module */
	pDRMRec->GetReceiver()->
		SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
}

void systemevalDlg::OnCheckRecFilter()
{
	/* Set parameter in working thread module */
	pDRMRec->GetOFDMDemod()->
		SetRecFilter(CheckBoxRecFilter->isChecked());
}

void systemevalDlg::OnCheckModiMetric()
{
	/* Set parameter in working thread module */
	pDRMRec->GetChanEst()->SetIntCons(CheckBoxModiMetric->isChecked());
}

void systemevalDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	pDRMRec->GetWriteData()->MuteAudio(CheckBoxMuteAudio->isChecked());
}

void systemevalDlg::OnCheckSaveAudioWAV()
{
/*
	This code is copied in AnalogDemDlg.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (CheckBoxSaveAudioWave->isChecked() == TRUE)
	{
		/* Show "save file" dialog */
		QString strFileName =
			QFileDialog::getSaveFileName(tr("DreamOut.wav"), "*.wav", this);

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

void systemevalDlg::OnTimerLogFileStart()
{
	/* Start logging (if not already done) */
	if (!CheckBoxWriteLog->isChecked())
	{
		CheckBoxWriteLog->setChecked(TRUE);
		OnCheckWriteLog();
	}
}

void systemevalDlg::OnCheckWriteLog()
{
	if (CheckBoxWriteLog->isChecked())
	{
		/* Activte log file timer for long and short log file */
		TimerLogFileShort.start(60000); /* Every minute (i.e. 60000 ms) */
		TimerLogFileLong.start(1000); /* Every second */

		/* Get frequency from front-end edit control */
		QString strFreq = EdtFrequency->text();
		iCurFrequency = strFreq.toUInt();
		pDRMRec->GetParameters()->ReceptLog.SetFrequency(iCurFrequency);

		/* Set some other information obout this receiption */
		QString strAddText = "";

		/* Check if receiver does receive a DRM signal */
		if ((pDRMRec->GetReceiverState() == CDRMReceiver::AS_WITH_SIGNAL) &&
			(pDRMRec->GetReceiverMode() == CDRMReceiver::RM_DRM))
		{
			/* First get current selected audio service */
			int iCurSelServ =
				pDRMRec->GetParameters()->GetCurSelAudioService();

			/* Check whether service parameters were not transmitted yet */
			if (pDRMRec->GetParameters()->Service[iCurSelServ].IsActive())
			{
				strAddText = tr("Label            ");

				/* Service label (UTF-8 encoded string -> convert) */
				strAddText += QString().fromUtf8(QCString(
					pDRMRec->GetParameters()->Service[iCurSelServ].
					strLabel.c_str()));

				strAddText += tr("\nBitrate          ");

				strAddText += QString().setNum(pDRMRec->GetParameters()->
					GetBitRateKbps(iCurSelServ, FALSE), 'f', 2) + " kbps";

				strAddText += tr("\nMode             ") + GetRobModeStr();
				strAddText += tr("\nBandwidth        ") + GetSpecOccStr();
			}
		}

		/* Set additional text for log file. Conversion from QString to STL
		   string is needed (done with .latin1() function of QT string) */
		string strTemp = strAddText.latin1();
		pDRMRec->GetParameters()->ReceptLog.SetAdditText(strTemp);

		/* Activate log file */
		pDRMRec->GetParameters()->ReceptLog.SetLog(TRUE);
	}
	else
	{
		/* Deactivate log file timer */
		TimerLogFileShort.stop();
		TimerLogFileLong.stop();

		pDRMRec->GetParameters()->ReceptLog.SetLog(FALSE);
	}
}

void systemevalDlg::OnTimerLogFileShort()
{
	/* Write new parameters in log file (short version) */
	pDRMRec->GetParameters()->ReceptLog.WriteParameters(FALSE);
}

void systemevalDlg::OnTimerLogFileLong()
{
	/* Write new parameters in log file (long version) */
	pDRMRec->GetParameters()->ReceptLog.WriteParameters(TRUE);
}

QString	systemevalDlg::GetRobModeStr()
{
	switch (pDRMRec->GetParameters()->GetWaveMode())
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
	switch (pDRMRec->GetParameters()->GetSpectrumOccup())
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

void systemevalDlg::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* DC Frequency Offset */
	QWhatsThis::add(TextFreqOffset,
		"<b>" + tr("DC Frequency Offset:") + "</b> " + tr("This is the "
		"estimation of the DC frequency offset. This offset corresponds "
		"to the resulting sound card intermedia frequency of the front-end. "
		"This frequency is not restricted to a certain value. The only "
		"restriction is that the DRM spectrum must be completely inside the "
		"bandwidth of the sound card."));

	/* Sample Frequency Offset */
	QWhatsThis::add(TextSampFreqOffset,
		"<b>" + tr("Sample Frequency Offset:") + "</b> " + tr("This is the "
		"estimation of the sample rate offset between the sound card sample "
		"rate of the local computer and the sample rate of the D / A (digital "
		"to analog) converter in the transmitter. Usually the sample rate "
		"offset is very constant for a given sound card. Therefore it is "
		"useful to inform the Dream software about this value at application "
		"startup to increase the acquisition speed and reliability."));

	/* Doppler / Delay */
	QWhatsThis::add(TextWiener,
		"<b>" + tr("Doppler / Delay:") + "</b> " + tr("The Doppler frequency "
		"of the channel is estimated for the Wiener filter design of channel "
		"estimation in time direction. If linear interpolation is set for "
		"channel estimation in time direction, this estimation is not updated. "
		"The Doppler frequency is an indication of how fast the channel varies "
		"with time. The higher the frequency, the faster the channel changes "
		"are.") + "<br>" + tr("The total delay of the Power Delay Spectrum "
		"(PDS) is estimated from the impulse response estimation derived from "
		"the channel estimation. This delay corresponds to the range between "
		"the two vertical dashed black lines in the Impulse Response (IR) "
		"plot."));

	/* I / O Interface LED */
	const QString strLEDIOInterface =
		"<b>" + tr("I / O Interface LED:") + "</b> " + tr("This LED shows the "
		"current status of the sound card interface. The yellow light shows "
		"that the audio output was corrected. Since the sample rate of the "
		"transmitter and local computer are different, from time to time the "
		"audio buffers will overflow or under run and a correction is "
		"necessary. When a correction occurs, a \"click\" sound can be heard. "
		"The red light shows that a buffer was lost in the sound card input "
		"stream. This can happen if a thread with a higher priority is at "
		"100% and the Dream software cannot read the provided blocks fast "
		"enough. In this case, the Dream software will instantly loose the "
		"synchronization and has to re-synchronize. Another reason for red "
		"light is that the processor is too slow for running the Dream "
		"software.");

	QWhatsThis::add(TextLabelLEDIOInterface, strLEDIOInterface);
	QWhatsThis::add(LEDIOInterface, strLEDIOInterface);

	/* Time Sync Acq LED */
	const QString strLEDTimeSyncAcq =
		"<b>" + tr("Time Sync Acq LED:") + "</b> " + tr("This LED shows the "
		"state of the timing acquisition (search for the beginning of an OFDM "
		"symbol). If the acquisition is done, this LED will stay green.");

	QWhatsThis::add(TextLabelLEDTimeSyncAcq, strLEDTimeSyncAcq);
	QWhatsThis::add(LEDTimeSync, strLEDTimeSyncAcq);

	/* Frame Sync LED */
	const QString strLEDFrameSync =
		"<b>" + tr("Frame Sync LED:") + "</b> " + tr("The DRM frame "
		"synchronization status is shown with this LED. This LED is also only "
		"active during acquisition state of the Dream receiver. In tracking "
		"mode, this LED is always green.");

	QWhatsThis::add(TextLabelLEDFrameSync, strLEDFrameSync);
	QWhatsThis::add(LEDFrameSync, strLEDFrameSync);

	/* FAC CRC LED */
	const QString strLEDFACCRC =
		"<b>" + tr("FAC CRC LED:") + "</b> " + tr("This LED shows the Cyclic "
		"Redundancy Check (CRC) of the Fast Access Channel (FAC) of DRM. FAC "
		"is one of the three logical channels and is always modulated with a "
		"4-QAM. If the FAC CRC check was successful, the receiver changes to "
		"tracking mode. The FAC LED is the indication whether the receiver "
		"is synchronized to a DRM transmission or not.");

	QWhatsThis::add(TextLabelLEDFACCRC, strLEDFACCRC);
	QWhatsThis::add(LEDFAC, strLEDFACCRC);

	/* SDC CRC LED */
	const QString strLEDSDCCRC =
		"<b>" + tr("SDC CRC LED:") + "</b> " + tr("This LED shows the CRC "
		"check result of the Service Description Channel (SDC) which is one "
		"logical channel of the DRM stream. This data is transmitted in "
		"approx. 1 second intervals and contains information about station "
		"label, audio and data format, etc. The error protection is normally "
		"lower than the protection of the FAC. Therefore this LED will turn "
		"to red earlier than the FAC LED in general.");

	QWhatsThis::add(TextLabelLEDSDCCRC, strLEDSDCCRC);
	QWhatsThis::add(LEDSDC, strLEDSDCCRC);

	/* MSC CRC LED */
	const QString strLEDMSCCRC =
		"<b>" + tr("MSC CRC LED:") + "</b> " + tr("This LED shows the status "
		"of the Main Service Channel (MSC). This channel contains the actual "
		"audio and data bits. The LED shows the CRC check of the AAC core "
		"decoder. The SBR has a separate CRC, but this status is not shown "
		"with this LED. If SBR CRC is wrong but the AAC CRC is ok one can "
		"still hear something (of course, the high frequencies are not there "
		"in this case). If this LED turns red, interruptions of the audio are "
		"heard. The yellow light shows that only one 40 ms audio frame CRC "
		"was wrong. This causes usually no hearable artifacts.");

	QWhatsThis::add(TextLabelLEDMSCCRC, strLEDMSCCRC);
	QWhatsThis::add(LEDMSC, strLEDMSCCRC);

	/* MLC, Number of Iterations */
	const QString strNumOfIterations =
		"<b>" + tr("MLC, Number of Iterations:") + "</b> " + tr("In DRM, a "
		"multilevel channel coder is used. With this code it is possible to "
		"iterate the decoding process in the decoder to improve the decoding "
		"result. The more iterations are used the better the result will be. "
		"But switching to more iterations will increase the CPU load. "
		"Simulations showed that the first iteration (number of "
		"iterations = 1) gives the most improvement (approx. 1.5 dB at a "
		"BER of 10-4 on a Gaussian channel, Mode A, 10 kHz bandwidth). The "
		"improvement of the second iteration will be as small as 0.3 dB.") +
		"<br>" + tr("The recommended number of iterations given in the DRM "
		"standard is one iteration (number of iterations = 1).");

	QWhatsThis::add(TextNumOfIterations, strNumOfIterations);
	QWhatsThis::add(SliderNoOfIterations, strNumOfIterations);

	/* Flip Input Spectrum */
	QWhatsThis::add(CheckBoxFlipSpec,
		"<b>" + tr("Flip Input Spectrum:") + "</b> " + tr("Checking this box "
		"will flip or invert the input spectrum. This is necessary if the "
		"mixer in the front-end uses the lower side band."));

	/* Mute Audio */
	QWhatsThis::add(CheckBoxMuteAudio,
		"<b>" + tr("Mute Audio:") + "</b> " + tr("The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Log File */
	QWhatsThis::add(CheckBoxWriteLog,
		"<b>" + tr("Log File:") + "</b> " + tr("Checking this box brings the "
		"Dream software to write a log file about the current reception. "
		"Every minute the average SNR, number of correct decoded FAC and "
		"number of correct decoded MSC blocks are logged including some "
		"additional information, e.g. the station label and bit-rate. The "
		"log mechanism works only for audio services using AAC source coding. "
#ifdef _WIN32
		"During the logging no Dream windows "
		"should be moved or re-sized. This can lead to incorrect log files "
		"(problem with QT timer implementation under Windows). This problem "
		"does not exist in the Linux version of Dream."
#endif
		) + "<br>" + tr("The log file will be "
		"written in the directory were the Dream application was started and "
		"the name of this file is always DreamLog.txt"));

	/* Freq */
	QWhatsThis::add(EdtFrequency,
		"<b>" + tr("Freq:") + "</b> " + tr("In this edit control, the current "
		"selected frequency on the front-end can be specified. This frequency "
		"will be written into the log file."));

	/* Wiener */
	const QString strWienerChanEst =
		"<b>" + tr("Channel Estimation Settings:") + "</b> " + tr("With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.") + "<br>"
		"<b>" + tr("Wiener:") + "</b> " + tr("Wiener interpolation method "
		"uses estimation of the statistics of the channel to design an optimal "
		"filter for noise reduction.");

	QWhatsThis::add(RadioButtonFreqWiener, strWienerChanEst);
	QWhatsThis::add(RadioButtonTiWiener, strWienerChanEst);

	/* Linear */
	const QString strLinearChanEst =
		"<b>" + tr("Channel Estimation Settings:") + "</b> " + tr("With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.") + "<br>"
		"<b>" + tr("Linear:") + "</b> " + tr("Simple linear interpolation "
		"method to get the channel estimate. The real and imaginary parts "
		"of the estimated channel at the pilot positions are linearly "
		"interpolated. This algorithm causes the lowest CPU load but "
		"performs much worse than the Wiener interpolation at low SNRs.");

	QWhatsThis::add(RadioButtonFreqLinear, strLinearChanEst);
	QWhatsThis::add(RadioButtonTiLinear, strLinearChanEst);

	/* DFT Zero Pad */
	QWhatsThis::add(RadioButtonFreqDFT,
		"<b>" + tr("Channel Estimation Settings:") + "</b> " + tr("With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.") + "<br>"
		"<b>" + tr("DFT Zero Pad:") + "</b> " + tr("Channel estimation method "
		"for the frequency direction using Discrete Fourier Transformation "
		"(DFT) to transform the channel estimation at the pilot positions to "
		"the time domain. There, a zero padding is applied to get a higher "
		"resolution in the frequency domain -> estimates at the data cells. "
		"This algorithm is very speed efficient but has problems at the edges "
		"of the OFDM spectrum due to the leakage effect."));

	/* Guard Energy */
	QWhatsThis::add(RadioButtonTiSyncEnergy,
		"<b>" + tr("Guard Energy:") + "</b> " + tr("Time synchronization "
		"tracking algorithm utilizes the estimation of the impulse response. "
		"This method tries to maximize the energy in the guard-interval to set "
		"the correct timing."));

	/* First Peak */
	QWhatsThis::add(RadioButtonTiSyncFirstPeak,
		"<b>" + tr("First Peak:") + "</b> " + tr("This algorithms searches for "
		"the first peak in the estimated impulse response and moves this peak "
		"to the beginning of the guard-interval (timing tracking algorithm)."));

	/* SNR */
	const QString strSNREst =
		"<b>" + tr("SNR:") + "</b> " + tr("Signal to Noise Ratio (SNR) "
		"estimation based on FAC cells. Since the FAC cells are only "
		"located approximately in the region 0-5 kHz relative to the DRM DC "
		"frequency, it may happen that the SNR value is very high "
		"although the DRM spectrum on the left side of the DRM DC frequency "
		"is heavily distorted or disturbed by an interferer so that the true "
		"overall SNR is lower as indicated by the SNR value. Similarly, "
		"the SNR value might show a very low value but audio can still be "
		"decoded if only the right side of the DRM spectrum is degraded "
		"by an interferer.");

	QWhatsThis::add(ValueSNR, strSNREst);
	QWhatsThis::add(TextSNRText, strSNREst);

	/* DRM Mode / Bandwidth */
	const QString strRobustnessMode =
		"<b>" + tr("DRM Mode / Bandwidth:") + "</b> " + tr("In a DRM system, "
		"four possible robustness modes are defined to adapt the system to "
		"different channel conditions. According to the DRM standard:") + "<ul>"
		"<li><i>" + tr("Mode A:") + "</i> " + tr("Gaussian channels, with "
		"minor fading") + "</li><li><i>" + tr("Mode B:") + "</i> " + tr("Time "
		"and frequency selective channels, with longer delay spread") + "</li>"
		"<li><i>" + tr("Mode C:") + "</i> " + tr("As robustness mode B, but "
		"with higher Doppler spread") + "</li>"
		"<li><i>" + tr("Mode D:") + "</i> " + tr("As robustness mode B, but "
		"with severe delay and Doppler spread") + "</li></ul>" + tr("The "
		"bandwith is the gross bandwidth of the current DRM signal");

	QWhatsThis::add(FACDRMModeBWL, strRobustnessMode);
	QWhatsThis::add(FACDRMModeBWV, strRobustnessMode);

	/* Interleaver Depth */
	const QString strInterleaver =
		"<b>" + tr("Interleaver Depth:") + "</b> " + tr("The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the "
		"longer the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	QWhatsThis::add(FACInterleaverDepthL, strInterleaver);
	QWhatsThis::add(FACInterleaverDepthV, strInterleaver);

	/* SDC / MSC Mode */
	const QString strSDCMSCMode =
		"<b>" + tr("SDC / MSC Mode:") + "</b> " + tr("Shows the modulation "
		"type of the SDC and MSC channel. For the MSC channel, some "
		"hierarchical modes are defined which can provide a very strong "
		"protected service channel.");

	QWhatsThis::add(FACSDCMSCModeL, strSDCMSCMode);
	QWhatsThis::add(FACSDCMSCModeV, strSDCMSCMode);

	/* Prot. Level (B/A) */
	const QString strProtLevel =
		"<b>" + tr("Prot. Level (B/A):") + "</b> " + tr("The error protection "
		"level of the channel coder. For 64-QAM, there are four protection "
		"levels defined in the DRM standard. Protection level 0 has the "
		"highest protection whereas level 3 has the lowest protection. The "
		"letters A and B are the names of the higher and lower protected parts "
		"of a DRM block when Unequal Error Protection (UEP) is used. If Equal "
		"Error Protection (EEP) is used, only the protection level of part B "
		"is valid.");

	QWhatsThis::add(FACCodeRateL, strProtLevel);
	QWhatsThis::add(FACCodeRateV, strProtLevel);

	/* Number of Services */
	const QString strNumServices =
		"<b>" + tr("Number of Services:") + "</b> " + tr("This shows the "
		"number of audio and data services transmitted in the DRM stream. "
		"The maximum number of streams is four.");

	QWhatsThis::add(FACNumServicesL, strNumServices);
	QWhatsThis::add(FACNumServicesV, strNumServices);

	/* Received time - date */
	const QString strTimeDate =
		"<b>" + tr("Received time - date:") + "</b> " + tr("This label shows "
		"the received time and date in UTC. This information is carried in "
		"the SDC channel.");

	QWhatsThis::add(FACTimeDateL, strTimeDate);
	QWhatsThis::add(FACTimeDateV, strTimeDate);

	/* Save audio as wave */
	QWhatsThis::add(CheckBoxSaveAudioWave,
		"<b>" + tr("Save Audio as WAV:") + "</b> " + tr("Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording."));

	/* Chart Selector */
	QWhatsThis::add(ListViewCharSel,
		"<b>" + tr("Chart Selector:") + "</b> " + tr("With the chart selector "
		"different types of graphical display of parameters and receiver "
		"states can be chosen. The different plot types are sorted in "
		"different groups. To open a group just double-click on the group or "
		"click on the plus left of the group name. After clicking on an item "
		"it is possible to choose other items by using the up / down arrow "
		"keys. With these keys it is also possible to open and close the "
		"groups by using the left / right arrow keys."));

	/* Interferer Rejection */
	const QString strInterfRej =
		"<b>" + tr("Interferer Rejection:") + "</b> " + tr("There are two "
		"algorithms available to reject interferers:") + "<ul>" +
		"<li><b>" + tr("Bandpass filter (BP-Filter):") + "</b>" +
		tr("The bandpass filter is designed to have the same bandwidth as "
		"the DRM signal. If, e.g., a strong signal is close to the border "
		"of the actual DRM signal, under some conditions this signal will "
		"produce interference in the useful bandwidth of the DRM signal "
		"although it is not on the same frequency as the DRM signal. "
		"The reason for that behaviour lies in the way the OFDM "
		"demodulation is done. Since OFDM demodulation is a block-wise "
		"operation, a windowing has to be applied (which is rectangular "
		"in case of OFDM). As a result, the spectrum of a signal is "
		"convoluted with a Sinc function in the frequency domain. If a "
		"sinusoidal signal close to the border of the DRM signal is "
		"considered, its spectrum will not be a distinct peak but a "
		"shifted Sinc function. So its spectrum is broadened caused by "
		"the windowing. Thus, it will spread in the DRM spectrum and "
		"act as an in-band interferer.") + "<br>" +
		tr("There is a special case if the sinusoidal signal is in a "
		"distance of a multiple of the carrier spacing of the DRM signal. "
		"Since the Sinc function has zeros at certain positions it happens "
		"that in this case the zeros are exactly at the sub-carrier "
		"frequencies of the DRM signal. In this case, no interference takes "
		"place. If the sinusoidal signal is in a distance of a multiple of "
		"the carrier spacing plus half of the carrier spacing away from the "
		"DRM signal, the interference reaches its maximum.") + "<br>" +
		tr("As a result, if only one DRM signal is present in the 20 kHz "
		"bandwidth, bandpass filtering has no effect. Also,  if the "
		"interferer is far away from the DRM signal, filtering will not "
		"give much improvement since the squared magnitude of the spectrum "
		"of the Sinc function is approx -15 dB down at 1 1/2 carrier "
		"spacing (approx 70 Hz with DRM mode B) and goes down to approx "
		"-30 dB at 10 times the carrier spacing plus 1 / 2 of the carrier "
		"spacing (approx 525 Hz with DRM mode B). The bandpass filter must "
		"have very sharp edges otherwise the gain in performance will be "
		"very small so that it consumes high CPU power.") + "</li>" +
		"<li><b>" + tr("Modificated Metric:") + "</b>" + tr("Based on the "
		"information from the SNR versus sub-carrier estimation, the metric "
		"for the Viterbi decoder can be modified so that sub-carriers with "
		"high noise are attenuated and do not contribute too much to the "
		"decoding result. That can improve reception under bad conditions but "
		"may worsen the reception in situations where a lot of fading happens "
		"and no interferer are present since the SNR estimation may be "
		"not correct.") + "</li></ul>";

	QWhatsThis::add(GroupBoxInterfRej, strInterfRej);
	QWhatsThis::add(CheckBoxRecFilter, strInterfRej);
	QWhatsThis::add(CheckBoxModiMetric, strInterfRej);
}

void systemevalDlg::AddWhatsThisHelpChar(const ECharType NCharType)
{
	QString strCurPlotHelp;

	switch (NCharType)
	{
	case AVERAGED_IR:
		/* Impulse Response */
		strCurPlotHelp =
			"<b>" + tr("Impulse Response:") + "</b> " + tr("This plot shows "
			"the estimated Impulse Response (IR) of the channel based on the "
			"channel estimation. It is the averaged, Hamming Window weighted "
			"Fourier back transformation of the transfer function. The length "
			"of PDS estimation and time synchronization tracking is based on "
			"this function. The two red dashed vertical lines show the "
			"beginning and the end of the guard-interval. The two black dashed "
			"vertical lines show the estimated beginning and end of the PDS of "
			"the channel (derived from the averaged impulse response "
			"estimation). If the \"First Peak\" timing tracking method is "
			"chosen, a bound for peak estimation (horizontal dashed red line) "
			"is shown. Only peaks above this bound are used for timing "
			"estimation.");
		break;

	case TRANSFERFUNCTION:
		/* Transfer Function */
		strCurPlotHelp =
			"<b>" + tr("Transfer Function / Group Delay:") + "</b> " +
			tr("This plot shows the squared magnitude and the group delay of "
			"the estimated channel at each sub-carrier.");
		break;

	case FAC_CONSTELLATION:
	case SDC_CONSTELLATION:
	case MSC_CONSTELLATION:
	case ALL_CONSTELLATION:
		/* Constellations */
		strCurPlotHelp =
			"<b>" + tr("FAC, SDC, MSC:") + "</b> " + tr("The plots show the "
			"constellations of the FAC, SDC and MSC logical channel of the DRM "
			"stream. Depending on the current transmitter settings, the SDC "
			"and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
		break;

	case POWER_SPEC_DENSITY:
		/* Shifted PSD */
		strCurPlotHelp =
			"<b>" + tr("Shifted PSD:") + "</b> " + tr("This plot shows the "
			"estimated Power Spectrum Density (PSD) of the input signal. The "
			"DC frequency (red dashed vertical line) is fixed at 6 kHz. If "
			"the frequency offset acquisition was successful, the rectangular "
			"DRM spectrum should show up with a center frequency of 6 kHz. "
			"This plot represents the frequency synchronized OFDM spectrum. "
			"If the frequency synchronization was successful, the useful "
			"signal really shows up only inside the actual DRM bandwidth "
			"since the side loops have in this case only energy between the "
			"samples in the frequency domain. On the sample positions outside "
			"the actual DRM spectrum, the DRM signal has zero crossings "
			"because of the orthogonality. Therefore this spectrum represents "
			"NOT the actual spectrum but the \"idealized\" OFDM spectrum.");
		break;

	case SNR_SPECTRUM:
		/* SNR Spectrum (Weighted MER on MSC Cells) */
		strCurPlotHelp =
			"<b>" + tr("SNR Spectrum (Weighted MER on MSC Cells):") + "</b> " +
			tr("This plot shows the Weighted MER on MSC cells for each carrier "
			"separately.");
		break;

	case INPUTSPECTRUM_NO_AV:
		/* Input Spectrum */
		strCurPlotHelp =
			"<b>" + tr("Input Spectrum:") + "</b> " + tr("This plot shows the "
			"Fast Fourier Transformation (FFT) of the input signal. This plot "
			"is active in both modes, analog and digital. There is no "
			"averaging applied. The screen shot of the Evaluation Dialog shows "
			"the significant shape of a DRM signal (almost rectangular). The "
			"dashed vertical line shows the estimated DC frequency. This line "
			"is very important for the analog AM demodulation. Each time a "
			"new carrier frequency is acquired, the red line shows the "
			"selected AM spectrum. If more than one AM spectrums are within "
			"the sound card frequency range, the strongest signal is chosen.");
		break;

	case INPUT_SIG_PSD:
		/* Input PSD */
		strCurPlotHelp =
			"<b>" + tr("Input PSD:") + "</b> " + tr("This plot shows the "
			"estimated power spectral density (PSD) of the input signal. The "
			"PSD is estimated by averaging some Hamming Window weighted "
			"Fourier transformed blocks of the input signal samples. The "
			"dashed vertical line shows the estimated DC frequency.");
		break;

	case AUDIO_SPECTRUM:
		/* Audio Spectrum */
		strCurPlotHelp =
			"<b>" + tr("Audio Spectrum:") + "</b> " + tr("This plot shows the "
			"averaged audio spectrum of the currently played audio. With this "
			"plot the actual audio bandwidth can easily determined. Since a "
			"linear scale is used for the frequency axis, most of the energy "
			"of the signal is usually concentrated on the far left side of the "
			"spectrum.");
		break;

	case FREQ_SAM_OFFS_HIST:
		/* Frequency Offset / Sample Rate Offset History */
		strCurPlotHelp =
			"<b>" + tr("Frequency Offset / Sample Rate Offset History:") +
			"</b> " + tr("The history "
			"of the values for frequency offset and sample rate offset "
			"estimation is shown. If the frequency offset drift is very small, "
			"this is an indication that the analog front end is of high "
			"quality.");
		break;

	case DOPPLER_DELAY_HIST:
		/* Doppler / Delay History */
		strCurPlotHelp =
			"<b>" + tr("Doppler / Delay History:") + "</b> " +
			tr("The history of the values for the "
			"Doppler and Impulse response length is shown. Large Doppler "
			"values might be responsable for audio drop-outs.");
		break;

	case SNR_AUDIO_HIST:
		/* SNR History */
		strCurPlotHelp =
			"<b>" + tr("SNR History:") + "</b> " +
			tr("The history of the values for the "
			"SNR and correctly decoded audio blocks is shown. The maximum "
			"achievable number of correctly decoded audio blocks per DRM "
			"frame is 10 or 5 depending on the audio sample rate (24 kHz "
			"or 12 kHz AAC core bandwidth).");
		break;
	}

	/* Main plot */
	QWhatsThis::add(MainPlot, strCurPlotHelp);
}
