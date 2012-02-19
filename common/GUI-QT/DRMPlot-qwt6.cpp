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

#include "DRMPlot-qwt6.h"

#include <QWhatsThis>
#include <QPixmap>
#include <QFrame>
#include <QHideEvent>
#include <QMouseEvent>
#include <QShowEvent>
#if QWT_VERSION < 0x060000
# define setSamples(x,y,s) setData(x,y,s)
#endif

/* Define the plot color profiles */

/* BLUEWHITE */
#define BLUEWHITE_MAIN_PEN_COLOR_PLOT			Qt::blue
#define BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION		Qt::blue
#define BLUEWHITE_BCKGRD_COLOR_PLOT			Qt::white
#define BLUEWHITE_MAIN_GRID_COLOR_PLOT			Qt::gray
#define BLUEWHITE_SPEC_LINE1_COLOR_PLOT			Qt::red
#define BLUEWHITE_SPEC_LINE2_COLOR_PLOT			Qt::black
#define BLUEWHITE_PASS_BAND_COLOR_PLOT			QColor(192, 192, 255)

/* GREENBLACK */
#define GREENBLACK_MAIN_PEN_COLOR_PLOT			Qt::green
#define GREENBLACK_MAIN_PEN_COLOR_CONSTELLATION		Qt::red
#define GREENBLACK_BCKGRD_COLOR_PLOT			Qt::black
#define GREENBLACK_MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
#define GREENBLACK_SPEC_LINE1_COLOR_PLOT		Qt::yellow
#define GREENBLACK_SPEC_LINE2_COLOR_PLOT		Qt::blue
#define GREENBLACK_PASS_BAND_COLOR_PLOT			QColor(0, 96, 0)

/* BLACKGREY */
#define BLACKGREY_MAIN_PEN_COLOR_PLOT			Qt::black
#define BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION		Qt::green
#define BLACKGREY_BCKGRD_COLOR_PLOT			Qt::gray
#define BLACKGREY_MAIN_GRID_COLOR_PLOT			Qt::white
#define BLACKGREY_SPEC_LINE1_COLOR_PLOT			Qt::blue
#define BLACKGREY_SPEC_LINE2_COLOR_PLOT			Qt::yellow
#define BLACKGREY_PASS_BAND_COLOR_PLOT			QColor(128, 128, 128)


Chart::Chart(CDRMReceiver *pDRMRec, QwtPlot* p):plot(p),grid(NULL),receiver(pDRMRec)
        //,MainPenColorPlot (), MainPenColorConst (), BckgrdColorPlot (), MainGridColorPlot(), SpecLine1ColorPlot(),
        //SpecLine2ColorPlot(), PassBandColorPlot()
{
    grid = new QwtPlotGrid();

    /* Grid defaults */
    grid->enableXMin(false);
    grid->enableYMin(false);
    grid->setPen(QPen(Qt::gray, 1, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
    grid->attach(plot);

    curve1 = new QwtPlotCurve("");
    curve2 = new QwtPlotCurve("");
    curve3 = new QwtPlotCurve("");
    curve4 = new QwtPlotCurve("");
    curve5 = new QwtPlotCurve("");
    main1curve = new QwtPlotCurve("");
    main2curve = new QwtPlotCurve("");

    plotManager = pDRMRec->GetPlotManager();
}

void Chart::SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
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

    main1curve->setSamples(pdScale, pdData, vecrData.Size());
    delete[] pdData;
    delete[] pdScale;
}

void Chart::SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
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

    main1curve->setSamples(pdScale, pdData1, vecrData1.Size());
    main2curve->setSamples(pdScale, pdData2, vecrData2.Size());
    delete[] pdData1;
    delete[] pdData2;
    delete[] pdScale;
}

void Chart::SetData(QwtPlotCurve* curve, CVector<_COMPLEX>& veccData, const QwtSymbol& marker)
{
    /* Copy data from vectors in temporary arrays */
    int iDataSize = veccData.Size();
    double* pdData = new double[iDataSize];
    double* pdScale = new double[iDataSize];
    for (int i = 0; i < iDataSize; i++)
    {
        pdData[i] = veccData[i].real();
        pdScale[i] = veccData[i].imag();
    }
#if QWT_VERSION < 0x060000
    curve->setSymbol( marker );
#else
    curve->setSymbol( &marker );
#endif
    curve->setStyle( QwtPlotCurve::NoCurve );
    curve->setSamples(pdScale, pdData, iDataSize);
    delete[] pdData;
    delete[] pdScale;
}

void Chart::SetDCCarrier()
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    _REAL rDCFreq = Parameters.GetDCFrequency();
    Parameters.Unlock();

    /* Insert line for DC carrier */
    double dX[2], dY[2];
    dX[0] = dX[1] = rDCFreq / 1000;

    /* Take the min-max values from scale to get vertical line */
    dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
    dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

    curve1->setSamples(dX, dY, 2);
}

void Chart::SetBWMarker(const _REAL rBWCenter, const _REAL rBWWidth)
{
    double	dX[2], dY[2];
    /* Insert marker for filter bandwidth if required */
    if (rBWWidth != (_REAL) 0.0)
    {
        dX[0] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
        dX[1] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

        /* Take the min-max values from scale to get vertical line */
        dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
        dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;

        curve2->setSamples(dX, dY, 2);
    }
    else
    {
        curve2->setSamples(NULL, NULL, 0);
    }

}

void Chart::SetPlotStyle(const int iNewStyleID)
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
    grid->setMajPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
    grid->setMinPen(QPen(MainGridColorPlot, 0, Qt::DotLine));
    plot->setCanvasBackground(QColor(BckgrdColorPlot));

    /* was in every use of this */
    plot->setStyleSheet("QwtPlot { padding: 1px }");

    Setup();
}

AvIR::AvIR(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void AvIR::Setup()
{
    /* Init chart for averaged impulse response */
    plot->setTitle(tr("Channel Impulse Response"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));
    /* Insert curves */
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

    curve5->setItemAttribute(QwtPlotItem::Legend, false);
    curve5->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
    curve5->attach(plot);

    main1curve->setTitle(tr("Channel Impulse Response"));

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main1curve->setItemAttribute(QwtPlotItem::Legend, false);
}

void AvIR::Update()
{
    /* Get data from module */
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    _REAL rLowerBound, rHigherBound, rStartGuard, rEndGuard, rPDSBegin, rPDSEnd;;
    plotManager->GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
                             rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);

    if (vecrScale.Size() != 0)
    {
        SetVerticalBounds(rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);
        SetData(vecrData, vecrScale);
        SetHorizontalBounds(vecrScale[0], vecrScale[vecrScale.Size() - 1], rLowerBound, rHigherBound);
    }
    else
    {
        /* No input data */
	plot->clear();
    }
}

void AvIR::SetVerticalBounds(
    const _REAL rStartGuard, const _REAL rEndGuard,
    const _REAL rBeginIR, const _REAL rEndIR)
{
    /* Fixed scale */
    const double cdAxMinLeft = (double) -20.0;
    const double cdAxMaxLeft = (double) 40.0;
    plot->setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);

    /* Vertical bounds -------------------------------------------------- */
    double dX[2], dY[2];

    /* These bounds show the beginning and end of the guard-interval */
    dY[0] = cdAxMinLeft;
    dY[1] = cdAxMaxLeft;

    /* Left bound */
    dX[0] = dX[1] = rStartGuard;
    curve1->setSamples(dX, dY, 2);

    /* Right bound */
    dX[0] = dX[1] = rEndGuard;
    curve2->setSamples(dX, dY, 2);
    /* Estimated begin of impulse response */
    dX[0] = dX[1] = rBeginIR;
    curve3->setSamples(dX, dY, 2);

    /* Estimated end of impulse response */
    dX[0] = dX[1] = rEndIR;
    curve4->setSamples(dX, dY, 2);
}

void AvIR::SetHorizontalBounds( _REAL rScaleMin, _REAL rScaleMax, _REAL rLowerB, _REAL rHigherB)
{
    double dX[2], dY[2];
    /* These bounds show the peak detection bound from timing tracking */
    dX[0] = rScaleMin;
    dX[1] = rScaleMax;

    /* Only include highest bound */
    dY[0] = dY[1] = Max(rHigherB, rLowerB);
    curve5->setSamples(dX, dY, 2);
    /* Adjust scale for x-axis */
    plot->setAxisScale(QwtPlot::xBottom, (double) rScaleMin, (double) rScaleMax);
}

InpSpecWaterf::InpSpecWaterf(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void InpSpecWaterf::Setup()
{
    plot->setTitle(tr("Waterfall Input Spectrum"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(FALSE);
    grid->enableYMin(FALSE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, FALSE);

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::xBottom,
                       (double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);
}

void InpSpecWaterf::Update()
{
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    /* Get data from module */
    receiver->GetReceiveData()->GetInputSpec(vecrData, vecrScale);

#if QWT_VERSION < 0x050000
    /* Calculate sizes */
    const QSize CanvSize = plot->canvas()->size();
    int iLenScale = axisScaleDraw(QwtPlot::xBottom)->length();

    if ((iLenScale > 0) && (iLenScale < CanvSize.width()))
    {
        /* Calculate start and end of scale (needed for the borders) */
        iStartScale =
            (int) Floor(((_REAL) CanvSize.width() - iLenScale) / 2) - 1;

        iEndScale = iLenScale + iStartScale;
    }
    else
    {
        /* Something went wrong, use safe parameters */
        iStartScale = 0;
        iEndScale = CanvSize.width();
        iLenScale = CanvSize.width();
    }

    const QPixmap* pBPixmap = plot->canvas()->backgroundPixmap();

    QPixmap Canvas(CanvSize);
    /* In case the canvas width has changed or there is no bitmap, reset
       background */
    if ((pBPixmap == NULL) || (LastCanvasSize.width() != CanvSize.width()))
        Canvas.fill(backgroundColor());
    else
    {
        /* If height is larger, write background color in new space */
        if (LastCanvasSize.height() < CanvSize.height())
        {
            /* Prepare bitmap for copying background color */
            QPixmap CanvasTMP(CanvSize);
            CanvasTMP.fill(backgroundColor());

            /* Actual copy */
            bitBlt(&Canvas, 0, LastCanvasSize.height(), &CanvasTMP, 0, 0,
                   CanvSize.width(), CanvSize.height() - LastCanvasSize.height(),
                   Qt::CopyROP);
        }

        /* Move complete block one line further. Use old bitmap */
        bitBlt(&Canvas, 0, 1, pBPixmap, 0, 0,
               CanvSize.width(), CanvSize.height() - 1, Qt::CopyROP);
    }

    /* Store current canvas size */
    LastCanvasSize = CanvSize;

    /* Paint new line (top line) */
    QPainter Painter;
    Painter.begin(&Canvas);

    /* Left of the scale (left border) */
    for (i = 0; i < iStartScale; i++)
    {
        /* Generate pixel */
        Painter.setPen(backgroundColor());
        Painter.drawPoint(i, 0); /* line 0 -> top line */
    }

    /* Actual waterfall data */
    for (i = iStartScale; i < iEndScale; i++)
    {
        /* Init some constants */
        const int iMaxHue = 359; /* Range of "Hue" is 0-359 */
        const int iMaxSat = 255; /* Range of saturation is 0-255 */

        /* Stretch width to entire canvas width */
        const int iCurIdx =
            (int) Round((_REAL) (i - iStartScale) / iLenScale * vecrData.Size());

        /* Translate dB-values in colors */
        const int iCurCol =
            (int) Round((vecrData[iCurIdx] - MIN_VAL_INP_SPEC_Y_AXIS_DB) /
                        (MAX_VAL_INP_SPEC_Y_AXIS_DB - MIN_VAL_INP_SPEC_Y_AXIS_DB) *
                        iMaxHue);

        /* Reverse colors and add some offset (to make it look a bit nicer) */
        const int iColOffset = 60;
        int iFinalCol = iMaxHue - iColOffset - iCurCol;
        if (iFinalCol < 0) /* Prevent from out-of-range */
            iFinalCol = 0;

        /* Also change saturation to get dark colors when low level */
        const int iCurSat = (int) ((1 - (_REAL) iFinalCol / iMaxHue) * iMaxSat);

        /* Generate pixel */
        Painter.setPen(QColor(iFinalCol, iCurSat, iCurSat, QColor::Hsv));
        Painter.drawPoint(i, 0); /* line 0 -> top line */
    }

    /* Right of scale (right border) */
    for (i = iEndScale; i < CanvSize.width(); i++)
    {
        /* Generate pixel */
        Painter.setPen(backgroundColor());
        Painter.drawPoint(i, 0); /* line 0 -> top line */
    }

    Painter.end();

    /* Show the bitmap */
    plot->canvas()->setBackgroundPixmap(Canvas);
#endif
}

TranFct::TranFct(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void TranFct::Setup()
{
    /* Init chart for transfer function */
    plot->setTitle(tr("Channel Transfer Function / Group Delay"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));

    plot->setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
    plot->setAxisScale(QwtPlot::yRight, (double) -50.0, (double) 50.0);

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) -85.0, (double) -35.0);

    /* Add main curves */
    main1curve->setTitle(tr("Transf. Fct."));
    main1curve->attach(plot);
    main2curve->setTitle(tr("Group Del."));
    main2curve->setYAxis(QwtPlot::yRight);
    main2curve->attach(plot);
    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void TranFct::Update()
{
    /* Get data from module */
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    plotManager->GetTransferFunction(vecrData, vecrData2, vecrScale);

    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) vecrScale.Size());
    SetData(vecrData, vecrData2, vecrScale);
}

AudioSpec::AudioSpec(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void AudioSpec::Setup()
{
    /* Init chart for audio spectrum */
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(false);
    grid->enableYMin(false);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, true);
    plot->setAxisTitle(QwtPlot::yLeft, "AS [dB]");

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
    double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2400; /* 20.0 for 48 kHz */
    if (dBandwidth < 20.0)
        dBandwidth = (double) 20.0;

    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

    /* Add main curve */
    main1curve->setTitle(tr("Audio Spectrum"));
    main1curve->attach(plot);

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void AudioSpec::Update()
{
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    receiver->GetWriteData()->GetAudioSpec(vecrData, vecrScale);
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    string audiodecoder = Parameters.audiodecoder;
    Parameters.Unlock();
    if(audiodecoder=="")
    {
        plot->setTitle(tr("No audio decoding possible"));
    }
    else
    {
        plot->setTitle(tr("Audio Spectrum"));
    }
    SetData(vecrData, vecrScale);

}

FreqSamOffsHist::FreqSamOffsHist(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void FreqSamOffsHist::Setup()
{
    /* Init chart for transfer function. Enable right axis, too */
    plot->setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));

    /* Add main curves */
    main1curve->setTitle(tr("Freq."));
    main1curve->attach(plot);
    main2curve->setTitle(tr("Samp."));
    main2curve->setYAxis(QwtPlot::yRight);
    main2curve->attach(plot);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));

}

void FreqSamOffsHist::Update()
{
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    CVector<_REAL> vecrData2;
    _REAL rFreqAcquVal;
    plotManager->GetFreqSamOffsHist(vecrData, vecrData2, vecrScale, rFreqAcquVal);
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Freq. Offset [Hz] rel. to ") + QString().setNum(rFreqAcquVal) + " Hz");
    AutoScale(vecrData, vecrData2, vecrScale);
    SetData(vecrData, vecrData2, vecrScale);
}

void FreqSamOffsHist::AutoScale(CVector<_REAL>& vecrData,
                                CVector<_REAL>& vecrData2,
                                CVector<_REAL>& vecrScale)
{

    /* Customized auto-scaling. We adjust the y scale so that it is not larger
       than rMinScaleRange"  */
    const _REAL rMinScaleRange = (_REAL) 1.0; /* Hz */

    /* Get maximum and minimum values */
    _REAL MaxFreq = -_MAXREAL;
    _REAL MinFreq = _MAXREAL;
    _REAL MaxSam = -_MAXREAL;
    _REAL MinSam = _MAXREAL;

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
    plot->setAxisScale(QwtPlot::yLeft, (double) Floor(MinFreq / rMinScaleRange),
                       (double) Ceil(MaxFreq / rMinScaleRange));
    plot->setAxisScale(QwtPlot::yRight, (double) Floor(MinSam / rMinScaleRange),
                       (double) Ceil(MaxSam / rMinScaleRange));
    plot->setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

}


DopplerDelayHist::DopplerDelayHist(CDRMReceiver* pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void DopplerDelayHist::Setup()
{
    /* Init chart for transfer function. Enable right axis, too */
    plot->setTitle(tr("Delay / Doppler History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
    plot->setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

    /* Add main curves */
    main1curve->setTitle(tr("Delay"));
    main1curve->attach(plot);
    main2curve->setTitle(tr("Doppler"));
    main2curve->setYAxis(QwtPlot::yRight);
    main2curve->attach(plot);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

}

void DopplerDelayHist::Update()
{
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    plotManager->GetDopplerDelHist(vecrData, vecrData2, vecrScale);
    plot->setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);
    SetData(vecrData, vecrData2, vecrScale);
}

SNRAudHist::SNRAudHist(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void SNRAudHist::Setup()
{
    /* Init chart for transfer function. Enable right axis, too */
    plot->setTitle(tr("SNR / Correctly Decoded Audio History"));
    plot->enableAxis(QwtPlot::yRight);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
    plot->setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));

    /* Add main curves */
    main1curve->setTitle(tr("SNR"));
    main1curve->attach(plot);
    main2curve->setTitle(tr("Audio"));
    main2curve->setYAxis(QwtPlot::yRight);
    main2curve->attach(plot);

    /* Curve colors */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
    main2curve->setPen(QPen(SpecLine2ColorPlot, 1, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
}

void SNRAudHist::Update()
{
    CVector<_REAL> vecrData, vecrData2, vecrScale;
    plotManager->GetSNRHist(vecrData, vecrData2, vecrScale);
    AutoScale(vecrData, vecrData2, vecrScale);
    SetData(vecrData, vecrData2, vecrScale);
}

void SNRAudHist::AutoScale(CVector<_REAL>& vecrData,
                           CVector<_REAL>& vecrData2,
                           CVector<_REAL>& vecrScale)
{
    /* Customized auto-scaling. We adjust the y scale maximum so that it
       is not more than "rMaxDisToMax" to the curve */
    const int iMaxDisToMax = 5; /* dB */
    const int iMinValueSNRYScale = 15; /* dB */

    /* Get maximum value */
    _REAL MaxSNR = -_MAXREAL;

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
    plot->setAxisScale(QwtPlot::yLeft, (double) 0.0, dMaxYScaleSNR);
    plot->setAxisScale(QwtPlot::yRight, (double) 0.0, dMaxYScaleAudio);
    plot->setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

}

PSD::PSD(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void PSD::Setup()
{
    /* Init chart for power spectram density estimation */
    plot->setTitle(tr("Shifted Power Spectral Density of Input Signal"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("PSD [dB]"));

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::xBottom,
                       (double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

    plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_SHIF_PSD_Y_AXIS_DB,
                       MAX_VAL_SHIF_PSD_Y_AXIS_DB);

    /* Insert line for DC carrier */
    curve1->attach(plot);
    curve1->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));

    double dX[2], dY[2];
    dX[0] = dX[1] = (_REAL) VIRTUAL_INTERMED_FREQ / 1000;

    /* Take the min-max values from scale to get vertical line */
    dY[0] = MIN_VAL_SHIF_PSD_Y_AXIS_DB;
    dY[1] = MAX_VAL_SHIF_PSD_Y_AXIS_DB;

#if QWT_VERSION < 0x060000
#else
    curve1->setSamples(dX, dY, 2);
#endif
    /* Add main curve */
    main1curve->setTitle(tr("Shifted PSD"));
    main1curve->attach(plot);

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 1, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
}

void PSD::Update()
{
    CVector<_REAL> vecrData, vecrScale;
    receiver->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);
    SetData(vecrData, vecrScale);
}

SNRSpectrum::SNRSpectrum(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void SNRSpectrum::Setup()
{
    /* Init chart for power spectram density estimation */
    plot->setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));

    /* Add main curve */
    main1curve->setTitle(tr("SNR Spectrum"));
    main1curve->attach(plot);

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
}

void SNRSpectrum::Update()
{
    CVector<_REAL> vecrData, vecrScale;
    plotManager->GetSNRProfile(vecrData, vecrScale);
    AutoScale(vecrData, vecrScale);
    SetData(vecrData, vecrScale);
}

void SNRSpectrum::AutoScale(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
    const int iSize = vecrScale.Size();

    /* Fixed scale for x-axis */
    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) iSize);

    /* Fixed / variable scale (if SNR is in range, use fixed scale otherwise enlarge scale) */
    /* Get maximum value */
    _REAL rMaxSNR = -_MAXREAL;
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
    plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

}


InpSpec::InpSpec(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void InpSpec::Setup()
{
    /* Init chart for power spectram density estimation */
    plot->setTitle(tr("Input Spectrum"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Input Spectrum [dB]"));

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::xBottom,
                       (double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

    plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
                       MAX_VAL_INP_SPEC_Y_AXIS_DB);

    /* Insert line for DC carrier */
    curve1->attach(plot);
    curve1->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));

    /* Add main curve */
    main1curve->setTitle(tr("Input spectrum"));
    main1curve->attach(plot);

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 1, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
}


void InpSpec::Update()
{
    CVector<_REAL> vecrData, vecrScale;
    receiver->GetReceiveData()->GetInputSpec(vecrData, vecrScale);
    SetDCCarrier();
    SetData(vecrData, vecrScale);
}

InpPSD::InpPSD(CDRMReceiver *pDRMRec, QwtPlot* p):Chart(pDRMRec, p)
{
}

void InpPSD::Setup()
{
    /* Init chart for power spectram density estimation */
    plot->setTitle(tr("Input PSD"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(FALSE);
    grid->enableYMin(FALSE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Input PSD [dB]"));

    /* Fixed scale */
    const double dXScaleMax = (double) SOUNDCRD_SAMPLE_RATE / 2000;
    plot->setAxisScale(QwtPlot::xBottom, (double) 0.0, dXScaleMax);

    plot->setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
                       MAX_VAL_INP_SPEC_Y_AXIS_DB);

    /* Insert line for bandwidth marker */
    curve1->attach(plot);

    /* Make sure that line is bigger than the current plots height. Do this by
       setting the width to a very large value. TODO: better solution */
    curve1->setPen(QPen(PassBandColorPlot, 10000));

#if QWT_VERSION < 0x050000
    /* Since we want to have the "filter bandwidth" bar behind the grid, we have
       to draw our own grid after the previous curve was inserted. TODO: better
       solution */
    /* y-axis: get major ticks */
    const int iNumMajTicksYAx = axisScale(QwtPlot::yLeft)->majCnt();

    /* Make sure the grid does not end close to the border of the canvas.
       Introduce some "margin" */
    dX[0] = -dXScaleMax;
    dX[1] = dXScaleMax + dXScaleMax;

    /* Draw the grid for y-axis */
    for (i = 0; i < iNumMajTicksYAx; i++)
    {
        const long curvegrid = insertCurve(tr("My Grid"));
        curvegrid->setPen(QPen(MainGridColorPlot, 0, Qt::DotLine));

        dY[0] = dY[1] = axisScale(QwtPlot::yLeft)->majMark(i);
        curvegrid->setSamples(dX, dY, 2);
    }

    /* x-axis: get major ticks */
    const int iNumMajTicksXAx = axisScale(QwtPlot::xBottom)->majCnt();

    /* Make sure the grid does not end close to the border of the canvas.
       Introduce some "margin" */
    const double dDiffY =
        MAX_VAL_INP_SPEC_Y_AXIS_DB - MIN_VAL_INP_SPEC_Y_AXIS_DB;

    dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB - dDiffY;
    dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB + dDiffY;

    /* Draw the grid for x-axis */
    for (i = 0; i < iNumMajTicksXAx; i++)
    {
        const long curvegrid = insertCurve(tr("My Grid"));
        curvegrid->setPen(QPen(MainGridColorPlot, 0, Qt::DotLine));

        dX[0] = dX[1] = axisScale(QwtPlot::xBottom)->majMark(i);
        curvegrid->setSamples(dX, dY, 2);
    }

    /* Insert line for DC carrier */
    curve2->attach(plot);
    curve2->setPen(QPen(SpecLine1ColorPlot, 1, Qt::DotLine));
#endif
    /* Add main curve */
    main1curve->setTitle(tr("Input PSD"));
    main1curve->attach(plot);

    /* Curve color */
    main1curve->setPen(QPen(MainPenColorPlot, 2, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
}

void InpPSD::Update()
{
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    plotManager->GetInputPSD(vecrData, vecrScale);
    SetDCCarrier();
    SetBWMarker(0.0, 0.0); // clear the marker
    SetData(vecrData, vecrScale);
}

AnalogInpPSD::AnalogInpPSD(CDRMReceiver *pDRMRec, QwtPlot* p):InpPSD(pDRMRec, p)
{
}

void AnalogInpPSD::Update()
{
    CVector<_REAL> vecrData;
    CVector<_REAL> vecrScale;
    _REAL rCenterFreq, rBandwidth;
    receiver->GetReceiveData()->GetInputPSD(vecrData, vecrScale);
    receiver->GetAMDemod()->GetBWParameters(rCenterFreq, rBandwidth);
    SetDCCarrier();
    SetBWMarker(rCenterFreq, receiver->GetAMDemod()->GetCurMixFreqOffs());
    SetData(vecrData, vecrScale);
}

ConstellationChart::ConstellationChart(CDRMReceiver* pDRMRec, QwtPlot* p):Chart(pDRMRec, p),
    symbolMSC(), symbolSDC(), symbolFAC()
{
    /* initialise constellation symbols */
    symbolMSC.setStyle(QwtSymbol::Rect);
    symbolMSC.setSize(2);
    symbolMSC.setPen(QPen(Qt::blue));
    symbolMSC.setBrush(QBrush(Qt::blue));

    symbolSDC.setStyle(QwtSymbol::XCross);
    symbolSDC.setSize(4);
    symbolSDC.setPen(QPen(Qt::red));
    symbolSDC.setBrush(QBrush(Qt::red));

    symbolFAC.setStyle(QwtSymbol::Ellipse);
    symbolFAC.setSize(4);
    symbolFAC.setPen(QPen(Qt::black));
    symbolFAC.setBrush(QBrush(Qt::black));

}

void ConstellationChart::Setup()
{
}

void ConstellationChart::Update()
{
}

/* Get bounds of scale */
void ConstellationChart::getAxisScaleBounds(double& dXMax0, double& dXMax1, double& dYMax0, double& dYMax1)
{
    dXMax0 = plot->axisScaleDiv(QwtPlot::xBottom)->lowerBound();
    dXMax1 = plot->axisScaleDiv(QwtPlot::xBottom)->upperBound();
    dYMax0 = plot->axisScaleDiv(QwtPlot::yLeft)->lowerBound();
    dYMax1 = plot->axisScaleDiv(QwtPlot::yLeft)->upperBound();
}

void ConstellationChart::SetQAM4Grid()
{
#if QWT_VERSION < 0x050000
    long	curve;
    double	dXMax[2], dYMax[2];
    double	dX[2];

    /* Set scale style */
    QPen ScalePen(MainGridColorPlot, 1, Qt::DotLine);

    getAxisScaleBounds(this, dXMax[0], dXMax[1], dYMax[0], dYMax[1]);

    dX[0] = dX[1] = (double) 0.0;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);
#endif
}

void ConstellationChart::SetQAM16Grid()
{
#if QWT_VERSION < 0x050000
    long	curve;
    double	dXMax[2], dYMax[2];
    double	dX[2];

    /* Set scale style */
    QPen ScalePen(MainGridColorPlot, 1, Qt::DotLine);

    /* Get bounds of scale */
    getAxisScaleBounds(this, dXMax[0], dXMax[1], dYMax[0], dYMax[1]);

    dX[0] = dX[1] = (double) 0.0;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) 0.6333;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) -0.6333;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);
#endif
}

void ConstellationChart::SetQAM64Grid()
{
#if QWT_VERSION < 0x050000
    long	curve;
    double	dXMax[2], dYMax[2];
    double	dX[2];

    /* Set scale style */
    QPen ScalePen(MainGridColorPlot, 1, Qt::DotLine);

    /* Get bounds of scale */
    getAxisScaleBounds(this, dXMax[0], dXMax[1], dYMax[0], dYMax[1]);

    dX[0] = dX[1] = (double) 0.0;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) 0.3086;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) -0.3086;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) 0.6172;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) -0.6172;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) 0.9258;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);

    dX[0] = dX[1] = (double) -0.9258;
    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dX, dYMax, 2);

    curve = insertCurve("line");
    curve->setPen(ScalePen);
    curve->setSamples(dXMax, dX, 2);
#endif
}

FACConst::FACConst(CDRMReceiver* pDRMRec, QwtPlot* p):ConstellationChart(pDRMRec, p)
{
}

void FACConst::Setup()
{
    /* Init chart for FAC constellation */
    plot->setTitle(tr("FAC Constellation"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(FALSE);
    grid->enableYMin(FALSE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

    /* Fixed scale (2 / sqrt(2)) */
    plot->setAxisScale(QwtPlot::xBottom, (double) -1.4142, (double) 1.4142);
    plot->setAxisScale(QwtPlot::yLeft, (double) -1.4142, (double) 1.4142);

    /* Set marker symbol */
    symbolMSC.setStyle(QwtSymbol::Ellipse);
    symbolMSC.setSize(4);
    symbolMSC.setPen(QPen(MainPenColorConst));
    symbolMSC.setBrush(QBrush(MainPenColorConst));

    /* Insert grid */
    SetQAM4Grid();
}

void FACConst::Update()
{
    CVector<_COMPLEX> veccData;
    receiver->GetFACMLC()->GetVectorSpace(veccData);
    SetData(main1curve, veccData, symbolFAC);
}

SDCConst::SDCConst(CDRMReceiver* pDRMRec, QwtPlot* p):ConstellationChart(pDRMRec, p)
{
}

void SDCConst::Setup()
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    ECodScheme eSDCCodingScheme = Parameters.eSDCCodingScheme;
    Parameters.Unlock();

    /* Init chart for SDC constellation */
    plot->setTitle(tr("SDC Constellation"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(FALSE);
    grid->enableYMin(FALSE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

    /* Fixed scale (4 / sqrt(10)) */
    plot->setAxisScale(QwtPlot::xBottom, (double) -1.2649, (double) 1.2649);
    plot->setAxisScale(QwtPlot::yLeft, (double) -1.2649, (double) 1.2649);

    /* Insert grid */
    if(eSDCCodingScheme == CS_1_SM)
        SetQAM4Grid();
    else
        SetQAM16Grid();
}

void SDCConst::Update()
{
    CVector<_COMPLEX> veccData;
    /* Get data vector */
    receiver->GetSDCMLC()->GetVectorSpace(veccData);

    SetData(main1curve, veccData, symbolSDC);
}

MSCConst::MSCConst(CDRMReceiver* pDRMRec, QwtPlot* p):ConstellationChart(pDRMRec, p)
{
}

void MSCConst::Setup()
{
    CParameter& Parameters = *receiver->GetParameters();
    Parameters.Lock();
    ECodScheme eMSCCodingScheme = Parameters.eMSCCodingScheme;
    Parameters.Unlock();

    /* Init chart for MSC constellation */
    plot->setTitle(tr("MSC Constellation"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(FALSE);
    grid->enableYMin(FALSE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

    /* Fixed scale (8 / sqrt(42)) */
    plot->setAxisScale(QwtPlot::xBottom, (double) -1.2344, (double) 1.2344);
    plot->setAxisScale(QwtPlot::yLeft, (double) -1.2344, (double) 1.2344);

    /* Insert grid */
    if (eMSCCodingScheme == CS_2_SM)
        SetQAM16Grid();
    else
        SetQAM64Grid();
}

void MSCConst::Update()
{
    CVector<_COMPLEX> veccData;
    receiver->GetMSCMLC()->GetVectorSpace(veccData);
    SetData(main1curve, veccData, symbolMSC);
}

AllConst::AllConst(CDRMReceiver* pDRMRec, QwtPlot* p):ConstellationChart(pDRMRec, p)
{
}

void AllConst::Setup()
{
    /* Init chart for constellation */
    plot->setTitle(tr("MSC / SDC / FAC Constellation"));
    plot->enableAxis(QwtPlot::yRight, FALSE);
    grid->enableXMin(TRUE);
    grid->enableYMin(TRUE);
    plot->setAxisTitle(QwtPlot::xBottom, tr("Real"));
    plot->enableAxis(QwtPlot::yLeft, TRUE);
    plot->setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

    /* Fixed scale */
    plot->setAxisScale(QwtPlot::xBottom, (double) -1.5, (double) 1.5);
    plot->setAxisScale(QwtPlot::yLeft, (double) -1.5, (double) 1.5);

    /* Insert "dummy" curves for legend */
    // TODO plot->enableLegend(TRUE, curve1);
}

void AllConst::Update()
{
    CVector<_COMPLEX> veccData;
    receiver->GetMSCMLC()->GetVectorSpace(veccData);
    SetData(main1curve, veccData, symbolMSC);
    receiver->GetSDCMLC()->GetVectorSpace(veccData);
    SetData(main2curve, veccData, symbolSDC);
    receiver->GetFACMLC()->GetVectorSpace(veccData);
    SetData(curve1, veccData, symbolFAC);
}

/* Implementation *************************************************************/
CDRMPlot::CDRMPlot(QwtPlot* pplot) :
    plot(pplot), CurCharType(NONE_OLD), InitCharType(NONE_OLD),
    leftTitle(), rightTitle(), bottomTitle(),
    bOnTimerCharMutexFlag(FALSE),
    pDRMRec(NULL),chart(NULL)
{
    if(plot==NULL)
        plot = new QwtPlot(NULL);

    chart = new InpPSD(pDRMRec, plot);
    /* Legend should be on the right side */
    //setLegendPos(QwtPlot::Right);

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
    plot->setStyleSheet("QwtPlot { padding: 10px }");

    /* Canvas */
    plot->setCanvasLineWidth(0);

    /* Set default style */
    //chart->SetPlotStyle(0);

    /* Connections */
    connect(plot, SIGNAL(MouseReleased(const QMouseEvent&)),
            this, SLOT(OnClicked(const QMouseEvent&)));
}

void CDRMPlot::OnTimerChart()
{
    /* In some cases, if the user moves the mouse very fast over the chart
       selection list view, this function is called by two different threads.
       Somehow, using QMutex does not help. Therefore we introduce a flag for
       doing this job. This solution is a work-around. TODO: better solution */
    if (bOnTimerCharMutexFlag == TRUE)
        return;

    bOnTimerCharMutexFlag = TRUE;

    SetupChart(CurCharType);
    chart->Update();
    plot->replot();

    /* "Unlock" mutex flag */
    bOnTimerCharMutexFlag = FALSE;
}

void CDRMPlot::SetupChart(ECharType InitCharType)
{

    /* First check if plot must be set up */

    if (InitCharType != CurCharType)
    {
        CurCharType = InitCharType;
        delete chart; // ot maybe hide, etc
	plot->clear();
        switch(CurCharType) {
        case INPUT_SIG_PSD:
            chart = new InpPSD(pDRMRec, plot);
            break;
        case TRANSFERFUNCTION:
            chart = new TranFct(pDRMRec, plot);
            break;
        case FAC_CONSTELLATION:
            chart = new FACConst(pDRMRec, plot);
            break;
        case SDC_CONSTELLATION:
            chart = new SDCConst(pDRMRec, plot);
            break;
        case MSC_CONSTELLATION:
            chart = new MSCConst(pDRMRec, plot);
            break;
        case POWER_SPEC_DENSITY:
            chart = new PSD(pDRMRec, plot);
            break;
        case INPUTSPECTRUM_NO_AV:
            chart = new InpSpec(pDRMRec, plot);
            break;
        case AUDIO_SPECTRUM:
            chart = new AudioSpec(pDRMRec, plot);
            break;
        case FREQ_SAM_OFFS_HIST:
            chart = new FreqSamOffsHist(pDRMRec, plot);
            break;
        case DOPPLER_DELAY_HIST:
            chart = new DopplerDelayHist(pDRMRec, plot);
            break;
        case ALL_CONSTELLATION:
            chart = new AllConst(pDRMRec, plot);
            break;
        case SNR_AUDIO_HIST:
            chart = new SNRAudHist(pDRMRec, plot);
            break;
        case AVERAGED_IR:
            chart = new AvIR(pDRMRec, plot);
            break;
        case SNR_SPECTRUM:
            chart = new SNRSpectrum(pDRMRec, plot);
            break;
        case INPUT_SIG_PSD_ANALOG:
            chart = new AnalogInpPSD(pDRMRec, plot);
            break;
        case INP_SPEC_WATERF:
            chart = new InpSpecWaterf(pDRMRec, plot);
            break;
        default:
            chart = new InpPSD(pDRMRec, plot);
        }
	chart->Setup();
    }

}

void CDRMPlot::OnClicked(const QMouseEvent& e)
{
    /* Get frequency from current cursor position */
    const double dFreq = plot->invTransform(QwtPlot::xBottom, e.x());

    /* Send normalized frequency to receiver */
    const double dMaxxBottom = plot->axisScaleDiv(QwtPlot::xBottom)->upperBound();

    /* Check if value is valid */
    if (dMaxxBottom != (double) 0.0)
    {
        /* Emit signal containing normalized selected frequency */
        emit xAxisValSet(dFreq / dMaxxBottom);
    }
}

void CDRMPlot::SetPlotStyle(const int iNewStyleID)
{
	chart->SetPlotStyle(iNewStyleID);
}

void CDRMPlot::AddWhatsThisHelpChar(const ECharType NCharType)
{
    QString strCurPlotHelp;

    switch (NCharType)
    {
    case AVERAGED_IR:
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

    case TRANSFERFUNCTION:
        /* Transfer Function */
        strCurPlotHelp =
            tr("<b>Transfer Function / Group Delay:</b> "
               "This plot shows the squared magnitude and the group delay of "
               "the estimated channel at each sub-carrier.");
        break;

    case FAC_CONSTELLATION:
    case SDC_CONSTELLATION:
    case MSC_CONSTELLATION:
    case ALL_CONSTELLATION:
        /* Constellations */
        strCurPlotHelp =
            tr("<b>FAC, SDC, MSC:</b> The plots show the "
               "constellations of the FAC, SDC and MSC logical channel of the DRM "
               "stream. Depending on the current transmitter settings, the SDC "
               "and MSC can have 4-QAM, 16-QAM or 64-QAM modulation.");
        break;

    case POWER_SPEC_DENSITY:
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
               "since the side loops have in this case only energy between the "
               "samples in the frequency domain. On the sample positions outside "
               "the actual DRM spectrum, the DRM signal has zero crossings "
               "because of the orthogonality. Therefore this spectrum represents "
               "NOT the actual spectrum but the \"idealized\" OFDM spectrum.");
        break;

    case SNR_SPECTRUM:
        /* SNR Spectrum (Weighted MER on MSC Cells) */
        strCurPlotHelp =
            tr("<b>SNR Spectrum (Weighted MER on MSC Cells):</b> "
               "This plot shows the Weighted MER on MSC cells for each carrier "
               "separately.");
        break;

    case INPUTSPECTRUM_NO_AV:
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

    case INPUT_SIG_PSD:
    case INPUT_SIG_PSD_ANALOG:
        /* Input PSD */
        strCurPlotHelp =
            tr("<b>Input PSD:</b> This plot shows the "
               "estimated power spectral density (PSD) of the input signal. The "
               "PSD is estimated by averaging some Hamming Window weighted "
               "Fourier transformed blocks of the input signal samples. The "
               "dashed vertical line shows the estimated DC frequency.");
        break;

    case AUDIO_SPECTRUM:
        /* Audio Spectrum */
        strCurPlotHelp =
            tr("<b>Audio Spectrum:</b> This plot shows the "
               "averaged audio spectrum of the currently played audio. With this "
               "plot the actual audio bandwidth can easily determined. Since a "
               "linear scale is used for the frequency axis, most of the energy "
               "of the signal is usually concentrated on the far left side of the "
               "spectrum.");
        break;

    case FREQ_SAM_OFFS_HIST:
        /* Frequency Offset / Sample Rate Offset History */
        strCurPlotHelp =
            tr("<b>Frequency Offset / Sample Rate Offset History:"
               "</b> The history "
               "of the values for frequency offset and sample rate offset "
               "estimation is shown. If the frequency offset drift is very small, "
               "this is an indication that the analog front end is of high "
               "quality.");
        break;

    case DOPPLER_DELAY_HIST:
        /* Doppler / Delay History */
        strCurPlotHelp =
            tr("<b>Doppler / Delay History:</b> "
               "The history of the values for the "
               "Doppler and Impulse response length is shown. Large Doppler "
               "values might be responsable for audio drop-outs.");
        break;

    case SNR_AUDIO_HIST:
        /* SNR History */
        strCurPlotHelp =
            tr("<b>SNR History:</b> "
               "The history of the values for the "
               "SNR and correctly decoded audio blocks is shown. The maximum "
               "achievable number of correctly decoded audio blocks per DRM "
               "frame is 10 or 5 depending on the audio sample rate (24 kHz "
               "or 12 kHz AAC core bandwidth).");
        break;

    case INP_SPEC_WATERF:
        /* Waterfall Display of Input Spectrum */
        strCurPlotHelp =
            tr("<b>Waterfall Display of Input Spectrum:</b> "
               "The input spectrum is displayed as a waterfall type. The "
               "different colors represent different levels.");
        break;

    case NONE_OLD:
        break;
    }

    /* Main plot */
    plot->setWhatsThis(strCurPlotHelp);
}
