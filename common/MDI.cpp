/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI) and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820 and ETSI TS 102 821.
 *
 *	This module relies on the socket and timer routines offered by QT.
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

#include "MDI.h"


/* Implementation *************************************************************/
CMDI::CMDI() : SocketDevice(QSocketDevice::Datagram /* UDP */), iLogFraCnt(0),
	iSeqNumber(0), bMDIEnabled(FALSE), vecbiTagStr(MAX_NUM_STREAMS)
{
	/* Reset all tags for initialization and reset flag for SDC tag */
	ResetTags(TRUE);
	bSDCWasSet = FALSE;

	/* Init constant tag */
	GenTagProTy();


	/* Default settings for the "special protocol settings" ----------------- */
	/* Use CRC for AF packets */
	bUseAFCRC = TRUE;




#if 0
// FIXME: Does not work right now!!!!!!!!!!!!!!!!!!!!!!
// needed for MDI receive which is not implemented right now anyway...

	/* Generate the socket notifier and connect the signal */
	pSocketNotivRead = new QSocketNotifier(SocketDevice.socket(),
		QSocketNotifier::Read);

	/* Connect the "activated" signal */
    QObject::connect(pSocketNotivRead, SIGNAL(activated(int)),
		this, SLOT(OnDataReceived()));
#endif
}


/******************************************************************************\
* MDI transmit                                                                 *
\******************************************************************************/
/* Access functions ***********************************************************/
void CMDI::SetFACData(CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter)
{
	/* If an FAC block is ready, we send all OLD tags. The reason for that is
	   because an FAC block is always available if a DRM stream is connected
	   whereby the regular stream might not be present. Therefore we should use
	   the FAC blocks for synchronization of AF packets.
	   One problem is the SDC block. If we receive a new FAC block, the SDC of
	   the current DRM frame is already processed. Therefore it does not belong
	   to the OLD tags. To correct this, we introduced a flag showing that the
	   SDC block was received. In this case, delay the SDC tag and send it with
	   the next AF packet */
	if (bSDCWasSet == TRUE)
	{
		/* Do not send SDC packet with this AF packet, do not reset SDC tag
		   since we need it for the next AF packet */
		SendPacket(GenAFPacket(FALSE));
		ResetTags(FALSE);

		/* Reset flag */
		bSDCWasSet = FALSE;
	}
	else
	{
		SendPacket(GenAFPacket(TRUE));
		ResetTags(TRUE);
	}

	/* Now generate tags for the current DRM frame */
	GenTagFAC(vecbiFACData);
	GenTagRobMod(Parameter.GetWaveMode());

	/* SDC channel information tag must be created here because it must be sent
	   with each AF packet */
	GenTagSDCChanInf(Parameter);
}

void CMDI::SetSDCData(CVectorEx<_BINARY>& vecbiSDCData)
{
	GenTagSDC(vecbiSDCData);

	/* Set flag to identify special case */
	bSDCWasSet = TRUE;
}

void CMDI::SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData)
{
	GenTagStr(iStrNum, vecbiStrData);
}


/* Network interface implementation *******************************************/
_BOOLEAN CMDI::SetNetwOutAddr(const string strNewIPPort)
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
				SetEnableMDI(TRUE);
				bAddressOK = TRUE;
			}
		}
	}

	return bAddressOK;
}

void CMDI::SendPacket(CVector<_BINARY>& vecbiPacket)
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


/* Actual MDI protocol implementation *****************************************/
CVector<_BINARY> CMDI::GenAFPacket(const _BOOLEAN bWithSDC)
{
/*
	The AF layer encapsulates a single TAG Packet. Mandatory TAG items:
	*ptr, dlfc, fac_, sdc_, sdci, robm, str0-3
*/
	int					i;
	CVector<_BINARY>	vecbiAFPkt;

	/* Payload length in bytes */
// TODO: check if padding bits are needed to get byte alignment!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	const int iPayloadLenBytes = (vecbiTagProTy.Size() +
		vecbiTagLoFrCnt.Size() + vecbiTagFAC.Size() + vecbiTagSDC.Size() +
		vecbiTagSDCChanInf.Size() + vecbiTagRobMod.Size() +
		vecbiTagStr[0].Size() + vecbiTagStr[1].Size() + vecbiTagStr[2].Size() +
		vecbiTagStr[3].Size()) / SIZEOF__BYTE;

	/* 10 bytes AF header, 2 bytes CRC, payload */
	const int iAFPktLenBits =
		iPayloadLenBytes * SIZEOF__BYTE + 12 * SIZEOF__BYTE;

	/* Init vector length */
	vecbiAFPkt.Init(iAFPktLenBits);
	vecbiAFPkt.ResetBitAccess();

	/* SYNC: two-byte ASCII representation of "AF" */
	vecbiAFPkt.Enqueue((uint32_t) 'A', SIZEOF__BYTE);
	vecbiAFPkt.Enqueue((uint32_t) 'F', SIZEOF__BYTE);

	/* LEN: length of the payload, in bytes (4 bytes long -> 32 bits) */
	vecbiAFPkt.Enqueue((uint32_t) iPayloadLenBytes, 32);

	/* SEQ: sequence number. Each AF Packet shall increment the sequence number
	   by one for each packet sent, regardless of content. There shall be no
	   requirement that the first packet received shall have a specific value.
	   The counter shall wrap from FFFF_[16] to 0000_[16], thus the value shall
	   count, FFFE_[16], FFFF_[16], 0000_[16], 0001_[16], etc.
	   (2 bytes long -> 16 bits) */
	vecbiAFPkt.Enqueue((uint32_t) iSeqNumber, 16);

	iSeqNumber++;
	if (iSeqNumber > 0xFFFF)
		iSeqNumber = 0;

	/* AR: AF protocol Revision -
	   a field combining the CF, MAJ and MIN fields */
	/* CF: CRC Flag, 0 if the CRC field is not used (CRC value shall be
	   0000_[16]) or 1 if the CRC field contains a valid CRC (1 bit long) */
	if (bUseAFCRC == TRUE)
		vecbiAFPkt.Enqueue((uint32_t) 1, 1);
	else
		vecbiAFPkt.Enqueue((uint32_t) 0, 1);

	/* MAJ: major revision of the AF protocol in use (3 bits long) */
	vecbiAFPkt.Enqueue((uint32_t) AF_MAJOR_REVISION, 3);

	/* MIN: minor revision of the AF protocol in use (4 bits long) */
	vecbiAFPkt.Enqueue((uint32_t) AF_MINOR_REVISION, 4);

	/* Protocol Type (PT): single byte encoding the protocol of the data carried
	   in the payload. For TAG Packets, the value shall be the ASCII
	   representation of "T" */
	vecbiAFPkt.Enqueue((uint32_t) 'T', SIZEOF__BYTE);


	/* Payload -------------------------------------------------------------- */

// TODO: copy data byte wise -> check if possible to do that...

	/* *ptr tag */
	vecbiTagProTy.ResetBitAccess();
	for (i = 0; i < vecbiTagProTy.Size(); i++)
		vecbiAFPkt.Enqueue(vecbiTagProTy.Separate(1), 1);

	/* dlfc tag */
	vecbiTagLoFrCnt.ResetBitAccess();
	for (i = 0; i < vecbiTagLoFrCnt.Size(); i++)
		vecbiAFPkt.Enqueue(vecbiTagLoFrCnt.Separate(1), 1);

	/* fac_ tag */
	vecbiTagFAC.ResetBitAccess();
	for (i = 0; i < vecbiTagFAC.Size(); i++)
		vecbiAFPkt.Enqueue(vecbiTagFAC.Separate(1), 1);

	/* SDC tag must be delayed in some cases */
	if (bWithSDC == TRUE)
	{
		/* sdc_ tag */
		vecbiTagSDC.ResetBitAccess();
		for (i = 0; i < vecbiTagSDC.Size(); i++)
			vecbiAFPkt.Enqueue(vecbiTagSDC.Separate(1), 1);
	}

	/* sdci tag */
	vecbiTagSDCChanInf.ResetBitAccess();
	for (i = 0; i < vecbiTagSDCChanInf.Size(); i++)
		vecbiAFPkt.Enqueue(vecbiTagSDCChanInf.Separate(1), 1);

	/* robm tag */
	vecbiTagRobMod.ResetBitAccess();
	for (i = 0; i < vecbiTagRobMod.Size(); i++)
		vecbiAFPkt.Enqueue(vecbiTagRobMod.Separate(1), 1);

	/* strx tag */
	for (int j = 0; j < MAX_NUM_STREAMS; j++)
	{
		vecbiTagStr[j].ResetBitAccess();
		for (i = 0; i < vecbiTagStr[j].Size(); i++)
			vecbiAFPkt.Enqueue(vecbiTagStr[j].Separate(1), 1);
	}


	/* CRC: CRC calculated as described in annex A if the CF field is 1,
	   otherwise 0000_[16] */
	if (bUseAFCRC == TRUE)
	{
		CCRC CRCObject;

		/* CRC -------------------------------------------------------------- */
		/* Calculate the CRC and put at the end of the stream */
		CRCObject.Reset(16);

		vecbiAFPkt.ResetBitAccess();

		/* 2 bytes CRC -> "- 2" */
		for (int i = 0; i < iAFPktLenBits / SIZEOF__BYTE - 2; i++)
			CRCObject.AddByte((_BYTE) vecbiAFPkt.Separate(SIZEOF__BYTE));

		/* Now, pointer in "enqueue"-function is back at the same place, 
		   add CRC */
		vecbiAFPkt.Enqueue(CRCObject.GetCRC(), 16);
	}
	else
		vecbiAFPkt.Enqueue((uint32_t) 0, 16);


	/* Increment MDI packet counter */
	GenTagLoFrCnt();

	return vecbiAFPkt;
}

void CMDI::GenTagProTy()
{
	/* Length: 8 bytes = 64 bits */
	PrepareTag(vecbiTagProTy, "*ptr", 64);

	/* Protocol type: DMDI */
	vecbiTagProTy.Enqueue((uint32_t) 'D', SIZEOF__BYTE);
	vecbiTagProTy.Enqueue((uint32_t) 'M', SIZEOF__BYTE);
	vecbiTagProTy.Enqueue((uint32_t) 'D', SIZEOF__BYTE);
	vecbiTagProTy.Enqueue((uint32_t) 'I', SIZEOF__BYTE);

	/* Major revision */
	vecbiTagProTy.Enqueue((uint32_t) MDI_MAJOR_REVISION, 16);

	/* Minor revision */
	vecbiTagProTy.Enqueue((uint32_t) MDI_MINOR_REVISION, 16);
}

void CMDI::GenTagLoFrCnt()
{
	/* Length: 4 bytes = 32 bits */
	PrepareTag(vecbiTagLoFrCnt, "dlfc", 32);

	/* Logical frame count */
	vecbiTagLoFrCnt.Enqueue(iLogFraCnt, 32);

	/* Count: the value shall be incremented by one by the device generating the
	   MDI Packets for each MDI Packet sent. Wraps around at a value of
	   "(1 << 32)" since the variable type is "uint32_t" */
	iLogFraCnt++;
}

void CMDI::GenTagFAC(CVectorEx<_BINARY>& vecbiFACData)
{
	/* Length: 9 bytes = 72 bits */
	PrepareTag(vecbiTagFAC, "fac_", 72);

	/* Channel parameters, service parameters, CRC */
	vecbiFACData.ResetBitAccess();

	/* FAC data is always 72 bits long which is 9 bytes, copy data byte-wise */
	for (int i = 0; i < 9; i++)
		vecbiTagFAC.Enqueue(vecbiFACData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
}

void CMDI::GenTagSDC(CVectorEx<_BINARY>& vecbiSDCData)
{
	const int iLenSDCDataBits = vecbiSDCData.Size();

	/* Length: "length SDC block" bytes. Our SDC data vector does not
	   contain the 4 bits "Rfu" */
	PrepareTag(vecbiTagSDC, "sdc_", iLenSDCDataBits + 4);

	/* Service Description Channel Block */
	vecbiSDCData.ResetBitAccess();

	vecbiTagSDC.Enqueue((uint32_t) 0, 4); /* Rfu */

	/* We have to copy bits instead of bytes since the length of SDC data is
	   usually not a multiple of 8 */
	for (int i = 0; i < iLenSDCDataBits; i++)
		vecbiTagSDC.Enqueue(vecbiSDCData.Separate(1), 1);
}

void CMDI::GenTagSDCChanInf(CParameter& Parameter)
{
	CVector<int> veciActStreams;

	/* Get active streams */
	Parameter.GetActiveStreams(veciActStreams);

	/* Get number of active streams */
	const int iNumActStreams = veciActStreams.Size();

	/* Length: 1 + n * 3 bytes */
	PrepareTag(
		vecbiTagSDCChanInf, "sdci", (1 + 3 * iNumActStreams) * SIZEOF__BYTE);

	/* Protection */
	/* Rfu */
	vecbiTagSDCChanInf.Enqueue((uint32_t) 0, 4);

	/* PLA */
	vecbiTagSDCChanInf.Enqueue((uint32_t) Parameter.MSCPrLe.iPartA, 2);

	/* PLB */
	vecbiTagSDCChanInf.Enqueue((uint32_t) Parameter.MSCPrLe.iPartB, 2);

	/* n + 1 stream description(s) */
	for (int i = 0; i < iNumActStreams; i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hierarchical data */
		if ((i == 0) &&
			((Parameter.eMSCCodingScheme == CParameter::CS_3_HMSYM) ||
			(Parameter.eMSCCodingScheme == CParameter::CS_3_HMMIX)))
		{
			/* Protection level for hierarchical */
			vecbiTagSDCChanInf.Enqueue(
				(uint32_t) Parameter.MSCPrLe.iHierarch, 2);

			/* rfu */
			vecbiTagSDCChanInf.Enqueue((uint32_t) 0, 10);

			/* Data length for hierarchical (always stream 0) */
			vecbiTagSDCChanInf.Enqueue(
				(uint32_t) Parameter.Stream[0].iLenPartB, 12);
		}
		else
		{
			/* Data length for part A */
			vecbiTagSDCChanInf.Enqueue(
				(uint32_t) Parameter.Stream[veciActStreams[i]].iLenPartA, 12);

			/* Data length for part B */
			vecbiTagSDCChanInf.Enqueue(
				(uint32_t) Parameter.Stream[veciActStreams[i]].iLenPartB, 12);
		}
	}
}

void CMDI::GenTagRobMod(const ERobMode eCurRobMode)
{
	/* Length: 1 byte */
	PrepareTag(vecbiTagRobMod, "robm", SIZEOF__BYTE);

	/* Robustness mode */
	switch (eCurRobMode)
	{
	case RM_ROBUSTNESS_MODE_A:
		vecbiTagRobMod.Enqueue((uint32_t) 0, 8);
		break;

	case RM_ROBUSTNESS_MODE_B:
		vecbiTagRobMod.Enqueue((uint32_t) 1, 8);
		break;

	case RM_ROBUSTNESS_MODE_C:
		vecbiTagRobMod.Enqueue((uint32_t) 2, 8);
		break;

	case RM_ROBUSTNESS_MODE_D:
		vecbiTagRobMod.Enqueue((uint32_t) 3, 8);
		break;
	}
}

void CMDI::GenTagStr(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData)
{
	const int iLenStrData = vecbiStrData.Size();

	/* Only generate this tag if stream input data is not of zero length */
	if ((iLenStrData != 0) && (iStrNum < MAX_NUM_STREAMS))
	{
		/* Data length: n * 8 bits, different tag names for different streams */
		switch (iStrNum)
		{
		case 0:
			PrepareTag(vecbiTagStr[0], "str0", iLenStrData);
			break;

		case 1:
			PrepareTag(vecbiTagStr[1], "str1", iLenStrData);
			break;

		case 2:
			PrepareTag(vecbiTagStr[2], "str2", iLenStrData);
			break;

		case 3:
			PrepareTag(vecbiTagStr[3], "str3", iLenStrData);
			break;
		}

		/* Data */
		vecbiStrData.ResetBitAccess();

		/* Data is always a multiple of 8 -> copy bytes */
		for (int i = 0; i < iLenStrData / SIZEOF__BYTE; i++)
		{
			vecbiTagStr[iStrNum].Enqueue(
				vecbiStrData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
	}
}

void CMDI::GenTagInfo(string strUTF8Text)
{
	/* Data length: n * 8 bits */
	PrepareTag(vecbiTagInfo, "info", strUTF8Text.size() * SIZEOF__BYTE);

	/* UTF-8 text */
	for (int i = 0; i < strUTF8Text.size(); i++)
	{
		const char cNewChar = strUTF8Text[i];

		/* Set character */
		vecbiTagInfo.Enqueue((uint32_t) cNewChar, SIZEOF__BYTE);
	}
}

void CMDI::PrepareTag(CVector<_BINARY>& vecbiTag, const string strTagName,
					  const int iLenDataBits)
{
/*
	Function called by all tags
*/
	/* Init vector length. 4 bytes for tag name and 4 bytes for data length
	   plus the length of the actual data */
	vecbiTag.Init(8 * SIZEOF__BYTE + iLenDataBits);
	vecbiTag.ResetBitAccess();

	/* Set tag name (always four bytes long) */
	for (int i = 0; i < 4; i++)
		vecbiTag.Enqueue((uint32_t) strTagName[i], SIZEOF__BYTE);

	/* Set tag data length */
	vecbiTag.Enqueue((uint32_t) iLenDataBits, 32);
}

void CMDI::ResetTags(const _BOOLEAN bResetSDC)
{
	/* Do not reset "*ptr" tag because this one is static */
	vecbiTagLoFrCnt.Init(0); /* dlfc tag */
	vecbiTagFAC.Init(0); /* fac_ tag */
	vecbiTagSDCChanInf.Init(0); /* sdci tag */
	vecbiTagRobMod.Init(0); /* robm tag */
	vecbiTagInfo.Init(0); /* info tag */

	for (int i = 0; i < MAX_NUM_STREAMS; i++)
		vecbiTagStr[i].Init(0); /* strx tag */

	/* Special case with SDC block */
	if (bResetSDC == TRUE)
		vecbiTagSDC.Init(0); /* sdc_ tag */
}


/******************************************************************************\
* MDI receive                                                                  *
\******************************************************************************/
/* Access functions ***********************************************************/


/* Network interface implementation *******************************************/
_BOOLEAN CMDI::SetNetwInPort(const int iPort)
{
	/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
	return SocketDevice.bind(QHostAddress((Q_UINT32) 0), iPort);
}

void CMDI::OnDataReceived()
{

CVector<_BYTE> vecsRecBuf(MAX_SIZE_BYTES_NETW_BUF);

	/* Read block from network interface */
	const int iNumBytesRead = SocketDevice.readBlock(
		(char*) &vecsRecBuf[0], MAX_SIZE_BYTES_NETW_BUF);

	/* get host address of client */
SocketDevice.peerAddress();
SocketDevice.peerPort();

// TEST
CVector<_BINARY> vecbiAFPkt(iNumBytesRead * SIZEOF__BYTE);
for (int i = 0; i < iNumBytesRead; i++)
	vecbiAFPkt.Enqueue(vecsRecBuf[i], SIZEOF__BYTE);

DecAFPacket(vecbiAFPkt);
}


/* Actual MDI protocol implementation *****************************************/
void CMDI::DecAFPacket(CVector<_BINARY>& vecbiAFPkt)
{
	int i;

	/* CRC check ------------------------------------------------------------ */
	CCRC CRCObject;

// FIXME: is this length always the correct length? In the actual packet
// there is also a value for the length included!!!???!???
const int iLenAFPkt = vecbiAFPkt.Size();

	/* We do the CRC check at the beginning no matter if it is used or not
	   since we have to reset bit access for that */
	/* Reset bit extraction access */
	vecbiAFPkt.ResetBitAccess();

	/* Check the CRC of this packet */
	CRCObject.Reset(16);

	/* "- 2": 16 bits for CRC at the end */
	for (i = 0; i < iLenAFPkt / SIZEOF__BYTE - 2; i++)
		CRCObject.AddByte((_BYTE) vecbiAFPkt.Separate(SIZEOF__BYTE));

	const _BOOLEAN bCRCOk = CRCObject.CheckCRC(vecbiAFPkt.Separate(16));


	/* Actual packet decoding ----------------------------------------------- */
	vecbiAFPkt.ResetBitAccess();

	/* SYNC: two-byte ASCII representation of "AF" (2 bytes) */
	string strSyncASCII = "";
	for (i = 0; i < 2; i++)
		strSyncASCII += (_BYTE) vecbiAFPkt.Separate(SIZEOF__BYTE);

	/* Check if string is correct */
	if (strSyncASCII.compare("AF") != 0)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* LEN: length of the payload, in bytes (4 bytes long -> 32 bits) */
	const int iPayLLen = (int) vecbiAFPkt.Separate(32);

	/* SEQ: sequence number. Each AF Packet shall increment the sequence number
	   by one for each packet sent, regardless of content. There shall be no
	   requirement that the first packet received shall have a specific value.
	   The counter shall wrap from FFFF_[16] to 0000_[16], thus the value shall
	   count, FFFE_[16], FFFF_[16], 0000_[16], 0001_[16], etc.
	   (2 bytes long -> 16 bits) */

// TODO: use sequence number somehow!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const int iCurSeqNum = (int) vecbiAFPkt.Separate(16);
/*
iSeqNumber++;
if (iSeqNumber > 0xFFFF)
	iSeqNumber = 0;
*/

	/* AR: AF protocol Revision -
	   a field combining the CF, MAJ and MIN fields */
	/* CF: CRC Flag, 0 if the CRC field is not used (CRC value shall be
	   0000_[16]) or 1 if the CRC field contains a valid CRC (1 bit long) */
	if ((_BOOLEAN) vecbiAFPkt.Separate(1))
	{
		/* Use CRC check which was already done */
		if (!bCRCOk)
			return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!
	}

	/* MAJ: major revision of the AF protocol in use (3 bits long) */
	const int iMajRevAFProt = (int) vecbiAFPkt.Separate(3);

	/* MIN: minor revision of the AF protocol in use (4 bits long) */
	const int iMinRevAFProt = (int) vecbiAFPkt.Separate(4);

// TODO: check if protocol versions match our version!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	/* Protocol Type (PT): single byte encoding the protocol of the data carried
	   in the payload. For TAG Packets, the value shall be the ASCII
	   representation of "T" */
	if ((_BYTE) vecbiAFPkt.Separate(SIZEOF__BYTE) != 'T')
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!


	/* Payload -------------------------------------------------------------- */
	/* Decode all tags */
	int iCurConsBytes = 0;

	/* Each tag must have at least a header with 8 bytes -> "- 8" */
	while (iCurConsBytes < iPayLLen - 8)
		iCurConsBytes += DecodeTag(vecbiAFPkt);
}

int CMDI::DecodeTag(CVector<_BINARY>& vecbiTag)
{
	/* Decode tag name (always four bytes long) */
	string strTagName = "";
	for (int i = 0; i < 4; i++)
		strTagName += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);

	/* Get tag data length (4 bytes = 32 bits) */
	const int iLenDataBits = vecbiTag.Separate(32);

	/* We cannot use the "switch" command here. Check each case separately with
	   a "if" condition */
	if (strTagName.compare("*ptr") == 0) /* "*ptr" tag */
		DecTagProTy(vecbiTag, iLenDataBits);

	if (strTagName.compare("dlfc") == 0) /* "dlfc" tag */
		DecTagLoFrCnt(vecbiTag, iLenDataBits);

	if (strTagName.compare("fac_") == 0) /* "fac_" tag */
		DecTagFAC(vecbiTag, iLenDataBits);

	if (strTagName.compare("sdc_") == 0) /* "sdc_" tag */
		DecTagSDC(vecbiTag, iLenDataBits);

	if (strTagName.compare("sdci") == 0) /* "sdci" tag */
		DecTagSDCChanInf(vecbiTag, iLenDataBits);

	if (strTagName.compare("robm") == 0) /* "robm" tag */
		DecTagRobMod(vecbiTag, iLenDataBits);

	if (strTagName.compare("str0") == 0) /* "str0" tag */
		DecTagStr(vecbiTag, iLenDataBits, 0);

	if (strTagName.compare("str1") == 0) /* "str1" tag */
		DecTagStr(vecbiTag, iLenDataBits, 1);

	if (strTagName.compare("str2") == 0) /* "str2" tag */
		DecTagStr(vecbiTag, iLenDataBits, 2);

	if (strTagName.compare("str3") == 0) /* "str3" tag */
		DecTagStr(vecbiTag, iLenDataBits, 3);

	if (strTagName.compare("info") == 0) /* "info" tag */
		DecTagInfo(vecbiTag, iLenDataBits);


// TODO take care of tags that are not supported!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	/* Return number of consumed bytes. This number is the actual body plus two
	   times for bytes for the header = 8 bytes */
	return iLenDataBits / SIZEOF__BYTE + 8;
}

void CMDI::DecTagProTy(CVector<_BINARY>& vecbiTag, const int iLen)
{
/*
	Changes to the protocol which will allow existing decoders to still function
	will be represented by an increment of the minor version number only. Any
	new features added by the change will obviously not need to be supported by
	older modulators. Existing TAG Items will not be altered except for the
	definition of bits previously declared Rfu. New TAG Items may be added.

	Changes to the protocol which will render previous implementations unable to
	correctly process the new format will be represented by an increment of the
	major version number. Older implementations should not attempt to decode
	such MDI packets. Changes may include modification to or removal of existing
	TAG Item definitions.
*/

	/* Protocol type and revision (*ptr) always 8 bytes long */
	if (iLen != 64)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* Decode protocol type (32 bits = 4 bytes) */
	string strProtType = "";
	for (int i = 0; i < 4 /* bytes */; i++)
		strProtType += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);

	if (strProtType.compare("DMDI") != 0)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!!

	/* Get major and minor revision of protocol */
	const int iMdiMajRev = (int) vecbiTag.Separate(16);
	const int iMdiMinRev = (int) vecbiTag.Separate(16);
}

void CMDI::DecTagLoFrCnt(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* DRM logical frame count (dlfc) always 4 bytes long */
	if (iLen != 32)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	iLogFraCnt = (int) vecbiTag.Separate(32);

// TODO:
/* Do something with the count */
}

void CMDI::DecTagFAC(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Fast access channel (fac_) always 9 bytes long */
	if (iLen != 72)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	/* Copy incoming FAC data */
	vecbiIncFACData.Init(72);
	vecbiIncFACData.ResetBitAccess();

	for (int i = 0; i < 9; i++)
		vecbiIncFACData.Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
}

void CMDI::DecTagSDC(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Rfu */
	vecbiTag.Separate(4);

	/* Copy incoming SDC data */
	const int iSDCDataSize = iLen - 4;

	vecbiIncSDCData.Init(iSDCDataSize);
	vecbiIncSDCData.ResetBitAccess();

	/* We have to copy bits instead of bytes since the length of SDC data is
	   usually not a multiple of 8 */
	for (int i = 0; i < iSDCDataSize; i++)
		vecbiIncSDCData.Enqueue(vecbiTag.Separate(1), 1);
}

void CMDI::DecTagSDCChanInf(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Get the number of streams */
	const int iNumStreams = (iLen - 8) / 3 / SIZEOF__BYTE;

	/* Get protection levels */
	/* Rfu */
	vecbiTag.Separate(4);

	/* Protection level for part A */ // TODO
	vecbiTag.Separate(2);

	/* Protection level for part B */ // TODO
	vecbiTag.Separate(2);

	/* Get stream parameters */

	/* Determine if hierarchical modulation is used */ // TODO
_BOOLEAN bHierarchical = FALSE;

	for (int i = 0; i < iNumStreams; i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hierarchical data */
		if ((i == 0) && (bHierarchical = TRUE))
		{
			/* Protection level for hierarchical */ // TODO
			vecbiTag.Separate(2);

			/* rfu */
			vecbiTag.Separate(10);

			/* Data length for hierarchical */ // TODO
			vecbiTag.Separate(12);
		}
		else
		{
			/* Data length for part A */ // TODO
			vecbiTag.Separate(12);

			/* Data length for part B */ // TODO
			vecbiTag.Separate(12);
		}
	}
}

void CMDI::DecTagRobMod(CVector<_BINARY>& vecbiTag, const int iLen)
{
	/* Robustness mode (robm) always one byte long */
	if (iLen != 8)
		return; // TODO: error handling!!!!!!!!!!!!!!!!!!!!!!

	switch (vecbiTag.Separate(8)) // TODO
	{
	case 0:
		/* Robustness mode A */
		break;

	case 1:
		/* Robustness mode B */
		break;

	case 2:
		/* Robustness mode C */
		break;

	case 3:
		/* Robustness mode D */
		break;
	}
}

void CMDI::DecTagStr(CVector<_BINARY>& vecbiTag, const int iLen,
					 const int iStrNum)
{
	int i;

	switch (iStrNum)
	{
	case 0:
		vecbiIncStr0.Init(iLen);
		vecbiIncStr0.ResetBitAccess();

		for (i = 0; i < iLen / SIZEOF__BYTE; i++)
			vecbiIncStr0.Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		break;

	case 1:
		vecbiIncStr1.Init(iLen);
		vecbiIncStr1.ResetBitAccess();

		for (i = 0; i < iLen / SIZEOF__BYTE; i++)
			vecbiIncStr1.Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		break;

	case 2:
		vecbiIncStr2.Init(iLen);
		vecbiIncStr2.ResetBitAccess();

		for (i = 0; i < iLen / SIZEOF__BYTE; i++)
			vecbiIncStr2.Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		break;

	case 3:
		vecbiIncStr3.Init(iLen);
		vecbiIncStr3.ResetBitAccess();

		for (i = 0; i < iLen / SIZEOF__BYTE; i++)
			vecbiIncStr3.Enqueue(vecbiTag.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		break;
	}
}

void CMDI::DecTagInfo(CVector<_BINARY>& vecbiTag, const int iLen)
{
// TODO use text somehow
	/* Decode info string */
	string strInfo = "";
	for (int i = 0; i < iLen / SIZEOF__BYTE; i++)
		strInfo += (_BYTE) vecbiTag.Separate(SIZEOF__BYTE);
}
