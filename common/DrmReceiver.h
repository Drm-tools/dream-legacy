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

/* Length of the history for synchronization parameters (used for the plot) */
#define LEN_HIST_PLOT_SYNC_PARMS		2250


/* Classes ********************************************************************/
class CSettings;
class CHamlib;
class CRigCaps;

enum EInpTy { SoundCard, Dummy, Shm, File, RSCI };
enum ERecState {RS_TRACKING, RS_ACQUISITION};

class CSoundInProxy : public CSelectionInterface
{
public:
	CSoundInProxy();
	virtual 			~CSoundInProxy();
	virtual void		Enumerate(vector<string>&);
	virtual int			GetDev();
	virtual void		SetDev(int iNewDev);
	void				SetMode(EDemodulationType);
	void				SetHamlib(CHamlib*);
	void				SetRigModelForAllModes(int);
	void				SetRigModel(int);
	void				SetReadPCMFromFile(const string strNFN);
	void				SetUsingDI(const string strSource);
	void				Update();

protected:

	EInpTy				pcmInput, pcmWantedInput;
	int					iWantedSoundDev;
	int					iWantedRig;
	EDemodulationType	eWantedRigMode;
	bool			    bRigUpdateForAllModes;
	string				filename;
public:
	CSoundInInterface*	pSoundInInterface;
	bool				bRigUpdateNeeded;
	CHamlib*			pHamlib;
	CDRMReceiver*		pDrmRec;
#ifdef USE_QT_GUI
	QMutex				mutex;
#endif
	EInChanSel			eWantedChanSel;
	bool				bOnBoardDemod, bOnBoardDemodWanted;
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
		{this->iInputBlockSize = p.GetStreamLen(iStreamID) * BITS_BINARY;}

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
	void					RequestNewAcquisition() { bRestartFlag = true; }
	EAcqStat				GetAcquiState() {return Parameters.eAcquiState;}
	EDemodulationType		GetReceiverMode() {return eReceiverMode;}
	void					SetReceiverMode(EDemodulationType eNewMode);
	void					SetInitResOff(_REAL rNRO)
								{rInitResampleOffset = rNRO;}

	CReal					GetAnalogCurMixFreqOffs() const { return AMDemodulation.GetCurMixFreqOffs(); }
	void					SetAnalogDemodType(EDemodulationType);
	EDemodulationType		GetAnalogDemodType() { return AMDemodulation.GetDemodType(); }
	int						GetAnalogFilterBWHz();
	void					SetAnalogFilterBWHz(int);

	void					SetAnalogDemodAcq(_REAL rNewNorCen);

	void					EnableAnalogAutoFreqAcq(const bool bNewEn);
	bool					AnalogAutoFreqAcqEnabled();

	void					EnableAnalogPLL(const bool bNewEn);
	bool					AnalogPLLEnabled();
	bool					GetAnalogPLLPhase(CReal& rPhaseOut);

	void					SetAnalogAGCType(const CAGC::EType eNewType);
	CAGC::EType				GetAnalogAGCType();
	void					SetAnalogNoiseReductionType(const CAMDemodulation::ENoiRedType eNewType);
	CAMDemodulation::ENoiRedType GetAnalogNoiseReductionType();
	void					GetAnalogBWParameters(CReal& rCenterFreq, CReal& rBW);

	void					SetUseAnalogHWDemod(bool);
	bool					GetUseAnalogHWDemod();

	void	 				SetEnableSMeter(bool bNew);
	bool		 			GetEnableSMeter();
	bool 					SetFrequency(int iNewFreqkHz);
	int		 				GetFrequency() { return iFreqkHz; }
	void					SetIQRecording(bool);
	void					SetRSIRecording(bool, const char);
	void					SetHamlib(CHamlib*);

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

	void SetIntCons(const bool bNewIntCons)
		{ChannelEstimation.SetIntCons(bNewIntCons);}

	bool GetIntCons()
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

	bool GetRecFilter() { return FreqSyncAcq.GetRecFilter(); }
	void SetRecFilter(bool bVal) { FreqSyncAcq.SetRecFilter(bVal); }

	void SetFlippedSpectrum(bool bNewF) {ReceiveData.SetFlippedSpectrum(bNewF);}
	bool GetFlippedSpectrum() {return ReceiveData.GetFlippedSpectrum();}

	void GetAudioSpec(vector<_REAL>& vecrData, vector<_REAL>& vecrScale)
		{ WriteData.GetAudioSpec(vecrData, vecrScale); }

	void GetFACVectorSpace(vector<_COMPLEX>& vec)
		{ FACMLCDecoder.GetVectorSpace(vec); }

	void GetSDCVectorSpace(vector<_COMPLEX>& vec)
		{ SDCMLCDecoder.GetVectorSpace(vec); }

	void GetMSCVectorSpace(vector<_COMPLEX>& vec)
		{ MSCMLCDecoder.GetVectorSpace(vec); }

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
	CSelectionInterface*	GetSoundInInterface() {return &SoundInProxy;}
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
	bool					doSetFrequency();
	void					SetInStartMode();
	void					SetInTrackingMode();
	void					SetInTrackingModeDelayed();
	void					InitsForAllModules();
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
	CSoundInProxy			SoundInProxy;
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
	EDemodulationType		eReceiverMode;
	EDemodulationType		eNewReceiverMode;

	int						iAudioStreamID;
	int						iDataStreamID;


	bool					bDoInitRun;
	bool					bRestartFlag;

	_REAL					rInitResampleOffset;


	CVectorEx<_BINARY>		vecbiMostRecentSDC;
	int						iFreqkHz;

	/* number of frames without FAC data before generating free-running RSCI */
	static const int		MAX_UNLOCKED_COUNT;

	/* Counter for unlocked frames, to keep generating RSCI even when unlocked */
	int						iUnlockedCount;
	time_t					time_keeper;
};

#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
