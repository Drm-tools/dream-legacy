/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Reception Log 
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

#ifndef _RECEPTION_LOG_H
#define _RECEPTION_LOG_H

#include "GlobalDefinitions.h"

/* Reception log -------------------------------------------------------- */
class CReceptLog
{
public:
	CReceptLog();
	virtual ~CReceptLog() {CloseFile(pFileLong, TRUE);
		CloseFile(pFileShort, FALSE);}

	void StartLogging();
	void StopLogging();
	void SetFAC(const _BOOLEAN bCRCOk);
	void SetMSC(const _BOOLEAN bCRCOk);
	void SetSync(const _BOOLEAN bCRCOk);
	void SetSNR(const _REAL rNewCurSNR);
	void SetNumAAC(const int iNewNum);
	void SetLoggingEnabled(const _BOOLEAN bLog) { bLogEnabled = bLog; }
	_BOOLEAN GetLoggingEnabled() {return bLogEnabled;}
	_BOOLEAN GetLoggingActivated() {return bLogActivated;}
	void SetLogHeader(FILE* pFile, const _BOOLEAN bIsLong);
	void SetFrequency(const int iNewFreq) {iFrequency = iNewFreq;}
	int GetFrequency() {return iFrequency;}
	void SetLatitude(const string strLat) {strLatitude = strLat;}
	string GetLatitude() {return strLatitude;}
	void SetLongitude(const string strLon) {strLongitude = strLon;}
	string GetLongitude() {return strLongitude;}
	void SetAdditText(const string strNewTxt) {strAdditText = strNewTxt;}
	void WriteParameters(const _BOOLEAN bIsLong);
	void SetDelLogStart(const int iSecDel) { iSecDelLogStart = iSecDel; }
	int GetDelLogStart() {return iSecDelLogStart;}

	void ResetTransParams();
	void SetMSCScheme(const ECodScheme eNewMCS) {eCurMSCScheme = eNewMCS;}
	void SetRobMode(const ERobMode eNewRM) {eCurRobMode = eNewRM;}
	void SetProtLev(const CMSCProtLev eNPL) {CurProtLev = eNPL;}
	void SetDoppler(_REAL r) {rDoppler = r;}
	void SetDelay(_REAL r) {rDelay = r;}

protected:
	void ResetLog(const _BOOLEAN bIsLong);
	void CloseFile(FILE* pFile, const _BOOLEAN bIsLong);
	int				iNumSNR;
	int				iNumCRCOkFAC, iNumCRCOkMSC;
	int				iNumCRCOkMSCLong, iNumCRCMSCLong;
	int				iNumAACFrames, iTimeCntShort;
	time_t			TimeCntLong;
	_BOOLEAN		bSyncOK, bFACOk, bMSCOk;
	_BOOLEAN		bSyncOKValid, bFACOkValid, bMSCOkValid;
	int				iFrequency;
	_REAL			rAvSNR, rCurSNR;
	_REAL			rMaxSNR, rMinSNR;
	_BOOLEAN		bLogActivated;
	_BOOLEAN		bLogEnabled;
	FILE*			pFileLong;
	FILE*			pFileShort;
	string			strAdditText;
	string			strLatitude;
	string			strLongitude;
	int				iSecDelLogStart;

	ERobMode		eCurRobMode;
	ECodScheme		eCurMSCScheme;
	CMSCProtLev		CurProtLev;
	_REAL			rDoppler;
	_REAL			rDelay;

	CMutex			Mutex;
};
#endif
