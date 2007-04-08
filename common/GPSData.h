/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Andrew Murphy
 *
 * Description:
 *	passive data class for GPS data
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

#ifndef _GPS_DATA_H
#define _GPS_DATA_H

#include "GlobalDefinitions.h"
#ifdef USE_GUI_QT
# if QT_VERSION >= 0x030000 	 
#  include <qmutex.h>
# else
#  include <qthread.h>
# endif
#endif

class CAutoMutex
{
public:
#ifdef USE_GUI_QT
	CAutoMutex(QMutex& Mutex) : m_Mutex(Mutex) { m_Mutex.lock(); }
	~CAutoMutex() { m_Mutex.unlock(); }

	QMutex& m_Mutex;
#else
	CAutoMutex(int Mutex) {}
#endif
};

class CGPSData
{
public:
	CGPSData() 
	{ 
		Reset();
	}
	~CGPSData() {}

	enum EFix { MODE_NO_FIX, MODE_2D, MODE_3D };

	enum EStatus { GPS_RX_NOT_CONNECTED, GPS_RX_NO_DATA, GPS_RX_DATA_AVAILABLE };

	enum EGPSSource
	{ GPS_SOURCE_INVALID, GPS_SOURCE_GPS_RECEIVER,
		GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER, GPS_SOURCE_MANUAL_ENTRY,
		GPS_SOURCE_NOT_AVAILABLE
	};

	/////////

	EGPSSource GetGPSSource();
	void SetGPSSource(EGPSSource eNewSource);

	/////////

	void SetSatellitesVisibleAvailable(_BOOLEAN bNew);
	_BOOLEAN GetSatellitesVisibleAvailable();
	
	void SetSatellitesVisible(uint16_t usSatellitesVisible);
	uint16_t GetSatellitesVisible();
	
	/////////
	
	void SetPositionAvailable(_BOOLEAN bNew);
	_BOOLEAN GetPositionAvailable();
	
	void SetLatLongDegrees(double fLatitudeDegrees, double fLongitudeDegrees);
	void GetLatLongDegrees(double& fLatitudeDegrees, double& fLongitudeDegrees);
	
	/////////

	void SetSpeedAvailable(_BOOLEAN bNew);
	_BOOLEAN GetSpeedAvailable();
	
	void SetSpeedMetresPerSecond(double fSpeedMetresPerSecond);
	double GetSpeedMetresPerSecond();

	/////////

	void SetHeadingAvailable(_BOOLEAN bNew);
	_BOOLEAN GetHeadingAvailable();
	
	void SetHeadingDegrees(uint16_t usHeadingDegrees);
	unsigned short GetHeadingDegrees();

	/////////

	void SetTimeAndDateAvailable(_BOOLEAN bNew);
	_BOOLEAN GetTimeAndDateAvailable();
	
	void SetTimeSecondsSince1970(uint32_t ulTimeSecondsSince1970);
	uint32_t GetTimeSecondsSince1970();
	string GetTimeDate();
	void GetTimeDate(uint32_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute, uint8_t& second);

	/////////

	void SetAltitudeAvailable(_BOOLEAN bNew);
	_BOOLEAN GetAltitudeAvailable();
	
	void SetAltitudeMetres(double fAltitudeMetres);
	double GetAltitudeMetres();
	
	/////////

	void SetFix(EFix Fix);
	EFix GetFix();

	/////////

	void SetStatus(EStatus eStatus);
	EStatus GetStatus();

	string host;
	uint16_t port;

private:
	_BOOLEAN m_bSatellitesVisibleAvailable;
	uint16_t m_usSatellitesVisible;

	_BOOLEAN m_bPositionAvailable;
	double	m_fLatitudeDegrees;
	double	m_fLongitudeDegrees;

	_BOOLEAN m_bSpeedAvailable;
	double	m_fSpeedMetresPerSecond;

	_BOOLEAN m_bHeadingAvailable;
	uint16_t	m_usHeadingDegrees;

	_BOOLEAN m_bTimeAndDateAvailable;
	uint32_t m_ulTimeSecondsSince1970;

	_BOOLEAN m_bAltitudeAvailable;
	double m_fAltitudeMetres;

#ifdef USE_GUI_QT
	QMutex m_Mutex;
#else
	int m_Mutex;
#endif

	EFix	m_eFix;
	EStatus m_eStatus;
	EGPSSource eGPSSource;

	void Reset();
};
#endif
