/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright(c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *
 * This is an implementation of the CPacketSocket interface that wraps up a QT socket.
 *
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or(at your option) any later
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

#include "PacketSocketQT.h"
#include <qstringlist.h>
#include <qtimer.h>
#include <iostream>
#include <sstream>

#include <errno.h>
#ifndef _WIN32
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

#ifdef _WIN32
/* Always include winsock2.h before windows.h */
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
#endif
#if QT_VERSION >= 0x040000
# include <QUdpSocket>
# include <QTcpSocket>
#endif

/* Some defines needed for compatibility when using Linux */
#ifndef _WIN32
typedef int SOCKET;
# define SOCKET_ERROR				(-1)
# define INVALID_SOCKET				(-1)
#endif

CPacketSocketQT::CPacketSocketQT(bool udp):
#if QT_VERSION < 0x040000
	pPacketSink(NULL), HostAddrOut(), iHostPortOut(-1),
	SocketDevice(udp?QSocketDevice::Datagram:QSocketDevice::Stream),
	pSocketNotivRead(NULL), pSocketNotivWrite(NULL),
	writeLock(),writeBuf()
#else
	HostAddrOut(), iHostPortOut(-1), pSocket(
		udp?((QAbstractSocket*)new QUdpSocket())
		:((QAbstractSocket*)new QTcpSocket())),
	writeLock(),writeBuf()
#endif
{

#if QT_VERSION < 0x040000
	// reading is always asynchronous and uses QSocketNotifier
	pSocketNotivRead = new QSocketNotifier(SocketDevice.socket(), QSocketNotifier::Read);

	QObject::connect(pSocketNotivRead, SIGNAL(activated(int)),
					  this, SLOT(OnDataReceived()));

	// UDP can get away with blocking mode but TCP needs async I/O
	if(!udp)
	{
		pSocketNotivWrite = new QSocketNotifier(SocketDevice.socket(), QSocketNotifier::Write);
		QObject::connect(pSocketNotivWrite, SIGNAL(activated(int)),
						  this, SLOT(OnWritePossible()));
		pSocketNotivWrite->setEnabled(false);
	}
	/* allow connection when others are listening */
	SocketDevice.setAddressReusable(true);
#else
	QObject::connect(pSocket, SIGNAL(readyRead()), this, SLOT(OnDataReceived()));
#endif
}

CPacketSocketQT::~CPacketSocketQT()
{
#if QT_VERSION < 0x040000
	if(pSocketNotivRead)
	{
		pSocketNotivRead->disconnect();
		delete pSocketNotivRead;
	}
#endif
}

// Set the sink which will receive the packets
void
CPacketSocketQT::SetPacketSink(CPacketSink * pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void
CPacketSocketQT::ResetPacketSink(void)
{
	pPacketSink = NULL;
}

// Send packet to the socket
void
CPacketSocketQT::SendPacket(const vector < _BYTE > &vecbydata, uint32_t addr, uint16_t port)
{
	int bytes_written;
	/* Send packet to network */
	//cout << "CPacketSocketQT::SendPacket(" << vecbydata.size() << " bytes, " << addr << ", " << port << ") " << HostAddrOut.toString() << ":" << iHostPortOut << endl;

#if QT_VERSION < 0x040000
	if(SocketDevice.type() == QSocketDevice::Datagram)
	{
		if(addr==0)
			bytes_written = SocketDevice.writeBlock((char*)&vecbydata[0], vecbydata.size(), HostAddrOut, iHostPortOut);
		else
			bytes_written = SocketDevice.writeBlock((char*)&vecbydata[0], vecbydata.size(), QHostAddress(addr), port);
		/* should we throw an exception or silently accept? */
		/* the most likely cause is that we are sending unicast and no-one
		   is listening, or the interface is down, there is no route */
		if(bytes_written == -1)
		{
			QSocketDevice::Error x = SocketDevice.error();
			if(x != QSocketDevice::NetworkFailure)
				qDebug("error sending packet");
		}
	}
	else
	{
		writeLock.lock();
		for(size_t i=0; i<vecbydata.size(); i++)
			writeBuf.push_back(vecbydata[i]);
		writeLock.unlock();
		pSocketNotivWrite->setEnabled(true);
	}
#else
	QUdpSocket* s = dynamic_cast<QUdpSocket*>(pSocket);
	if(s != NULL)
		bytes_written = s->writeDatagram((char*)&vecbydata[0], vecbydata.size(), HostAddrOut, iHostPortOut);
	else
		bytes_written = pSocket->write((char*)&vecbydata[0], vecbydata.size());
#endif
}

QStringList
CPacketSocketQT::parseDest(const string & strNewAddr)
{
#if QT_VERSION < 0x040000
	return QStringList::split(":", strNewAddr.c_str(), TRUE);
#else
	return QString(strNewAddr.c_str()).split(":", QString::KeepEmptyParts);
#endif
}

_BOOLEAN
CPacketSocketQT::SetDestination(const string & strNewAddr)
{
	/* syntax
	   1:  <port>                send to port on localhost
	   2:  <ip>:<port>           send to port on host or port on m/c group
	   3:  <ip>:<ip>:<port>      send to port on m/c group via interface
	 */
	/* Init return flag and copy string in QT-String "QString" */
	int ttl = 127;
	_BOOLEAN bAddressOK = TRUE;
#if QT_VERSION >= 0x040000
	QUdpSocket* pUdps = dynamic_cast<QUdpSocket*>(pSocket);
#endif
	QStringList parts = parseDest(strNewAddr);
	switch(parts.count())
	{
	case 1: // Just a port - send to ourselves
		bAddressOK = HostAddrOut.setAddress("127.0.0.1");
		iHostPortOut = parts[0].toUInt();
		break;
	case 2: // host and port, unicast
		bAddressOK = HostAddrOut.setAddress(parts[0]);
		iHostPortOut = parts[1].toUInt();
#if QT_VERSION < 0x040000
		if(SocketDevice.type() == QSocketDevice::Stream)
		{
			bool connected = SocketDevice.connect(HostAddrOut, iHostPortOut);
			if(!connected)
			{
				if(SocketDevice.error()!=0)
				{
					cerr << int(SocketDevice.error()) << endl;
					bAddressOK = FALSE;
				}
			}
		}
		else
		{
			if(setsockopt(SocketDevice.socket(), IPPROTO_IP, IP_TTL,
				(char*)&ttl, sizeof(ttl))==SOCKET_ERROR)
				bAddressOK = FALSE;
		}
#else
		if(pUdps != NULL)
		{
# if QT_VERSION < 0x040800
			if(setsockopt(pSocket->socketDescriptor(), IPPROTO_IP, IP_TTL,
					(char*)&ttl, sizeof(ttl))==SOCKET_ERROR)
			bAddressOK = FALSE;
# else
			pUdps->setSocketOption(QAbstractSocket::MulticastTtlOption, ttl);
# endif
		}
		else
		{
			pSocket->connectToHost(HostAddrOut, iHostPortOut);
			bAddressOK = pSocket->waitForConnected(5000);
		}
#endif
		break;
	case 3:
		{
			QHostAddress AddrInterface;
			AddrInterface.setAddress(parts[0]);
			bAddressOK = HostAddrOut.setAddress(parts[1]);
			iHostPortOut = parts[2].toUInt();
#if QT_VERSION < 0x040800
# if QT_VERSION < 0x040000
			const SOCKET s = SocketDevice.socket();
# else
			const SOCKET s = pSocket->socketDescriptor();
# endif
# if QT_VERSION < 0x030000
			uint32_t mc_if = htonl(AddrInterface.ip4Addr());
# else
			uint32_t mc_if = htonl(AddrInterface.toIPv4Address());
# endif
			if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF,
						(char *) &mc_if, sizeof(mc_if)) == SOCKET_ERROR)
				bAddressOK = FALSE;
			if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL,
							(char*) &ttl, sizeof(ttl)) == SOCKET_ERROR)
					bAddressOK = FALSE;
#else
            if(pUdps != NULL)
			{
                pUdps->setMulticastInterface(GetInterface(AddrInterface));
                pUdps->setSocketOption(QAbstractSocket::MulticastTtlOption, ttl);
			}
#endif
		}
		break;
	default:
		bAddressOK = FALSE;
	}
	return bAddressOK;
}


_BOOLEAN
CPacketSocketQT::GetDestination(string & str)
{
	stringstream s;
#if QT_VERSION < 0x040000
	s << HostAddrOut.toString().latin1() << ":" << iHostPortOut;
#else
	s << HostAddrOut.toString().toLatin1().constData() << ":" << iHostPortOut;
#endif
	str = s.str();
	return TRUE;
}


_BOOLEAN
CPacketSocketQT::SetOrigin(const string & strNewAddr)
{
	/* syntax (unwanted fields can be empty, e.g. <source ip>::<group ip>:<port>
	   1:  <port>
	   2:  <group ip>:<port>
	   3:  <interface ip>:<group ip>:<port>
	   4:  <source ip>:<interface ip>:<group ip>:<port>
	   5: - for TCP - no need to separately set origin
	 */

	if(strNewAddr == "-")
	{
		return TRUE;
	}

	int iPort=-1;
	QHostAddress AddrGroup, AddrInterface, AddrSource;
	QStringList parts = parseDest(strNewAddr);
	bool ok=true;
	switch(parts.count())
	{
	case 1:
		iPort = parts[0].toUInt(&ok);
		break;
	case 2:
		iPort = parts[1].toUInt(&ok);
		ok &= AddrGroup.setAddress(parts[0]);
		break;
	case 3:
		iPort = parts[2].toUInt(&ok);
		if(parts[0].length() > 0)
			ok &= AddrInterface.setAddress(parts[0]);
		if(parts[1].length() > 0)
			ok &= AddrGroup.setAddress(parts[1]);
		break;
	case 4:
		iPort = parts[3].toUInt(&ok);
		if(parts[0].length() > 0)
			ok &= AddrSource.setAddress(parts[0]);
		if(parts[1].length() > 0)
			ok &= AddrInterface.setAddress(parts[1]);
		if(parts[2].length() > 0)
			ok &= AddrGroup.setAddress(parts[2]);
		break;
	default:
		ok = false;
	}

	if(ok)
	{
		return doSetSource(AddrGroup, AddrInterface, iPort, AddrSource);
	}
	return FALSE;
}

#if QT_VERSION < 0x040000
_BOOLEAN CPacketSocketQT::doSetSource(QHostAddress AddrGroup, QHostAddress AddrInterface, int iPort, QHostAddress AddrSource)
{
	bool udp = SocketDevice.type() == QSocketDevice::Datagram;
# if QT_VERSION < 0x030000
		sourceAddr = AddrSource.ip4Addr();
# else
		sourceAddr = AddrSource.toIPv4Address();
# endif
	AddrSource = source;
	SOCKET s = SocketDevice.socket();
	if(udp)
	{
		/* Multicast ? */
# if QT_VERSION < 0x030000
		uint32_t gp = AddrGroup.ip4Addr();
# else
		uint32_t gp = AddrGroup.toIPv4Address();
# endif
		if(gp == 0)
		{
			/* Initialize the listening socket. */
			SocketDevice.bind(AddrInterface, iPort);
		}
		else if((gp & 0xe0000000) == 0xe0000000)	/* multicast! */
		{
			struct ip_mreq mreq;
			/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
			bool ok = SocketDevice.bind(QHostAddress(UINT32(0)), iPort);
			if(ok == false)
			{
				//QSocketDevice::Error x = SocketDevice.error();
				throw CGenErr("Can't bind to port to receive packets");
			}
#if QT_VERSION < 0x030000
			mreq.imr_multiaddr.s_addr = htonl(AddrGroup.ip4Addr());
			mreq.imr_interface.s_addr = htonl(AddrInterface.ip4Addr());
#else
			mreq.imr_multiaddr.s_addr = htonl(AddrGroup.toIPv4Address());
			mreq.imr_interface.s_addr = htonl(AddrInterface.toIPv4Address());
#endif
			int n = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *) &mreq,
								sizeof(mreq));
			if(n == SOCKET_ERROR)
			{
				throw
					CGenErr(string
							("Can't join multicast group to receive packets: ") +
							strerror(errno));
			}
		}
		else /* one address specified, but not multicast - listen on a specific interface */
		{
			/* Initialize the listening socket. */
			SocketDevice.bind(AddrGroup, iPort);
		}
	}
	return TRUE;
}
#else
_BOOLEAN CPacketSocketQT::doSetSource(QHostAddress AddrGroup, QHostAddress AddrInterface, int iPort, QHostAddress AddrSource)
{
	bool udp = true;
	QUdpSocket* pUdps = dynamic_cast<QUdpSocket*>(pSocket);
	sourceAddr = AddrSource.toIPv4Address();
	if(pUdps == NULL)
		udp = false;
	if(udp)
	{
		/* Multicast ? */
		uint32_t gp = AddrGroup.toIPv4Address();
		if(gp == 0)
		{
			/* Initialize the listening socket. */
			pUdps->bind(AddrInterface, iPort);
		}
		else if((gp & 0xe0000000) == 0xe0000000)	/* multicast! */
		{

			/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" 
    udpSocket->joinMulticastGroup(groupAddress);

    connect(udpSocket, SIGNAL(readyRead()),this, SLOT(processPendingDatagrams()));

			*/
			bool ok = pUdps->bind(iPort, QUdpSocket::ShareAddress);
			if(ok == false)
			{
				throw CGenErr("Can't bind to port to receive packets");
			}
#if QT_VERSION < 0x040800
			struct ip_mreq mreq;
			mreq.imr_multiaddr.s_addr = htonl(AddrGroup.toIPv4Address());
			mreq.imr_interface.s_addr = htonl(AddrInterface.toIPv4Address());
			int n = setsockopt(pUdps->socketDescriptor(), IPPROTO_IP, IP_ADD_MEMBERSHIP,(char *) &mreq,	sizeof(mreq));
			if(n == SOCKET_ERROR)
				ok = false;
#else
			//ok = pUdps->joinMulticastGroup(AddrGroup, GetInterface(AddrInterface));
			ok = pUdps->joinMulticastGroup(AddrGroup);
#endif
			if(!ok)
			{
				qDebug("Can't join multicast group");
				//throw CGenErr(string());
			}
		}
		else /* one address specified, but not multicast - listen on a specific interface */
		{
			/* Initialize the listening socket. */
			pUdps->bind(AddrGroup, iPort);
		}
	}
	return TRUE;
}
#endif

#if QT_VERSION >= 0x040200
QNetworkInterface 
CPacketSocketQT::GetInterface(QHostAddress AddrInterface)
{
	QList<QNetworkInterface> list = QNetworkInterface::allInterfaces ();
	QList<QNetworkInterface>::const_iterator ifIt;
	for( ifIt = list.begin(); ifIt != list.end(); ++ifIt )
	{
		QList<QNetworkAddressEntry> addresses = ifIt->addressEntries();
		QList<QNetworkAddressEntry>::const_iterator i;
		for(i = addresses.begin(); i!=addresses.end(); ++i)
		{
			if(i->ip() == AddrInterface)
			{
				return *ifIt;
			}
		}
	}
	return QNetworkInterface::allInterfaces().first();
}
#endif

void
CPacketSocketQT::OnDataReceived()
{
	//cerr << "DataReceived" << endl;
	vector < _BYTE > vecbydata(MAX_SIZE_BYTES_NETW_BUF);

	/* Read block from network interface */
#if QT_VERSION < 0x040000
	int iNumBytesRead = SocketDevice.readBlock((char *) &vecbydata[0], MAX_SIZE_BYTES_NETW_BUF);
#else
	int iNumBytesRead = pSocket->read((char *) &vecbydata[0], MAX_SIZE_BYTES_NETW_BUF);
#endif
	if(iNumBytesRead > 0)
	{
		/* Decode the incoming packet */
		if(pPacketSink != NULL)
		{
			vecbydata.resize(iNumBytesRead);
#if QT_VERSION < 0x040000
			QHostAddress peer = SocketDevice.peerAddress();
# if QT_VERSION < 0x030000
			uint32_t addr = peer.ip4Addr();
# else
			uint32_t addr = peer.toIPv4Address();
# endif
			int port = SocketDevice.peerPort();
#else
			QHostAddress peer = pSocket->peerAddress();
			uint32_t addr = peer.toIPv4Address();
			int port = pSocket->peerPort();
#endif
			if(sourceAddr == 0 || sourceAddr == addr) // optionally filter on source address
				pPacketSink->SendPacket(vecbydata, addr, port);
		}
	}
}

#if QT_VERSION < 0x040000
void
CPacketSocketQT::OnWritePossible()
{
	//cerr << "OnWritePossible()" << endl;
	writeLock.lock();
	SocketDevice.writeBlock((char*)&writeBuf[0], writeBuf.size());
	writeBuf.clear();
	writeLock.unlock();
	pSocketNotivWrite->setEnabled(false);
}
#endif
