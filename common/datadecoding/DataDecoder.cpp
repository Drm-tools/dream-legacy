/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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


/* Implementation *************************************************************/
void CDataDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	_UINT32BIT	CRC;
	int			i, j;
	int			iPacketID;
	int			iNewContInd;
	int			iNewPacketDataSize;
	int			iOldPacketDataSize;
	int			iNumSkipBytes;
	_BINARY		biFirstFlag;
	_BINARY		biLastFlag;
	_BINARY		biPadPackInd;
	CCRC		CRCObject;

	/* CRC check for all packets -------------------------------------------- */
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	for (j = 0; j < iNumDataPackets; j++)
	{
		/* Check the CRC of this packet */
		CRCObject.Reset(16);

		/* "- 2": 16 bits for CRC at the end */
		for (i = 0; i < iTotalPacketSize - 2; i++)
			CRCObject.AddByte((_BYTE) (*pvecInputData).Separate(SIZEOF__BYTE));

		CRC = CRCObject.GetCRC();

		/* Store result in vector and show CRC in multimedia window */
		if (CRC == (_UINT32BIT) (*pvecInputData).Separate(16))
		{
			veciCRCOk[j] = 1; /* CRC ok */
			PostWinMessage(MS_MOT_OBJ_STAT, 0); /* Green light */
		}
		else
		{
			veciCRCOk[j] = 0; /* CRC wrong */
			PostWinMessage(MS_MOT_OBJ_STAT, 2); /* Red light */
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
				DataUnit[iPacketID].bOK = FALSE;

			/* Store continuity index */
			iContInd[iPacketID] = iNewContInd;

			/* Reset flag for data unit ok when receiving the first packet of
			   a new data unit */
			if (biFirstFlag == TRUE)
			{
				DataUnit[iPacketID].Reset();

				DataUnit[iPacketID].bOK = TRUE;
			}

			/* If all packets are received correctely, data unit is ready */
			if (biLastFlag == TRUE)
				if (DataUnit[iPacketID].bOK == TRUE)
					DataUnit[iPacketID].bReady = TRUE;


			/* Data field --------------------------------------------------- */
			/* Get size of new data block */
			if (biPadPackInd == TRUE)
			{
				/* Padding is present: the first byte gives the number of
				   useful data bytes in the data field. */
				iNewPacketDataSize =
					(int) (*pvecInputData).Separate(SIZEOF__BYTE) *
					SIZEOF__BYTE;

				/* Number of unused bytes ("- 2" because we also have the one
				   byte which stored the size, the other byte is the header) */
				iNumSkipBytes =
					iTotalPacketSize - 2 - iNewPacketDataSize / SIZEOF__BYTE;

				/* Packets with no useful data are permitted if no packet
				   data is available to fill the logical frame. The PPI
				   shall be set to 1 and the first byte of the data field
				   shall be set to 0 to indicate no useful data. The first
				   and last flags shall be set to 1. The continuity index
				   shall be incremented for these empty packets */
				if ((biFirstFlag == TRUE) &&
					(biLastFlag == TRUE) &&
					(iNewPacketDataSize == 0))
				{
					/* Packet with no useful data, reset flag */
					DataUnit[iPacketID].bReady = FALSE;
				}
			}
			else
			{
				iNewPacketDataSize = (iTotalPacketSize - 3) * SIZEOF__BYTE;

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
				(*pvecInputData).Separate(SIZEOF__BYTE);


			/* Use data unit ------------------------------------------------ */
			if (DataUnit[iPacketID].bReady == TRUE)
			{

// TEST
// Decode all IDs regardless whether activated or not
//				/* Use only data units with the correct packet ID */
//				if (iPacketID == iServPacketID)
				{
					/* Only DAB multimedia is supported */
					if (eServAppDomain == CParameter::AD_DAB_SPEC_APP)
					{
						switch (iDABUserAppIdent)
						{
						case 2: /* MOTSlideshow */
							/* Packet unit decoding */
							DABData[iPacketID].
								AddDataUnit(DataUnit[iPacketID].vecbiData,
								MOTPicture);
							break;
						}
					}
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
				(*pvecInputData).Separate(SIZEOF__BYTE);
		}
	}
}

void CDataDecoder::InitInternal(CParameter& ReceiverParam)
{
	int	iCurDataStreamID;
	int iCurSelDataServ;

	/* Get current selected data service */
	iCurSelDataServ = ReceiverParam.GetCurSelDataService();

	if (ReceiverParam.bUsingMultimedia)
	{
		/* Get current data stream ID */
		iCurDataStreamID =
			ReceiverParam.Service[iCurSelDataServ].DataParam.iStreamID;
	}
	else
		iCurDataStreamID = STREAM_ID_NOT_USED;

	/* Check, if service is activated */
	if (iCurDataStreamID != STREAM_ID_NOT_USED)
	{
		/* Length of higher and lower protected part of data stream */
		iLenDataHigh = ReceiverParam.Stream[iCurDataStreamID].iLenPartA;
		iLenDataLow = ReceiverParam.Stream[iCurDataStreamID].iLenPartB;

		/* Calculate total packet size. DRM standard: packet length: this field
		   indicates the length in bytes of the data field of each packet specified
		   as an unsigned binary number (the total packet length is three bytes
		   longer as it includes the header and CRC fields) */
		iTotalPacketSize =
			ReceiverParam.Service[iCurSelDataServ].DataParam.iPacketLen + 3;

		/* Check total packet size, could be wrong due to wrong SDC */
		if ((iTotalPacketSize == 0) ||
			(iTotalPacketSize > iLenDataHigh + iLenDataLow))
		{
			/* False parameter, set everyting to zero and disable this module */
			iLenDataHigh = 0;
			iLenDataLow = 0;
		}
		else
		{
			/* Number of data packets in one data block */
			iNumDataPackets = (iLenDataHigh + iLenDataLow) / iTotalPacketSize;

			/* Get the packet ID of the selected service */
			iServPacketID =
				ReceiverParam.Service[iCurSelDataServ].DataParam.iPacketID;

			/* Get application domain of selected service */
			eServAppDomain =
				ReceiverParam.Service[iCurSelDataServ].DataParam.eAppDomain;

			/* Get application identifier of current selected service, only used
			   with DAB */
			iDABUserAppIdent =
				ReceiverParam.Service[iCurSelDataServ].DataParam.iUserAppIdent;

			/* Vector for storing the CRC results for each packet */
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
	{
		/* Selected stream is not active, set everyting to zero */
		iLenDataHigh = 0;
		iLenDataLow = 0;
	}

	/* Set input block size */
	iInputBlockSize = (iLenDataHigh + iLenDataLow) * SIZEOF__BYTE;
}

void CDataDecoder::GetSlideShowPicture(CMOTPicture& NewPic)
{
	/* Lock resources */
	Lock();

	/* Copy picture content */
	NewPic.iTransportID = MOTPicture.iTransportID;
	NewPic.strFormat = MOTPicture.strFormat;

	NewPic.vecbRawData.Init(MOTPicture.vecbRawData.Size());
	for (int i = 0; i < MOTPicture.vecbRawData.Size();	i++)
		NewPic.vecbRawData[i] = MOTPicture.vecbRawData[i];

	/* Release resources */
	Unlock();
}
