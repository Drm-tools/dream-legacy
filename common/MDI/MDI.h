/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	Pure abstract interface definition for MDI class. See MDIConcrete.cpp
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


#if !defined(MDI_H__3B0346264660_CA63_3452345DGERH31912__INCLUDED_)
#define MDI_H__3B0346264660_CA63_3452345DGERH31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "MDIDefinitions.h"
#include "MDITagItems.h"
#include "MDIInBuffer.h"
#include "../Parameter.h"
#include "../util/Vector.h"

#include "MDITagPacketDecoder.h"
#include "TagPacketDecoderRSCIControl.h"

/* Definitions ****************************************************************/

class CDRMReceiver; /* forward declaration so it can contain a pointer back to the DRMReceiver */

/* Classes ********************************************************************/
class CMDI
{
public:
	virtual void SetDRMReceiver(CDRMReceiver *pRx) = 0;

	virtual ~CMDI() {}

	/* MDI out */
	virtual void SetFACData(_BOOLEAN bFACOK, CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter)  = 0;
	virtual void SetSDCData(_BOOLEAN bSDCOK, CVectorEx<_BINARY>& vecbiSDCData) = 0;
	virtual void SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData) = 0;
	virtual void SetMERs(const _REAL rRMER, const _REAL rRWMM, const _REAL rRWMF) = 0;
	virtual void SetRDEL(const CRealVector &vecrThresholds, const CRealVector &vecrIntervals) = 0;
	virtual void SetRDOP(const CReal rDoppler) = 0;
	virtual void SetInterference(const CReal rIntFreq, const CReal rINR, const CReal rICR) = 0;
	virtual void SetAudioFrameStatus(_BOOLEAN bIsValid, CVector<_BINARY>& vecbiAudioStatus) = 0;

	virtual void SendUnlockedFrame(CParameter& Parameter) = 0; /* called once per frame if the Rx isn't synchronised */

	virtual _BOOLEAN SetNetwOutAddr(const string strNewIPPort) = 0;
	virtual _BOOLEAN GetMDIOutEnabled() = 0;

	virtual void SetAFPktCRC(const _BOOLEAN bNAFPktCRC)  = 0;
	virtual _BOOLEAN GetAFPktCRC()  = 0;

	virtual void SetProfile(const char cProfile) = 0;

	/* MDI in */
	virtual ERobMode GetFACData(CVectorEx<_BINARY>& vecbiFACData) = 0;
	virtual void GetSDCData(CVectorEx<_BINARY>& vecbiSDCData) = 0;
	virtual void GetStreamData(CVectorEx<_BINARY>& vecbiStrData, const int iLen,
		const int iStrNum) = 0;
	virtual _BOOLEAN SetNetwInPort(const int iPort) = 0;
	virtual _BOOLEAN SetNetwInMcast(const string strNewIPIP) = 0;
	virtual _BOOLEAN SetNetwInPortRSCI(const int iPort) = 0;
	virtual _BOOLEAN GetMDIInEnabled()  = 0;

};


#endif // !defined(MDI_H__3B0346264660_CA63_3452345DGERH31912__INCLUDED_)
