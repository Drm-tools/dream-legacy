/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	Derivation of dummy MDI class that doesn't do anything. Can be used e.g. in simulation.
 *  This avoids the need to have "if pMDI != NULL" every time the MDI object is accessed.
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


#if !defined(MDI_NULL_H_INCLUDED)
#define MDI_NULL_H_INCLUDED

#include "MDI.h"

/* Definitions ****************************************************************/


/* Classes ********************************************************************/
class CMDINull : public CMDI
{
public:
	virtual void SetDRMReceiver(CDRMReceiver *pRx) {}

	virtual ~CMDINull() {}

	/* MDI out */
	virtual void SetFACData(_BOOLEAN bFACOK, CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter) {}
	virtual void SetSDCData(_BOOLEAN bSDCOK, CVectorEx<_BINARY>& vecbiSDCData) {}
	virtual void SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData) {}
	virtual void SetMERs(const _REAL rRMER, const _REAL rRWMM, const _REAL rRWMF) {}
	virtual void SetRDEL(const CRealVector &vecrThresholds, const CRealVector &vecrIntervals) {}
	virtual void SetRDOP(const CReal rDoppler) {}
	virtual void SetInterference(const CReal rIntFreq, const CReal rINR, const CReal rICR) {}
	virtual void SetAudioFrameStatus(_BOOLEAN bIsValid, CVector<_BINARY>& vecbiAudioStatus) {}

	virtual void SendUnlockedFrame(CParameter& Parameter) {}  /* called once per frame if the Rx isn't synchronised */

	virtual _BOOLEAN SetNetwOutAddr(const string strNewIPPort) {return TRUE;}
	virtual _BOOLEAN GetMDIOutEnabled() {return FALSE;}

	virtual void SetAFPktCRC(const _BOOLEAN bNAFPktCRC) {}

	void SetProfile(const char cProf) {}
	
		/* GetAFPktCRC() doesn't seem to be called, so I can't decide if it should return true or false */
	virtual _BOOLEAN GetAFPktCRC()  {return FALSE;} 

	/* MDI in */
	/* These functions don't do anything, but shouldn't be called anyway because GetMDIInEnabled is always false */
	virtual ERobMode GetFACData(CVectorEx<_BINARY>& vecbiFACData) {return RM_NO_MODE_DETECTED;}
	virtual void GetSDCData(CVectorEx<_BINARY>& vecbiSDCData) {}
	virtual void GetStreamData(CVectorEx<_BINARY>& vecbiStrData, const int iLen,
		const int iStrNum) {}
	virtual _BOOLEAN SetNetwInPort(const int iPort) {return TRUE;}
	virtual _BOOLEAN SetNetwInMcast(const string strNewIPIP) {return TRUE;}
	virtual _BOOLEAN SetNetwInPortRSCI(const int iPort) {return TRUE;};
	virtual _BOOLEAN GetMDIInEnabled()  {return FALSE;}

};


#endif // !defined(MDI_NULL_H_INCLUDED)
