/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	DRM Parameters
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

#include "GPSData.h"
#include "time.h"

CGPSData::EGPSSource CGPSData::GetGPSSource()
{
	return eGPSSource;
}

void
CGPSData::SetGPSSource(EGPSSource eNewSource)
{
	eGPSSource = eNewSource;
}

void
CGPSData::SetSatellitesVisibleAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bSatellitesVisibleAvailable = bNew;
}

_BOOLEAN CGPSData::GetSatellitesVisibleAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bSatellitesVisibleAvailable;
}

void
CGPSData::SetSatellitesVisible(uint16_t usSatellitesVisible)
{
	CAutoMutex mutex(m_Mutex);
	m_usSatellitesVisible = usSatellitesVisible;
}

uint16_t CGPSData::GetSatellitesVisible()
{
	CAutoMutex mutex(m_Mutex);
	return m_usSatellitesVisible;
}

void
CGPSData::SetPositionAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bPositionAvailable = bNew;
}

_BOOLEAN CGPSData::GetPositionAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bPositionAvailable;
}

void
CGPSData::SetLatLongDegrees(double fLatitudeDegrees, double fLongitudeDegrees)
{
	CAutoMutex mutex(m_Mutex);
	m_fLatitudeDegrees = fLatitudeDegrees;
	m_fLongitudeDegrees = fLongitudeDegrees;
}

void
CGPSData::GetLatLongDegrees(double &fLatitudeDegrees, double &fLongitudeDegrees)
{
	CAutoMutex mutex(m_Mutex);
	fLatitudeDegrees = m_fLatitudeDegrees;
	fLongitudeDegrees = m_fLongitudeDegrees;
}

void
CGPSData::SetSpeedAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bSpeedAvailable = bNew;
}

_BOOLEAN CGPSData::GetSpeedAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bSpeedAvailable;
}

void
CGPSData::SetSpeedMetresPerSecond(double fSpeedMetresPerSecond)
{
	CAutoMutex mutex(m_Mutex);
	m_fSpeedMetresPerSecond = fSpeedMetresPerSecond;
}

double
CGPSData::GetSpeedMetresPerSecond()
{
	CAutoMutex mutex(m_Mutex);
	return m_fSpeedMetresPerSecond;
}

void
CGPSData::SetHeadingAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bHeadingAvailable = bNew;
}

_BOOLEAN CGPSData::GetHeadingAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bHeadingAvailable;
}

void
CGPSData::SetHeadingDegrees(uint16_t usHeadingDegrees)
{
	CAutoMutex mutex(m_Mutex);
	m_usHeadingDegrees = usHeadingDegrees;
}

unsigned short
CGPSData::GetHeadingDegrees()
{
	CAutoMutex mutex(m_Mutex);
	return m_usHeadingDegrees;
}

void
CGPSData::SetTimeAndDateAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bTimeAndDateAvailable = bNew;
}

_BOOLEAN CGPSData::GetTimeAndDateAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bTimeAndDateAvailable;
}

void
CGPSData::SetTimeSecondsSince1970(uint32_t ulTimeSecondsSince1970)
{
	CAutoMutex mutex(m_Mutex);
	m_ulTimeSecondsSince1970 = ulTimeSecondsSince1970;
}

uint32_t CGPSData::GetTimeSecondsSince1970()
{
	CAutoMutex mutex(m_Mutex);
	return m_ulTimeSecondsSince1970;
}

void
CGPSData::SetAltitudeAvailable(_BOOLEAN bNew)
{
	CAutoMutex mutex(m_Mutex);
	m_bAltitudeAvailable = bNew;
}

_BOOLEAN CGPSData::GetAltitudeAvailable()
{
	CAutoMutex mutex(m_Mutex);
	return m_bAltitudeAvailable;
}

void
CGPSData::SetAltitudeMetres(double fAltitudeMetres)
{
	CAutoMutex mutex(m_Mutex);
	m_fAltitudeMetres = fAltitudeMetres;
}

double
CGPSData::GetAltitudeMetres()
{
	CAutoMutex mutex(m_Mutex);
	return m_fAltitudeMetres;
}

void
CGPSData::SetFix(CGPSData::EFix Fix)
{
	CAutoMutex mutex(m_Mutex);
	m_eFix = Fix;
}

CGPSData::EFix CGPSData::GetFix()
{
	CAutoMutex mutex(m_Mutex);
	return m_eFix;
}

void
CGPSData::SetStatus(EStatus eStatus)
{
	CAutoMutex mutex(m_Mutex);
	m_eStatus = eStatus;
	if (m_eStatus != GPS_RX_DATA_AVAILABLE)
		Reset();
}

CGPSData::EStatus CGPSData::GetStatus()
{
	CAutoMutex mutex(m_Mutex);
	return m_eStatus;
}

void
CGPSData::Reset()
{
	m_bPositionAvailable = FALSE;
	m_fLatitudeDegrees = 0;
	m_fLongitudeDegrees = 0;
	m_bSpeedAvailable = FALSE;
	m_fSpeedMetresPerSecond = 0;
	m_bHeadingAvailable = FALSE;
	m_usHeadingDegrees = 0;
	m_bTimeAndDateAvailable = FALSE;
	m_ulTimeSecondsSince1970 = 0;
	m_bAltitudeAvailable = FALSE;
	m_fAltitudeMetres = 0;
	m_eStatus = GPS_RX_NOT_CONNECTED;
	m_eFix = MODE_NO_FIX;
	m_bSatellitesVisibleAvailable = FALSE;
	m_usSatellitesVisible = 0;
}

void
CGPSData::GetTimeDate(uint32_t & year, uint8_t & month, uint8_t & day,
					  uint8_t & hour, uint8_t & minute, uint8_t & second)
{
	struct tm *p_ts;
	time_t tt;
	{
		CAutoMutex mutex(m_Mutex);
		tt = time_t(m_ulTimeSecondsSince1970);
	}
	p_ts = gmtime(&tt);
	year = 1900+p_ts->tm_year;
	month = p_ts->tm_mon+1;
	day = p_ts->tm_mday;
	hour = p_ts->tm_hour;
	minute = p_ts->tm_min;
	second = p_ts->tm_sec;
}

string CGPSData::GetTimeDate()
{
	struct tm * p_ts;
	time_t tt;
	{
		CAutoMutex mutex(m_Mutex);
		tt = time_t(m_ulTimeSecondsSince1970);
	}
	p_ts = gmtime(&tt);
	stringstream
		ss;
	ss.width(2);
	ss.fill('0');
	ss << 1900 + p_ts->tm_year << "/" << 1 +
		p_ts->tm_mon << "/" << p_ts->tm_mday;
	ss << " " << p_ts->tm_hour << ":" << p_ts->tm_min << ":" << p_ts->tm_sec;

	return ss.str();
}
