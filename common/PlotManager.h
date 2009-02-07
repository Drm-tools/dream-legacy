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
//# include <qwt_plot.h>
#endif

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

	CPlotManager();

	void SetReceiver(CDRMReceiver *pRx) {pReceiver = pRx;}

	void Init();

	void SetCurrentCDAud(int iN) {iCurrentCDAud = iN;}

	void UpdateParamHistories();

	void UpdateParamHistoriesRSIIn();

	void GetTransferFunction(CVector<_REAL>& vecrData,
		CVector<_REAL>& vecrGrpDly,	CVector<_REAL>& vecrScale);

	void GetAvPoDeSp(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
					 _REAL& rLowerBound, _REAL& rHigherBound,
					 _REAL& rStartGuard, _REAL& rEndGuard, _REAL& rPDSBegin,
					 _REAL& rPDSEnd);

	void GetSNRProfile(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void GetPowDenSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void  GetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void   GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void  GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

	void GetFACVectorSpace(CVector<_COMPLEX>&);
	void GetSDCVectorSpace(CVector<_COMPLEX>&, ECodScheme&);
	void GetMSCVectorSpace(CVector<_COMPLEX>&, ECodScheme&);

	void GetAnalogBWParameters(CReal& rCenterFreq, CReal& rBW);
    CReal GetAnalogCurMixFreqOffs() const;

	/* Interfaces to internal parameters/vectors used for the plot */
	void GetFreqSamOffsHist(CVector<_REAL>& vecrFreqOffs,
		CVector<_REAL>& vecrSamOffs, CVector<_REAL>& vecrScale,
		_REAL& rFreqAquVal);

	void GetDopplerDelHist(CVector<_REAL>& vecrLenIR,
		CVector<_REAL>& vecrDoppler, CVector<_REAL>& vecrScale);

	void GetSNRHist(CVector<_REAL>& vecrSNR, CVector<_REAL>& vecrCDAud,
		CVector<_REAL>& vecrScale);

    _REAL GetDCFrequency();


private:
	CDRMReceiver			*pReceiver;
	/* Storing parameters for plot */
	CShiftRegister<_REAL>	vecrFreqSyncValHist;
	CShiftRegister<_REAL>	vecrSamOffsValHist;
	CShiftRegister<_REAL>	vecrLenIRHist;
	CShiftRegister<_REAL>	vecrDopplerHist;
	CShiftRegister<_REAL>	vecrSNRHist;
	CShiftRegister<int>		veciCDAudHist;
	int						iSymbolCount;
	_REAL					rSumDopplerHist;
	_REAL					rSumSNRHist;
	int						iCurrentCDAud;
#ifdef USE_QT_GUI
	QMutex					MutexHist;
#endif

};

#endif
