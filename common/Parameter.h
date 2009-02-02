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
#include "GPSData.h"
#include "ServiceInformation.h"
#include <map>
#ifdef HAVE_QT
# include <qthread.h>
# include <qmutex.h>
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

	/* Acquisition state of receiver */
enum EAcqStat {AS_NO_SIGNAL, AS_WITH_SIGNAL};

enum EStreamType { SF_AUDIO, SF_DATA };

enum EPackMod { PM_SYNCHRON_STR_MODE, PM_PACKET_MODE }; /* PM: Packet Mode */

enum EDemodulationType { DRM, AM, USB, LSB, CW, NBFM, WBFM, NONE };

enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ_POS,
		OF_IQ_NEG /* I / Q */, OF_EP /* envelope / phase */};

/* Classes ********************************************************************/

    class CDumpable
    {
    public:
        virtual void dump(ostream&) const = 0;
        CDumpable() {}
        virtual ~CDumpable() {}
    };

	class CAudioParam : public CDumpable
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

        CAudioParam();
        CAudioParam(const CAudioParam&);
        CAudioParam& operator=(const CAudioParam&);
		bool operator!=(const CAudioParam&);
		void dump(ostream&) const;

		/* Text-message */
		string strTextMessage;	/* Max length is (8 * 16 Bytes) */

		EAudCod eAudioCoding;	/* This field indicated the source coding system */
		ESBRFlag eSBRFlag;		/* SBR flag */
		EAudSamRat eAudioSamplRate;	/* Audio sampling rate */
		bool bTextflag;		/* Indicates whether a text message is present or not */
		bool bEnhanceFlag;	/* Enhancement flag */

		/* For AAC: Mono, LC Stereo, Stereo --------------------------------- */
		EAudMode eAudioMode;	/* Audio mode */

		/* For CELP --------------------------------------------------------- */
		int iCELPIndex;			/* This field indicates the CELP bit rate index */
		bool bCELPCRC;		/* This field indicates whether the CRC is used or not */

		/* For HVXC --------------------------------------------------------- */
		EHVXCRate eHVXCRate;	/* This field indicates the rate of the HVXC */
		bool bHVXCCRC;		/* This field indicates whether the CRC is used or not */

	};

	class CDataParam : public CDumpable
	{
	  public:

		/* DU: Data Unit */
		enum EDatUnit { DU_SINGLE_PACKETS, DU_DATA_UNITS };

		/* AD: Application Domain */
		enum EApplDomain { AD_DRM_SPEC_APP, AD_DAB_SPEC_APP, AD_OTHER_SPEC_APP };

		EPackMod ePacketModInd;	/* Packet mode indicator */

		/* In case of packet mode ------------------------------------------- */
		EDatUnit eDataUnitInd;	/* Data unit indicator */

		// "DAB specified application" not yet implemented!!!
		EApplDomain eAppDomain;	/* Application domain */
		int iUserAppIdent;		/* User application identifier, only DAB */

		CDataParam();
		CDataParam(const CDataParam&);
        CDataParam& operator=(const CDataParam&);
        bool operator!=(const CDataParam&);
		void dump(ostream&) const;
	};

	class CService : public CDumpable
	{
	  public:

		/* CA: CA system */
		enum ECACond { CA_USED, CA_NOT_USED };


		CService();
		CService(const CService& s);
		CService& operator=(const CService& s);

		bool IsActive() const
		{
			return iServiceID != SERV_ID_NOT_USED;
		}
		void dump(ostream&) const;

		uint32_t iServiceID;
		ECACond eCAIndication;
		int iLanguage;
		EStreamType eAudDataFlag;
		int iServiceDescr;
		string strCountryCode;
		string strLanguageCode;

		/* Label of the service */
		string strLabel;

		/* Audio Component */
		int iAudioStream;

		/* Data Component */
		int iDataStream;
		int iPacketID;
	};

	class CStream : public CDumpable
	{
	  public:

		CStream();
		CStream(const CStream&);
		CStream& operator=(const CStream&);
		bool operator==(const CStream&);
		void dump(ostream&) const;

		int iLenPartA;			/* Data length for part A */
		int iLenPartB;			/* Data length for part B */
		EStreamType eAudDataFlag; /* stream is audio or data */
		EPackMod ePacketModInd;	/* Packet mode indicator for data streams */
		int iPacketLen;			/* Packet length for packet streams */
	};

	class CMSCProtLev : public CDumpable
	{
	  public:

		CMSCProtLev():CDumpable(),iPartA(0),iPartB(0),iHierarch(0) {}
		CMSCProtLev(const CMSCProtLev& p):
		CDumpable(),iPartA(p.iPartA),iPartB(p.iPartB),iHierarch(p.iHierarch) {}
		CMSCProtLev& operator=(const CMSCProtLev& NewMSCProtLev)
		{
			iPartA = NewMSCProtLev.iPartA;
			iPartB = NewMSCProtLev.iPartB;
			iHierarch = NewMSCProtLev.iHierarch;
			return *this;
		}

		int iPartA;				/* MSC protection level for part A */
		int iPartB;				/* MSC protection level for part B */
		int iHierarch;			/* MSC protection level for hierachical frame */
		void dump(ostream&) const;
	};

	/* Alternative Frequency Signalling ************************************** */
	/* Alternative frequency signalling Schedules informations class */
	class CAltFreqSched : public CDumpable
	{
	  public:
		CAltFreqSched():CDumpable(),iDayCode(0),iStartTime(0),iDuration(0)
		{
		}
		CAltFreqSched(const CAltFreqSched& nAFS):
            CDumpable(),
			iDayCode(nAFS.iDayCode), iStartTime(nAFS.iStartTime),
			iDuration(nAFS.iDuration)
		{
		}

		CAltFreqSched& operator=(const CAltFreqSched& nAFS)
		{
			iDayCode = nAFS.iDayCode;
			iStartTime = nAFS.iStartTime;
			iDuration = nAFS.iDuration;

			return *this;
		}

		bool operator==(const CAltFreqSched& nAFS)
		{
			if (iDayCode != nAFS.iDayCode)
				return false;
			if (iStartTime != nAFS.iStartTime)
				return false;
			if (iDuration != nAFS.iDuration)
				return false;

			return true;
		}

		bool IsActive(const time_t ltime);

		int iDayCode;
		int iStartTime;
		int iDuration;
		void dump(ostream&) const;
	};

	/* Alternative frequency signalling Regions informations class */
	class CAltFreqRegion : public CDumpable
	{
	  public:
		CAltFreqRegion():CDumpable(),veciCIRAFZones(),
			iLatitude(0), iLongitude(0),
			iLatitudeEx(0), iLongitudeEx(0)
		{
		}
		CAltFreqRegion(const CAltFreqRegion& nAFR):
            CDumpable(),
			veciCIRAFZones(nAFR.veciCIRAFZones),
			iLatitude(nAFR.iLatitude),
			iLongitude(nAFR.iLongitude),
			iLatitudeEx(nAFR.iLatitudeEx), iLongitudeEx(nAFR.iLongitudeEx)
		{
		}

		CAltFreqRegion& operator=(const CAltFreqRegion& nAFR)
		{
			iLatitude = nAFR.iLatitude;
			iLongitude = nAFR.iLongitude;
			iLatitudeEx = nAFR.iLatitudeEx;
			iLongitudeEx = nAFR.iLongitudeEx;

			veciCIRAFZones = nAFR.veciCIRAFZones;

			return *this;
		}

		bool operator==(const CAltFreqRegion& nAFR)
		{
			if (iLatitude != nAFR.iLatitude)
				return false;
			if (iLongitude != nAFR.iLongitude)
				return false;
			if (iLatitudeEx != nAFR.iLatitudeEx)
				return false;
			if (iLongitudeEx != nAFR.iLongitudeEx)
				return false;

			/* Vector sizes */
			if (veciCIRAFZones.size() != nAFR.veciCIRAFZones.size())
				return false;

			/* Vector contents */
			for (size_t i = 0; i < veciCIRAFZones.size(); i++)
				if (veciCIRAFZones[i] != nAFR.veciCIRAFZones[i])
					return false;

			return true;
		}

		vector<int> veciCIRAFZones;
		int iLatitude;
		int iLongitude;
		int iLatitudeEx;
		int iLongitudeEx;
		void dump(ostream&) const;
	};

	class CServiceDefinition : public CDumpable
	{
 	public:
		CServiceDefinition():
		CDumpable(),veciFrequencies(), iRegionID(0), iScheduleID(0),iSystemID(0)
		{
		}

		CServiceDefinition(const CServiceDefinition& nAF):
			CDumpable(),veciFrequencies(nAF.veciFrequencies),
			iRegionID(nAF.iRegionID), iScheduleID(nAF.iScheduleID),
			iSystemID(nAF.iSystemID)
		{
		}

		CServiceDefinition& operator=(const CServiceDefinition& nAF)
		{
			veciFrequencies = nAF.veciFrequencies;
			iRegionID = nAF.iRegionID;
			iScheduleID = nAF.iScheduleID;
			iSystemID = nAF.iSystemID;
			return *this;
		}

		bool operator==(const CServiceDefinition& sd) const
		{
			size_t i;

			/* Vector sizes */
			if (veciFrequencies.size() != sd.veciFrequencies.size())
				return false;

			/* Vector contents */
			for (i = 0; i < veciFrequencies.size(); i++)
				if (veciFrequencies[i] != sd.veciFrequencies[i])
					return false;

			if (iRegionID != sd.iRegionID)
				return false;

			if (iScheduleID != sd.iScheduleID)
				return false;

			if (iSystemID != sd.iSystemID)
				return false;

			return true;
		}
		bool operator!=(const CServiceDefinition& sd) const { return !(*this==sd); }

		string Frequency(size_t) const;
		string FrequencyUnits() const;
		string System() const;

		vector<int> veciFrequencies;
		int iRegionID;
		int iScheduleID;
		int iSystemID;
		void dump(ostream&) const;
	};

	class CMultiplexDefinition: public CServiceDefinition
	{
 	public:
		CMultiplexDefinition():CServiceDefinition(), veciServRestrict(4), bIsSyncMultplx(false)
		{
		}

		CMultiplexDefinition(const CMultiplexDefinition& nAF):CServiceDefinition(nAF),
			veciServRestrict(nAF.veciServRestrict),
			bIsSyncMultplx(nAF.bIsSyncMultplx)
		{
		}

		CMultiplexDefinition& operator=(const CMultiplexDefinition& nAF)
		{
			CServiceDefinition(*this) = nAF;
			veciServRestrict = nAF.veciServRestrict;
			bIsSyncMultplx = nAF.bIsSyncMultplx;
			return *this;
		}

		bool operator==(const CMultiplexDefinition& md) const
		{
			if (CServiceDefinition(*this) != md)
				return false;

			/* Vector sizes */
			if (veciServRestrict.size() != md.veciServRestrict.size())
				return false;

			/* Vector contents */
			for (size_t i = 0; i < veciServRestrict.size(); i++)
				if (veciServRestrict[i] != md.veciServRestrict[i])
					return false;

			if (bIsSyncMultplx != md.bIsSyncMultplx)
				return false;

			return true;
		}

		vector<int> veciServRestrict;
		bool bIsSyncMultplx;
		void dump(ostream&) const;
	};

	class COtherService: public CServiceDefinition
	{
	public:
		COtherService(): CServiceDefinition(), bSameService(true),
			iShortID(0), iServiceID(SERV_ID_NOT_USED)
		{
		}

		COtherService(const COtherService& nAF):
			CServiceDefinition(nAF), bSameService(nAF.bSameService),
			iShortID(nAF.iShortID), iServiceID(nAF.iServiceID)
		{
		}

		COtherService& operator=(const COtherService& nAF)
		{
			CServiceDefinition(*this) = nAF;

			bSameService = nAF.bSameService;
			iShortID = nAF.iShortID;
			iServiceID = nAF.iServiceID;

			return *this;
		}

		bool operator==(const COtherService& nAF)
		{
			if (CServiceDefinition(*this) != nAF)
				return false;

			if (bSameService != nAF.bSameService)
				return false;

			if (iShortID != nAF.iShortID)
				return false;

			if (iServiceID != nAF.iServiceID)
				return false;

			return true;
		}

		string ServiceID() const;

		bool bSameService;
		int iShortID;
		uint32_t iServiceID;
		void dump(ostream&) const;
	};

	/* Alternative frequency signalling class */
	class CAltFreqSign : public CDumpable
	{
	  public:

		CAltFreqSign():
            CDumpable(),
            vecRegions(16),vecSchedules(16),vecMultiplexes(),vecOtherServices(),
			bRegionVersionFlag(false),bScheduleVersionFlag(false),
			bMultiplexVersionFlag(false),bOtherServicesVersionFlag(false)
		{
		}

		CAltFreqSign(const CAltFreqSign& a):
            CDumpable(),vecRegions(a.vecRegions),
			vecSchedules(a.vecSchedules), vecMultiplexes(a.vecMultiplexes),
			bRegionVersionFlag(a.bRegionVersionFlag),
			bScheduleVersionFlag(a.bScheduleVersionFlag),
			bMultiplexVersionFlag(a.bMultiplexVersionFlag),
			bOtherServicesVersionFlag(a.bOtherServicesVersionFlag)
		{
		}

		CAltFreqSign& operator=(const CAltFreqSign& a)
		{
			vecRegions = a.vecRegions;
			vecSchedules = a.vecSchedules;
			vecMultiplexes = a.vecMultiplexes;
			bRegionVersionFlag = a.bRegionVersionFlag;
			bScheduleVersionFlag = a.bScheduleVersionFlag;
			bMultiplexVersionFlag = a.bMultiplexVersionFlag;
			bOtherServicesVersionFlag = a.bOtherServicesVersionFlag;
			return *this;
		}

		void ResetRegions(bool b)
		{
			vecRegions.clear();
			vecRegions.resize(16);
			bRegionVersionFlag=b;
		}

		void ResetSchedules(bool b)
		{
			vecSchedules.clear();
			vecSchedules.resize(16);
			bScheduleVersionFlag=b;
		}

		void ResetMultiplexes(bool b)
		{
			vecMultiplexes.clear();
			bMultiplexVersionFlag=b;
		}

		void ResetOtherServices(bool b)
		{
			vecOtherServices.clear();
			bOtherServicesVersionFlag=b;
		}

		void Reset()
		{
			ResetRegions(false);
			ResetSchedules(false);
			ResetMultiplexes(false);
			ResetOtherServices(false);
		}

		vector < vector<CAltFreqRegion> > vecRegions; // outer vector indexed by regionID
		vector < vector<CAltFreqSched> > vecSchedules; // outer vector indexed by scheduleID
		vector < CMultiplexDefinition > vecMultiplexes;
		vector < COtherService > vecOtherServices;
		bool bRegionVersionFlag;
		bool bScheduleVersionFlag;
		bool bMultiplexVersionFlag;
		bool bOtherServicesVersionFlag;
		void dump(ostream&) const;
	};

	/* Class to store information about the last service selected ------------- */

	class CLastService : public CDumpable
	{
	  public:
		CLastService():CDumpable(),iService(0), iServiceID(SERV_ID_NOT_USED)
		{
		}
		CLastService(const CLastService& l):
		CDumpable(),iService(l.iService), iServiceID(l.iServiceID)
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
		void dump(ostream&) const;
	};

	/* Classes to keep track of status flags for RSCI rsta tag and log file */
	class CRxStatus : public CDumpable
	{
	public:
		CRxStatus():CDumpable(),status(NOT_PRESENT),iNum(0),iNumOK(0) {}
		CRxStatus(const CRxStatus& s):
		CDumpable(),status(s.status),iNum(s.iNum),iNumOK(s.iNumOK) {}
		CRxStatus& operator=(const CRxStatus& s)
			{ status = s.status; iNum = s.iNum; iNumOK = s.iNumOK; return *this;}
		void SetStatus(const ETypeRxStatus);
		ETypeRxStatus GetStatus() { return status; }
		int GetCount() { return iNum; }
		int GetOKCount() { return iNumOK; }
		void ResetCounts() { iNum=0; iNumOK = 0; }
		void dump(ostream&) const;
	private:
		ETypeRxStatus status;
		int iNum, iNumOK;
	};

	class CReceiveStatus : public CDumpable
	{
	  public:
		CReceiveStatus():CDumpable(),FSync(),TSync(),Interface(),
		FAC(),SDC(),Audio(),LLAudio(),MOT()
		{
		}
		CReceiveStatus(const CReceiveStatus& s):
            CDumpable(),FSync(s.FSync), TSync(s.TSync),
			Interface(s.Interface), FAC(s.FAC), SDC(s.SDC),
			Audio(s.Audio),LLAudio(s.LLAudio),MOT(s.MOT)
		{
		}
		CReceiveStatus& operator=(const CReceiveStatus& s)
		{
			FSync = s.FSync;
			TSync = s.TSync;
			Interface = s.Interface;
			FAC = s.FAC;
			SDC = s.SDC;
			Audio = s.Audio;
			LLAudio = s.LLAudio;
			MOT = s.MOT;
			return *this;
		}

		CRxStatus FSync;
		CRxStatus TSync;
		CRxStatus Interface;
		CRxStatus FAC;
		CRxStatus SDC;
		CRxStatus Audio;
		CRxStatus LLAudio;
		CRxStatus MOT;
		void dump(ostream&) const;
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

	class CFrontEndParameters : public CDumpable
	{
	public:
		enum ESMeterCorrectionType {S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY, S_METER_CORRECTION_TYPE_AGC_ONLY, S_METER_CORRECTION_TYPE_AGC_RSSI};

		// Constructor
		CFrontEndParameters():
            CDumpable(),
			eSMeterCorrectionType(S_METER_CORRECTION_TYPE_CAL_FACTOR_ONLY), rSMeterBandwidth(10000.0),
				rDefaultMeasurementBandwidth(10000.0), bAutoMeasurementBandwidth(true), rCalFactorAM(0.0),
				rCalFactorDRM(0.0), rIFCentreFreq(12000.0)
			{}
		CFrontEndParameters(const CFrontEndParameters& p):
            CDumpable(),
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
		bool bAutoMeasurementBandwidth; // true: use the current FAC bandwidth if locked, false: use default bandwidth always
		_REAL rCalFactorAM;
		_REAL rCalFactorDRM;
		_REAL rIFCentreFreq;

		void dump(ostream&) const;
	};


    class CMinMaxMean : public CDumpable
    {
    public:
        CMinMaxMean();

        void addSample(_REAL);
        _REAL getCurrent();
        _REAL getMean();
        void getMinMax(_REAL&, _REAL&);
        void setInvalid();
        bool isValid();
        void dump(ostream&) const;
    protected:
        _REAL rSum, rCur, rMin, rMax;
        int iNum;
    };

class CParameter : public CDumpable
{
  public:
	CParameter();
	CParameter(const CParameter&);
	//CParameter(CDRMReceiver *pRx, CParameter *pParameter); // OPH - just copy some of the members
	virtual ~CParameter();
	CParameter& operator=(const CParameter&);

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
	void SetReceiver(CDRMReceiver* p);
	EDemodulationType GetReceiverMode();
	void GenerateRandomSerialNumber();
	void GenerateReceiverID();
	void ResetServicesStreams();
	void GetActiveServices(set<int>& actServ);
	void GetActiveStreams(set<int>& actStr);

	void SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC);
	void SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC);
	void SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot);

	bool SetWaveMode(const ERobMode eNewWaveMode);
	ERobMode GetWaveMode() const { return eRobustnessMode; }

	void SetFrequency(int iNewFrequency) { iFrequency = iNewFrequency; }
	int GetFrequency() { return iFrequency; }

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

	void EnableMultimedia(const bool bFlag);
	bool GetEnableMultimedia() const { return bUsingMultimedia; }

	_REAL GetDCFrequency() const
	{
		return SOUNDCRD_SAMPLE_RATE * (rFreqOffsetAcqui + rFreqOffsetTrack);
	}

	_REAL GetBitRateKbps(const int iShortID, const bool bAudData);
	_REAL PartABLenRatio(const int iShortID);

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

	void SetNumOfServices(const size_t iNNumAuSe, const size_t iNNumDaSe);
	size_t GetTotNumServices()
	{
		return iNumAudioService + iNumDataService;
	}

	void SetAudDataFlag(const int iShortID, const EStreamType iNewADaFl);
	void SetServiceID(const int iShortID, const uint32_t iNewServiceID);

	CDRMReceiver* pDRMRec;

	/* Symbol interleaver mode (long or short interleaving) */
	ESymIntMod eSymbolInterlMode;

	ECodScheme eMSCCodingScheme;	/* MSC coding scheme */
	ECodScheme eSDCCodingScheme;	/* SDC coding scheme */

	size_t iNumAudioService;
	size_t iNumDataService;

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

	void SetMSCProtLev(const CMSCProtLev NewMSCPrLe, const bool bWithHierarch);
	void SetStreamLen(const int iStreamID, const int iNewLenPartA, const int iNewLenPartB);
	void GetStreamLen(const int iStreamID, int& iLenPartA, int& iLenPartB);
	int GetStreamLen(const int iStreamID);

	/* Protection levels for MSC */
	CMSCProtLev MSCPrLe;

	vector<CStream> Stream;
	vector<CService> Service;
	vector<CAudioParam> AudioParam; /* index by streamID */
	vector<vector<CDataParam> > DataParam; /* first index streamID, second index packetID */

	/* information about services gathered from SDC, EPG and web schedules */
	map<uint32_t,CServiceInformation> ServiceInformation;

	/* These values are used to set input and output block sizes of some modules */
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

	EAcqStat GetAcquiState() { return eAcquiState; }
	EAcqStat eAcquiState;
	int iNumAudioFrames;

	CVector <_BINARY> vecbiAudioFrameStatus;
	bool bMeasurePSD;
	_REAL rPIRStart;
	_REAL rPIREnd;

	/* vector to hold the PSD values for the rpsd tag. */
	CVector <_REAL> vecrPSD;

	// vector to hold impulse response values for (proposed) rpir tag
	CVector <_REAL> vecrPIR;

	CMatrix <_COMPLEX> matcReceivedPilotValues;

	/* For Transmitter */
	_REAL rCarOffset;
	enum EOutFormat eOutputFormat;

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

	void SetSNR(const _REAL);
	_REAL GetSNR();
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

	void Lock()
	{
#ifdef HAVE_QT
		Mutex.lock();
#endif
	}
	void unlock()
	{
#ifdef HAVE_QT
		Mutex.Unlock();
#endif
	}
	/* Channel Estimation */
	_REAL rMER;
	_REAL rWMERMSC;
	_REAL rWMERFAC;
	_REAL rSigmaEstimate;
	_REAL rMinDelay;
	_REAL rMaxDelay;

	bool bMeasureDelay;
	CRealVector vecrRdel;
	CRealVector vecrRdelThresholds;
	CRealVector vecrRdelIntervals;
	bool bMeasureDoppler;
	_REAL rRdop;
	/* interference (constellation-based measurement rnic)*/
	bool bMeasureInterference;
	_REAL rIntFreq, rINR, rICR;

	/* peak of PSD - for PSD-based interference measurement rnip */
	_REAL rMaxPSDwrtSig;
	_REAL rMaxPSDFreq;

	/* the signal level as measured at IF by dream */
	void SetIFSignalLevel(_REAL);
	_REAL GetIFSignalLevel();
	_REAL rSigStrengthCorrection;

	/* General -------------------------------------------------------------- */
	_REAL GetNominalBandwidth();
	_REAL GetSysToNomBWCorrFact();
	bool bRunThread;
	bool bUsingMultimedia;

	CCellMappingTable CellMappingTable;

	CGPSData GPSData;
	CMinMaxMean SNRstat, SigStrstat;
    void dump(ostream&) const;

protected:

	_REAL rSysSimSNRdB;

	int iFrequency;
	bool bValidSignalStrength;
	_REAL rSigStr;
	_REAL rIFSigStr;

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
