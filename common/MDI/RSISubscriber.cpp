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


CRSISubscriber::CRSISubscriber(CPacketSink *pSink) : pPacketSink(pSink),
	cProfile(0), bNeedPft(FALSE), pDRMReceiver(0), bUseAFCRC(TRUE), sequence_counter(0)
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
	bNeedPft = FALSE;
}

void CRSISubscriber::TransmitPacket(CTagPacketGenerator& Generator)
{
	if (pPacketSink != 0)
	{
		size_t fragment_size = 1470;
	 	Generator.SetProfile(cProfile);
		vector<_BYTE> packet = AFPacketGenerator.GenAFPacket(bUseAFCRC, Generator);
		//bNeedPft |= packet.size()>fragment_size;
		if(bNeedPft)
		{
			vector< vector<_BYTE> > packets;
			CPft::MakePFTPackets(packet, packets, sequence_counter, fragment_size);
			sequence_counter++;
			for(size_t i=0; i<packets.size(); i++)
				pPacketSink->SendPacket(packets[i]);
		}
		else
			pPacketSink->SendPacket(packet);
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
	bNeedPft = FALSE;
	char c = strDestination[0];
	if(c == 'P' || c == 'p')
	{
		bNeedPft = TRUE;
		strDestination.erase(0, 1);
	}
	if(pSocket)
		return pSocket->SetDestination(strDestination);
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

CRSISubscriberFile::CRSISubscriberFile(): CRSISubscriber(NULL), pPacketSinkFile(NULL)
{
	/* override the subscriber back to NULL to prevent Cpro doing anything */
	TagPacketDecoderRSCIControl.SetSubscriber(NULL);
}

_BOOLEAN CRSISubscriberFile::SetDestination(const string& strFName)
{
	if(pPacketSink)
	{
		delete pPacketSink;
		pPacketSink = NULL;
		pPacketSinkFile = NULL;
	}
	string ext;
	size_t p = strFName.rfind('.');
	if (p != string::npos)
		ext = strFName.substr(p + 1);
	if (ext.substr(0,4) == "pcap")
		pPacketSinkFile = new CPacketSinkPcapFile;
	else if(ext.substr(0,2) == "ff")
		pPacketSinkFile = new CPacketSinkFileFraming;
	else
		pPacketSinkFile = new CPacketSinkRawFile;
	if(pPacketSinkFile)
	{
		pPacketSink = pPacketSinkFile;
		return pPacketSinkFile->SetDestination(strFName);
	}
	return FALSE;
}

_BOOLEAN CRSISubscriberFile::GetDestination(string& strFName)
{
	if(pPacketSinkFile)
		return pPacketSinkFile->GetDestination(strFName);
	return FALSE;
}

void CRSISubscriberFile::StartRecording()
{
	if(pPacketSinkFile)
		pPacketSinkFile->StartRecording();
}

void CRSISubscriberFile::StopRecording()
{
	if(pPacketSinkFile)
		pPacketSinkFile->StopRecording();
}
