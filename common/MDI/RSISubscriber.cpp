/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Oliver Haffenden
 *
 * Description:
 *	
 *	This class represents a particular consumer of RSI information and supplier of
 *  RCI commands. There could be several of these. The profile is a property of the
 *  particular subscriber and different subscribers could have different profiles.
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

#include "RSISubscriber.h"
#include "../DrmReceiver.h"
#include "TagPacketGenerator.h"
#ifdef USE_QT_GUI
# include "PacketSocketQT.h"
#else
# include "PacketSocketNull.h"
#endif


CRSISubscriber::CRSISubscriber(CPacketSink *pSink) : pPacketSink(pSink), cProfile(0), pDRMReceiver(0), bUseAFCRC(TRUE)
{
	TagPacketDecoderRSCIControl.SetSubscriber(this);
}

void CRSISubscriber::SetReceiver(CDRMReceiver *pReceiver)
{
	pDRMReceiver = pReceiver;
	TagPacketDecoderRSCIControl.SetReceiver(pReceiver);
}

void CRSISubscriber::SetProfile(const char c)
{
	cProfile = c;
}

void CRSISubscriber::TransmitPacket(CTagPacketGeneratorWithProfiles *pGenerator)
{
	if (pPacketSink != 0)
	{
	 	pGenerator->SetProfile(cProfile);
		pPacketSink->SendPacket(AFPacketGenerator.GenAFPacket(bUseAFCRC, pGenerator));
	}
}


/* implementation of function from CPacketSink interface - process incoming RCI commands */
void CRSISubscriber::SendPacket(const vector<_BYTE>& vecbydata, uint32_t, uint16_t)
{
	CVectorEx<_BINARY> vecbidata;
	vecbidata.Init(vecbydata.size()*SIZEOF__BYTE);
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<vecbydata.size(); i++)
		vecbidata.Enqueue(vecbydata[i], SIZEOF__BYTE);
	CTagPacketDecoder::Error err = TagPacketDecoderRSCIControl.DecodeAFPacket(vecbidata);
	if(err != CTagPacketDecoder::E_OK)
		cerr << "bad RSCI Control Packet Received" << endl;
}


/* TODO wrap a sendto in a class and store it in pPacketSink */
CRSISubscriberSocket::CRSISubscriberSocket(CPacketSink *pSink):CRSISubscriber(pSink),pSocket(NULL)
,uIf(0),uAddr(0),uPort(0)
{
#ifdef USE_QT_GUI
	pSocket = new CPacketSocketQT;
#else
	pSocket = new CPacketSocketNull;
#endif
	pPacketSink = pSocket;
}

CRSISubscriberSocket::~CRSISubscriberSocket()
{
	delete pSocket;
}

_BOOLEAN CRSISubscriberSocket::SetDestination(const string& str)
{
	strDestination = str;
	if(pSocket)
		return pSocket->SetDestination(str);
	return FALSE;;
}

_BOOLEAN CRSISubscriberSocket::GetDestination(string& str)
{
	/* want the canonical version so incoming can match */
	if(pSocket)
		return pSocket->GetDestination(str);
	return FALSE;
}

_BOOLEAN CRSISubscriberSocket::SetOrigin(const string& str)
{
	if(pSocket)
	{
		// Delegate to socket
		_BOOLEAN bOK = pSocket->SetOrigin(str);

		if (bOK)
			// Connect socket to the MDI decoder
			pSocket->SetPacketSink(this);
		return bOK;
	}
	return FALSE;
}

CRSISubscriberFile::CRSISubscriberFile(): CRSISubscriber(&PacketSinkFile)
{
}

_BOOLEAN CRSISubscriberFile::SetDestination(const string& strFName)
{
	return PacketSinkFile.SetDestination(strFName);
}

_BOOLEAN CRSISubscriberFile::GetDestination(string& strFName)
{
	return PacketSinkFile.GetDestination(strFName);
}

void CRSISubscriberFile::StartRecording()
{
	PacketSinkFile.StartRecording();
}

void CRSISubscriberFile::StopRecording()
{
	PacketSinkFile.StopRecording();
}
