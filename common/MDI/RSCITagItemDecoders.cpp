/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module derives, from the CTagItemDecoder base class, tag item decoders specialised to decode each of the tag
 *  items defined in the control part of RSCI.
 *  Decoded commands are generally sent straight to the CDRMReceiver object which
 *	they hold a pointer to.
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


#include "RSCITagItemDecoders.h"
#include "../DrmReceiver.h"
#include <time.h>
#include <stdlib.h>

/* RX_STAT Items */

_REAL CTagItemDecoderRSI::decodeDb(CVector<_BINARY>& vecbiTag)
{
 	  int8_t n = (int8_t)vecbiTag.Separate(8);
 	  uint8_t m = (uint8_t)vecbiTag.Separate(8);
 	  return _REAL(n)+_REAL(m)/256.0;
}

void CTagItemDecoderRdbv::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
	_REAL rSigStr = decodeDb(vecbiTag);
 	 pParameter->SigStrstat.addSample(rSigStr);
	 /* this is the only signal strength we have so update the IF level too.
	  * TODO scaling factor ? */
 	 pParameter->SetIFSignalLevel(rSigStr);
}

void CTagItemDecoderRsta::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;
 	uint8_t sync = (uint8_t)vecbiTag.Separate(8);
 	uint8_t fac = (uint8_t)vecbiTag.Separate(8);
 	uint8_t sdc = (uint8_t)vecbiTag.Separate(8);
 	uint8_t audio = (uint8_t)vecbiTag.Separate(8);
 	if(sync==0)
		pParameter->ReceiveStatus.TSync.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.TSync.SetStatus(CRC_ERROR);
 	if(fac==0)
		pParameter->ReceiveStatus.FAC.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.FAC.SetStatus(CRC_ERROR);
 	if(sdc==0)
		pParameter->ReceiveStatus.SDC.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.SDC.SetStatus(CRC_ERROR);
 	if(audio==0)
		pParameter->ReceiveStatus.Audio.SetStatus(RX_OK);
	else
		pParameter->ReceiveStatus.Audio.SetStatus(CRC_ERROR);
}

void CTagItemDecoderRwmf::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rWMERFAC = decodeDb(vecbiTag);
}

void CTagItemDecoderRwmm::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rWMERMSC = decodeDb(vecbiTag);
}

void CTagItemDecoderRmer::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rMER = decodeDb(vecbiTag);
}

void CTagItemDecoderRdop::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 16)
		return;
 	 pParameter->rRdop = decodeDb(vecbiTag);
}

void CTagItemDecoderRdel::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	int iNumEntries = iLen/(3*SIZEOF__BYTE);
	pParameter->vecrRdelIntervals.Init(iNumEntries);
	pParameter->vecrRdelThresholds.Init(iNumEntries);

	for (int i=0; i<iNumEntries; i++)
	{
 		pParameter->vecrRdelThresholds[i] = vecbiTag.Separate(SIZEOF__BYTE);
		pParameter->vecrRdelIntervals[i] = decodeDb(vecbiTag);
	}
}

void CTagItemDecoderRpsd::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 680 && iLen !=1112)
		return;

	int iVectorLen = iLen/SIZEOF__BYTE;

	pParameter->vecrPSD.Init(iVectorLen);

	for (int i = 0; i < iVectorLen; i++)
	{
		pParameter->vecrPSD[i] = -(_REAL(vecbiTag.Separate(SIZEOF__BYTE))/_REAL(2.0));
	}

}

void CTagItemDecoderRgps::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 26 * SIZEOF__BYTE)
		return;

    CGPSData& GPSData = pParameter->GPSData;

 	uint16_t source = (uint16_t)vecbiTag.Separate(SIZEOF__BYTE);
 	switch(source)
 	{
 	    case 0:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_INVALID);
            break;
 	    case 1:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_GPS_RECEIVER);
            break;
 	    case 2:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER);
            break;
 	    case 3:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
            break;
 	    case 0xff:
            GPSData.SetGPSSource(CGPSData::GPS_SOURCE_NOT_AVAILABLE);
            break;
 	    default:
            cerr << "error decoding rgps" << endl;
 	}

 	uint16_t nSats = (uint16_t)vecbiTag.Separate(SIZEOF__BYTE);
 	if(nSats == 0xff)
 	{
 	    GPSData.SetSatellitesVisibleAvailable(FALSE);
 	}
 	else
 	{
 	    GPSData.SetSatellitesVisible(nSats);
 	    GPSData.SetSatellitesVisibleAvailable(TRUE);
 	}

	int iLatitudeDegrees = int(vecbiTag.Separate(2 * SIZEOF__BYTE));
    uint8_t uiLatitudeMinutes = (uint8_t)vecbiTag.Separate(SIZEOF__BYTE);
	uint16_t uiLatitudeMinuteFractions = (uint16_t)vecbiTag.Separate(2 * SIZEOF__BYTE);
	uint16_t iLongitudeDegrees = (uint16_t)vecbiTag.Separate(2 * SIZEOF__BYTE);
    uint8_t uiLongitudeMinutes = (uint8_t)vecbiTag.Separate(SIZEOF__BYTE);
	uint16_t uiLongitudeMinuteFractions = (uint16_t)vecbiTag.Separate(2 * SIZEOF__BYTE);

    if(uiLatitudeMinutes == 0xff)
    {
        GPSData.SetPositionAvailable(FALSE);
    }
    else
    {
		double latitude, longitude;
		latitude = double(iLatitudeDegrees)
		 + (double(uiLatitudeMinutes) + double(uiLatitudeMinuteFractions)/65536.0)/60.0;
		longitude = double(iLongitudeDegrees)
		 + (double(uiLongitudeMinutes) + double(uiLongitudeMinuteFractions)/65536.0)/60.0;
        GPSData.SetLatLongDegrees(latitude, longitude);
        GPSData.SetPositionAvailable(TRUE);
    }

	uint16_t iAltitudeMetres = (uint16_t)vecbiTag.Separate(2 * SIZEOF__BYTE);
    uint8_t uiAltitudeMetreFractions = (uint8_t)vecbiTag.Separate(SIZEOF__BYTE);
    if(uiAltitudeMetreFractions == 0xff)
    {
        GPSData.SetAltitudeAvailable(FALSE);
    }
    else
    {
        GPSData.SetAltitudeMetres(iAltitudeMetres+uiAltitudeMetreFractions/256.0);
        GPSData.SetAltitudeAvailable(TRUE);
    }

    struct tm tm;
    tm.tm_hour = uint8_t(vecbiTag.Separate(SIZEOF__BYTE));
    tm.tm_min = uint8_t(vecbiTag.Separate(SIZEOF__BYTE));
    tm.tm_sec = uint8_t(vecbiTag.Separate(SIZEOF__BYTE));
    uint16_t year = uint16_t(vecbiTag.Separate(2*SIZEOF__BYTE));
    tm.tm_year = year - 1900;
    tm.tm_mon = uint8_t(vecbiTag.Separate(SIZEOF__BYTE))-1;
    tm.tm_mday = uint8_t(vecbiTag.Separate(SIZEOF__BYTE));

    if(tm.tm_hour == 0xff)
    {
        GPSData.SetTimeAndDateAvailable(FALSE);
    }
    else
    {
#ifdef _WIN32
        //_putenv("TZ=UTC");
        //_tzset();
#else
        putenv("TZ=UTC");
        tzset();
#endif
        time_t t = mktime(&tm);
        GPSData.SetTimeSecondsSince1970(t);
        GPSData.SetTimeAndDateAvailable(TRUE);
    }

    uint16_t speed = (uint16_t)vecbiTag.Separate(2*SIZEOF__BYTE);
    if(speed == 0xff)
    {
        GPSData.SetSpeedAvailable(FALSE);
    }
    else
    {
        GPSData.SetSpeedMetresPerSecond(double(speed)/10.0);
        GPSData.SetSpeedAvailable(TRUE);
    }

    uint16_t heading = (uint16_t)vecbiTag.Separate(2*SIZEOF__BYTE);
    if(heading == 0xff)
    {
        GPSData.SetHeadingAvailable(FALSE);
    }
    else
    {
        GPSData.SetHeadingDegrees(heading);
        GPSData.SetHeadingAvailable(TRUE);
    }
}

/* RX_CTRL Items */

void CTagItemDecoderCact::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	const int iNewState = vecbiTag.Separate(8) - '0';

	if (pDRMReceiver == NULL)
		return;

	// TODO pDRMReceiver->SetState(iNewState);
	(void)iNewState;

}

void CTagItemDecoderCfre::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	if (pDRMReceiver == NULL)
		return;

	const int iNewFrequency = vecbiTag.Separate(32);

	pDRMReceiver->SetFrequency(iNewFrequency/1000);

}

void CTagItemDecoderCdmo::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	string s = "";
	for (int i = 0; i < iLen / SIZEOF__BYTE; i++)
		s += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);

	if (pDRMReceiver == NULL)
		return;

	if(s == "drm_")
		pDRMReceiver->SetReceiverMode(RM_DRM);
	if(s == "am__")
		pDRMReceiver->SetReceiverMode(RM_AM);
}

void CTagItemDecoderCrec::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 32)
		return;

	string s = "";
	for (int i = 0; i < 2; i++)
		s += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);
	char c3 = (char) vecbiTag.Separate(SIZEOF__BYTE);
	char c4 = (char) vecbiTag.Separate(SIZEOF__BYTE);

	if (pDRMReceiver == NULL)
		return;

	if(s == "st")
		pDRMReceiver->SetRSIRecording(c4=='1', c3);
	if(s == "iq")
		pDRMReceiver->SetIQRecording(c4=='1');
}

void CTagItemDecoderCpro::DecodeTag(CVector<_BINARY>& vecbiTag, const int iLen)
{
	if (iLen != 8)
		return;

	char c = char(vecbiTag.Separate(SIZEOF__BYTE));
	if (pRSISubscriber != NULL)
		pRSISubscriber->SetProfile(c);
}
/* TODO: other control tag items */
