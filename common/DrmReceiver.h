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
#include <iostream>
#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "MDI/MDIDecode.h"
#include "Parameter.h"
#include "util/Buffer.h"
#include "util/Utilities.h"
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
	void					Init();
	void					Start();
	void					Stop();
	EAcqStat				GetReceiverState() {return eAcquiState;}
	ERecMode				GetReceiverMode() {return eReceiverMode;}
	void					SetReceiverMode(ERecMode eNewMode)
								{eNewReceiverMode = eNewMode;}

	void					SetInitResOff(_REAL rNRO)
								{rInitResampleOffset = rNRO;}

	void					SetAMDemodAcq(_REAL rNewNorCen);
	_BOOLEAN 				SetFrequency(int iNewFreqkHz);
	int		 				GetFrequency() { return iFreqkHz; }

	void SetRSIRecording(const _BOOLEAN bOn, const char cPro);
	void SetIQRecording(const _BOOLEAN bOn);

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

	void GetTransferFunction(CVector<_REAL>& vecrData,
		CVector<_REAL>& vecrGrpDly,	CVector<_REAL>& vecrScale)
		{ChannelEstimation.GetTransferFunction(vecrData, vecrGrpDly, vecrScale);}

	void GetAvPoDeSp(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
					 _REAL& rLowerBound, _REAL& rHigherBound,
					 _REAL& rStartGuard, _REAL& rEndGuard, _REAL& rPDSBegin,
					 _REAL& rPDSEnd)
		{ChannelEstimation.GetAvPoDeSp(vecrData, vecrScale, rLowerBound, rHigherBound,
		rStartGuard, rEndGuard, rPDSBegin, rPDSEnd);}

	void GetSNRProfile(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale)
		{ChannelEstimation.GetSNRProfile(vecrData, vecrScale);}

#ifdef _WIN32
	void SetEnableProcessPriority(_BOOLEAN bValue)
		{bProcessPriorityEnabled = bValue;}

	_BOOLEAN GetEnableProcessPriority()
		{return bProcessPriorityEnabled;}
#endif

	/* Get pointer to internal modules */
	CSelectionInterface*	GetSoundInInterface() {return pSoundInInterface;}
	CSelectionInterface*	GetSoundOutInterface() {return pSoundOutInterface;}
	CUtilizeFACData*		GetFAC() {return &UtilizeFACData;}
	CUtilizeSDCData*		GetSDC() {return &UtilizeSDCData;}
	CTimeSync*				GetTimeSync() {return &TimeSync;}
	CFACMLCDecoder*			GetFACMLC() {return &FACMLCDecoder;}
	CSDCMLCDecoder*			GetSDCMLC() {return &SDCMLCDecoder;}
	CMSCMLCDecoder*			GetMSCMLC() {return &MSCMLCDecoder;}
	CReceiveData*			GetReceiver() {return &ReceiveData;}
	COFDMDemodulation*		GetOFDMDemod() {return &OFDMDemodulation;}
	CSyncUsingPil*			GetSyncUsPil() {return &SyncUsingPil;}
	CWriteData*				GetWriteData() {return &WriteData;}
	CDataDecoder*			GetDataDecoder() {return &DataDecoder;}
	CAMDemodulation*		GetAMDemod() {return &AMDemodulation;}
	CAMSSPhaseDemod*		GetAMSSPhaseDemod() {return &AMSSPhaseDemod;}
	CAMSSDecode*			GetAMSSDecode() {return &AMSSDecode;}
	CFreqSyncAcq*			GetFreqSyncAcq() {return &FreqSyncAcq;}
	CAudioSourceDecoder*	GetAudSorceDec() {return &AudioSourceDecoder;}
	CRSIMDIInRCIOut*		GetRSIIn() {return &upstreamRSCI;}
	CRSIMDIOutRCIIn*		GetRSIOut() {return &downstreamRSCI;}
#ifdef HAVE_LIBHAMLIB
	CHamlib*				GetHamlib() {return &Hamlib;}
#endif
	_BOOLEAN				SignalStrengthAvailable() { return TRUE; }
	_BOOLEAN				GetSignalStrength(_REAL& rSigStr);

	CParameter*				GetParameters() {return &ReceiverParam;}
	void					StartParameters(CParameter& Param);
	void					SetInStartMode();
	void					SetInTrackingMode();
	void					SetInTrackingModeDelayed();

	void					SetReadDRMFromFile(const string strNFN);

	void					InitsForAllModules();

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


	/* Interfaces to internal parameters/vectors used for the plot */
	void GetFreqSamOffsHist(CVector<_REAL>& vecrFreqOffs,
		CVector<_REAL>& vecrSamOffs, CVector<_REAL>& vecrScale,
		_REAL& rFreqAquVal);

	void GetDopplerDelHist(CVector<_REAL>& vecrLenIR,
		CVector<_REAL>& vecrDoppler, CVector<_REAL>& vecrScale);

	void GetSNRHist(CVector<_REAL>& vecrSNR, CVector<_REAL>& vecrCDAud,
		CVector<_REAL>& vecrScale);

protected:
	void					Run();
	void					DetectAcquiFAC();
	void					DetectAcquiSymbol();
	void					InitReceiverMode();
	void					UpdateParamHistories();

	/* Modules */
	CSoundInInterface*		pSoundInInterface;
	CSoundOutInterface*		pSoundOutInterface;
	CReceiveData			ReceiveData;
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
	CSplitFAC				SplitFAC;
	CSplitSDC				SplitSDC;
	CSplitMSC				SplitMSC[MAX_NUM_STREAMS];
	CAMDemodulation			AMDemodulation;
	CAMSSPhaseDemod			AMSSPhaseDemod;
	CAMSSExtractBits		AMSSExtractBits;
	CAMSSDecode				AMSSDecode;

	CRSIMDIInRCIOut			upstreamRSCI;
	CDecodeRSIMDI			DecodeRSIMDI;
	CRSIMDIOutRCIIn			downstreamRSCI;

	/* Parameters */
	CParameter				ReceiverParam;

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
	CCyclicBuffer<_SAMPLE>			AudSoDecBuf;

	EAcqStat				eAcquiState;
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

	_REAL					rInitResampleOffset;

#ifdef HAVE_LIBHAMLIB
	CHamlib					Hamlib;
#endif

	/* Storing parameters for plot */
	CShiftRegister<_REAL>	vecrFreqSyncValHist;
	CShiftRegister<_REAL>	vecrSamOffsValHist;
	CShiftRegister<_REAL>	vecrLenIRHist;
	CShiftRegister<_REAL>	vecrDopplerHist;
	CShiftRegister<_REAL>	vecrSNRHist;
	CShiftRegister<int>		veciCDAudHist;
	int						iAvCntParamHist;
	_REAL					rAvLenIRHist;
	_REAL					rAvDopplerHist;
	_REAL					rAvSNRHist;
	int						iCurrentCDAud;
	CMutex					MutexHist;
	CVectorEx<_BINARY>		vecbiMostRecentSDC;
	int						iFreqkHz;

	/* number of frames without FAC data before generating free-running RSCI */
	static const int		MAX_UNLOCKED_COUNT;

	/* Counter for unlocked frames, to keep generating RSCI even when unlocked */
	int						iUnlockedCount;

#ifdef USE_QT_GUI
	class CRigPoll : public QThread
	{
	public:
		CRigPoll():pDrmRec(NULL),bQuit(FALSE){ }
		virtual void	run();
		virtual void	stop(){bQuit=TRUE;}
		void setReceiver(CDRMReceiver* pRx){pDrmRec=pRx;}
	protected:
			CDRMReceiver* pDrmRec;
			_BOOLEAN	bQuit;
	} RigPoll;
#endif

public:
	_BOOLEAN				bEnableSMeter;

	/* Analog demodulation settings */
	int						iBwAM;
	int						iBwLSB;
	int						iBwUSB;
	int						iBwCW;
	int						iBwFM;
	CAMDemodulation::EDemodType	AMDemodType;

#ifdef _WIN32
	_BOOLEAN				bProcessPriorityEnabled;
#endif
	_BOOLEAN				bReadFromFile;
	time_t					time_keeper;

/* _WIN32 check because in Visual c++ the GUI files are always compiled even
   if USE_QT_GUI is set or not */
#if defined(USE_QT_GUI) || defined(_WIN32)
	/* DRMReceiver object serves as a storage a "exchange platform" for the
	   window size and position parameters for init-file usage. This is not
	   nice but it works for now. TODO: better solution */
	class CWinGeom
	{
	public:
		CWinGeom() : iXPos(0), iYPos(0), iHSize(0), iWSize(0),
			bVisible(FALSE), iType(0) {}

		int iXPos, iYPos;
		int iHSize, iWSize;
		_BOOLEAN bVisible;
		int iType;
	};

	/* Main window */
	CWinGeom GeomFdrmdialog;

	/* System evaluation window */
	CWinGeom GeomSystemEvalDlg;

	/* Multimedia window */
	CWinGeom GeomMultimediaDlg;

	/* Stations dialog */
	CWinGeom GeomStationsDlg;

	/* Live schedule */
	CWinGeom GeomLiveScheduleDlg;

	/* Analog demodulation dialog */
	CWinGeom GeomAnalogDemDlg;
	

	/* Analog demodulation dialog */
	CWinGeom GeomAMSSDlg;

	/* Chart windows */
	CVector<CWinGeom> GeomChartWindows;

	/* EPG Programme Guide */
	CWinGeom GeomEPGDlg;

	int			iMainPlotColorStyle;
	int			iSecondsPreview;

	/* Parameters for live schedule dialog */
	int			iSecondsPreviewLiveSched;
	_BOOLEAN	bShowAllStations;

	/* Sort parameters for stations dialog */
	class CSortParam
	{
	public:
		CSortParam(const int iCol,const _BOOLEAN bAsc) :
			iColumn(iCol), bAscending(bAsc) {}

		int			iColumn;
		_BOOLEAN	bAscending;
	};

	/* Analog sort parameter in stations dialog */	
	CSortParam SortParamAnalog;

	/* DRM sort parameter in stations dialog */
	CSortParam SortParamDRM;

	/* sort parameter in live schedule dialog */
	CSortParam SortParamLiveSched;


	int				iSysEvalDlgPlotType;
	int				iMOTBWSRefreshTime;
	_BOOLEAN		bAddRefreshHeader;
	string			strStoragePathMMDlg;
	string			strStoragePathLiveScheduleDlg;
	int				iMainDisplayColor;

	/* Font parameters for Multimedia Dlg */
	class CFontParam
	{
	public:
		CFontParam(const string strFontFamily, const int intFontPointSize,
			const int intFontWeight , const _BOOLEAN bFontItalic) :
			strFamily(strFontFamily), intPointSize(intFontPointSize),
			intWeight(intFontWeight), bItalic(bFontItalic) {}

		string		strFamily;
		int			intPointSize;
		int			intWeight;
		_BOOLEAN	bItalic;
	};

	CFontParam		FontParamMMDlg;
#endif

};


#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
