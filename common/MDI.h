/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	see MDI.cpp
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

#include "GlobalDefinitions.h"
#include "Parameter.h"
#include "Vector.h"
#include "CRC.h"
#include <qsocketdevice.h>
#include <qsocketnotifier.h>


/* Definitions ****************************************************************/
/* Major revision: Currently 0000_[16] */
#define MDI_MAJOR_REVISION			0

/* Minor revision: Currently 0000_[16] */
#define MDI_MINOR_REVISION			0

/* Major revision of the AF protocol in use: Currently 01_[16] */
#define AF_MAJOR_REVISION			1

/* Minor revision of the AF protocol in use: Currently 00_[16] */
#define AF_MINOR_REVISION			0

/* Maximum number of bytes received from the network interface. Maximum data
   rate of DRM is approx. 80 kbps. One MDI packet must be sent each DRM frame
   which is every 400 ms -> 0.4 * 80000 / 8 = 4000 bytes. Allocating more than
   double of this size should be ok for all possible cases */
#define MAX_SIZE_BYTES_NETW_BUF		10000


/* Classes ********************************************************************/
class CMDI : public QObject
{
	Q_OBJECT

public:
	CMDI();
	virtual ~CMDI() {}

	/* MDI out */
	void SetFACData(CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter);
	void SetSDCData(CVectorEx<_BINARY>& vecbiSDCData);
	void SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData);
	_BOOLEAN SetNetwOutAddr(const string strNewIPPort);
	_BOOLEAN GetMDIOutEnabled() {return bMDIOutEnabled;}

	void SetAFPktCRC(const _BOOLEAN bNAFPktCRC) {bUseAFCRC = bNAFPktCRC;}
	_BOOLEAN GetAFPktCRC() {return bUseAFCRC;}

	/* MDI in */
	ERobMode GetFACData(CVectorEx<_BINARY>& vecbiFACData);
	void GetSDCData(CVectorEx<_BINARY>& vecbiSDCData);
	void GetStreamData(CVectorEx<_BINARY>& vecbiStrData, const int iLen,
		const int iStrNum);
	_BOOLEAN SetNetwInPort(const int iPort);
	_BOOLEAN GetMDIInEnabled() {return bMDIInEnabled;}

protected:
	/* MDI transmit --------------------------------------------------------- */
	void SetEnableMDIOut(const _BOOLEAN bNEnMOut) {bMDIOutEnabled = bNEnMOut;}

	void SendPacket(CVector<_BINARY> vecbiPacket);

	void PrepareTag(CVector<_BINARY>& vecbiTag, const string strTagName,
		const int iLenDataBits);

	void GenTagProTy();
	void GenTagLoFrCnt();
	void GenTagFAC(CVectorEx<_BINARY>& vecbiFACData);
	void GenTagSDC(CVectorEx<_BINARY>& vecbiSDCData);
	void GenTagSDCChanInf(CParameter& Parameter);
	void GenTagRobMod(const ERobMode eCurRobMode);
	void GenTagStr(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData);
	void GenTagInfo(string strUTF8Text);

	CVector<_BINARY> GenAFPacket(const _BOOLEAN bWithSDC);
	void ResetTags(const _BOOLEAN bResetSDC);
	_BOOLEAN					bSDCWasSet;

	uint32_t					iLogFraCnt;

	int							iSeqNumber;

	_BOOLEAN					bMDIOutEnabled;
	_BOOLEAN					bMDIInEnabled;

	QHostAddress				HostAddrOut;
	int							iHostPortOut;

	CVector<_BINARY>			vecbiTagProTy; /* *ptr tag */
	CVector<_BINARY>			vecbiTagLoFrCnt; /* dlfc tag */
	CVector<_BINARY>			vecbiTagFAC; /* fac_ tag */
	CVector<_BINARY>			vecbiTagSDC; /* sdc_ tag */
	CVector<_BINARY>			vecbiTagSDCChanInf; /* sdci tag */
	CVector<_BINARY>			vecbiTagRobMod; /* robm tag */
	CVector<_BINARY>			vecbiTagInfo; /* info tag */
	CVector<CVector<_BINARY> >	vecbiTagStr; /* strx tag */

	/* Special settings */
	_BOOLEAN			bUseAFCRC;


	/* MDI receive ---------------------------------------------------------- */
	void SetEnableMDIIn(const _BOOLEAN bNEnMIn) {bMDIInEnabled = bNEnMIn;}

	int DecodeTag(CVector<_BINARY>& vecbiTag);

	void DecTagProTy(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagLoFrCnt(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagFAC(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagSDC(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagSDCChanInf(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagRobMod(CVector<_BINARY>& vecbiTag, const int iLen);
	void DecTagStr(CVector<_BINARY>& vecbiTag, const int iLen,
		const int iStrNum);
	void DecTagInfo(CVector<_BINARY>& vecbiTag, const int iLen);

	void DecAFPacket(CVector<_BINARY>& vecbiAFPkt);

	CVector<_BINARY>			vecbiIncFACData;
	CVector<_BINARY>			vecbiIncSDCData;
	CVector<CVector<_BINARY> >	vecbiIncStr;
	ERobMode					eMDIInRobMode;
	QSocketDevice				SocketDevice;
	QSocketNotifier*			pSocketNotivRead;

	CMutex						Mutex;

public slots:
	void OnDataReceived();
};


#endif // !defined(MDI_H__3B0346264660_CA63_3452345DGERH31912__INCLUDED_)
