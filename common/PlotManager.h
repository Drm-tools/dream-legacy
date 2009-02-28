/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Andrea Russo, Oliver Haffenden
 *
 * Description:
 *	See PlotManager.cpp
 *
 *
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

#if !defined(PLOT_MANAGER_H_INCLUDED)
#define PLOT_MANAGER_H_INCLUDED

#include "Parameter.h"
#ifdef USE_QT_GUI
# include <qmutex.h>
#endif
#include <deque>

/* Definitions ****************************************************************/

/* Length of the history for synchronization parameters (used for the plot) */
#define LEN_HIST_PLOT_SYNC_PARMS		2250


class CDRMReceiver;

class CPlotManager
{
public:

	enum EPlotType
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

	CPlotManager(CDRMReceiver*);

	void Init();

    void startPlot(EPlotType);
    void endPlot(EPlotType);

    void GetTransferFunction(vector<double>& transferFunc, vector<double>& groupDelay);
	void GetInputPSD(vector<_REAL>& vecrData, _REAL& rStart, _REAL& rStep);
	void GetInputSpectrum(vector<_REAL>& vecrData, _REAL& rStart, _REAL& rStep);
	bool GetAvPoDeSp(CMeasurements::CPIR&);
	void GetSNRProfile(vector<_REAL>& vecrData, vector<_REAL>& vecrScale);
	void GetPowDenSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale);
	void GetAudioSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale);

	void GetFACVectorSpace(vector<_COMPLEX>&);
	void GetSDCVectorSpace(vector<_COMPLEX>&);
	void GetMSCVectorSpace(vector<_COMPLEX>&);

	ECodScheme GetSDCCodingScheme();
	ECodScheme GetMSCCodingScheme();

	void GetAnalogBWParameters(_REAL& rCenterFreq, _REAL& rBW);
    _REAL GetAnalogCurMixFreqOffs() const;

	/* Interfaces to internal parameters/vectors used for the plot */
	void GetFreqSamOffsHist(vector<_REAL>& vecrFreqOffs,
		vector<_REAL>& vecrSamOffs, _REAL& rFreqAquVal);

	void GetDopplerDelHist(vector<_REAL>& vecrLenIR, vector<_REAL>& vecrDoppler);

	void GetSNRHist(vector<_REAL>& vecrSNR, vector<_REAL>& vecrCDAud);

    _REAL GetDCFrequency();
    _REAL GetSymbolDuration();
    _REAL GetFrameDuration();


private:
	CDRMReceiver			*pReceiver;
#ifdef USE_QT_GUI
	QMutex					MutexHist;
#endif

};

#endif
