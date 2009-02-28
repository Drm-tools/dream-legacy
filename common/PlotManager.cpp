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
#include <iostream>

CPlotManager::CPlotManager(CDRMReceiver* rec) :
	pReceiver(rec)
{
}

void CPlotManager::startPlot(EPlotType e)
{
    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
    const _REAL rDRMFrameDur =
        _REAL(Parameters.CellMappingTable.iFFTSizeN + Parameters.CellMappingTable.iGuardSize)
        /
		_REAL(SOUNDCRD_SAMPLE_RATE * Parameters.CellMappingTable.iNumSymPerFrame);
	switch (e)
	{
	case AVERAGED_IR:
        Parameters.Measurements.PIR.subscribe();
		break;
	case TRANSFERFUNCTION:
        Parameters.Measurements.ChannelEstimate.subscribe();
		break;
	case POWER_SPEC_DENSITY:
        //Parameters.Measurements.subscribe();
		break;
	case SNR_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
        Parameters.Measurements.inputSpectrum.subscribe();
		break;
	case INP_SPEC_WATERF:
        Parameters.Measurements.inputSpectrum.subscribe();
		break;
	case INPUT_SIG_PSD:
	case INPUT_SIG_PSD_ANALOG:
        Parameters.Measurements.PSD.subscribe();
		break;
	case AUDIO_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
        Parameters.Measurements.FreqSyncValHist.subscribe();
        Parameters.Measurements.SamOffsValHist.subscribe();
        Parameters.Measurements.FreqSyncValHist.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
        Parameters.Measurements.SamOffsValHist.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
		break;
	case DOPPLER_DELAY_HIST:
        Parameters.Measurements.Doppler.subscribe();
        //Parameters.Measurements.Doppler.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
        Parameters.Measurements.Delay.subscribe();
        //Parameters.Measurements.Delay.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
		break;
	case SNR_AUDIO_HIST:
        Parameters.Measurements.SNRHist.subscribe();
        Parameters.Measurements.SNRHist.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
        Parameters.Measurements.audioFrameStatus.subscribe();
        Parameters.Measurements.audioFrameStatus.configure(LEN_HIST_PLOT_SYNC_PARMS, rDRMFrameDur);
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
    Parameters.Unlock();
}

void CPlotManager::endPlot(EPlotType e)
{
    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
	switch (e)
	{
	case AVERAGED_IR:
        Parameters.Measurements.PIR.unsubscribe();
		break;
	case TRANSFERFUNCTION:
        Parameters.Measurements.ChannelEstimate.unsubscribe();
		break;
	case POWER_SPEC_DENSITY:
        //Parameters.Measurements.subscribe();
		break;
	case SNR_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case INPUTSPECTRUM_NO_AV:
        Parameters.Measurements.inputSpectrum.unsubscribe();
		break;
	case INP_SPEC_WATERF:
        Parameters.Measurements.inputSpectrum.unsubscribe();
		break;
	case INPUT_SIG_PSD:
        Parameters.Measurements.PSD.subscribe();
		break;
	case INPUT_SIG_PSD_ANALOG:
        Parameters.Measurements.inputSpectrum.unsubscribe();
		break;
	case AUDIO_SPECTRUM:
        //Parameters.Measurements.subscribe();
		break;
	case FREQ_SAM_OFFS_HIST:
        Parameters.Measurements.FreqSyncValHist.unsubscribe();
        Parameters.Measurements.SamOffsValHist.unsubscribe();
		break;
	case DOPPLER_DELAY_HIST:
        Parameters.Measurements.Doppler.unsubscribe();
        Parameters.Measurements.Delay.unsubscribe();
		break;
	case SNR_AUDIO_HIST:
        Parameters.Measurements.SNRHist.unsubscribe();
        Parameters.Measurements.audioFrameStatus.unsubscribe();
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
    Parameters.Unlock();
}

void CPlotManager::Init()
{
    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Measurements.Doppler.reset();
    Parameters.Measurements.SNRHist.reset();
    // TODO - why not the other histories ?
}

/* Duration of OFDM symbol */
_REAL CPlotManager::GetSymbolDuration()
{
	CParameter& Parameters = *pReceiver->GetParameters();
	Parameters.Lock();
    _REAL r = _REAL(Parameters.CellMappingTable.iFFTSizeN + Parameters.CellMappingTable.iGuardSize)
        /
        _REAL(SOUNDCRD_SAMPLE_RATE);
	Parameters.Unlock();
    return r;
}

_REAL CPlotManager::GetFrameDuration()
{
	CParameter& Parameters = *pReceiver->GetParameters();
	Parameters.Lock();
	/* Duration of DRM frame */
	_REAL r = _REAL(Parameters.CellMappingTable.iNumSymPerFrame)
        *
        _REAL(Parameters.CellMappingTable.iFFTSizeN+Parameters.CellMappingTable.iGuardSize)
        /
		_REAL(SOUNDCRD_SAMPLE_RATE);
	Parameters.Unlock();
    return r;
}

void
CPlotManager::GetFreqSamOffsHist(vector<_REAL>& vecrFreqOffs,
								 vector<_REAL>& vecrSamOffs,
								 _REAL& rFreqAquVal)
{
	CParameter& Parameters = *pReceiver->GetParameters();

	Parameters.Lock();
	/* Value from frequency acquisition */
	rFreqAquVal = Parameters.rFreqOffsetAcqui * SOUNDCRD_SAMPLE_RATE;

    // TODO set the max on the measurement objects to LEN_HIST_PLOT_SYNC_PARMS
    Parameters.Measurements.FreqSyncValHist.get(vecrFreqOffs);
    Parameters.Measurements.SamOffsValHist.get(vecrSamOffs);

	Parameters.Unlock();

}

void
CPlotManager::GetDopplerDelHist(vector<_REAL>& vecrLenIR, vector<_REAL>& vecrDoppler)
{
	CParameter& Parameters = *pReceiver->GetParameters();

	Parameters.Lock();

	Parameters.Measurements.Delay.get(vecrLenIR);
	Parameters.Measurements.Doppler.get(vecrDoppler);

	Parameters.Unlock();
}

void
CPlotManager::GetSNRHist(vector <_REAL>& vecrSNR, vector <_REAL>& vecrCDAud)
{
	CParameter& Parameters = *pReceiver->GetParameters();
	/* Duration of DRM frame */
	Parameters.Lock();
    Parameters.Measurements.SNRHist.get(vecrSNR);
    vector<int> v;
    Parameters.Measurements.CDAudHist.get(v);
    vecrCDAud.resize(v.size());
    vecrCDAud.assign(v.begin(), v.end());
	Parameters.Unlock();
}

void
CPlotManager::GetInputPSD(vector<_REAL>& vecrData, _REAL& rStart, _REAL& rStep)
{
	CParameter& Parameters = *pReceiver->GetParameters();
    // read it from the parameter structure
    Parameters.Lock();
    bool psdOk = Parameters.Measurements.PSD.get(vecrData);
    bool etsi = Parameters.Measurements.bETSIPSD;
    Parameters.Unlock();

    if(psdOk==false)
        return;

    if(etsi) // if the RSCI output is turned on we display that version
    {
        // starting frequency and frequency step as defined in TS 102 349
        // plot expects the scale values in kHz
        rStart = _REAL(-7.875) + VIRTUAL_INTERMED_FREQ/_REAL(1000.0);
        rStep =_REAL(0.1875);

    }
    else // Traditional Dream values
    {
        rStart = 0.0;
        rStep = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;
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
    vector<double>& groupDelay)
{
	vector<_COMPLEX> veccChanEst;

    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
    bool b = Parameters.Measurements.ChannelEstimate.get(veccChanEst);
    int iFFTSizeN = Parameters.CellMappingTable.iFFTSizeN;
    Parameters.Unlock();

    if(b==false || veccChanEst.size()==0)
        return; // not running yet

    int iNumCarrier = veccChanEst.size();

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

bool CPlotManager::GetAvPoDeSp(CMeasurements::CPIR& pir)
{
	CParameter& Parameters = *pReceiver->GetParameters();

    // read it from the parameter structure
    Parameters.Lock();
    bool b = Parameters.Measurements.PIR.get(pir);
    Parameters.Unlock();
    return b;
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

void CPlotManager::GetInputSpectrum(vector<_REAL>& vecrData, _REAL& rStart, _REAL& rStep)
{
    CParameter& Parameters = *pReceiver->GetParameters();
    Parameters.Lock();
    Parameters.Measurements.inputSpectrum.get(vecrData);
    Parameters.Unlock();
    rStart = 0.0;
    rStep = _REAL(SOUNDCRD_SAMPLE_RATE) / _REAL(vecrData.size()) / 2000.0;

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

void CPlotManager::GetSDCVectorSpace(vector<_COMPLEX>& vec)
{
    pReceiver->GetSDCVectorSpace(vec);
}

void CPlotManager::GetMSCVectorSpace(vector<_COMPLEX>& vec)
{
    pReceiver->GetMSCVectorSpace(vec);
}

ECodScheme CPlotManager::GetSDCCodingScheme()
{
    return pReceiver->GetParameters()->eSDCCodingScheme;
}

ECodScheme CPlotManager::GetMSCCodingScheme()
{
    return pReceiver->GetParameters()->eMSCCodingScheme;
}

_REAL CPlotManager::GetDCFrequency()
{
    return pReceiver->GetParameters()->GetDCFrequency();
}
