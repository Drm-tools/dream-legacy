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
#include <qtimer.h>
#include <iostream>
#include <errno.h>

CPacketSocketQT::CPacketSocketQT ():
	pPacketSink(NULL), HostAddrOut(), iHostPortOut(-1),
	SocketDevice (QSocketDevice:: Datagram /* UDP */ ),
	pSocketNotivRead(NULL)
#if defined(HAVE_LIBWTAP) || defined(HAVE_LIBPCAP)
			  , pf(NULL)
#endif
{
	/* Generate the socket notifier and connect the signal */
	pSocketNotivRead = new QSocketNotifier (SocketDevice.socket (),
											QSocketNotifier::Read);

	/* Connect the "activated" signal */
	QObject::connect (pSocketNotivRead, SIGNAL (activated (int)),
					  this, SLOT (OnDataReceived ()));
					  
	/* allow connection when others are listening */
	SocketDevice.setAddressReusable(true);
}

CPacketSocketQT::~CPacketSocketQT()
{
	if(pPacketSink)
		delete pPacketSink;

	if(pSocketNotivRead)
	{
		pSocketNotivRead->disconnect();
		delete pSocketNotivRead;
	}

#ifdef HAVE_LIBPCAP
	if(pf)
		pcap_close(pf);
#endif
#ifdef HAVE_LIBWTAP
	if(pf)
		wtap_close(pf);
#endif
}

// Set the sink which will receive the packets
void
CPacketSocketQT::SetPacketSink (CPacketSink * pSink)
{
	pPacketSink = pSink;
}

// Stop sending packets to the sink
void
CPacketSocketQT::ResetPacketSink (void)
{
	pPacketSink = NULL;
}

// Send packet to the socket
void
CPacketSocketQT::SendPacket (const vector < _BYTE > &vecbydata)
{
	/* Send packet to network */
	char *p = new char[vecbydata.size ()];
	for (size_t i = 0; i < vecbydata.size (); i++)
		p[i] = vecbydata[i];
	int bytes_written = SocketDevice.writeBlock (p, vecbydata.size (),
													  HostAddrOut,
													  iHostPortOut);
	/* should we throw an exception or silently accept? */
	/* the most likely cause is that we are sending unicast and no-one
	   is listening, or the interface is down, there is no route */
	if (bytes_written == -1)
	{
		QSocketDevice::Error x = SocketDevice.error ();
		if (x != QSocketDevice::NetworkFailure)
			qDebug ("error sending packet");
	}
}

_BOOLEAN
CPacketSocketQT::SetNetwOutAddr (const string & strNewAddr)
{
	/* syntax
	   1:  <ip>:<port>
	   2:  <ip>:<ip>:<port>
	 */
	/* Init return flag and copy string in QT-String "QString" */
	_BOOLEAN bAddressOK = FALSE;
	QStringList parts = QStringList::split (":", strNewAddr.c_str (), TRUE);
	switch (parts.count ())
	{
	case 2:
		bAddressOK = HostAddrOut.setAddress (parts[0]);
		iHostPortOut = parts[1].toInt ();
		break;
	case 3:
		QHostAddress AddrInterface;
		AddrInterface.setAddress (parts[0]);
		bAddressOK = HostAddrOut.setAddress (parts[1]);
		iHostPortOut = parts[2].toInt ();
		const SOCKET s = SocketDevice.socket ();
#if QT_VERSION < 0x030000
		uint32_t mc_if = htonl (AddrInterface.ip4Addr ());
#else
		uint32_t mc_if = htonl (AddrInterface.toIPv4Address ());
#endif
		if (setsockopt (s, IPPROTO_IP, IP_MULTICAST_IF,
						(char *) &mc_if, sizeof (mc_if)) == SOCKET_ERROR)
			bAddressOK = FALSE;
		break;
	}
	return bAddressOK;
}


_BOOLEAN
CPacketSocketQT::SetNetwInAddr (const string & strNewAddr)
{

	/* syntax
	   1:  <port>
	   2:  <group ip>:<port>
	   3:  <interface ip>:<group ip>:<port>
	   4:  <interface ip>::<port>
	   5:  :<group ip>:<port>
	 */
	int iPort=-1;
	QHostAddress AddrGroup, AddrInterface;
	QStringList parts = QStringList::split (":", strNewAddr.c_str (), TRUE);
	bool ok;
	switch (parts.count ())
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
		if (parts[0].length () > 0)
			ok &= AddrInterface.setAddress(parts[0]);
		if (parts[1].length () > 0)
			ok &= AddrGroup.setAddress(parts[1]);
		break;
	default:
		ok = false;
	}
	if(ok)
	{
#if defined(HAVE_LIBWTAP) || defined(HAVE_LIBPCAP)
		pf = NULL;
#endif
	}
	else
	{
	/* it might be a file */
#ifdef HAVE_LIBPCAP
		char errbuf[PCAP_ERRBUF_SIZE];
		pf = pcap_open_offline(strNewAddr.c_str(), errbuf);
#endif
#ifdef HAVE_LIBWTAP
		int err;
		gchar *err_info;
		pf = wtap_open_offline(strNewAddr.c_str(), &err, &err_info, FALSE);
#endif
#if defined(HAVE_LIBWTAP) || defined(HAVE_LIBPCAP)
		if ( pf != NULL)
		{
			timeKeeper = QTime::currentTime();
			QTimer::singleShot(400, this, SLOT(OnDataReceived()));
			return TRUE;
    	}
#endif
		throw CGenErr (string("Can't parse rsiin arg ")+strNewAddr);
	}

	/* Multicast ? */

#if QT_VERSION < 0x030000
	uint32_t gp = AddrGroup.ip4Addr ();
#else
	uint32_t gp = AddrGroup.toIPv4Address ();
#endif
	if (gp == 0)
	{
		/* Initialize the listening socket. */
		SocketDevice.bind (AddrInterface, iPort);
	}
	else if ((gp & 0xe0000000) == 0xe0000000)	/* multicast! */
	{
		struct ip_mreq mreq;

		/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
		bool ok = SocketDevice.bind (QHostAddress (UINT32 (0)), iPort);
		if (ok == false)
		{
			//QSocketDevice::Error x = SocketDevice.error ();
			throw CGenErr ("Can't bind to port to receive packets");
			return FALSE;
		}

#if QT_VERSION < 0x030000
		mreq.imr_multiaddr.s_addr = htonl (AddrGroup.ip4Addr ());
		mreq.imr_interface.s_addr = htonl (AddrInterface.ip4Addr ());
#else
		mreq.imr_multiaddr.s_addr = htonl (AddrGroup.toIPv4Address ());
		mreq.imr_interface.s_addr = htonl (AddrInterface.toIPv4Address ());
#endif
		const SOCKET s = SocketDevice.socket ();
		int n = setsockopt (s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq,
							sizeof (mreq));
		if (n == SOCKET_ERROR)
		{
			throw
				CGenErr (string
						 ("Can't join multicast group to receive packets: ") +
						 strerror (errno));
		}
		return TRUE;
	}
	else						/* one address specified, but not multicast - listen on a specific interface */
	{
		/* Initialize the listening socket. */
		SocketDevice.bind (AddrGroup, iPort);
	}
	return TRUE;
}

void
CPacketSocketQT::OnDataReceived ()
{
	vector < _BYTE > vecsRecBuf (MAX_SIZE_BYTES_NETW_BUF);
#if defined(HAVE_LIBWTAP) || defined(HAVE_LIBPCAP)
	if(pf)
	{ 
		int link_len = 0;
		const _BYTE* pkt_data;
#ifdef HAVE_LIBPCAP
   		struct pcap_pkthdr *header;
		int res;
		const u_char* data;
		/* Retrieve the packet from the file */
		if((res = pcap_next_ex( pf, &header, &data)) != 1)
		{
			pcap_close(pf);
			pf = NULL;
			return;
		}
		else
		{
			int lt = pcap_datalink(pf);
			pkt_data = (_BYTE*)data;
			/* 14 bytes ethernet header */
			if(lt==DLT_EN10MB)
			{
				link_len=14;
			}
			/* raw IP header ? */
			if(lt==DLT_RAW)
			{
				link_len=0;
			}
		}
#endif
#ifdef HAVE_LIBWTAP
		int res, err;
		gchar *err_info;
		long int data_offset;
		/* Retrieve the packet from the file */
		if((res = wtap_read(pf, &err, &err_info, &data_offset)) == 0)
		{
			cout << "wtap read result: " << res << endl;
			wtap_close(pf);
			pf = NULL;
			return;
		}
		else
		{
		 	struct wtap_pkthdr *phdr = wtap_phdr(pf);
			pkt_data = (_BYTE*)wtap_buf_ptr(pf);
			/* 14 bytes ethernet header */
			if(phdr->pkt_encap == WTAP_ENCAP_ETHERNET)
			{
				link_len=14;
			}
			/* raw IP header ? */
			if(phdr->pkt_encap == WTAP_ENCAP_RAW_IP)
			{
				link_len=0;
			}
		}
#endif
		/* 4n bytes IP header, 8 bytes UDP header */
		int udp_ip_hdr_len = 4*(pkt_data[link_len] & 0x0f) + 8;
		int ip_packet_len = ntohs(*(uint16_t*)&pkt_data[link_len+2]);
		int data_len = ip_packet_len - udp_ip_hdr_len;
		vecsRecBuf.resize (data_len);
			for(int i=0; i<data_len; i++)
				vecsRecBuf[i] = pkt_data[link_len+udp_ip_hdr_len+i];
		/* Decode the incoming packet */
		if (pPacketSink != NULL)
			pPacketSink->SendPacket (vecsRecBuf);
		QTime iNow = QTime::currentTime();
		int iDelay = 400 - timeKeeper.msecsTo(iNow);
		if(iDelay < 0)
		    iDelay = 0;
		QTimer::singleShot(iDelay, this, SLOT(OnDataReceived()));
		timeKeeper = timeKeeper.addMSecs(400);
		return;
	}
#endif

	/* Read block from network interface */
	const int iNumBytesRead = SocketDevice.readBlock ((char *) &vecsRecBuf[0],
													  MAX_SIZE_BYTES_NETW_BUF);

	if (iNumBytesRead > 0)
	{
		vecsRecBuf.resize (iNumBytesRead);
		/* Decode the incoming packet */
		if (pPacketSink != NULL)
			pPacketSink->SendPacket (vecsRecBuf);
	}
}
