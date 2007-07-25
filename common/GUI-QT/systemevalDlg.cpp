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
#include "../GPSReceiver.h"
#include <qmessagebox.h>
#include <qgroupbox.h>

/* Implementation *************************************************************/
systemevalDlg::systemevalDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f) :
	systemevalDlgBase(parent, name, modal, f),
	DRMReceiver(NDRMR),
	Settings(NSettings),
	Timer(), TimerLogFileLong(), TimerLogFileShort(), TimerLogFileStart(),
	shortLog(*NDRMR.GetParameters()), longLog(*NDRMR.GetParameters()),
	bEnableShortLog(FALSE), bEnableLongLog(FALSE),
	iLogDelay(0), iCurFrequency(0), pGPSReceiver(NULL)
{
	/* Get window geometry data and apply it */
	CWinGeom s;
	Settings.Get("System Evaluation Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			setGeometry(WinGeom);

	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Init controls -------------------------------------------------------- */
	/* Init main plot */
	int iScheme = Settings.Get("System Evaluation Dialog", "colorscheme", 0);
	Settings.Put("System Evaluation Dialog", "colorscheme", iScheme);
	MainPlot->SetRecObj(&DRMReceiver);
	MainPlot->SetPlotStyle(iScheme);
	MainPlot->setMargin(1);

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
	/* Get pixmaps from dummy list view entries which where inserted in the
	   qdesigner environment (storage container for the pixmaps) */
	QListViewItem* pCurLiViIt = ListViewCharSel->firstChild();
	const QPixmap pixSpectrum(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrPSD(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrInpSpec(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrWaterf(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrShiftedPSD(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrAudio(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSpectrSNR(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixChannel(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixChannelIR(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixChannelTF(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixConstellation(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixFAC(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSDC(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixMSC(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixHistory(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixSNRAudHist(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixDelDoppHist(*pCurLiViIt->pixmap(0));
	pCurLiViIt = pCurLiViIt->firstChild();
	const QPixmap pixFreqSamHist(*pCurLiViIt->pixmap(0));

	/* Now clear the dummy list view items */	
	ListViewCharSel->clear();

	/* No sorting of items */
	ListViewCharSel->setSorting(-1);

	/* Insert parent list view items. Parent list view items should not be
	   selectable */
	CCharSelItem* pHistoryLiViIt =
		new CCharSelItem(ListViewCharSel, tr("History"),
		CDRMPlot::NONE_OLD, FALSE);
	pHistoryLiViIt->setPixmap(0, pixHistory);

	CCharSelItem* pConstellationLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Constellation"),
		CDRMPlot::NONE_OLD, FALSE);
	pConstellationLiViIt->setPixmap(0, pixConstellation);

	CCharSelItem* pChannelLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Channel"),
		CDRMPlot::NONE_OLD, FALSE);
	pChannelLiViIt->setPixmap(0, pixChannel);

	CCharSelItem* pSpectrumLiViIt =
		new CCharSelItem(ListViewCharSel, tr("Spectrum"),
		CDRMPlot::NONE_OLD, FALSE);
	pSpectrumLiViIt->setPixmap(0, pixSpectrum);

 
	/* Inser actual items. The list is not sorted -> items which are inserted
	   first show up at the end of the list */
	/* Spectrum */
	CCharSelItem* pListItSNRSpec = new CCharSelItem(pSpectrumLiViIt,
		tr("SNR Spectrum"), CDRMPlot::SNR_SPECTRUM);
	pListItSNRSpec->setPixmap(0, pixSpectrSNR);
	CCharSelItem* pListItAudSpec = new CCharSelItem(pSpectrumLiViIt,
		tr("Audio Spectrum"), CDRMPlot::AUDIO_SPECTRUM);
	pListItAudSpec->setPixmap(0, pixSpectrAudio);
	CCharSelItem* pListItPowSpecDens = new CCharSelItem(pSpectrumLiViIt,
		tr("Shifted PSD"), CDRMPlot::POWER_SPEC_DENSITY);
	pListItPowSpecDens->setPixmap(0, pixSpectrShiftedPSD);
	CCharSelItem* pListItInpSpecWater = new CCharSelItem(pSpectrumLiViIt,
		tr("Waterfall Input Spectrum"), CDRMPlot::INP_SPEC_WATERF);
	pListItInpSpecWater->setPixmap(0, pixSpectrWaterf);
	CCharSelItem* pListItInpSpectrNoAv = new CCharSelItem(pSpectrumLiViIt,
		tr("Input Spectrum"), CDRMPlot::INPUTSPECTRUM_NO_AV);
	pListItInpSpectrNoAv->setPixmap(0, pixSpectrInpSpec);
	CCharSelItem* pListItInpPSD = new CCharSelItem(pSpectrumLiViIt,
		tr("Input PSD"), CDRMPlot::INPUT_SIG_PSD);
	pListItInpPSD->setPixmap(0, pixSpectrPSD);

	/* Constellation */
	CCharSelItem* pListItConstMSC = new CCharSelItem(pConstellationLiViIt,
		tr("MSC"), CDRMPlot::MSC_CONSTELLATION);
	pListItConstMSC->setPixmap(0, pixMSC);
	CCharSelItem* pListItConstSDC = new CCharSelItem(pConstellationLiViIt,
		tr("SDC"), CDRMPlot::SDC_CONSTELLATION);
	pListItConstSDC->setPixmap(0, pixSDC);
	CCharSelItem* pListItConstFAC = new CCharSelItem(pConstellationLiViIt,
		tr("FAC"), CDRMPlot::FAC_CONSTELLATION);
	pListItConstFAC->setPixmap(0, pixFAC);
	CCharSelItem* pListItConstAll = new CCharSelItem(pConstellationLiViIt,
		tr("FAC / SDC / MSC"), CDRMPlot::ALL_CONSTELLATION);
	pListItConstAll->setPixmap(0, pixConstellation);

	/* History */
	CCharSelItem* pListItHistFrSa = new CCharSelItem(pHistoryLiViIt,
		tr("Frequency / Sample Rate"), CDRMPlot::FREQ_SAM_OFFS_HIST);
	pListItHistFrSa->setPixmap(0, pixFreqSamHist);
	CCharSelItem* pListItHistDeDo = new CCharSelItem(pHistoryLiViIt,
		tr("Delay / Doppler"), CDRMPlot::DOPPLER_DELAY_HIST);
	pListItHistDeDo->setPixmap(0, pixDelDoppHist);
	CCharSelItem* pListItHistSNRAu = new CCharSelItem(pHistoryLiViIt,
		tr("SNR / Audio"), CDRMPlot::SNR_AUDIO_HIST);
	pListItHistSNRAu->setPixmap(0, pixSNRAudHist);

	/* Channel */
	CCharSelItem* pListItChanTF = new CCharSelItem(pChannelLiViIt,
		tr("Transfer Function"), CDRMPlot::TRANSFERFUNCTION);
	pListItChanTF->setPixmap(0, pixChannelTF);
	CCharSelItem* pListItChanIR = new CCharSelItem(pChannelLiViIt,
		tr("Impulse Response"), CDRMPlot::AVERAGED_IR);
	pListItChanIR->setPixmap(0, pixChannelIR);

	/* Use this trick to update the automatic column width adjustment to the
	   new items inserted above. If we do not do the update, the column width
	   is much larger than desired because of the dummy items inserted for
	   storing the pixmaps in the QDesigner */
	ListViewCharSel->setColumnWidth(0, 0);
	ListViewCharSel->setColumnWidthMode(0, QListView::Maximum);


	/* If RSCI in is enabled, disable some of the controls and use different
	   initialization for the chart and chart selector */
	if (DRMReceiver.GetRSIIn()->GetInEnabled() == TRUE)
	{
		//ListViewCharSel->setEnabled(FALSE);

		EdtFrequency->setText("0");
		EdtFrequency->setEnabled(FALSE);

		/* Only audio spectrum makes sence for MDI in */
		ListViewCharSel->setSelected(pListItAudSpec, TRUE);
		ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
		MainPlot->SetupChart(CDRMPlot::AUDIO_SPECTRUM);
	}
	else
	{
		int iSysEvalDlgPlotType = Settings.Get("System Evaluation Dialog", "sysevplottype", 0);
		/* Set chart type */
		switch (iSysEvalDlgPlotType)
		{
		case (int) CDRMPlot::POWER_SPEC_DENSITY:
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItPowSpecDens, TRUE);
			MainPlot->SetupChart(CDRMPlot::POWER_SPEC_DENSITY);
			break;

		case (int) CDRMPlot::INPUTSPECTRUM_NO_AV:
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItInpSpectrNoAv, TRUE);
			MainPlot->SetupChart(CDRMPlot::INPUTSPECTRUM_NO_AV);
			break;

		case (int) CDRMPlot::AUDIO_SPECTRUM:
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItAudSpec, TRUE);
			MainPlot->SetupChart(CDRMPlot::AUDIO_SPECTRUM);
			break;

		case (int) CDRMPlot::SNR_SPECTRUM:
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItSNRSpec, TRUE);
			MainPlot->SetupChart(CDRMPlot::SNR_SPECTRUM);
			break;

		case (int) CDRMPlot::INP_SPEC_WATERF:
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItInpSpecWater, TRUE);
			MainPlot->SetupChart(CDRMPlot::INP_SPEC_WATERF);
			break;

		case (int) CDRMPlot::TRANSFERFUNCTION:
			ListViewCharSel->setOpen(pChannelLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItChanTF, TRUE);
			MainPlot->SetupChart(CDRMPlot::TRANSFERFUNCTION);
			break;

		case (int) CDRMPlot::AVERAGED_IR:
			ListViewCharSel->setOpen(pChannelLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItChanIR, TRUE);
			MainPlot->SetupChart(CDRMPlot::AVERAGED_IR);
			break;

		case (int) CDRMPlot::FAC_CONSTELLATION:
			ListViewCharSel->setOpen(pConstellationLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItConstFAC, TRUE);
			MainPlot->SetupChart(CDRMPlot::FAC_CONSTELLATION);
			break;

		case (int) CDRMPlot::SDC_CONSTELLATION:
			ListViewCharSel->setOpen(pConstellationLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItConstSDC, TRUE);
			MainPlot->SetupChart(CDRMPlot::SDC_CONSTELLATION);
			break;

		case (int) CDRMPlot::MSC_CONSTELLATION:
			ListViewCharSel->setOpen(pConstellationLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItConstMSC, TRUE);
			MainPlot->SetupChart(CDRMPlot::MSC_CONSTELLATION);
			break;

		case (int) CDRMPlot::ALL_CONSTELLATION:
			ListViewCharSel->setOpen(pConstellationLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItConstAll, TRUE);
			MainPlot->SetupChart(CDRMPlot::ALL_CONSTELLATION);
			break;

		case (int) CDRMPlot::FREQ_SAM_OFFS_HIST:
			ListViewCharSel->setOpen(pHistoryLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItHistFrSa, TRUE);
			MainPlot->SetupChart(CDRMPlot::FREQ_SAM_OFFS_HIST);
			break;

		case (int) CDRMPlot::DOPPLER_DELAY_HIST:
			ListViewCharSel->setOpen(pHistoryLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItHistDeDo, TRUE);
			MainPlot->SetupChart(CDRMPlot::DOPPLER_DELAY_HIST);
			break;

		case (int) CDRMPlot::SNR_AUDIO_HIST:
			ListViewCharSel->setOpen(pHistoryLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItHistSNRAu, TRUE);
			MainPlot->SetupChart(CDRMPlot::SNR_AUDIO_HIST);
			break;

		default: /* INPUT_SIG_PSD, includes INPUT_SIG_PSD_ANALOG and NONE_OLD */
			ListViewCharSel->setOpen(pSpectrumLiViIt, TRUE);
			ListViewCharSel->setSelected(pListItInpPSD, TRUE);
			MainPlot->SetupChart(CDRMPlot::INPUT_SIG_PSD);
			break;
		}
	}

	/* Init context menu for list view */
	pListViewContextMenu = new QPopupMenu(this, tr("ListView context menu"));
	pListViewContextMenu->insertItem(tr("&Open in separate window"), this,
		SLOT(OnListViContMenu()));


	/* Connect controls ----------------------------------------------------- */

	/* Chart selector list view */
	connect(ListViewCharSel, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListSelChanged(QListViewItem*)));
	connect(ListViewCharSel,
		SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
		this, SLOT(OnListRightButClicked(QListViewItem*, const QPoint&, int)));

	/* Buttons */
	connect(buttonOk, SIGNAL(clicked()),
		this, SLOT(accept()));

	/* Timers */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(&TimerLogFileLong, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileLong()));
	connect(&TimerLogFileShort, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileShort()));
	connect(&TimerLogFileStart, SIGNAL(timeout()),
		this, SLOT(OnTimerLogFileStart()));

	StopLogTimers();


	/* GPS */
	_REAL latitude, longitude;
	/* Latitude string for log file */
	latitude = Settings.Get("Logfile", "latitude", 1000.0);
	/* Longitude string for log file */
	longitude = Settings.Get("Logfile", "longitude", 1000.0);
	FrameGPS->hide();

	CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock(); 

	if(-90.0 <= latitude && latitude <= 90.0 && -180.0 <= longitude  && longitude <= 180.0)
	{
		Parameters.GPSData.SetPositionAvailable(TRUE);
		Parameters.GPSData.SetLatLongDegrees(latitude, longitude);
	}
	else
		Parameters.GPSData.SetPositionAvailable(FALSE);
	Parameters.Unlock(); 

}

systemevalDlg::~systemevalDlg()
{
	if(longLog.GetLoggingActivated())
		shortLog.Stop();
	if(longLog.GetLoggingActivated())
		longLog.Stop();

	double latitude, longitude;
	DRMReceiver.GetParameters()->GPSData.GetLatLongDegrees(latitude, longitude);
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);

	if (pGPSReceiver)
		delete pGPSReceiver;
}

void systemevalDlg::UpdateControls()
{
	/* Update frequency edit control (frequency could be changed by
	   schedule dialog */
	QString strFreq = EdtFrequency->text();
	const int iFrequency = DRMReceiver.GetFrequency();

	if (iFrequency != iCurFrequency)
	{
		EdtFrequency->setText(QString().setNum(iFrequency));
		iCurFrequency = iFrequency;
	}
}

void systemevalDlg::showEvent(QShowEvent*)
{
	/* Restore chart windows */
	const size_t iNumChartWin = Settings.Get("System Evaluation Dialog", "numchartwin", 0);
	for (size_t i = 0; i < iNumChartWin; i++)
	{
		stringstream s;

		/* create the section key for this window */
		s << "Chart Window " << i;

		/* get the chart type */
		const CDRMPlot::ECharType eNewType = (CDRMPlot::ECharType) Settings.Get(s.str(), "type", 0);

		/* get window geometry data */
		CWinGeom c;
		Settings.Get(s.str(), c);
		const QRect WinGeom(c.iXPos, c.iYPos, c.iWSize, c.iHSize);

		/* Open the new chart window */
		CDRMPlot* pNewChartWin = OpenChartWin(eNewType);

		/* and restore its geometry */
		if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
			pNewChartWin->setGeometry(WinGeom);

		/* Add window pointer in vector (needed for closing the windows) */
		vecpDRMPlots.push_back(pNewChartWin);
	}

	/* Update controls */
	UpdateControls();

	/* Activate real-time timer */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void systemevalDlg::hideEvent(QHideEvent*)
{
	/* Stop the real-time timer */
	Timer.stop();

	/* Store size and position of all additional chart windows */
	int iNumOpenCharts = 0;

	for (size_t i = 0; i < vecpDRMPlots.size(); i++)
	{
		/* Check, if window wasn't closed by the user */
		if (vecpDRMPlots[i]->isVisible())
		{
			stringstream s;
			CWinGeom c;
			const QRect CWGeom = vecpDRMPlots[i]->geometry();

			/* Set parameters */
			c.iXPos = CWGeom.x();
			c.iYPos = CWGeom.y();
			c.iHSize = CWGeom.height();
			c.iWSize = CWGeom.width();

			s << "Chart Window " << iNumOpenCharts;
			Settings.Put(s.str(), c);
			/* Convert plot type into an integer type. TODO: better solution */
			Settings.Put(s.str(), "type", (int) vecpDRMPlots[i]->GetChartType());

			/* Close window afterwards */
			vecpDRMPlots[i]->close();

			iNumOpenCharts++;
		}
	}
	Settings.Put("System Evaluation Dialog", "numchartwin", iNumOpenCharts);

	/* We do not need the pointers anymore, reset vector */
	vecpDRMPlots.clear();

	/* Set window geometry data in DRMReceiver module */
	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("System Evaluation Dialog", s);

	/* Store current plot type. Convert plot type into an integer type.
	 * TODO: better solution
	 */
	Settings.Put("System Evaluation Dialog", "sysevplottype", (int) MainPlot->GetChartType());
}

void systemevalDlg::UpdatePlotsStyle()
{
	int iColor = Settings.Get("System Evaluation Dialog", "colorscheme", 0xff0000);
	/* Update chart windows */
	for (size_t i = 0; i < vecpDRMPlots.size(); i++)
		vecpDRMPlots[i]->SetPlotStyle(iColor);

	/* Update main plot window */
	MainPlot->SetPlotStyle(iColor);
}

CDRMPlot* systemevalDlg::OpenChartWin(const CDRMPlot::ECharType eNewType)
{
	/* Create new chart window */
	CDRMPlot* pNewChartWin = new CDRMPlot(NULL);
	pNewChartWin->setCaption(tr("Chart Window"));

	/* Set color scheme */
	pNewChartWin->SetPlotStyle(Settings.Get("System Evaluation Dialog", "colorscheme", 0xff0000));

	/* Set correct icon (use the same as this dialog) */
	pNewChartWin->setIcon(*this->icon());

	/* Set receiver object and correct chart type */
	pNewChartWin->SetRecObj(&DRMReceiver);
	pNewChartWin->SetupChart(eNewType);

	/* Show new window */
	pNewChartWin->show();

	return pNewChartWin;
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

void systemevalDlg::SetStatus(CMultColorLED* LED, ETypeRxStatus state)
{
	switch(state)
	{
	case NOT_PRESENT:
		LED->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LED->SetLight(2); /* RED */
		break;

	case DATA_ERROR:
		LED->SetLight(1); /* YELLOW */
		break;

	case RX_OK:
		LED->SetLight(0); /* GREEN */
		break;
	}
}

void systemevalDlg::OnTimer()
{
	CParameter& ReceiverParam = *(DRMReceiver.GetParameters());

	ReceiverParam.Lock(); 

	if (this->isVisible())
	{
		SetStatus(LEDMSC, ReceiverParam.ReceiveStatus.Audio.GetStatus());
    	SetStatus(LEDSDC, ReceiverParam.ReceiveStatus.SDC.GetStatus());
    	SetStatus(LEDFAC, ReceiverParam.ReceiveStatus.FAC.GetStatus());
    	SetStatus(LEDFrameSync, ReceiverParam.ReceiveStatus.FSync.GetStatus());
    	SetStatus(LEDTimeSync, ReceiverParam.ReceiveStatus.TSync.GetStatus());
    	SetStatus(LEDIOInterface, ReceiverParam.ReceiveStatus.Interface.GetStatus());

		if (ReceiverParam.pDRMRec->SignalStrengthAvailable())
		{
			_REAL rRF = ReceiverParam.SigStrstat.getCurrent() + S9_DBUV;
			ValueRF->setText("<b>" + QString().setNum(rRF, 'f', 1) + " dBuV</b>");
		}
		else
		{
			ValueRF->setText("---");
		}

	/* Show SNR if receiver is in tracking mode */
	if (DRMReceiver.GetAcquiState() == AS_WITH_SIGNAL)
	{
		/* Get a consistant snapshot */
		
		/* We only get SNR from a local DREAM Front-End */
		_REAL rSNR = ReceiverParam.GetSNR();
		if (rSNR >= 0.0)
		{
			/* SNR */
			ValueSNR->setText("<b>" +
				QString().setNum(rSNR, 'f', 1) + " dB</b>");
		}
		else
		{
			ValueSNR->setText("<b>---</b>");
		}
		/* We get MER from a local DREAM Front-End or an RSCI input but not an MDI input */
		_REAL rMER = ReceiverParam.rMER;
		if (rMER >= 0.0 )
		{
			ValueMERWMER->setText(QString().
				setNum(ReceiverParam.rWMERMSC, 'f', 1) + " dB / "
                + QString().setNum(rMER, 'f', 1) + " dB");
		}
		else
		{
			ValueMERWMER->setText("<b>---</b>");
		}

		/* Doppler estimation (assuming Gaussian doppler spectrum) */
		if (ReceiverParam.rSigmaEstimate >= 0.0)
		{
			/* Plot delay and Doppler values */
			ValueWiener->setText(
				QString().setNum(ReceiverParam.rSigmaEstimate, 'f', 2) + " Hz / "
				+ QString().setNum(ReceiverParam.rMinDelay, 'f', 2) + " ms");
		}
		else
		{
			/* Plot only delay, Doppler not available */
			ValueWiener->setText("--- / "
            + QString().setNum(ReceiverParam.rMinDelay, 'f', 2) + " ms");
		}

		/* Sample frequency offset estimation */
		const _REAL rCurSamROffs = ReceiverParam.rResampleOffset;

		/* Display value in [Hz] and [ppm] (parts per million) */
		ValueSampFreqOffset->setText(
			QString().setNum(rCurSamROffs, 'f', 2) + " Hz (" +
			QString().setNum((int) (rCurSamROffs / SOUNDCRD_SAMPLE_RATE * 1e6))
			+ " ppm)");

	}
	else
	{
		ValueSNR->setText("<b>---</b>");
		ValueMERWMER->setText("<b>---</b>");
		ValueWiener->setText("--- / ---");
		ValueSampFreqOffset->setText("---");
	}

#ifdef _DEBUG_
	TextFreqOffset->setText("DC: " +
		QString().setNum(ReceiverParam.
		GetDCFrequency(), 'f', 3) + " Hz ");

	/* Metric values */
	ValueFreqOffset->setText(tr("Metrics [dB]: MSC: ") +
		QString().setNum(
		DRMReceiver.GetMSCMLC()->GetAccMetric(), 'f', 2) +	"\nSDC: " +
		QString().setNum(
		DRMReceiver.GetSDCMLC()->GetAccMetric(), 'f', 2) +	" / FAC: " +
		QString().setNum(
		DRMReceiver.GetFACMLC()->GetAccMetric(), 'f', 2));
#else
	/* DC frequency */
	ValueFreqOffset->setText(QString().setNum(
		ReceiverParam.GetDCFrequency(), 'f', 2) + " Hz");
#endif

/* _WIN32 fix because in Visual c++ the GUI files are always compiled even
   if USE_QT_GUI is set or not (problem with MDI in DRMReceiver) */
#ifdef USE_QT_GUI
	/* If MDI in is enabled, do not show any synchronization parameter */
	if (DRMReceiver.GetRSIIn()->GetInEnabled() == TRUE)
	{
		ValueSNR->setText("<b>---</b>");
		if (ReceiverParam.vecrRdelThresholds.GetSize() > 0)
			ValueWiener->setText(QString().setNum(ReceiverParam.rRdop, 'f', 2) + " Hz / "
					+ QString().setNum(ReceiverParam.vecrRdelIntervals[0], 'f', 2) + " ms ("
					+ QString().setNum(ReceiverParam.vecrRdelThresholds[0]) + "%)");
		else
			ValueWiener->setText(QString().setNum(ReceiverParam.rRdop, 'f', 2) + " Hz / ---");

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
	switch (ReceiverParam.eSymbolInterlMode)
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
	switch (ReceiverParam.eSDCCodingScheme)
	{
	case CS_1_SM:
		strFACInfo = "4-QAM / ";
		break;

	case CS_2_SM:
		strFACInfo = "16-QAM / ";
		break;

	default:
		break;
	}

	/* MSC */
	switch (ReceiverParam.eMSCCodingScheme)
	{
	case CS_2_SM:
		strFACInfo += "SM 16-QAM";
		break;

	case CS_3_SM:
		strFACInfo += "SM 64-QAM";
		break;

	case CS_3_HMSYM:
		strFACInfo += "HMsym 64-QAM";
		break;

	case CS_3_HMMIX:
		strFACInfo += "HMmix 64-QAM";
		break;

	default:
		break;
	}

	FACSDCMSCModeL->setText(tr("SDC / MSC Mode:")); /* Label */
	FACSDCMSCModeV->setText(strFACInfo); /* Value */


	/* Code rates #################### */
	strFACInfo = QString().setNum(ReceiverParam.MSCPrLe.iPartB);
	strFACInfo += " / ";
	strFACInfo += QString().setNum(ReceiverParam.MSCPrLe.iPartA);

	FACCodeRateL->setText(tr("Prot. Level (B / A):")); /* Label */
	FACCodeRateV->setText(strFACInfo); /* Value */


	/* Number of services #################### */
	strFACInfo = tr("Audio: ");
	strFACInfo += QString().setNum(ReceiverParam.iNumAudioService);
	strFACInfo += tr(" / Data: ");
	strFACInfo +=QString().setNum(ReceiverParam.iNumDataService);

	FACNumServicesL->setText(tr("Number of Services:")); /* Label */
	FACNumServicesV->setText(strFACInfo); /* Value */


	/* Time, date #################### */
	if ((ReceiverParam.iUTCHour == 0) &&
		(ReceiverParam.iUTCMin == 0) &&
		(ReceiverParam.iDay == 0) &&
		(ReceiverParam.iMonth == 0) &&
		(ReceiverParam.iYear == 0))
	{
		/* No time service available */
		strFACInfo = tr("Service not available");
	}
	else
	{
#ifdef GUI_QT_DATE_TIME_TYPE
		/* QT type of displaying date and time */
		QDateTime DateTime;
		DateTime.setDate(QDate(ReceiverParam.iYear,
			ReceiverParam.iMonth,
			ReceiverParam.iDay));
		DateTime.setTime(QTime(ReceiverParam.iUTCHour,
			ReceiverParam.iUTCMin));

		strFACInfo = DateTime.toString();
#else
		/* Set time and date */
		QString strMin;
		const int iMin = ReceiverParam.iUTCMin;

		/* Add leading zero to number smaller than 10 */
		if (iMin < 10)
			strMin = "0";
		else
			strMin = "";

		strMin += QString().setNum(iMin);

		strFACInfo =
			/* Time */
			QString().setNum(ReceiverParam.iUTCHour) + ":" +
			strMin + "  -  " +
			/* Date */
			QString().setNum(ReceiverParam.iMonth) + "/" +
			QString().setNum(ReceiverParam.iDay) + "/" +
			QString().setNum(ReceiverParam.iYear);
#endif
	}

	FACTimeDateL->setText(tr("Received time - date:")); /* Label */
	FACTimeDateV->setText(strFACInfo); /* Value */

	//display GPS info

	switch (ReceiverParam.GPSData.GetStatus())
	{
		case CGPSData::GPS_RX_NOT_CONNECTED:
			LEDGPS->SetLight(2); // Red
			break;

		case CGPSData::GPS_RX_NO_DATA:
			LEDGPS->SetLight(1); // Yellow
			break;

		case CGPSData::GPS_RX_DATA_AVAILABLE:
			LEDGPS->SetLight(0); // Green
			break;
	}

	if (ReceiverParam.GPSData.GetPositionAvailable())
	{
		double latitude, longitude;
		ReceiverParam.GPSData.GetLatLongDegrees(latitude, longitude);
		GPSLatV->setText(QString("%1\260").arg(latitude, 0, 'f', 6));
		GPSLngV->setText(QString("%1\260").arg(longitude, 0, 'f', 6));
	}
	else
	{
		GPSLatV->setText("?");
		GPSLngV->setText("?");
	}

	//if (ReceiverParam.GPSData.GetAltitudeAvailable())
		//qStrPosition += QString("  Alt: %1 m").arg(ReceiverParam.GPSData.GetAltitudeMetres(), 0, 'f', 0);

	if (ReceiverParam.GPSData.GetSpeedAvailable())
		GPSSpeedV->setText(QString("%1 m/s").arg(ReceiverParam.GPSData.GetSpeedMetresPerSecond(), 0, 'f', 1));
	else
		GPSSpeedV->setText("?");

	if (ReceiverParam.GPSData.GetHeadingAvailable())
		GPSHeadingV->setText(QString("%1\260").arg(ReceiverParam.GPSData.GetHeadingDegrees()));
	else
		GPSHeadingV->setText("?");

	if (ReceiverParam.GPSData.GetTimeAndDateAvailable())
		GPSTimeDateV->setText(ReceiverParam.GPSData.GetTimeDate().c_str());
	else
		GPSTimeDateV->setText("?");

/*
	if (ReceiverParam.GPSData.GetSatellitesVisibleAvailable())
		qStrTime += "Satellites: " + QString().setNum(ReceiverParam.GPSData.GetSatellitesVisible());
	else
		qStrTime += "Satellites: ?";

	TextLabelGPSTime->setText(qStrTime);
*/

	/* Update controls */
	UpdateControls();
	}
	ReceiverParam.Unlock(); 
}

void systemevalDlg::OnListSelChanged(QListViewItem* NewSelIt)
{
	/* Get char type from selected item and setup chart */
	MainPlot->SetupChart(((CCharSelItem*) NewSelIt)->GetCharType());
}

void systemevalDlg::OnListRightButClicked(QListViewItem* NewSelIt, const QPoint&, int)
{
	/* Make sure that list item is valid */
	if (NewSelIt != NULL)
	{
		/* Show menu at mouse position only if selectable item was chosen */
		if (NewSelIt->isSelectable())
			pListViewContextMenu->exec(QCursor::pos());
	}
}

void systemevalDlg::OnListViContMenu()
{
	/* Get chart type from current selected list view item */
	QListViewItem* pCurSelLVItem = ListViewCharSel->selectedItem();

	if (pCurSelLVItem != NULL)
	{
		/* Open new chart window and add window pointer in vector
		   (needed for closing the windows) */
		vecpDRMPlots.push_back(OpenChartWin(((CCharSelItem*) pCurSelLVItem)->GetCharType()));
	}
}

void systemevalDlg::StopLogTimers()
{
	TimerLogFileStart.stop();
	TimerLogFileShort.stop();
	TimerLogFileLong.stop();
}

void systemevalDlg::OnTimerLogFileStart()
{
	/* Start logging (if not already done) */
	if(!longLog.GetLoggingActivated() || !longLog.GetLoggingActivated())
	{
		/* Activate log file timer for long and short log file */
		TimerLogFileShort.start(60000); /* Every minute (i.e. 60000 ms) */
		TimerLogFileLong.start(1000); /* Every second */

		/* Get frequency from front-end edit control */
		QString strFreq = EdtFrequency->text();
		int iFrequency = strFreq.toUInt();
		longLog.SetLogFrequency(iFrequency);
		shortLog.SetLogFrequency(iFrequency);

		/* Open log file */
		shortLog.Start("DreamLog.txt");
		longLog.Start("DreamLogLong.csv");

	}
}

void systemevalDlg::OnTimerLogFileShort()
{
	/* Write new parameters in log file (short version) */
	shortLog.Update();
}

void systemevalDlg::OnTimerLogFileLong()
{
	/* Write new parameters in log file (long version) */
	longLog.Update();
}

QString	systemevalDlg::GetRobModeStr()
{		
	CParameter& Parameters = *DRMReceiver.GetParameters();
	switch (Parameters.GetWaveMode())
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

void systemevalDlg::AddWhatsThisHelp()
{
/*
	This text was taken from the only documentation of Dream software
*/
	/* DC Frequency Offset */
	const QString strDCFreqOffs =
		tr("<b>DC Frequency Offset:</b> This is the "
		"estimation of the DC frequency offset. This offset corresponds "
		"to the resulting sound card intermedia frequency of the front-end. "
		"This frequency is not restricted to a certain value. The only "
		"restriction is that the DRM spectrum must be completely inside the "
		"bandwidth of the sound card.");

	QWhatsThis::add(TextFreqOffset, strDCFreqOffs);
	QWhatsThis::add(ValueFreqOffset, strDCFreqOffs);

	/* Sample Frequency Offset */
	const QString strFreqOffset =
		tr("<b>Sample Frequency Offset:</b> This is the "
		"estimation of the sample rate offset between the sound card sample "
		"rate of the local computer and the sample rate of the D / A (digital "
		"to analog) converter in the transmitter. Usually the sample rate "
		"offset is very constant for a given sound card. Therefore it is "
		"useful to inform the Dream software about this value at application "
		"startup to increase the acquisition speed and reliability.");

	QWhatsThis::add(TextSampFreqOffset, strFreqOffset);
	QWhatsThis::add(ValueSampFreqOffset, strFreqOffset);

	/* Doppler / Delay */
	const QString strDopplerDelay =
		tr("<b>Doppler / Delay:</b> The Doppler frequency "
		"of the channel is estimated for the Wiener filter design of channel "
		"estimation in time direction. If linear interpolation is set for "
		"channel estimation in time direction, this estimation is not updated. "
		"The Doppler frequency is an indication of how fast the channel varies "
		"with time. The higher the frequency, the faster the channel changes "
		"are.<br>The total delay of the Power Delay Spectrum "
		"(PDS) is estimated from the impulse response estimation derived from "
		"the channel estimation. This delay corresponds to the range between "
		"the two vertical dashed black lines in the Impulse Response (IR) "
		"plot.");

	QWhatsThis::add(TextWiener, strDopplerDelay);
	QWhatsThis::add(ValueWiener, strDopplerDelay);

	/* I / O Interface LED */
	const QString strLEDIOInterface =
		tr("<b>I / O Interface LED:</b> This LED shows the "
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
		tr("<b>Time Sync Acq LED:</b> This LED shows the "
		"state of the timing acquisition (search for the beginning of an OFDM "
		"symbol). If the acquisition is done, this LED will stay green.");

	QWhatsThis::add(TextLabelLEDTimeSyncAcq, strLEDTimeSyncAcq);
	QWhatsThis::add(LEDTimeSync, strLEDTimeSyncAcq);

	/* Frame Sync LED */
	const QString strLEDFrameSync =
		tr("<b>Frame Sync LED:</b> The DRM frame "
		"synchronization status is shown with this LED. This LED is also only "
		"active during acquisition state of the Dream receiver. In tracking "
		"mode, this LED is always green.");

	QWhatsThis::add(TextLabelLEDFrameSync, strLEDFrameSync);
	QWhatsThis::add(LEDFrameSync, strLEDFrameSync);

	/* FAC CRC LED */
	const QString strLEDFACCRC =
		tr("<b>FAC CRC LED:</b> This LED shows the Cyclic "
		"Redundancy Check (CRC) of the Fast Access Channel (FAC) of DRM. FAC "
		"is one of the three logical channels and is always modulated with a "
		"4-QAM. If the FAC CRC check was successful, the receiver changes to "
		"tracking mode. The FAC LED is the indication whether the receiver "
		"is synchronized to a DRM transmission or not.<br>"
		"The bandwidth of the DRM signal, the constellation scheme of MSC and "
		"SDC channels and the interleaver depth are some of the parameters "
		"which are provided by the FAC.");

	QWhatsThis::add(TextLabelLEDFACCRC, strLEDFACCRC);
	QWhatsThis::add(LEDFAC, strLEDFACCRC);

	/* SDC CRC LED */
	const QString strLEDSDCCRC =
		tr("<b>SDC CRC LED:</b> This LED shows the CRC "
		"check result of the Service Description Channel (SDC) which is one "
		"logical channel of the DRM stream. This data is transmitted in "
		"approx. 1 second intervals and contains information about station "
		"label, audio and data format, etc. The error protection is normally "
		"lower than the protection of the FAC. Therefore this LED will turn "
		"to red earlier than the FAC LED in general.<br>If the CRC check "
		"is ok but errors in the content were detected, the LED turns "
		"yellow.");

	QWhatsThis::add(TextLabelLEDSDCCRC, strLEDSDCCRC);
	QWhatsThis::add(LEDSDC, strLEDSDCCRC);

	/* MSC CRC LED */
	const QString strLEDMSCCRC =
		tr("<b>MSC CRC LED:</b> This LED shows the status "
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

	/* Freq */
	QWhatsThis::add(EdtFrequency,
		tr("<b>Freq:</b> In this edit control, the current "
		"selected frequency on the front-end can be specified. This frequency "
		"will be written into the log file."));

	/* SNR */
	const QString strSNREst =
		tr("<b>SNR:</b> Signal to Noise Ratio (SNR) "
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

	/* MSC WMER / MSC MER */
	const QString strMERWMEREst =
		tr("<b>MSC WMER / MSC MER:</b> Modulation Error Ratio (MER) and "
		"weighted MER (WMER) calculated on the MSC cells is shown. The MER is "
		"calculated as follows: For each equalized MSC cell (only MSC cells, "
		"no FAC cells, no SDC cells, no pilot cells), the error vector from "
		"the nearest ideal point of the constellation diagram is measured. The "
		"squared magnitude of this error is found, and a mean of the squared "
		"errors is calculated (over one frame). The MER is the ratio in [dB] "
		"of the mean of the squared magnitudes of the ideal points of the "
		"constellation diagram to the mean squared error. This gives an "
		"estimate of the ratio of the total signal power to total noise "
		"power at the input to the equalizer for channels with flat frequency "
		"response.<br> In case of the WMER, the calculations of the means are "
		"multiplied by the squared magnitude of the estimated channel "
		"response.<br>For more information see ETSI TS 102 349.");

	QWhatsThis::add(ValueMERWMER, strMERWMEREst);
	QWhatsThis::add(TextMERWMER, strMERWMEREst);

	/* DRM Mode / Bandwidth */
	const QString strRobustnessMode =
		tr("<b>DRM Mode / Bandwidth:</b> In a DRM system, "
		"four possible robustness modes are defined to adapt the system to "
		"different channel conditions. According to the DRM standard:<ul>"
		"<li><i>Mode A:</i> Gaussian channels, with "
		"minor fading</li><li><i>Mode B:</i> Time "
		"and frequency selective channels, with longer delay spread</li>"
		"<li><i>Mode C:</i> As robustness mode B, but "
		"with higher Doppler spread</li>"
		"<li><i>Mode D:</i> As robustness mode B, but "
		"with severe delay and Doppler spread</li></ul>The "
		"bandwith is the gross bandwidth of the current DRM signal");

	QWhatsThis::add(FACDRMModeBWL, strRobustnessMode);
	QWhatsThis::add(FACDRMModeBWV, strRobustnessMode);

	/* Interleaver Depth */
	const QString strInterleaver =
		tr("<b>Interleaver Depth:</b> The symbol "
		"interleaver depth can be either short (approx. 400 ms) or long "
		"(approx. 2 s). The longer the interleaver the better the channel "
		"decoder can correct errors from slow fading signals. But the "
		"longer the interleaver length the longer the delay until (after a "
		"re-synchronization) audio can be heard.");

	QWhatsThis::add(FACInterleaverDepthL, strInterleaver);
	QWhatsThis::add(FACInterleaverDepthV, strInterleaver);

	/* SDC / MSC Mode */
	const QString strSDCMSCMode =
		tr("<b>SDC / MSC Mode:</b> Shows the modulation "
		"type of the SDC and MSC channel. For the MSC channel, some "
		"hierarchical modes are defined which can provide a very strong "
		"protected service channel.");

	QWhatsThis::add(FACSDCMSCModeL, strSDCMSCMode);
	QWhatsThis::add(FACSDCMSCModeV, strSDCMSCMode);

	/* Prot. Level (B/A) */
	const QString strProtLevel =
		tr("<b>Prot. Level (B/A):</b> The error protection "
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
		tr("<b>Number of Services:</b> This shows the "
		"number of audio and data services transmitted in the DRM stream. "
		"The maximum number of streams is four.");

	QWhatsThis::add(FACNumServicesL, strNumServices);
	QWhatsThis::add(FACNumServicesV, strNumServices);

	/* Received time - date */
	const QString strTimeDate =
		tr("<b>Received time - date:</b> This label shows "
		"the received time and date in UTC. This information is carried in "
		"the SDC channel.");

	QWhatsThis::add(FACTimeDateL, strTimeDate);
	QWhatsThis::add(FACTimeDateV, strTimeDate);

#if QT_VERSION < 0x030000
	/* if QWhatsThis is added don't work the right click popup (it used to work in QT2.3) */

	/* Chart Selector */
	QWhatsThis::add(ListViewCharSel,
		tr("<b>Chart Selector:</b> With the chart selector "
		"different types of graphical display of parameters and receiver "
		"states can be chosen. The different plot types are sorted in "
		"different groups. To open a group just double-click on the group or "
		"click on the plus left of the group name. After clicking on an item "
		"it is possible to choose other items by using the up / down arrow "
		"keys. With these keys it is also possible to open and close the "
		"groups by using the left / right arrow keys.<br>A separate chart "
		"window for a selected item can be opened by right click on the item "
		"and click on the context menu item."));
#endif

}

void systemevalDlg::EnableLog(bool b)
{
	if(b)
	{
		TimerLogFileStart.start(1, TRUE);
		/* set the focus */
		if (EdtFrequency->isEnabled() && !EdtFrequency->hasFocus())
			EdtFrequency->setFocus();
	}
	else
	{
		/* Deactivate log file timer */
		StopLogTimers();
		shortLog.Stop();
		longLog.Stop();
	}
}

void systemevalDlg::LogStartDel(long iValue)
{
	iLogDelay = iValue;
}

void systemevalDlg::LogPosition(bool b)
{
	shortLog.SetPositionEnabled(b);
	longLog.SetPositionEnabled(b);
}

void systemevalDlg::LogSigStr(bool b)
{
	shortLog.SetRxlEnabled(b);
	longLog.SetRxlEnabled(b);
}


void systemevalDlg::EnableGPS(bool b)
{
	// let gps data come from RSCI
	if(DRMReceiver.GetRSIIn()->GetInEnabled())
	{
		FrameGPS->show();
		return;
	}

	CParameter& Parameters = *DRMReceiver.GetParameters();
	if(b)
	{
		Parameters.Lock(); 
		Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_GPS_RECEIVER);
		Parameters.Unlock(); 
		FrameGPS->show();
		if(pGPSReceiver == NULL)
			pGPSReceiver = new CGPSReceiver(Parameters, Settings);
	}
	else
	{
		Parameters.Lock(); 
		Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
		Parameters.Unlock(); 
		FrameGPS->hide();
		if(pGPSReceiver)
		{
			delete pGPSReceiver;
			pGPSReceiver = NULL;
		}
	}
}

