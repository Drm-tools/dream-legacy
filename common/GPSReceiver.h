/******************************************************************************\
 * BBC Research & Development
 * Copyright (c) 2007
 *
 * Author(s):
 *	Andrew Murphy
 *
 * Description:
 *	See GPSReceiver.cpp
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

#if !defined(_GPSRECEIVER_H_)
#define _GPSRECEIVER_H_

#include "GlobalDefinitions.h"

#ifdef USE_QT_GUI

#include <qsocket.h>

class CAutoMutex
{
public:
	CAutoMutex(CMutex& Mutex) : m_Mutex(Mutex) { m_Mutex.Lock(); }
	~CAutoMutex() { m_Mutex.Unlock(); }

	CMutex& m_Mutex;
	
};

class CGPSRxData
// *TODO* - put satus stuff from CParameter::CGPSInformation in here
{
public:
	CGPSRxData() 
	{ 
		Reset();
	}
	~CGPSRxData() {}

	enum EFix { MODE_NO_FIX, MODE_2D, MODE_3D };

	enum EStatus { GPS_RX_NOT_CONNECTED, GPS_RX_NO_DATA, GPS_RX_DATA_AVAILABLE };

	/////////

	void SetSatellitesVisibleAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bSatellitesVisibleAvailable = bNew; }
	_BOOLEAN GetSatellitesVisibleAvailable() { CAutoMutex mutex(m_Mutex); return m_bSatellitesVisibleAvailable; }	
	
	void SetSatellitesVisible(unsigned short usSatellitesVisible) { CAutoMutex mutex(m_Mutex); m_usSatellitesVisible = usSatellitesVisible; }
	unsigned short GetSatellitesVisible() { CAutoMutex mutex(m_Mutex); return m_usSatellitesVisible; } 
	
	/////////
	
	void SetPositionAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bPositionAvailable = bNew; }
	_BOOLEAN GetPositionAvailable() { CAutoMutex mutex(m_Mutex); return m_bPositionAvailable; }	
	
	void SetLatLongDegrees(float fLatitudeDegrees, float fLongitudeDegrees) { CAutoMutex mutex(m_Mutex); m_fLatitudeDegrees = fLatitudeDegrees; m_fLongitudeDegrees = fLongitudeDegrees; }
	void GetLatLongDegrees(float& fLatitudeDegrees, float& fLongitudeDegrees) { CAutoMutex mutex(m_Mutex); fLatitudeDegrees = m_fLatitudeDegrees; fLongitudeDegrees = m_fLongitudeDegrees; } 
	
	/////////

	void SetSpeedAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bSpeedAvailable = bNew; }
	_BOOLEAN GetSpeedAvailable() { CAutoMutex mutex(m_Mutex); return m_bSpeedAvailable; }	
	
	void SetSpeedMetresPerSecond(float fSpeedMetresPerSecond) { CAutoMutex mutex(m_Mutex); m_fSpeedMetresPerSecond = fSpeedMetresPerSecond; }
	float GetSpeedMetresPerSecond() { CAutoMutex mutex(m_Mutex); return m_fSpeedMetresPerSecond; } 

	/////////

	void SetHeadingAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bHeadingAvailable = bNew; }
	_BOOLEAN GetHeadingAvailable() { CAutoMutex mutex(m_Mutex); return m_bHeadingAvailable; }	
	
	void SetHeadingDegrees(unsigned short usHeadingDegrees) { CAutoMutex mutex(m_Mutex); m_usHeadingDegrees = usHeadingDegrees; }
	unsigned short GetHeadingDegrees() { CAutoMutex mutex(m_Mutex); return m_usHeadingDegrees; } 

	/////////

	void SetTimeAndDateAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bTimeAndDateAvailable = bNew; }
	_BOOLEAN GetTimeAndDateAvailable() { CAutoMutex mutex(m_Mutex); return m_bTimeAndDateAvailable; }	
	
	void SetTimeSecondsSince1970(unsigned long ulTimeSecondsSince1970) { CAutoMutex mutex(m_Mutex); m_ulTimeSecondsSince1970 = ulTimeSecondsSince1970; }
	unsigned long GetTimeSecondsSince1970() { CAutoMutex mutex(m_Mutex); return m_ulTimeSecondsSince1970; }

	/////////

	void SetAltitudeAvailable(_BOOLEAN bNew) { CAutoMutex mutex(m_Mutex); m_bAltitudeAvailable = bNew; }
	_BOOLEAN GetAltitudeAvailable() { CAutoMutex mutex(m_Mutex); return m_bAltitudeAvailable; }	
	
	void SetAltitudeMetres(float fAltitudeMetres) { CAutoMutex mutex(m_Mutex); m_fAltitudeMetres = fAltitudeMetres; }
	float GetAltitudeMetres() { CAutoMutex mutex(m_Mutex); return m_fAltitudeMetres; } 
	
	/////////

	void SetFix(EFix Fix) { CAutoMutex mutex(m_Mutex); m_eFix = Fix; }
	EFix GetFix() { CAutoMutex mutex(m_Mutex); return m_eFix; }

	/////////

	void SetStatus(EStatus eStatus) { CAutoMutex mutex(m_Mutex); m_eStatus = eStatus; 
		if (m_eStatus != GPS_RX_DATA_AVAILABLE)
			Reset();			
	}
	EStatus GetStatus() { CAutoMutex mutex(m_Mutex); return m_eStatus; }

private:
	_BOOLEAN m_bSatellitesVisibleAvailable;
	unsigned short m_usSatellitesVisible;

	_BOOLEAN m_bPositionAvailable;
	float	m_fLatitudeDegrees;
	float	m_fLongitudeDegrees;

	_BOOLEAN m_bSpeedAvailable;
	float	m_fSpeedMetresPerSecond;

	_BOOLEAN m_bHeadingAvailable;
	unsigned short	m_usHeadingDegrees;

	_BOOLEAN m_bTimeAndDateAvailable;
	unsigned long	m_ulTimeSecondsSince1970;

	_BOOLEAN m_bAltitudeAvailable;
	float m_fAltitudeMetres;

	CMutex m_Mutex;

	EFix	m_eFix;
	EStatus m_eStatus;

	void Reset()
	{
		m_bPositionAvailable = FALSE;	m_fLatitudeDegrees = 0;	m_fLongitudeDegrees = 0; 
		m_bSpeedAvailable = FALSE; m_fSpeedMetresPerSecond = 0;
		m_bHeadingAvailable = FALSE; m_usHeadingDegrees = 0; 
		m_bTimeAndDateAvailable = FALSE; m_ulTimeSecondsSince1970 = 0; 
		m_bAltitudeAvailable = FALSE; m_fAltitudeMetres = 0;
		m_eStatus = GPS_RX_NOT_CONNECTED;
		m_eFix = MODE_NO_FIX;
		m_bSatellitesVisibleAvailable = FALSE; m_usSatellitesVisible = 0;
	}
};


class CGPSReceiver
#ifdef USE_QT_GUI
	: public QObject, public QThread
#endif
{
	Q_OBJECT
public:
	CGPSReceiver();
	~CGPSReceiver();
	
	void SetGPSRxData(CGPSRxData* pGPSRxData) { m_pGPSRxData = pGPSRxData; }
	void SetGPSd(const string& host, int port) { m_GPSdHostName = host; m_GPSdPort = port; }
	virtual void	run();
	void Stop() { m_bFinished = TRUE; }

protected:
	void Start();

	void DecodeGPSDReply(string Reply);
	void DecodeString(char Command, string Value);
	void DecodeO(string Value);
	void DecodeY(string Value);
	
	enum EGPSState { DISCONNECTED, INITIALISING, WAITING, STREAMING, COMMS_ERROR };

	EGPSState m_eGPSState;

	short m_sStateMachineCyclesSinceLastGPSDReply;
	static const short c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset;
	static const unsigned short c_usReconnectIntervalSeconds;

	CGPSRxData		m_GPSRxData;
	CGPSRxData*		m_pGPSRxData;

	unsigned short m_GPSdPort;
	string	m_GPSdHostName;

	QSocket	m_Socket;

	_BOOLEAN m_bFinished;

public slots:
	void slotReadyRead();
	void slotSocketError(int);
};

#endif // QT

#endif // !defined(_GPSRECEIVER_H_)
