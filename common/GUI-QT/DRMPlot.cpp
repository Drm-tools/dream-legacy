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
	setGridMajPen(QPen(MAIN_GRID_COLOR_PLOT, 0, DotLine));

	enableGridXMin(FALSE);
	enableGridYMin(FALSE);
	setGridMinPen(QPen(MAIN_GRID_COLOR_PLOT, 0, DotLine));

	/* Fonts */
	setTitleFont(QFont("SansSerif", 8, QFont::Bold));
	setAxisFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisFont(QwtPlot::yLeft, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::xBottom, QFont("SansSerif", 8));
	setAxisTitleFont(QwtPlot::yLeft, QFont("SansSerif", 8));

	/* Global frame */
	setFrameStyle(QFrame::Panel|QFrame::Sunken);
	setLineWidth(2);
	setMargin(10);

	/* Canvas */
	setCanvasLineWidth(0);
	setCanvasBackground(QColor(BCKGRD_COLOR_PLOT)); 
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
	setCurvePen(curve1, QPen(MAIN_PEN_COLOR_PLOT, size, SolidLine, RoundCap,
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
		setCurvePen(curveLeft, QPen(SPEC_LINE1_COLOR_PLOT, 1, DotLine));
		setCurvePen(curveRight, QPen(SPEC_LINE1_COLOR_PLOT, 1, DotLine));
		setCurvePen(curveBeginIR, QPen(SPEC_LINE2_COLOR_PLOT, 1, DotLine));
		setCurvePen(curveEndIR, QPen(SPEC_LINE2_COLOR_PLOT, 1, DotLine));

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
		setCurvePen(curveLow, QPen(SPEC_LINE1_COLOR_PLOT));
		setCurvePen(curveHigh, QPen(SPEC_LINE2_COLOR_PLOT));

		/* Lower bound */
		dY[0] = dY[1] = rLowerB;
		setCurveData(curveLow, dX, dY, 2);

		/* Higher bound */
		dY[0] = dY[1] = rHigherB;
#else
		/* Only include highest bound */
		setCurvePen(curveHigh, QPen(SPEC_LINE1_COLOR_PLOT, 1, DotLine));
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

void CDRMPlot::SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* Init chart for transfer function */
	setTitle("Estimated Channel Transfer Function");
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, "Carrier Index");
	setAxisTitle(QwtPlot::yLeft, "TF [dB]");

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -50.0, (double) 0.0);
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) vecrScale.Size());

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
	setCurvePen(lCurveDC, QPen(SPEC_LINE1_COLOR_PLOT, 1, DotLine));

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
						  const _REAL rDCFreq)
{
	long	lCurveDC;
	double	dX[2], dY[2];

	/* Init chart for power spectram density estimation */
	setTitle("Input Spectrum");
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
	lCurveDC = insertCurve("DC carrier");
	setCurvePen(lCurveDC, QPen(SPEC_LINE1_COLOR_PLOT, 1, DotLine));

	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = cdAxMinLeft;
	dY[1] = cdAxMaxLeft;

	setCurveData(lCurveDC, dX, dY, 2);

	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetFACConst(CVector<_COMPLEX>& veccData)
{
	/* Init chart for FAC constellation */
	setTitle("FAC Constellation");
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, "Real");
	setAxisTitle(QwtPlot::yLeft, "Imaginary");

	/* Fixed scale (2 / sqrt(2)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.4142, (double) 1.4142);
	setAxisScale(QwtPlot::yLeft, (double) -1.4142, (double) 1.4142);

	clear();
	SetQAM4Grid();
	SetData(veccData, MAIN_PEN_COLOR_CONSTELLATION, 4);
	replot();
}

void CDRMPlot::SetSDCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for SDC constellation */
	setTitle("SDC Constellation");
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

	SetData(veccData, MAIN_PEN_COLOR_CONSTELLATION, 4);
	replot();
}

void CDRMPlot::SetMSCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for MSC constellation */
	setTitle("MSC Constellation");
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

	SetData(veccData, MAIN_PEN_COLOR_CONSTELLATION, 2);
	replot();
}

void CDRMPlot::SetQAM4Grid()
{
	long	curve;
	double	dXMax[2], dYMax[2];
	double	dX[2];

	/* Set scale style */
	QPen ScalePen(MAIN_GRID_COLOR_PLOT, 1, DotLine);

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
	QPen ScalePen(MAIN_GRID_COLOR_PLOT, 1, DotLine);

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
	QPen ScalePen(MAIN_GRID_COLOR_PLOT, 1, DotLine);

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
