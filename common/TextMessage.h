/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TextMessage.cpp
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

#if !defined(TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "CRC.h"
#include "Vector.h"


/* Definitions ****************************************************************/
/* The text message may comprise up to 8 segments ... The body shall contain
   16 bytes of character data ... 
   We add "+ 1" since the entry "0" is not used */
#define MAX_NO_SEG_TEXT_MESSAGE			(8 + 1)
#define BYTES_PER_SEG_TEXT_MESS			16

/* Four pieces of data per MSC frame */
#define NO_BY_PER_PIECE					4

#define TOT_NO_BITS_PER_PIECE			((BYTES_PER_SEG_TEXT_MESS /* Max body */ + 2 /* Header */ + 2 /* CRC */) * SIZEOF__BYTE)


/* Classes ********************************************************************/
class CTextMessSegment
{
public:
	CTextMessSegment() {bIsOK = FALSE; iNoBytes = 0;}

	_BYTE		byData[BYTES_PER_SEG_TEXT_MESS];
	_BOOLEAN	bIsOK;
	int			iNoBytes;
};

class CTextMessage
{
public:
	CTextMessage() {}
	virtual ~CTextMessage() {}

	void Decode(CVector<_BINARY>& pData);
	void Init(string* pstrNewPText);

protected:
	void ReadHeader();
	void ClearText();
	void SetText();

	CVector<_BINARY>	biStreamBuffer;

	string*				pstrText;

	_BINARY				biCommandFlag;

	_BINARY				biFirstFlag;
	_BINARY				biLastFlag;
	_BYTE				byCommand;
	_BYTE				bySegmentNo;
	_BINARY				biToggleBit;
	_BYTE				byLengthBody;
	int					iBitCount;
	int					iNoSegments;

	_BINARY				biOldToggleBit;

	CTextMessSegment	Segment[MAX_NO_SEG_TEXT_MESSAGE];

	CCRC				CRCObject;
};


#endif // !defined(TEXT_MESSAGE_H__3B00_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
