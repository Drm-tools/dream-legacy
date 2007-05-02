/******************************************************************************\
 * BBC Research & Development
 * Copyright (c) 2007
 *
 * Author(s):
 *	Andrew Murphy
 *
 * Description:
 *	Talks to a GPS receiver courtesy of gpsd (http://gpsd.berlios.de)
 *
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
 * 59 Temple Place, Suite 3c_usReconnectIntervalSeconds, Boston, MA 02111-1307 USA
 *
\******************************************************************************/


#include "GPSReceiver.h"

#ifdef USE_QT_GUI
# include <qsocket.h>
# include <qsignal.h>
#endif

#include <sstream>
#include <iomanip>
using namespace std;

const unsigned short CGPSReceiver::c_usReconnectIntervalSeconds = 30;

CGPSReceiver::CGPSReceiver(CParameter& p, CSettings& s):
	Parameters(p),m_Settings(s),m_pSocket(NULL),m_iCounter(0),
	m_sHost("localhost"),m_iPort(2947)
{	
    m_pTimer = new QTimer(this);

	m_sHost = m_Settings.Get("GPS", "host", m_sHost);
	m_iPort = m_Settings.Get("GPS", "port", m_iPort);

	open();
}

CGPSReceiver::~CGPSReceiver()
{
	close();
}

void CGPSReceiver::open()
{
	Parameters.Lock(); 
	Parameters.GPSData.SetStatus(CGPSData::GPS_RX_NOT_CONNECTED);
	Parameters.Unlock(); 
	if(m_pSocket == NULL)
	{
		m_pSocket = new QSocket();
		if(m_pSocket == NULL)
			return;
		connect(m_pSocket, SIGNAL(connected()), this, SLOT(slotConnected()));
		connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
		connect(m_pSocket, SIGNAL(error(int)), this, SLOT(slotSocketError(int)));
	}
	m_pSocket->connectToHost(m_sHost.c_str(), m_iPort);
}

void CGPSReceiver::close()
{
	if(m_pSocket == NULL)
		return;

	Parameters.Lock(); 
	Parameters.GPSData.SetStatus(CGPSData::GPS_RX_NOT_CONNECTED);
	Parameters.Unlock(); 

	m_pSocket->close();
	disconnect(m_pSocket, SIGNAL(connected()), this, SLOT(slotConnected()));
	disconnect(m_pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
	disconnect(m_pSocket, SIGNAL(error(int)), this, SLOT(slotSocketError(int)));
	delete m_pSocket;
	m_pSocket = NULL;
}

void CGPSReceiver::DecodeGPSDReply(string Reply)
{
	string TotalReply;

	TotalReply = Reply;

	//GPSD\n\r
	//GPSD,F=1,A=?\n\r

	size_t GPSDPos=0;

	while ((GPSDPos = TotalReply.find("GPSD",0)) != string::npos)
	{
		TotalReply=TotalReply.substr(GPSDPos+5,TotalReply.length()-(5+GPSDPos));

		_BOOLEAN finished = FALSE;

		while (!finished)		// while not all of message has been consumed
		{
			size_t CurrentPos = 0;
			for (size_t i=0; i < TotalReply.length(); i++)
			{
				if (TotalReply[i] == ',' || TotalReply[i] == '\r' || TotalReply[i] == '\n')
				{
					char Command = (char) TotalReply[0];
					string Value = TotalReply.substr(CurrentPos+2,i-CurrentPos);	// 2 to allow for equals sign
					
					DecodeString(Command,Value);
					
					CurrentPos = i;

					if (TotalReply[i] == '\r' || TotalReply[i] == '\n')		// end of line
					{
						finished = TRUE;
						break;
					}
				}
			}			
		}
	}

}

//decode gpsd strings
void CGPSReceiver::DecodeString(char Command, string Value)
{
		switch (Command)
		{
			case 'O':
				DecodeO(Value);
				break;
			case 'Y':
				DecodeY(Value);
				break;
			case 'X':		// online/offline status
				break;
		default:
				//do nowt
			break;
		}
}

void CGPSReceiver::DecodeO(string Value)
{	
	Parameters.Lock(); 
	if (Value[0] == '?')
	{
		Parameters.GPSData.SetPositionAvailable(FALSE);
		Parameters.GPSData.SetAltitudeAvailable(FALSE);
		Parameters.GPSData.SetTimeAndDateAvailable(FALSE);
		Parameters.GPSData.SetHeadingAvailable(FALSE);
		Parameters.GPSData.SetSpeedAvailable(FALSE);
		return;
	}

	string sTag, sTime, sTimeError;
	double fLatitude, fLongitude;
	string sAltitude, sErrorHoriz, sErrorVert;
	string sHeading, sSpeed, sClimb, sHeadingError, sSpeedError, sClimbError;

	stringstream ssValue(Value);

	ssValue >> sTag >> sTime >> sTimeError >> fLatitude >> fLongitude >> sAltitude;
	ssValue >> sErrorHoriz >> sErrorVert >> sHeading >> sSpeed >> sClimb >> sHeadingError;
	ssValue >> sSpeedError >> sClimbError;

	Parameters.GPSData.SetLatLongDegrees(fLatitude, fLongitude);
	Parameters.GPSData.SetPositionAvailable(TRUE);
	m_Settings.Put("Logfile", "latitude", fLatitude);
	m_Settings.Put("Logfile", "longitude", fLongitude);

	if (sTime.find('?') == string::npos)
	{
		stringstream ssTime(sTime);
		unsigned long ulTime;
		ssTime >> ulTime;
		Parameters.GPSData.SetTimeSecondsSince1970(ulTime);
		Parameters.GPSData.SetTimeAndDateAvailable(TRUE);
	}
	else
	{
		Parameters.GPSData.SetTimeAndDateAvailable(FALSE);
	}

	if (sAltitude.find('?') == string::npos)	// if '?' not found..
	{
		Parameters.GPSData.SetAltitudeAvailable(TRUE);
		Parameters.GPSData.SetAltitudeMetres(atof(sAltitude.c_str()));
	}
	else
		Parameters.GPSData.SetAltitudeAvailable(FALSE);

	if (sHeading.find('?') == string::npos)
	{
		Parameters.GPSData.SetHeadingAvailable(TRUE);
		Parameters.GPSData.SetHeadingDegrees((unsigned short) atof(sHeading.c_str()));
	}
	else
		Parameters.GPSData.SetHeadingAvailable(FALSE);

	if (sSpeed.find('?') == string::npos)
	{
		Parameters.GPSData.SetSpeedAvailable(TRUE);
		Parameters.GPSData.SetSpeedMetresPerSecond(atof(sSpeed.c_str()));
	}
	else
		Parameters.GPSData.SetSpeedAvailable(FALSE);

	Parameters.Unlock(); 
}

void CGPSReceiver::DecodeY(string Value)
{
	Parameters.Lock(); 
	if (Value[0] == '?')
	{
		Parameters.GPSData.SetSatellitesVisibleAvailable(FALSE);
		Parameters.GPSData.SetTimeAndDateAvailable(FALSE);
		return;
	}

	string sTag, sTimestamp;
	unsigned short usSatellites;
	stringstream ssValue(Value);

	ssValue >> sTag >> sTimestamp >> usSatellites;

	Parameters.GPSData.SetSatellitesVisible(usSatellites);
	Parameters.GPSData.SetSatellitesVisibleAvailable(TRUE);

	//todo - timestamp//
	Parameters.Unlock(); 

}

void CGPSReceiver::slotInit()
{
	disconnect(m_pTimer);
	open();
}

void CGPSReceiver::slotConnected()
{
	m_iCounter = 0;
	Parameters.Lock(); 
	Parameters.GPSData.SetStatus(CGPSData::GPS_RX_NO_DATA);
	Parameters.Unlock(); 
	// clear current buffer
	while(m_pSocket->canReadLine())
		m_pSocket->readLine();

	m_pSocket->writeBlock("W1\n",2);	// try to force gpsd into watcher mode
	connect( m_pTimer, SIGNAL(timeout()), SLOT(slotTimeout()) );
	m_pTimer->start(c_usReconnectIntervalSeconds*1000);
}

void CGPSReceiver::slotTimeout()
{
	m_iCounter--;
	if(m_iCounter==0)
	{
		disconnect(m_pTimer);
		close();
		open();
	}
	else
	{
		Parameters.Lock(); 
		Parameters.GPSData.SetStatus(CGPSData::GPS_RX_NO_DATA);
		Parameters.Unlock(); 
	}
}

void CGPSReceiver::slotReadyRead()
{
	m_iCounter = c_usReconnectIntervalSeconds/5;
	Parameters.Lock(); 
	Parameters.GPSData.SetStatus(CGPSData::GPS_RX_DATA_AVAILABLE);
	Parameters.Unlock(); 
	while (m_pSocket->canReadLine())
		DecodeGPSDReply((const char*) m_pSocket->readLine());
	m_pTimer->start(5*1000, TRUE); // if no data in 30 seconds abort
}

void CGPSReceiver::slotSocketError(int)
{
	close();
	connect( m_pTimer, SIGNAL(timeout()), SLOT(slotInit()) );
	m_pTimer->start(c_usReconnectIntervalSeconds*1000, TRUE);
}
