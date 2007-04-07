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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/


#include "GPSReceiver.h"

#ifdef USE_QT_GUI
# include <qsocket.h>
# include <qsignal.h>
# include <qtimer.h>
#endif

#include <sstream>
#include <iomanip>
using namespace std;

const unsigned short CGPSReceiver::c_usReconnectIntervalSeconds = 30;

CGPSReceiver::CGPSReceiver(CGPSData& data): m_GPSData(data),m_pSocket(NULL)
{	
	open();
}

CGPSReceiver::~CGPSReceiver()
{
	close();
}

void CGPSReceiver::open()
{
	m_GPSData.SetStatus(CGPSData::GPS_RX_NOT_CONNECTED);
	if(m_pSocket == NULL)
	{
		m_pSocket = new QSocket();
		connect(m_pSocket, SIGNAL(connected()), this, SLOT(slotConnected()));
		connect(m_pSocket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
		connect(m_pSocket, SIGNAL(error(int)), this, SLOT(slotSocketError(int)));
	}
	m_pSocket->connectToHost(m_GPSData.host.c_str(), m_GPSData.port);
}

void CGPSReceiver::close()
{
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
	if (Value[0] == '?')
	{
		m_GPSData.SetPositionAvailable(FALSE);
		m_GPSData.SetAltitudeAvailable(FALSE);
		m_GPSData.SetTimeAndDateAvailable(FALSE);
		m_GPSData.SetHeadingAvailable(FALSE);
		m_GPSData.SetSpeedAvailable(FALSE);
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

	m_GPSData.SetLatLongDegrees(fLatitude, fLongitude);
	m_GPSData.SetPositionAvailable(TRUE);

	if (sTime.find('?') == string::npos)
	{
		stringstream ssTime(sTime);
		unsigned long ulTime;
		ssTime >> ulTime;
		m_GPSData.SetTimeSecondsSince1970(ulTime);
		m_GPSData.SetTimeAndDateAvailable(TRUE);
	}
	else
	{
		m_GPSData.SetTimeAndDateAvailable(FALSE);
	}

	if (sAltitude.find('?') == string::npos)	// if '?' not found..
	{
		m_GPSData.SetAltitudeAvailable(TRUE);
		m_GPSData.SetAltitudeMetres(atof(sAltitude.c_str()));
	}
	else
		m_GPSData.SetAltitudeAvailable(FALSE);

	if (sHeading.find('?') == string::npos)
	{
		m_GPSData.SetHeadingAvailable(TRUE);
		m_GPSData.SetHeadingDegrees((unsigned short) atof(sHeading.c_str()));
	}
	else
		m_GPSData.SetHeadingAvailable(FALSE);

	if (sSpeed.find('?') == string::npos)
	{
		m_GPSData.SetSpeedAvailable(TRUE);
		m_GPSData.SetSpeedMetresPerSecond(atof(sSpeed.c_str()));
	}
	else
		m_GPSData.SetSpeedAvailable(FALSE);

}

void CGPSReceiver::DecodeY(string Value)
{
	if (Value[0] == '?')
	{
		m_GPSData.SetSatellitesVisibleAvailable(FALSE);
		m_GPSData.SetTimeAndDateAvailable(FALSE);
		return;
	}

	string sTag, sTimestamp;
	unsigned short usSatellites;
	stringstream ssValue(Value);

	ssValue >> sTag >> sTimestamp >> usSatellites;

	m_GPSData.SetSatellitesVisible(usSatellites);
	m_GPSData.SetSatellitesVisibleAvailable(TRUE);

	//todo - timestamp//

}

void CGPSReceiver::slotInit()
{
	open();
}

void CGPSReceiver::slotConnected()
{
	m_GPSData.SetStatus(CGPSData::GPS_RX_NO_DATA);
	//clear current buffer
	while(m_pSocket->canReadLine())
		m_pSocket->readLine();

	m_pSocket->writeBlock("W1\n",2);	// try to force gpsd into watcher mode
	QTimer::singleShot(30000, this, SLOT(slotConnected()));
}

void CGPSReceiver::slotAbort()
{
	close();
	QTimer::singleShot(30000, this, SLOT(slotInit()));
}

void CGPSReceiver::slotReadyRead()
{
	m_GPSData.SetStatus(CGPSData::GPS_RX_DATA_AVAILABLE);
	while (m_pSocket->canReadLine())
		DecodeGPSDReply((const char*) m_pSocket->readLine());
	QTimer::singleShot(30000, this, SLOT(slotConnected()));
}

void CGPSReceiver::slotSocketError(int)
{
	close();
	QTimer::singleShot(30000, this, SLOT(slotInit()));
}
