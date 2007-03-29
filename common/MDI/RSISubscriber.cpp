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
#include <iomanip>


CRSISubscriber::CRSISubscriber(CPacketSink *pSink)
: pPacketSink(pSink)
, pDRMReceiver(0)
, bUseAFCRC(TRUE)
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
		pPacketSink->SendPacket(pGenerator->GenAFPacket(bUseAFCRC));
	}
}


/* implementation of function from CPacketSink interface - process incoming RCI commands */
void CRSISubscriber::SendPacket(const vector<_BYTE>& vecbydata)
{
	CVectorEx<_BINARY> vecbidata;
	vecbidata.Init(vecbydata.size()*SIZEOF__BYTE);
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<vecbydata.size(); i++)
		vecbidata.Enqueue(vecbydata[i], SIZEOF__BYTE);
	CTagPacketDecoder::Error err = 
		TagPacketDecoderRSCIControl.DecodeAFPacket(vecbidata);
	if(err != CTagPacketDecoder::E_OK)
		cerr << "bad RSCI Control Packet Received" << endl;
}


CRSISubscriberSocket::CRSISubscriberSocket()
{
}

_BOOLEAN CRSISubscriberSocket::SetOutAddr(const string& strArgument)
{
	_BOOLEAN bAddressOK = PacketSocket.SetNetwOutAddr(strArgument);
	
	/* Only connect the sink to the socket if the setup is successful */
	if (bAddressOK)
		pPacketSink = &PacketSocket;

	return bAddressOK;
}

_BOOLEAN CRSISubscriberSocket::SetInAddr(const string& strAddr)
{
	// Delegate to socket
	_BOOLEAN bOK = PacketSocket.SetNetwInAddr(strAddr);

	if (bOK)
		// Connect socket to the MDI decoder
		PacketSocket.SetPacketSink(this);
return PacketSocket.SetNetwInAddr(strAddr);
}

CRSISubscriberFile::CRSISubscriberFile()
: CRSISubscriber(&PacketSinkFile)
, bIsRecording(FALSE)
, iFrequency(0)
{
}

void CRSISubscriberFile::StartRecording(CParameter &ReceiverParam)
{
	iFrequency = ReceiverParam.ReceptLog.GetFrequency();

	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	stringstream filename;
	filename << ReceiverParam.sDataFilesDirectory;
	filename << ReceiverParam.sReceiverID << "_";
	filename << setw(4) << setfill('0') << gmtCur->tm_year + 1900 << "-" << setw(2) << setfill('0')<< gmtCur->tm_mon + 1;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_mday << "_";
	filename << setw(2) << setfill('0') << gmtCur->tm_hour << "-" << setw(2) << setfill('0')<< gmtCur->tm_min;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_sec << "_";
	filename << setw(8) << setfill('0') << (iFrequency*1000) << ".rs" << char(toupper(cProfile));

	PacketSinkFile.StartRecording(filename.str());

	bIsRecording = TRUE;
}

void CRSISubscriberFile::StopRecording()
{
	PacketSinkFile.StopRecording();
	bIsRecording = FALSE;
}

void CRSISubscriberFile::NewFrequency(CParameter &ReceiverParam)
{
	/* Has it really changed? */
	int iNewFrequency = ReceiverParam.ReceptLog.GetFrequency();

	if (iNewFrequency != iFrequency)
	{
		iFrequency = iNewFrequency;

		if (bIsRecording)
		{
			StopRecording();
			StartRecording(ReceiverParam);
		}
	}
}
