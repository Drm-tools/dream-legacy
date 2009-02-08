/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Custom settings of the qwt-plot
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

#include "../util/Settings.h"
#include "DRMPlot.h"
#include <limits>
#include <qpixmap.h>
#include <qwt_scale_engine.h>
#include <qwt_legend.h>
#include <qwt_color_map.h>
#include <qwt_plot_layout.h>

/* Implementation *************************************************************/
CDRMPlot::CDRMPlot(QWidget *p, const char *name) :
	QwtPlot (p, name), CurCharType(CPlotManager::NONE_OLD), InitCharType(CPlotManager::NONE_OLD),
	bOnTimerCharMutexFlag(false), pPlotManager(NULL)
{
	/* Grid defaults */
	grid = new QwtPlotGrid();
	grid->enableXMin(false);
	grid->enableYMin(false);
	grid->attach(this);

	/* Fonts */
	setAxisFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yRight, QFont("SansSerif", 8));
	QwtText title;
	title.setFont(QFont("SansSerif", 8, QFont::Bold));
	setTitle(title);

    /* axis titles */
    bottomTitle.setFont(QFont("SansSerif", 8));
    setAxisTitle(QwtPlot::xBottom, bottomTitle);

    leftTitle.setFont(QFont("SansSerif", 8));
    setAxisTitle(QwtPlot::yLeft, leftTitle);

    rightTitle.setFont(QFont("SansSerif", 8));
    setAxisTitle(QwtPlot::yRight, rightTitle);

	/* Global frame */
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setLineWidth(2);
	setMargin(10);

	/* Canvas */
	setCanvasLineWidth(0);

	/* Set default style */
	SetPlotStyle(0);

	/* Set marker symbols */
	/* MSC */
	MarkerSymMSC.setStyle(QwtSymbol::Rect);
	MarkerSymMSC.setSize(2);
	MarkerSymMSC.setPen(QPen(MainPenColorConst));
	MarkerSymMSC.setBrush(QBrush(MainPenColorConst));

	/* SDC */
	MarkerSymSDC.setStyle(QwtSymbol::XCross);
	MarkerSymSDC.setSize(4);
	MarkerSymSDC.setPen(QPen(SpecLine1ColorPlot));
	MarkerSymSDC.setBrush(QBrush(SpecLine1ColorPlot));

	/* FAC */
	MarkerSymFAC.setStyle(QwtSymbol::Ellipse);
	MarkerSymFAC.setSize(4);
	MarkerSymFAC.setPen(QPen(SpecLine2ColorPlot));
	MarkerSymFAC.setBrush(QBrush(SpecLine2ColorPlot));

    /* Legend */
    insertLegend(new QwtLegend(), QwtPlot::RightLegend);

	/* Connections */
	// TODO use a QwtPlotPicker http://qwt.sourceforge.net/class_qwt_plot_picker.html
	//connect(this, SIGNAL(plotMouseReleased(const QMouseEvent&)), this, SLOT(OnClicked(const QMouseEvent&)));
	connect(&TimerChart, SIGNAL(timeout()), this, SLOT(OnTimerChart()));

	TimerChart.stop();
}

void CDRMPlot::OnTimerChart()
{
	/* In some cases, if the user moves the mouse very fast over the chart
	   selection list view, this function is called by two different threads.
	   Somehow, using QMutex does not help. Therefore we introduce a flag for
	   doing this job. This solution is a work-around. TODO: better solution */
	if (bOnTimerCharMutexFlag == true)
		return;

	bOnTimerCharMutexFlag = true;

	if (InitCharType != CurCharType)
	{
		pPlotManager->endPlot(InitCharType);
		pPlotManager->startPlot(CurCharType);
	}

	switch (CurCharType)
	{
	case CPlotManager::AVERAGED_IR:
		SetAvIR();
		break;

	case CPlotManager::TRANSFERFUNCTION:
		SetTranFct();
		break;

	case CPlotManager::POWER_SPEC_DENSITY:
		SetPSD();
		break;

	case CPlotManager::SNR_SPECTRUM:
		SetSNRSpectrum();
		break;

	case CPlotManager::INPUTSPECTRUM_NO_AV:
		SetInpSpec();
		break;

	case CPlotManager::INP_SPEC_WATERF:
		SetInpSpecWaterf();
		break;

	case CPlotManager::INPUT_SIG_PSD:
		SetInpPSD();
		break;

	case CPlotManager::INPUT_SIG_PSD_ANALOG:
        SetInpPSDAnalog();
		break;

	case CPlotManager::AUDIO_SPECTRUM:
		SetAudioSpec();
		break;

	case CPlotManager::FREQ_SAM_OFFS_HIST:
		SetFreqSamOffsHist();
		break;

	case CPlotManager::DOPPLER_DELAY_HIST:
		SetDopplerDelayHist();
		break;

	case CPlotManager::SNR_AUDIO_HIST:
		SetSNRAudHist();
		break;

	case CPlotManager::FAC_CONSTELLATION:
		SetFACConst();
		break;

	case CPlotManager::SDC_CONSTELLATION:
		SetSDCConst();
		break;

	case CPlotManager::MSC_CONSTELLATION:
		SetMSCConst();
		break;

	case CPlotManager::ALL_CONSTELLATION:
		SetAllConst();
		break;

	case CPlotManager::NONE_OLD:
		break;
	}

	/* "Unlock" mutex flag */
	bOnTimerCharMutexFlag = false;
}

void CDRMPlot::SetupChart(const CPlotManager::EPlotType eNewType)
{
	if (eNewType != CPlotManager::NONE_OLD)
	{
		/* Set internal variable */
		CurCharType = eNewType;

		/* Update help text connected with the plot widget */
		AddWhatsThisHelpChar(eNewType);

		/* Update chart */
		OnTimerChart();

		/* Set up timer */
		switch (eNewType)
		{
		case CPlotManager::INP_SPEC_WATERF:
			/* Very fast update */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_WATERFALL);
			break;

		case CPlotManager::AVERAGED_IR:
		case CPlotManager::TRANSFERFUNCTION:
		case CPlotManager::POWER_SPEC_DENSITY:
		case CPlotManager::INPUT_SIG_PSD:
		case CPlotManager::INPUT_SIG_PSD_ANALOG:
		case CPlotManager::SNR_SPECTRUM:
			/* Fast update */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME_FAST);
			break;

		case CPlotManager::FAC_CONSTELLATION:
		case CPlotManager::SDC_CONSTELLATION:
		case CPlotManager::MSC_CONSTELLATION:
		case CPlotManager::ALL_CONSTELLATION:
		case CPlotManager::INPUTSPECTRUM_NO_AV:
		case CPlotManager::AUDIO_SPECTRUM:
		case CPlotManager::FREQ_SAM_OFFS_HIST:
		case CPlotManager::DOPPLER_DELAY_HIST:
		case CPlotManager::SNR_AUDIO_HIST:
			/* Slow update of plot */
			TimerChart.changeInterval(GUI_CONTROL_UPDATE_TIME);
			break;

		case CPlotManager::NONE_OLD:
			break;
		}
	}
}

void CDRMPlot::showEvent(QShowEvent*)
{
	/* Activate real-time timers when window is shown */
	SetupChart(CurCharType);

	/* Update window */
	OnTimerChart();
}

void CDRMPlot::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timers when window is hide to save CPU power */
	TimerChart.stop();
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
	QColor BckgrdColorPlot;

	switch (iNewStyleID)
	{
	case 1:
		MainPenColorPlot = GREENBLACK_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = GREENBLACK_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = GREENBLACK_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = GREENBLACK_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = GREENBLACK_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = GREENBLACK_PASS_BAND_COLOR_PLOT;
		break;

	case 2:
		MainPenColorPlot = BLACKGREY_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLACKGREY_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLACKGREY_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLACKGREY_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLACKGREY_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = BLACKGREY_PASS_BAND_COLOR_PLOT;
		break;

	case 0: /* 0 is default */
	default:

		MainPenColorPlot = BLUEWHITE_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLUEWHITE_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLUEWHITE_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLUEWHITE_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLUEWHITE_SPEC_LINE2_COLOR_PLOT;
		PassBandColorPlot = BLUEWHITE_PASS_BAND_COLOR_PLOT;
		break;
	}

	/* Apply colors */
	grid->setMajPen(QPen(MainGridColorPlot, 0, DotLine));
	grid->setMinPen(QPen(MainGridColorPlot, 0, DotLine));
	setCanvasBackground(QColor(BckgrdColorPlot));

	/* Make sure that plot are being initialized again */
	InitCharType = CPlotManager::NONE_OLD;
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	double* pdData = new double[vecrData.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Copy data from vectors in temporary arrays */
	const int iScaleSize = vecrScale.Size();
	for (int i = 0; i < iScaleSize; i++)
	{
		pdData[i] = vecrData[i];
		pdScale[i] = vecrScale[i];
	}

	main1curve->setData(pdScale, pdData, vecrData.Size());

	delete[] pdData;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
					   CVector<_REAL>& vecrScale)
{
	double* pdData1 = new double[vecrData1.Size()];
	double* pdData2 = new double[vecrData2.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Copy data from vectors in temporary arrays */
	const int iScaleSize = vecrScale.Size();
	for (int i = 0; i < iScaleSize; i++)
	{
		pdData1[i] = vecrData1[i];
		pdData2[i] = vecrData2[i];
		pdScale[i] = vecrScale[i];
	}

	main1curve->setData(pdScale, pdData1, vecrData1.Size());
	main2curve->setData(pdScale, pdData2, vecrData2.Size());

	delete[] pdData1;
	delete[] pdData2;
	delete[] pdScale;
}

void CDRMPlot::SetData(QwtPlotCurve* curve, CVector<_COMPLEX>& veccData)
{
	const int iPoints = veccData.Size();
	/* Copy data from vector into a temporary array */
	double* pdX = new double[iPoints];
	double* pdY = new double[iPoints];
	for (int i = 0; i < iPoints; i++)
	{
		pdX[i] = veccData[i].real();
		pdY[i] = veccData[i].imag();
	}
	curve->setData(pdX, pdY, iPoints);
	delete[] pdX;
	delete[] pdY;
}

void CDRMPlot::SetAvIR()
{
    CVector<_REAL> vecrData, vecrScale;
    _REAL rLowerB, rHigherB, rStartGuard, rEndGuard, rBeginIR, rEndIR;

    /* Get data from module */
    pPlotManager->GetAvPoDeSp(vecrData, vecrScale,
        rLowerB, rHigherB, rStartGuard, rEndGuard, rBeginIR, rEndIR);

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::AVERAGED_IR || vecrScale.Size()==0)
	{
		InitCharType = CPlotManager::AVERAGED_IR;
        setTitle(tr("Channel Impulse Response"));
        enableAxis(QwtPlot::yRight, false);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Insert curves */
        clear();
        curve1 = new QwtPlotCurve(tr("Guard-interval beginning"));
        curve2 = new QwtPlotCurve(tr("Guard-interval end"));
        curve3 = new QwtPlotCurve(tr("Estimated begin of impulse response"));
        curve4 = new QwtPlotCurve(tr("Estimated end of impulse response"));
        curve1->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));
        curve2->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));
        curve3->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));
        curve4->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));

        curve1->setItemAttribute(QwtPlotItem::Legend, false);
        curve2->setItemAttribute(QwtPlotItem::Legend, false);
        curve3->setItemAttribute(QwtPlotItem::Legend, false);
        curve4->setItemAttribute(QwtPlotItem::Legend, false);

        curve1->attach(this);
        curve2->attach(this);
        curve3->attach(this);
        curve4->attach(this);

        curve5 = new QwtPlotCurve(tr("Higher Bound"));
        curve5->setItemAttribute(QwtPlotItem::Legend, false);
#ifdef _DEBUG_
        curve6 = new QwtPlotCurve(tr("Lower bound"));
        curve5->setPen(QPen(SpecLine1ColorPlot));
        curve6->setPen(QPen(SpecLine2ColorPlot));
        curve6->setItemAttribute(QwtPlotItem::Legend, false);
        curve5->attach(this);
        curve6->attach(this);
#else
        curve5->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));
        curve5->attach(this);
#endif

        /* Add main curve */
        main1curve = new QwtPlotCurve(tr("Channel Impulse Response"));

        /* Curve color */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main1curve->setItemAttribute(QwtPlotItem::Legend, false);
        main1curve->attach(this);
	}

	if (vecrScale.size() != 0)
	{
		/* Fixed scale */
		const double cdAxMinLeft = (double) -20.0;
		const double cdAxMaxLeft = (double) 40.0;
		setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);

		/* Vertical bounds -------------------------------------------------- */
		double dX[2], dY[2];

		/* These bounds show the beginning and end of the guard-interval */
		dY[0] = cdAxMinLeft;
		dY[1] = cdAxMaxLeft;

		/* Left bound */
		dX[0] = dX[1] = rStartGuard;
		curve1->setData(dX, dY, 2);

		/* Right bound */
		dX[0] = dX[1] = rEndGuard;
		curve2->setData(dX, dY, 2);

		/* Estimated begin of impulse response */
		dX[0] = dX[1] = rBeginIR;
		curve3->setData(dX, dY, 2);

		/* Estimated end of impulse response */
		dX[0] = dX[1] = rEndIR;
		curve4->setData(dX, dY, 2);

		/* Data for the actual impulse response curve */
		main1curve->setData(&vecrData[0], &vecrScale[0], vecrData.size());

		/* Horizontal bounds ------------------------------------------------ */
		/* These bounds show the peak detection bound from timing tracking */
		dX[0] = vecrScale[0];
		dX[1] = vecrScale[vecrScale.size() - 1];

#ifdef _DEBUG_
		/* Lower bound */
		dY[0] = dY[1] = rLowerB;
		setCurveData(curve6, dX, dY, 2);

		/* Higher bound */
		dY[0] = dY[1] = rHigherB;
#else
		/* Only include highest bound */
		dY[0] = dY[1] = Max(rHigherB, rLowerB);
#endif
		curve5->setData(dX, dY, 2);

		/* Adjust scale for x-axis */
		setAxisScale(QwtPlot::xBottom, (double) vecrScale[0],
			(double) vecrScale[vecrScale.size() - 1]);

		replot();
	}
}

void CDRMPlot::SetTranFct()
{

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::TRANSFERFUNCTION)
	{
		InitCharType = CPlotManager::TRANSFERFUNCTION;
        setTitle(tr("Channel Transfer Function / Group Delay"));
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));
        setAxisScale(QwtPlot::yLeft,  -85.0,  -35.0);

        enableAxis(QwtPlot::yRight);
        setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
        setAxisScale(QwtPlot::yRight, -50.0, 50.0);

        /* Add curves */
        clear();

        /* TODO - check that its group delay that should be scaled to right axis!! */
        main1curve = new QwtPlotCurve(tr("Transf. Fct."));
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));

        main2curve = new QwtPlotCurve(tr("Group Del."));
        main2curve->setPen(QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap, RoundJoin));
        main2curve->setYAxis(QwtPlot::yRight);

        main1curve->attach(this);
        main2curve->attach(this);
	}

    vector<double> transferFunc, groupDelay, scale;
    pPlotManager->GetTransferFunction(transferFunc, groupDelay, scale);

	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) scale.size());

	main1curve->setData(&scale[0], &transferFunc[0], scale.size());
	main2curve->setData(&scale[0], &groupDelay[0], scale.size());

	replot();
}

void CDRMPlot::SetAudioSpec()
{
    CVector<_REAL> vecrData, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::AUDIO_SPECTRUM)
	{
		InitCharType = CPlotManager::AUDIO_SPECTRUM;
        setTitle(tr("Audio Spectrum"));
        enableAxis(QwtPlot::yRight, false);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, "AS [dB]");
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Fixed scale */
        setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
        double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2400; /* 20.0 for 48 kHz */
        if (dBandwidth < 20.0)
            dBandwidth = (double) 20.0;

        setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

        /* Add main curve */
        clear();
        main1curve = new QwtPlotCurve(tr("Audio Spectrum"));
        main1curve->setItemAttribute(QwtPlotItem::Legend, false);
        /* Curve color */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main1curve->attach(this);
	}
		/* Get data from module */
    pPlotManager->GetAudioSpec(vecrData, vecrScale);
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetFreqSamOffsHist()
{
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    _REAL rFreqOffAcquVal;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::FREQ_SAM_OFFS_HIST)
	{
		InitCharType = CPlotManager::FREQ_SAM_OFFS_HIST;
        setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
        enableAxis(QwtPlot::yRight);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Add main curves */
        clear();
        main1curve = new QwtPlotCurve(tr("Freq."));
        main2curve = new QwtPlotCurve(tr("Samp."));
        main2curve->setYAxis(QwtPlot::yRight);

        /* Curve colors */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main2curve->setPen(QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap, RoundJoin));

        main1curve->attach(this);
        main2curve->attach(this);
	}

	QString strYLeftLabel = tr("Freq. Offset [Hz] rel. to ") +
		QString().setNum(rFreqOffAcquVal) + " Hz";
	enableAxis(QwtPlot::yLeft, true);
	setAxisTitle(QwtPlot::yLeft, strYLeftLabel);

	pPlotManager->GetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqOffAcquVal);

	/* Customized auto-scaling. We adjust the y scale so that it is not larger
	   than rMinScaleRange"  */
	const _REAL rMinScaleRange = (_REAL) 1.0; /* Hz */

	/* Get maximum and minimum values */
	_REAL MaxFreq = - numeric_limits<_REAL>::max();
	_REAL MinFreq = numeric_limits<_REAL>::max();
	_REAL MaxSam = - numeric_limits<_REAL>::max();
	_REAL MinSam = numeric_limits<_REAL>::max();

	const int iSize = vecrScale.Size();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > MaxFreq)
			MaxFreq = vecrData[i];

		if (vecrData[i] < MinFreq)
			MinFreq = vecrData[i];

		if (vecrData2[i] > MaxSam)
			MaxSam = vecrData2[i];

		if (vecrData2[i] < MinSam)
			MinSam = vecrData2[i];
	}

	/* Apply scale to plot */
	setAxisScale(QwtPlot::yLeft, (double) Floor(MinFreq / rMinScaleRange),
		(double) Ceil(MaxFreq / rMinScaleRange));
	setAxisScale(QwtPlot::yRight, (double) Floor(MinSam / rMinScaleRange),
		(double) Ceil(MaxSam / rMinScaleRange));
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetDopplerDelayHist()
{
    CVector<_REAL> vecrData, vecrData2, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::DOPPLER_DELAY_HIST)
	{
		InitCharType = CPlotManager::DOPPLER_DELAY_HIST;
        setTitle(tr("Delay / Doppler History"));
        enableAxis(QwtPlot::yRight);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
        setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Fixed scale */
        setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
        setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

        /* Add main curves */
        clear();
        main1curve = new QwtPlotCurve(tr("Delay"));
        main2curve = new QwtPlotCurve(tr("Doppler"));
        main2curve->setYAxis(QwtPlot::yRight);

        /* Curve colors */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main2curve->setPen(QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap, RoundJoin));

        main1curve->attach(this);
        main2curve->attach(this);
	}
	pPlotManager->GetDopplerDelHist(vecrData, vecrData2, vecrScale);
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);
	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetSNRAudHist()
{
    CVector<_REAL> vecrData, vecrData2, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::SNR_AUDIO_HIST)
	{
		InitCharType = CPlotManager::SNR_AUDIO_HIST;
        setTitle(tr("SNR / Correctly Decoded Audio History"));
        enableAxis(QwtPlot::yRight);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
        setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Add main curves */
        clear();
        main1curve = new QwtPlotCurve(tr("SNR"));
        main2curve = new QwtPlotCurve(tr("Audio"));
        main2curve->setYAxis(QwtPlot::yRight);

        /* Curve colors */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main2curve->setPen(QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap, RoundJoin));

        main1curve->attach(this);
        main2curve->attach(this);
	}

    pPlotManager->GetSNRHist(vecrData, vecrData2, vecrScale);

	/* Customized auto-scaling. We adjust the y scale maximum so that it
	   is not more than "rMaxDisToMax" to the curve */
	const int iMaxDisToMax = 5; /* dB */
	const int iMinValueSNRYScale = 15; /* dB */

	/* Get maximum value */
	_REAL MaxSNR = numeric_limits<_REAL>::min();

	const int iSize = vecrScale.Size();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > MaxSNR)
			MaxSNR = vecrData[i];
	}

	/* Quantize scale to a multiple of "iMaxDisToMax" */
	double dMaxYScaleSNR =
		(double) (Ceil(MaxSNR / iMaxDisToMax) * iMaxDisToMax);

	/* Bound at the minimum allowed value */
	if (dMaxYScaleSNR < (double) iMinValueSNRYScale)
		dMaxYScaleSNR = (double) iMinValueSNRYScale;

	/* Ratio between the maximum values for audio and SNR. The ratio should be
	   chosen so that the audio curve is not in the same range as the SNR curve
	   under "normal" conditions to increase readability of curves.
	   Since at very low SNRs, no audio can received anyway so we do not have to
	   check whether the audio y-scale is in range of the curve */
	const _REAL rRatioAudSNR = (double) 1.5;
	const double dMaxYScaleAudio = dMaxYScaleSNR * (double) rRatioAudSNR;

	/* Apply scale to plot */
	setAxisScale(QwtPlot::yLeft, (double) 0.0, dMaxYScaleSNR);
	setAxisScale(QwtPlot::yRight, (double) 0.0, dMaxYScaleAudio);
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SpectrumPlotDefaults(
    const QString& title, const QString& axistitle, uint penwidth)
{
	/* Init chart for power spectral density estimation */
	setTitle(title);
	enableAxis(QwtPlot::yRight, false);
	grid->enableX(true);
	grid->enableY(true);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	enableAxis(QwtPlot::yLeft, true);
	setAxisTitle(QwtPlot::yLeft, axistitle+" [dB]");
    //setAxisLabelRotation(QwtPlot::yLeft, 90.0);
	canvas()->setBackgroundMode(QWidget::PaletteBackground);

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for DC carrier */
	DCCarrierCurve = new QwtPlotCurve(tr("DC carrier"));
	DCCarrierCurve->setPen(QPen(SpecLine1ColorPlot, 1, DotLine));
	DCCarrierCurve->setItemAttribute(QwtPlotItem::Legend, false);
	DCCarrierCurve->attach(this);

	/* Add main curve */
	main1curve = new QwtPlotCurve(axistitle);
	main1curve->setPen(QPen(MainPenColorPlot, penwidth, SolidLine, RoundCap, RoundJoin));
	main1curve->setItemAttribute(QwtPlotItem::Legend, false);
	main1curve->attach(this);
}

void CDRMPlot::SetDCCarrier(double dVal)
{
	double dX[2], dY[2];
	dX[0] = dX[1] = dVal / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	DCCarrierCurve->setData(dX, dY, 2);
}

void CDRMPlot::SetPSD()
{
    CVector<_REAL> vecrData, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::POWER_SPEC_DENSITY)
	{
		InitCharType = CPlotManager::POWER_SPEC_DENSITY;
        /* Init chart for power spectram density estimation */
        clear();

        SpectrumPlotDefaults(
            tr("Shifted Power Spectral Density of Input Signal"),
            tr("Shifted PSD"), 1);

        /* fixed values for DC Carrier line */
        SetDCCarrier(VIRTUAL_INTERMED_FREQ);
	}

    pPlotManager->GetPowDenSpec(vecrData, vecrScale);
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetSNRSpectrum()
{
    CVector<_REAL> vecrData, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::SNR_SPECTRUM)
	{
		InitCharType = CPlotManager::SNR_SPECTRUM;
        setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
        enableAxis(QwtPlot::yRight, false);
        grid->enableX(true);
        grid->enableY(true);
        setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
        enableAxis(QwtPlot::yLeft, true);
        setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));
        canvas()->setBackgroundMode(QWidget::PaletteBackground);

        /* Add main curve */
        clear();
        main1curve = new QwtPlotCurve(tr("SNR Spectrum"));

        /* Curve color */
        main1curve->setPen(QPen(MainPenColorPlot, 2, SolidLine, RoundCap, RoundJoin));
        main1curve->setItemAttribute(QwtPlotItem::Legend, false);

        main1curve->attach(this);
	}

    /* Get data from module */
    pPlotManager->GetSNRProfile(vecrData, vecrScale);

	const int iSize = vecrScale.Size();

	/* Fixed scale for x-axis */
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) iSize);

	/* Fixed / variable scale (if SNR is in range, use fixed scale otherwise
	   enlarge scale) */
	/* Get maximum value */
	_REAL rMaxSNR = numeric_limits<_REAL>::min();
	for (int i = 0; i < iSize; i++)
	{
		if (vecrData[i] > rMaxSNR)
			rMaxSNR = vecrData[i];
	}

	double dMaxScaleYAxis = MAX_VAL_SNR_SPEC_Y_AXIS_DB;

	if (rMaxSNR > dMaxScaleYAxis)
	{
		const double rEnlareStep = (double) 10.0; /* dB */
		dMaxScaleYAxis = ceil(rMaxSNR / rEnlareStep) * rEnlareStep;
	}

	/* Set scale */
	setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

#include <iostream>

void CDRMPlot::SetInpSpec()
{
    vector<_REAL> vecrData, vecrScale;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::INPUTSPECTRUM_NO_AV)
	{
		InitCharType = CPlotManager::INPUTSPECTRUM_NO_AV;
        clear();
        SpectrumPlotDefaults(tr("Input Spectrum"), tr("Input Spectrum"), 1);
	}

    SetDCCarrier(pPlotManager->GetDCFrequency());
    pPlotManager->GetInputSpec(vecrData, vecrScale);
    //cerr << vecrData.size() << endl;
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
	replot();
}

void CDRMPlot::SetInpPSD()
{
    vector<_REAL> vecrData, vecrScale;

	/* First check if plot must be set up. */
	if (InitCharType != CPlotManager::INPUT_SIG_PSD)
	{
		InitCharType = CPlotManager::INPUT_SIG_PSD;
        clear();
        SpectrumPlotDefaults(tr("Input PSD"), tr("Input PSD"), 2);
	}
    SetDCCarrier(pPlotManager->GetDCFrequency());
    pPlotManager->GetInputPSD(vecrData, vecrScale);
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
	replot();
}

void CDRMPlot::SetInpPSDAnalog()
{
	/* First check if plot must be set up. The char type "INPUT_SIG_PSD_ANALOG"
	   has the same initialization as "INPUT_SIG_PSD" */
	if (InitCharType != CPlotManager::INPUT_SIG_PSD_ANALOG)
	{
		InitCharType = CPlotManager::INPUT_SIG_PSD_ANALOG;
        /* Init chart for power spectram density estimation */
        clear();

        /* Insert line for bandwidth marker */
        BandwidthMarkerCurve = new QwtPlotCurve(tr("Filter bandwidth"));

        /* Make sure that line is bigger than the current plots height. Do this by
           setting the width to a very large value. TODO: better solution */
        BandwidthMarkerCurve->setPen(QPen(PassBandColorPlot, 10000));
        BandwidthMarkerCurve->setItemAttribute(QwtPlotItem::Legend, false);
        BandwidthMarkerCurve->attach(this);

        SpectrumPlotDefaults(tr("Input PSD"), tr("Input PSD"), 2);
	}

	/* Insert marker for filter bandwidth if required */
    _REAL rBWCenter, rBWWidth;
    pPlotManager->GetAnalogBWParameters(rBWCenter, rBWWidth);
	if (rBWWidth != (_REAL) 0.0)
	{
	    double dX[2], dY[2];
		dX[0] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
		dX[1] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

		/* Take the min-max values from scale to get vertical line */
		dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
		dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;

		BandwidthMarkerCurve->setData(dX, dY, 2);
	}
	else
		BandwidthMarkerCurve->setData(NULL, NULL, 0);

	/* Insert actual spectrum data */
    SetDCCarrier(pPlotManager->GetAnalogCurMixFreqOffs());
    vector<_REAL> vecrData, vecrScale;
    pPlotManager->GetInputPSD(vecrData, vecrScale);
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
	replot();
}


QwtRasterData *SpectrogramData::copy() const
{
    SpectrogramData* c = new SpectrogramData();
    c->height = height;
    c->data = data;
    c->scale = scale;
    return c;
}

double SpectrogramData::value(double x, double y) const
{
    size_t row, col;
    // need to interpolate ?
    // first map the y value to index the row
    row = height - size_t(y);
    // then see if we have data for it
    if(row>=data.size())
        return MIN_VAL_INP_SPEC_Y_AXIS_DB; // lowest possible dB value
    // then find the x value in scale
    const vector<double>& rowdata = data.at(row);
    //col = size_t(x/boundingRect().width()*double(rowdata.size()));
    col = size_t(x*170.0);

    if(col>=rowdata.size())
        return rowdata.at(rowdata.size()-1);
    // return the value
    return rowdata.at(col);
}

void SpectrogramData::setHeight(size_t h)
{
    height = h;
    // left top width height
    setBoundingRect(QwtDoubleRect(0.0, 0.0, SOUNDCRD_SAMPLE_RATE / 2000, h));
}

void SpectrogramData::setData(vector<double>& row)
{
    data.push_front(row);
    if(data.size()>height)
        data.pop_back();
}

void CDRMPlot::SetInpSpecWaterf()
{
    vector<_REAL> vecrData, vecrScale;

    pPlotManager->GetInputSpec(vecrData, vecrScale);

	const QSize CanvSize = canvas()->size();

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::INP_SPEC_WATERF
        || CanvSize != LastCanvasSize
	)
	{
		InitCharType = CPlotManager::INP_SPEC_WATERF;
		LastCanvasSize = CanvSize;
		clear();
        setTitle(tr("Waterfall Input Spectrum"));
        enableAxis(QwtPlot::yRight, false);
        enableAxis(QwtPlot::yLeft, false);
        enableAxis(QwtPlot::xBottom, true);
        setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
        // spectrogram uses scales to determine value range
        // each pixel represents one sample (400 ms ?)
        setAxisScale(QwtPlot::yLeft, 0.0, CanvSize.height());
        setAxisScale(QwtPlot::xBottom, 0.0, SOUNDCRD_SAMPLE_RATE / 2000);
        grid->enableX(false);
        grid->enableY(false);

        spectrogram = new QwtPlotSpectrogram();
        spectrogram->attach(this);
        plotLayout()->setAlignCanvasToScales(true);
        spectrogramData.setHeight(CanvSize.height());
	}

    spectrogramData.setData(vecrData);
    spectrogram->setData(spectrogramData);
    replot();
}

void CDRMPlot::ConstellationPlotDefaults(const QString& title, double limit, int n)
{
	setTitle(title+" "+tr("Constellation"));
	enableAxis(QwtPlot::yRight, false);
	grid->enableX(true);
	grid->enableY(true);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	enableAxis(QwtPlot::yLeft, true);
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	canvas()->setBackgroundMode(QWidget::PaletteBackground);
	double step = limit/n;
	setAxisScale(QwtPlot::xBottom, -limit, limit, step);
	setAxisScale(QwtPlot::yLeft, -limit, limit, step);
}

QwtPlotCurve* CDRMPlot::ScatterCurve(const QString& title, const QwtSymbol& s)
{
    QwtPlotCurve* curve = new QwtPlotCurve(title);
    curve->setSymbol(s);
    curve->setPen(QPen(Qt::NoPen));
    curve->setItemAttribute(QwtPlotItem::Legend, false);
    curve->attach(this);
    return curve;
}

void CDRMPlot::SetFACConst()
{
    CVector<_COMPLEX> veccData;

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::FAC_CONSTELLATION)
	{
		InitCharType = CPlotManager::FAC_CONSTELLATION;
        ConstellationPlotDefaults(tr("FAC"), 2.0 / sqrt(2.0), 1);
        clear();
        main1curve = ScatterCurve("FAC", MarkerSymFAC);
	}
	pPlotManager->GetFACVectorSpace(veccData);
	SetData(main1curve, veccData);
	replot();
}

void CDRMPlot::SetSDCConst()
{
    CVector<_COMPLEX> veccData;
    ECodScheme eNewCoSc;

	pPlotManager->GetSDCVectorSpace(veccData, eNewCoSc);

	/* only set up plot if chart type or constellation scheme has changed */
	if (InitCharType != CPlotManager::SDC_CONSTELLATION
        || eNewCoSc != eCurSDCCodingScheme
	)
    {
        InitCharType = CPlotManager::SDC_CONSTELLATION;
        eCurSDCCodingScheme = eNewCoSc;
        ConstellationPlotDefaults("SDC", 4.0 / sqrt(10.0),
                (eNewCoSc == CS_1_SM)?1:2);
        clear();
        main1curve = ScatterCurve("SDC", MarkerSymSDC);
	}
	SetData(main1curve, veccData);
	replot();
}

void CDRMPlot::SetMSCConst()
{
    CVector<_COMPLEX> veccData;
    ECodScheme eNewCoSc;

	pPlotManager->GetMSCVectorSpace(veccData, eNewCoSc);

	/* only set up plot if chart type or constellation scheme has changed */
	if (InitCharType != CPlotManager::MSC_CONSTELLATION
        || eNewCoSc != eCurMSCCodingScheme
	)
    {
        InitCharType = CPlotManager::MSC_CONSTELLATION;
        eCurMSCCodingScheme = eNewCoSc;
        /* Fixed scale (8 / sqrt(42)) */
        ConstellationPlotDefaults("MSC", 8.0 / sqrt(42.0),
            (eNewCoSc == CS_2_SM)?2:4);
        clear();
        main1curve = ScatterCurve("MSC", MarkerSymMSC);
	}
	SetData(main1curve, veccData);
	replot();
}

void CDRMPlot::SetAllConst()
{
    CVector<_COMPLEX> veccMSC, veccSDC, veccFAC;
    ECodScheme eNewSDCCoSc, eNewMSCCoSc;

    pPlotManager->GetFACVectorSpace(veccFAC);
    pPlotManager->GetSDCVectorSpace(veccSDC, eNewSDCCoSc);
    pPlotManager->GetMSCVectorSpace(veccMSC, eNewMSCCoSc);

	/* First check if plot must be set up */
	if (InitCharType != CPlotManager::ALL_CONSTELLATION
	        || eNewSDCCoSc != eCurSDCCodingScheme
	        || eNewMSCCoSc != eCurMSCCodingScheme
	)
	{
		InitCharType = CPlotManager::ALL_CONSTELLATION;
        eCurSDCCodingScheme = eNewSDCCoSc;
        eCurMSCCodingScheme = eNewMSCCoSc;

        ConstellationPlotDefaults(tr("MSC / SDC / FAC"), 1.5, 4);
        clear();
        curve1 = ScatterCurve("FAC", MarkerSymFAC);
        curve2 = ScatterCurve("SDC", MarkerSymSDC);
        curve3 = ScatterCurve("MSC", MarkerSymMSC);
        curve1->setItemAttribute(QwtPlotItem::Legend, true);
        curve2->setItemAttribute(QwtPlotItem::Legend, true);
        curve3->setItemAttribute(QwtPlotItem::Legend, true);
	}

    SetData(curve1, veccFAC);
    SetData(curve2, veccSDC);
    SetData(curve3, veccMSC);

	replot();
}

void CDRMPlot::OnClicked(const QMouseEvent& e)
{
	/* Get frequency from current cursor position */
	const double dFreq = invTransform(QwtPlot::xBottom, e.x());

	/* Send normalized frequency to receiver */
	const double dMaxxBottom = axisScaleDiv(QwtPlot::xBottom)->hBound();

	/* Check if value is valid */
	if (dMaxxBottom != (double) 0.0)
	{
		/* Emit signal containing normalized selected frequency */
		emit xAxisValSet(dFreq / dMaxxBottom);
	}
}

void CDRMPlot::AddWhatsThisHelpChar(const CPlotManager::EPlotType NCharType)
{
	QString strCurPlotHelp;

	switch (NCharType)
	{
	case CPlotManager::AVERAGED_IR:
		/* Impulse Response */
		strCurPlotHelp =
			tr("<b>Impulse Response:</b> This plot shows "
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

	case CPlotManager::TRANSFERFUNCTION:
		/* Transfer Function */
		strCurPlotHelp =
			tr("<b>Transfer Function / Group Delay:</b> "
			"This plot shows the squared magnitude and the group delay of "
			"the estimated channel at each sub-carrier.");
		break;

	case CPlotManager::FAC_CONSTELLATION:
	case CPlotManager::SDC_CONSTELLATION:
	case CPlotManager::MSC_CONSTELLATION:
	case CPlotManager::ALL_CONSTELLATION:
		/* Constellations */
		strCurPlotHelp =
			tr("<b>FAC, SDC, MSC:</b> The plots show the "
			"constellations of the FAC, SDC and MSC logical channel of the DRM "
			"stream. Depending on the current transmitter settings, the SDC "
			"and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
		break;

	case CPlotManager::POWER_SPEC_DENSITY:
		/* Shifted PSD */
		strCurPlotHelp =
			tr("<b>Shifted PSD:</b> This plot shows the "
			"estimated Power Spectrum Density (PSD) of the input signal. The "
			"DC frequency (red dashed vertical line) is fixed at 6 kHz. If "
			"the frequency offset acquisition was successful, the rectangular "
			"DRM spectrum should show up with a center frequency of 6 kHz. "
			"This plot represents the frequency synchronized OFDM spectrum. "
			"If the frequency synchronization was successful, the useful "
			"signal really shows up only inside the actual DRM bandwidth "
			"since the side loops have in this case CPlotManager::only energy between the "
			"samples in the frequency domain. On the sample positions outside "
			"the actual DRM spectrum, the DRM signal has zero crossings "
			"because of the orthogonality. Therefore this spectrum represents "
			"NOT the actual spectrum but the \"idealized\" OFDM spectrum.");
		break;

	case CPlotManager::SNR_SPECTRUM:
		/* SNR Spectrum (Weighted MER on MSC Cells) */
		strCurPlotHelp =
			tr("<b>SNR Spectrum (Weighted MER on MSC Cells):</b> "
			"This plot shows the Weighted MER on MSC cells for each carrier "
			"separately.");
		break;

	case CPlotManager::INPUTSPECTRUM_NO_AV:
		/* Input Spectrum */
		strCurPlotHelp =
			tr("<b>Input Spectrum:</b> This plot shows the "
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

	case CPlotManager::INPUT_SIG_PSD:
	case CPlotManager::INPUT_SIG_PSD_ANALOG:
		/* Input PSD */
		strCurPlotHelp =
			tr("<b>Input PSD:</b> This plot shows the "
			"estimated power spectral density (PSD) of the input signal. The "
			"PSD is estimated by averaging some Hamming Window weighted "
			"Fourier transformed blocks of the input signal samples. The "
			"dashed vertical line shows the estimated DC frequency.");
		break;

	case CPlotManager::AUDIO_SPECTRUM:
		/* Audio Spectrum */
		strCurPlotHelp =
			tr("<b>Audio Spectrum:</b> This plot shows the "
			"averaged audio spectrum of the currently played audio. With this "
			"plot the actual audio bandwidth can easily determined. Since a "
			"linear scale is used for the frequency axis, most of the energy "
			"of the signal is usually concentrated on the far left side of the "
			"spectrum.");
		break;

	case CPlotManager::FREQ_SAM_OFFS_HIST:
		/* Frequency Offset / Sample Rate Offset History */
		strCurPlotHelp =
			tr("<b>Frequency Offset / Sample Rate Offset History:"
			"</b> The history "
			"of the values for frequency offset and sample rate offset "
			"estimation is shown. If the frequency offset drift is very small, "
			"this is an indication that the analog front end is of high "
			"quality.");
		break;

	case CPlotManager::DOPPLER_DELAY_HIST:
		/* Doppler / Delay History */
		strCurPlotHelp =
			tr("<b>Doppler / Delay History:</b> "
			"The history of the values for the "
			"Doppler and Impulse response length is shown. Large Doppler "
			"values might be responsable for audio drop-outs.");
		break;

	case CPlotManager::SNR_AUDIO_HIST:
		/* SNR History */
		strCurPlotHelp =
			tr("<b>SNR History:</b> "
			"The history of the values for the "
			"SNR and correctly decoded audio blocks is shown. The maximum "
			"achievable number of correctly decoded audio blocks per DRM "
			"frame is 10 or 5 depending on the audio sample rate (24 kHz "
			"or 12 kHz AAC core bandwidth).");
		break;

	case CPlotManager::INP_SPEC_WATERF:
		/* Waterfall Display of Input Spectrum */
		strCurPlotHelp =
			tr("<b>Waterfall Display of Input Spectrum:</b> "
			"The input spectrum is displayed as a waterfall type. The "
			"different colors represent different levels.");
		break;

	case CPlotManager::NONE_OLD:
		break;
	}

	/* Main plot */
	QWhatsThis::add(this, strCurPlotHelp);
}
