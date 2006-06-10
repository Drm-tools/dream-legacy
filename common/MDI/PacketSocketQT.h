/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	Implementation of CPacketSocket interface that wraps up a QSocket. See PacketSocketQT.cpp.
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

#ifndef PACKET_SOCKET_QT_H_INCLUDED
#define PACKET_SOCKET_QT_H_INCLUDED

#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../util/Buffer.h"

#include <qsocketdevice.h>
#include <qsocketnotifier.h>
#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
#else
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

/* Some defines needed for compatibility when using Linux */
#ifndef _WIN32
typedef int SOCKET;
# define SOCKET_ERROR				(-1)
# define INVALID_SOCKET				(-1)
#endif

/* Maximum number of bytes received from the network interface. Maximum data
   rate of DRM is approx. 80 kbps. One MDI packet must be sent each DRM frame
   which is every 400 ms -> 0.4 * 80000 / 8 = 4000 bytes. Allocating more than
   double of this size should be ok for all possible cases */
#define MAX_SIZE_BYTES_NETW_BUF		10000


#include "PacketInOut.h"

class CPacketSocketQT : public QObject, public CPacketSocket
{
	Q_OBJECT

public:
	CPacketSocketQT();
	// Set the sink which will receive the packets
	virtual void SetPacketSink(CPacketSink *pSink);
	// Stop sending packets to the sink
	virtual void ResetPacketSink(void);

	// Send packet to the socket
	virtual void SendPacket(CVector<_BINARY> vecbiPacket);

	virtual _BOOLEAN SetNetwOutAddr(const string strNewIPPort);
	virtual _BOOLEAN SetNetwInPort(const int iPort);
	virtual _BOOLEAN SetNetwInMcast(const string strNewIPIP);

private:
	CPacketSink *pPacketSink;

	QHostAddress				HostAddrOut;
	int							iHostPortOut;

	QSocketDevice				SocketDevice;
	QSocketNotifier*			pSocketNotivRead;

	CMutex						Mutex;

public slots:
	void OnDataReceived();

};

#endif
