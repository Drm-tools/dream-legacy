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
#include <algorithm>
#include <functional>
#include <qwt_scale_engine.h>
#include <qwt_legend.h>
#include <qwt_color_map.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>
#include <QPixmap>
#include <QFrame>
#include <QLayout>
#include <iostream>

/* TODO - see if we have lost any dynamic rescaling */

/* Implementation *************************************************************/
class scaleGen
{
public:
  scaleGen(double step = 1.0, double start=0.0):
    current(start), interval(step) {}
  double operator() () { return current+=interval; }
private:
  double current;
  double interval;
};

template<typename T>
void boundValues(const vector<T>& vec, T& min, T&max)
{
	max = - numeric_limits<T>::max();
	min = numeric_limits<T>::max();

	for (size_t i = 0; i < vec.size(); i++)
	{
		if (vec[i] > max)
			max = vec[i];

		if (vec[i] < min)
			min = vec[i];
	}
}

CDRMPlot::CDRMPlot(QwtPlot *p, CDRMReceiver* rec) :
	CurrentChartType(CPlotManager::NONE_OLD), WantedChartType(CPlotManager::NONE_OLD),
	grid(NULL), spectrogram(NULL),bOnTimerCharMutexFlag(false),pPlotManager(NULL),
	plot(p)
{

    //addWidget(plot);  // TODO needed ?
    pPlotManager = new CPlotManager(rec);

    grid = new QwtPlotGrid();

	/* Grid defaults */
	grid->enableXMin(false);
	grid->enableYMin(false);
	grid->attach(plot);

	/* Fonts */
	QFont axisfont;
	axisfont.setPointSize(8);
	axisfont.setStyleHint(QFont::SansSerif, QFont::PreferOutline);
	QFont titlefont(axisfont);
	titlefont.setWeight(QFont::Bold);

	plot->setAxisFont(QwtPlot::xBottom, axisfont);
	plot->setAxisFont(QwtPlot::yLeft, axisfont);
	plot->setAxisFont(QwtPlot::yRight, axisfont);
	QwtText title;
	title.setFont(titlefont);
	plot->setTitle(title);

    /* axis titles */
    bottomTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::xBottom, bottomTitle);

    leftTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::yLeft, leftTitle);

    rightTitle.setFont(axisfont);
    plot->setAxisTitle(QwtPlot::yRight, rightTitle);

	/* Global frame */
	plot->setFrameStyle(QFrame::Panel|QFrame::Sunken);
	plot->setLineWidth(2);
	plot->setMargin(10);

	/* Canvas */
	plot->setCanvasLineWidth(0);

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
    plot->insertLegend(new QwtLegend(), QwtPlot::RightLegend);

    /* For torn off plots */
	plot->setCaption(tr("Chart Window"));
    plot->setIcon(QPixmap(":/icons/MainIcon.png"));

	/* Connections */
	// TODO use a QwtPlotPicker http://qwt.sourceforge.net/class_qwt_plot_picker.html
	QwtPlotPicker* picker = new QwtPlotPicker(
        QwtPlot::xBottom, QwtPlot::yLeft, QwtPicker::PointSelection,
        QwtPicker::NoRubberBand, QwtPicker::AlwaysOff, plot->canvas()
    );
	connect(picker, SIGNAL(selected(const QwtDoublePoint&)), this, SLOT(OnClicked(const QwtDoublePoint&)));
	connect(&TimerChart, SIGNAL(timeout()), this, SLOT(OnTimerChart()));

	TimerChart.stop();
}

CDRMPlot::~CDRMPlot()
{
    // TODO - do we need stuff here or does QT do it all ?
}

void CDRMPlot::load(const CSettings& s, const string& section)
{
	CWinGeom g;
	s.Get(section, g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
	{
        plot->setGeometry(WinGeom);
	}
	SetPlotStyle(s.Get(section, "plotstyle", 0));
	SetupChart(CPlotManager::EPlotType(s.Get(section, "plottype", 0)));
}

void CDRMPlot::save(CSettings& s, const string& section)
{
    /* Check, if window wasn't closed by the user */
    if (plot->isVisible())
    {
        CWinGeom c;
        const QRect CWGeom = plot->geometry();

        /* Set parameters */
        c.iXPos = CWGeom.x();
        c.iYPos = CWGeom.y();
        c.iHSize = CWGeom.height();
        c.iWSize = CWGeom.width();

        s.Put(section, c);
        /* Convert plot type into an integer type. TODO: better solution */
        s.Put(section, "plottype", (int) GetChartType());
        s.Put(section, "plotstyle", styleId);
    }
    // if its a free standing window, close it
    if(plot->parent() == NULL)
        plot->close();
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

	bool chartChangeNeeded = false;

    ECodScheme e = pPlotManager->GetSDCCodingScheme();
    if(e!=eCurSDCCodingScheme)
    {
        eCurSDCCodingScheme = e;
        chartChangeNeeded = true;
    }

    e = pPlotManager->GetMSCCodingScheme();
    if(e!=eCurMSCCodingScheme)
    {
        eCurMSCCodingScheme = e;
        chartChangeNeeded = true;
    }

    if(CurrentChartType != WantedChartType)
    {
        chartChangeNeeded = true;
    }

    if(chartChangeNeeded)
    {
        SetupChartNow();
        CurrentChartType = WantedChartType;
    }
    UpdateChartNow();
	/* "Unlock" mutex flag */
	bOnTimerCharMutexFlag = false;
}

void CDRMPlot::SetupChartNow()
{
    pPlotManager->endPlot(CurrentChartType);
    pPlotManager->startPlot(WantedChartType);

    plot->clear();

	switch (WantedChartType)
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
		SetInpSpectrum();
		break;

	case CPlotManager::INP_SPEC_WATERF:
		SetInpSpecWaterf();
		break;

	case CPlotManager::INPUT_SIG_PSD:
	case CPlotManager::INPUT_SIG_PSD_ANALOG:
		SetInpPSD();
		break;

	case CPlotManager::AUDIO_SPECTRUM:
		SetAudioSpectrum();
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
	CurrentChartType = WantedChartType;
}

void CDRMPlot::UpdateChartNow()
{
	switch (CurrentChartType)
	{
	case CPlotManager::AVERAGED_IR:
		UpdateAvIR();
		break;

	case CPlotManager::TRANSFERFUNCTION:
		UpdateTranFct();
		break;

	case CPlotManager::POWER_SPEC_DENSITY:
		UpdatePSD();
		break;

	case CPlotManager::SNR_SPECTRUM:
		UpdateSNRSpectrum();
		break;

	case CPlotManager::INPUTSPECTRUM_NO_AV:
		UpdateInpSpectrum();
		break;

	case CPlotManager::INP_SPEC_WATERF:
		UpdateInpSpecWaterf();
		break;

	case CPlotManager::INPUT_SIG_PSD:
	case CPlotManager::INPUT_SIG_PSD_ANALOG:
		UpdateInpPSD();
		break;

	case CPlotManager::AUDIO_SPECTRUM:
		UpdateAudioSpectrum();
		break;

	case CPlotManager::FREQ_SAM_OFFS_HIST:
		UpdateFreqSamOffsHist();
		break;

	case CPlotManager::DOPPLER_DELAY_HIST:
		UpdateDopplerDelayHist();
		break;

	case CPlotManager::SNR_AUDIO_HIST:
		UpdateSNRAudHist();
		break;

	case CPlotManager::FAC_CONSTELLATION:
		UpdateFACConst();
		break;

	case CPlotManager::SDC_CONSTELLATION:
		UpdateSDCConst();
		break;

	case CPlotManager::MSC_CONSTELLATION:
		UpdateMSCConst();
		break;

	case CPlotManager::ALL_CONSTELLATION:
		UpdateAllConst();
		break;

	case CPlotManager::NONE_OLD:
		break;
	}
    plot->replot();
}

void CDRMPlot::SetupChart(const CPlotManager::EPlotType eNewType)
{
	if (eNewType == CPlotManager::NONE_OLD)
        return;

    if(eNewType == CPlotManager::INP_SPEC_WATERF)
    {
    }
    {
        if(spectrogram)
        {
            spectrogram->detach();
            spectrogram = NULL;
        }
    }

    /* Set internal variable */
    WantedChartType = eNewType;

    /* Update help text connected with the plot widget */
    AddWhatsThisHelpChar(eNewType);

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
    SetupChartNow();
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
	QColor BckgrdColorPlot;

    styleId = iNewStyleID;

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
	grid->setMajPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
	grid->setMinPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
	plot->setCanvasBackground(BckgrdColorPlot);

	/* Make sure that plot are being initialized again */
	WantedChartType = CPlotManager::NONE_OLD;
}

void CDRMPlot::SetData(QwtPlotCurve* curve, vector<_COMPLEX>& veccData)
{
	const int iPoints = veccData.size();
	/* Copy data from vector into a temporary array */
	double pdX[iPoints];
	double pdY[iPoints];
	for (int i = 0; i < iPoints; i++)
	{
		pdX[i] = veccData[i].real();
		pdY[i] = veccData[i].imag();
	}
	curve->setData(pdX, pdY, iPoints);
}

void CDRMPlot::SetAvIR()
{
    plot->setTitle(tr("Channel Impulse Response"));
    plot->enableAxis(QwtPlot::yRight, false);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));

    /* Fixed vertical scale  */
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));
    plot->setAxisScale(QwtPlot::yLeft, -20.0, 40.0);

    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    /* Insert curves */
    curve1 = new QwtPlotCurve(tr("Guard-interval beginning"));
    curve2 = new QwtPlotCurve(tr("Guard-interval end"));
    curve3 = new QwtPlotCurve(tr("Estimated begin of impulse response"));
    curve4 = new QwtPlotCurve(tr("Estimated end of impulse response"));
    curve1->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve2->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve3->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve4->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));

    curve1->setItemAttribute(QwtPlotItem::Legend, false);
    curve2->setItemAttribute(QwtPlotItem::Legend, false);
    curve3->setItemAttribute(QwtPlotItem::Legend, false);
    curve4->setItemAttribute(QwtPlotItem::Legend, false);

    curve1->attach(plot);
    curve2->attach(plot);
    curve3->attach(plot);
    curve4->attach(plot);

    curve5 = new QwtPlotCurve(tr("Higher Bound"));
    curve5->setItemAttribute(QwtPlotItem::Legend, false);
#ifdef _DEBUG_
    curve6 = new QwtPlotCurve(tr("Lower bound"));
    curve5->setPen(QPen(SpecLine1ColorPlot));
    curve6->setPen(QPen(SpecLine2ColorPlot));
    curve6->setItemAttribute(QwtPlotItem::Legend, false);
    curve5->attach(plot);
    curve6->attach(plot);
#else
    curve5->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve5->attach(plot);
#endif

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("Channel Impulse Response"));

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);
    main1curve->attach(plot);
}

void CDRMPlot::UpdateAvIR()
{
    CMeasurements::CPIR pir;
    /* Get data from module */
    if(pPlotManager->GetAvPoDeSp(pir)==false)
    {
        return; // data not being generated yet
    }

	if(pir.data.size() == 0)
        return;

    /* Vertical bounds -------------------------------------------------- */
    double dX[2], dY[2];

    dY[0] = plot->axisScaleDiv(QwtPlot::yLeft)->lBound();
    dY[1] = plot->axisScaleDiv(QwtPlot::yLeft)->hBound();

    /* These bounds show the beginning and end of the guard-interval */

    /* Left bound */
    dX[0] = dX[1] = pir.rStartGuard;
    curve1->setData(dX, dY, 2);

    /* Right bound */
    dX[0] = dX[1] = pir.rEndGuard;
    curve2->setData(dX, dY, 2);

    /* Estimated begin of impulse response */
    dX[0] = dX[1] = pir.rPDSBegin;
    curve3->setData(dX, dY, 2);

    /* Estimated end of impulse response */
    dX[0] = dX[1] = pir.rPDSEnd;
    curve4->setData(dX, dY, 2);

    /* Data for the actual impulse response curve */
    vector<_REAL> vecrScale(pir.data.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(pir.rStep, pir.rStart));
    main1curve->setData(&vecrScale[0], &pir.data[0], vecrScale.size());

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
    dY[0] = dY[1] = max(pir.rHigherBound, pir.rLowerBound);
#endif
    curve5->setData(dX, dY, 2);

    /* Adjust scale for x-axis */
    plot->setAxisScale(QwtPlot::xBottom, vecrScale[0], vecrScale[vecrScale.size() - 1]);
}

void CDRMPlot::SetTranFct()
{
    plot->setTitle(tr("Channel Transfer Function / Group Delay"));
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));
    plot->setAxisScale(QwtPlot::yLeft,  -85.0,  -35.0);

    plot->enableAxis(QwtPlot::yRight);
    plot->setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
    plot->setAxisScale(QwtPlot::yRight, -50.0, 50.0);

    /* Add curves */

    /* TODO - check that its group delay that should be scaled to right axis!! */
    main1curve = new QwtPlotCurve(tr("Transf. Fct."));
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main2curve = new QwtPlotCurve(tr("Group Del."));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setYAxis(QwtPlot::yRight);

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateTranFct()
{
    vector<double> transferFunc, groupDelay, scale;
    pPlotManager->GetTransferFunction(transferFunc, groupDelay);

	scale.resize(transferFunc.size());
    generate(scale.begin(), scale.end(), scaleGen());

	plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) scale.size());

	main1curve->setData(&scale[0], &transferFunc[0], scale.size());
	main2curve->setData(&scale[0], &groupDelay[0], scale.size());
}

void CDRMPlot::SetAudioSpectrum()
{
    plot->setTitle(tr("Audio Spectrum"));
    plot->enableAxis(QwtPlot::yRight, false);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, "AS [dB]");
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
    double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2400; /* 20.0 for 48 kHz */
    if (dBandwidth < 20.0)
        dBandwidth = (double) 20.0;

    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("Audio Spectrum"));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);
    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->attach(plot);
}

void CDRMPlot::UpdateAudioSpectrum()
{
    /* Get data from module */
    vector<_REAL> vecrData, vecrScale;
    pPlotManager->GetAudioSpec(vecrData, vecrScale);
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}

void CDRMPlot::SetFreqSamOffsHist()
{
    plot->setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);
    plot->enableAxis(QwtPlot::yLeft, true);

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("Freq."));
    main2curve = new QwtPlotCurve(tr("Samp."));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);

    // initial values
    plot->setAxisScale(QwtPlot::yLeft, -10.0, 10.0);
    plot->setAxisScale(QwtPlot::yRight, -5.0, 5.0);
}

void CDRMPlot::UpdateFreqSamOffsHist()
{
	/* Calculate time scale */
    _REAL rTs = pPlotManager->GetSymbolDuration();
	_REAL rStep = -rTs;
	_REAL rStart = (1-LEN_HIST_PLOT_SYNC_PARMS)*rTs;
	plot->setAxisScale(QwtPlot::xBottom, rStart, 0.0);

    vector<double> vecrFreqOffs, vecrSamOffs, vecrScale;
    _REAL rFreqOffAcquVal;
	pPlotManager->GetFreqSamOffsHist( vecrFreqOffs, vecrSamOffs, rFreqOffAcquVal);

    /* left axis title can change */
	QString strYLeftLabel = tr("Freq. Offset [Hz] rel. to ")+QString().setNum(rFreqOffAcquVal) + " Hz";
    plot->setAxisTitle(QwtPlot::yLeft, strYLeftLabel);

	/* Customized auto-scaling. We adjust the y scale so that it is not larger
	   than rMinScaleRange"  */
	const _REAL rMinScaleRange = (_REAL) 1.0; /* Hz */

    if(vecrFreqOffs.size()>0)
    {
        /* Get maximum and minimum values */
        _REAL MinFreq, MaxFreq;
        boundValues(vecrFreqOffs, MinFreq, MaxFreq);
        /* Apply scale to plot */
        double ymin = floor(MinFreq / rMinScaleRange);
        double ymax = ceil(MaxFreq / rMinScaleRange);
        plot->setAxisScale(QwtPlot::yLeft, ymin, ymax);
        vecrScale.resize(vecrFreqOffs.size());
        _REAL rLeft = -_REAL(vecrFreqOffs.size()-1)*rStep;
        generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
        main1curve->setData(&vecrScale[0], &vecrFreqOffs[0], vecrFreqOffs.size());
    }

    if(vecrSamOffs.size()>0)
    {
        _REAL MinSam, MaxSam;
        boundValues(vecrSamOffs, MinSam, MaxSam);
        /* Apply scale to plot */
        double ymin = floor(MinSam / rMinScaleRange);
        double ymax = ceil(MaxSam / rMinScaleRange);
        plot->setAxisScale(QwtPlot::yRight, ymin, ymax);
        _REAL rLeft = -_REAL(vecrSamOffs.size()-1)*rStep;
        vecrScale.resize(vecrSamOffs.size());
        generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
        main2curve->setData(&vecrScale[0], &vecrSamOffs[0], vecrSamOffs.size());
    }
}

void CDRMPlot::SetDopplerDelayHist()
{
    plot->setTitle(tr("Delay / Doppler History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
    plot->setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("Delay"));
    main2curve = new QwtPlotCurve(tr("Doppler"));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateDopplerDelayHist()
{
    vector<double> vecrDelay, vecrDoppler, vecrScale;
	pPlotManager->GetDopplerDelHist(vecrDelay, vecrDoppler);
	_REAL rStep = pPlotManager->GetFrameDuration()/60.0;
	_REAL rStart = -(rStep*_REAL(LEN_HIST_PLOT_SYNC_PARMS-1));
    plot->setAxisScale(QwtPlot::xBottom, rStart, 0.0);
	vecrScale.resize(vecrDelay.size());
	_REAL rLeft = -_REAL(vecrDelay.size()-1)*rStep;
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main1curve->setData(&vecrScale[0], &vecrDelay[0], vecrDelay.size());
	vecrScale.resize(vecrDoppler.size());
    rLeft = -_REAL(vecrDoppler.size()-1)*rStep;
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
	main2curve->setData(&vecrScale[0], &vecrDoppler[0], vecrDoppler.size());
}

void CDRMPlot::SetSNRAudHist()
{
    plot->setTitle(tr("SNR / Correctly Decoded Audio History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    /* Add main curves */
    main1curve = new QwtPlotCurve(tr("SNR"));
    main2curve = new QwtPlotCurve(tr("Audio"));
    main2curve->setYAxis(QwtPlot::yRight);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    main1curve->attach(plot);
    main2curve->attach(plot);
}

void CDRMPlot::UpdateSNRAudHist()
{
    vector<double> vecrSNR, vecrAudio, vecrScale;
    pPlotManager->GetSNRHist(vecrSNR, vecrAudio);

	/* Customized auto-scaling. We adjust the y scale maximum so that it
	   is not more than "rMaxDisToMax" to the curve */
	const int iMaxDisToMax = 5; /* dB */
	const int iMinValueSNRYScale = 15; /* dB */

	/* Get maximum value */
	_REAL MaxSNR = numeric_limits<_REAL>::min();
	for (size_t i = 0; i < vecrSNR.size(); i++)
	{
		if (vecrSNR[i] > MaxSNR)
			MaxSNR = vecrSNR[i];
	}

	/* Quantize scale to a multiple of "iMaxDisToMax" */
	double dMaxYScaleSNR = ceil(MaxSNR / _REAL(iMaxDisToMax)) * _REAL(iMaxDisToMax);

	/* Bound at the minimum allowed value */
	if (dMaxYScaleSNR < double(iMinValueSNRYScale))
		dMaxYScaleSNR = double(iMinValueSNRYScale);

	/* Ratio between the maximum values for audio and SNR. The ratio should be
	   chosen so that the audio curve is not in the same range as the SNR curve
	   under "normal" conditions to increase readability of curves.
	   Since at very low SNRs, no audio can received anyway so we do not have to
	   check whether the audio y-scale is in range of the curve */
	const double rRatioAudSNR = 1.5;
	const double dMaxYScaleAudio = dMaxYScaleSNR * rRatioAudSNR;

	_REAL rStep = pPlotManager->GetFrameDuration()/60.0;
	_REAL rStart = -(rStep*_REAL(LEN_HIST_PLOT_SYNC_PARMS-1));

	/* Apply scale to plot */
	plot->setAxisScale(QwtPlot::yLeft, 0.0, dMaxYScaleSNR);
	plot->setAxisScale(QwtPlot::yRight, 0.0, dMaxYScaleAudio);
	plot->setAxisScale(QwtPlot::xBottom, rStart, 0.0);

    if(vecrSNR.size()>0)
    {
        vecrScale.resize(vecrSNR.size());
        _REAL rLeft = -_REAL(vecrSNR.size()-1)*rStep;
        generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
        main1curve->setData(&vecrScale[0], &vecrSNR[0], vecrSNR.size());
    }
    if(vecrAudio.size()>0)
    {
        vecrScale.resize(vecrAudio.size());
        _REAL rLeft = -_REAL(vecrAudio.size()-1)*rStep;
        generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rLeft));
        main2curve->setData(&vecrScale[0], &vecrAudio[0], vecrAudio.size());
    }
}

void CDRMPlot::SpectrumPlotDefaults(
    const QString& title, const QString& axistitle, uint penwidth)
{
	/* Init chart for power spectral density estimation */
	plot->setTitle(title);
	plot->enableAxis(QwtPlot::yRight, false);
	grid->enableX(true);
	grid->enableY(true);
	plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	plot->enableAxis(QwtPlot::yLeft, true);
	plot->setAxisTitle(QwtPlot::yLeft, axistitle+" [dB]");
	plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

	/* Fixed scale */
	plot->setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for DC carrier */
	DCCarrierCurve = new QwtPlotCurve(tr("DC carrier"));
	DCCarrierCurve->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
	DCCarrierCurve->setItemAttribute(QwtPlotItem::Legend, false);
	DCCarrierCurve->attach(plot);

	/* Add main curve */
	main1curve = new QwtPlotCurve(axistitle);
	main1curve->setPen(QPen(MainPenColorPlot, penwidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
	main1curve->setItemAttribute(QwtPlotItem::Legend, false);
	main1curve->attach(plot);
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
    /* Init chart for power spectral density estimation */
    SpectrumPlotDefaults(
        tr("Shifted Power Spectral Density of Input Signal"),
        tr("Shifted PSD"), 1);

    /* fixed values for DC Carrier line */
    SetDCCarrier(VIRTUAL_INTERMED_FREQ);
}

void CDRMPlot::UpdatePSD()
{
    vector<_REAL> vecrData, vecrScale;
    pPlotManager->GetPowDenSpec(vecrData, vecrScale);
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}

void CDRMPlot::SetSNRSpectrum()
{
    plot->setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
    plot->enableAxis(QwtPlot::yRight, false);
    grid->enableX(true);
    grid->enableY(true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));
    plot->canvas()->setBackgroundMode(Qt::PaletteBackground);

    /* Add main curve */
    main1curve = new QwtPlotCurve(tr("SNR Spectrum"));

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);

    main1curve->attach(plot);
}

void CDRMPlot::UpdateSNRSpectrum()
{
    vector<_REAL> vecrData, vecrScale;
    /* Get data from module */
    pPlotManager->GetSNRProfile(vecrData, vecrScale);

	const int iSize = vecrScale.size();

	/* Fixed scale for x-axis */
	plot->setAxisScale(QwtPlot::xBottom, 0.0, double(iSize));

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
		const double rEnlareStep = 10.0; /* dB */
		dMaxScaleYAxis = ceil(rMaxSNR / rEnlareStep) * rEnlareStep;
	}

	/* Set scale */
	plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

	/* Set actual data */
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}


void CDRMPlot::SetInpSpectrum()
{

    SpectrumPlotDefaults(tr("Input Spectrum"), tr("Input Spectrum"), 1);
}

void CDRMPlot::UpdateInpSpectrum()
{
    vector<_REAL> vecrData, vecrScale;
    SetDCCarrier(pPlotManager->GetDCFrequency());
    _REAL rStart, rStep;
    pPlotManager->GetInputSpectrum(vecrData, rStart, rStep);
	vecrScale.resize(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rStart));
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
}

void CDRMPlot::SetInpPSD()
{
    if(WantedChartType==CPlotManager::INPUT_SIG_PSD_ANALOG)
    {
        /* Insert line for bandwidth marker behind main curves */
        BandwidthMarkerCurve = new QwtPlotCurve(tr("Filter bandwidth"));

        BandwidthMarkerCurve->setBrush(PassBandColorPlot);
        BandwidthMarkerCurve->setItemAttribute(QwtPlotItem::Legend, false);
        BandwidthMarkerCurve->attach(plot);
    }
    SpectrumPlotDefaults(tr("Input PSD"), tr("Input PSD"), 2);
}

void CDRMPlot::UpdateInpPSD()
{
    _REAL rStart, rStep;
    vector<_REAL> vecrData;
    pPlotManager->GetInputPSD(vecrData, rStart, rStep);
    vector<_REAL> vecrScale(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rStart));

    if(WantedChartType==CPlotManager::INPUT_SIG_PSD_ANALOG)
    {
        SetDCCarrier(pPlotManager->GetAnalogCurMixFreqOffs());

        /* Insert marker for filter bandwidth if required */
        _REAL rBWCenter, rBWWidth;
        pPlotManager->GetAnalogBWParameters(rBWCenter, rBWWidth);
        if (rBWWidth != (_REAL) 0.0)
        {
            double dX[4], dY[4];
            dX[0] = dX[1] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
            dX[2] = dX[3] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

            /* Take the min-max values from scale to get vertical line */
            dY[0] = MAX_VAL_INP_SPEC_Y_AXIS_DB;
            dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
            dY[2] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
            dY[3] = MAX_VAL_INP_SPEC_Y_AXIS_DB;
            BandwidthMarkerCurve->setData(dX, dY, 4);
        }
        else
            BandwidthMarkerCurve->setData(NULL, NULL, 0);
    }
    else
    {
        SetDCCarrier(pPlotManager->GetDCFrequency());
    }
	main1curve->setData(&vecrScale[0], &vecrData[0], vecrData.size());
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
	const QSize CanvSize = plot->canvas()->size();

    LastCanvasSize = CanvSize;

    plot->setTitle(tr("Waterfall Input Spectrum"));
    plot->enableAxis(QwtPlot::yRight, false);
    plot->enableAxis(QwtPlot::yLeft, false);
    plot->enableAxis(QwtPlot::xBottom, true);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    // spectrogram uses scales to determine value range
    // each pixel represents one sample (400 ms ?)
    plot->setAxisScale(QwtPlot::yLeft, 0.0, CanvSize.height());
    plot->setAxisScale(QwtPlot::xBottom, 0.0, SOUNDCRD_SAMPLE_RATE / 2000);
    grid->enableX(false);
    grid->enableY(false);

    // TODO check with cvs plot->Layout()->setAlignCanvasToScales(true);
    spectrogramData.setHeight(CanvSize.height());
    spectrogram = new QwtPlotSpectrogram();
    spectrogram->attach(plot);
}

void CDRMPlot::UpdateInpSpecWaterf()
{
    _REAL rStart, rStep;
    vector<_REAL> vecrData;
    pPlotManager->GetInputSpectrum(vecrData, rStep, rStart);
    vector<_REAL> vecrScale(vecrData.size());
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rStep, rStart));
    spectrogramData.setData(vecrData);
    spectrogram->setData(spectrogramData);
}

void CDRMPlot::ConstellationPlotDefaults(const QString& title, double limit, int n)
{
	plot->setTitle(title+" "+tr("Constellation"));
	plot->enableAxis(QwtPlot::yRight, false);
	grid->enableX(true);
	grid->enableY(true);
	plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
	plot->enableAxis(QwtPlot::yLeft, true);
	plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));
	plot->canvas()->setBackgroundMode(Qt::PaletteBackground);
	double step = limit/n;
	plot->setAxisScale(QwtPlot::xBottom, -limit, limit, step);
	plot->setAxisScale(QwtPlot::yLeft, -limit, limit, step);
}

QwtPlotCurve* CDRMPlot::ScatterCurve(const QString& title, const QwtSymbol& s)
{
    QwtPlotCurve* curve = new QwtPlotCurve(title);
    curve->setSymbol(s);
    curve->setPen(QPen(Qt::NoPen));
    curve->setItemAttribute(QwtPlotItem::Legend, false);
    curve->attach(plot);
    return curve;
}

void CDRMPlot::SetFACConst()
{
    ConstellationPlotDefaults(tr("FAC"), 2.0 / sqrt(2.0), 1);
    curve1 = ScatterCurve("FAC", MarkerSymFAC);
}

void CDRMPlot::UpdateFACConst()
{
    vector<_COMPLEX> veccData;
	pPlotManager->GetFACVectorSpace(veccData);
	SetData(curve1, veccData);
}

void CDRMPlot::SetSDCConst()
{
    ConstellationPlotDefaults("SDC", 4.0 / sqrt(10.0), (eCurSDCCodingScheme == CS_1_SM)?1:2);
    curve2 = ScatterCurve("SDC", MarkerSymSDC);
}

void CDRMPlot::UpdateSDCConst()
{
    vector<_COMPLEX> veccData;
	pPlotManager->GetSDCVectorSpace(veccData);
	SetData(curve2, veccData);
}

void CDRMPlot::SetMSCConst()
{
    /* Fixed scale (8 / sqrt(42)) */
    ConstellationPlotDefaults("MSC", 8.0 / sqrt(42.0), (eCurMSCCodingScheme == CS_2_SM)?2:4);
    curve3 = ScatterCurve("MSC", MarkerSymMSC);
}

void CDRMPlot::UpdateMSCConst()
{
    vector<_COMPLEX> veccData;
	pPlotManager->GetMSCVectorSpace(veccData);
	SetData(curve3, veccData);
}

void CDRMPlot::SetAllConst()
{
    ConstellationPlotDefaults(tr("MSC / SDC / FAC"), 1.5, 4);
    curve1 = ScatterCurve("FAC", MarkerSymFAC);
    curve2 = ScatterCurve("SDC", MarkerSymSDC);
    curve3 = ScatterCurve("MSC", MarkerSymMSC);
    curve1->setItemAttribute(QwtPlotItem::Legend, true);
    curve2->setItemAttribute(QwtPlotItem::Legend, true);
    curve3->setItemAttribute(QwtPlotItem::Legend, true);
}

void CDRMPlot::UpdateAllConst()
{
    UpdateFACConst();
    UpdateSDCConst();
    UpdateMSCConst();
}

void CDRMPlot::OnClicked(const QwtDoublePoint& p)
{
	/* Get frequency from current cursor position */
	const double dFreq = p.x();

	/* Send normalized frequency to receiver */
	const double dMaxxBottom = plot->axisScaleDiv(QwtPlot::xBottom)->hBound();
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
	plot->setWhatsThis(strCurPlotHelp);
}
