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

#if !defined(DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_)
#define DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_spectrogram.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qwhatsthis.h>
#include "../PlotManager.h"
#include <deque>

/* Definitions ****************************************************************/
#define GUI_CONTROL_UPDATE_WATERFALL			100	/* Milliseconds */


/* Define the plot color profiles */
/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION	blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT				white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION	red
#define GREENBLACK_BCKGRD_COLOR_PLOT			black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION	green
#define BLACKGREY_BCKGRD_COLOR_PLOT				gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128)


/* Maximum and minimum values of x-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB				double( -125.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB				double( -25.0)

/* Maximum and minimum values of x-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB				double( -85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB				double( -35.0)

/* Maximum and minimum values of x-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB				double( 0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB				double( 35.0)


/* Classes ********************************************************************/
class SpectrogramData: public QwtRasterData
{
public:
    SpectrogramData():QwtRasterData(),data(),scale(),height(0)
    {
    }

    virtual QwtRasterData *copy() const;

    virtual QwtDoubleInterval range() const
    {
        return QwtDoubleInterval(MIN_VAL_INP_SPEC_Y_AXIS_DB, MAX_VAL_INP_SPEC_Y_AXIS_DB);
    }

    virtual double value(double x, double y) const;
    void setHeight(size_t h);
    void setData(vector<double>& row);

protected:
    deque<vector<double> > data;
    vector<double> scale;
    size_t height;
};

class CDRMPlot : public QwtPlot
{
    Q_OBJECT

public:

	CDRMPlot(QWidget *p = 0, const char *name = 0);
	virtual ~CDRMPlot() {}

	/* This function has to be called before chart can be used! */
	void SetPlotManager(CPlotManager* pm) {pPlotManager = pm;}

	void SetupChart(const CPlotManager::EPlotType eNewType);
	CPlotManager::EPlotType GetChartType() const {return CurCharType;}
	void Update() {OnTimerChart();}

	void SetPlotStyle(const int iNewStyleID);

protected:
	void SetAvIR();
	void SetTranFct();
	void SetAudioSpec();
	void SetPSD();
	void SetSNRSpectrum();
	void SetInpSpec();
	void SetInpPSD();
    void SetInpPSDAnalog();
	void SetInpSpecWaterf();
	void SetFreqSamOffsHist();
	void SetDopplerDelayHist();
	void SetSNRAudHist();
	void SetFACConst();
	void SetSDCConst();
	void SetMSCConst();
	void SetAllConst();
    void SetData(QwtPlotCurve* curve, vector<_COMPLEX>& veccData);

	void SpectrumPlotDefaults(const QString&, const QString&, uint);
	void SetDCCarrier(double);
    void ConstellationPlotDefaults(const QString& title, double limit, int n);
    QwtPlotCurve* ScatterCurve(const QString& title, const QwtSymbol& s);

	void AddWhatsThisHelpChar(const CPlotManager::EPlotType NCharType);
    virtual void showEvent(QShowEvent* pEvent);
	virtual void hideEvent(QHideEvent* pEvent);

	/* Colors */
	QColor			MainPenColorPlot;
	QColor			MainPenColorConst;
	QColor			MainGridColorPlot;
	QColor			SpecLine1ColorPlot;
	QColor			SpecLine2ColorPlot;
	QColor			PassBandColorPlot;
	QColor			BckgrdColorPlot;

	/* Axis Titles */
	QwtText         leftTitle, rightTitle, bottomTitle;

	QSize			LastCanvasSize;
	ECodScheme      eCurSDCCodingScheme;
	ECodScheme      eCurMSCCodingScheme;

	CPlotManager::EPlotType		CurCharType;
	CPlotManager::EPlotType		InitCharType;
	QwtPlotCurve	*main1curve, *main2curve;
	QwtPlotCurve	*DCCarrierCurve, *BandwidthMarkerCurve;
	QwtPlotCurve	*curve1, *curve2, *curve3, *curve4, *curve5, *curve6;
	QwtSymbol		MarkerSymFAC, MarkerSymSDC, MarkerSymMSC;
    QwtPlotGrid     grid;
    QwtPlotSpectrogram* spectrogram;
    SpectrogramData spectrogramData;

	bool		    bOnTimerCharMutexFlag;
	QTimer			TimerChart;

    CPlotManager*   pPlotManager;

public slots:
	void OnClicked(const QMouseEvent& e);
	void OnTimerChart();

signals:
	void xAxisValSet(double);
};


#endif // DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_
