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
	QwtPlot (p, name), CurCharType(DISABLE_PLOT)
{
	/* Grid defaults */
	enableGridX(TRUE);
	enableGridY(TRUE);

	enableGridXMin(FALSE);
	enableGridYMin(FALSE);

	/* Legend should be on the right side */
	setLegendPos(QwtPlot::Right);

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
	setGridMajPen(QPen(MainGridColorPlot, 0, DotLine));
	setGridMinPen(QPen(MainGridColorPlot, 0, DotLine));
	setCanvasBackground(QColor(BckgrdColorPlot));

	/* Refresh the plot */
	replot();
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

	setCurveData(main1curve, pdScale, pdData, vecrData.Size());

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

	setCurveData(main1curve, pdScale, pdData1, vecrData1.Size());
	setCurveData(main2curve, pdScale, pdData2, vecrData2.Size());

	delete[] pdData1;
	delete[] pdData2;
	delete[] pdScale;
}

void CDRMPlot::SetData(CVector<_COMPLEX>& veccData)
{
	/* Copy data from vectors in temporary arrays */
	const int iDataSize = veccData.Size();
	for (int i = 0; i < iDataSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym1);
		setMarkerPos(lMarkerKey, veccData[i].real(), veccData[i].imag());
	}
}

void CDRMPlot::SetData(CVector<_COMPLEX>& veccMSCConst,
					   CVector<_COMPLEX>& veccSDCConst,
					   CVector<_COMPLEX>& veccFACConst)
{
	int i;

	/* Copy data from vectors in temporary arrays */
	const int iMSCSize = veccMSCConst.Size();
	for (i = 0; i < iMSCSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym1);
		setMarkerPos(lMarkerKey,
			veccMSCConst[i].real(), veccMSCConst[i].imag());
	}

	const int iSDCSize = veccSDCConst.Size();
	for (i = 0; i < iSDCSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym2);
		setMarkerPos(lMarkerKey,
			veccSDCConst[i].real(), veccSDCConst[i].imag());
	}

	const int iFACSize = veccFACConst.Size();
	for (i = 0; i < iFACSize; i++)
	{
		const long lMarkerKey = insertMarker();
		setMarkerSymbol(lMarkerKey, MarkerSym3);
		setMarkerPos(lMarkerKey,
			veccFACConst[i].real(), veccFACConst[i].imag());
	}
}

void CDRMPlot::SetupAvIR()
{
	/* Init chart for averaged impulse response */
	setTitle(tr("Channel Impulse Response"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [ms]"));
	setAxisTitle(QwtPlot::yLeft, tr("IR [dB]"));

	/* Insert curves */
	clear();
	curve1 = insertCurve(tr("Guard-interval beginning"));
	curve2 = insertCurve(tr("Guard-interval end"));
	curve3 = insertCurve(tr("Estimated begin of impulse response"));
	curve4 = insertCurve(tr("Estimated end of impulse response"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));
	setCurvePen(curve2, QPen(SpecLine1ColorPlot, 1, DotLine));
	setCurvePen(curve3, QPen(SpecLine2ColorPlot, 1, DotLine));
	setCurvePen(curve4, QPen(SpecLine2ColorPlot, 1, DotLine));

	curve5 = insertCurve(tr("Higher Bound"));
#ifdef _DEBUG_
	curve6 = insertCurve(tr("Lower bound"));
	setCurvePen(curve5, QPen(SpecLine1ColorPlot));
	setCurvePen(curve6, QPen(SpecLine2ColorPlot));
#else
	setCurvePen(curve5, QPen(SpecLine1ColorPlot, 1, DotLine));
#endif

	/* Add main curve */
	main1curve = insertCurve(tr("Channel Impulse Response"));
	
	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetAvIR(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					   _REAL rLowerB, _REAL rHigherB,
					   const _REAL rStartGuard, const _REAL rEndGuard,
					   const _REAL rBeginIR, const _REAL rEndIR)
{
	/* First check if plot must be set up */
	if (CurCharType != AVERAGED_IR)
	{
		CurCharType = AVERAGED_IR;
		SetupAvIR();
	}

	if (vecrScale.Size() != 0)
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
		setCurveData(curve1, dX, dY, 2);

		/* Right bound */
		dX[0] = dX[1] = rEndGuard;
		setCurveData(curve2, dX, dY, 2);

		/* Estimated begin of impulse response */
		dX[0] = dX[1] = rBeginIR;
		setCurveData(curve3, dX, dY, 2);

		/* Estimated end of impulse response */
		dX[0] = dX[1] = rEndIR;
		setCurveData(curve4, dX, dY, 2);


		/* Data for the actual impulse response curve */
		SetData(vecrData, vecrScale);


		/* Horizontal bounds ------------------------------------------------ */
		/* These bounds show the peak detection bound from timing tracking */
		dX[0] = vecrScale[0];
		dX[1] = vecrScale[vecrScale.Size() - 1];

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
		setCurveData(curve5, dX, dY, 2);

		/* Adjust scale for x-axis */
		setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], 
			(double) vecrScale[vecrScale.Size() - 1]);

		replot();
	}
	else
	{
		/* No input data, clear plot (by resetting it) */
		SetupAvIR();
	}
}

void CDRMPlot::SetupTranFct()
{
	/* Init chart for transfer function */
	setTitle(tr("Channel Transfer Function / Group Delay"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
	setAxisTitle(QwtPlot::yLeft, tr("TF [dB]"));

	setAxisTitle(QwtPlot::yRight, tr("Group Delay [ms]"));
	setAxisScale(QwtPlot::yRight, (double) -50.0, (double) 50.0);

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -85.0, (double) -35.0);

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Transf. Fct."));
	main2curve = insertCurve(tr("Group Del."),
		QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrData2,
						  CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != TRANSFERFUNCTION)
	{
		CurCharType = TRANSFERFUNCTION;
		SetupTranFct();
	}

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) vecrScale.Size());

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupAudioSpec()
{
	/* Init chart for audio spectrum */
	setTitle(tr("Audio Spectrum"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	setAxisTitle(QwtPlot::yLeft, "AS [dB]");

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) -90.0, (double) -20.0);
	double dBandwidth = (double) SOUNDCRD_SAMPLE_RATE / 2000;
	if (SOUNDCRD_SAMPLE_RATE == 48000)
		dBandwidth = (double) 20.0; /* Special value in case of 48 kHz */

	setAxisScale(QwtPlot::xBottom, (double) 0.0, dBandwidth);

	/* Add main curve */
	clear();
	main1curve = insertCurve(tr("Audio Spectrum"));
	
	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != AUDIO_SPECTRUM)
	{
		CurCharType = AUDIO_SPECTRUM;
		SetupAudioSpec();
	}

	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupFreqSamOffsHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("Rel. Frequency Offset / Sample Rate Offset History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [s]"));
	setAxisTitle(QwtPlot::yRight, tr("Sample Rate Offset [Hz]"));

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Freq."));
	main2curve = insertCurve(tr("Samp."), QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetFreqSamOffsHist(CVector<_REAL>& vecrData,
								  CVector<_REAL>& vecrData2,
								  CVector<_REAL>& vecrScale,
								  const _REAL rFreqOffAcquVal)
{
	/* First check if plot must be set up */
	if (CurCharType != FREQ_SAM_OFFS_HIST)
	{
		CurCharType = FREQ_SAM_OFFS_HIST;
		SetupFreqSamOffsHist();
	}

	QString strYLeftLabel = tr("Freq. Offset [Hz] rel. to ") +
		QString().setNum(rFreqOffAcquVal) + " Hz";
	setAxisTitle(QwtPlot::yLeft, strYLeftLabel);

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
	setAxisScale(QwtPlot::yLeft, (double) Floor(MinFreq / rMinScaleRange),
		(double) Ceil(MaxFreq / rMinScaleRange));
	setAxisScale(QwtPlot::yRight, (double) Floor(MinSam / rMinScaleRange),
		(double) Ceil(MaxSam / rMinScaleRange));
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupDopplerDelayHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("Delay / Doppler History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
	setAxisTitle(QwtPlot::yLeft, tr("Delay [ms]"));
	setAxisTitle(QwtPlot::yRight, tr("Doppler [Hz]"));

	/* Fixed scale */
	setAxisScale(QwtPlot::yLeft, (double) 0.0, (double) 10.0);
	setAxisScale(QwtPlot::yRight, (double) 0.0, (double) 4.0);

	/* Add main curves */
	clear();
	main1curve = insertCurve(tr("Delay"));
	main2curve = insertCurve(tr("Doppler"), QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetDopplerDelayHist(CVector<_REAL>& vecrData,
								   CVector<_REAL>& vecrData2,
								   CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != DOPPLER_DELAY_HIST)
	{
		CurCharType = DOPPLER_DELAY_HIST;
		SetupDopplerDelayHist();
	}

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupSNRAudHist()
{
	/* Init chart for transfer function. Enable right axis, too */
	setTitle(tr("SNR / Correctly Decoded Audio History"));
	enableAxis(QwtPlot::yRight);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Time [min]"));
	setAxisTitle(QwtPlot::yLeft, tr("SNR [dB]"));
	setAxisTitle(QwtPlot::yRight, tr("Corr. Dec. Audio / DRM-Frame"));

	/* Add main curves */
	clear();
	main1curve = insertCurve("SNR");
	main2curve = insertCurve("Audio", QwtPlot::xBottom, QwtPlot::yRight);

	/* Curve colors */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
	setCurvePen(main2curve, QPen(SpecLine2ColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));

	/* Legend */
	enableLegend(TRUE, main1curve);
	enableLegend(TRUE, main2curve);
}

void CDRMPlot::SetSNRAudHist(CVector<_REAL>& vecrData,
							 CVector<_REAL>& vecrData2,
							 CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != SNR_AUDIO_HIST)
	{
		CurCharType = SNR_AUDIO_HIST;
		SetupSNRAudHist();
	}

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
	setAxisScale(QwtPlot::yLeft, (double) 0.0, dMaxYScaleSNR);
	setAxisScale(QwtPlot::yRight, (double) 0.0, dMaxYScaleAudio);
	setAxisScale(QwtPlot::xBottom, (double) vecrScale[0], (double) 0.0);

	SetData(vecrData, vecrData2, vecrScale);
	replot();
}

void CDRMPlot::SetupPSD()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("Shifted Power Spectral Density of Input Signal"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	setAxisTitle(QwtPlot::yLeft, tr("PSD [dB]"));

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_SHIF_PSD_Y_AXIS_DB,
		MAX_VAL_SHIF_PSD_Y_AXIS_DB);

	/* Insert line for DC carrier */
	clear();
	curve1 = insertCurve(tr("DC carrier"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));

	double dX[2], dY[2];
	dX[0] = dX[1] = (_REAL) VIRTUAL_INTERMED_FREQ / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_SHIF_PSD_Y_AXIS_DB;
	dY[1] = MAX_VAL_SHIF_PSD_Y_AXIS_DB;

	setCurveData(curve1, dX, dY, 2);

	/* Add main curve */
	main1curve = insertCurve(tr("Shifted PSD"));
	
	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != POWER_SPEC_DENSITY)
	{
		CurCharType = POWER_SPEC_DENSITY;
		SetupPSD();
	}

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupSNRSpectrum()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("SNR Spectrum (Weighted MER on MSC Cells)"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Carrier Index"));
	setAxisTitle(QwtPlot::yLeft, tr("WMER [dB]"));

	/* Add main curve */
	clear();
	main1curve = insertCurve(tr("SNR Spectrum"));
	
	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetSNRSpectrum(CVector<_REAL>& vecrData,
							  CVector<_REAL>& vecrScale)
{
	/* First check if plot must be set up */
	if (CurCharType != SNR_SPECTRUM)
	{
		CurCharType = SNR_SPECTRUM;
		SetupSNRSpectrum();
	}

	const int iSize = vecrScale.Size();

	/* Fixed scale for x-axis */
	setAxisScale(QwtPlot::xBottom, (double) 0.0, (double) iSize);

	/* Fixed / variable scale (if SNR is in range, use fixed scale otherwise
	   enlarge scale) */
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
	setAxisScale(QwtPlot::yLeft, MIN_VAL_SNR_SPEC_Y_AXIS_DB, dMaxScaleYAxis);

	/* Set actual data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupInpSpec()
{
	/* Init chart for power spectram density estimation */
	setTitle(tr("Input Spectrum"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	setAxisTitle(QwtPlot::yLeft, tr("Input Spectrum [dB]"));

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom,
		(double) 0.0, (double) SOUNDCRD_SAMPLE_RATE / 2000);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for DC carrier */
	clear();
	curve1 = insertCurve(tr("DC carrier"));
	setCurvePen(curve1, QPen(SpecLine1ColorPlot, 1, DotLine));

	/* Add main curve */
	main1curve = insertCurve(tr("Input spectrum"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 1, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetInpSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
						  const _REAL rDCFreq)
{
	/* First check if plot must be set up */
	if (CurCharType != INPUTSPECTRUM_NO_AV)
	{
		CurCharType = INPUTSPECTRUM_NO_AV;
		SetupInpSpec();
	}

	/* Insert line for DC carrier */
	double dX[2], dY[2];
	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	setCurveData(curve1, dX, dY, 2);

	/* Insert actual spectrum data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupInpPSD()
{
	int		i;
	double	dX[2], dY[2];

	/* Init chart for power spectram density estimation */
	setTitle(tr("Input PSD"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Frequency [kHz]"));
	setAxisTitle(QwtPlot::yLeft, tr("Input PSD [dB]"));

	/* Fixed scale */
	const double dXScaleMax = (double) SOUNDCRD_SAMPLE_RATE / 2000;
	setAxisScale(QwtPlot::xBottom, (double) 0.0, dXScaleMax);

	setAxisScale(QwtPlot::yLeft, MIN_VAL_INP_SPEC_Y_AXIS_DB,
		MAX_VAL_INP_SPEC_Y_AXIS_DB);

	/* Insert line for bandwidth marker */
	clear();
	curve1 = insertCurve(tr("Filter bandwidth"));

	/* Make sure that line is bigger than the current plots height. Do this by
	   setting the width to a very large value. TODO: better solution */
	setCurvePen(curve1, QPen(PassBandColorPlot, 10000));

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
		setCurvePen(curvegrid, QPen(MainGridColorPlot, 0, DotLine));

		dY[0] = dY[1] = axisScale(QwtPlot::yLeft)->majMark(i);
		setCurveData(curvegrid, dX, dY, 2);
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
		setCurvePen(curvegrid, QPen(MainGridColorPlot, 0, DotLine));

		dX[0] = dX[1] = axisScale(QwtPlot::xBottom)->majMark(i);
		setCurveData(curvegrid, dX, dY, 2);
	}

	/* Insert line for DC carrier */
	curve2 = insertCurve(tr("DC carrier"));
	setCurvePen(curve2, QPen(SpecLine1ColorPlot, 1, DotLine));

	/* Add main curve */
	main1curve = insertCurve(tr("Input PSD"));

	/* Curve color */
	setCurvePen(main1curve, QPen(MainPenColorPlot, 2, SolidLine, RoundCap,
		RoundJoin));
}

void CDRMPlot::SetInpPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
						  const _REAL rDCFreq, const _REAL rBWCenter,
						  const _REAL rBWWidth)
{
	/* First check if plot must be set up */
	if (CurCharType != INPUT_SIG_PSD)
	{
		CurCharType = INPUT_SIG_PSD;
		SetupInpPSD();
	}

	/* Insert line for DC carrier */
	double dX[2], dY[2];
	dX[0] = dX[1] = rDCFreq / 1000;

	/* Take the min-max values from scale to get vertical line */
	dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
	dY[1] = MAX_VAL_INP_SPEC_Y_AXIS_DB;

	setCurveData(curve2, dX, dY, 2);

	/* Insert marker for filter bandwidth if required */
	if (rBWWidth != (_REAL) 0.0)
	{
		dX[0] = (rBWCenter - rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;
		dX[1] = (rBWCenter + rBWWidth / 2) * SOUNDCRD_SAMPLE_RATE / 1000;

		/* Take the min-max values from scale to get vertical line */
		dY[0] = MIN_VAL_INP_SPEC_Y_AXIS_DB;
		dY[1] = MIN_VAL_INP_SPEC_Y_AXIS_DB;

		setCurveData(curve1, dX, dY, 2);
	}
	else
		setCurveData(curve1, NULL, NULL, 0);

	/* Insert actual spectrum data */
	SetData(vecrData, vecrScale);
	replot();
}

void CDRMPlot::SetupFACConst()
{
	/* Init chart for FAC constellation */
	setTitle(tr("FAC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

	/* Fixed scale (2 / sqrt(2)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.4142, (double) 1.4142);
	setAxisScale(QwtPlot::yLeft, (double) -1.4142, (double) 1.4142);

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(4);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));

	/* Insert grid */
	clear();
	SetQAM4Grid();
}

void CDRMPlot::SetFACConst(CVector<_COMPLEX>& veccData)
{
	/* First check if plot must be set up */
	if (CurCharType != FAC_CONSTELLATION)
	{
		CurCharType = FAC_CONSTELLATION;
		SetupFACConst();
	}

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupSDCConst(const CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for SDC constellation */
	setTitle(tr("SDC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

	/* Fixed scale (4 / sqrt(10)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2649, (double) 1.2649);
	setAxisScale(QwtPlot::yLeft, (double) -1.2649, (double) 1.2649);

	/* Insert grid */
	clear();
	if (eNewCoSc == CParameter::CS_1_SM)
		SetQAM4Grid();
	else
		SetQAM16Grid();

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(4);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));
}

void CDRMPlot::SetSDCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Always set up plot. TODO: only set up plot if constellation
	   scheme has changed */
	CurCharType = SDC_CONSTELLATION;
	SetupSDCConst(eNewCoSc);

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupMSCConst(const CParameter::ECodScheme eNewCoSc)
{
	/* Init chart for MSC constellation */
	setTitle(tr("MSC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(FALSE);
	enableGridY(FALSE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

	/* Fixed scale (8 / sqrt(42)) */
	setAxisScale(QwtPlot::xBottom, (double) -1.2344, (double) 1.2344);
	setAxisScale(QwtPlot::yLeft, (double) -1.2344, (double) 1.2344);

	/* Insert grid */
	clear();
	if (eNewCoSc == CParameter::CS_2_SM)
		SetQAM16Grid();
	else
		SetQAM64Grid();

	/* Set marker symbol */
	MarkerSym1.setStyle(QwtSymbol::Ellipse);
	MarkerSym1.setSize(2);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));
}

void CDRMPlot::SetMSCConst(CVector<_COMPLEX>& veccData,
						   CParameter::ECodScheme eNewCoSc)
{
	/* Always set up plot. TODO: only set up plot if constellation
	   scheme has changed */
	CurCharType = MSC_CONSTELLATION;
	SetupMSCConst(eNewCoSc);

	removeMarkers();
	SetData(veccData);
	replot();
}

void CDRMPlot::SetupAllConst()
{
	/* Init chart for constellation */
	setTitle(tr("MSC / SDC / FAC Constellation"));
	enableAxis(QwtPlot::yRight, FALSE);
	enableGridX(TRUE);
	enableGridY(TRUE);
	setAxisTitle(QwtPlot::xBottom, tr("Real"));
	setAxisTitle(QwtPlot::yLeft, tr("Imaginary"));

	/* Fixed scale */
	setAxisScale(QwtPlot::xBottom, (double) -1.5, (double) 1.5);
	setAxisScale(QwtPlot::yLeft, (double) -1.5, (double) 1.5);

	/* Set marker symbols */
	/* MSC */
	MarkerSym1.setStyle(QwtSymbol::Rect);
	MarkerSym1.setSize(2);
	MarkerSym1.setPen(QPen(MainPenColorConst));
	MarkerSym1.setBrush(QBrush(MainPenColorConst));

	/* SDC */
	MarkerSym2.setStyle(QwtSymbol::XCross);
	MarkerSym2.setSize(4);
	MarkerSym2.setPen(QPen(SpecLine1ColorPlot));
	MarkerSym2.setBrush(QBrush(SpecLine1ColorPlot));

	/* FAC */
	MarkerSym3.setStyle(QwtSymbol::Ellipse);
	MarkerSym3.setSize(4);
	MarkerSym3.setPen(QPen(SpecLine2ColorPlot));
	MarkerSym3.setBrush(QBrush(SpecLine2ColorPlot));

	/* Insert "dummy" curves for legend */
	clear();
	curve1 = insertCurve("MSC");
	setCurveSymbol(curve1, MarkerSym1);
	setCurvePen(curve1, QPen(Qt::NoPen));
	enableLegend(TRUE, curve1);

	curve2 = insertCurve("SDC");
	setCurveSymbol(curve2, MarkerSym2);
	setCurvePen(curve2, QPen(Qt::NoPen));
	enableLegend(TRUE, curve2);

	curve3 = insertCurve("FAC");
	setCurveSymbol(curve3, MarkerSym3);
	setCurvePen(curve3, QPen(Qt::NoPen));
	enableLegend(TRUE, curve3);
}

void CDRMPlot::SetAllConst(CVector<_COMPLEX>& veccMSC,
						   CVector<_COMPLEX>& veccSDC,
						   CVector<_COMPLEX>& veccFAC)
{
	/* First check if plot must be set up */
	if (CurCharType != ALL_CONSTELLATION)
	{
		CurCharType = ALL_CONSTELLATION;
		SetupAllConst();
	}

	removeMarkers();

	SetData(veccMSC, veccSDC, veccFAC);

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

	/* Check if value is valid */
	if (dMaxxBottom != (double) 0.0)
	{
		/* Emit signal containing normalized selected frequency */
		emit xAxisValSet(dFreq / dMaxxBottom);
	}
}
