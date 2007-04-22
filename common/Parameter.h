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
#include <time.h>
#ifdef USE_QT_GUI
# include "GPSReceiver.h"
#else
# include "GPSData.h"
#endif

class CDRMReceiver;

	/* CS: Coding Scheme */
	enum ECodScheme { CS_1_SM, CS_2_SM, CS_3_SM, CS_3_HMSYM, CS_3_HMMIX };

	/* CT: Channel Type */
	enum EChanType { CT_MSC, CT_SDC, CT_FAC };

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

	class CAudioParam
	{
	  public:

		/* AC: Audio Coding */
		enum EAudCod { AC_AAC, AC_CELP, AC_HVXC };

		/* SB: SBR */
		enum ESBRFlag { SB_NOT_USED, SB_USED };

		/* AM: Audio Mode */
		enum EAudMode { AM_MONO, AM_P_STEREO, AM_STEREO };

		/* HR: HVXC Rate */
		enum EHVXCRate { HR_2_KBIT, HR_4_KBIT };

		/* AS: Audio Sampling rate */
		enum EAudSamRat { AS_8_KHZ, AS_12KHZ, AS_16KHZ, AS_24KHZ };

		CAudioParam(): strTextMessage(), iStreamID(STREAM_ID_NOT_USED),
			eAudioCoding(AC_AAC), eSBRFlag(SB_NOT_USED), eAudioSamplRate(AS_24KHZ),
			bTextflag(FALSE), bEnhanceFlag(FALSE), eAudioMode(AM_MONO),
			iCELPIndex(0), bCELPCRC(FALSE), eHVXCRate(HR_2_KBIT), bHVXCCRC(FALSE)
		{
		}
		CAudioParam(const CAudioParam& ap):
			strTextMessage(ap.strTextMessage),
			iStreamID(ap.iStreamID),
			eAudioCoding(ap.eAudioCoding),
			eSBRFlag(ap.eSBRFlag),
			eAudioSamplRate(ap.eAudioSamplRate),
			bTextflag(ap.bTextflag),	
			bEnhanceFlag(ap.bEnhanceFlag),
			eAudioMode(ap.eAudioMode),	
			iCELPIndex(ap.iCELPIndex),
			bCELPCRC(ap.bCELPCRC),
			eHVXCRate(ap.eHVXCRate),
			bHVXCCRC(ap.bHVXCCRC)
		{
		}
		CAudioParam& operator=(const CAudioParam& ap)
		{
			strTextMessage = ap.strTextMessage;
			iStreamID = ap.iStreamID;
			eAudioCoding = ap.eAudioCoding;
			eSBRFlag = ap.eSBRFlag;
			eAudioSamplRate = ap.eAudioSamplRate;
			bTextflag =	ap.bTextflag;	
			bEnhanceFlag = ap.bEnhanceFlag;
			eAudioMode = ap.eAudioMode;	
			iCELPIndex = ap.iCELPIndex;
			bCELPCRC = ap.bCELPCRC;
			eHVXCRate = ap.eHVXCRate;
			bHVXCCRC = ap.bHVXCCRC;
			return *this;
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

		/* PM: Packet Mode */
		enum EPackMod { PM_SYNCHRON_STR_MODE, PM_PACKET_MODE };

		/* DU: Data Unit */
		enum EDatUnit { DU_SINGLE_PACKETS, DU_DATA_UNITS };

		/* AD: Application Domain */
		enum EApplDomain { AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP };

		int iStreamID;			/* Stream Id of the stream which carries the data service */

		EPackMod ePacketModInd;	/* Packet mode indicator */

		/* In case of packet mode ------------------------------------------- */
		EDatUnit eDataUnitInd;	/* Data unit indicator */
		int iPacketID;			/* Packet Id (2 bits) */
		int iPacketLen;			/* Packet length */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain;	/* Application domain */
		int iUserAppIdent;		/* User application identifier, only DAB */

		CDataParam():
			iStreamID(STREAM_ID_NOT_USED),
			ePacketModInd(PM_PACKET_MODE),
			eDataUnitInd(DU_DATA_UNITS),
			iPacketID(0),
			iPacketLen(0),
			eAppDomain(AD_DAB_SPEC_APP),
			iUserAppIdent(0)
		{
		}
		CDataParam(const CDataParam& DataParam):
			iStreamID(DataParam.iStreamID),
			ePacketModInd(DataParam.ePacketModInd),
			eDataUnitInd(DataParam.eDataUnitInd),
			iPacketID(DataParam.iPacketID),
			iPacketLen(DataParam.iPacketLen),
			eAppDomain(DataParam.eAppDomain),
			iUserAppIdent(DataParam.iUserAppIdent)
		{
		}
		CDataParam& operator=(const CDataParam& DataParam)
		{
			iStreamID = DataParam.iStreamID;
			ePacketModInd = DataParam.ePacketModInd;
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

		/* CA: CA system */
		enum ECACond { CA_USED, CA_NOT_USED };

		/* SF: Service Flag */
		enum ETyOServ { SF_AUDIO, SF_DATA };

		CService():
			iServiceID(SERV_ID_NOT_USED), eCAIndication(CA_NOT_USED),
			iLanguage(0), eAudDataFlag(SF_AUDIO), iServiceDescr(0),
			strCountryCode(), strLanguageCode(), strLabel(),
			AudioParam(), DataParam()
		{
		}
		CService(const CService& s):
			iServiceID(s.iServiceID), eCAIndication(s.eCAIndication),
			iLanguage(s.iLanguage), eAudDataFlag(s.eAudDataFlag),
			iServiceDescr(s.iServiceDescr), strCountryCode(s.strCountryCode),
			strLanguageCode(s.strLanguageCode), strLabel(s.strLabel),
			AudioParam(s.AudioParam), DataParam(s.DataParam)
		{
		}
		CService& operator=(const CService& s)
		{
			iServiceID = s.iServiceID;
			eCAIndication = s.eCAIndication;
			iLanguage = s.iLanguage;
			eAudDataFlag = s.eAudDataFlag;
			iServiceDescr = s.iServiceDescr;
			strCountryCode = s.strCountryCode;
			strLanguageCode = s.strLanguageCode;
			strLabel = s.strLabel;
			AudioParam = s.AudioParam;
			DataParam = s.DataParam;
			return *this;
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
		CStream(const CStream& s):iLenPartA(s.iLenPartA), iLenPartB(s.iLenPartB)
		{
		}
		CStream& operator!=(const CStream& Stream)
		{
			iLenPartA=Stream.iLenPartA; iLenPartB=Stream.iLenPartB;
			return *this;
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

		CMSCProtLev():iPartA(0),iPartB(0),iHierarch(0) {}
		CMSCProtLev(const CMSCProtLev& p):iPartA(p.iPartA),iPartB(p.iPartB),iHierarch(p.iHierarch) {}
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
		CAltFreqSched():iScheduleID(0),iDayCode(0),iStartTime(0),iDuration(0)
		{
		}
		CAltFreqSched(const CAltFreqSched& nAFS):iScheduleID(nAFS.iScheduleID),
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
		CAltFreqRegion():iRegionID(0), veciCIRAFZones(),
			iLatitude(0), iLongitude(0),
			iLatitudeEx(0), iLongitudeEx(0)
		{
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
			if (veciCIRAFZones.size() != nAFR.veciCIRAFZones.size())
				return FALSE;

			/* Vector contents */
			for (size_t i = 0; i < veciCIRAFZones.size(); i++)
				if (veciCIRAFZones[i] != nAFR.veciCIRAFZones[i])
					return FALSE;

			return TRUE;
		}

		void Reset()
		{
			iRegionID = 0;
			veciCIRAFZones.clear();
			iLatitude = 0;
			iLongitude = 0;
			iLatitudeEx = 0;
			iLongitudeEx = 0;
		}

		int iRegionID;
		vector<int> veciCIRAFZones;
		int iLatitude;
		int iLongitude;
		int iLatitudeEx;
		int iLongitudeEx;
	};

		class CAltFreq
		{
		  public:
			CAltFreq():veciFrequencies(), veciServRestrict(4),
				bIsSyncMultplx(FALSE), bRegionSchedFlag(FALSE),
				iRegionID(0), iScheduleID(0)
			{
			}
			CAltFreq(const CAltFreq& nAF):veciFrequencies(nAF.veciFrequencies),
				veciServRestrict(nAF.veciServRestrict),
				bIsSyncMultplx(nAF.bIsSyncMultplx),
				bRegionSchedFlag(nAF.bRegionSchedFlag),
				iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID)
			{
			}

			CAltFreq& operator=(const CAltFreq& nAF)
			{
				veciFrequencies = nAF.veciFrequencies;

				veciServRestrict = nAF.veciServRestrict;

				bIsSyncMultplx = nAF.bIsSyncMultplx;
				iRegionID = nAF.iRegionID;
				iScheduleID = nAF.iScheduleID;
				bRegionSchedFlag = nAF.bRegionSchedFlag;
				return *this;
			}

			_BOOLEAN operator==(const CAltFreq& nAF)
			{
				size_t i;

				/* Vector sizes */
				if (veciFrequencies.size() != nAF.veciFrequencies.size())
					return FALSE;
				if (veciServRestrict.size() != nAF.veciServRestrict.size())
					return FALSE;

				/* Vector contents */
				for (i = 0; i < veciFrequencies.size(); i++)
					if (veciFrequencies[i] != nAF.veciFrequencies[i])
						return FALSE;
				for (i = 0; i < veciServRestrict.size(); i++)
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
				veciFrequencies.clear();
				veciServRestrict.clear();
				bIsSyncMultplx = FALSE;
				bRegionSchedFlag = FALSE;
				iRegionID = iScheduleID = 0;
			}

			vector<int> veciFrequencies;
			vector<int> veciServRestrict;
			_BOOLEAN bIsSyncMultplx;
			_BOOLEAN bRegionSchedFlag;
			int iRegionID;
			int iScheduleID;
		};
	/* Alternative frequency signalling class */
	class CAltFreqSign
	{
	  public:

		CAltFreqSign():vecAltFreqRegions(),vecAltFreqSchedules(),
			vecAltFreq(),bVersionFlag(FALSE)
		{
		}

		CAltFreqSign(const CAltFreqSign& a):vecAltFreqRegions(a.vecAltFreqRegions),
			vecAltFreqSchedules(a.vecAltFreqSchedules), vecAltFreq(a.vecAltFreq),bVersionFlag(a.bVersionFlag)
		{
		}

		CAltFreqSign& operator=(const CAltFreqSign& a)
		{
			vecAltFreqRegions = a.vecAltFreqRegions;
			vecAltFreqSchedules = a.vecAltFreqSchedules;
			vecAltFreq =  a.vecAltFreq;
			bVersionFlag = a.bVersionFlag;
			return *this;
		}

		void Reset()
		{
			vecAltFreqRegions.clear();
			vecAltFreqSchedules.clear();
			vecAltFreq.clear();
			bVersionFlag = FALSE;
		}

		vector < CAltFreqRegion > vecAltFreqRegions;
		vector < CAltFreqSched > vecAltFreqSchedules;
		vector < CAltFreq > vecAltFreq;
		_BOOLEAN bVersionFlag;
	};

	/* Other Services alternative frequency signalling class */
	class CAltFreqOtherServicesSign
	{
	  public:
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
				size_t i;

				/* Vector sizes */
				if (veciFrequencies.size() != nAF.veciFrequencies.size())
					return FALSE;

				/* Vector contents */
				for (i = 0; i < veciFrequencies.size(); i++)
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

		CAltFreqOtherServicesSign():vecAltFreqOtherServices(),bVersionFlag(FALSE)
		{
		}

		CAltFreqOtherServicesSign(const CAltFreqOtherServicesSign& a):
			vecAltFreqOtherServices(a.vecAltFreqOtherServices),bVersionFlag(a.bVersionFlag)
		{
		}

		CAltFreqOtherServicesSign& operator=(const CAltFreqOtherServicesSign& a)
		{
			vecAltFreqOtherServices = a.vecAltFreqOtherServices;
			bVersionFlag = a.bVersionFlag;
			return *this;
		}

		void Reset()
		{
			vecAltFreqOtherServices.clear();
			bVersionFlag = FALSE;
		}

		vector < CAltFreqOtherServices > vecAltFreqOtherServices;
		_BOOLEAN bVersionFlag;
	};

	class CReceptLog; // forward

	class CLog
	{
	public:
		CLog():pLog(NULL),pFile(NULL) {}
		virtual ~CLog() { close(); }
		void open(const char* filename, const time_t now);
		void setLog(CReceptLog* pl) { pLog = pl; }
		void close();
		virtual void writeParameters(CDRMReceiver*)=0;
		virtual void writeHeader(time_t)=0;
		virtual void writeTrailer()=0;
		virtual void reset()=0;
	protected:
		CReceptLog* pLog;
		FILE *pFile;
	};

	class CShortLog: public CLog
	{
	public:
		virtual void writeParameters(CDRMReceiver*);
		virtual void writeHeader(time_t);
		virtual void writeTrailer();
		virtual void reset();
		void SetSNR(_REAL);
		void SetSignalStrength(_REAL);
	protected:
		int iTimeCntShort;
		int iNumSNR, iNumSigStr;
		_REAL rSumSNR, rMaxSNR, rMinSNR;
		_REAL rSumSigStr, rMaxSigStr, rMinSigStr;
	};

	class CLongLog: public CLog
	{
	public:
		virtual void writeParameters(CDRMReceiver*);
		virtual void writeHeader(time_t);
		virtual void writeTrailer();
		virtual void reset();
		void SetSNR(_REAL);
		void SetSignalStrength(_REAL);
	protected:
		time_t TimeCntLong;
		_REAL rCurSNR;
		_REAL rCurSigStr;
	};

	class CReceptLog
	{
	  public:

	  	friend class CShortLog;
	  	friend class CLongLog;

		CReceptLog();
		CReceptLog(const CReceptLog&);
		virtual ~CReceptLog()
		{
			longlog.close();
			shortlog.close();
		}
		CReceptLog& operator=(const CReceptLog&);

		void StartLogging();
		void StopLogging();
		void WriteParameters(CDRMReceiver* pDRMRec, _BOOLEAN bLong);
		void SetFAC(const _BOOLEAN bCRCOk);
		void SetMSC(const _BOOLEAN bCRCOk);
		void SetSync(const _BOOLEAN bCRCOk);
		void SetNumAAC(const int iNewNum);

		void SetLoggingEnabled(const _BOOLEAN bLog) { bLogEnabled = bLog; }
		_BOOLEAN GetLoggingEnabled() { return bLogEnabled; }

		void SetRxlEnabled(const _BOOLEAN b) { bRxlEnabled = b; }
		_BOOLEAN GetRxlEnabled() { return bRxlEnabled; }

		void SetPositionEnabled(const _BOOLEAN b) { bPositionEnabled = b; }
		_BOOLEAN GetPositionEnabled() { return bPositionEnabled; }
		unsigned int ExtractMinutes(double dblDeg);


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

		void SetAdditText(const string strNewTxt)
		{
			strAdditText = strNewTxt;
		}

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

		CGPSData GPSData; /* TODO facade this ? */
		_BOOLEAN bValidSignalStrength;
		_REAL rSigStr;  
		_REAL rIFSigStr;  
	    CShortLog shortlog;
	    CLongLog longlog;

	  protected:
		int iNumCRCOkFAC, iNumCRCOkMSC;
		int iNumCRCOkMSCLong, iNumCRCMSCLong;
		int iNumAACFrames;
		_BOOLEAN bSyncOK, bFACOk, bMSCOk;
		_BOOLEAN bSyncOKValid, bFACOkValid, bMSCOkValid;
		int iFrequency;
		_BOOLEAN bLogActivated;
		_BOOLEAN bLogEnabled;
		_BOOLEAN bRxlEnabled;
		_BOOLEAN bPositionEnabled;
		string strAdditText;
		
		int iSecDelLogStart;

		ERobMode eCurRobMode;
		ECodScheme eCurMSCScheme;
		CMSCProtLev CurProtLev;
		CMutex Mutex;
	};

	/* Class to store information about the last service selected ------------- */

	class CLastService
	{
	  public:
		CLastService():iService(0), iServiceID(SERV_ID_NOT_USED)
		{
		}
		CLastService(const CLastService& l):iService(l.iService), iServiceID(l.iServiceID)
		{
		}
		CLastService& operator=(const CLastService& l)
		{
			iService = l.iService;
			iServiceID = l.iServiceID;
			return *this;
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

	/* Class to keep track of status flags for RSCI rsta tag */
	class CReceiveStatus
	{
	  public:
		CReceiveStatus():FSyncOK(NOT_PRESENT), TSyncOK(NOT_PRESENT),InterfaceOK(NOT_PRESENT),
			FACOK(NOT_PRESENT), SDCOK(NOT_PRESENT), AudioOK(NOT_PRESENT),MOTOK(NOT_PRESENT)
		{
		}
		CReceiveStatus(const CReceiveStatus& s):FSyncOK(s.FSyncOK), TSyncOK(s.TSyncOK),
			InterfaceOK(s.InterfaceOK), FACOK(s.FACOK), SDCOK(s.SDCOK),
			AudioOK(s.AudioOK),MOTOK(s.MOTOK)
		{
		}
		CReceiveStatus& operator=(const CReceiveStatus& s)
		{
			FSyncOK = s.FSyncOK;
			TSyncOK = s.TSyncOK;
			InterfaceOK = s.InterfaceOK;
			FACOK = s.FACOK;
			SDCOK = s.SDCOK;
			AudioOK = s.AudioOK;
			MOTOK = s.MOTOK;
			return *this;
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
	};


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
	};

	class CFrontEndParameters
	{
	public:
		enum ESMeterCorrectionType {S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, S_METER_CORRECTION_TYPE_AGC_ONLY, S_METER_CORRECTION_TYPE_AGC_RSSI};

		// Constructor
		CFrontEndParameters():
			eSMeterCorrectionType(S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY), rSMeterBandwidth(10000.0),
				rDefaultMeasurementBandwidth(10000.0), bAutoMeasurementBandwidth(TRUE), rCalFactorAM(0.0),
				rCalFactorDRM(0.0), rIFCentreFreq(12000.0)
			{}
		CFrontEndParameters(const CFrontEndParameters& p):
			eSMeterCorrectionType(p.eSMeterCorrectionType), rSMeterBandwidth(p.rSMeterBandwidth),
			rDefaultMeasurementBandwidth(p.rDefaultMeasurementBandwidth),
			bAutoMeasurementBandwidth(p.bAutoMeasurementBandwidth),
			rCalFactorAM(p.rCalFactorAM), rCalFactorDRM(p.rCalFactorDRM),
			rIFCentreFreq(p.rIFCentreFreq)
			{}
		CFrontEndParameters& operator=(const CFrontEndParameters& p)
		{
			eSMeterCorrectionType = p.eSMeterCorrectionType;
			rSMeterBandwidth = p.rSMeterBandwidth;
			rDefaultMeasurementBandwidth = p.rDefaultMeasurementBandwidth;
			bAutoMeasurementBandwidth = p.bAutoMeasurementBandwidth;
			rCalFactorAM = p.rCalFactorAM;
			rCalFactorDRM = p.rCalFactorDRM;
			rIFCentreFreq = p.rIFCentreFreq;
			return *this;
		}

		ESMeterCorrectionType eSMeterCorrectionType;
		_REAL rSMeterBandwidth; // The bandwidth the S-meter uses to do the measurement

		_REAL rDefaultMeasurementBandwidth; // Bandwidth to do measurement if not synchronised
		_BOOLEAN bAutoMeasurementBandwidth; // TRUE: use the current FAC bandwidth if locked, FALSE: use default bandwidth always
		_REAL rCalFactorAM;
		_REAL rCalFactorDRM;
		_REAL rIFCentreFreq;
		
	};

class CParameter
{
  public:
	CParameter(CDRMReceiver *pRx);
	CParameter(const CParameter& p);
	virtual ~CParameter();
	CParameter& operator=(const CParameter& p);

	/* Enumerations --------------------------------------------------------- */
	/* AS: AFS in SDC is valid or not */
	enum EAFSVali { AS_VALID, AS_NOT_VALID };


	/* SI: Symbol Interleaver */
	enum ESymIntMod { SI_LONG, SI_SHORT };

	/* ST: Simulation Type */
	enum ESimType
	{ ST_NONE, ST_BITERROR, ST_MSECHANEST, ST_BER_IDEALCHAN,
		ST_SYNC_PARAM, ST_SINR
	};

	/* Misc. Functions ------------------------------------------------------ */
	void GenerateRandomSerialNumber();
	void GenerateReceiverID();
	void ResetServicesStreams();
	void GetActiveServices(vector<int>& veciActServ);
	void GetActiveStreams(vector<int>& veciActStr);
	int GetNumActiveServices();
	void InitCellMapTable(const ERobMode eNewWaveMode, const ESpecOcc eNewSpecOcc);

	void SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC);
	void SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC);
	void SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot);
	void SetNumAudioDecoderBits(const int iNewNumAudioDecoderBits);
	void SetNumDataDecoderBits(const int iNewNumDataDecoderBits);

	_BOOLEAN SetWaveMode(const ERobMode eNewWaveMode);
	ERobMode GetWaveMode() const { return eRobustnessMode; }

	void SetServiceParameters(int iShortID, const CService& newService);

	void SetCurSelAudioService(const int iNewService);
	int GetCurSelAudioService() const { return iCurSelAudioService; }
	void SetCurSelDataService(const int iNewService);
	int GetCurSelDataService() const { return iCurSelDataService; }

	void ResetCurSelAudDatServ()
	{
		iCurSelAudioService = 0;
		iCurSelDataService = 0;
	}

	void EnableMultimedia(const _BOOLEAN bFlag);
	_BOOLEAN GetEnableMultimedia() const { return bUsingMultimedia; }

	_REAL GetDCFrequency() const
	{
		return SOUNDCRD_SAMPLE_RATE * (rFreqOffsetAcqui + rFreqOffsetTrack);
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

	void SetAudDataFlag(const int iServID, const CService::ETyOServ iNewADaFl);
	void SetServID(const int iServID, const uint32_t iNewServID);

	CDRMReceiver * pDRMRec;

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
	void SetAudioParam(const int iShortID, const CAudioParam& NewAudParam);
	CAudioParam GetAudioParam(const int iShortID);
	CDataParam GetDataParam(const int iShortID);
	void SetDataParam(const int iShortID, const CDataParam& NewDataParam);

	void SetMSCProtLev(const CMSCProtLev NewMSCPrLe, const _BOOLEAN bWithHierarch);
	void SetStreamLen(const int iStreamID, const int iNewLenPartA, const int iNewLenPartB);
	void GetStreamLen(const int iStreamID, int& iLenPartA, int& iLenPartB);
	int GetStreamLen(const int iStreamID);

	/* Protection levels for MSC */
	CMSCProtLev MSCPrLe;

	vector<CStream> Stream;
	vector<CService> Service;

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

	ERecMode GetReceiverMode() { return eReceiverMode; }
	ERecMode eReceiverMode;
	EAcqStat GetReceiverState() { return eAcquiState; }
	EAcqStat eAcquiState;

	CVector <_BINARY> vecbiAudioFrameStatus;
	_BOOLEAN bMeasurePSD;

	/* vector to hold the PSD valued for the rpsd tag. */
	CVector <_REAL> vecrPSD;

	CMatrix <_COMPLEX> matcReceivedPilotValues;

	/* Simulation ----------------------------------------------------------- */
	CRawSimData RawSimDa;
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
	vector<int> iPathDelay;
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

	CReceiveStatus ReceiveStatus;
	CFrontEndParameters FrontEndParameters;
	CAltFreqSign AltFreqSign;
	CAltFreqOtherServicesSign AltFreqOtherServicesSign;
	CReceptLog ReceptLog;

	void Lock()
	{
		Mutex.Lock();
	}
	void Unlock()
	{
		Mutex.Unlock();
	}
	/* Channel Estimation */
	void SetSNR(_REAL);
	_REAL rSNREstimate;
	_REAL rMER;
	_REAL rWMERMSC;
	_REAL rWMERFAC;
	_REAL rSigmaEstimate;
	_REAL rMinDelay;
	_REAL rMaxDelay;

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
	
	/* the signal level as measured at IF by dream */
	void SetIFSignalLevel(_REAL);
	_REAL GetIFSignalLevel();

	/* the signal level as measured (at RF ?) by the front end */
	void SetSignalStrength(_BOOLEAN bValid, _REAL rNewSigStr);
	_BOOLEAN GetSignalStrength(_REAL& rSigStr);

	_REAL rSigStrengthCorrection;

	/* General -------------------------------------------------------------- */
	_REAL GetNominalBandwidth();
	_REAL GetSysToNomBWCorrFact();
	_BOOLEAN bRunThread;
	_BOOLEAN bUsingMultimedia;

	CCellMappingTable CellMappingTable;

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
};

#endif // !defined(PARAMETER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
