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

#include <qsignal.h>

#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;


const short CGPSReceiver::c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset = 10;
const unsigned short CGPSReceiver::c_usReconnectIntervalSeconds = 10;

CGPSReceiver::CGPSReceiver() :
	m_GPSdPort(2947),
	m_GPSdHostName("localhost")
{
	m_bFinished = FALSE;
	m_eGPSState = DISCONNECTED;

	m_pGPSRxData = &m_GPSRxData;

	//connect signals
	connect(&m_Socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()));
	connect(&m_Socket, SIGNAL(error(int)), this, SLOT(slotSocketError(int)));

	m_sStateMachineCyclesSinceLastGPSDReply = c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset;
}

CGPSReceiver::~CGPSReceiver()
{
}

void CGPSReceiver::run()
{
	try
	{
		// Call GPS main routine 
		Start();
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
}

void CGPSReceiver::Start()
{
	while (!m_bFinished)
	{
		switch(m_eGPSState)
		{
			case DISCONNECTED:		// attempt to connect
				m_pGPSRxData->SetStatus(CGPSRxData::GPS_RX_NOT_CONNECTED);

				m_sStateMachineCyclesSinceLastGPSDReply = c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset;

				switch(m_Socket.state())
				{
					case(QSocket::Idle):		// stay put
						m_Socket.connectToHost(m_GPSdHostName.c_str(), m_GPSdPort);
						break;
					case(QSocket::HostLookup):	// stay put, wait for open
						usleep(100000);
						break;
					case(QSocket::Connecting):
						usleep(100000);
						break;
					case (QSocket::Closing):		// problem sleep for retry interval and stay put
						sleep(1000*c_usReconnectIntervalSeconds);
						break;
					case(QSocket::Connection):		// change state to initialise
						m_eGPSState = INITIALISING;
						break;
				}
				break;

			case INITIALISING:
				m_pGPSRxData->SetStatus(CGPSRxData::GPS_RX_NO_DATA);

				m_sStateMachineCyclesSinceLastGPSDReply = c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset;

				switch(m_Socket.state())
				{
					case(QSocket::Idle):
					case(QSocket::HostLookup):
					case(QSocket::Connecting):
					case(QSocket::Closing):		// problem, sleep for retry interval
						m_eGPSState = DISCONNECTED;
						sleep(1000*c_usReconnectIntervalSeconds);
						break;
					case (QSocket::Connection):	// send init string
						//clear current buffer
						while(m_Socket.canReadLine())
							m_Socket.readLine();

						m_Socket.writeBlock("W1\n",2);	// try to force gpsd into watcher mode
						usleep(100000);								// sleep for 2 seconds
						m_eGPSState = WAITING;
						break;
				}
				break;

			case WAITING:
				m_pGPSRxData->SetStatus(CGPSRxData::GPS_RX_NO_DATA);
				
				if (m_Socket.state() != QSocket::Connection)
					m_eGPSState = DISCONNECTED;
		
				if (m_sStateMachineCyclesSinceLastGPSDReply >= c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset)
					m_eGPSState = INITIALISING;							// need to resend "W1" command
				else
					m_eGPSState = STREAMING;
				
				break;

			case STREAMING:
				m_pGPSRxData->SetStatus(CGPSRxData::GPS_RX_DATA_AVAILABLE);
				if (m_Socket.state() != QSocket::Connection)
					m_eGPSState = DISCONNECTED;

				if (m_sStateMachineCyclesSinceLastGPSDReply >= c_sMaximumStateMachineCyclesSinceLastGPSDReplyBeforeReset)
				{
					m_eGPSState = DISCONNECTED;		// give up, should have been in watcher mode but something terrible has happened, try to reopen socket to see if that helps!
					m_Socket.close();
				}
				
				//otherwise stay put
				break;

			case COMMS_ERROR:
				m_Socket.close(); // try and free the resources - maybe we need to destroy the socket ?
				sleep(30);		// sleep for 30 seconds
				m_eGPSState = DISCONNECTED;
				break;
		}

		usleep(500000);		// sleep for half a second
		m_sStateMachineCyclesSinceLastGPSDReply++;
	}

}

void CGPSReceiver::DecodeGPSDReply(string Reply)
{
	m_sStateMachineCyclesSinceLastGPSDReply = 0;

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
		m_pGPSRxData->SetPositionAvailable(FALSE);
		m_pGPSRxData->SetAltitudeAvailable(FALSE);
		m_pGPSRxData->SetTimeAndDateAvailable(FALSE);
		m_pGPSRxData->SetHeadingAvailable(FALSE);
		m_pGPSRxData->SetSpeedAvailable(FALSE);
		return;
	}

	string sTag, sTime, sTimeError;
	float fLatitude, fLongitude;
	string sAltitude, sErrorHoriz, sErrorVert;
	string sHeading, sSpeed, sClimb, sHeadingError, sSpeedError, sClimbError;

	stringstream ssValue(Value);

	ssValue >> sTag >> sTime >> sTimeError >> fLatitude >> fLongitude >> sAltitude;
	ssValue >> sErrorHoriz >> sErrorVert >> sHeading >> sSpeed >> sClimb >> sHeadingError;
	ssValue >> sSpeedError >> sClimbError;

	m_pGPSRxData->SetLatLongDegrees(fLatitude, fLongitude);
	m_pGPSRxData->SetPositionAvailable(TRUE);

	if (sTime.find('?') == string::npos)
	{
		stringstream ssTime(sTime);
		unsigned long ulTime;
		ssTime >> ulTime;
		m_pGPSRxData->SetTimeSecondsSince1970(ulTime);
		m_pGPSRxData->SetTimeAndDateAvailable(TRUE);
	}
	else
	{
		m_pGPSRxData->SetTimeAndDateAvailable(FALSE);
	}

	if (sAltitude.find('?') == string::npos)	// if '?' not found..
	{
		m_pGPSRxData->SetAltitudeAvailable(TRUE);
		m_pGPSRxData->SetAltitudeMetres(atof(sAltitude.c_str()));
	}
	else
		m_pGPSRxData->SetAltitudeAvailable(FALSE);

	if (sHeading.find('?') == string::npos)
	{
		m_pGPSRxData->SetHeadingAvailable(TRUE);
		m_pGPSRxData->SetHeadingDegrees((unsigned short) atof(sHeading.c_str()));
	}
	else
		m_pGPSRxData->SetHeadingAvailable(FALSE);

	if (sSpeed.find('?') == string::npos)
	{
		m_pGPSRxData->SetSpeedAvailable(TRUE);
		m_pGPSRxData->SetSpeedMetresPerSecond(atof(sSpeed.c_str()));
	}
	else
		m_pGPSRxData->SetSpeedAvailable(FALSE);

}

void CGPSReceiver::DecodeY(string Value)
{
	if (Value[0] == '?')
	{
		m_pGPSRxData->SetSatellitesVisibleAvailable(FALSE);
		m_pGPSRxData->SetTimeAndDateAvailable(FALSE);
		return;
	}

	string sTag, sTimestamp;
	unsigned short usSatellites;
	stringstream ssValue(Value);

	ssValue >> sTag >> sTimestamp >> usSatellites;

	m_pGPSRxData->SetSatellitesVisible(usSatellites);
	m_pGPSRxData->SetSatellitesVisibleAvailable(TRUE);

	//todo - timestamp//

}


void CGPSReceiver::slotReadyRead()
{
	while (m_Socket.canReadLine())
		DecodeGPSDReply((const char*) m_Socket.readLine());
}

void CGPSReceiver::slotSocketError(int)
{
	m_eGPSState = COMMS_ERROR;
}

#endif
