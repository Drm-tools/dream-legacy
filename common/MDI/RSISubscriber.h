/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Oliver Haffenden
 *
 * Description:
 *	see RSISubscriber.cpp
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

#ifndef RSI_SUBSCRIBER_H_INCLUDED
#define RSI_SUBSCRIBER_H_INCLUDED

#include "../GlobalDefinitions.h"
#include "TagPacketDecoderRSCIControl.h"
#include "PacketSocketQT.h"
#include "PacketSinkFile.h"
#include "PacketInOut.h"

class CPacketSink;
class CDRMReceiver;
class CTagPacketGeneratorWithProfiles;

class CRSISubscriber : public CPacketSink
{
public:
	CRSISubscriber(CPacketSink *pSink = NULL);

	/* provide a pointer to the receiver for incoming RCI commands */
	/* leave it set to NULL if you want incoming commands to be ignored */
	void SetReceiver(CDRMReceiver *pReceiver);

	/* Set the profile for this subscriber - could be different for different subscribers */
	void SetProfile(const char c);

	/* Generate and send a packet */
	void TransmitPacket(CTagPacketGeneratorWithProfiles *pGenerator);

	void SetAFPktCRC(const _BOOLEAN bNAFPktCRC) {bUseAFCRC = bNAFPktCRC;}


	/* from CPacketSink interface */
		virtual void SendPacket(const vector<_BYTE>& vecbydata);

protected:
	CPacketSink *pPacketSink;
	char cProfile;
private:
	CTagPacketDecoderRSCIControl TagPacketDecoderRSCIControl;
	CDRMReceiver *pDRMReceiver;

	_BOOLEAN bUseAFCRC;

};


class CRSISubscriberSocket : public CRSISubscriber
{
public:
	CRSISubscriberSocket();

	_BOOLEAN SetOutAddr(const string& strArgument);
	_BOOLEAN SetInAddr(const string& strAddr);


private:
	CPacketSocketQT				PacketSocket;

};


class CRSISubscriberFile : public CRSISubscriber
{
public:
	CRSISubscriberFile();

	void StartRecording(CParameter &ReceiverParam);
	void StopRecording();
	/* this needs to be called when the frequency changes so the filename can be changed */
	void NewFrequency(CParameter& ReceiverParam);

private:
	CPacketSinkRawFile PacketSinkFile;
	_BOOLEAN bIsRecording;
	int	iFrequency;
};

#endif

