/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	see MDIConcrete.cpp
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


#if !defined(MDI_CONCRETE_H_INCLUDED)
#define MDI_CONCRETE_H_INCLUDED

#include "MDI.h"

#ifdef USE_QT_GUI
#  include "PacketSocketQT.h"
#else
#  include "PacketSocketNull.h"
#endif


#include "../GlobalDefinitions.h"
#include "MDIDefinitions.h"
#include "MDITagItems.h"
#include "MDIInBuffer.h"
#include "../Parameter.h"
#include "../util/Vector.h"
#include "../util/Buffer.h"
#include "../util/CRC.h"

#include "MDITagPacketDecoder.h"
#include "TagPacketDecoderRSCIControl.h"
#include "TagPacketGenerator.h"



/* Definitions ****************************************************************/

class CDRMReceiver; /* forward declaration so it can contain a pointer back to the DRMReceiver */

/* Classes ********************************************************************/
class CMDIConcrete : public CMDI
{
public:
	CMDIConcrete();
	void SetDRMReceiver(CDRMReceiver *pRx);

	virtual ~CMDIConcrete() {}

	/* MDI out */
	void SetFACData(_BOOLEAN bFACOK, CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter);
	void SetSDCData(_BOOLEAN bSDCOK, CVectorEx<_BINARY>& vecbiSDCData);
	void SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData);
	void SetMERs(const _REAL rRMER, const _REAL rRWMM, const _REAL rRWMF);
	void SetRDEL(const CRealVector &vecrThresholds, const CRealVector &vecrIntervals);
	void SetRDOP(const CReal rDoppler);
	void SetInterference(const CReal rIntFreq, const CReal rINR, const CReal rICR);
	void SetAudioFrameStatus(_BOOLEAN bIsValid, CVector<_BINARY>& vecbiAudioStatus);

	void SendUnlockedFrame(CParameter& Parameter); /* called once per frame even if the Rx isn't synchronised */

	_BOOLEAN SetNetwOutAddr(const string strNewIPPort);
	_BOOLEAN GetMDIOutEnabled() {return bMDIOutEnabled;}

	void SetAFPktCRC(const _BOOLEAN bNAFPktCRC) {bUseAFCRC = bNAFPktCRC;}
	_BOOLEAN GetAFPktCRC() {return bUseAFCRC;}

	void SetProfile(const char cProf) {cProfile = cProf;}

	/* MDI in */
	ERobMode GetFACData(CVectorEx<_BINARY>& vecbiFACData);
	void GetSDCData(CVectorEx<_BINARY>& vecbiSDCData);
	void GetStreamData(CVectorEx<_BINARY>& vecbiStrData, const int iLen,
		const int iStrNum);
	_BOOLEAN SetNetwInPort(const int iPort);
	_BOOLEAN SetNetwInMcast(const string strNewIPIP);
	_BOOLEAN SetNetwInPortRSCI(const int iPort);
	_BOOLEAN GetMDIInEnabled() {return bMDIInEnabled;}

protected:
	_BOOLEAN					bSDCOutWasSet; // OPH - this flag was causing interaction between MDI in and out
	_BOOLEAN					bSDCInWasSet;

	
	/* MDI transmit --------------------------------------------------------- */


	
	void SetEnableMDIOut(const _BOOLEAN bNEnMOut) {bMDIOutEnabled = bNEnMOut;}

	void SendPacket(CVector<_BINARY> vecbiPacket);

	CVector<_BINARY> GenMDIPacket(const _BOOLEAN bWithSDC);

	void ResetTags(const _BOOLEAN bResetSDC);

	uint32_t					iLogFraCnt;

	_BOOLEAN					bMDIOutEnabled;
	_BOOLEAN					bMDIInEnabled;

	/* Counter for unlocked frames, to keep generating RSCI even when unlocked */
	int iUnlockedCount;

	/* Flags used for generating the rsta tag */
	CRSCIStatusFlags  RSCIStatusFlags;

	/* Generators for all of the MDI and RSCI tags */

	CTagItemGeneratorProTyMDI TagItemGeneratorProTyMDI; /* *ptr tag */
	CTagItemGeneratorProTyRSCI TagItemGeneratorProTyRSCI; /* *ptr tag */
	CTagItemGeneratorLoFrCnt TagItemGeneratorLoFrCnt ; /* dlfc tag */
	CTagItemGeneratorFAC TagItemGeneratorFAC; /* fac_ tag */
	CTagItemGeneratorSDC TagItemGeneratorSDC; /* sdc_ tag */
	CTagItemGeneratorSDC TagItemGeneratorSDCEmpty; /* empty sdc_ tag for use in non-SDC frames */
	CTagItemGeneratorSDCChanInf TagItemGeneratorSDCChanInf; /* sdci tag */
	CTagItemGeneratorRobMod TagItemGeneratorRobMod; /* robm tag */
	CTagItemGeneratorInfo TagItemGeneratorInfo; /* info tag */
	CTagItemGeneratorRWMF TagItemGeneratorRWMF; /* RWMF tag */
	CTagItemGeneratorRWMM TagItemGeneratorRWMM; /* RWMM tag */
	CTagItemGeneratorRMER TagItemGeneratorRMER; /* RMER tag */
	CTagItemGeneratorRDOP TagItemGeneratorRDOP; /* RDOP tag */
	CTagItemGeneratorRDEL TagItemGeneratorRDEL; /* RDEL tag */
	CTagItemGeneratorRAFS TagItemGeneratorRAFS; /* RAFS tag */
	CTagItemGeneratorRINT TagItemGeneratorRINT; /* RINT tag */
	CTagItemGeneratorSignalStrength TagItemGeneratorSignalStrength; /* rdbv tag */
	CTagItemGeneratorReceiverStatus TagItemGeneratorReceiverStatus; /* rsta tag */

	CTagItemGeneratorProfile TagItemGeneratorProfile; /* rpro */
	CTagItemGeneratorRxDemodMode TagItemGeneratorRxDemodMode; /* rdmo */
	CTagItemGeneratorRxFrequency TagItemGeneratorRxFrequency; /* rfre */
	CTagItemGeneratorRxActivated TagItemGeneratorRxActivated; /* ract */
	CTagItemGeneratorRxBandwidth TagItemGeneratorRxBandwidth; /* rbw_ */
	CTagItemGeneratorRxService TagItemGeneratorRxService; /* rser */

	CVector<CTagItemGeneratorStr>	vecTagItemGeneratorStr; /* strx tag */

	/* Mandatory tags but not implemented yet */
	CVector<CTagItemGeneratorRBP>	vecTagItemGeneratorRBP;

	/* TAG Packet generator */
	CTagPacketGeneratorWithProfiles TagPacketGenerator;

	/* Special settings */
	_BOOLEAN					bUseAFCRC;
	char						cProfile;

	/* MDI receive ---------------------------------------------------------- */

	// Null packet socket has no dependence on QT. (but doesn't do anything either!)
#ifdef USE_QT_GUI
	CPacketSocketQT				PacketSocket;
#else
	CPacketSocketNull			PacketSocket;
#endif

	void SetEnableMDIIn(const _BOOLEAN bNEnMIn) {bMDIInEnabled = bNEnMIn;}

	CTagPacketDecoderMDI TagPacketDecoderMDI;
	CTagPacketDecoderRSCIControl TagPacketDecoderRSCIControl;

	CMDIInPkt					CurMDIPkt;
	CMDIInBuffer				MDIInBuffer;

	CMutex						Mutex;

	CDRMReceiver *				pReceiver;
};


#endif // !defined(MDI_H__3B0346264660_CA63_3452345DGERH31912__INCLUDED_)
