/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Parameter.cpp
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

#if !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "GlobalDefinitions.h"
#include "ofdmcellmapping/CellMappingTable.h"
#include "matlib/Matlib.h"


/* Classes ********************************************************************/
class CParameter : public CCellMappingTable
{
public:
	CParameter() : bRunThread(FALSE), Stream(MAX_NO_STREAMS), 
		FACRepitition(15) /* See 6.3.6 */ {}
	virtual ~CParameter() {}

	/* Enumerations --------------------------------------------------------- */
	/* BE: Basement, enhancement */
	enum ETransLay {BE_BASE_LAYER, BE_ENHM_LAYER};
	
	/* CA: CA system */
	enum ECACond {CA_USED, CA_NOT_USED};

	/* SF: Service Flag */
	enum ETyOServ {SF_AUDIO, SF_DATA};

	/* AS: AFS in SDC is valid or not */
	enum EAFSVali {AS_VALID, AS_NOT_VALID};

	/* PM: Packet Mode */
	enum EPackMod {PM_SYNCHRON_STR_MODE, PM_PACKET_MODE};

	/* DU: Data Unit */
	enum EDatUnit {DU_SINGLE_PACKETS, DU_DATA_UNITS};

	/* AD: Application Domain */
	enum EApplDomain {AD_DRM_SPEC_APP, AD_DAB_SPEC_APP};

	/* AC: Audio Coding */
	enum EAudCod {AC_AAC, AC_CELP, AC_HVXC};

	/* SB: SBR */
	enum ESBRFlag {SB_NOT_USED, SB_USED};

	/* AM: Audio Mode */
	enum EAudMode {AM_MONO, AM_LC_STEREO, AM_STEREO};

	/* HR: HVXC Rate */
	enum EHVXCRate {HR_2_KBIT, HR_4_KBIT};

	/* AS: Audio Sampling rate */
	enum EAudSamRat {AS_8_KHZ, AS_12KHZ, AS_16KHZ, AS_24KHZ};

	/* CS: Coding Scheme */
	enum ECodScheme {CS_1_SM, CS_2_SM, CS_3_SM, CS_3_HMSYM, CS_3_HMMIX};

	/* SI: Symbol Interleaver */
	enum ESymIntMod {SI_LONG, SI_SHORT};

	/* CT: Channel Type */
	enum EChanType {CT_MSC, CT_SDC, CT_FAC};	


	/* Classes -------------------------------------------------------------- */
	class CAudioParam
	{
	public:
		CAudioParam() : strTextMessage("") {}

		/* Text-message */
		string		strTextMessage; /* Max length is (8 * 16 Bytes) */	

		int			iStreamID; /* Stream Id of the stream which carries the audio service */

		EAudCod		eAudioCoding; /* This field indicated the source coding system */
		ESBRFlag	eSBRFlag; /* SBR flag */
		EAudSamRat	eAudioSamplRate; /* Audio sampling rate */
		_BOOLEAN	bTextflag; /* Indicates whether a text message is present or not */
		_BOOLEAN	bEnhanceFlag; /* Enhancement flag */

// For AAC: Mono, LC Stereo, Stereo
		EAudMode	eAudioMode; /* Audio mode */

// For CELP
		int			iCELPIndex; /* This field indicates the CELP bit rate index */
		_BOOLEAN	bCELPCRC; /* This field indicates whether the CRC is used or not*/

// For HVXC
		EHVXCRate	eHVXCRate; /* This field indicates the rate of the HVXC */
		_BOOLEAN	bHVXCCRC; /* This field indicates whether the CRC is used or not */

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CAudioParam AudioParam)
		{
			if (iStreamID != AudioParam.iStreamID) return TRUE;
			if (eAudioCoding != AudioParam.eAudioCoding) return TRUE;
			if (eSBRFlag != AudioParam.eSBRFlag) return TRUE;
			if (eAudioSamplRate != AudioParam.eAudioSamplRate) return TRUE;
			if (bTextflag != AudioParam.bTextflag) return TRUE;
			if (bEnhanceFlag != AudioParam.bEnhanceFlag) return TRUE;

			switch (AudioParam.eAudioCoding)
			{
			case AC_AAC:
				if (eAudioMode != AudioParam.eAudioMode) return TRUE;
				break;

			case AC_CELP:
				if (bCELPCRC != AudioParam.bCELPCRC) return TRUE;
				if (iCELPIndex != AudioParam.iCELPIndex) return TRUE;
				break;

			case AC_HVXC:
				if (eHVXCRate != AudioParam.eHVXCRate) return TRUE;
				if (bHVXCCRC != AudioParam.bHVXCCRC) return TRUE;
				break;
			}
			return FALSE;
		}
	};

	class CDataParam
	{
	public:
		int			iStreamID; /* Stream Id of the stream which carries the data service */

		EPackMod	ePacketModInd; /* Packet mode indicator */

// In case of packet mode:
		EDatUnit	eDataUnitInd; /* Data unit indicator */
		int			iPacketID; /* Packet Id */
		int			iPacketLen; /* Packet length */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain; /* Application domain */ 

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CDataParam DataParam)
		{
			if (iStreamID != DataParam.iStreamID) return TRUE;
			if (ePacketModInd != DataParam.ePacketModInd) return TRUE;
			if (DataParam.ePacketModInd == PM_PACKET_MODE)
			{
				if (eDataUnitInd != DataParam.eDataUnitInd) return TRUE;
				if (iPacketID != DataParam.iPacketID) return TRUE;
				if (eAppDomain != DataParam.eAppDomain) return TRUE;
				if (iPacketLen != DataParam.iPacketLen) return TRUE;
			}
			return FALSE;
		}
	};

	class CService
	{
	public:
		CService() : strLabel("") {}

		_UINT32BIT	iServiceID;
		ECACond		eCAIndication;
		int			iLanguage;
		ETyOServ	eAudDataFlag;
		int			iServiceDescr;

		/* Label of the service */
		string		strLabel;

		/* Audio parameters */
		CAudioParam	AudioParam;

		/* Data parameters */
		CDataParam	DataParam;
	};

	class CStream
	{
	public:
		CStream() : iLenPartA(0), iLenPartB(0) {}

		int	iLenPartA; /* Data length for part A */
		int	iLenPartB; /* Data length for part B */

		_BOOLEAN operator!=(const CStream Stream)
		{
			if (iLenPartA != Stream.iLenPartA) return TRUE;
			if (iLenPartB != Stream.iLenPartB) return TRUE;
			return FALSE;
		}
	};

	class CMSCProtLev
	{
	public:
		int	iPartA; /* MSC protection level for part A */
		int	iPartB; /* MSC protection level for part B */
		int	iHierarch; /* MSC protection level for hierachical frame */

		CMSCProtLev& operator=(const CMSCProtLev& NewMSCProtLev)
		{
			iPartA = NewMSCProtLev.iPartA;
			iPartB = NewMSCProtLev.iPartB;
			iHierarch = NewMSCProtLev.iHierarch;
			return *this; 
		}

		_BOOLEAN operator!=(const CMSCProtLev MSCProtLev)
		{
			if (iPartA != MSCProtLev.iPartA) return TRUE;
			if (iPartB != MSCProtLev.iPartB) return TRUE;
			if (iHierarch != MSCProtLev.iHierarch) return TRUE;
			return FALSE;
		}
	};

	void			ResetServicesStreams();
	void			GetActiveStreams(CVector<int>& veciActStr);
	void			InitCellMapTable(const ERobMode eNewWaveMode, const ESpecOcc eNewSpecOcc);

	void			SetNoDecodedBitsMSC(const int iNewNoDecodedBitsMSC);
	void			SetNoDecodedBitsSDC(const int iNewNoDecodedBitsSDC);

	_BOOLEAN		SetWaveMode(const ERobMode eNewWaveMode);
	ERobMode		GetWaveMode() const {return eRobustnessMode;}

	void			SetCurSelectedService(const int iNewService);
	int				GetCurSelectedService() const {return iCurSelService;}

	_REAL			GetFrequencyOffset() const {return SOUNDCRD_SAMPLE_RATE *
						(rFreqOffsetAcqui + rFreqOffsetTrack);}
	_REAL			GetSampFreqEst() const {return rResampleOffset;}





	/* Parameters controlled by FAC ----------------------------------------- */
	void			SetInterleaverDepth(const ESymIntMod eNewDepth);

	void			SetMSCCodingScheme(const ECodScheme eNewScheme);
	void			SetSDCCodingScheme(const ECodScheme eNewScheme);

	void			SetSpectrumOccup(ESpecOcc eNewSpecOcc);
	ESpecOcc		GetSpectrumOccup() const {return eSpectOccup;}

	void			SetNoAudioServ(const int iNewNoAuSe);
	void			SetNoDataServ(const int iNewNoDaSe);

	void			SetAudDataFlag(const int iServID, const ETyOServ iNewADaFl);
	void			SetServID(const int iServID, const _UINT32BIT iNewServID);

	/* These two parameters are only for transmitter */
	CVector<int>	FACRepitition; /* See 6.3.6 */
	int				FACNoRep;

	ETransLay		eBaseEnhFlag;

	/* Symbol interleaver mode (long or short interleaving) */
	ESymIntMod		eSymbolInterlMode; 

	ECodScheme		eMSCCodingScheme; /* MSC coding scheme */
	ECodScheme		eSDCCodingScheme; /* SDC coding scheme */


	int				iNoAudioService;
	int				iNoDataService;
	int				iReConfigIndex;

	EAFSVali		eAFSFlag;


	/* Parameters controlled by SDC ----------------------------------------- */
	void SetAudioParam(const int iShortID, const CAudioParam NewAudParam);
	void SetDataParam(const int iShortID, const CDataParam NewDataParam);

	void SetMSCProtLev(const CMSCProtLev NewMSCPrLe, const _BOOLEAN bWithHierarch);

	void SetStreamLenPartA(const int iStreamNo, const int iNewLenPartA);
	void SetStreamLenPartB(const int iStreamNo, const int iNewLenPartB);

	/* Protection levels for MSC */
	CMSCProtLev			MSCPrLe;

	CVector<CStream>	Stream;
	CService			Service[MAX_NO_SERVICES];

	int					iNoBitsHierarchFrameTotal;

	int					iNoDecodedBitsMSC;
	int					iNoSDCBitsPerSFrame; /* Number of SDC bits per super-frame */

	int					iAFSIndex;

	/* Date */
	int					iYear;
	int					iMonth;
	int					iDay;

	/* UTC (hours and minutes) */
	int					iUTCHour;
	int					iUTCMin;
	


	/* Identifies the current frame. This parameter is set by FAC */
	int					iFrameIDTransm;
	int					iFrameIDReceiv;


// Synchronization **********************
	_REAL				rFreqOffsetAcqui;
	_REAL				rFreqOffsetTrack;

	_REAL				rResampleOffset;

	_REAL				rTimingOffsTrack;


// Simulation **********************
	int					iDRMChannelNo;
	_REAL				rSimSNRdB;
	_REAL				rBitErrRate;
	int					iNoBitErrors;

	_BOOLEAN			bRunThread;

protected:
	/* Current selected audio service for processing */
	int					iCurSelService;

	ERobMode			eRobustnessMode; /* E.g.: Mode A, B, C or D */
	ESpecOcc			eSpectOccup;
};


#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
