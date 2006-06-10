/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)  
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module derives, from the CTagItemGenerator base class, tag item generators 
 *  specialised to generate each of the tag items defined in MDI and RSCI.
 *  . 
 *  An intermediate derived class, CTagItemGeneratorWithProfiles, is used as the
 *  base class for all these tag item generators. This takes care of the common
 *	task of checking whether a given tag is in a particular profile.
 *  The profiles for each tag are defined by the GetProfiles() member function.
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

#include "MDITagItems.h"

CTagItemGeneratorWithProfiles::CTagItemGeneratorWithProfiles()
{
}

_BOOLEAN CTagItemGeneratorWithProfiles::IsInProfile(char cProfile)
{
	string strProfiles = GetProfiles();

	for (int i=0; i<strProfiles.length(); i++)
		if (strProfiles[i] == cProfile)
			return TRUE;

		return FALSE;
}

/* Default implementation: unless otherwise specified, tag will be in all RSCI profiles, but not MDI */
/* Make this pure virtual and remove the implementation if you want to force all tags to specify the profiles explicitly */
string CTagItemGeneratorWithProfiles::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorProTyMDI::GenTag()
{	
	/* Length: 8 bytes = 64 bits */
	PrepareTag(64);

	/* Protocol type: DMDI */
	Enqueue((uint32_t) 'D', SIZEOF__BYTE);
	Enqueue((uint32_t) 'M', SIZEOF__BYTE);
	Enqueue((uint32_t) 'D', SIZEOF__BYTE);
	Enqueue((uint32_t) 'I', SIZEOF__BYTE);

	/* Major revision */
	Enqueue((uint32_t) MDI_MAJOR_REVISION, 16);

	/* Minor revision */
	Enqueue((uint32_t) MDI_MINOR_REVISION, 16);
}

string CTagItemGeneratorProTyMDI::GetTagName(void) {return "*ptr";}
string CTagItemGeneratorProTyMDI::GetProfiles(void) {return "M";}

void CTagItemGeneratorProTyRSCI::GenTag()
{	
	/* Length: 8 bytes = 64 bits */
	PrepareTag(64);

	/* Protocol type: DMDI */
	Enqueue((uint32_t) 'R', SIZEOF__BYTE);
	Enqueue((uint32_t) 'S', SIZEOF__BYTE);
	Enqueue((uint32_t) 'C', SIZEOF__BYTE);
	Enqueue((uint32_t) 'I', SIZEOF__BYTE);

	/* Major revision */
	Enqueue((uint32_t) RSCI_MAJOR_REVISION, 16);

	/* Minor revision */
	Enqueue((uint32_t) RSCI_MINOR_REVISION, 16);
}

string CTagItemGeneratorProTyRSCI::GetTagName(void) {return "*ptr";}
string CTagItemGeneratorProTyRSCI::GetProfiles(void) {return "ABCDQ";}

CTagItemGeneratorLoFrCnt::CTagItemGeneratorLoFrCnt(void) 
: iLogFraCnt(0)
{}

void CTagItemGeneratorLoFrCnt::GenTag()
{
	/* Length: 4 bytes = 32 bits */
	PrepareTag(32);

	/* Logical frame count */
	Enqueue(iLogFraCnt, 32);

	/* Count: the value shall be incremented by one by the device generating the
	   MDI Packets for each MDI Packet sent. Wraps around at a value of
	   "(1 << 32)" since the variable type is "uint32_t" */
	iLogFraCnt++;
}

string CTagItemGeneratorLoFrCnt::GetTagName(void) {return "dlfc";}
string CTagItemGeneratorLoFrCnt::GetProfiles(void) {return "ABCDQM";}

void CTagItemGeneratorFAC::GenTag(_BOOLEAN bFACOK, CVectorEx<_BINARY>& vecbiFACData)
{
	if (bFACOK == FALSE)
	{
		/* Empty tag if FAC is invalid */
		PrepareTag(0);
	}
	else
	{

		/* Length: 9 bytes = 72 bits */
		PrepareTag(NUM_FAC_BITS_PER_BLOCK);

		/* Channel parameters, service parameters, CRC */
		vecbiFACData.ResetBitAccess();

		/* FAC data is always 72 bits long which is 9 bytes, copy data byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE; i++)
			Enqueue(vecbiFACData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
	}
}

string CTagItemGeneratorFAC::GetTagName(void) {return "fac_";}
string CTagItemGeneratorFAC::GetProfiles(void) {return "ACDQM";}

void CTagItemGeneratorSDC::GenTag(_BOOLEAN bSDCOK, CVectorEx<_BINARY>& vecbiSDCData)
{
	if (bSDCOK == FALSE)
	{
		PrepareTag(0);
	}
	else
	{
	/* Fixed by O.Haffenden, BBC R&D */
		/* The input SDC vector is 4 bits SDC index + a whole number of bytes plus padding. */
		/* The padding is not sent in the MDI */
		const int iLenSDCDataBits = SIZEOF__BYTE * ((vecbiSDCData.Size() - 4) / SIZEOF__BYTE) + 4;

		/* Length: "length SDC block" bytes. Our SDC data vector does not
		   contain the 4 bits "Rfu" */
		PrepareTag(iLenSDCDataBits + 4);

		/* Service Description Channel Block */
		vecbiSDCData.ResetBitAccess();

		Enqueue((uint32_t) 0, 4); /* Rfu */

		/* We have to copy bits instead of bytes since the length of SDC data is
		   usually not a multiple of 8 */
		for (int i = 0; i < iLenSDCDataBits; i++)
			Enqueue(vecbiSDCData.Separate(1), 1);
	}
}

string CTagItemGeneratorSDC::GetTagName(void) {return "sdc_";}
string CTagItemGeneratorSDC::GetProfiles(void) {return "ACDM";}

void CTagItemGeneratorSDCChanInf::GenTag(CParameter& Parameter)
{
	CVector<int> veciActStreams;

	/* Get active streams */
	Parameter.GetActiveStreams(veciActStreams);

	/* Get number of active streams */
	const int iNumActStreams = veciActStreams.Size();

	/* Length: 1 + n * 3 bytes */
	PrepareTag((1 + 3 * iNumActStreams) * SIZEOF__BYTE);

	/* Protection */
	/* Rfu */
	Enqueue((uint32_t) 0, 4);

	/* PLA */
	Enqueue((uint32_t) Parameter.MSCPrLe.iPartA, 2);

	/* PLB */
	Enqueue((uint32_t) Parameter.MSCPrLe.iPartB, 2);

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
			Enqueue((uint32_t) Parameter.MSCPrLe.iHierarch, 2);

			/* rfu */
			Enqueue((uint32_t) 0, 10);

			/* Data length for hierarchical (always stream 0) */
			Enqueue((uint32_t) Parameter.Stream[0].iLenPartB, 12);
		}
		else
		{
			/* Data length for part A */
			Enqueue((uint32_t) Parameter.Stream[veciActStreams[i]].iLenPartA, 12);

			/* Data length for part B */
			Enqueue((uint32_t) Parameter.Stream[veciActStreams[i]].iLenPartB, 12);
		}
	}
}


string CTagItemGeneratorSDCChanInf::GetTagName(void) {return "sdci";}
string CTagItemGeneratorSDCChanInf::GetProfiles(void) {return "ACDQM";}

void CTagItemGeneratorRobMod::GenTag(const ERobMode eCurRobMode)
{
	/* Length: 1 byte */
	PrepareTag(SIZEOF__BYTE);

	/* Robustness mode */
	switch (eCurRobMode)
	{
	case RM_ROBUSTNESS_MODE_A:
		Enqueue((uint32_t) 0, 8);
		break;

	case RM_ROBUSTNESS_MODE_B:
		Enqueue((uint32_t) 1, 8);
		break;

	case RM_ROBUSTNESS_MODE_C:
		Enqueue((uint32_t) 2, 8);
		break;

	case RM_ROBUSTNESS_MODE_D:
		Enqueue((uint32_t) 3, 8);
		break;
	}
}

string CTagItemGeneratorRobMod::GetTagName(void) {return "robm";}
string CTagItemGeneratorRobMod::GetProfiles(void) {return "ABCDQM";}

void CTagItemGeneratorInfo::GenTag(string strUTF8Text)
{
	/* Data length: n * 8 bits */
	PrepareTag(strUTF8Text.size() * SIZEOF__BYTE);

	/* UTF-8 text */
	for (int i = 0; i < strUTF8Text.size(); i++)
	{
		const char cNewChar = strUTF8Text[i];

		/* Set character */
		Enqueue((uint32_t) cNewChar, SIZEOF__BYTE);
	}
}

string CTagItemGeneratorInfo::GetTagName(void) {return "info";}
string CTagItemGeneratorInfo::GetProfiles(void) {return "ABCDQM";}

CTagItemGeneratorStr::CTagItemGeneratorStr()
: iStreamNumber(0)
{}

// Sets the stream number. Should be called just after construction. (can't have it in the constructor because
// we want a vector of them

void CTagItemGeneratorStr::SetStreamNumber(const int iStrNum)
{
	iStreamNumber = iStrNum;
}

void CTagItemGeneratorStr::GenTag(CVectorEx<_BINARY>& vecbiStrData)
{

	const int iLenStrData = vecbiStrData.Size();

	/* Only generate this tag if stream input data is not of zero length */
	if ((iLenStrData != 0) && (iStreamNumber < MAX_NUM_STREAMS))
	{
		PrepareTag(iLenStrData);

		/* Data */
		vecbiStrData.ResetBitAccess();

		/* Data is always a multiple of 8 -> copy bytes */
		for (int i = 0; i < iLenStrData / SIZEOF__BYTE; i++)
		{
			Enqueue(vecbiStrData.Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
	}
}

string CTagItemGeneratorStr::GetProfiles(void) {return "ADM";}

string CTagItemGeneratorStr::GetTagName(void) 
{
	switch (iStreamNumber)
	{
	case 0: return "str0";
	case 1: return "str1";
	case 2: return "str2";
	case 3: return "str3";
	default: return "str?";
	}
}

void CTagItemGeneratorMERFormat::GenTag(const _BOOLEAN bIsValid, const _REAL rMER)
{
	/* Common routine for rmer, rwmf, rwmm tags (all have the same format) */
	/* If no MER value is available, set tag length to zero */
	if (bIsValid == FALSE)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 2 bytes = 16 bits */
		PrepareTag(16);

		/* Set value: the format of this single value is (Byte1 + Byte2 / 256)
		   = (Byte1.Byte2) in [dB] with: Byte1 is an 8-bit signed integer value;
		   and Byte2 is an 8-bit unsigned integer value */
		/* Integer part */
		Enqueue((uint32_t) rMER, SIZEOF__BYTE);

		/* Fractional part */
		const _REAL rFracPart = rMER - (int) rMER;
		Enqueue((uint32_t) (rFracPart * 256), SIZEOF__BYTE);
	}
}


string CTagItemGeneratorRWMF::GetTagName(void) {return "rwmf";}
string CTagItemGeneratorRWMF::GetProfiles(void) {return "ABCDQ";}

string CTagItemGeneratorRWMM::GetTagName(void) {return "rwmm";}
string CTagItemGeneratorRWMM::GetProfiles(void) {return "ABCDQ";}

string CTagItemGeneratorRMER::GetTagName(void) {return "rmer";}
string CTagItemGeneratorRMER::GetProfiles(void) {return "ABCDQ";}

string CTagItemGeneratorRDOP::GetTagName(void) {return "rdop";}
string CTagItemGeneratorRDOP::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRDEL::GenTag(const _BOOLEAN bIsValid, const CRealVector &vecrThresholds, const CRealVector &vecrIntervals)
{
	/* If no value is available, set tag length to zero */
	if (bIsValid == FALSE)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 3 bytes per value = 16 bits */
		PrepareTag(24*vecrThresholds.GetSize());

		for (int i=0; i<vecrThresholds.GetSize(); i++)
		{
			/* percentage for this window */
			Enqueue((uint32_t) vecrThresholds[i], SIZEOF__BYTE);
			/* Set value: the format of this single value is (Byte1 + Byte2 / 256)
			   = (Byte1.Byte2) in [dB] with: Byte1 is an 8-bit signed integer value;
			   and Byte2 is an 8-bit unsigned integer value */
			/* Integer part */
			_REAL rDelay = vecrIntervals[i];
			Enqueue((uint32_t) rDelay, SIZEOF__BYTE);

			/* Fractional part */
			const _REAL rFracPart = rDelay - (int) rDelay;
			Enqueue((uint32_t) (rFracPart * 256), SIZEOF__BYTE);
		}
	}
}

string CTagItemGeneratorRDEL::GetTagName(void) {return "rdel";}
string CTagItemGeneratorRDEL::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRAFS::GenTag(const _BOOLEAN bIsValid, CVector<_BINARY>& vecbiAudioStatus)
{
	if (bIsValid == FALSE)
	{
		/* zero length tag item */
		PrepareTag(0);
	}
	else
	{
		/* Header - length is always 48 */
		const int iNumUnits = vecbiAudioStatus.Size();
		PrepareTag(48);

		/* data */
		vecbiAudioStatus.ResetBitAccess();

		/* number of units: 8 bits */
		Enqueue(iNumUnits, 8);

		/* status for each unit */
		for (int i=0; i<iNumUnits; i++)
		{
			Enqueue(vecbiAudioStatus.Separate(1),1);
		}
		/* pad the rest with zeros */
		Enqueue(0, 40-iNumUnits);
	}
}

string CTagItemGeneratorRAFS::GetTagName(void) {return "rafs";}
string CTagItemGeneratorRAFS::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRINT::GenTag(const _BOOLEAN bIsValid, const CReal rIntFreq, const CReal rINR, const CReal rICR)
{

	/* Use Bint (BBC proprietary tag name) until tag is accepted by SE group */
	/* If no value is available, set tag length to zero */
	if (bIsValid == FALSE)
	{
		/* Length: 0 byte */
		PrepareTag(0);
	}
	else
	{
		/* Length: 2 bytes per value, 3 values = 48 bits */
		PrepareTag(48);

		/* Interference frequency (Hz) : signed value */
		Enqueue((uint32_t) ((int)rIntFreq), 16);

		/* Interference-to-noise ratio */
		/* integer part */
		Enqueue((uint32_t) rINR, SIZEOF__BYTE);
		/* Fractional part */
		Enqueue((uint32_t) ((rINR - (int) rINR) * 256), SIZEOF__BYTE);

		/* Interference-to-carrier ratio */
		/* integer part */
		Enqueue((uint32_t) rICR, SIZEOF__BYTE);
		/* Fractional part */
		Enqueue((uint32_t) ((rICR - (int) rICR) * 256), SIZEOF__BYTE);

	}

}

string CTagItemGeneratorRINT::GetTagName(void) {return "Bint";}
string CTagItemGeneratorRINT::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorSignalStrength::GenTag(const _BOOLEAN bIsValid, const _REAL rSigStrength)
{
	if (bIsValid == FALSE)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(16);
		Enqueue((uint32_t)((int) (rSigStrength*256)), 16);
	}
}

string CTagItemGeneratorSignalStrength::GetTagName(void) {return "rdbv";}
string CTagItemGeneratorSignalStrength::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorReceiverStatus::GenTag(const CRSCIStatusFlags &Flags)
{
	PrepareTag(4*SIZEOF__BYTE);
	Enqueue(Flags.GetSyncStatus()==TRUE ? 0 : 1, SIZEOF__BYTE); /* 0=ok, 1=bad */
	Enqueue(Flags.GetFACStatus()==TRUE ? 0 : 1, SIZEOF__BYTE); /* 0=ok, 1=bad */
	Enqueue(Flags.GetSDCStatus()==TRUE ? 0 : 1, SIZEOF__BYTE); /* 0=ok, 1=bad */
	Enqueue(Flags.GetAudioStatus()==TRUE ? 0 : 1, SIZEOF__BYTE); /* 0=ok, 1=bad */
}

string CTagItemGeneratorReceiverStatus::GetTagName(void) {return "rsta";}
string CTagItemGeneratorReceiverStatus::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorProfile::GenTag(const char cProfile)
{
	PrepareTag(8);
	Enqueue((uint32_t)cProfile, SIZEOF__BYTE);
}

string CTagItemGeneratorProfile::GetTagName(void) {return "rpro";}
string CTagItemGeneratorProfile::GetProfiles(void) {return "ABCDQ";}

//void CTagItemGeneratorRxDemodMode::GenTag(const CDRMReceiver::ERecMode eMode) /* ERecMode defined in DRMReceiver.h but can't include it! */
/*void CMDI::GenTagRxDemodMode(ERecMode eMode) // rdmo
{
	PrepareTag(4*SIZEOF__BYTE);
	switch (eMode)
	{
	case RM_DRM:
		Enqueue((uint32_t) 'd', SIZEOF__BYTE);
		Enqueue((uint32_t) 'r', SIZEOF__BYTE);
		Enqueue((uint32_t) 'm', SIZEOF__BYTE);
		Enqueue((uint32_t) '_', SIZEOF__BYTE);
		break;
	case RM_AM:
		Enqueue((uint32_t) 'a', SIZEOF__BYTE);
		Enqueue((uint32_t) 'm', SIZEOF__BYTE);
		Enqueue((uint32_t) '_', SIZEOF__BYTE);
		Enqueue((uint32_t) '_', SIZEOF__BYTE);
		break;
	default:
		Enqueue((uint32_t) ' ', SIZEOF__BYTE);
		Enqueue((uint32_t) ' ', SIZEOF__BYTE);
		Enqueue((uint32_t) ' ', SIZEOF__BYTE);
		Enqueue((uint32_t) ' ', SIZEOF__BYTE);
		break;
	}
	
}*/

string CTagItemGeneratorRxDemodMode::GetTagName(void) {return "rdmo";}
string CTagItemGeneratorRxDemodMode::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRxFrequency::GenTag(_BOOLEAN bIsValid, const int iFrequency) // Frequency in kHz
{
	if (bIsValid == FALSE)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(4*SIZEOF__BYTE);
		Enqueue((uint32_t) iFrequency*1000, 4*SIZEOF__BYTE);
	}
}

string CTagItemGeneratorRxFrequency::GetTagName(void) {return "rfre";}
string CTagItemGeneratorRxFrequency::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRxActivated::GenTag(_BOOLEAN bActivated)
{
	PrepareTag(SIZEOF__BYTE);
	Enqueue(bActivated==TRUE ? '0' : '1', SIZEOF__BYTE);
}

string CTagItemGeneratorRxActivated::GetTagName(void) {return "ract";}
string CTagItemGeneratorRxActivated::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRxBandwidth::GenTag(_BOOLEAN bIsValid, _REAL rBandwidth)
{
	if (bIsValid==FALSE)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(2*SIZEOF__BYTE);
		Enqueue((uint32_t)((int)(rBandwidth*256.0)), 2*SIZEOF__BYTE); 

	}
}

string CTagItemGeneratorRxBandwidth::GetTagName(void) {return "rbw_";}
string CTagItemGeneratorRxBandwidth::GetProfiles(void) {return "ABCDQ";}

void CTagItemGeneratorRxService::GenTag(_BOOLEAN bIsValid, int iService)
{
	if (bIsValid==FALSE)
	{
		PrepareTag(0);
	}
	else
	{
		PrepareTag(SIZEOF__BYTE);
		Enqueue((uint32_t)iService, SIZEOF__BYTE); 

	}
}

string CTagItemGeneratorRxService::GetTagName(void) {return "rser";}
string CTagItemGeneratorRxService::GetProfiles(void) {return "ABCDQ";}

CTagItemGeneratorRBP::CTagItemGeneratorRBP()
{}

void CTagItemGeneratorRBP::SetStreamNumber(const int iStrNum)
{
	iStreamNumber = iStrNum;
}

void CTagItemGeneratorRBP::GenTag() // Not yet implemented
{
}

string CTagItemGeneratorRBP::GetTagName(void) 
{
	switch (iStreamNumber)
	{
	case 0: return "rbp0";
	case 1: return "rbp1";
	case 2: return "rbp2";
	case 3: return "rbp3";
	default: return "rbp?"; // error!
	}
}

string CTagItemGeneratorRBP::GetProfiles(void) {return "ABCD";}



void CTagItemGenerator::PutTagItemData(CVector<_BINARY> &vecbiDestination) // Call this to write the binary data (header + payload) to the vector
{
	vecbiTagData.ResetBitAccess();
	for (int i=0; i < vecbiTagData.Size(); i++)
		vecbiDestination.Enqueue(vecbiTagData.Separate(1), 1);
}


void CTagItemGenerator::Reset(void) // Resets bit vector to zero length (i.e. no header)
{
	vecbiTagData.Init(0);
}

void CTagItemGenerator::GenEmptyTag(void) // Generates valid tag item with zero payload length
{
	PrepareTag(0);
}

// Prepare vector and make the header
void CTagItemGenerator::PrepareTag(const int iLenDataBits)
{
	string strTagName = GetTagName();

	/* Init vector length. 4 bytes for tag name and 4 bytes for data length
	   plus the length of the actual data */
	vecbiTagData.Init(8 * SIZEOF__BYTE + iLenDataBits);
	vecbiTagData.ResetBitAccess();

	/* Set tag name (always four bytes long) */
	for (int i = 0; i < 4; i++)
		vecbiTagData.Enqueue((uint32_t) strTagName[i], SIZEOF__BYTE);

	/* Set tag data length */
	vecbiTagData.Enqueue((uint32_t) iLenDataBits, 32);
}

// Put the bits to the bit vector (avoids derived classes needing to access the bit vector directly
void CTagItemGenerator::Enqueue(uint32_t iInformation, const int iNumOfBits)
{
	vecbiTagData.Enqueue(iInformation, iNumOfBits);
}

/* TODO: there are still some RSCI tags left to implement */
/* e.g. rpil, rpsd, ... */
