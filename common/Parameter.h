/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Andrea Russo
 *
 * Description:
 *	See Parameter.cpp
 *
 * 10/01/2007 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include additional RSCI related fields
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additions to include AMSS demodulation (Added class
 *    CAltFreqOtherServicesSign)
 *
 * 11/28/2005 Andrea Russo
 *	- Added classes for store alternative frequencies schedules and regions
 *
 *******************************************************************************
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
#include "GPSReceiver.h"


enum ETypeIntFreq
{ FLINEAR, FDFTFILTER, FWIENER };
enum ETypeIntTime
{ TLINEAR, TWIENER };
enum ETypeSNREst
{ SNR_FAC, SNR_PIL };
enum ETypeRxStatus
{ NOT_PRESENT, CRC_ERROR, DATA_ERROR, RX_OK };
	/* RM: Receiver mode (analog or digital demodulation) */

enum ERecMode
{ RM_DRM, RM_AM, RM_NONE };

	/* Acquisition state of receiver */
enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL};

	/* Receiver state */
enum ERecState {RS_TRACKING, RS_ACQUISITION};

/* Classes ********************************************************************/
class CParameter:public CCellMappingTable
{
  public:
	CParameter():
		sReceiverID("                "),
		Stream(MAX_NUM_STREAMS), iChanEstDelay(0),
		bRunThread(FALSE), bUsingMultimedia(TRUE),
		iCurSelAudioService(0), iCurSelDataService(0),
		rSigStrengthCorrection(_REAL(0.0)),
		vecbiAudioFrameStatus(),
		vecrPSD(0)
	{
		GenerateRandomSerialNumber();
	}
	virtual ~ CParameter()
	{
	}

	/* Enumerations --------------------------------------------------------- */
	/* CA: CA system */
	enum ECACond
	{ CA_USED, CA_NOT_USED };

	/* SF: Service Flag */
	enum ETyOServ
	{ SF_AUDIO, SF_DATA };

	/* AS: AFS in SDC is valid or not */
	enum EAFSVali
	{ AS_VALID, AS_NOT_VALID };

	/* PM: Packet Mode */
	enum EPackMod
	{ PM_SYNCHRON_STR_MODE, PM_PACKET_MODE };

	/* DU: Data Unit */
	enum EDatUnit
	{ DU_SINGLE_PACKETS, DU_DATA_UNITS };

	/* AD: Application Domain */
	enum EApplDomain
	{ AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP };

	/* AC: Audio Coding */
	enum EAudCod
	{ AC_AAC, AC_CELP, AC_HVXC };

	/* SB: SBR */
	enum ESBRFlag
	{ SB_NOT_USED, SB_USED };

	/* AM: Audio Mode */
	enum EAudMode
	{ AM_MONO, AM_P_STEREO, AM_STEREO };

	/* HR: HVXC Rate */
	enum EHVXCRate
	{ HR_2_KBIT, HR_4_KBIT };

	/* AS: Audio Sampling rate */
	enum EAudSamRat
	{ AS_8_KHZ, AS_12KHZ, AS_16KHZ, AS_24KHZ };

	/* CS: Coding Scheme */
	enum ECodScheme
	{ CS_1_SM, CS_2_SM, CS_3_SM, CS_3_HMSYM, CS_3_HMMIX };

	/* SI: Symbol Interleaver */
	enum ESymIntMod
	{ SI_LONG, SI_SHORT };

	/* CT: Channel Type */
	enum EChanType
	{ CT_MSC, CT_SDC, CT_FAC };

	/* ST: Simulation Type */
	enum ESimType
	{ ST_NONE, ST_BITERROR, ST_MSECHANEST, ST_BER_IDEALCHAN,
		ST_SYNC_PARAM, ST_SINR
	};

	/* Classes -------------------------------------------------------------- */
	class CAudioParam
	{
	  public:
		CAudioParam():strTextMessage("")
		{
		}

		/* Text-message */
		string strTextMessage;	/* Max length is (8 * 16 Bytes) */

		int iStreamID;			/* Stream Id of the stream which carries the audio service */

		EAudCod eAudioCoding;	/* This field indicated the source coding system */
		ESBRFlag eSBRFlag;		/* SBR flag */
		EAudSamRat eAudioSamplRate;	/* Audio sampling rate */
		_BOOLEAN bTextflag;		/* Indicates whether a text message is present or not */
		_BOOLEAN bEnhanceFlag;	/* Enhancement flag */

		/* For AAC: Mono, LC Stereo, Stereo --------------------------------- */
		EAudMode eAudioMode;	/* Audio mode */

		/* For CELP --------------------------------------------------------- */
		int iCELPIndex;			/* This field indicates the CELP bit rate index */
		_BOOLEAN bCELPCRC;		/* This field indicates whether the CRC is used or not */

		/* For HVXC --------------------------------------------------------- */
		EHVXCRate eHVXCRate;	/* This field indicates the rate of the HVXC */
		_BOOLEAN bHVXCCRC;		/* This field indicates whether the CRC is used or not */

/* TODO: Copy operator. Now, default copy operator is used! */

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CAudioParam AudioParam)
		{
			if (iStreamID != AudioParam.iStreamID)
				return TRUE;
			if (eAudioCoding != AudioParam.eAudioCoding)
				return TRUE;
			if (eSBRFlag != AudioParam.eSBRFlag)
				return TRUE;
			if (eAudioSamplRate != AudioParam.eAudioSamplRate)
				return TRUE;
			if (bTextflag != AudioParam.bTextflag)
				return TRUE;
			if (bEnhanceFlag != AudioParam.bEnhanceFlag)
				return TRUE;

			switch (AudioParam.eAudioCoding)
			{
			case AC_AAC:
				if (eAudioMode != AudioParam.eAudioMode)
					return TRUE;
				break;

			case AC_CELP:
				if (bCELPCRC != AudioParam.bCELPCRC)
					return TRUE;
				if (iCELPIndex != AudioParam.iCELPIndex)
					return TRUE;
				break;

			case AC_HVXC:
				if (eHVXCRate != AudioParam.eHVXCRate)
					return TRUE;
				if (bHVXCCRC != AudioParam.bHVXCCRC)
					return TRUE;
				break;
			}
			return FALSE;
		}
	};

	class CDataParam
	{
	  public:
		int iStreamID;			/* Stream Id of the stream which carries the data service */

		EPackMod ePacketModInd;	/* Packet mode indicator */

		/* In case of packet mode ------------------------------------------- */
		EDatUnit eDataUnitInd;	/* Data unit indicator */
		int iPacketID;			/* Packet Id (2 bits) */
		int iPacketLen;			/* Packet length */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain;	/* Application domain */
		int iUserAppIdent;		/* User application identifier, only DAB */

/* TODO: Copy operator. Now, default copy operator is used! */
		CDataParam& operator=(const CDataParam DataParam)
		{
			iStreamID = DataParam.iStreamID;
			eDataUnitInd = DataParam.eDataUnitInd;
			iPacketID = DataParam.iPacketID;
			iPacketLen = DataParam.iPacketLen;
			eAppDomain = DataParam.eAppDomain;
			iUserAppIdent = DataParam.iUserAppIdent;
			return *this;
		}

		/* This function is needed for detection changes in the class */
		_BOOLEAN operator!=(const CDataParam DataParam)
		{
			if (iStreamID != DataParam.iStreamID)
				return TRUE;
			if (ePacketModInd != DataParam.ePacketModInd)
				return TRUE;
			if (DataParam.ePacketModInd == PM_PACKET_MODE)
			{
				if (eDataUnitInd != DataParam.eDataUnitInd)
					return TRUE;
				if (iPacketID != DataParam.iPacketID)
					return TRUE;
				if (iPacketLen != DataParam.iPacketLen)
					return TRUE;
				if (eAppDomain != DataParam.eAppDomain)
					return TRUE;
				if (DataParam.eAppDomain == AD_DAB_SPEC_APP)
					if (iUserAppIdent != DataParam.iUserAppIdent)
						return TRUE;
			}
			return FALSE;
		}
	};

	class CService
	{
	  public:
		CService():strLabel("")
		{
		}

		_BOOLEAN IsActive()
		{
			return iServiceID != SERV_ID_NOT_USED;
		}

		uint32_t iServiceID;
		ECACond eCAIndication;
		int iLanguage;
		ETyOServ eAudDataFlag;
		int iServiceDescr;
		string strCountryCode;
		string strLanguageCode;

		/* Label of the service */
		string strLabel;

		/* Audio parameters */
		CAudioParam AudioParam;

		/* Data parameters */
		CDataParam DataParam;
	};

	class CStream
	{
	  public:
		CStream():iLenPartA(0), iLenPartB(0)
		{
		}

		int iLenPartA;			/* Data length for part A */
		int iLenPartB;			/* Data length for part B */

		_BOOLEAN operator!=(const CStream Stream)
		{
			if (iLenPartA != Stream.iLenPartA)
				return TRUE;
			if (iLenPartB != Stream.iLenPartB)
				return TRUE;
			return FALSE;
		}
	};

	class CMSCProtLev
	{
	  public:
		int iPartA;				/* MSC protection level for part A */
		int iPartB;				/* MSC protection level for part B */
		int iHierarch;			/* MSC protection level for hierachical frame */

		CMSCProtLev& operator=(const CMSCProtLev& NewMSCProtLev)
		{
			iPartA = NewMSCProtLev.iPartA;
			iPartB = NewMSCProtLev.iPartB;
			iHierarch = NewMSCProtLev.iHierarch;
			return *this;
		}
	};

	/* Alternative Frequency Signalling ************************************** */
	/* Alternative frequency signalling Schedules informations class */
	class CAltFreqSched
	{
	  public:
		CAltFreqSched()
		{
			Reset();
		}
		CAltFreqSched(const CAltFreqSched& nAFS):iScheduleID(nAFS.
															  iScheduleID),
			iDayCode(nAFS.iDayCode), iStartTime(nAFS.iStartTime),
			iDuration(nAFS.iDuration)
		{
		}

		CAltFreqSched& operator=(const CAltFreqSched& nAFS)
		{
			iScheduleID = nAFS.iScheduleID;
			iDayCode = nAFS.iDayCode;
			iStartTime = nAFS.iStartTime;
			iDuration = nAFS.iDuration;

			return *this;
		}

		_BOOLEAN operator==(const CAltFreqSched& nAFS)
		{
			if (iScheduleID != nAFS.iScheduleID)
				return FALSE;
			if (iDayCode != nAFS.iDayCode)
				return FALSE;
			if (iStartTime != nAFS.iStartTime)
				return FALSE;
			if (iDuration != nAFS.iDuration)
				return FALSE;

			return TRUE;
		}

		void Reset()
		{
			iScheduleID = 0;
			iDayCode = 0;
			iStartTime = 0;
			iDuration = 0;
		}

		int iScheduleID;
		int iDayCode;
		int iStartTime;
		int iDuration;
	};

	/* Alternative frequency signalling Regions informations class */
	class CAltFreqRegion
	{
	  public:
		CAltFreqRegion()
		{
			Reset();
		}
		CAltFreqRegion(const CAltFreqRegion& nAFR):iRegionID(nAFR.iRegionID),
			veciCIRAFZones(nAFR.veciCIRAFZones),
			iLatitude(nAFR.iLatitude),
			iLongitude(nAFR.iLongitude),
			iLatitudeEx(nAFR.iLatitudeEx), iLongitudeEx(nAFR.iLongitudeEx)
		{
		}

		CAltFreqRegion& operator=(const CAltFreqRegion& nAFR)
		{
			iRegionID = nAFR.iRegionID;

			iLatitude = nAFR.iLatitude;
			iLongitude = nAFR.iLongitude;
			iLatitudeEx = nAFR.iLatitudeEx;
			iLongitudeEx = nAFR.iLongitudeEx;

			veciCIRAFZones.Init(nAFR.veciCIRAFZones.Size());
			veciCIRAFZones = nAFR.veciCIRAFZones;

			return *this;
		}

		_BOOLEAN operator==(const CAltFreqRegion& nAFR)
		{
			if (iRegionID != nAFR.iRegionID)
				return FALSE;

			if (iLatitude != nAFR.iLatitude)
				return FALSE;
			if (iLongitude != nAFR.iLongitude)
				return FALSE;
			if (iLatitudeEx != nAFR.iLatitudeEx)
				return FALSE;
			if (iLongitudeEx != nAFR.iLongitudeEx)
				return FALSE;

			/* Vector sizes */
			if (veciCIRAFZones.Size() != nAFR.veciCIRAFZones.Size())
				return FALSE;

			/* Vector contents */
			for (int i = 0; i < veciCIRAFZones.Size(); i++)
				if (veciCIRAFZones[i] != nAFR.veciCIRAFZones[i])
					return FALSE;

			return TRUE;
		}

		void Reset()
		{
			iRegionID = 0;
			veciCIRAFZones.Init(0);
			iLatitude = 0;
			iLongitude = 0;
			iLatitudeEx = 0;
			iLongitudeEx = 0;
		}

		int iRegionID;
		CVector < int >veciCIRAFZones;
		int iLatitude;
		int iLongitude;
		int iLatitudeEx;
		int iLongitudeEx;
	};

	/* Alternative frequency signalling class */
	class CAltFreqSign
	{
	  public:
		CAltFreqSign()
		{
			Reset();
		}

		class CAltFreq
		{
		  public:
			CAltFreq()
			{
				Reset();
			}
			CAltFreq(const CAltFreq& nAF):veciFrequencies(nAF.
														   veciFrequencies),
				veciServRestrict(nAF.veciServRestrict),
				bIsSyncMultplx(nAF.bIsSyncMultplx),
				bRegionSchedFlag(nAF.bRegionSchedFlag),
				iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID)
			{
			}

			CAltFreq& operator=(const CAltFreq& nAF)
			{
				veciFrequencies.Init(nAF.veciFrequencies.Size());
				veciFrequencies = nAF.veciFrequencies;

				veciServRestrict.Init(nAF.veciServRestrict.Size());
				veciServRestrict = nAF.veciServRestrict;

				bIsSyncMultplx = nAF.bIsSyncMultplx;
				iRegionID = nAF.iRegionID;
				iScheduleID = nAF.iScheduleID;
				bRegionSchedFlag = nAF.bRegionSchedFlag;
				return *this;
			}

			_BOOLEAN operator==(const CAltFreq& nAF)
			{
				int i;

				/* Vector sizes */
				if (veciFrequencies.Size() != nAF.veciFrequencies.Size())
					return FALSE;
				if (veciServRestrict.Size() != nAF.veciServRestrict.Size())
					return FALSE;

				/* Vector contents */
				for (i = 0; i < veciFrequencies.Size(); i++)
					if (veciFrequencies[i] != nAF.veciFrequencies[i])
						return FALSE;
				for (i = 0; i < veciServRestrict.Size(); i++)
					if (veciServRestrict[i] != nAF.veciServRestrict[i])
						return FALSE;

				if (bIsSyncMultplx != nAF.bIsSyncMultplx)
					return FALSE;
				if (iRegionID != nAF.iRegionID)
					return FALSE;
				if (iScheduleID != nAF.iScheduleID)
					return FALSE;

				if (bRegionSchedFlag != nAF.bRegionSchedFlag)
					return FALSE;

				return TRUE;
			}

			void Reset()
			{
				veciFrequencies.Init(0);
				veciServRestrict.Init(MAX_NUM_SERVICES, 0);
				bIsSyncMultplx = FALSE;
				bRegionSchedFlag = FALSE;
				iRegionID = iScheduleID = 0;
			}

			CVector < int >veciFrequencies;
			CVector < int >veciServRestrict;
			_BOOLEAN bIsSyncMultplx;
			_BOOLEAN bRegionSchedFlag;
			int iRegionID;
			int iScheduleID;
		};

		void Reset()
		{
			vecAltFreqRegions.Init(0);
			vecAltFreqSchedules.Init(0);
			vecAltFreq.Init(0);
			bVersionFlag = FALSE;
		}

		CVector < CAltFreq > vecAltFreq;
		CVector < CAltFreqSched > vecAltFreqSchedules;
		CVector < CAltFreqRegion > vecAltFreqRegions;
		_BOOLEAN bVersionFlag;
	} AltFreqSign;

	/* Other Services alternative frequency signalling class */
	class CAltFreqOtherServicesSign
	{
	  public:
		CAltFreqOtherServicesSign()
		{
			Reset();
		}

		class CAltFreqOtherServices
		{
		  public:
			CAltFreqOtherServices()
			{
				Reset();
			}
			CAltFreqOtherServices(const CAltFreqOtherServices&
								  nAF):veciFrequencies(nAF.veciFrequencies),
				bShortIDAnnounceFlag(nAF.bShortIDAnnounceFlag),
				iShortIDAnnounce(nAF.iShortIDAnnounce),
				bRegionSchedFlag(nAF.bRegionSchedFlag),
				bSameService(nAF.bSameService), iSystemID(nAF.iSystemID),
				iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID),
				iOtherServiceID(nAF.iOtherServiceID)
			{
			}

			CAltFreqOtherServices& operator=(const CAltFreqOtherServices& nAF)
			{
				veciFrequencies.Init(nAF.veciFrequencies.Size());
				veciFrequencies = nAF.veciFrequencies;

				bShortIDAnnounceFlag = nAF.bShortIDAnnounceFlag;
				iShortIDAnnounce = nAF.iShortIDAnnounce;
				bRegionSchedFlag = nAF.bRegionSchedFlag;
				bSameService = nAF.bSameService;
				iSystemID = nAF.iSystemID;
				iRegionID = nAF.iRegionID;
				iScheduleID = nAF.iScheduleID;
				iOtherServiceID = nAF.iOtherServiceID;

				return *this;
			}

			_BOOLEAN operator==(const CAltFreqOtherServices& nAF)
			{
				int i;

				/* Vector sizes */
				if (veciFrequencies.Size() != nAF.veciFrequencies.Size())
					return FALSE;

				/* Vector contents */
				for (i = 0; i < veciFrequencies.Size(); i++)
					if (veciFrequencies[i] != nAF.veciFrequencies[i])
						return FALSE;

				if (bShortIDAnnounceFlag != nAF.bShortIDAnnounceFlag)
					return FALSE;
				if (iShortIDAnnounce != nAF.iShortIDAnnounce)
					return FALSE;
				if (bRegionSchedFlag != nAF.bRegionSchedFlag)
					return FALSE;
				if (bSameService != nAF.bSameService)
					return FALSE;

				if (iSystemID != nAF.iSystemID)
					return FALSE;
				if (iRegionID != nAF.iRegionID)
					return FALSE;
				if (iScheduleID != nAF.iScheduleID)
					return FALSE;
				if (iOtherServiceID != nAF.iOtherServiceID)
					return FALSE;
				return TRUE;
			}

			void Reset()
			{
				veciFrequencies.Init(0);
				bShortIDAnnounceFlag = FALSE;
				iShortIDAnnounce = 0;
				bRegionSchedFlag = FALSE;
				bSameService = TRUE;
				iSystemID = 0;
				iRegionID = iScheduleID = 0;
				iOtherServiceID = 0;
			}

			CVector < int >veciFrequencies;
			_BOOLEAN bShortIDAnnounceFlag;
			int iShortIDAnnounce;
			_BOOLEAN bRegionSchedFlag;
			_BOOLEAN bSameService;
			int iSystemID;
			int iRegionID;
			int iScheduleID;
			unsigned long iOtherServiceID;
		};

		void Reset()
		{
			vecAltFreqOtherServices.Init(0);
			bVersionFlag = FALSE;
		}

		CVector < CAltFreqOtherServices > vecAltFreqOtherServices;
		_BOOLEAN bVersionFlag;
	} AltFreqOtherServicesSign;

	/* Misc. Functions ------------------------------------------------------ */
	void FillGRPSData();
	void GenerateRandomSerialNumber();
	void ResetServicesStreams();
	void GetActiveServices(CVector < int >&veciActServ);
	void GetActiveStreams(CVector < int >&veciActStr);
	int GetNumActiveServices();
	void InitCellMapTable(const ERobMode eNewWaveMode,
						  const ESpecOcc eNewSpecOcc);

	void SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC);
	void SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC);
	void SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot);
	void SetNumAudioDecoderBits(const int iNewNumAudioDecoderBits);
	void SetNumDataDecoderBits(const int iNewNumDataDecoderBits);

	_BOOLEAN SetWaveMode(const ERobMode eNewWaveMode);
	ERobMode GetWaveMode() const
	{
		return eRobustnessMode;
	}

	void SetCurSelAudioService(const int iNewService);
	int GetCurSelAudioService() const
	{
		return iCurSelAudioService;
	}
	void SetCurSelDataService(const int iNewService);
	int GetCurSelDataService() const
	{
		return iCurSelDataService;
	}

	void ResetCurSelAudDatServ()
	{
		iCurSelAudioService = 0;
		iCurSelDataService = 0;
	}

	void EnableMultimedia(const _BOOLEAN bFlag);
	_BOOLEAN GetEnableMultimedia() const
	{
		return bUsingMultimedia;
	}

	_REAL GetDCFrequency() const
	{
		return SOUNDCRD_SAMPLE_RATE * (rFreqOffsetAcqui + rFreqOffsetTrack);
	}
	_REAL GetSampFreqEst() const
	{
		return rResampleOffset;
	}

	_REAL GetBitRateKbps(const int iServiceID, const _BOOLEAN bAudData);
	_REAL PartABLenRatio(const int iServiceID);

	/* Parameters controlled by FAC ----------------------------------------- */
	void SetInterleaverDepth(const ESymIntMod eNewDepth);
	ESymIntMod GetInterleaverDepth()
	{
		return eSymbolInterlMode;
	}

	void SetMSCCodingScheme(const ECodScheme eNewScheme);
	void SetSDCCodingScheme(const ECodScheme eNewScheme);

	void SetSpectrumOccup(ESpecOcc eNewSpecOcc);
	ESpecOcc GetSpectrumOccup() const
	{
		return eSpectOccup;
	}

	void SetNumOfServices(const int iNNumAuSe, const int iNNumDaSe);
	int GetTotNumServices()
	{
		return iNumAudioService + iNumDataService;
	}

	void SetAudDataFlag(const int iServID, const ETyOServ iNewADaFl);
	void SetServID(const int iServID, const uint32_t iNewServID);

	/* Symbol interleaver mode (long or short interleaving) */
	ESymIntMod eSymbolInterlMode;

	ECodScheme eMSCCodingScheme;	/* MSC coding scheme */
	ECodScheme eSDCCodingScheme;	/* SDC coding scheme */

	int iNumAudioService;
	int iNumDataService;

	/* AMSS */
	int iAMSSCarrierMode;

	/* Serial number and received ID */
	string sReceiverID;
	string sSerialNumber;


	/* Directory for data files */
	string sDataFilesDirectory;


	/* Parameters controlled by SDC ----------------------------------------- */
	void SetAudioParam(const int iShortID, const CAudioParam NewAudParam);
	CAudioParam GetAudioParam(const int iShortID)
	{
		return Service[iShortID].AudioParam;
	}
	void SetDataParam(const int iShortID, const CDataParam NewDataParam);
	CDataParam GetDataParam(const int iShortID)
	{
		return Service[iShortID].DataParam;
	}

	void SetMSCProtLev(const CMSCProtLev NewMSCPrLe,
					   const _BOOLEAN bWithHierarch);
	void SetStreamLen(const int iStreamID, const int iNewLenPartA,
					  const int iNewLenPartB);
	int GetStreamLen(const int iStreamID);

	/* Protection levels for MSC */
	CMSCProtLev MSCPrLe;

	CVector < CStream > Stream;
	CService Service[MAX_NUM_SERVICES];

	/* These values are used to set input and output block sizes of some
	   modules */
	int iNumBitsHierarchFrameTotal;
	int iNumDecodedBitsMSC;
	int iNumSDCBitsPerSFrame;	/* Number of SDC bits per super-frame */
	int iNumAudioDecoderBits;	/* Number of input bits for audio module */
	int iNumDataDecoderBits;	/* Number of input bits for data decoder module */

	/* Date */
	int iYear;
	int iMonth;
	int iDay;

	/* UTC (hours and minutes) */
	int iUTCHour;
	int iUTCMin;

	/* Identifies the current frame. This parameter is set by FAC */
	int iFrameIDTransm;
	int iFrameIDReceiv;

	/* Synchronization ------------------------------------------------------ */
	_REAL rFreqOffsetAcqui;
	_REAL rFreqOffsetTrack;

	_REAL rResampleOffset;

	int iTimingOffsTrack;

	/* Reception log -------------------------------------------------------- */
	class CReceptLog
	{
	  public:
		CReceptLog();
		virtual ~ CReceptLog()
		{
			CloseFile(pFileLong, TRUE);
			CloseFile(pFileShort, FALSE);
		}

		void StartLogging();
		void StopLogging();
		void SetFAC(const _BOOLEAN bCRCOk);
		void SetMSC(const _BOOLEAN bCRCOk);
		void SetSync(const _BOOLEAN bCRCOk);
		void SetSNR(const _REAL rNewCurSNR);
		void SetNumAAC(const int iNewNum);
		void SetLoggingEnabled(const _BOOLEAN bLog)
		{
			bLogEnabled = bLog;
		}
		_BOOLEAN GetLoggingEnabled()
		{
			return bLogEnabled;
		}
		_BOOLEAN GetLoggingActivated()
		{
			return bLogActivated;
		}
		void SetLogHeader(FILE * pFile, const _BOOLEAN bIsLong);
		void SetFrequency(const int iNewFreq)
		{
			iFrequency = iNewFreq;
		}
		int GetFrequency()
		{
			return iFrequency;
		}
		void SetLatitude(const string snewLat);
		void SetLongitude(const string sNewLong);
		string GetLatitudeDegreesMinutesString();
		string GetLongitudeDegreesMinutesString();
		string GetLatitudeDegreesString();
		string GetLongitudeDegreesString();
		_REAL GetLatitudeDegrees() { return rLatitudeDegrees; }
		_REAL GetLongitudeDegrees() { return rLongitudeDegrees; }
		_BOOLEAN GetLatitudeValid() { return bLatValid; }
		_BOOLEAN GetLongitudeValid() { return bLongValid; }

		void SetAdditText(const string strNewTxt)
		{
			strAdditText = strNewTxt;
		}
		void WriteParameters(const _BOOLEAN bIsLong);
		void SetDelLogStart(const int iSecDel)
		{
			iSecDelLogStart = iSecDel;
		}
		int GetDelLogStart()
		{
			return iSecDelLogStart;
		}

		void ResetTransParams();
		void SetMSCScheme(const ECodScheme eNewMCS)
		{
			eCurMSCScheme = eNewMCS;
		}
		void SetRobMode(const ERobMode eNewRM)
		{
			eCurRobMode = eNewRM;
		}
		void SetProtLev(const CMSCProtLev eNPL)
		{
			CurProtLev = eNPL;
		}

	  protected:
		void ResetLog(const _BOOLEAN bIsLong);
		void CloseFile(FILE * pFile, const _BOOLEAN bIsLong);
		int iNumSNR;
		int iNumCRCOkFAC, iNumCRCOkMSC;
		int iNumCRCOkMSCLong, iNumCRCMSCLong;
		int iNumAACFrames, iTimeCntShort;
		time_t TimeCntLong;
		_BOOLEAN bSyncOK, bFACOk, bMSCOk;
		_BOOLEAN bSyncOKValid, bFACOkValid, bMSCOkValid;
		int iFrequency;
		_REAL rAvSNR, rCurSNR;
		_REAL rMaxSNR, rMinSNR;
		_BOOLEAN bLogActivated;
		_BOOLEAN bLogEnabled;
		FILE *pFileLong;
		FILE *pFileShort;
		string strAdditText;
		
		_REAL rLatitudeDegrees, rLongitudeDegrees;
		_BOOLEAN bLatValid, bLongValid;
		int iSecDelLogStart;

		ERobMode eCurRobMode;
		ECodScheme eCurMSCScheme;
		CMSCProtLev CurProtLev;

		CMutex Mutex;
	} ReceptLog;

	/* Class for store informations about last service selected ------------- */

	class CLastService
	{
	  public:
		CLastService()
		{
			Reset();
		}

		void Reset()
		{
			iService = 0;
			iServiceID = SERV_ID_NOT_USED;
		};

		void Save(const int iCurSel, const int iCurServiceID)
		{
			if (iCurServiceID != SERV_ID_NOT_USED)
			{
				iService = iCurSel;
				iServiceID = iCurServiceID;
			}
		};

		/* store only fac parameters */
		int iService;
		uint32_t iServiceID;
	};

	/* Class for keeping track of status flags for RSCI rsta tag */
	class CReceiveStatus
	{
	  public:
		CReceiveStatus():FSyncOK(NOT_PRESENT), TSyncOK(NOT_PRESENT),
			FACOK(NOT_PRESENT), SDCOK(NOT_PRESENT), AudioOK(NOT_PRESENT)
		{
		}

		void SetFrameSyncStatus(const ETypeRxStatus OK);
		void SetTimeSyncStatus(const ETypeRxStatus OK);
		void SetInterfaceStatus(const ETypeRxStatus OK);
		void SetFACStatus(const ETypeRxStatus OK);
		void SetSDCStatus(const ETypeRxStatus OK);
		void SetAudioStatus(const ETypeRxStatus OK);
		void SetMOTStatus(const ETypeRxStatus OK);

		ETypeRxStatus GetFrameSyncStatus();
		ETypeRxStatus GetTimeSyncStatus();
		ETypeRxStatus GetInterfaceStatus();
		ETypeRxStatus GetFACStatus();
		ETypeRxStatus GetSDCStatus();
		ETypeRxStatus GetAudioStatus();
		ETypeRxStatus GetMOTStatus();
	  private:
		ETypeRxStatus FSyncOK;
		ETypeRxStatus TSyncOK;
		ETypeRxStatus InterfaceOK;
		ETypeRxStatus FACOK;
		ETypeRxStatus SDCOK;
		ETypeRxStatus AudioOK;
		ETypeRxStatus MOTOK;
	} ReceiveStatus;

	/* Simulation ----------------------------------------------------------- */
	ESimType eSimType;

	int iDRMChannelNum;
	int iSpecChDoppler;
	_REAL rBitErrRate;
	_REAL rSyncTestParam;		/* For any other simulations, used
								   with "ST_SYNC_PARAM" type */
	_REAL rSINR;
	int iNumBitErrors;
	int iChanEstDelay;

	int iNumTaps;
	int iPathDelay[MAX_NUM_TAPS_DRM_CHAN];
	_REAL rGainCorr;
	int iOffUsfExtr;

	void SetNominalSNRdB(const _REAL rSNRdBNominal);
	_REAL GetNominalSNRdB();
	void SetSystemSNRdB(const _REAL rSNRdBSystem)
	{
		rSysSimSNRdB = rSNRdBSystem;
	}
	_REAL GetSystemSNRdB() const
	{
		return rSysSimSNRdB;
	}
	_REAL GetSysSNRdBPilPos() const;

	/* Simulation raw-data management. We have to implement a shift register
	   with varying size. We do that by adding a variable for storing the
	   current write position. */
	class CRawSimData
	{
		/* We have to implement a shift register with varying size. We do that
		   by adding a variable for storing the current write position. We use
		   always the first value of the array for reading and do a shift of the
		   other data by adding a arbitrary value (0) at the end of the whole
		   shift register */
	  public:
		/* Here, the maximal size of the shift register is set */
		CRawSimData():ciMaxDelBlocks(50), iCurWritePos(0)
		{
			veciShRegSt.Init(ciMaxDelBlocks);
		}

		void Add(uint32_t iNewSRS);
		uint32_t Get();

		void Reset()
		{
			iCurWritePos = 0;
		}

	  protected:
		/* Max number of delayed blocks */
		int ciMaxDelBlocks;
		CShiftRegister < uint32_t > veciShRegSt;
		int iCurWritePos;
	} RawSimDa;

	/* General -------------------------------------------------------------- */
	_REAL GetNominalBandwidth();
	_REAL GetSysToNomBWCorrFact();
	_BOOLEAN bRunThread;
	_BOOLEAN bUsingMultimedia;

	//andrewm - GPS Receiver Info
	CGPSRxData GPSRxData;

	enum EGPSSource { GPS_SOURCE_GPS_RECEIVER, GPS_SOURCE_MANUAL_ENTRY };

	EGPSSource eGPSSource;
	string	sGPSdHost;
	int		iGPSdPort;

	//RGPS TAG item data
		//andrewm - GPS location
	class CRGPSData
	{
	  public:
		CRGPSData():
			eGPSSource(GPS_SOURCE_INVALID),
			bSatellitesVisibleAvailable(FALSE),
			bPositionAvailable(FALSE), bAltitudeAvailable(FALSE),
			bTimeAvailable(FALSE), bDateAvailable(FALSE),
			bSpeedAvailable(FALSE), bHeadingAvailable(FALSE)
		{
		}

		enum EGPSSource
		{ GPS_SOURCE_INVALID, GPS_SOURCE_GPS_RECEIVER,
			GPS_SOURCE_DIFFERENTIAL_GPS_RECEIVER, GPS_SOURCE_MANUAL_ENTRY,
			GPS_SOURCE_NOT_AVAILABLE
		};

		EGPSSource GetGPSSource() {	return eGPSSource; }
		void SetGPSSource(EGPSSource eNewSource) { eGPSSource = eNewSource; }

		void SetSatellitesVisibleAvailable(_BOOLEAN bNewSatVisAv) {	bSatellitesVisibleAvailable = bNewSatVisAv; }
		_BOOLEAN GetSatellitesVisibleAvailable() { return bSatellitesVisibleAvailable; }

		void SetSatellitesVisible(uint8_t uiNewSatVis) { uiSatellitesVisible = uiNewSatVis;	}
		uint8_t GetSatellitesVisible() { return uiSatellitesVisible; }

		void SetPositionAvailable(_BOOLEAN bNewPosAv) { bPositionAvailable = bNewPosAv; }
		_BOOLEAN GetPositionAvailable() { return bPositionAvailable; }

		void SetLatLongDegrees(_REAL rNewLatDeg, _REAL rNewLongDeg) { rLatitudeDegrees = rNewLatDeg; rLongitudeDegrees = rNewLongDeg; }
		_REAL GetLatitudeDegrees() { return rLatitudeDegrees; }
		_REAL GetLongitudeDegrees() { return rLongitudeDegrees; }

		void SetAltitudeAvailable(_BOOLEAN bNewAltAv) {	bAltitudeAvailable = bNewAltAv; }
		_BOOLEAN GetAltitudeAvailable() { return bAltitudeAvailable; }

		void SetAltitudeMetres(_REAL rNewAlt) { rAltitudeMetres = rNewAlt; }
		_REAL GetAltitudeMetres() { return rAltitudeMetres; }

		void SetTimeAvailable(_BOOLEAN bNewTimeAv) { bTimeAvailable = bNewTimeAv; }
		_BOOLEAN GetTimeAvailable() { return bTimeAvailable; }

		void SetTimeDate(const time_t ttSecondsSince1970);

		void SetTime(uint8_t uiNewHours, uint8_t uiNewMinutes, uint8_t uiNewSeconds) { uiTimeHours = uiNewHours; uiTimeMinutes = uiNewMinutes; uiTimeSeconds = uiNewSeconds; }
		
		uint8_t GetTimeHours() { return uiTimeHours; }
		uint8_t GetTimeMinutes() { return uiTimeMinutes; }
		uint8_t GetTimeSeconds() { return uiTimeSeconds; }

		void SetDateAvailable(_BOOLEAN bNewDateAv) { bDateAvailable = bNewDateAv; }
		_BOOLEAN GetDateAvailable() { return bDateAvailable; }

		void SetDate(uint8_t uiNewDay, uint8_t uiNewMonth, uint16_t uiNewYear) { uiDateDay = uiNewDay; uiDateMonth = uiNewMonth; uiDateYear = uiNewYear; }
		uint8_t GetDateDay() { return uiDateDay; }
		uint8_t GetDateMonth() { return uiDateMonth; }
		uint16_t GetDateYear() { return uiDateYear; }

		void SetSpeedAvailable(_BOOLEAN bNewSpeedAv) { bSpeedAvailable = bNewSpeedAv; }
		_BOOLEAN GetSpeedAvailable() { return bSpeedAvailable; }

		void SetSpeedMetresPerSecond(_REAL rNewSpeed) { rSpeedMetresPerSecond = rNewSpeed; }
		_REAL GetSpeedMetresPerSecond() { return rSpeedMetresPerSecond; }

		void SetHeadingAvailable(_BOOLEAN bNewHeadAv) { bHeadingAvailable = bNewHeadAv; }
		_BOOLEAN GetHeadingAvailable() { return bHeadingAvailable; }

		void SetHeadingDegreesFromNorth(uint8_t uiNewHeading) { uiHeadingDegreesFromNorth = uiNewHeading; }
		uint8_t GetHeadingDegreesFromNorth() { return uiHeadingDegreesFromNorth; }

	  private:
		_BOOLEAN bUse;

		EGPSSource eGPSSource;

		_BOOLEAN bSatellitesVisibleAvailable;
		uint8_t uiSatellitesVisible;

		_BOOLEAN bPositionAvailable;
		_REAL rLatitudeDegrees;	// +ve for North
		_REAL rLongitudeDegrees;	// +ve for East

		_BOOLEAN bAltitudeAvailable;
		_REAL rAltitudeMetres;

		_BOOLEAN bTimeAvailable;
		uint8_t uiTimeHours;
		uint8_t uiTimeMinutes;
		uint8_t uiTimeSeconds;

		_BOOLEAN bDateAvailable;
		uint8_t uiDateDay;
		uint8_t uiDateMonth;
		uint16_t uiDateYear;

		_BOOLEAN bSpeedAvailable;
		_REAL rSpeedMetresPerSecond;

		_BOOLEAN bHeadingAvailable;
		uint8_t uiHeadingDegreesFromNorth;

	}
	RGPSData;


	  protected:
	_REAL rSysSimSNRdB;

	/* Current selected audio service for processing */
	int iCurSelAudioService;
	int iCurSelDataService;

	ERobMode eRobustnessMode;	/* E.g.: Mode A, B, C or D */
	ESpecOcc eSpectOccup;

	/* For resync to last service------------------------------------------- */
	CLastService LastAudioService;
	CLastService LastDataService;

	CMutex Mutex;
  public:
	void Lock()
	{
		Mutex.Lock();
	}
	void Unlock()
	{
		Mutex.Unlock();
	}
	/* Channel Estimation */
	_REAL rSNREstimate;
	_REAL rMER;
	_REAL rWMERMSC;
	_REAL rWMERFAC;
	_REAL rSigmaEstimate;
	_REAL rMinDelay;

	_BOOLEAN bMeasureDelay;
	CRealVector vecrRdel;
	CRealVector vecrRdelThresholds;
	CRealVector vecrRdelIntervals;
	_BOOLEAN bMeasureDoppler;
	_REAL rRdop;
	/* interference (constellation-based measurement rnic)*/
	_BOOLEAN bMeasureInterference;
	_REAL rIntFreq, rINR, rICR;

	/* peak of PSD - for PSD-based interference measurement rnip */
	_REAL rMaxPSDwrtSig;
	_REAL rMaxPSDFreq;
	
	void SetSignalStrength(_BOOLEAN bValid, _REAL rNewSigStr);
	_BOOLEAN GetSignalStrength(_REAL& rSigStr);
	_BOOLEAN bValidSignalStrength;
	_REAL rSigStr;  

	_REAL rSigStrengthCorrection;

	class CFrontEndParameters
	{
	public:
		enum ESMeterCorrectionType {S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, S_METER_CORRECTION_TYPE_AGC_ONLY, S_METER_CORRECTION_TYPE_AGC_RSSI};

		// Constructor
		CFrontEndParameters(ESMeterCorrectionType eType=S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, 
			_REAL rMeterBW = _REAL(10000.0), 
			_REAL rDefBW = _REAL(10000.0), 
			_BOOLEAN bAutoBW = true, 
			_REAL rFactorAM = _REAL(0.0), 
			_REAL rFactorDRM = _REAL(0.0),
			_REAL rIFCentFreq = _REAL(12000.0))
			:
			eSMeterCorrectionType(eType), rSMeterBandwidth(rMeterBW), rDefaultMeasurementBandwidth(rDefBW),
				bAutoMeasurementBandwidth(bAutoBW), rCalFactorAM(rFactorAM), rCalFactorDRM(rFactorDRM),
				rIFCentreFreq(rIFCentFreq)
			{}

		ESMeterCorrectionType eSMeterCorrectionType;
		_REAL rSMeterBandwidth; // The bandwidth the S-meter uses to do the measurement

		_REAL rDefaultMeasurementBandwidth; // Bandwidth to do measurement if not synchronised
		_BOOLEAN bAutoMeasurementBandwidth; // TRUE: use the current FAC bandwidth if locked, FALSE: use default bandwidth always
		_REAL rCalFactorAM;
		_REAL rCalFactorDRM;
		_REAL rIFCentreFreq;
		
	} FrontEndParameters;


	ERecMode GetReceiverMode();
	EAcqStat GetReceiverState();
	CVector <_BINARY> vecbiAudioFrameStatus;
	_BOOLEAN bMeasurePSD;

	/* vector to hold the PSD valued for the rpsd tag. */
	CVector <_REAL> vecrPSD;

	CMatrix <_COMPLEX> matcReceivedPilotValues;
};

#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
