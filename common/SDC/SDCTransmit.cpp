/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	SDC
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

#include "SDC.h"


/* Implementation *************************************************************/
void CSDCTransmit::SDCParam(CVector<_BINARY>* pbiData, CParameter& Parameter)
{
/* 
	Put SDC parameters on a stream 
*/
	int i;
	int iNoUsedBits;
	int iLengthDataFieldBytes;
	int	iUsefulBitsSDC;
	_BYTE byFirstByte;

	/* Calculate length of data field in bytes 
	   (consistant to table 61 in (6.4.1)) */
	iLengthDataFieldBytes = 
		(int) ((_REAL) (Parameter.iNumSDCBitsPerSFrame - 20) / 8);

	/* 20 bits from AFS index and CRC */
	iUsefulBitsSDC = 20 + iLengthDataFieldBytes * 8;

	/* Reset enqueue function */
	(*pbiData).ResetBitAccess();


	/* SDC Header ----------------------------------------------------------- */
	/* AFS index (not used by this application, insert a "1" */
	(*pbiData).Enqueue((_UINT32BIT) 1, 4);


	/* Data Entities -------------------------------------------------------- */
	/* Init bit-count */
	iNoUsedBits = 0;

// Choose types, TEST. Send only important types for this test!
	/* Type 0 */
	iNoUsedBits += DataEntityType0(pbiData, Parameter);

	/* Type 9 */
	iNoUsedBits += DataEntityType9(pbiData, 0, Parameter);



	/* Zero-pad the unused bits in this SDC-block 
	   ("- 20" for the AFS-index and CRC!) */
	for (i = 0; i < iUsefulBitsSDC - iNoUsedBits - 20; i++)
		(*pbiData).Enqueue((_UINT32BIT) 0, 1);


	/* CRC ------------------------------------------------------------------ */
	/* Calculate the CRC and put at the end of the stream */
	CRCObject.Reset(16);

	(*pbiData).ResetBitAccess();

	/* Special treatment of SDC data stream: The CRC (Cyclic Redundancy 
	Check) field shall contain a 16-bit CRC calculated over the AFS 
	index coded in an 8-bit field (4 msbs are 0) and the data field.
	4 MSBs from AFS-index. Insert four "0" in the data-stream */
	byFirstByte = (_BYTE) (*pbiData).Separate(4);
	CRCObject.AddByte(byFirstByte);

	for (i = 0; i < (iUsefulBitsSDC - 4) / SIZEOF__BYTE - 2; i++)
		CRCObject.AddByte((_BYTE) (*pbiData).Separate(SIZEOF__BYTE));

	/* Now, pointer in "enqueue"-function is back at the same place, 
	   add CRC */
	(*pbiData).Enqueue(CRCObject.GetCRC(), 16);
}


/******************************************************************************\
* Data entity Type 0														   *
\******************************************************************************/
int CSDCTransmit::DataEntityType0(CVector<_BINARY>* pbiData, 
								  CParameter& Parameter)
{
	_UINT32BIT iRfuDummy;
	int iNoBitsTotal;
	const int iNoBitsHeader = 12; /* Data entity header */

	/* 24 bits for each stream description + 4 bits for protection levels */
	iNoBitsTotal = 4 + Parameter.GetTotNumServices() * 24;

	/**** Multiplex description data entity - type 0 ****/
	/* Length of the body, excluding the initial 4 bits ("- 4"), 
	   measured in bytes ("/ 8") */
	_UINT32BIT iLengthInBytes = (iNoBitsTotal - 4) / 8;
	(*pbiData).Enqueue(iLengthInBytes, 7);

	/* Version flag (not used in this implementation) */
	(*pbiData).Enqueue((_UINT32BIT) 0, 1);

	/* Data entity type */
	(*pbiData).Enqueue((_UINT32BIT) 00, 4); /* Type 00 */

	/* ********** */

	/* Protection level for part A */
	(*pbiData).Enqueue((_UINT32BIT) Parameter.MSCPrLe.iPartA, 2);

	/* Protection level for part B */
	(*pbiData).Enqueue((_UINT32BIT) Parameter.MSCPrLe.iPartB, 2);

	for (int i = 0; i < Parameter.GetTotNumServices(); i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hirarchical data */
		if ((i == 0) && 
			((Parameter.eMSCCodingScheme == CParameter::CS_3_HMSYM) ||
			(Parameter.eMSCCodingScheme == CParameter::CS_3_HMMIX)))
		{
			/* Protection level for hierarchical */
			(*pbiData).Enqueue((_UINT32BIT) Parameter.MSCPrLe.iHierarch, 2);
		
			/* rfu */
			iRfuDummy = 0;
			(*pbiData).Enqueue((_UINT32BIT) iRfuDummy, 10);
	
			/* Data length for hierarchical */
			(*pbiData).Enqueue((_UINT32BIT) Parameter.Stream[i].iLenPartB, 12);
		}
		else
		{
			/* Data length for part A */
			(*pbiData).Enqueue((_UINT32BIT) Parameter.Stream[i].iLenPartA, 12);
		
			/* Data length for part B */
			(*pbiData).Enqueue((_UINT32BIT) Parameter.Stream[i].iLenPartB, 12);
		}
	}

	return iNoBitsTotal + iNoBitsHeader;
}


/******************************************************************************\
* Data entity Type 5														   *
\******************************************************************************/
int CSDCTransmit::DataEntityType5(CVector<_BINARY>* pbiData, int ServiceID, 
								  CParameter& Parameter)
{
	_UINT32BIT iRfuDummy = 0;
	int iNoBitsTotal;
	const int iNoBitsHeader = 12; /* Data entity header */

	/* Set total number of bits */
	switch (Parameter.Service[ServiceID].DataParam.ePacketModInd)
	{
	case CParameter::PM_SYNCHRON_STR_MODE:
		iNoBitsTotal = 12 /* + application data TODO! */;
		break;

	case CParameter::PM_PACKET_MODE:
		iNoBitsTotal = 20 /* + application data TODO! */;
		break;
	}

	/**** Multiplex description data entity - type 5 ****/
	/* Length of the body, excluding the initial 4 bits ("- 4"), 
	   measured in bytes ("/ 8") */
	(*pbiData).Enqueue((_UINT32BIT) (iNoBitsTotal - 4) / 8, 7);

	/* Version flag (not used in this implementation) */
	(*pbiData).Enqueue((_UINT32BIT) 0, 1);

	/* Data entity type */
	(*pbiData).Enqueue((_UINT32BIT) 05, 4); /* Type 05 */

	/* ********** */
	/* Short Id */
	(*pbiData).Enqueue((_UINT32BIT) ServiceID, 2);

	/* Stream Id */
	(*pbiData).Enqueue((_UINT32BIT) Parameter.Service[ServiceID].DataParam.
		iStreamID, 2);

	/* Packet mode indicator */
	switch (Parameter.Service[ServiceID].DataParam.ePacketModInd)
	{
	case CParameter::PM_SYNCHRON_STR_MODE:
		(*pbiData).Enqueue(0 /* 0 */, 1);

		/* Descriptor */
		(*pbiData).Enqueue((_UINT32BIT) iRfuDummy, 7);
		break;

	case CParameter::PM_PACKET_MODE:
		(*pbiData).Enqueue(1 /* 1 */, 1);

		/* Descriptor */
		/* Data unit indicator */
		switch (Parameter.Service[ServiceID].DataParam.eDataUnitInd)
		{
		case CParameter::DU_SINGLE_PACKETS:
			(*pbiData).Enqueue(0 /* 0 */, 1);
			break;

		case CParameter::DU_DATA_UNITS:
			(*pbiData).Enqueue(1 /* 1 */, 1);
			break;
		}

		/* Packet Id */
		(*pbiData).Enqueue( 
			(_UINT32BIT) Parameter.Service[ServiceID].DataParam.iPacketID, 2);

		/* Application domain */
		switch (Parameter.Service[ServiceID].DataParam.eAppDomain)
		{
		case CParameter::AD_DRM_SPEC_APP:
			(*pbiData).Enqueue(0 /* 0000 */, 4);
			break;

		case CParameter::AD_DAB_SPEC_APP:
			(*pbiData).Enqueue(1 /* 0001 */, 4);
			break;
		}

		/* Packet length */
		(*pbiData).Enqueue( 
			(_UINT32BIT) Parameter.Service[ServiceID].DataParam.iPacketLen, 8);

		break;
	}

	/* Application data */
// Not used, yet!!!

	return iNoBitsTotal + iNoBitsHeader;
}


/******************************************************************************\
* Data entity Type 9														   *
\******************************************************************************/
int CSDCTransmit::DataEntityType9(CVector<_BINARY>* pbiData, int ServiceID, 
								  CParameter& Parameter)
{
	_UINT32BIT iRfuDummy = 0;
	int iNoBitsTotal;
	const int iNoBitsHeader = 12; /* Data entity header */

	/* Set total number of bits */
	iNoBitsTotal = 20;

	/**** Multiplex description data entity - type 9 ****/
	/* Length of the body, excluding the initial 4 bits ("- 4"), 
	   measured in bytes ("/ 8") */
	(*pbiData).Enqueue((_UINT32BIT) (iNoBitsTotal - 4) / 8, 7);

	/* Version flag (not used in this implementation) */
	(*pbiData).Enqueue((_UINT32BIT) 0, 1);

	/* Data entity type */
	(*pbiData).Enqueue((_UINT32BIT) 9, 4); /* Type 09 */

	/* ********** */
	/* Short Id */
	(*pbiData).Enqueue((_UINT32BIT) ServiceID, 2);

	/* Stream Id */
	(*pbiData).Enqueue((_UINT32BIT) Parameter.Service[ServiceID].AudioParam.
		iStreamID, 2);

	/* Audio coding */
	switch (Parameter.Service[ServiceID].AudioParam.eAudioCoding)
	{
	case CParameter::AC_AAC:
		(*pbiData).Enqueue(0 /* 00 */, 2);
		break;

	case CParameter::AC_CELP:
		(*pbiData).Enqueue(1 /* 01 */, 2);
		break;

	case CParameter::AC_HVXC:
		(*pbiData).Enqueue(2 /* 10 */, 2);
		break;
	}

	/* SBR flag */
	switch (Parameter.Service[ServiceID].AudioParam.eSBRFlag)
	{
	case CParameter::SB_NOT_USED:
		(*pbiData).Enqueue(0 /* 0 */, 1);
		break;

	case CParameter::SB_USED:
		(*pbiData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Audio mode */
	switch (Parameter.Service[ServiceID].AudioParam.eAudioCoding)
	{
	case CParameter::AC_AAC:
		/* Channel type */
		switch (Parameter.Service[ServiceID].AudioParam.eAudioMode)
		{
		case CParameter::AM_MONO:
			(*pbiData).Enqueue(0 /* 00 */, 2);
			break;

		case CParameter::AM_LC_STEREO:
			(*pbiData).Enqueue(1 /* 01 */, 2);
			break;

		case CParameter::AM_STEREO:
			(*pbiData).Enqueue(2 /* 10 */, 2);
			break;
		}
		break;

	case CParameter::AC_CELP:
		/* rfa */
		(*pbiData).Enqueue((_UINT32BIT) iRfuDummy, 1);

		/* CELP_CRC */
		switch (Parameter.Service[ServiceID].AudioParam.bCELPCRC)
		{
		case FALSE:
			(*pbiData).Enqueue(0 /* 0 */, 1);
			break;

		case TRUE:
			(*pbiData).Enqueue(1 /* 1 */, 1);
			break;
		}
		break;

	case CParameter::AC_HVXC:
		/* HVXC_rate */
		switch (Parameter.Service[ServiceID].AudioParam.eHVXCRate)
		{
		case CParameter::HR_2_KBIT:
			(*pbiData).Enqueue(0 /* 0 */, 1);
			break;

		case CParameter::HR_4_KBIT:
			(*pbiData).Enqueue(1 /* 1 */, 1);
			break;
		}

		/* HVXC CRC */
		switch (Parameter.Service[ServiceID].AudioParam.bHVXCCRC)
		{
		case FALSE:
			(*pbiData).Enqueue(0 /* 0 */, 1);
			break;

		case TRUE:
			(*pbiData).Enqueue(1 /* 1 */, 1);
			break;
		}
		break;
	}

	/* Audio sampling rate */
	switch (Parameter.Service[ServiceID].AudioParam.eAudioSamplRate)
	{
	case CParameter::AS_8_KHZ:
		(*pbiData).Enqueue(0 /* 000 */, 3);
		break;

	case CParameter::AS_12KHZ:
		(*pbiData).Enqueue(1 /* 001 */, 3);
		break;

	case CParameter::AS_16KHZ:
		(*pbiData).Enqueue(2 /* 010 */, 3);
		break;

	case CParameter::AS_24KHZ:
		(*pbiData).Enqueue(3 /* 011 */, 3);
		break;
	}

	/* Text flag */
	switch (Parameter.Service[ServiceID].AudioParam.bTextflag)
	{
	case FALSE:
		(*pbiData).Enqueue(0 /* 0 */, 1);
		break;

	case TRUE:
		(*pbiData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Enhancement flag */
	switch (Parameter.Service[ServiceID].AudioParam.bEnhanceFlag)
	{
	case FALSE:
		(*pbiData).Enqueue(0 /* 0 */, 1);
		break;

	case TRUE:
		(*pbiData).Enqueue(1 /* 1 */, 1);
		break;
	}

	/* Coder field */
	if (Parameter.Service[ServiceID].AudioParam.
		eAudioCoding == CParameter::AC_CELP)
	{
		/* CELP index */
		(*pbiData).Enqueue( 
			(_UINT32BIT) Parameter.Service[ServiceID].AudioParam.iCELPIndex, 5);
	}
	else
	{
		/* rfa 5 bit */
		(*pbiData).Enqueue((_UINT32BIT) iRfuDummy, 5);
	}
	
	/* rfa 1 bit */
	(*pbiData).Enqueue((_UINT32BIT) iRfuDummy, 1);

	return iNoBitsTotal + iNoBitsHeader;
}

