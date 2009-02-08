/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Andrea Russo, Oliver Haffenden
 *
 * Description:
 *	This class takes care of keeping the history plots as well as interfacing to the
 *  relevant module in the case of the other plots, including getting data either from
 *  the RSCI input or the demodulator
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

#include "PlotManager.h"
#include "DrmReceiver.h"
#include <algorithm>
#include <functional>
#include <iostream>

CPlotManager::CPlotManager() :
	pReceiver(0),
	vecrFreqSyncValHist(LEN_HIST_PLOT_SYNC_PARMS),
	vecrSamOffsValHist(LEN_HIST_PLOT_SYNC_PARMS),
	vecrLenIRHist(LEN_HIST_PLOT_SYNC_PARMS),
	vecrDopplerHist(LEN_HIST_PLOT_SYNC_PARMS),
	vecrSNRHist(LEN_HIST_PLOT_SYNC_PARMS),
	veciCDAudHist(LEN_HIST_PLOT_SYNC_PARMS), iSymbolCount(0),
	rSumDopplerHist((_REAL) 0.0), rSumSNRHist((_REAL) 0.0), iCurrentCDAud(0)
{
}

void CPlotManager::startPlot(EPlotType e)
{
    CParameter& Parameters = *pReceiver->GetParameters();
	switch (e)
	{
	case AVERAGED_IR:
        //Parameters.Measurements.subscribe(CMeasurements::);
		break;
	case TRANSFERFUNCTION:
        Parameters.Measurements.subscribe(CMeasurements::CHANNEL);
		break;
	case POWER_SPEC_DENSITY:
        //Parameters.Measurements.subscribe();
		break;
	case SNR_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case INP_SPEC_WATERF:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case INPUT_SIG_PSD:
        Parameters.Measurements.subscribe(CMeasurements::PSD);
		break;
	case INPUT_SIG_PSD_ANALOG:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case AUDIO_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
        //Parameters.Measurements.subscribe();
		break;
	case DOPPLER_DELAY_HIST:
        Parameters.Measurements.subscribe(CMeasurements::DELAY);
        Parameters.Measurements.subscribe(CMeasurements::DOPPLER);
		break;
	case SNR_AUDIO_HIST:
        //Parameters.Measurements.subscribe();
		break;
	case FAC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case SDC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case MSC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case ALL_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case NONE_OLD:
		break;
	}
}

void CPlotManager::endPlot(EPlotType e)
{
    CParameter& Parameters = *pReceiver->GetParameters();
	switch (e)
	{
	case AVERAGED_IR:
        //Parameters.Measurements.subscribe(CMeasurements::);
		break;
	case TRANSFERFUNCTION:
        Parameters.Measurements.subscribe(CMeasurements::CHANNEL);
		break;
	case POWER_SPEC_DENSITY:
        //Parameters.Measurements.subscribe();
		break;
	case SNR_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case INP_SPEC_WATERF:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case INPUT_SIG_PSD:
        Parameters.Measurements.subscribe(CMeasurements::PSD);
		break;
	case INPUT_SIG_PSD_ANALOG:
        Parameters.Measurements.subscribe(CMeasurements::INPUT_SPECTRUM);
		break;
	case AUDIO_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
        //Parameters.Measurements.subscribe();
		break;
	case DOPPLER_DELAY_HIST:
        Parameters.Measurements.subscribe(CMeasurements::DELAY);
        Parameters.Measurements.subscribe(CMeasurements::DOPPLER);
		break;
	case SNR_AUDIO_HIST:
        //Parameters.Measurements.subscribe();
		break;
	case FAC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case SDC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case MSC_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case ALL_CONSTELLATION:
        //Parameters.Measurements.subscribe();
		break;
	case NONE_OLD:
		break;
	}
}

void CPlotManager::Init()
{
	iSymbolCount = 0;
	rSumDopplerHist = (_REAL) 0.0;
	rSumSNRHist = (_REAL) 0.0;
}

// ====== Generators for scale (X data) ============================
class iotaGen
{
public:
  iotaGen (int start = 0) : current(start) { }
  int operator() () { return current++; }
private:
  int current;
};

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

void
CPlotManager::UpdateParamHistories()
{

	/* TODO: do not use the shift register class, build a new
	   one which just increments a pointer in a buffer and put
	   the new value at the position of the pointer instead of
	   moving the total data all the time -> special care has
	   to be taken when reading out the data */

    CParameter& Parameters = *pReceiver->GetParameters();

    Parameters.Lock();
    _REAL rFreqOffsetTrack = Parameters.rFreqOffsetTrack;
    _REAL rResampleOffset = Parameters.rResampleOffset;
    _REAL rSNR = Parameters.GetSNR();
    _REAL rSigmaEstimate = Parameters.Measurements.rSigmaEstimate;
    _REAL iNumSymPerFrame = Parameters.CellMappingTable.iNumSymPerFrame;
    _REAL rMeanDelay = (Parameters.Measurements.rMinDelay + Parameters.Measurements.rMaxDelay) / 2.0;
    Parameters.Unlock();

#ifdef USE_QT_GUI
    MutexHist.lock();
#endif

    /* Frequency offset tracking values */
    vecrFreqSyncValHist.push_front(rFreqOffsetTrack * SOUNDCRD_SAMPLE_RATE);

    /* Sample rate offset estimation */
    vecrSamOffsValHist.push_front(rResampleOffset);
    /* Signal to Noise ratio estimates */
    rSumSNRHist += rSNR;

    /* TODO - reconcile this with Ollies RSCI Doppler code in ChannelEstimation */
    /* Average Doppler estimate */
    rSumDopplerHist += rSigmaEstimate;

    /* Only evaluate Doppler and delay once in one DRM frame */
    iSymbolCount++;
    if (iSymbolCount == iNumSymPerFrame)
    {
        /* Apply averaged values to the history vectors */
        vecrLenIRHist.push_front(rMeanDelay);

        vecrSNRHist.push_front(rSumSNRHist / iNumSymPerFrame);

        vecrDopplerHist.push_front(rSumDopplerHist / iNumSymPerFrame);

        /* At the same time, add number of correctly decoded audio blocks.
           This number is updated once a DRM frame. Since the other
           parameters like SNR is also updated once a DRM frame, the two
           values are synchronized by one DRM frame */
        veciCDAudHist.push_front(iCurrentCDAud);

        /* Reset parameters used for averaging */
        iSymbolCount = 0;
        rSumDopplerHist = (_REAL) 0.0;
        rSumSNRHist = (_REAL) 0.0;
    }

#ifdef USE_QT_GUI
    MutexHist.unlock();
#endif
}

void
CPlotManager::UpdateParamHistoriesRSIIn()
{
	/* This function is only called once per RSI frame, so process every time */

	CParameter& Parameters = *pReceiver->GetParameters();

	Parameters.Lock();
	_REAL rDelay = _REAL(0.0);
	if (Parameters.Measurements.vecrRdelIntervals.size() > 0)
		rDelay = Parameters.Measurements.vecrRdelIntervals[0];
	_REAL rMER = Parameters.Measurements.rMER;
	_REAL rRdop = Parameters.Measurements.rRdop;
	Parameters.Unlock();

#ifdef USE_QT_GUI
		MutexHist.lock();
#endif

	/* Apply averaged values to the history vectors */
	vecrLenIRHist.push_front(rDelay);
	vecrSNRHist.push_front(rMER);
	vecrDopplerHist.push_front(rRdop);

	/* At the same time, add number of correctly decoded audio blocks.
	   This number is updated once a DRM frame. Since the other
	   parameters like SNR is also updated once a DRM frame, the two
	   values are synchronized by one DRM frame */
	veciCDAudHist.push_front(iCurrentCDAud);
	/* Reset parameters used for averaging */
	iSymbolCount = 0;
	rSumDopplerHist = (_REAL) 0.0;
	rSumSNRHist = (_REAL) 0.0;

#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CPlotManager::GetFreqSamOffsHist(vector<_REAL>& vecrFreqOffs,
								 vector<_REAL>& vecrSamOffs,
								 vector<_REAL>& vecrScale,
								 _REAL& rFreqAquVal)
{
	CParameter& ReceiverParam = *pReceiver->GetParameters();

	ReceiverParam.Lock();
	/* Duration of OFDM symbol */
	const _REAL rTs = (CReal) (ReceiverParam.CellMappingTable.iFFTSizeN + ReceiverParam.CellMappingTable.iGuardSize) / SOUNDCRD_SAMPLE_RATE;
	/* Value from frequency acquisition */
	rFreqAquVal = ReceiverParam.rFreqOffsetAcqui * SOUNDCRD_SAMPLE_RATE;
	ReceiverParam.Unlock();

	/* Init output vectors */
	vecrFreqOffs.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrSamOffs.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrScale.resize(LEN_HIST_PLOT_SYNC_PARMS);

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffers in output buffers */
	vecrFreqOffs.assign(vecrFreqSyncValHist.begin(), vecrFreqSyncValHist.end());
	vecrSamOffs.assign(vecrSamOffsValHist.begin(), vecrSamOffsValHist.end());

	/* Calculate time scale */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rTs;

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CPlotManager::GetDopplerDelHist(vector<_REAL>& vecrLenIR,
								vector<_REAL>& vecrDoppler,
								vector<_REAL>& vecrScale)
{
	CParameter& ReceiverParam = *pReceiver->GetParameters();

	/* Init output vectors */
	vecrLenIR.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrDoppler.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrScale.resize(LEN_HIST_PLOT_SYNC_PARMS);

	ReceiverParam.Lock();
	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (ReceiverParam.CellMappingTable.iFFTSizeN
							+ ReceiverParam.CellMappingTable.iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * ReceiverParam.CellMappingTable.iNumSymPerFrame;
	ReceiverParam.Unlock();

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffers in output buffers */
	vecrLenIR.assign(vecrLenIRHist.begin(), vecrLenIRHist.end());
	vecrDoppler.assign(vecrDopplerHist.begin(), vecrDopplerHist.end());


	/* Calculate time scale in minutes */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rDRMFrameDur / 60;

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CPlotManager::GetSNRHist(vector < _REAL > &vecrSNR,
						 vector < _REAL > &vecrCDAud,
						 vector < _REAL > &vecrScale)
{
	CParameter& ReceiverParam = *pReceiver->GetParameters();
	/* Duration of DRM frame */
	ReceiverParam.Lock();
	/* Duration of DRM frame */
	const _REAL rDRMFrameDur = (CReal) (ReceiverParam.CellMappingTable.iFFTSizeN + ReceiverParam.CellMappingTable.iGuardSize) /
		SOUNDCRD_SAMPLE_RATE * ReceiverParam.CellMappingTable.iNumSymPerFrame;
	ReceiverParam.Unlock();

	/* Init output vectors */
	vecrSNR.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrCDAud.resize(LEN_HIST_PLOT_SYNC_PARMS);
	vecrScale.resize(LEN_HIST_PLOT_SYNC_PARMS);

	/* Lock resources */
#ifdef USE_QT_GUI
	MutexHist.lock();
#endif

	/* Simply copy history buffer in output buffer */
	vecrSNR.assign(vecrSNRHist.begin(), vecrSNRHist.end());

	/* Calculate time scale. Copy correctly decoded audio blocks history (must
	   be transformed from "int" to "real", therefore we need a for-loop */
	for (int i = 0; i < LEN_HIST_PLOT_SYNC_PARMS; i++)
	{
		/* Scale in minutes */
		vecrScale[i] = (i - LEN_HIST_PLOT_SYNC_PARMS + 1) * rDRMFrameDur / 60;

		/* Correctly decoded audio blocks */
		vecrCDAud[i] = (_REAL) veciCDAudHist[i];
	}

	/* Release resources */
#ifdef USE_QT_GUI
	MutexHist.unlock();
#endif
}

void
CPlotManager::GetInputPSD(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
{
	CParameter& Parameters = *pReceiver->GetParameters();
    // read it from the parameter structure
    Parameters.Lock();
    vecrData = Parameters.Measurements.vecrPSD;
    bool etsi = Parameters.Measurements.bETSIPSD;
    Parameters.Unlock();

    int iVectorLen = vecrData.size();
    vecrScale.resize(iVectorLen);

    if(etsi) // if the RSI output is turned on we display that version
    {
        // starting frequency and frequency step as defined in TS 102 349
        // plot expects the scale values in kHz
        _REAL f = _REAL(-7.875) + VIRTUAL_INTERMED_FREQ/_REAL(1000.0);
        const _REAL fstep =_REAL(0.1875);

        generate(vecrScale.begin(), vecrScale.end(), scaleGen(fstep, f));
    }
    else // Traditional Dream values
    {
        _REAL rFactorScale = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(iVectorLen) / 2000.0;
        generate(vecrScale.begin(), vecrScale.end(), scaleGen(rFactorScale));
    }
}

class TransferFunction : unary_function<CComplex,double>
{
public:
    double operator() (CComplex) const;
};

double TransferFunction::operator()(CComplex val) const
{
    const _REAL rNormData = (_REAL) numeric_limits<_SAMPLE>::max() * numeric_limits<_SAMPLE>::max();
    const _REAL rNormSqMagChanEst = SqMag(val) / rNormData;

    if (rNormSqMagChanEst > 0)
        return (_REAL) 10.0 * Log10(rNormSqMagChanEst);
    else
        return RET_VAL_LOG_0;
}

class GroupDelay : unary_function<CComplex,double>
{
public:
    GroupDelay(CComplex init, CReal tu):rOldPhase(Angle(init)),rTu(tu){}
    double operator() (CComplex);
private:
    CReal rOldPhase;
    CReal rTu;
};

double GroupDelay::operator()(CComplex val)
{
    CReal rCurphase = Angle(val);
    CReal rDiffPhase = rCurphase - rOldPhase;

    /* Store phase */
    rOldPhase = rCurphase;

    /* Take care of wrap around of angle() function */
    if (rDiffPhase > WRAP_AROUND_BOUND_GRP_DLY)
        rDiffPhase -= 2.0 * crPi;
    if (rDiffPhase < -WRAP_AROUND_BOUND_GRP_DLY)
        rDiffPhase += 2.0 * crPi;

    /* Apply normalization */
    return rDiffPhase * rTu * 1000.0 /* ms */;
}

void CPlotManager::GetTransferFunction(vector<double>& transferFunc,
    vector<double>& groupDelay, vector<double>& scale)
{
    int iNumCarrier, iFFTSizeN=0;
	vector<CComplex> veccChanEst;

    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
    if(Parameters.Measurements.veccChanEst.size()>0)
    {
        veccChanEst = Parameters.Measurements.veccChanEst;
        iFFTSizeN = Parameters.CellMappingTable.iFFTSizeN;
    }
    Parameters.Unlock();

    iNumCarrier = veccChanEst.size();
    if(iNumCarrier==0)
        return; // not running yet

	scale.resize(iNumCarrier);
    generate(scale.begin(), scale.end(), iotaGen());

    transferFunc.resize(iNumCarrier);
    transform(
        veccChanEst.begin(), veccChanEst.end(),
        transferFunc.begin(),
        TransferFunction()
    );

	groupDelay.resize(iNumCarrier);
    transform(veccChanEst.begin(), veccChanEst.end(),
        groupDelay.begin(),
        GroupDelay(veccChanEst[0], CReal(iFFTSizeN) / CReal(SOUNDCRD_SAMPLE_RATE))
    );
}

void CPlotManager::GetAvPoDeSp(vector<_REAL>& vecrData, vector<_REAL>& vecrScale,
	        _REAL& rLowerB, _REAL& rHigherB, _REAL& rStartGuard, _REAL& rEndGuard,
	        _REAL& rBeginIR, _REAL& rEndIR)
{
	CParameter& Parameters = *pReceiver->GetParameters();

    // read it from the parameter structure
    Parameters.Lock();
    vector<_REAL> vecrPIR = Parameters.Measurements.vecrPIR;
    _REAL rPIRStart = Parameters.Measurements.rPIRStart;
    _REAL rPIREnd = Parameters.Measurements.rPIREnd;
    rLowerB = Parameters.Measurements.rLowerBound;
    rHigherB = Parameters.Measurements.rHigherBound;
    rStartGuard = Parameters.Measurements.rStartGuard;
    rEndGuard = Parameters.Measurements.rEndGuard;
    rBeginIR = Parameters.Measurements.rPDSBegin;
    rEndIR = Parameters.Measurements.rPDSEnd;
    Parameters.Unlock();

    int iVectorLen = vecrPIR.size();
    vecrData.resize(iVectorLen);
    vecrScale.resize(iVectorLen);

    // starting frequency and frequency step as defined in TS 102 349
    // plot expects the scale values in kHz
    _REAL t = rPIRStart;
    const _REAL tstep = (rPIREnd-rPIRStart)/(_REAL(iVectorLen)-1);

    for (int i=0; i<iVectorLen; i++)
    {
        vecrData[i] = vecrPIR[i];
        vecrScale[i] = t;
        t += tstep;
    }
}

void CPlotManager::GetSNRProfile(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
{
	pReceiver->GetChannelEstimation()->GetSNRProfile(vecrData, vecrScale);
}

void CPlotManager::GetPowDenSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
{
    pReceiver->GetOFDMDemod()->GetPowDenSpec(vecrData, vecrScale);
}

void CPlotManager::GetAudioSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
{
    pReceiver->GetAudioSpec(vecrData, vecrScale);
}

void CPlotManager::GetInputSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
{
    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
    vecrData = Parameters.Measurements.vecrInpSpec;
    Parameters.Unlock();
	vecrScale.resize(vecrData.size());
    _REAL rFactorScale = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
    generate(vecrScale.begin(), vecrScale.end(), scaleGen(rFactorScale));
}

void CPlotManager::GetAnalogBWParameters(CReal& rCenterFreq, CReal& rBW)
{
    pReceiver->GetAnalogBWParameters(rCenterFreq, rBW);
}

CReal CPlotManager::GetAnalogCurMixFreqOffs() const
{
    return pReceiver->GetAnalogCurMixFreqOffs();
}

void CPlotManager::GetFACVectorSpace(vector<_COMPLEX>& vec)
{
    pReceiver->GetFACVectorSpace(vec);
}

void CPlotManager::GetSDCVectorSpace(vector<_COMPLEX>& vec, ECodScheme& eCS)
{
    pReceiver->GetSDCVectorSpace(vec);
    eCS = pReceiver->GetParameters()->eSDCCodingScheme;
}

void CPlotManager::GetMSCVectorSpace(vector<_COMPLEX>& vec, ECodScheme& eCS)
{
    pReceiver->GetMSCVectorSpace(vec);
    eCS = pReceiver->GetParameters()->eMSCCodingScheme;
}

_REAL CPlotManager::GetDCFrequency()
{
    return pReceiver->GetParameters()->GetDCFrequency();
}
