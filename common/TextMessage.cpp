/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Implementation of the text message application of section 6.5
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

#include "TextMessage.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Text message encoder (for transmitter)                                       *
\******************************************************************************/
void CTextMessageEncoder::Encode(CVector<_BINARY>& pData)
{
	int j;

	/* Set data for this piece from segment data */
	for (int i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
	{
		if (iPieceCnt * NUM_BYTES_TEXT_MESS_IN_AUD_STR + i <
			vecbSegment[iSegCnt].Size() / SIZEOF__BYTE)
		{
			for (j = 0; j < SIZEOF__BYTE; j++)
				pData[i * SIZEOF__BYTE + j] =
					(_BINARY) vecbSegment[iSegCnt].Separate(1);
		}
		else
		{
			/* If the length of the last segment is not a multiple of four then
			   the incomplete frame shall be padded with 0x00 bytes */
			for (j = 0; j < SIZEOF__BYTE; j++)
				pData[i * SIZEOF__BYTE + j] = 0;
		}
	}

	/* Take care of piece count and segment count */
	iPieceCnt++;

	if (iPieceCnt * NUM_BYTES_TEXT_MESS_IN_AUD_STR >=
		vecbSegment[iSegCnt].Size() / SIZEOF__BYTE)
	{
		iPieceCnt = 0;

		/* Next segment (test for wrap around) */
		iSegCnt++;
		if (iSegCnt == iNumSeg)
			iSegCnt = 0;

		vecbSegment[iSegCnt].ResetBitAccess();
	}
}

void CTextMessageEncoder::SetMessage(const string& strMessage)
{
	int		i, j;
	int		iPosInStr;
	int		iLenBytesOfText;
	int		iNumBodyBytes;
	_BINARY	biFirstFlag;
	_BINARY	biLastFlag;
	CCRC	CRCObject;

	/* Reset counter for encoder function */
	iSegCnt = 0;
	iPieceCnt = 0;

	/* Get length of text message. 
       TODO: take care of multiple byte characters (UTF-8 coding)! */
	iLenBytesOfText = strMessage.length();

	/* Calculate required number of segments. The body shall contain 16 bytes
	   of character data */
	iNumSeg = ceil((_REAL) iLenBytesOfText / 16);


	/* Generate segments ---------------------------------------------------- */
	/* Reset position in string */
	iPosInStr = 0;

	for (i = 0; i < iNumSeg; i++)
	{
		/* Calculate number of bytes for body */
		const int iRemainingBytes = iLenBytesOfText - iPosInStr;
		
		if (iRemainingBytes > BYTES_PER_SEG_TEXT_MESS)
			iNumBodyBytes = BYTES_PER_SEG_TEXT_MESS;
		else
			iNumBodyBytes = iRemainingBytes;

		/* Init segment vector: "Beginning of segment" 4 * 8 bits,
		   Header 16 bits, body n * 8 bits, CRC 16 bits */
		vecbSegment[i].Init(4 * 8 + 16 + iNumBodyBytes * 8 + 16);

		/* Reset enqueue function */
		vecbSegment[i].ResetBitAccess();


		/* Generate "beginning of segment" identification by "all 0xFF" ----- */
		for (j = 0; j < NUM_BYTES_TEXT_MESS_IN_AUD_STR; j++)
			vecbSegment[i].Enqueue((_UINT32BIT) 0xFF, SIZEOF__BYTE);


		/* Header ----------------------------------------------------------- */
		/* Toggle bit (not use right now since we only offer one message
		   at the moment. TODO: more than only one message) */
		vecbSegment[i].Enqueue((_UINT32BIT) 0, 1);

		/* Determine position of segment */
		if (i == 0)
			biFirstFlag = 1;
		else
			biFirstFlag = 0;

		if (iLenBytesOfText - iPosInStr < BYTES_PER_SEG_TEXT_MESS)
			biLastFlag = 1;
		else
			biLastFlag = 0;

		/* Enqueue first flag */
		vecbSegment[i].Enqueue((_UINT32BIT) biFirstFlag, 1);

		/* Enqueue last flag */
		vecbSegment[i].Enqueue((_UINT32BIT) biLastFlag, 1);

		/* Command flag (no commands used right now, TODO) */
		vecbSegment[i].Enqueue((_UINT32BIT) 0, 1);

		/* Field 1: specify the number of bytes in the body minus 1 (It
		   shall normally take the value 15 except in the last segment) */
		vecbSegment[i].Enqueue((_UINT32BIT) iNumBodyBytes - 1, 4);

		/* Field 2, Rfa. The bit shall be set to zero until it is defined */
		vecbSegment[i].Enqueue((_UINT32BIT) 0, 1);

		/* SegNum: specify the sequence number of the current segment 
		   minus 1. The value 0 is reserved for future use */
		vecbSegment[i].Enqueue((_UINT32BIT) i, 3);

		/* Rfa. These bits shall be set to zero until they are defined */
		vecbSegment[i].Enqueue((_UINT32BIT) 0, 4);


		/* Body ------------------------------------------------------------- */
		/* Set body bytes */
		for (j = 0; j < iNumBodyBytes; j++)
		{
			vecbSegment[i].Enqueue((_UINT32BIT) strMessage.at(iPosInStr),
				SIZEOF__BYTE);

			iPosInStr++;
		}


		/* CRC -------------------------------------------------------------- */
		/* Reset bit access and skip segment beginning piece (all 0xFFs) */
		vecbSegment[i].ResetBitAccess();
		vecbSegment[i].Separate(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);

		/* Calculate the CRC and put it at the end of the segment */
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (j = 0; j < iNumBodyBytes + 2 /* Header */; j++)
			CRCObject.AddByte(vecbSegment[i].Separate(SIZEOF__BYTE));

		/* Now, pointer in "enqueue"-function is back at the same place, 
		   add CRC */
		vecbSegment[i].Enqueue(CRCObject.GetCRC(), 16);


		/* Reset bit access for using segment in encoder function */
		vecbSegment[i].ResetBitAccess();
	}
}


/******************************************************************************\
* Text message decoder (for receiver)                                          *
\******************************************************************************/
void CTextMessageDecoder::Decode(CVector<_BINARY>& pData)
{
	int			i, j;
	_BOOLEAN	bBeginningFound;

	/* Reset binary vector function. Always four bytes of data */
	pData.ResetBitAccess();

	/* Identify the beginning of a segment, all four bytes are 0xFF, otherwise
	   store total new buffer in internal intermediate buffer. This buffer is
	   read, when a beginning was found */
	bBeginningFound = TRUE;
	for (i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
	{
		if (pData.Separate(SIZEOF__BYTE) != 0xFF)
			bBeginningFound = FALSE;
	}

	if (bBeginningFound)
	{
		/* Analyse old stream buffer, which should be completed now --------- */
		/* Get header information. This function separates 16 bits from 
		   stream */
		biStreamBuffer.ResetBitAccess();
		ReadHeader();

		/* Check CRC */
		biStreamBuffer.ResetBitAccess();
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (i = 0; i < byLengthBody + 2 /* Header */; i++)
			CRCObject.AddByte(biStreamBuffer.Separate(SIZEOF__BYTE));

		if (CRCObject.CheckCRC(biStreamBuffer.Separate(16)) == TRUE)
		{
			if (biCommandFlag == 1)
			{
				switch (byCommand)
				{
				case 1: /* 0001 */
					/* The message shall be removed from the display */
					ClearText();
					break;
				}
			}
			else
			{
				/* Body ----------------------------------------------------- */
				/* Evaluate toggle bit */
				if (biToggleBit != biOldToggleBit)
				{
					/* A new message is sent, clear all old segments */
					ResetSegments();

					iNumSegments = 0;

					biOldToggleBit = biToggleBit;
				}


				/* Read all bytes in stream buffer -------------------------- */
				/* First, check if segment was already OK and if new data has
				   the same content or not. If the content is different, a new
				   message is being send, clear all other segments */
				if (Segment[bySegmentID].bIsOK == TRUE)
				{
					/* Reset bit access and skip header bits to go directly to 
					the body bytes */
					biStreamBuffer.ResetBitAccess();
					biStreamBuffer.Separate(16);

					_BOOLEAN bIsSame = TRUE;
					for (i = 0; i < byLengthBody; i++)
					{
						if (Segment[bySegmentID].byData[i] != 
							biStreamBuffer.Separate(SIZEOF__BYTE))
						{
							bIsSame = FALSE;
						}
					}

					if (bIsSame == FALSE)
					{
						/* A new message is sent, clear all old segments */
						ResetSegments();

						iNumSegments = 0;
					}
				}

				/* Reset bit access and skip header bits to go directly to the
				   body bytes */
				biStreamBuffer.ResetBitAccess();
				biStreamBuffer.Separate(16);

				/* Get all body bytes */
				for (i = 0; i < byLengthBody; i++)
					Segment[bySegmentID].byData[i] = 
						biStreamBuffer.Separate(SIZEOF__BYTE);

				/* Set length of this segment and OK flag */
				Segment[bySegmentID].iNumBytes = byLengthBody;
				Segment[bySegmentID].bIsOK = TRUE;

				/* Check length of text message */
				if (biLastFlag == 1)
					iNumSegments = bySegmentID + 1;

				/* Set text to display */
				SetText();
			}
		}

		/* Reset byte count */
		iBitCount = 0;
	}
	else
	{
		for (i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
		{
			/* Check, if number of bytes is not too big */
			if (iBitCount < TOT_NUM_BITS_PER_PIECE)
			{
				for (j = 0; j < SIZEOF__BYTE; j++)
					biStreamBuffer[iBitCount + j] = pData[i * SIZEOF__BYTE + j];
				
				iBitCount += SIZEOF__BYTE;
			}
		}
	}
}

void CTextMessageDecoder::ReadHeader()
{
	/* Toggle bit */
	biToggleBit = (_BINARY) biStreamBuffer.Separate(1);

	/* First flag */
	biFirstFlag = (_BINARY) biStreamBuffer.Separate(1);

	/* Last flag */
	biLastFlag = (_BINARY) biStreamBuffer.Separate(1);

	/* Command flag */
	biCommandFlag = biStreamBuffer.Separate(1);
	if (biCommandFlag == 1)
	{
		/* Command */
		/* Field 1 */
		byCommand = (_BYTE) biStreamBuffer.Separate(4);

		/* 0001: the message shall be removed from the display */

		/* Field 2: This field shall contain the value "1111", not
		   tested here */
		biStreamBuffer.Separate(4);

		/* When a command ist send, no body is present */
		byLengthBody = 0;
	}
	else
	{
		/* Field 1: specify the number of bytes in the body minus 1 */
		byLengthBody = (_BYTE) biStreamBuffer.Separate(4) + 1;

		/* Field 2, Rfa */
		biStreamBuffer.Separate(1);

		/* SegNum: specify the sequence number of the current segment 
		   minus 1. The value 0 is reserved for future use */
		bySegmentID = (_BYTE) biStreamBuffer.Separate(3);

		if (biFirstFlag == 1)
			bySegmentID = 0;
	}

	/* Rfa */
	biStreamBuffer.Separate(4);
}

void CTextMessageDecoder::Init(string* pstrNewPText)
{
	int i;

	pstrText = pstrNewPText;
	biOldToggleBit = 0;
	iBitCount = 0;

	/* Init segments */
	ResetSegments();

	iNumSegments = 0;

	/* Init and reset stream buffer */
	biStreamBuffer.Init(TOT_NUM_BITS_PER_PIECE, 0);
}

void CTextMessageDecoder::SetText()
{
	int			i, j;
	_BOOLEAN	bTextMessageReady;

#ifndef _DEBUG_
	if (iNumSegments != 0)
#endif
	{
		/* Check, if all segments are ready */
		bTextMessageReady = TRUE;
		for (i = 0; i < iNumSegments; i++)
		{
			if (Segment[i].bIsOK == FALSE)
				bTextMessageReady = FALSE;
		}

#ifndef _DEBUG_
		if (bTextMessageReady == TRUE)
#endif
		{
			/* Clear text */
			*pstrText = "";

// We can run into problems with the rich text (in QT) if the text message
// contains html tags as well: FIXME

#ifdef USE_QT_GUI
			/* Center text */
			(*pstrText).append("<center>", 8);
#endif

			for (i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
			{
				if (Segment[i].bIsOK == TRUE)
				{
					for (j = 0; j < Segment[i].iNumBytes; j++)
					{
						/* Get character */
						char cNewChar = Segment[i].byData[j];

						switch (cNewChar)
						{
						case 0x0A:
							/* Code 0x0A may be inserted to indicate a preferred
							   line break */
#ifdef USE_QT_GUI
							(*pstrText).append("<br>", 4);
#else
							(*pstrText).append("\r\n", 2);
#endif
							break;

						case 0x1F:
							/* Code 0x1F (hex) may be inserted to indicate a
							   preferred word break. This code may be used to 
							   display long words comprehensibly */
#ifdef USE_QT_GUI
							(*pstrText).append("<br>", 4);
#else
							(*pstrText).append("\r\n", 2);
#endif
							break;

						case 0x0B:
							/* End of a headline */
#ifdef USE_QT_GUI
							/* Use rich text possibility offered by QT */
							(*pstrText).insert(0, "<b><u>", 6);
							(*pstrText).append("</center></u></b><br><center>", 29);
#else
							/* Two line-breaks */
							(*pstrText).append("\r\n\r\n", 4);
							cNewChar = 0x0A;
#endif
							break;

						default:
							/* Append new character */
							(*pstrText).append(&cNewChar, 1);
							break;
						}
					}
				}
			}

#ifdef USE_QT_GUI
			/* End of centering text */
			(*pstrText).append("</center>", 9);
#endif
		}
	}
}

void CTextMessageDecoder::ClearText()
{
	/* Reset segments */
	ResetSegments();

	/* Clear text */	
	*pstrText = "";
}

void CTextMessageDecoder::ResetSegments()
{
	for (int i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
		Segment[i].bIsOK = FALSE;
}
