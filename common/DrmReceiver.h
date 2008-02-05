/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Andrea Russo, Oliver Haffenden
 *
 * Description:
 *	See DrmReceiver.cpp
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include AMSS demodulation
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

#if !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "util/ReceiverModul_impl.h"
#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "MDI/MDIDecode.h"
#ifdef HAVE_LIBHAMLIB
#include "util/Hamlib.h"
#endif
#include "DataIO.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "MSCMultiplexer.h"
#include "InputResample.h"
#include "datadecoding/DataDecoder.h"
#include "sourcedecoders/AudioSourceDecoder.h"
#include "mlc/MLC.h"
#include "interleaver/SymbolInterleaver.h"
#include "ofdmcellmapping/OFDMCellMapping.h"
#include "chanest/ChannelEstimation.h"
#include "sync/FreqSyncAcq.h"
#include "sync/TimeSync.h"
#include "sync/SyncUsingPil.h"
#include "AMDemodulation.h"
#include "AMSSDemodulation.h"
#include "soundinterface.h"
#include "PlotManager.h"

#ifdef USE_QT_GUI
# include <qthread.h>
# if QT_VERSION >= 0x030000
#  include <qmutex.h>
# endif
#endif

/* Definitions ****************************************************************/
/* Number of FAC frames until the acquisition is activated in case a signal
   was successfully decoded */
#define	NUM_FAC_FRA_U_ACQ_WITH			10

/* Number of OFDM symbols until the acquisition is activated in case no signal
   could be decoded after previous acquisition try */
#define	NUM_OFDMSYM_U_ACQ_WITHOUT		150

/* Number of FAC blocks for delayed tracking mode switch (caused by time needed
   for initalizing the channel estimation */
#define NUM_FAC_DEL_TRACK_SWITCH		2

/* Length of the history for synchronization parameters (used for the plot) */
#define LEN_HIST_PLOT_SYNC_PARMS		2250


/* Classes ********************************************************************/
class CSettings;
class CHamlib;

class CRequestStateChange
{
public:
	CRequestStateChange():pending(false),new_value(0) {}
	void request(const int val) { pending=true; new_value=val; }
	bool is_pending() { return pending; }
	int value() { pending=false; return new_value; }
protected:
	bool pending;
	int  new_value;
};

class CSplitFAC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;}
};

class CSplitSDC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.iNumSDCBitsPerSFrame;}
};

class CSplitMSC : public CSplitModul<_BINARY>
{
public:
	void SetStream(int iID) {iStreamID = iID;}

protected:
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.GetStreamLen(iStreamID) * SIZEOF__BYTE;}

	int iStreamID;
};

class CSplitAudio : public CSplitModul<_SAMPLE>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE * (_REAL) 0.4 /* 400 ms */) * 2 /* stereo */;}
};

class CDRMReceiver
#ifdef USE_QT_GUI
	: public QThread
#endif
{
public:

	CDRMReceiver();
	virtual ~CDRMReceiver();

	/* For GUI */
#ifdef USE_QT_GUI
	virtual void			run();
#else /* keep the windows builds happy when compiling without the GUI */
	int						wait(int) { return 0;}
	bool					finished() { return true; }
#endif
	void					LoadSettings(CSettings&); // can write to settings to set defaults
	void					SaveSettings(CSettings&);
	void					Init();
	void					Start();
	void					Stop();
	void					RequestNewAcquisition() { bRestartFlag = TRUE; }
	EAcqStat				GetAcquiState() {return Parameters.eAcquiState;}
	ERecMode				GetReceiverMode() {return eReceiverMode;}
	void					SetReceiverMode(ERecMode eNewMode);
	void					SetInitResOff(_REAL rNRO)
								{rInitResampleOffset = rNRO;}

	CReal					GetAnalogCurMixFreqOffs() const { return AMDemodulation.GetCurMixFreqOffs(); }
	void					SetAnalogDemodType(EDemodType);
	EDemodType				GetAnalogDemodType() { return AMDemodulation.GetDemodType(); }
	int						GetAnalogFilterBWHz();
	void					SetAnalogFilterBWHz(int);

	void					SetAnalogDemodAcq(_REAL rNewNorCen);

	void					EnableAnalogAutoFreqAcq(const _BOOLEAN bNewEn);
	_BOOLEAN				AnalogAutoFreqAcqEnabled();

	void					EnableAnalogPLL(const _BOOLEAN bNewEn);
	_BOOLEAN				AnalogPLLEnabled();
	_BOOLEAN				GetAnalogPLLPhase(CReal& rPhaseOut);

	void					SetAnalogAGCType(const CAGC::EType eNewType);
	CAGC::EType				GetAnalogAGCType();
	void					SetAnalogNoiseReductionType(const CAMDemodulation::ENoiRedType eNewType);
	CAMDemodulation::ENoiRedType GetAnalogNoiseReductionType();
	void					GetAnalogBWParameters(CReal& rCenterFreq, CReal& rBW);

	void					SetUseAnalogHWDemod(_BOOLEAN);
	_BOOLEAN				GetUseAnalogHWDemod();

	void	 				SetEnableSMeter(_BOOLEAN bNew);
	_BOOLEAN		 		GetEnableSMeter();
	_BOOLEAN 				SetFrequency(int iNewFreqkHz);
	int		 				GetFrequency() { return iFreqkHz; }
	void					SetIQRecording(_BOOLEAN);
	void					SetRSIRecording(_BOOLEAN, const char);
	void					SetHamlib(CHamlib*);

	void					UpdateSoundIn();
	void					SetReadPCMFromFile(const string strNFN);

	/* Channel Estimation */
	void SetFreqInt(CChannelEstimation::ETypeIntFreq eNewTy) 
		{ChannelEstimation.SetFreqInt(eNewTy);}

	CChannelEstimation::ETypeIntFreq GetFreqInt()
		{return ChannelEstimation.GetFreqInt();}

	void SetTimeInt(CChannelEstimation::ETypeIntTime eNewTy) 
		{ChannelEstimation.SetTimeInt(eNewTy);}

	CChannelEstimation::ETypeIntTime GetTimeInt() const 
		{return ChannelEstimation.GetTimeInt();}

	void SetIntCons(const _BOOLEAN bNewIntCons) 
		{ChannelEstimation.SetIntCons(bNewIntCons);}

	_BOOLEAN GetIntCons()
		{return ChannelEstimation.GetIntCons();}

	void SetSNREst(CChannelEstimation::ETypeSNREst eNewTy)
		{ChannelEstimation.SetSNREst(eNewTy);}

	CChannelEstimation::ETypeSNREst GetSNREst() 
		{return ChannelEstimation.GetSNREst();}

	void SetTiSyncTracType(CTimeSyncTrack::ETypeTiSyncTrac eNewTy)
     {ChannelEstimation.GetTimeSyncTrack()->SetTiSyncTracType(eNewTy);}

	CTimeSyncTrack::ETypeTiSyncTrac GetTiSyncTracType()
		{return ChannelEstimation.GetTimeSyncTrack()->GetTiSyncTracType();}

	int GetInitNumIterations()
		{ return MSCMLCDecoder.GetInitNumIterations(); }
	void SetNumIterations(int value)
		{ MSCMLCDecoder.SetNumIterations(value); }
	
	_BOOLEAN GetRecFilter() { return FreqSyncAcq.GetRecFilter(); }
	void SetRecFilter(_BOOLEAN bVal) { FreqSyncAcq.SetRecFilter(bVal); }

	_BOOLEAN GetFlippedSpectrum() { return ReceiveData.GetFlippedSpectrum(); }
	void SetFlippedSpectrum(_BOOLEAN bVal)
		{ ReceiveData.SetFlippedSpectrum(bVal); }
	void GetInputPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
		{ ReceiveData.GetInputPSD(vecrData, vecrScale); }
	void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
		{ ReceiveData.GetInputSpec(vecrData, vecrScale); }
	void GetAudioSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
		{ WriteData.GetAudioSpec(vecrData, vecrScale); }

	void GetFACVectorSpace(CVector<_COMPLEX>& vec)
		{ FACMLCDecoder.GetVectorSpace(vec); }

	void GetSDCVectorSpace(CVector<_COMPLEX>& vec)
		{ SDCMLCDecoder.GetVectorSpace(vec); }

	void GetMSCVectorSpace(CVector<_COMPLEX>& vec)
		{ MSCMLCDecoder.GetVectorSpace(vec); }

	_BOOLEAN GetReverbEffect() { return AudioSourceDecoder.GetReverbEffect(); }
	void SetReverbEffect(_BOOLEAN bVal)
		{ AudioSourceDecoder.SetReverbEffect(bVal); }

	void MuteAudio(_BOOLEAN bVal) { WriteData.MuteAudio(bVal); }
	_BOOLEAN GetMuteAudio() { return WriteData.GetMuteAudio(); }
	void StartWriteWaveFile(const string& strFile)
		{ WriteData.StartWriteWaveFile(strFile); }
	void StopWriteWaveFile() { WriteData.StopWriteWaveFile(); }
	_BOOLEAN GetIsWriteWaveFile() { return WriteData.GetIsWriteWaveFile(); }


	/* Get pointer to internal modules */
	CSelectionInterface*	GetSoundInInterface() {return pSoundInInterface;}
	CSelectionInterface*	GetSoundOutInterface() {return pSoundOutInterface;}
	CUtilizeFACData*		GetFAC() {return &UtilizeFACData;}
	CUtilizeSDCData*		GetSDC() {return &UtilizeSDCData;}
	CTimeSync*				GetTimeSync() {return &TimeSync;}
	COFDMDemodulation*		GetOFDMDemod() {return &OFDMDemodulation;}
	CSyncUsingPil*			GetSyncUsPil() {return &SyncUsingPil;}
	CDataDecoder*			GetDataDecoder() {return &DataDecoder;}
	CAMSSPhaseDemod*		GetAMSSPhaseDemod() {return &AMSSPhaseDemod;}
	CAMSSDecode*			GetAMSSDecode() {return &AMSSDecode;}
	CFreqSyncAcq*			GetFreqSyncAcq() {return &FreqSyncAcq;}
	CAudioSourceDecoder*	GetAudSorceDec() {return &AudioSourceDecoder;}
	CUpstreamDI*			GetRSIIn() {return &upstreamRSCI;}
	CDownstreamDI*			GetRSIOut() {return &downstreamRSCI;}
	CChannelEstimation*		GetChannelEstimation() {return &ChannelEstimation;}
#ifdef HAVE_LIBHAMLIB
	void					SetRigModel(int);
	int						GetRigModel();
	void					GetRigList(map<int, CRigCaps>& rigs);
	void					GetRigCaps(CRigCaps& caps);
	void					GetComPortList(map<string,string>& ports);
	string					GetRigComPort();
	void					SetRigComPort(const string&);
	void					SetRigFreqOffset(int);
	_BOOLEAN				GetRigChangeInProgress() { return rigmodelrequest.is_pending()?TRUE:FALSE; }
#endif
	_BOOLEAN				GetSignalStrength(_REAL& rSigStr);

	CParameter*				GetParameters() {return &Parameters;}

	CPlotManager*			GetPlotManager() {return &PlotManager;}

	void					InitsForWaveMode();
	void					InitsForSpectrumOccup();
	void					InitsForNoDecBitsSDC();
	void					InitsForAudParam();
	void					InitsForDataParam();
	void					InitsForInterlDepth();
	void					InitsForMSCCodSche();
	void					InitsForSDCCodSche();
	void					InitsForMSC();
	void					InitsForMSCDemux();

protected:

	void					InitReceiverMode();
	_BOOLEAN				doSetFrequency();
	void					doSetRigModel(int);
	void					SetInStartMode();
	void					SetInTrackingMode();
	void					SetInTrackingModeDelayed();
	void					InitsForAllModules();
	void					Run();
	void					DemodulateDRM(_BOOLEAN&);
	void					DecodeDRM(_BOOLEAN&, _BOOLEAN&);
	void					UtilizeDRM(_BOOLEAN&);
	void					DemodulateAM(_BOOLEAN&);
	void					DecodeAM(_BOOLEAN&);
	void					UtilizeAM(_BOOLEAN&);
	void					DetectAcquiFAC();
	void					DetectAcquiSymbol();
	void					saveSDCtoFile();
#ifdef HAVE_LIBHAMLIB
	void					UpdateRigSettings();
#endif

	/* Modules */
	CSoundInInterface*		pSoundInInterface;
	CSoundOutInterface*		pSoundOutInterface;
	CReceiveData			ReceiveData;
	COnboardDecoder			OnboardDecoder;
	CWriteData				WriteData;
	CInputResample			InputResample;
	CFreqSyncAcq			FreqSyncAcq;
	CTimeSync				TimeSync;
	COFDMDemodulation		OFDMDemodulation;
	CSyncUsingPil			SyncUsingPil;
	CChannelEstimation		ChannelEstimation;
	COFDMCellDemapping		OFDMCellDemapping;
	CFACMLCDecoder			FACMLCDecoder;
	CUtilizeFACData			UtilizeFACData;
	CSDCMLCDecoder			SDCMLCDecoder;
	CUtilizeSDCData			UtilizeSDCData;
	CSymbDeinterleaver		SymbDeinterleaver;
	CMSCMLCDecoder			MSCMLCDecoder;
	CMSCDemultiplexer		MSCDemultiplexer;
	CAudioSourceDecoder		AudioSourceDecoder;
	CDataDecoder			DataDecoder;
	CSplit					Split;
	CSplit					SplitForIQRecord;
	CWriteIQFile			WriteIQFile;
	CSplitAudio				SplitAudio;
	CAudioSourceEncoderRx	AudioSourceEncoder; // For encoding the audio for RSI
	CSplitFAC				SplitFAC;
	CSplitSDC				SplitSDC;
	CSplitMSC				SplitMSC[MAX_NUM_STREAMS];
	CAMDemodulation			AMDemodulation;
	CAMSSPhaseDemod			AMSSPhaseDemod;
	CAMSSExtractBits		AMSSExtractBits;
	CAMSSDecode				AMSSDecode;

	CUpstreamDI				upstreamRSCI;
	CDecodeRSI				DecodeRSIMDI;
	CDownstreamDI			downstreamRSCI;

	/* Parameters */
	CParameter				DRMParameters;
	CParameter				AMParameters;
	CParameter&				Parameters;

	/* Buffers */
	CSingleBuffer<_REAL>			AMDataBuf;
	CSingleBuffer<_REAL>			AMSSDataBuf;
	CSingleBuffer<_REAL>			AMSSPhaseBuf;
	CCyclicBuffer<_REAL>			AMSSResPhaseBuf;
	CCyclicBuffer<_BINARY>			AMSSBitsBuf;

	CSingleBuffer<_REAL>			DemodDataBuf;
	CSingleBuffer<_REAL>			IQRecordDataBuf;

	CSingleBuffer<_REAL>			RecDataBuf;
	CCyclicBuffer<_REAL>			InpResBuf;
	CCyclicBuffer<_COMPLEX>			FreqSyncAcqBuf;
	CSingleBuffer<_COMPLEX>			TimeSyncBuf;
	CSingleBuffer<_COMPLEX>			OFDMDemodBuf;
	CSingleBuffer<_COMPLEX>			SyncUsingPilBuf;
	CSingleBuffer<CEquSig>			ChanEstBuf;
	CCyclicBuffer<CEquSig>			MSCCarDemapBuf;
	CCyclicBuffer<CEquSig>			FACCarDemapBuf;
	CCyclicBuffer<CEquSig>			SDCCarDemapBuf;
	CSingleBuffer<CEquSig>			DeintlBuf;
	CSingleBuffer<_BINARY>			FACDecBuf;
	CSingleBuffer<_BINARY>			FACUseBuf;
	CSingleBuffer<_BINARY>			FACSendBuf;
	CSingleBuffer<_BINARY>			SDCDecBuf;
	CSingleBuffer<_BINARY>			SDCUseBuf;
	CSingleBuffer<_BINARY>			SDCSendBuf;
	CSingleBuffer<_BINARY>			MSCMLCDecBuf;
	CSingleBuffer<_BINARY>			RSIPacketBuf;
	vector<CSingleBuffer<_BINARY> >	MSCDecBuf;
	vector<CSingleBuffer<_BINARY> >	MSCUseBuf;
	vector<CSingleBuffer<_BINARY> >	MSCSendBuf;
	CSingleBuffer<_BINARY>			EncAMAudioBuf;
	CCyclicBuffer<_SAMPLE>			AudSoDecBuf;
	CCyclicBuffer<_SAMPLE>			AMAudioBuf;
	CCyclicBuffer<_SAMPLE>			AMSoEncBuf; // For encoding

	int						iAcquRestartCnt;
	int						iAcquDetecCnt;
	int						iGoodSignCnt;
	int						iDelayedTrackModeCnt;
	ERecState				eReceiverState;
	ERecMode				eReceiverMode;
	ERecMode				eNewReceiverMode;

	int						iAudioStreamID;
	int						iDataStreamID;


	_BOOLEAN				bDoInitRun;
	_BOOLEAN				bRestartFlag;

	_REAL					rInitResampleOffset;

	CHamlib*				pHamlib;

	CVectorEx<_BINARY>		vecbiMostRecentSDC;
	int						iFreqkHz;

	/* number of frames without FAC data before generating free-running RSCI */
	static const int		MAX_UNLOCKED_COUNT;

	/* Counter for unlocked frames, to keep generating RSCI even when unlocked */
	int						iUnlockedCount;
	time_t					time_keeper;

	CPlotManager PlotManager;
	enum { SoundCard, Dummy, Shm, File }
							pcmInput;
	enum { onBoard, inSoftware }
							demodulation;
#ifdef HAVE_LIBHAMLIB
	CRequestStateChange		rigmodelrequest;
	_BOOLEAN                bRigUpdateNeeded;
#endif
};

#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
