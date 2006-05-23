/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
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

#include "PacketSocketQT.h"
#include <qstringlist.h>
#include <errno.h>
#include <iostream>

CPacketSocketQT::CPacketSocketQT() : SocketDevice(QSocketDevice::Datagram /* UDP */)
{
	/* Generate the socket notifier and connect the signal */
	pSocketNotivRead = new QSocketNotifier(SocketDevice.socket(),
		QSocketNotifier::Read);

	/* Connect the "activated" signal */
    QObject::connect(pSocketNotivRead, SIGNAL(activated(int)),
		this, SLOT(OnDataReceived()));
}

// Set the sink which will receive the packets
void CPacketSocketQT::SetPacketSink(CPacketSink *pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void CPacketSocketQT::ResetPacketSink(void)
{
	pPacketSink = NULL;
}

// Send packet to the socket
void CPacketSocketQT::SendPacket(const vector<_BYTE>& vecbydata)
{
	/* Send packet to network */
	char *p = new char[vecbydata.size()];
	for(size_t i=0; i<vecbydata.size(); i++)
	p[i]=vecbydata[i];
	uint32_t bytes_written = SocketDevice.writeBlock(p, vecbydata.size(),
		HostAddrOut, iHostPortOut);
	if(bytes_written==-1)
	    cout << "error sending packet : " << SocketDevice.error() << endl;
}

_BOOLEAN CPacketSocketQT::SetNetwOutAddr(const string& strNewAddr)
{
	/* syntax
	1:  <ip>:<port>
	2:  <ip>:<ip>:<port>
     */
	/* Init return flag and copy string in QT-String "QString" */
	_BOOLEAN bAddressOK = FALSE;
	QStringList parts = QStringList::split(":", strNewAddr.c_str(), TRUE);
	switch(parts.count()) {
      case 2:
           bAddressOK = HostAddrOut.setAddress(parts[0]);
           iHostPortOut = parts[1].toInt();
           break;
      case 3:
		QHostAddress AddrInterface;
	  	AddrInterface.setAddress(parts[0]);
           bAddressOK = HostAddrOut.setAddress(parts[1]);
           iHostPortOut = parts[2].toInt();
           const SOCKET s = SocketDevice.socket();
#if QT_VERSION > 230
		   uint32_t mc_if = htonl(AddrInterface.toIPv4Address());
#else
		   uint32_t mc_if = htonl(inet_addr(AddrInterface.toString().toLatin1()));
#endif
           if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, 
             (char*) &mc_if, sizeof(mc_if))==SOCKET_ERROR)
               bAddressOK = FALSE;
           break;
    }
	return bAddressOK;
}


_BOOLEAN CPacketSocketQT::SetNetwInAddr(const string& strNewAddr)
{

	/* syntax
	1:  <port>
	2:  <ip>:<port>
	3:  <ip>:<ip>:<port>
	4:  <ip>::<port>
     */
	int iPort;
	QHostAddress AddrGroup, AddrInterface;
	/* Init return flag and copy string to a QString */
	_BOOLEAN bAddressOK = FALSE;
	QStringList parts = QStringList::split(":", strNewAddr.c_str(), TRUE);
	switch(parts.count()) {
      case 1:
           iPort = parts[0].toInt();
           break;
      case 2:
           AddrGroup.setAddress(parts[0]);
           iPort = parts[1].toInt();
           break;
      case 3:
           if(parts[0].length()>0)
               AddrInterface.setAddress(parts[0]);
           if(parts[1].length()>0)
               AddrGroup.setAddress(parts[1]);
           iPort = parts[2].toUInt();
           break;
    }

	/* Multicast ? */
	
#if QT_VERSION > 230
	uint32_t gp = htonl(AddrGroup.toIPv4Address());
#else
	uint32_t gp = htonl(inet_addr(AddrGroup.toString.toLatin1()));
#endif
	if (gp == 0)
	{
                                  
		/* Initialize the listening socket. */
		SocketDevice.bind(AddrInterface, iPort);
    }
	else
	{
		struct ip_mreq mreq;
		
		/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
        bool ok = SocketDevice.bind(QHostAddress(UINT32(0)), iPort);
        if(ok == false) {
		QSocketDevice::Error x = SocketDevice.error();
		cout << "bind failed" << endl;
		                     return FALSE;
                             }
  
#if QT_VERSION > 230
		mreq.imr_multiaddr.s_addr = htonl(AddrGroup.toIPv4Address());
		mreq.imr_interface.s_addr = htonl(AddrInterface.toIPv4Address());
#else
		mreq.imr_multiaddr.s_addr = htonl(inet_addr(AddrGroup.toString.toLatin1()));
		mreq.imr_interface.s_addr = htonl(inet_addr(AddrInterface.toString.toLatin1()));
#endif
        const SOCKET s = SocketDevice.socket();
        int n = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
				sizeof(mreq));
		if(n==SOCKET_ERROR) {
			cout << strerror(errno) << endl;
			return FALSE;
		}
		return TRUE;
	}

	return TRUE;
}

void CPacketSocketQT::OnDataReceived()
{
	vector<_BYTE> vecsRecBuf(MAX_SIZE_BYTES_NETW_BUF);

	/* Read block from network interface */
	const int iNumBytesRead = SocketDevice.readBlock(
		(char*) &vecsRecBuf[0], MAX_SIZE_BYTES_NETW_BUF);
	vecsRecBuf.resize(iNumBytesRead);

	if (iNumBytesRead > 0)
	{
		/* Decode the incoming packet */
		if (pPacketSink != NULL)
			pPacketSink->SendPacket(vecsRecBuf);
	}
}
