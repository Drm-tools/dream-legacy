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
void CTextMessage::Decode(CVector<_BINARY>& pData)
{
	int			i, j;
	_BOOLEAN	bBeginningFound;

	/* Reset binary vector function. Always four bytes of data */
	pData.ResetBitAccess();

	/* Identify the beginning of a segment, all four bytes are 0xFF, otherwise
	   store total new buffer in internal intermediate buffer. This buffer is
	   read, when a beginning was found */
	bBeginningFound = TRUE;
	for (i = 0; i < NUM_BY_PER_PIECE; i++)
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

		if (CRCObject.GetCRC() == biStreamBuffer.Separate(16))
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

					iNoSegments = 0;

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

						iNoSegments = 0;
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
				Segment[bySegmentID].iNoBytes = byLengthBody;
				Segment[bySegmentID].bIsOK = TRUE;

				/* Check length of text message */
				if (biLastFlag == 1)
					iNoSegments = bySegmentID;

				/* Set text to display */
				SetText();
			}
		}

		/* Reset byte count */
		iBitCount = 0;
	}
	else
	{
		for (i = 0; i < NUM_BY_PER_PIECE; i++)
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

void CTextMessage::ReadHeader()
{
	/* Header ------------------------------------------------------- */
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

void CTextMessage::Init(string* pstrNewPText)
{
	int i;

	pstrText = pstrNewPText;
	biOldToggleBit = 0;
	iBitCount = 0;

	/* Init segments */
	ResetSegments();

	iNoSegments = 0;

	/* Init and reset stream buffer */
	biStreamBuffer.Init(TOT_NUM_BITS_PER_PIECE, 0);
}

void CTextMessage::SetText()
{
	int			i, j;
	_BOOLEAN	bTextMessageReady;

#ifndef _DEBUG_
	if (iNoSegments != 0)
#endif
	{
		/* Check, if all segments are ready */
		bTextMessageReady = TRUE;
		for (i = 0; i < iNoSegments + 1; i++)
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

#ifdef QT_THREAD_SUPPORT
			/* Center text */
			(*pstrText).append("<center>", 8);
#endif

			for (i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
			{
				if (Segment[i].bIsOK == TRUE)
				{
					for (j = 0; j < Segment[i].iNoBytes; j++)
					{
						/* Get character */
						char cNewChar = Segment[i].byData[j];

						switch (cNewChar)
						{
						case 0x0A:
							/* Code 0x0A may be inserted to indicate a preferred
							   line break */
#ifdef QT_THREAD_SUPPORT
							(*pstrText).append("<br>", 4);
#else
							(*pstrText).append("\r\n", 2);
#endif
							break;

						case 0x1F:
							/* Code 0x1F (hex) may be inserted to indicate a
							   preferred word break. This code may be used to 
							   display long words comprehensibly */
#ifdef QT_THREAD_SUPPORT
							(*pstrText).append("<br>", 4);
#else
							(*pstrText).append("\r\n", 2);
#endif
							break;

						case 0x0B:
							/* End of a headline */
#ifdef QT_THREAD_SUPPORT
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

#ifdef QT_THREAD_SUPPORT
			/* End of centering text */
			(*pstrText).append("</center>", 9);
#endif
		}
	}
}

void CTextMessage::ClearText()
{
	/* Reset segments */
	ResetSegments();

	/* Clear text */	
	*pstrText = "";
}

void CTextMessage::ResetSegments()
{
	for (int i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
		Segment[i].bIsOK = FALSE;
}
