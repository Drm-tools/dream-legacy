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

#include "ReceiverInterface.h"
#include "util/ReceiverModul_impl.h"
#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "MDI/MDIDecode.h"
#include "DataIO.h"
#include "OFDM.h"
#include "DRMSignalIO.h"
#include "MSCMultiplexer.h"
#include "InputResample.h"
#include "datadecoding/DataDecoder.h"
#include "sourcedecoders/AudioSourceDecoder.h"
#include "sourcedecoders/AudioSourceEncoder.h"
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

#ifdef USE_QT_GUI
# include <QThread>
# include <QMutex>
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


/* Classes ********************************************************************/
class CSettings;
class CHamlib;
class CRigCaps;

template<typename T>
struct Request
{
    Request(const T& v) { current = wanted = v; }
    T current;
    T wanted;
};

enum EInpTy { SoundCard, Dummy, Shm, File, RSCI };
enum ERecState {RS_TRACKING, RS_ACQUISITION};

class CSplitFAC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;}
};

class CSplitSDC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.iNumSDCBitsPerSuperFrame;}
};

class CSplitMSC : public CSplitModul<_BINARY>
{
public:
	void SetStream(int iID) {iStreamID = iID;}

protected:
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.GetStreamLen(iStreamID) * BITS_BINARY;}

	int iStreamID;
};

class CSplitAudio : public CSplitModul<_SAMPLE>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE * (_REAL) 0.4 /* 400 ms */) * 2 /* stereo */;}
};

class CDRMReceiver : public ReceiverInterface
#ifdef USE_QT_GUI
	, public QThread
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
	//void					Init();
	void					Start();
	void					Stop();
	bool                    End();
	EAcqStat				GetAcquiState() {return Parameters.eAcquiState;}
	void					SetInitResOff(_REAL rNRO)
								{rInitResampleOffset = rNRO;}

	void					SetAnalogDemodType(EModulationType);
	EModulationType		    GetAnalogDemodType() { return AMDemodulation.GetDemodType(); }
	int						GetAnalogFilterBWHz();
	void					SetAnalogFilterBWHz(int);
	void					SetAnalogFilterBWHz(EModulationType, int);

	void					SetAnalogDemodAcq(_REAL rNewNorCen);

	void					EnableAnalogAutoFreqAcq(const bool bNewEn);
	bool					AnalogAutoFreqAcqEnabled();

	void					EnableAnalogPLL(const bool bNewEn);
	bool					AnalogPLLEnabled();
	bool					GetAnalogPLLPhase(CReal& rPhaseOut);

	void					SetAnalogAGCType(const EType eNewType);
	EType				    GetAnalogAGCType();
	void					SetAnalogNoiseReductionType(const ENoiRedType eNewType);
	ENoiRedType             GetAnalogNoiseReductionType();

	void					SetUseAnalogHWDemod(bool);
	bool					GetUseAnalogHWDemod();

	void	 				SetEnableSMeter(bool bNew);
	bool		 			GetEnableSMeter();
	bool 					SetFrequency(int iNewFreqkHz);
	int		 				GetFrequency() { return iFreqkHz; }
	void					SetIQRecording(bool);
	void					SetRSIRecording(bool, const char);
	void					SetHamlib(CHamlib*);
	bool				    UpstreamDIInputEnabled() { return upstreamRSCI.GetInEnabled(); }

	void					SetReadPCMFromFile(const string strNFN);

	/* Channel Estimation */
	void SetFreqInt(ETypeIntFreq eNewTy)
		{ChannelEstimation.SetFreqInt(eNewTy);}

	ETypeIntFreq GetFreqInt()
		{return ChannelEstimation.GetFreqInt();}

	void SetTimeInt(ETypeIntTime eNewTy)
		{ChannelEstimation.SetTimeInt(eNewTy);}

	ETypeIntTime GetTimeInt() const
		{return ChannelEstimation.GetTimeInt();}

	void SetIntCons(const bool bNewIntCons)
		{ChannelEstimation.SetIntCons(bNewIntCons);}

	bool GetIntCons()
		{return ChannelEstimation.GetIntCons();}

	void SetSNREst(ETypeSNREst eNewTy)
		{ChannelEstimation.SetSNREst(eNewTy);}

	ETypeSNREst GetSNREst()
		{return ChannelEstimation.GetSNREst();}

	void SetTiSyncTracType(ETypeTiSyncTrac eNewTy)
     {ChannelEstimation.GetTimeSyncTrack()->SetTiSyncTracType(eNewTy);}

	ETypeTiSyncTrac GetTiSyncTracType()
		{return ChannelEstimation.GetTimeSyncTrack()->GetTiSyncTracType();}

	int GetInitNumIterations()
		{ return MSCMLCDecoder.GetInitNumIterations(); }
	void SetNumIterations(int value)
		{ MSCMLCDecoder.SetNumIterations(value); }

	bool GetRecFilter() { return FreqSyncAcq.GetRecFilter(); }
	void SetRecFilter(bool bVal) { FreqSyncAcq.SetRecFilter(bVal); }

	void SetFlippedSpectrum(bool bNewF) {ReceiveData.SetFlippedSpectrum(bNewF);}
	bool GetFlippedSpectrum() {return ReceiveData.GetFlippedSpectrum();}

	bool GetReverbEffect() { return AudioSourceDecoder.GetReverbEffect(); }
	void SetReverbEffect(bool bVal)
		{ AudioSourceDecoder.SetReverbEffect(bVal); }

	void MuteAudio(bool bVal) { WriteData.MuteAudio(bVal); }
	bool GetMuteAudio() { return WriteData.GetMuteAudio(); }
	void StartWriteWaveFile(const string& strFile)
		{ WriteData.StartWriteWaveFile(strFile); }
	void StopWriteWaveFile() { WriteData.StopWriteWaveFile(); }
	bool GetIsWriteWaveFile() { return WriteData.GetIsWriteWaveFile(); }


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

	void					SetRigModelForAllModes(int);
	void					SetRigModel(int);
	int						GetRigModel() const;
	void					GetRigList(map<int, CRigCaps>&) const;
	void					GetRigCaps(CRigCaps&) const;
	void					GetRigCaps(int, CRigCaps&) const;
	void					GetComPortList(map<string,string>& ports) const;
	string					GetRigComPort() const;
	void					SetRigComPort(const string&);
	bool				    GetRigChangeInProgress();
	CParameter*				GetParameters() {return &Parameters;}

	void					InitReceiverMode(EModulationType);
	void					InitsForAllModules(EModulationType);
	void					InitsForWaveMode();
	void					InitsForSpectrumOccup();
	void					InitsForNoDecBitsSDC();
	void					InitsForAudParam();
	void					InitsForDataParam();
	void					InitsForInterlDepth();
	void					InitsForMSCCodSche();
	void					InitsForSDCCodSche();

protected:

    void                    UpdateHamlibAndSoundInput();
	bool					doSetFrequency();
	void					SetInStartMode();
	void					SetInTrackingMode();
	void					SetInTrackingModeDelayed();
	void					Run();
	void					DemodulateDRM(bool&);
	void					DecodeDRM(bool&, bool&);
	void					UtilizeDRM(bool&);
	void					DemodulateAM(bool&);
	void					DecodeAM(bool&);
	void					UtilizeAM(bool&);
	void					DetectAcquiFAC();
	void					DetectAcquiSymbol();
	void					saveSDCtoFile();

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
	CParameter				Parameters;

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
	int                     iAudioStreamID;
	int                     iDataStreamID; // or more than one?

	bool					bDoInitRun;
	bool					bRunning;

	_REAL					rInitResampleOffset;

	CVectorEx<_BINARY>		vecbiMostRecentSDC;
	int						iFreqkHz;

	/* number of frames without FAC data before generating free-running RSCI */
	static const int		MAX_UNLOCKED_COUNT;

	/* Counter for unlocked frames, to keep generating RSCI even when unlocked */
	int						iUnlockedCount;
	time_t					time_keeper;
	Request<bool>           onBoardDemod;
	Request<EInpTy>         pcmInput;
	Request<int>            soundDev;
	Request<int>            rig;
	Request<EModulationType> rigMode;
	Request<EInChanSel>     chanSel;
	string                  strPCMFile;
	bool                    bRigUpdateNeeded;
	bool                    bRigUpdateForAllModes;
    CHamlib*                pHamlib;
};

#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
