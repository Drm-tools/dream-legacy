/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmReceiver.cpp
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

#include <iostream>
#include "Parameter.h"
#include "Buffer.h"
#include "Data.h"
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
#ifdef _WIN32
# include "../../Windows/source/sound.h"
#else
# include "source/sound.h"
#endif


/* Definitions ****************************************************************/
/* No of FAC frames until the acquisition is activated in case a signal
   was successfully decoded */
#define	NO_FAC_FRA_U_ACQ_WITH			12

/* No of FAC frames until the acquisition is activated in case no signal
   could be decoded after previous acquisition try */
#define	NO_FAC_FRA_U_ACQ_WITHOUT		7


/* Classes ********************************************************************/
class CDRMReceiver
{
public:
	/* Acquisition state of receiver */
	enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL};

	/* Receiver state */
	enum ERecState {RS_TRACKING, RS_ACQUISITION};

	/* RM: Receiver mode (analog or digital demodulation) */
	enum ERecMode {RM_DRM, RM_AM, RM_NONE};


	CDRMReceiver() : eAcquiState(AS_NO_SIGNAL), iAcquDetecCnt(0),
		iGoodSignCnt(0), bWasFreqAcqu(TRUE), bDoInitRun(FALSE),
		eReceiverMode(RM_DRM), 	eNewReceiverMode(RM_NONE),
		ReceiveData(&SoundInterface), WriteData(&SoundInterface) {}
	virtual ~CDRMReceiver() {}

	/* For GUI */
	void					Init();
	void					Start();
	void					Stop();
	EAcqStat				GetReceiverState() {return eAcquiState;}
	ERecMode				GetReceiverMode() {return eReceiverMode;}
	void					SetReceiverMode(ERecMode eNewMode)
								{eNewReceiverMode = eNewMode;}

	/* Get pointer to internal modules */
	CUtilizeFACData*		GetFAC() {return &UtilizeFACData;}
	CUtilizeSDCData*		GetSDC() {return &UtilizeSDCData;}
	CTimeSync*				GetTimeSync() {return &TimeSync;}
	CChannelEstimation*		GetChanEst() {return &ChannelEstimation;}
	CFACMLCDecoder*			GetFACMLC() {return &FACMLCDecoder;}
	CSDCMLCDecoder*			GetSDCMLC() {return &SDCMLCDecoder;}
	CMSCMLCDecoder*			GetMSCMLC() {return &MSCMLCDecoder;}
	CReceiveData*			GetReceiver() {return &ReceiveData;}
	COFDMDemodulation*		GetOFDMDemod() {return &OFDMDemodulation;}
	CSyncUsingPil*			GetSyncUsPil() {return &SyncUsingPil;}
	CWriteData*				GetWriteData() {return &WriteData;}
	CSound*					GetSoundInterface() {return &SoundInterface;}


	CParameter*				GetParameters() {return &ReceiverParam;}
	void					StartParameters(CParameter& Param);
	void					SetInStartMode();
	void					SetInTrackingMode();


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


protected:
	void					Run();
	void					DetectAcqui();
	void					InitReceiverMode();

	/* Modules */
	CReceiveData			ReceiveData;
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
	CWriteData				WriteData;
	CAMDemodulation			AMDemodulation;

	/* Parameters */
	CParameter				ReceiverParam;

	/* Buffers */
	CCyclicBuffer<_REAL>	RecDataBuf;
	CSingleBuffer<_REAL>	FreqSyncAcqBuf;
	CCyclicBuffer<_REAL>	InpResBuf;
	CSingleBuffer<_REAL>	TimeSyncBuf;
	CSingleBuffer<_COMPLEX>	OFDMDemodBuf;
	CSingleBuffer<_COMPLEX>	SyncUsingPilBuf;
	CSingleBuffer<CEquSig>	ChanEstBuf;
	CCyclicBuffer<CEquSig>	MSCCarDemapBuf;
	CCyclicBuffer<CEquSig>	FACCarDemapBuf;
	CCyclicBuffer<CEquSig>	SDCCarDemapBuf;
	CSingleBuffer<CEquSig>	DeintlBuf;
	CSingleBuffer<_BINARY>	FACDecBuf;
	CSingleBuffer<_BINARY>	SDCDecBuf;
	CSingleBuffer<_BINARY>	MSCMLCDecBuf;
	CSingleBuffer<_BINARY>	MSCDeMUXBufAud;
	CSingleBuffer<_BINARY>	MSCDeMUXBufData;
	CCyclicBuffer<_SAMPLE>	AudSoDecBuf;

	EAcqStat				eAcquiState;
	int						iAcquDetecCnt;
	int						iGoodSignCnt;
	ERecState				eReceiverState;
	ERecMode				eReceiverMode;
	ERecMode				eNewReceiverMode;

	CSound					SoundInterface;

	_BOOLEAN				bWasFreqAcqu;
	_BOOLEAN				bDoInitRun;
};


#endif // !defined(DRMRECEIVER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
