/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	Implements the PFT (Protection, Fragmentation and Transport) layer of the
 *	Communications Protocol (DCP) as described in ETSI TS 102 821.
 *
 *  TODO: support RS FEC and any other missing features
 *  In this version, only in-order delivery is supported.
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

#include "Pft.h"
#include "../util/CRC.h"
#include <iostream>

CPft::CPft(int isrc, int idst) : iSource(isrc), iDest(idst), mapFragments()
{
}

bool CPft::DecodePFTPacket(const vector<_BYTE>& vecIn, vector<_BYTE>& vecOut)
{
	/* SYNC: two-byte ASCII representation of "PF" (2 bytes) */

	/* Check if string is correct */
	if ((vecIn[0] != 'P') && (vecIn[1] != 'F'))
	{
		cerr << "PF not found" << endl;
		return false;
	}

	iHeaderLen = 14;
	iPseq = (uint16_t(vecIn[2]) << 8) + vecIn[3];
	iFindex = (uint32_t(vecIn[4]) << 16) + (uint32_t(vecIn[5]) << 8) + vecIn[6];
	iFcount = (uint32_t(vecIn[7]) << 16) + (uint32_t(vecIn[8]) << 8) + vecIn[9];
	uint16_t n = (uint16_t(vecIn[10]) << 8) + vecIn[11];
	iFEC = (n&0x8000)?1:0;
	iAddr = (n&0x4000)?1:0;
	iPlen = n&0x3FFF;

	int iRSk, iRSz;
	if(iFEC==1)
	{
		iRSk = (int) vecIn[12];
		iRSz = (int) vecIn[13];
		iHeaderLen+=2;
	}

	int iPktSource=0, iPktDest=0;
	if(iAddr==1)
	{
		iPktSource = (uint16_t(vecIn[iHeaderLen-2]) << 8) + vecIn[iHeaderLen-1];
		iPktDest = (uint16_t(vecIn[iHeaderLen]) << 8) + vecIn[iHeaderLen+1];
		iHeaderLen+=4;
	}

	const int iHCRC = (uint16_t(vecIn[iHeaderLen-2]) << 8) + vecIn[iHeaderLen-1];

	/* CRC check ------------------------------------------------------------ */
	CCRC CRCObject;
	CRCObject.Reset(16);

	int i;
	for (i = 0; i < iHeaderLen-2; i++)
		CRCObject.AddByte(vecIn[i]);
	const _BOOLEAN bCRCOk = CRCObject.CheckCRC(iHCRC);
	if (!bCRCOk)
	{
		cerr << "PFT CRC Error" << endl;
		return false;
	}

	if( (iSource!=-1) && (iSource!=iPktSource) )
		return false;
	if( (iDest!=-1) && (iDest!=iPktDest) )
		return false;

	vector<_BYTE> frag;
	for(size_t j=iHeaderLen; j<vecIn.size(); j++)
		frag.push_back(vecIn[j]);
	if(iFEC==1)
		return DecodePFTPacketWithFEC(frag, vecOut);
	else
		return DecodeSimplePFTPacket(frag, vecOut);
}

bool CPft::DecodeSimplePFTPacket(const vector<_BYTE>& vecIn, vector<_BYTE>& vecOut)
{
	if(iFcount==1)
	{
		vecOut = vecIn;
		return true;
	}
	/* CReassembler does not modify the input vector, but its derived classes using CVectors do. */
	mapFragments[iPseq].AddSegment(const_cast<vector<_BYTE>&>(vecIn), iFindex, iFindex==(iFcount-1));

	if(mapFragments[iPseq].Ready())
	{
		vecOut = mapFragments[iPseq].vecData;
		return true;
	}

	return false;
}

bool CPft::DecodePFTPacketWithFEC(const vector<_BYTE>&, vector<_BYTE>&)
{
	cerr << "sorry, PFT/FEC not implemented yet" << endl;
	return false;
}
