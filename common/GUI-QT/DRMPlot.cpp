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

#include "DRMPlot.h"


/* Implementation *************************************************************/
CDRMPlot::CDRMPlot(QWidget *p, const char *name) :  
	QwtPlot (p, name)
{
	/* Grid defaults */
	enableGridX(TRUE);
	enableGridY(TRUE);

	enableGridXMin(FALSE);
	enableGridYMin(FALSE);

	/* Fonts */
	setTitleFont(QFont("SansSerif", 8, QFont::Bold));
	setAxisFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yRight, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::yRight, QFont("SansSerif", 8));

	/* Global frame */
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setLineWidth(2);
	setMargin(10);

	/* Canvas */
	setCanvasLineWidth(0);

	/* Set default style */
	SetPlotStyle(0);

	/* Connections */
	connect(this, SIGNAL(plotMouseReleased(const QMouseEvent&)),
		this, SLOT(OnClicked(const QMouseEvent&)));
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
		break;

	case 2:
		MainPenColorPlot = BLACKGREY_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLACKGREY_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLACKGREY_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLACKGREY_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLACKGREY_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLACKGREY_SPEC_LINE2_COLOR_PLOT;
		break;

	case 0: /* 0 is default */
	default:
		MainPenColorPlot = BLUEWHITE_MAIN_PEN_COLOR_PLOT;
		MainPenColorConst = BLUEWHITE_MAIN_PEN_COLOR_CONSTELLATION;
		BckgrdColorPlot = BLUEWHITE_BCKGRD_COLOR_PLOT;
		MainGridColorPlot = BLUEWHITE_MAIN_GRID_COLOR_PLOT;
		SpecLine1ColorPlot = BLUEWHITE_SPEC_LINE1_COLOR_PLOT;
		SpecLine2ColorPlot = BLUEWHITE_SPEC_LINE2_COLOR_PLOT;
		break;
	}

	/* Apply colors */
	setGridMajPen(QPen(MainGridColorPlot, 0, DotLine));
	setGridMinPen(QPen(MainGridColorPlot, 0, DotLine));
	setCanvasBackground(QColor(BckgrdColorPlot));

	/* Refresh the plot */
	replot();
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					   const int size)
{
	long curve1;

	double* pdData = new double[vecrData.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Add curve */
	curve1 = insertCurve("Graph 1");
	
	/* Curve color */
	setCurvePen(curve1, QPen(MainPenColorPlot, size, SolidLine, RoundCap,
		RoundJoin));

	/* Copy data from vectors in temporary arrays */
	for (int i = 0; i < vecrScale.Size(); i++)
	{
		pdData[i] = vecrData[i];
		pdScale[i] = vecrScale[i];
	}

	setCurveData(curve1, pdScale, pdData, vecrData.Size());

	delete[] pdData;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_REAL>& vecrData1, CVector<_REAL>& vecrData2,
					   CVector<_REAL>& vecrScale, const int size,
					   const int size2)
{
	long curve1, curve2;

	double* pdData1 = new double[vecrData1.Size()];
	double* pdData2 = new double[vecrData2.Size()];
	double* pdScale = new double[vecrScale.Size()];

	/* Add curves */
	curve1 = insertCurve("Graph 1");
	curve2 = insertCurve("Graph 2", QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(curve1, QPen(MainPenColorPlot, size, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(curve2, QPen(SpecLine2ColorPlot, size2, SolidLine, RoundCap,
		RoundJoin));

	/* Copy data from vectors in temporary arrays */
	for (int i = 0; i < vecrScale.Size(); i++)
	{
		pdData1[i] = vecrData1[i];
		pdData2[i] = vecrData2[i];
		pdScale[i] = vecrScale[i];
	}

	setCurveData(curve1, pdScale, pdData1, vecrData1.Size());
	setCurveData(curve2, pdScale, pdData2, vecrData2.Size());

	delete[] pdData1;
	delete[] pdData2;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_COMPLEX>& veccData, QColor color,
					   const int size)
{
	long		lMarkerKey;
	QwtSymbol	MarkerSym;

	/* Set marker symbol */
	MarkerSym.setStyle(QwtSymbol::Ellipse);
	MarkerSym.setSize(size);
	MarkerSym.setPen(QPen(color));
	MarkerSym.setBrush(QBrush(color));

	/* Copy data from vectors in temporary arrays */
	for (int i = 0; i < veccData.Size(); i++)
	{
		lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym);
		setMarkerPos(lMarkerKey, veccData[i].real(), veccData[i].imag());
	}
}

void CDRMPlot::SetAvIR(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					   _REAL rLowerB, _REAL rHigherB,
					   const _REAL rStartGuard, const _REAL rEndGuard,
					   const _REAL rBeginIR, const _REAL rEndIR)
{
	long	curveLow, curveHigh, curveLeft, curveRight;
	long	curveBeginIR, curveEndIR;
	double	dX[2], dY[2];

	if (vecrScale.Size() != 0)
	{
		/* Init chart for averaged impulse response */
		setTitle("Estimated Channel Impulse Response");
		enableAxis(QwtPlot::yRight, FALSE);
		enableGridX(TRUE);
		enableGridY(TRUE);
		setAxisTitle(QwtPlot::xBottom, "Time [ms]");
		setAxisTitle(QwtPlot::yLeft, "IR [dB]");

		/* Fixed scale */
		const double cdAxMinLeft = (double) -20.0;
		const double cdAxMaxLeft = (double) 40.0;
		setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);

		setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], 
			(double) vecrScale[vecrScale.Size() - 1]);

		clear();


		/* Vertical bounds -------------------------------------------------- */
		/* These bounds show the beginning and end of the guard-interval */
		curveLeft = insertCurve("Guard-interval beginning");
		curveRight = insertCurve("Guard-interval end");
		curveBeginIR = insertCurve("Estimated begin of impulse response");
		curveEndIR = insertCurve("Estimated end of impulse response");
		setCurvePen(curveLeft, QPen(SpecLine1ColorPlot, 1, DotLine));
		setCurvePen(curveRight, QPen(SpecLine1ColorPlot, 1, DotLine));
		setCurvePen(curveBeginIR, QPen(SpecLine2ColorPlot, 1, DotLine));
		setCurvePen(curveEndIR, QPen(SpecLine2ColorPlot, 1, DotLine));

		dY[0] = cdAxMinLeft;
		dY[1] = cdAxMaxLeft;

		/* Left bound */
		dX[0] = dX[1] = rStartGuard;
		setCurveData(curveLeft, dX, dY, 2);

		/* Right bound */
		dX[0] = dX[1] = rEndGuard;
		setCurveData(curveRight, dX, dY, 2);

		/* Estimated begin of impulse response */
		dX[0] = dX[1] = rBeginIR;
		setCurveData(curveBeginIR, dX, dY, 2);

		/* Estimated end of impulse response */
		dX[0] = dX[1] = rEndIR;
		setCurveData(curveEndIR, dX, dY, 2);


		/* Data for the actual impulse response curve */
		SetData(vecrData, vecrScale, 2);


		/* Horizontal bounds ------------------------------------------------ */
		/* These bounds show the peak detection bound from timing tracking */
		curveHigh = insertCurve("Higher Bound");
		dX[0] = vecrScale[0];
		dX[1] = vecrScale[vecrScale.Size() - 1];

#ifdef _DEBUG_
		/* Insert lines for lower and higher bound */
		curveLow = insertCurve("Lower Bound");
		setCurvePen(curveLow, QPen(SpecLine1ColorPlot));
		setCurvePen(curveHigh, QPen(SpecLine2ColorPlot));

		/* Lower bound */
		dY[0] = dY[1] = rLowerB;
		setCurveData(curveLow, dX, dY, 2);

		/* Higher bound */
		dY[0] = dY[1] = rHigherB;
#else
		/* Only include highest bound */
		setCurvePen(curveHigh, QPen(SpecLine1ColorPlot, 1, DotLine));
		dY[0] = dY[1] = Max(rHigherB, rLowerB);
#endif
		setCurveData(curveHigh, dX, dY, 2);

		replot();
	}
	else
	{
		/* No input data, just clear plot */
		clear();
		replot();
	}
}

void CDRMPlot::SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
						  CVector<_REAL>& vecrScale)
{
	/* Init chart for transfer function */
	setTitle("Estimated Channel Transfer Function");
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, "Carrier Index");
	setAxisTitle(QwtPlot::yLeft, "TF [dB]");

	setAxisTitle(QwtPlot::yRight, "Group Delay [ms]");
	setAxisScale(QwtPlot::yRight, (double) -50.0, (double) 50.0);

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -50.0, (double) 0.0);
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) vecrScale.Size());

	clear();
	SetData(vecrData, vecrData2, vecrScale, 2, 1);
	replot();
}

void CDRMPlot::SetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* Init chart for audio spectrum */
	setTitle("Audio Spectrum");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, "Frequency [kHz]");
	setAxisTitle(QwtPlot::yLeft, "AS [dB]");

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
	double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2000;
	if (SOUNDCRD_SAMPLE_RATE == 48000)
		dBandwidth = (double) 20.0; /* Special value in case of 48 kHz */

	setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

	clear();
	SetData(vecrData, vecrScale, 2);
	replot();
}

void CDRMPlot::SetPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	long	lCurveDC;
	double	dX[2], dY[2];

	/* Init chart for power spectram density estimation */
	setTitle("Shifted Power Spectral Density of Input Signal");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, "Frequency [kHz]");
	setAxisTitle(QwtPlot::yLeft, "PSD [dB]");

	/* Fixed scale */
	const double cdAxMinLeft = -50.0;
	const double cdAxMaxLeft = 0.0;
	setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	clear();

	/* Insert line for DC carrier */
	lCurveDC = insertCurve("DC carrier");
	setCurvePen(lCurveDC, QPen(SpecLine1ColorPlot, 1, DotLine));

	dX[0] = dX[1] = (_REAL) VIRTUAL_INTERMED_FREQ / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = cdAxMinLeft;
	dY[1] = cdAxMaxLeft;

	setCurveData(lCurveDC, dX, dY, 2);

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetInpSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
						  const _REAL rDCFreq, const _REAL rBWCenter,
						  const _REAL rBWWidth)
{
	double dX[2], dY[2];

	/* Init chart for power spectram density estimation */
	setTitle("Input Spectrum");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, "Frequency [kHz]");
	setAxisTitle(QwtPlot::yLeft, "Input Spectrum [dB]");

	/* Fixed scale */
	const double cdAxMinLeft = -100.0;
	const double cdAxMaxLeft = 0.0;
	setAxisScale(QwtPlot::yLeft, cdAxMinLeft, cdAxMaxLeft);
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	clear();

	/* Insert line for DC carrier */
	long lCurveDC = insertCurve("DC carrier");
	setCurvePen(lCurveDC, QPen(SpecLine1ColorPlot, 1, DotLine));

	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = cdAxMinLeft;
	dY[1] = cdAxMaxLeft;

	setCurveData(lCurveDC, dX, dY, 2);

	/* Insert marker for filter bandwidth if required */
	if (rBWWidth != (_REAL) 0.0)
	{
		/* Insert line for bandwidth marker */
		long lCurveBW = insertCurve("BW");
		setCurvePen(lCurveBW, QPen(SpecLine1ColorPlot, 6));

		dX[0] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
		dX[1] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

		/* Take the min-max values from scale to get vertical line */
		dY[0] = cdAxMinLeft;
		dY[1] = cdAxMinLeft;

		setCurveData(lCurveBW, dX, dY, 2);
	}


	/* Insert actual spectrum data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetFACConst(CVector<_COMPLEX>& veccData)
{
	/* Init chart for FAC constellation */
	setTitle("FAC Constellation");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, "Real");
	setAxisTitle(QwtPlot::yLeft, "Imaginary");

	/* Fixed scale (2 / sqrt(2)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.4142, (double) 1.4142);
	setAxisScale(QwtPlot::yLeft, (double) -1.4142, (double) 1.4142);

	clear();
	SetQAM4Grid();
	SetData(veccData, MainPenColorConst, 4);
	replot();
}

void CDRMPlot::SetSDCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for SDC constellation */
	setTitle("SDC Constellation");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, "Real");
	setAxisTitle(QwtPlot::yLeft, "Imaginary");

	/* Fixed scale (4 / sqrt(10)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2649, (double) 1.2649);
	setAxisScale(QwtPlot::yLeft, (double) -1.2649, (double) 1.2649);

	clear();

	if (eNewCoSc == CParameter::CS_1_SM)
		SetQAM4Grid();
	else
		SetQAM16Grid();

	SetData(veccData, MainPenColorConst, 4);
	replot();
}

void CDRMPlot::SetMSCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for MSC constellation */
	setTitle("MSC Constellation");
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, "Real");
	setAxisTitle(QwtPlot::yLeft, "Imaginary");

	/* Fixed scale (8 / sqrt(42)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2344, (double) 1.2344);
	setAxisScale(QwtPlot::yLeft, (double) -1.2344, (double) 1.2344);

	clear();

	if (eNewCoSc == CParameter::CS_2_SM)
		SetQAM16Grid();
	else
		SetQAM64Grid();

	SetData(veccData, MainPenColorConst, 2);
	replot();
}

void CDRMPlot::SetQAM4Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::SetQAM16Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.6333;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.6333;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::SetQAM64Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MainGridColorPlot, 1, DotLine);

	/* Get bounds of scale */
	dXMax[0] = axisScale(QwtPlot::xBottom)->lBound();
	dXMax[1] = axisScale(QwtPlot::xBottom)->hBound();
	dYMax[0] = axisScale(QwtPlot::yLeft)->lBound();
	dYMax[1] = axisScale(QwtPlot::yLeft)->hBound();

	dX[0] = dX[1] = (double) 0.0;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.3086;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.3086;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.6172;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.6172;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) 0.9258;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);

	dX[0] = dX[1] = (double) -0.9258;
	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dX, dYMax, 2);

	curve = insertCurve("line");
	setCurvePen(curve, ScalePen);
	setCurveData(curve, dXMax, dX, 2);
}

void CDRMPlot::OnClicked(const QMouseEvent& e)
{
	/* Get frequency from current cursor position */
	const double dFreq = invTransform(QwtPlot::xBottom, e.x());

	/* Send normalized frequency to receiver */
	const double dMaxxBottom = axisScale(QwtPlot::xBottom)->hBound();

	if (dMaxxBottom != (double) 0.0)
		DRMReceiver.SetAMDemodAcq(dFreq / dMaxxBottom);
}
