/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	Data module (using multimedia information carried in DRM stream)
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

#include "DataDecoder.h"
#include "./epg/epgutil.h"
#include "Journaline.h"
#include <iostream>

/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void
CDataEncoder::GeneratePacket(CVector < _BINARY > &vecbiPacket)
{
	int i;
	bool bLastFlag;

	/* Init size for whole packet, not only body */
	vecbiPacket.Init(iTotalPacketSize);
	vecbiPacket.ResetBitAccess();

	/* Calculate remaining data size to be transmitted */
	const int iRemainSize = vecbiCurDataUnit.Size() - iCurDataPointer;

	/* Header --------------------------------------------------------------- */
	/* First flag */
	if (iCurDataPointer == 0)
		vecbiPacket.Enqueue((uint32_t) 1, 1);
	else
		vecbiPacket.Enqueue((uint32_t) 0, 1);

	/* Last flag */
	if (iRemainSize > iPacketLen)
	{
		vecbiPacket.Enqueue((uint32_t) 0, 1);
		bLastFlag = false;
	}
	else
	{
		vecbiPacket.Enqueue((uint32_t) 1, 1);
		bLastFlag = true;
	}

	/* Packet Id */
	vecbiPacket.Enqueue((uint32_t) iPacketID, 2);

	/* Padded packet indicator (PPI) */
	if (iRemainSize < iPacketLen)
		vecbiPacket.Enqueue((uint32_t) 1, 1);
	else
		vecbiPacket.Enqueue((uint32_t) 0, 1);

	/* Continuity index (CI) */
	vecbiPacket.Enqueue((uint32_t) iContinInd, 3);

	/* Increment index modulo 8 (1 << 3) */
	iContinInd++;
	if (iContinInd == 8)
		iContinInd = 0;

	/* Body ----------------------------------------------------------------- */
	if (iRemainSize >= iPacketLen)
	{
		if (iRemainSize == iPacketLen)
		{
			/* Last packet */
			for (i = 0; i < iPacketLen; i++)
				vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
		}
		else
		{
			for (i = 0; i < iPacketLen; i++)
			{
				vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
				iCurDataPointer++;
			}
		}
	}
	else
	{
		/* Padded packet. If the PPI is 1 then the first byte shall indicate
		   the number of useful bytes that follow, and the data field is
		   completed with padding bytes of value 0x00 */
		vecbiPacket.Enqueue((uint32_t) (iRemainSize / BITS_BINARY),
							BITS_BINARY);

		/* Data */
		for (i = 0; i < iRemainSize; i++)
			vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);

		/* Padding */
		for (i = 0; i < iPacketLen - iRemainSize; i++)
			vecbiPacket.Enqueue(vecbiCurDataUnit.Separate(1), 1);
	}

	/* If this was the last packet, get data for next data unit */
	if (bLastFlag == true)
	{
		/* Generate new data unit */
		MOTSlideShowEncoder.GetDataUnit(vecbiCurDataUnit);
		vecbiCurDataUnit.ResetBitAccess();

		/* Reset data pointer and continuity index */
		iCurDataPointer = 0;
	}

	/* CRC ------------------------------------------------------------------ */
	CCRC CRCObject;

	/* Reset bit access */
	vecbiPacket.ResetBitAccess();

	/* Calculate the CRC and put it at the end of the segment */
	CRCObject.Reset(16);

	/* "byLengthBody" was defined in the header */
	for (i = 0; i < (iTotalPacketSize / BITS_BINARY - 2); i++)
		CRCObject.AddByte(_BYTE(vecbiPacket.Separate(BITS_BINARY)));

	/* Now, pointer in "enqueue"-function is back at the same place, add CRC */
	vecbiPacket.Enqueue(CRCObject.GetCRC(), 16);
}

int
CDataEncoder::Init(CParameter & Param)
{
	/* Init packet length and total packet size (the total packet length is
	   three bytes longer as it includes the header and CRC fields) */

// TODO we only use always the first service right now
	const int iCurSelDataServ = 0;

	Param.Lock();

	iPacketLen = Param.Stream[Param.Service[iCurSelDataServ].iDataStream].iPacketLen * BITS_BINARY;
	iTotalPacketSize = iPacketLen + 24 /* CRC + header = 24 bits */ ;

	iPacketID = Param.Service[iCurSelDataServ].iPacketID;

	Param.Unlock();

	/* Init DAB MOT encoder object */
	MOTSlideShowEncoder.Init();

	/* Generate first data unit */
	MOTSlideShowEncoder.GetDataUnit(vecbiCurDataUnit);
	vecbiCurDataUnit.ResetBitAccess();

	/* Reset pointer to current position in data unit and continuity index */
	iCurDataPointer = 0;
	iContinInd = 0;

	/* Return total packet size */
	return iTotalPacketSize;
}

/******************************************************************************\
* Decoder                                                                      *
\******************************************************************************/
CDataDecoder::CDataDecoder ():iServPacketID (0), DoNotProcessData (true),
	Journaline(*new CJournaline()),
	iOldJournalineServiceID (0), bDecodeEPG(true)
{
		for(size_t i=0; i<MAX_NUM_PACK_PER_STREAM; i++)
			eAppType[i] = AT_NOT_SUP;
}

CDataDecoder::~CDataDecoder ()
{
	delete &Journaline;
}

void
CDataDecoder::ProcessDataInternal(CParameter & ReceiverParam)
{
	int i, j;
	int iPacketID;
	int iNewContInd;
	int iNewPacketDataSize;
	int iOldPacketDataSize;
	int iNumSkipBytes;
	_BINARY biFirstFlag;
	_BINARY biLastFlag;
	_BINARY biPadPackInd;
	CCRC CRCObject;

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == true)
		return;

	/* CRC check for all packets -------------------------------------------- */
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	for (j = 0; j < iNumDataPackets; j++)
	{
		/* Check the CRC of this packet */
		CRCObject.Reset(16);

		/* "- 2": 16 bits for CRC at the end */
		for (i = 0; i < iTotalPacketSize - 2; i++)
		{
			_BYTE b =pvecInputData->Separate(BITS_BINARY);
			CRCObject.AddByte(b);
		}

		/* Store result in vector and show CRC in multimedia window */
		uint16_t crc = pvecInputData->Separate(16);
		if (CRCObject.CheckCRC(crc) == true)
		{
			veciCRCOk[j] = 1;	/* CRC ok */
			ReceiverParam.ReceiveStatus.MOT.SetStatus(RX_OK);
		}
		else
		{
			veciCRCOk[j] = 0;	/* CRC wrong */
			ReceiverParam.ReceiveStatus.MOT.SetStatus(CRC_ERROR);
		}
	}

	/* Extract packet data -------------------------------------------------- */
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	for (j = 0; j < iNumDataPackets; j++)
	{
		/* Check if CRC was ok */
		if (veciCRCOk[j] == 1)
		{
			/* Read header data --------------------------------------------- */
			/* First flag */
			biFirstFlag = (_BINARY) (*pvecInputData).Separate(1);

			/* Last flag */
			biLastFlag = (_BINARY) (*pvecInputData).Separate(1);

			/* Packet ID */
			iPacketID = (int) (*pvecInputData).Separate(2);

			/* Padded packet indicator (PPI) */
			biPadPackInd = (_BINARY) (*pvecInputData).Separate(1);

			/* Continuity index (CI) */
			iNewContInd = (int) (*pvecInputData).Separate(3);

			/* Act on parameters given in header */
			/* Continuity index: this 3-bit field shall increment by one
			   modulo-8 for each packet with this packet Id */
			if ((iContInd[iPacketID] + 1) % 8 != iNewContInd)
				DataUnit[iPacketID].bOK = false;

			/* Store continuity index */
			iContInd[iPacketID] = iNewContInd;

			/* Reset flag for data unit ok when receiving the first packet of
			   a new data unit */
			if (biFirstFlag == true)
			{
				DataUnit[iPacketID].Reset();
				DataUnit[iPacketID].bOK = true;
			}

			/* If all packets are received correctely, data unit is ready */
			if (biLastFlag == true)
			{
				if (DataUnit[iPacketID].bOK == true)
					DataUnit[iPacketID].bReady = true;
			}

			/* Data field --------------------------------------------------- */
			/* Get size of new data block */
			if (biPadPackInd == true)
			{
				/* Padding is present: the first byte gives the number of
				   useful data bytes in the data field. */
				iNewPacketDataSize =
					(int) (*pvecInputData).Separate(BITS_BINARY) *
					BITS_BINARY;

				if (iNewPacketDataSize > iMaxPacketDataSize)
				{
					/* Error, reset flags */
					DataUnit[iPacketID].bOK = false;
					DataUnit[iPacketID].bReady = false;

					/* Set values to read complete packet size */
					iNewPacketDataSize = iNewPacketDataSize;
					iNumSkipBytes = 2;	/* Only CRC has to be skipped */
				}
				else
				{
					/* Number of unused bytes ("- 2" because we also have the
					   one byte which stored the size, the other byte is the
					   header) */
					iNumSkipBytes = iTotalPacketSize - 2 -
						iNewPacketDataSize / BITS_BINARY;
				}

				/* Packets with no useful data are permitted if no packet
				   data is available to fill the logical frame. The PPI
				   shall be set to 1 and the first byte of the data field
				   shall be set to 0 to indicate no useful data. The first
				   and last flags shall be set to 1. The continuity index
				   shall be incremented for these empty packets */
				if ((biFirstFlag == true) &&
					(biLastFlag == true) && (iNewPacketDataSize == 0))
				{
					/* Packet with no useful data, reset flag */
					DataUnit[iPacketID].bReady = false;
				}
			}
			else
			{
				iNewPacketDataSize = iMaxPacketDataSize;

				/* All bytes are useful bytes, only CRC has to be skipped */
				iNumSkipBytes = 2;
			}

			/* Add new data to data unit vector (bit-wise copying) */
			iOldPacketDataSize = DataUnit[iPacketID].vecbiData.Size();

			DataUnit[iPacketID].vecbiData.Enlarge(iNewPacketDataSize);

			/* Read useful bits */
			for (i = 0; i < iNewPacketDataSize; i++)
				DataUnit[iPacketID].vecbiData[iOldPacketDataSize + i] =
					(_BINARY) (*pvecInputData).Separate(1);

			/* Read bytes which are not used */
			for (i = 0; i < iNumSkipBytes; i++)
				(*pvecInputData).Separate(BITS_BINARY);

			/* Use data unit ------------------------------------------------ */
			if (DataUnit[iPacketID].bReady == true)
			{
				/* Decode all IDs regardless whether activated or not
				   (iPacketID == or != iServPacketID) */
				/* Only DAB multimedia is supported */
				//cout << "new data unit for packet id " << iPacketID << " apptype " << eAppType[iPacketID] << endl;

				switch (eAppType[iPacketID])
				{
				case AT_MOTSLISHOW:	/* MOTSlideshow */
					/* Packet unit decoding */
					MOTObject[iPacketID].
						AddDataUnit(DataUnit[iPacketID].vecbiData);
					break;
				case AT_MOTEPG:	/* EPG */
					/* Packet unit decoding */
					if(iEPGPacketID == -1)
					{
                        cerr << "data unit received but EPG packetId not set" << endl;
                        iEPGPacketID = iPacketID;
					}
					MOTObject[iEPGPacketID].AddDataUnit(DataUnit[iPacketID].
														vecbiData);
					break;

				case AT_MOTBROADCASTWEBSITE:	/* MOT Broadcast Web Site */
					/* Packet unit decoding */
					MOTObject[iPacketID].AddDataUnit(DataUnit[iPacketID].
													 vecbiData);
					break;

				case AT_JOURNALINE:
					Journaline.AddDataUnit(DataUnit[iPacketID].vecbiData);
					break;
				default:		/* do nothing */
					;
				}

				/* Packet was used, reset it now for new filling with new data
				   (this will also reset the flag
				   "DataUnit[iPacketID].bReady") */
				DataUnit[iPacketID].Reset();
			}
		}
		else
		{
			/* Skip incorrect packet */
			for (i = 0; i < iTotalPacketSize; i++)
				(*pvecInputData).Separate(BITS_BINARY);
		}
	}
	if ((iEPGService >= 0) && (GetDecodeEPG() == true))	/* if EPG decoding is active */
		DecodeEPG(ReceiverParam);
}

void
CDataDecoder::DecodeEPG(const CParameter & ReceiverParam)
{
	/* Application Decoding - must run all the time and in background */
	if ((DoNotProcessData == false)
		&& (iEPGPacketID >= 0)
		&& MOTObject[iEPGPacketID].NewObjectAvailable())
	{
		CMOTObject NewObj;
		MOTObject[iEPGPacketID].GetNextObject(NewObj);
		string fileName;
		bool advanced = false;
		if (NewObj.iContentType == 7)
		{
			for (size_t i = 0; i < NewObj.vecbProfileSubset.size(); i++)
				if (NewObj.vecbProfileSubset[i] == 2)
				{
					advanced = true;
					break;
				}
			int iScopeId = NewObj.iScopeId;
			if (iScopeId == 0)
				iScopeId = ReceiverParam.Service[iEPGService].iServiceID;
			fileName = epgFilename(NewObj.ScopeStart, iScopeId,
								   NewObj.iContentSubType, advanced);
		}
		else
		{
			fileName = NewObj.strName;
		}

		string path = ReceiverParam.sDataFilesDirectory + "/epg/" + fileName;
		mkdirs(path);
		FILE *f = fopen(path.c_str(), "wb");
		if (f)
		{
			fwrite(&NewObj.Body.vecData.front(), 1,
				   NewObj.Body.vecData.size(), f);
			fclose(f);
		}
	}
}

void
CDataDecoder::InitInternal(CParameter & ReceiverParam)
{
	int iTotalNumInputBits;
	int iTotalNumInputBytes;
	int iCurDataStreamID;
	int iCurSelDataServ;

	/* Init error flag */
	DoNotProcessData = false;

	/* Get current selected data service */
	iCurSelDataServ = ReceiverParam.GetCurSelDataService();

	/* Get current data stream ID */
	iCurDataStreamID = ReceiverParam.Service[iCurSelDataServ].iDataStream;

	/* Get number of total input bits (and bytes) for this module */
	iTotalNumInputBits = ReceiverParam.iNumDataDecoderBits;
	iTotalNumInputBytes = iTotalNumInputBits / BITS_BINARY;

	/* Get the packet ID of the selected service */
	iServPacketID = ReceiverParam.Service[iCurSelDataServ].iPacketID;

	/* Init application type (will be overwritten by correct type later */
	eAppType[iServPacketID] = AT_NOT_SUP;

	CDataParam& dataParam = ReceiverParam.DataParam[iCurDataStreamID][iServPacketID];

	/* Check, if service is activated. Also, only packet services can be
	   decoded */
	if ((iCurDataStreamID != STREAM_ID_NOT_USED) &&
		(dataParam.ePacketModInd == PM_PACKET_MODE))
	{
		/* Calculate total packet size. DRM standard: packet length: this
		   field indicates the length in bytes of the data field of each
		   packet specified as an unsigned binary number (the total packet
		   length is three bytes longer as it includes the header and CRC
		   fields) */
		iTotalPacketSize = ReceiverParam.Stream[iCurDataStreamID].iPacketLen + 3;

		/* Check total packet size, could be wrong due to wrong SDC */
		if ((iTotalPacketSize <= 0) ||
			(iTotalPacketSize > iTotalNumInputBytes))
		{
			/* Set error flag */
			DoNotProcessData = true;
		}
		else
		{
			/* Maximum number of bits for the data part in a packet
			   ("- 3" because two bits for CRC and one for the header) */
			iMaxPacketDataSize = (iTotalPacketSize - 3) * BITS_BINARY;

			/* Number of data packets in one data block */
			iNumDataPackets = iTotalNumInputBytes / iTotalPacketSize;

			/* Only DAB application supported */
			if (dataParam.eAppDomain == CDataParam::AD_DAB_SPEC_APP)
			{
				/* Get application identifier of current selected service, only
				   used with DAB */
                eAppType[iServPacketID] = dataParam.eUserAppIdent;

                if(eAppType[iServPacketID] == AT_JOURNALINE)
                {

					/* Check, if service ID of Journaline application has
					   changed, that indicates that a new transmission is
					   received -> reset decoder in this case. Otherwise
					   use old buffer. That ensures that the decoder keeps
					   old data in buffer when synchronization was lost for
					   a short time */
					const uint32_t iNewServID =
						ReceiverParam.Service[iCurSelDataServ].iServiceID;

					if (iOldJournalineServiceID != iNewServID)
					{

                    // Problem: if two different services point to the same stream, they have different
                    // IDs and the Journaline is reset! TODO: fix this problem...

						/* Reset Journaline decoder and store the new service
						   ID number */
						Journaline.Reset();
						iOldJournalineServiceID = iNewServID;
					}
				}
			}

			/* Init vector for storing the CRC results for each packet */
			veciCRCOk.Init(iNumDataPackets);

			/* Reset data units for all possible data IDs */
			for (int i = 0; i < MAX_NUM_PACK_PER_STREAM; i++)
			{
				DataUnit[i].Reset();

				/* Reset continuity index (CI) */
				iContInd[i] = 0;
			}
		}
	}
	else
		DoNotProcessData = true;

	/* Set input block size */
	iInputBlockSize = iTotalNumInputBits;

	iEPGService = -1;			/* no service */
	iEPGPacketID = -1;

	/* look for EPG */
	for (size_t i = 0; i < ReceiverParam.DataParam.size(); i++)
	{
		for (size_t j = 0; j < ReceiverParam.DataParam[i].size(); j++)
		{
			if ((ReceiverParam.DataParam[i][j].eAppDomain == CDataParam::AD_DAB_SPEC_APP)
			&& (ReceiverParam.DataParam[i][j].eUserAppIdent == AT_MOTTPEG))
			{
				iEPGService = i;
				iEPGPacketID = j;
			}
		}
	}
}

bool
	CDataDecoder::GetMOTObject(CMOTObject & NewObj,
							   const EAppType eAppTypeReq)
{
	bool bReturn = false;

	/* Lock resources */
	Lock();

	/* Check if data service is current MOT application */
	if ((DoNotProcessData == false)
		&& (eAppType[iServPacketID] == eAppTypeReq)
		&& MOTObject[iServPacketID].NewObjectAvailable())
	{
		MOTObject[iServPacketID].GetNextObject(NewObj);
		bReturn = true;
	}
	/* Release resources */
	Unlock();

	return bReturn;
}

bool
	CDataDecoder::GetMOTDirectory(CMOTDirectory & MOTDirectoryOut,
								  const EAppType eAppTypeReq)
{
	bool bReturn = false;

	/* Lock resources */
	Lock();

	/* Check if data service is current MOT application */
	if ((DoNotProcessData == false)
		&& (eAppType[iServPacketID] == eAppTypeReq))
	{
		MOTObject[iServPacketID].GetDirectory(MOTDirectoryOut);
		bReturn = true;
	}
	/* Release resources */
	Unlock();

	return bReturn;
}

void
CDataDecoder::GetNews(const int iObjID, CNews & News)
{
	/* Lock resources */
	Lock();

	/* Check if data service is Journaline application */
	if ((DoNotProcessData == false)
		&& (eAppType[iServPacketID] == AT_JOURNALINE))
		Journaline.GetNews(iObjID, News);

	/* Release resources */
	Unlock();
}
