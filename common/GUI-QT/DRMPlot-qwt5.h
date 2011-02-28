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

#ifndef __DRMPLOT_QWT5_H
#define __DRMPLOT_QWT5_H

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qwhatsthis.h>
#include "../util/Vector.h"
#include "../Parameter.h"
#include "../DrmReceiver.h"


/* Definitions ****************************************************************/
#define GUI_CONTROL_UPDATE_WATERFALL			100	/* Milliseconds */

/* Maximum and minimum values of x-axis of input spectrum plots */
#define MIN_VAL_INP_SPEC_Y_AXIS_DB				((double) -125.0)
#define MAX_VAL_INP_SPEC_Y_AXIS_DB				((double) -25.0)

/* Maximum and minimum values of x-axis of input PSD (shifted) */
#define MIN_VAL_SHIF_PSD_Y_AXIS_DB				((double) -85.0)
#define MAX_VAL_SHIF_PSD_Y_AXIS_DB				((double) -35.0)

/* Maximum and minimum values of x-axis of SNR spectrum */
#define MIN_VAL_SNR_SPEC_Y_AXIS_DB				((double) 0.0)
#define MAX_VAL_SNR_SPEC_Y_AXIS_DB				((double) 35.0)


/* Classes ********************************************************************/
class CDRMPlot : public QwtPlot
{
    Q_OBJECT

public:
    enum ECharType
    {
        INPUT_SIG_PSD = 0, /* default */
        TRANSFERFUNCTION = 1,
        FAC_CONSTELLATION = 2,
        SDC_CONSTELLATION = 3,
        MSC_CONSTELLATION = 4,
        POWER_SPEC_DENSITY = 5,
        INPUTSPECTRUM_NO_AV = 6,
        AUDIO_SPECTRUM = 7,
        FREQ_SAM_OFFS_HIST = 8,
        DOPPLER_DELAY_HIST = 9,
        ALL_CONSTELLATION = 10,
        SNR_AUDIO_HIST = 11,
        AVERAGED_IR = 12,
        SNR_SPECTRUM = 13,
        INPUT_SIG_PSD_ANALOG = 14,
        INP_SPEC_WATERF = 15,
        NONE_OLD = 16 /* None must always be the last element! (see settings) */
    };

    CDRMPlot(QWidget *p = 0, const char *name = 0);
    virtual ~CDRMPlot() {}

    /* This function has to be called before chart can be used! */
    void SetRecObj(CDRMReceiver* pNDRMRec) {
        pDRMRec = pNDRMRec;
    }

    void SetupChart(const ECharType eNewType);
    ECharType GetChartType() const {
        return CurCharType;
    }
    void Update() {
        OnTimerChart();
    }
    void SetPlotStyle(const int iNewStyleID);


protected:
    void SetVerticalBounds(
        const _REAL rStartGuard, const _REAL rEndGuard,
        const _REAL rBeginIR, const _REAL rEndIR);
    void SetHorizontalBounds( _REAL rScaleMin, _REAL rScaleMax, _REAL rLowerB, _REAL rHigherB);
    void SetInpSpecWaterf(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void SetDCCarrier(const _REAL rDCFreq);
    void SetBWMarker(const _REAL rBWCenter, const _REAL rBWWidth);
    void AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
                   CVector<_REAL>& vecrScale);
    void AutoScale2(CVector<_REAL>& vecrData,
                    CVector<_REAL>& vecrData2,
                    CVector<_REAL>& vecrScale);
    void AutoScale3(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
    void SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
                 CVector<_REAL>& vecrScale);
    void SetData(CVector<_COMPLEX>& veccData);
    void SetData(CVector<_COMPLEX>& veccMSCConst,
                 CVector<_COMPLEX>& veccSDCConst,
                 CVector<_COMPLEX>& veccFACConst);
    void SetQAM4Grid();
    void SetQAM16Grid();
    void SetQAM64Grid();

    void SetupAvIR();
    void SetupTranFct();
    void SetupAudioSpec();
    void SetupFreqSamOffsHist();
    void SetupDopplerDelayHist();
    void SetupSNRAudHist();
    void SetupPSD();
    void SetupSNRSpectrum();
    void SetupInpSpec();
    void SetupFACConst();
    void SetupSDCConst(const ECodScheme eNewCoSc);
    void SetupMSCConst(const ECodScheme eNewCoSc);
    void SetupAllConst();
    void SetupInpPSD();
    void SetupInpSpecWaterf();

    void AddWhatsThisHelpChar(const ECharType NCharType);
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

    QSize			LastCanvasSize;

    ECharType		CurCharType;
    ECharType		InitCharType;
    QwtPlotCurve	*main1curve, *main2curve;
    QwtPlotCurve	*curve1, *curve2, *curve3, *curve4, *curve5;
    QwtPlotGrid*    	grid;
    QwtSymbol		MarkerSym1, MarkerSym2, MarkerSym3;
    QwtText         	leftTitle, rightTitle, bottomTitle;

    _BOOLEAN		bOnTimerCharMutexFlag;
    QTimer		TimerChart;

    CDRMReceiver*	pDRMRec;


public slots:
    void OnClicked(const QMouseEvent& e);
    void OnTimerChart();

signals:
    void xAxisValSet(double);
};


#endif
