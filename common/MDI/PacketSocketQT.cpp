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
void CPacketSocketQT::SendPacket(CVector<_BINARY> vecbiPacket)
{
	const int iSizeBytes =
		(int) ceil((_REAL) vecbiPacket.Size() / SIZEOF__BYTE);

	CVector<_BYTE> vecbyPacket(iSizeBytes);

	/* Copy binary vector in byte vector */
	vecbiPacket.ResetBitAccess();
	for (int i = 0; i < iSizeBytes; i++)
		vecbyPacket[i] = (_BYTE) vecbiPacket.Separate(SIZEOF__BYTE);

	/* Send packet to network */
	SocketDevice.writeBlock((const char*) &vecbyPacket[0], iSizeBytes,
		HostAddrOut, iHostPortOut);
}


_BOOLEAN CPacketSocketQT::SetNetwOutAddr(const string strNewIPPort)
{
	/* Init return flag and copy string in QT-String "QString" */
	_BOOLEAN bAddressOK = FALSE;
	const QString strIPPort = strNewIPPort.c_str();

	/* The address should be in the form of [IP]:[port], so first get the
	   position of the colon (and check if a colon is present */
	const int iPosofColon = strIPPort.find(':');

	/* The IP address has at least seven digits */
	if (iPosofColon >= 7)
	{
		/* Set network output host address (from the other listening client).
		   Use the part of the string left of the colon position */
		QHostAddress HostAddrOutTMP;

		const _BOOLEAN bAddrOk =
			HostAddrOutTMP.setAddress(strIPPort.left(iPosofColon));

		if (bAddrOk == TRUE)
		{
			/* Get port number from string. Use the part of the string which is
			   on the right side of the colon */
			const int iPortTMP =
				strIPPort.right(strIPPort.length() - iPosofColon - 1).toUInt();

			/* Check the range of the port number */
			if ((iPortTMP > 0) && (iPortTMP <= 65535))
			{
				/* Address was ok, accept parameters and set internal values */
				iHostPortOut = iPortTMP;
				HostAddrOut = HostAddrOutTMP;

				/* Enable MDI and set "OK" flag */
				bAddressOK = TRUE;
			}
		}
	}

	return bAddressOK;
}

_BOOLEAN CPacketSocketQT::SetNetwInPort(const int iPort)
{
	/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
	return SocketDevice.bind(QHostAddress((Q_UINT32) 0), iPort);
}

_BOOLEAN CPacketSocketQT::SetNetwInMcast(const string strNewIPIP)
{
	struct ip_mreq mreq;

	/* Copy string in QT-String "QString" */
	const QString strIPIP = strNewIPIP.c_str();

	/* The address should be in the form of [IP]:[IP], so first get the
	   position of the colon (and check if a colon is present) */
	const int iPosofColon = strIPIP.find(':');

	/* The IP address has at least seven digits */
	if (iPosofColon >= 7)
	{
		/* Get group IP from string. Use the part of the string which is
		   on the left side of the colon */
		const QString strGroup = strIPIP.left(iPosofColon);

		/* Get interface IP from string. Use the part of the string which is
		   on the right side of the colon */
		const QString strIface =
			strIPIP.right(strIPIP.length() - iPosofColon - 1);

		mreq.imr_multiaddr.s_addr = inet_addr(strGroup.latin1());
		mreq.imr_interface.s_addr = inet_addr(strIface.latin1());

		if ((ntohl(mreq.imr_multiaddr.s_addr) & 0xe0000000) == 0xe0000000)
		{
			const SOCKET s = SocketDevice.socket();

			return setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq,
				sizeof(mreq)) != SOCKET_ERROR;
		}
	}

	return FALSE;
}

void CPacketSocketQT::OnDataReceived()
{
	CVector<_BYTE> vecsRecBuf(MAX_SIZE_BYTES_NETW_BUF);

	/* Read block from network interface */
	const int iNumBytesRead = SocketDevice.readBlock(
		(char*) &vecsRecBuf[0], MAX_SIZE_BYTES_NETW_BUF);

	if (iNumBytesRead > 0)
	{
		/* Copy data from network buffer and decode received packet */
		CVector<_BINARY> vecbiPkt(iNumBytesRead * SIZEOF__BYTE);
		vecbiPkt.ResetBitAccess();
		for (int i = 0; i < iNumBytesRead; i++)
			vecbiPkt.Enqueue(vecsRecBuf[i], SIZEOF__BYTE);

		/* Decode the incoming packet thread-safe */
		Mutex.Lock();

		if (pPacketSink != NULL)
			pPacketSink->SendPacket(vecbiPkt);

		Mutex.Unlock();
	}
}
