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
_BOOLEAN CSDCReceive::SDCParam(CVector<_BINARY>* pbiData, CParameter& Parameter)
{
	_UINT32BIT	CRC;
	int			iLengthOfBody;
	int			iLengthDataFieldBytes;
	int			iUsefulBitsSDC;
	_BYTE		byFirstByte;

	/* Calculate length of data field in bytes
	   (consistant to table 61 in (6.4.1)) */
	iLengthDataFieldBytes = 
		(int) ((_REAL) (Parameter.iNumSDCBitsPerSFrame - 20) / 8);

	/* 20 bits from AFS index and CRC */
	iUsefulBitsSDC = 20 + iLengthDataFieldBytes * 8;

	/* CRC ------------------------------------------------------------------ */
	/* Check the CRC of this data block */
	CRCObject.Reset(16);

	(*pbiData).ResetBitAccess();

	/* Special treatment of SDC data stream: The CRC (Cyclic Redundancy
	Check) field shall contain a 16-bit CRC calculated over the AFS
	index coded in an 8-bit field (4 msbs are 0) and the data field.
	4 MSBs from AFS-index. Insert four "0" in the data-stream */
	byFirstByte = (_BYTE) (*pbiData).Separate(4);
	CRCObject.AddByte(byFirstByte);

	/* "- 4": Four bits already used, "/ SIZEOF__BYTE": We add bytes, not bits,
	   "- 2": 16 bits for CRC at the end */
	for (int i = 0; i < (iUsefulBitsSDC - 4) / SIZEOF__BYTE - 2; i++)
		CRCObject.AddByte((_BYTE) (*pbiData).Separate(SIZEOF__BYTE));
	CRC = CRCObject.GetCRC();

	if (CRC == (_UINT32BIT) (*pbiData).Separate(16))
	{
		/* CRC-check successful, extract data from SDC-stream */
		/* Reset separation function */
		(*pbiData).ResetBitAccess();

		/* SDC Header ------------------------------------------------------- */
		/* AFS index */
		/* Reconfiguration index (not used by this application) */
		(*pbiData).Separate(4);

		/* Length of the body, excluding the initial 4 bits ("- 4"),
		   measured in bytes ("/ 8").
		   With this condition also the error code of the "Separate" function
		   is checked! (implicitly) */
		while ((iLengthOfBody = (*pbiData).Separate(7)) != 0)
		{
			/* Version flag (not used in this implementation) */
			(*pbiData).Separate(1);

			/* Data entity type */
			/* Call the routine for the signalled type */
			switch ((*pbiData).Separate(4))
			{
			case 0:
				/* Type 0 */
				DataEntityType0(pbiData, iLengthOfBody, Parameter);
				break;

			case 1:
				/* Type 1 */
				DataEntityType1(pbiData, iLengthOfBody, Parameter);
				break;

			case 5:
				/* Type 5 */
				DataEntityType5(pbiData, iLengthOfBody, Parameter);
				break;

			case 8:
				/* Type 8 */
				DataEntityType8(pbiData, iLengthOfBody, Parameter);
				break;

			case 9:
				/* Type 9 */
				DataEntityType9(pbiData, iLengthOfBody, Parameter);
				break;

			default:
				/* This type is not supported, delete all bits of this entity from
				   the queue */
				(*pbiData).Separate(iLengthOfBody * 8 + 4);
			}
		}

		return TRUE;
	}
	else
	{
		/* Data is corrupted, do not use it. Return failure as FALSE */
		return FALSE;
	}
}


/******************************************************************************\
* Data entity Type 0 (Multiplex description data entity)					   *
\******************************************************************************/
void CSDCReceive::DataEntityType0(CVector<_BINARY>* pbiData, int iLengthOfBody,
								  CParameter& Parameter)
{
	int						iNumStreams;
	int						i;
	CParameter::CMSCProtLev	MSCPrLe;
	_BOOLEAN				bWithHierarch;
	int						iLenPartA;
	int						iLenPartB;

	/* The receiver may determine the number of streams present in the multiplex
	   by dividing the length field of the header by three (6.4.3.1) */
	iNumStreams = iLengthOfBody / 3;

	/* Check number of streams for overflow
	   TODO: Error handling at this point! */
	if (iNumStreams > MAX_NUM_STREAMS)
		iNumStreams = MAX_NUM_STREAMS;

	/* Get protection levels */
	/* Protection level for part A */
	MSCPrLe.iPartA = (*pbiData).Separate(2);

	/* Protection level for part B */
	MSCPrLe.iPartB = (*pbiData).Separate(2);

	/* Reset hierarchical flag (hierarchical present or not) */
	bWithHierarch = FALSE;

	/* Get stream parameters */
	for (i = 0; i < iNumStreams; i++)
	{
		/* In case of hirachical modulation stream 0 describes the protection
		   level and length of hirarchical data */
		if ((i == 0) && ((Parameter.eMSCCodingScheme == CParameter::CS_3_HMSYM) ||
			(Parameter.eMSCCodingScheme == CParameter::CS_3_HMMIX)))
		{
			/* Protection level for hierarchical */
			MSCPrLe.iHierarch = (*pbiData).Separate(2);
			bWithHierarch = TRUE;

			/* rfu */
			(*pbiData).Separate(10);

			/* Data length for hierarchical */
			iLenPartB = (*pbiData).Separate(12);

			/* Set new parameters in global struct. Lenght of part A is zero
			   with hierarchical modulation */
			Parameter.SetStreamLen(i, 0, iLenPartB);
		}
		else
		{
			/* Data length for part A */
			iLenPartA = (*pbiData).Separate(12);

			/* Data length for part B */
			iLenPartB = (*pbiData).Separate(12);

			/* Set new parameters in global struct */
			Parameter.SetStreamLen(i, iLenPartA, iLenPartB);
		}
	}

	/* Set new parameters in global struct */
	Parameter.SetMSCProtLev(MSCPrLe, bWithHierarch);
}


/******************************************************************************\
* Data entity Type 1 (Label data entity)									   *
\******************************************************************************/
void CSDCReceive::DataEntityType1(CVector<_BINARY>* pbiData, int iLengthOfBody,
								  CParameter& Parameter)
{
	int		i;
	int		iTempShortID;
	char	cNewChar;

	/* Short ID (the short ID is the index of the service-array) */
	iTempShortID = (*pbiData).Separate(2);

	/* rfu */
	(*pbiData).Separate(2);


	/* Get label string ----------------------------------------------------- */
	/* Reset label string */
	Parameter.Service[iTempShortID].strLabel = "";

	/* Check the following restriction to the length of label: label: this is a
	   variable length field of up to 16 bytes defining the label using UTF-8
	   coding
	   TODO: Error handling at this point! */
// Some DRM test files do not follow the 16 byte restriction in the DRM standard!!!
//	if (iLengthOfBody <= 16)
	{
		/* Get all characters from SDC-stream */
		for (i = 0; i < iLengthOfBody; i++)
		{
			/* Get character */
			cNewChar = (*pbiData).Separate(8);

			/* Append new character */
			Parameter.Service[iTempShortID].strLabel.append(&cNewChar, 1);
		}
	}
}


/******************************************************************************\
* Data entity Type 5 (Application information data entity)					   *
\******************************************************************************/
void CSDCReceive::DataEntityType5(CVector<_BINARY>* pbiData, int iLengthOfBody,
								  CParameter& Parameter)
{
	int						iTempShortID;
	CParameter::CDataParam	DataParam;

	/* Short ID (the short ID is the index of the service-array) */
	iTempShortID = (*pbiData).Separate(2);

	/* Load data parameters class with current parameters */
	DataParam = Parameter.GetDataParam(iTempShortID);

	/* Stream Id */
	DataParam.iStreamID = (*pbiData).Separate(2);

	/* Packet mode indicator */
	switch ((*pbiData).Separate(1))
	{
	case 0: /* 0 */
		DataParam.ePacketModInd = CParameter::PM_SYNCHRON_STR_MODE;

		/* Descriptor (Dummy bits, not used) */
		(*pbiData).Separate(7);
		break;

	case 1: /* 1 */
		DataParam.ePacketModInd = CParameter::PM_PACKET_MODE;

		/* Descriptor */
		/* Data unit indicator */
		switch ((*pbiData).Separate(1))
		{
		case 0: /* 0 */
			DataParam.eDataUnitInd = CParameter::DU_SINGLE_PACKETS;
			break;

		case 1: /* 1 */
			DataParam.eDataUnitInd = CParameter::DU_DATA_UNITS;
			break;
		}

		/* Packet Id */
		DataParam.iPacketID = (*pbiData).Separate(2);

		/* Application domain */
		switch ((*pbiData).Separate(4))
		{
		case 0: /* 0000 */
			DataParam.eAppDomain = CParameter::AD_DRM_SPEC_APP;
			break;

		case 1: /* 0001 */
			DataParam.eAppDomain = CParameter::AD_DAB_SPEC_APP;
			break;

		default: /* 2 - 15 reserved */
			DataParam.eAppDomain = CParameter::AD_OTHER_SPEC_APP;
			break;
		}

		/* Packet length */
		DataParam.iPacketLen = (*pbiData).Separate(8);
		break;
	}

	/* Application data */
	if (DataParam.ePacketModInd == CParameter::PM_SYNCHRON_STR_MODE)
	{
		/* Not used */
		(*pbiData).Separate(iLengthOfBody * 8 - 8);
	}
	else if (DataParam.eAppDomain == CParameter::AD_DAB_SPEC_APP)
	{
		/* rfu */
		(*pbiData).Separate(5);

		/* User application identifier */
		DataParam.iUserAppIdent = (*pbiData).Separate(11);

		/* Data fields as required by DAB application specification, not used */
		(*pbiData).Separate(iLengthOfBody * 8 - 32);
	}
	else
	{
		/* Not used */
		(*pbiData).Separate(iLengthOfBody * 8 - 16);
	}

	/* Set new parameters in global struct */
	Parameter.SetDataParam(iTempShortID, DataParam);
}


/******************************************************************************\
* Data entity Type 8 (Time and date information data entity)				   *
\******************************************************************************/
void CSDCReceive::DataEntityType8(CVector<_BINARY>* pbiData, int iLengthOfBody,
								  CParameter& Parameter)
{
	_UINT32BIT	iModJulDate, iZ, iA, iAlpha, iB, iC, iD, iE;
	_REAL		rJulDate, rF;

	/* Get bits from stream */
	iModJulDate = (*pbiData).Separate(17);


	/* Modified Julian Date ------------------------------------------------- */
	/* Definition of the Modified Julian Date */
	rJulDate = (_REAL) iModJulDate + 2400000.5;

	/* Get "real" date out of Julian Date
	   (Taken from "http://mathforum.org/library/drmath/view/51907.html") */
	// 1. Add .5 to the JD and let Z = integer part of (JD+.5) and F the
	// fractional part F = (JD+.5)-Z
	iZ = (_UINT32BIT) (rJulDate + (_REAL) 0.5);
	rF = (rJulDate + (_REAL) 0.5) - iZ;

	// 2. If Z < 2299161, take A = Z
	// If Z >= 2299161, calculate alpha = INT((Z-1867216.25)/36524.25)
	// and A = Z + 1 + alpha - INT(alpha/4).
	if (iZ < 2299161)
		iA = iZ;
	else
	{
		iAlpha = (int) (((_REAL) iZ - (_REAL) 1867216.25) / (_REAL) 36524.25);
		iA = iZ + 1 + iAlpha - (int) ((_REAL) iAlpha / (_REAL) 4.0);
	}

	// 3. Then calculate:
	// B = A + 1524
	// C = INT( (B-122.1)/365.25)
	// D = INT( 365.25*C )
	// E = INT( (B-D)/30.6001 )
	iB = iA + 1524;
	iC = (int) (((_REAL) iB - (_REAL) 122.1) / (_REAL) 365.25);
	iD = (int) ((_REAL) 365.25 * iC);
	iE = (int) (((_REAL) iB - iD) / (_REAL) 30.6001);

	// The day of the month dd (with decimals) is:
	// dd = B - D - INT(30.6001*E) + F
	Parameter.iDay = iB - iD - (int) ((_REAL) 30.6001 * iE);// + rF;

	// The month number mm is:
	// mm = E - 1, if E < 13.5
	// or
	// mm = E - 13, if E > 13.5
	if ((_REAL) iE < 13.5)
		Parameter.iMonth = iE - 1;
	else
		Parameter.iMonth = iE - 13;

	// The year yyyy is:
	// yyyy = C - 4716   if m > 2.5
	// or
	// yyyy = C - 4715   if m < 2.5
	if ((_REAL) Parameter.iMonth > 2.5)
		Parameter.iYear = iC - 4716;
	else
		Parameter.iYear = iC - 4715;


	/* UTC (hours and minutes) ---------------------------------------------- */
	Parameter.iUTCHour = (*pbiData).Separate(5);
	Parameter.iUTCMin = (*pbiData).Separate(6);
}


/******************************************************************************\
* Data entity Type 9 (Audio information data entity)						   *
\******************************************************************************/
void CSDCReceive::DataEntityType9(CVector<_BINARY>* pbiData, int iLengthOfBody,
								  CParameter& Parameter)
{
	int						iTempShortID;
	CParameter::CAudioParam	AudParam;
	_BOOLEAN				bError;

	/* Init error flag with "no error" */
	bError = FALSE;

	/* Short ID (the short ID is the index of the service-array) */
	iTempShortID = (*pbiData).Separate(2);

	/* Load audio parameters class with current parameters */
	AudParam = Parameter.GetAudioParam(iTempShortID);

	/* Stream Id */
	AudParam.iStreamID = (*pbiData).Separate(2);

	/* Audio coding */
	switch ((*pbiData).Separate(2))
	{
	case 0: /* 00 */
		AudParam.eAudioCoding = CParameter::AC_AAC;
		break;

	case 1: /* 01 */
		AudParam.eAudioCoding = CParameter::AC_CELP;
		break;

	case 2: /* 10 */
		AudParam.eAudioCoding = CParameter::AC_HVXC;
		break;

	default: /* reserved */
		bError = TRUE;
		break;
	}

	/* SBR flag */
	switch ((*pbiData).Separate(1))
	{
	case 0: /* 0 */
		AudParam.eSBRFlag = CParameter::SB_NOT_USED;
		break;

	case 1: /* 1 */
		AudParam.eSBRFlag = CParameter::SB_USED;
		break;
	}

	/* Audio mode */
	switch (AudParam.eAudioCoding)
	{
	case CParameter::AC_AAC:
		/* Channel type */
		switch ((*pbiData).Separate(2))
		{
		case 0: /* 00 */
			AudParam.eAudioMode = CParameter::AM_MONO;
			break;

		case 1: /* 01 */
			AudParam.eAudioMode = CParameter::AM_LC_STEREO;
			break;

		case 2: /* 10 */
			AudParam.eAudioMode = CParameter::AM_STEREO;
			break;

		default: /* reserved */
			bError = TRUE;
			break;
		}
		break;

	case CParameter::AC_CELP:
		/* rfa */
		(*pbiData).Separate(1);

		/* CELP_CRC */
		switch ((*pbiData).Separate(1))
		{
		case 0: /* 0 */
			AudParam.bCELPCRC = FALSE;
			break;

		case 1: /* 1 */
			AudParam.bCELPCRC = TRUE;
			break;
		}
		break;

	case CParameter::AC_HVXC:
		/* HVXC_rate */
		switch ((*pbiData).Separate(1))
		{
		case 0: /* 0 */
			AudParam.eHVXCRate = CParameter::HR_2_KBIT;
			break;

		case 1: /* 1 */
			AudParam.eHVXCRate = CParameter::HR_4_KBIT;
			break;
		}

		/* HVXC CRC */
		switch ((*pbiData).Separate(1))
		{
		case 0: /* 0 */
			AudParam.bHVXCCRC = FALSE;
			break;

		case 1: /* 1 */
			AudParam.bHVXCCRC = TRUE;
			break;
		}
		break;
	}

	/* Audio sampling rate */
	switch ((*pbiData).Separate(3))
	{
	case 0: /* 000 */
		AudParam.eAudioSamplRate = CParameter::AS_8_KHZ;
		break;

	case 1: /* 001 */
		AudParam.eAudioSamplRate = CParameter::AS_12KHZ;
		break;

	case 2: /* 010 */
		AudParam.eAudioSamplRate = CParameter::AS_16KHZ;
		break;

	case 3: /* 011 */
		AudParam.eAudioSamplRate = CParameter::AS_24KHZ;
		break;

	default: /* reserved */
		bError = TRUE;
		break;
	}

	/* Text flag */
	switch ((*pbiData).Separate(1))
	{
	case 0: /* 0 */
		AudParam.bTextflag = FALSE;
		break;

	case 1: /* 1 */
		AudParam.bTextflag = TRUE;
		break;
	}

	/* Enhancement flag */
	switch ((*pbiData).Separate(1))
	{
	case 0: /* 0 */
		AudParam.bEnhanceFlag = FALSE;
		break;

	case 1: /* 1 */
		AudParam.bEnhanceFlag = TRUE;
		break;
	}

	/* Coder field */
	if (AudParam.eAudioCoding == CParameter::AC_CELP)
	{
		/* CELP index */
		AudParam.iCELPIndex = (*pbiData).Separate(5);
	}
	else
	{
		/* rfa 5 bit */
		(*pbiData).Separate(5);
	}

	/* rfa 1 bit */
	(*pbiData).Separate(1);

	/* Set new parameters in global struct */
	if (bError == FALSE)
		Parameter.SetAudioParam(iTempShortID, AudParam);
}
