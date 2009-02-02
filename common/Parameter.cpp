/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	DRM Parameters
 *
 ******************************************************************************
 *
 * This program is free software(), you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation(), either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY(), without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program(), if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "Parameter.h"
#include "DrmReceiver.h"
#include "Version.h"
#include <limits>
#include <sstream>
#include <iomanip>
#include <algorithm>
//#include "util/LogPrint.h"

/* Implementation *************************************************************/

CAudioParam::CAudioParam(): CDumpable(),strTextMessage(),
    eAudioCoding(AC_AAC), eSBRFlag(SB_NOT_USED), eAudioSamplRate(AS_24KHZ),
    bTextflag(false), bEnhanceFlag(false), eAudioMode(AM_MONO),
    iCELPIndex(0), bCELPCRC(false), eHVXCRate(HR_2_KBIT), bHVXCCRC(false)
{
}

CAudioParam::CAudioParam(const CAudioParam& ap):
    CDumpable(),
    strTextMessage(ap.strTextMessage),
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

CAudioParam& CAudioParam::operator=(const CAudioParam& ap)
{
    strTextMessage = ap.strTextMessage;
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

bool CAudioParam::operator!=(const CAudioParam& AudioParam)
{
    if (eAudioCoding != AudioParam.eAudioCoding)
        return true;
    if (eSBRFlag != AudioParam.eSBRFlag)
        return true;
    if (eAudioSamplRate != AudioParam.eAudioSamplRate)
        return true;
    if (bTextflag != AudioParam.bTextflag)
        return true;
    if (bEnhanceFlag != AudioParam.bEnhanceFlag)
        return true;

    switch (AudioParam.eAudioCoding)
    {
    case AC_AAC:
        if (eAudioMode != AudioParam.eAudioMode)
            return true;
        break;

    case AC_CELP:
        if (bCELPCRC != AudioParam.bCELPCRC)
            return true;
        if (iCELPIndex != AudioParam.iCELPIndex)
            return true;
        break;

    case AC_HVXC:
        if (eHVXCRate != AudioParam.eHVXCRate)
            return true;
        if (bHVXCCRC != AudioParam.bHVXCCRC)
            return true;
        break;
    }
    return false;
}

CDataParam::CDataParam():
    CDumpable(),
    ePacketModInd(PM_PACKET_MODE),
    eDataUnitInd(DU_DATA_UNITS),
    eAppDomain(AD_DAB_SPEC_APP),
    iUserAppIdent(0)
{
}

CDataParam::CDataParam(const CDataParam& DataParam):
    CDumpable(),
    ePacketModInd(DataParam.ePacketModInd),
    eDataUnitInd(DataParam.eDataUnitInd),
    eAppDomain(DataParam.eAppDomain),
    iUserAppIdent(DataParam.iUserAppIdent)
{
}

CDataParam& CDataParam::operator=(const CDataParam& DataParam)
{
    ePacketModInd = DataParam.ePacketModInd;
    eDataUnitInd = DataParam.eDataUnitInd;
    eAppDomain = DataParam.eAppDomain;
    iUserAppIdent = DataParam.iUserAppIdent;
    return *this;
}

bool CDataParam::operator!=(const CDataParam& DataParam)
{
    if (ePacketModInd != DataParam.ePacketModInd)
        return true;
    if (DataParam.ePacketModInd == PM_PACKET_MODE)
    {
        if (eDataUnitInd != DataParam.eDataUnitInd)
            return true;
        if (eAppDomain != DataParam.eAppDomain)
            return true;
        if (DataParam.eAppDomain == AD_DAB_SPEC_APP)
            if (iUserAppIdent != DataParam.iUserAppIdent)
                return true;
    }
    return false;
}

CService::CService():
    CDumpable(),
    iServiceID(SERV_ID_NOT_USED), eCAIndication(CA_NOT_USED),
    iLanguage(0), eAudDataFlag(SF_AUDIO), iServiceDescr(0),
    strCountryCode(), strLanguageCode(), strLabel(),
    iAudioStream(STREAM_ID_NOT_USED), iDataStream(STREAM_ID_NOT_USED), iPacketID(0)
{
}

CService::CService(const CService& s):
    CDumpable(),
    iServiceID(s.iServiceID), eCAIndication(s.eCAIndication),
    iLanguage(s.iLanguage), eAudDataFlag(s.eAudDataFlag),
    iServiceDescr(s.iServiceDescr), strCountryCode(s.strCountryCode),
    strLanguageCode(s.strLanguageCode), strLabel(s.strLabel),
    iAudioStream(s.iAudioStream), iDataStream(s.iDataStream),
    iPacketID(s.iPacketID)
{
}

CService& CService::operator=(const CService& s)
{
    iServiceID = s.iServiceID;
    eCAIndication = s.eCAIndication;
    iLanguage = s.iLanguage;
    eAudDataFlag = s.eAudDataFlag;
    iServiceDescr = s.iServiceDescr;
    strCountryCode = s.strCountryCode;
    strLanguageCode = s.strLanguageCode;
    strLabel = s.strLabel;
    iAudioStream = s.iAudioStream;
    iDataStream = s.iDataStream;
    iPacketID = s.iPacketID;
    return *this;
}

CStream::CStream():CDumpable(),iLenPartA(0), iLenPartB(0),
eAudDataFlag(SF_AUDIO),ePacketModInd(PM_PACKET_MODE),iPacketLen(0)
{
}

CStream::CStream(const CStream& s):
CDumpable(),iLenPartA(s.iLenPartA), iLenPartB(s.iLenPartB),
eAudDataFlag(s.eAudDataFlag),ePacketModInd(s.ePacketModInd),
iPacketLen(s.iPacketLen)
{
}

CStream& CStream::operator=(const CStream& Stream)
{
    iLenPartA=Stream.iLenPartA; iLenPartB=Stream.iLenPartB;
    eAudDataFlag = Stream.eAudDataFlag;
    ePacketModInd=Stream.ePacketModInd;
    iPacketLen=Stream.iPacketLen;
    return *this;
}

bool CStream::operator==(const CStream& Stream)
{
    if (iLenPartA != Stream.iLenPartA)
        return false;
    if (iLenPartB != Stream.iLenPartB)
        return false;
    return true;
}

CParameter::CParameter():
 pDRMRec(NULL),
 eSymbolInterlMode(),
 eMSCCodingScheme(),
 eSDCCodingScheme(),
 iNumAudioService(0),
 iNumDataService(0),
 iAMSSCarrierMode(0),
 sReceiverID("                "),
 sSerialNumber(),
 sDataFilesDirectory("."),
 MSCPrLe(),
 Stream(MAX_NUM_STREAMS), Service(MAX_NUM_SERVICES),
 iNumBitsHierarchFrameTotal(0),
 iNumDecodedBitsMSC(0),
 iNumSDCBitsPerSFrame(0),
 iNumAudioDecoderBits(0),
 iNumDataDecoderBits(0),
 iYear(0),
 iMonth(0),
 iDay(0),
 iUTCHour(0),
 iUTCMin(0),
 iFrameIDTransm(0),
 iFrameIDReceiv(0),
 rFreqOffsetAcqui(0.0),
 rFreqOffsetTrack(0.0),
 rResampleOffset(0.0),
 iTimingOffsTrack(0),
 eAcquiState(AS_NO_SIGNAL),
 iNumAudioFrames(0),
 vecbiAudioFrameStatus(0),
 bMeasurePSD(),
 rPIRStart(_REAL(0.0)),
 rPIREnd(_REAL(0.0)),
 vecrPSD(0),
 vecrPIR(0),
 matcReceivedPilotValues(),
 RawSimDa(),
 eSimType(ST_NONE),
 iDRMChannelNum(0),
 iSpecChDoppler(0),
 rBitErrRate(0.0),
 rSyncTestParam(0.0),
 rSINR(0.0),
 iNumBitErrors(0),
 iChanEstDelay(0),
 iNumTaps(0),
 iPathDelay(MAX_NUM_TAPS_DRM_CHAN),
 rGainCorr(0.0),
 iOffUsfExtr(0),
 ReceiveStatus(),
 FrontEndParameters(),
 AltFreqSign(),
 rMER(0.0),
 rWMERMSC(0.0),
 rWMERFAC(0.0),
 rSigmaEstimate(0.0),
 rMinDelay(0.0),
 rMaxDelay(0.0),
 bMeasureDelay(),
 vecrRdel(0),
 vecrRdelThresholds(0),
 vecrRdelIntervals(0),
 bMeasureDoppler(0),
 rRdop(0.0),
 bMeasureInterference(false),
 rIntFreq(0.0),
 rINR(0.0),
 rICR(0.0),
 rMaxPSDwrtSig(0.0),
 rMaxPSDFreq(0.0),
 rSigStrengthCorrection(0.0),
 bRunThread(false),
 bUsingMultimedia(false),
 CellMappingTable(),
 GPSData(), SNRstat(), SigStrstat(),
 rSysSimSNRdB(0.0),
 iFrequency(0),
 bValidSignalStrength(false),
 rSigStr(0.0),
 rIFSigStr(0.0),
 iCurSelAudioService(0),
 iCurSelDataService(0),
 eRobustnessMode(RM_ROBUSTNESS_MODE_B),
 eSpectOccup(SO_3),
 LastAudioService(),
 LastDataService(),
 Mutex()
{
	GenerateRandomSerialNumber();
	CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);
}

CParameter::~CParameter()
{
}

CParameter::CParameter(const CParameter& p):
 CDumpable(),pDRMRec(p.pDRMRec),
 eSymbolInterlMode(p.eSymbolInterlMode),
 eMSCCodingScheme(p.eMSCCodingScheme),
 eSDCCodingScheme(p.eSDCCodingScheme),
 iNumAudioService(p.iNumAudioService),
 iNumDataService(p.iNumDataService),
 iAMSSCarrierMode(p.iAMSSCarrierMode),
 sReceiverID(p.sReceiverID),
 sSerialNumber(p.sSerialNumber),
 sDataFilesDirectory(p.sDataFilesDirectory),
 MSCPrLe(p.MSCPrLe),
 Stream(p.Stream), Service(p.Service),
 iNumBitsHierarchFrameTotal(p.iNumBitsHierarchFrameTotal),
 iNumDecodedBitsMSC(p.iNumDecodedBitsMSC),
 iNumSDCBitsPerSFrame(p.iNumSDCBitsPerSFrame),
 iNumAudioDecoderBits(p.iNumAudioDecoderBits),
 iNumDataDecoderBits(p.iNumDataDecoderBits),
 iYear(p.iYear),
 iMonth(p.iMonth),
 iDay(p.iDay),
 iUTCHour(p.iUTCHour),
 iUTCMin(p.iUTCMin),
 iFrameIDTransm(p.iFrameIDTransm),
 iFrameIDReceiv(p.iFrameIDReceiv),
 rFreqOffsetAcqui(p.rFreqOffsetAcqui),
 rFreqOffsetTrack(p.rFreqOffsetTrack),
 rResampleOffset(p.rResampleOffset),
 iTimingOffsTrack(p.iTimingOffsTrack),
 eAcquiState(p.eAcquiState),
 iNumAudioFrames(p.iNumAudioFrames),
 vecbiAudioFrameStatus(p.vecbiAudioFrameStatus),
 bMeasurePSD(p.bMeasurePSD),
 rPIRStart(p.rPIRStart),
 rPIREnd(p.rPIREnd),
 vecrPSD(p.vecrPSD),
 vecrPIR(p.vecrPIR),
 //matcReceivedPilotValues(p.matcReceivedPilotValues),
 matcReceivedPilotValues(), // OPH says copy constructor for CMatrix not safe yet
 RawSimDa(p.RawSimDa),
 eSimType(p.eSimType),
 iDRMChannelNum(p.iDRMChannelNum),
 iSpecChDoppler(p.iSpecChDoppler),
 rBitErrRate(p.rBitErrRate),
 rSyncTestParam	(p.rSyncTestParam),
 rSINR(p.rSINR),
 iNumBitErrors(p.iNumBitErrors),
 iChanEstDelay(p.iChanEstDelay),
 iNumTaps(p.iNumTaps),
 iPathDelay(p.iPathDelay),
 rGainCorr(p.rGainCorr),
 iOffUsfExtr(p.iOffUsfExtr),
 ReceiveStatus(p.ReceiveStatus),
 FrontEndParameters(p.FrontEndParameters),
 AltFreqSign(p.AltFreqSign),
 rMER(p.rMER),
 rWMERMSC(p.rWMERMSC),
 rWMERFAC(p.rWMERFAC),
 rSigmaEstimate(p.rSigmaEstimate),
 rMinDelay(p.rMinDelay),
 rMaxDelay(p.rMaxDelay),
 bMeasureDelay(p.bMeasureDelay),
 vecrRdel(p.vecrRdel),
 vecrRdelThresholds(p.vecrRdelThresholds),
 vecrRdelIntervals(p.vecrRdelIntervals),
 bMeasureDoppler(p.bMeasureDoppler),
 rRdop(p.rRdop),
 bMeasureInterference(p.bMeasureInterference),
 rIntFreq(p.rIntFreq),
 rINR(p.rINR),
 rICR(p.rICR),
 rMaxPSDwrtSig(p.rMaxPSDwrtSig),
 rMaxPSDFreq(p.rMaxPSDFreq),
 rSigStrengthCorrection(p.rSigStrengthCorrection),
 bRunThread(p.bRunThread),
 bUsingMultimedia(p.bUsingMultimedia),
 CellMappingTable(), // jfbc CCellMappingTable uses a CMatrix :(
 GPSData(p.GPSData), SNRstat(p.SNRstat), SigStrstat(p.SigStrstat),
 rSysSimSNRdB(p.rSysSimSNRdB),
 iFrequency(p.iFrequency),
 bValidSignalStrength(p.bValidSignalStrength),
 rSigStr(p.rSigStr),
 rIFSigStr(p.rIFSigStr),
 iCurSelAudioService(p.iCurSelAudioService),
 iCurSelDataService(p.iCurSelDataService),
 eRobustnessMode(p.eRobustnessMode),
 eSpectOccup(p.eSpectOccup),
 LastAudioService(p.LastAudioService),
 LastDataService(p.LastDataService)
 //, Mutex() // jfbc: I don't think this state should be copied
{
	CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);
	matcReceivedPilotValues = p.matcReceivedPilotValues; // TODO
}

CParameter& CParameter::operator=(const CParameter& p)
{
	pDRMRec = p.pDRMRec;
	eSymbolInterlMode = p.eSymbolInterlMode;
	eMSCCodingScheme = p.eMSCCodingScheme;
	eSDCCodingScheme = p.eSDCCodingScheme;
	iNumAudioService = p.iNumAudioService;
	iNumDataService = p.iNumDataService;
	iAMSSCarrierMode = p.iAMSSCarrierMode;
	sReceiverID = p.sReceiverID;
	sSerialNumber = p.sSerialNumber;
	sDataFilesDirectory = p.sDataFilesDirectory;
	MSCPrLe = p.MSCPrLe;
	Stream = p.Stream;
	Service = p.Service;
	iNumBitsHierarchFrameTotal = p.iNumBitsHierarchFrameTotal;
	iNumDecodedBitsMSC = p.iNumDecodedBitsMSC;
	iNumSDCBitsPerSFrame = p.iNumSDCBitsPerSFrame;
	iNumAudioDecoderBits = p.iNumAudioDecoderBits;
	iNumDataDecoderBits = p.iNumDataDecoderBits;
	iYear = p.iYear;
	iMonth = p.iMonth;
	iDay = p.iDay;
	iUTCHour = p.iUTCHour;
	iUTCMin = p.iUTCMin;
	iFrameIDTransm = p.iFrameIDTransm;
	iFrameIDReceiv = p.iFrameIDReceiv;
	rFreqOffsetAcqui = p.rFreqOffsetAcqui;
	rFreqOffsetTrack = p.rFreqOffsetTrack;
	rResampleOffset = p.rResampleOffset;
	iTimingOffsTrack = p.iTimingOffsTrack;
	eAcquiState = p.eAcquiState;
	iNumAudioFrames = p.iNumAudioFrames;
	vecbiAudioFrameStatus = p.vecbiAudioFrameStatus;
	bMeasurePSD = p.bMeasurePSD;
	vecrPSD = p.vecrPSD;
	vecrPIR = p.vecrPIR;
	rPIRStart = p.rPIRStart;
	rPIREnd = p.rPIREnd;
	matcReceivedPilotValues = p.matcReceivedPilotValues;
	RawSimDa = p.RawSimDa;
	eSimType = p.eSimType;
	iDRMChannelNum = p.iDRMChannelNum;
	iSpecChDoppler = p.iSpecChDoppler;
	rBitErrRate = p.rBitErrRate;
	rSyncTestParam	 = p.rSyncTestParam;
	rSINR = p.rSINR;
	iNumBitErrors = p.iNumBitErrors;
	iChanEstDelay = p.iChanEstDelay;
	iNumTaps = p.iNumTaps;
	iPathDelay = p.iPathDelay;
	rGainCorr = p.rGainCorr;
	iOffUsfExtr = p.iOffUsfExtr;
	ReceiveStatus = p.ReceiveStatus;
	FrontEndParameters = p.FrontEndParameters;
	AltFreqSign = p.AltFreqSign;
	rMER = p.rMER;
	rWMERMSC = p.rWMERMSC;
	rWMERFAC = p.rWMERFAC;
	rSigmaEstimate = p.rSigmaEstimate;
	rMinDelay = p.rMinDelay;
	rMaxDelay = p.rMaxDelay;
	bMeasureDelay = p.bMeasureDelay;
	vecrRdel = p.vecrRdel;
	vecrRdelThresholds = p.vecrRdelThresholds;
	vecrRdelIntervals = p.vecrRdelIntervals;
	bMeasureDoppler = p.bMeasureDoppler;
	rRdop = p.rRdop;
	bMeasureInterference = p.bMeasureInterference;
	rIntFreq = p.rIntFreq;
	rINR = p.rINR;
	rICR = p.rICR;
	rMaxPSDwrtSig = p.rMaxPSDwrtSig;
	rMaxPSDFreq = p.rMaxPSDFreq;
	rSigStrengthCorrection = p.rSigStrengthCorrection;
	bRunThread = p.bRunThread;
	bUsingMultimedia = p.bUsingMultimedia;
	CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup); // don't copy CMatrix
	GPSData = p.GPSData;
	SNRstat = p.SNRstat;
	SigStrstat = p.SigStrstat;
	rSysSimSNRdB = p.rSysSimSNRdB;
	iFrequency = p.iFrequency;
	bValidSignalStrength = p.bValidSignalStrength;
	rSigStr = p.rSigStr;
	rIFSigStr = p.rIFSigStr;
	iCurSelAudioService = p.iCurSelAudioService;
	iCurSelDataService = p.iCurSelDataService;
	eRobustnessMode = p.eRobustnessMode;
	eSpectOccup = p.eSpectOccup;
	LastAudioService = p.LastAudioService;
	LastDataService = p.LastDataService;

	return *this;
}

void
CParameter::SetReceiver(CDRMReceiver* p)
{
	pDRMRec = p;
}

void CParameter::ResetServicesStreams()
{
	int i;
	if(GetReceiverMode() == DRM)
	{

		/* Store informations about last services selected
		 * this for select current service automatically after a resync */

		if (iCurSelAudioService > 0)
			LastAudioService.Save(iCurSelAudioService, Service[iCurSelAudioService].iServiceID);

		if (iCurSelDataService > 0)
			LastDataService.Save(iCurSelDataService, Service[iCurSelDataService].iServiceID);

		/* Reset everything to possible start values */
		for (i = 0; i < MAX_NUM_SERVICES; i++)
		{
			Service[i].iAudioStream = STREAM_ID_NOT_USED;
			Service[i].iDataStream = STREAM_ID_NOT_USED;
			Service[i].iPacketID = 0;

			Service[i].iServiceID = SERV_ID_NOT_USED;
			Service[i].eCAIndication = CService::CA_NOT_USED;
			Service[i].iLanguage = 0;
			Service[i].strCountryCode = "";
			Service[i].strLanguageCode = "";
			Service[i].eAudDataFlag = SF_AUDIO;
			Service[i].iServiceDescr = 0;
			Service[i].strLabel = "";
		}
		/* force constructors to run */
		AudioParam.clear();
		DataParam.clear();
		/* allow index by streamID */
		AudioParam.resize(MAX_NUM_STREAMS);
		DataParam.resize(MAX_NUM_STREAMS);

		for (i = 0; i < MAX_NUM_STREAMS; i++)
		{
			Stream[i].iLenPartA = 0;
			Stream[i].iLenPartB = 0;
			DataParam[i].resize(1);
		}
	}
	else
	{

		// Set up encoded AM audio parameters
		AudioParam.resize(1);
		DataParam.clear();
		AudioParam[0].strTextMessage = "";
		AudioParam[0].eAudioCoding = CAudioParam::AC_AAC;
		AudioParam[0].eSBRFlag = CAudioParam::SB_NOT_USED;
		AudioParam[0].eAudioSamplRate = CAudioParam::AS_24KHZ;
		AudioParam[0].bTextflag = false;
		AudioParam[0].bEnhanceFlag = false;
		AudioParam[0].eAudioMode = CAudioParam::AM_MONO;
		AudioParam[0].iCELPIndex = 0;
		AudioParam[0].bCELPCRC = false;
		AudioParam[0].eHVXCRate = CAudioParam::HR_2_KBIT;
		AudioParam[0].bHVXCCRC = false;

		Service[0].iServiceID = SERV_ID_NOT_USED;
		Service[0].eCAIndication = CService::CA_NOT_USED;
		Service[0].iLanguage = 0;
		Service[0].strCountryCode = "";
		Service[0].strLanguageCode = "";
		Service[0].eAudDataFlag = SF_AUDIO;
		Service[0].iServiceDescr = 0;
		Service[0].strLabel = "";

		Stream[0].iLenPartA = 0;
		Stream[0].iLenPartB = 1044;
	}

	/* Reset alternative frequencies */
	AltFreqSign.Reset();

	/* Date, time */
	iDay = 0;
	iMonth = 0;
	iYear = 0;
	iUTCHour = 0;
	iUTCMin = 0;
}

void CParameter::GetActiveServices(set<int>& actServ)
{
	/* Init return vector */
	actServ.clear();

	/* Get active services */
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
			/* A service is active, add ID to set */
			actServ.insert(i);
	}
}

/* using a set ensures each stream appears only once */
void CParameter::GetActiveStreams(set<int>& actStr)
{
	actStr.clear();

	/* Determine which streams are active */
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
		{
			/* Audio stream */
			if (Service[i].iAudioStream != STREAM_ID_NOT_USED)
				actStr.insert(Service[i].iAudioStream);

			/* Data stream */
			if (Service[i].iDataStream != STREAM_ID_NOT_USED)
				actStr.insert(Service[i].iDataStream);
		}
	}
}

_REAL CParameter::GetBitRateKbps(const int iShortID, const bool bAudData)
{
	/* Init lengths to zero in case the stream is not yet assigned */
	int iLen = 0;

	/* First, check if audio or data service and get lengths */
	if (Service[iShortID].eAudDataFlag == SF_AUDIO)
	{
		/* Check if we want to get the data stream connected to an audio
		   stream */
		if (bAudData == true)
		{
			iLen = GetStreamLen( Service[iShortID].iDataStream);
		}
		else
		{
			iLen = GetStreamLen( Service[iShortID].iAudioStream);
		}
	}
	else
	{
		iLen = GetStreamLen( Service[iShortID].iDataStream);
	}

	/* We have 3 frames with time duration of 1.2 seconds. Bit rate should be
	   returned in kbps (/ 1000) */
	return (_REAL) iLen * BITS_BINARY * 3 / (_REAL) 1.2 / 1000;
}

_REAL CParameter::PartABLenRatio(const int iShortID)
{
	int iLenA = 0;
	int iLenB = 0;

	/* Get the length of protection part A and B */
	if (Service[iShortID].eAudDataFlag == SF_AUDIO)
	{
		/* Audio service */
		if (Service[iShortID].iAudioStream != STREAM_ID_NOT_USED)
		{
			iLenA = Stream[Service[iShortID].iAudioStream].iLenPartA;
			iLenB = Stream[Service[iShortID].iAudioStream].iLenPartB;
		}
	}
	else
	{
		/* Data service */
		if (Service[iShortID].iDataStream != STREAM_ID_NOT_USED)
		{
			iLenA = Stream[Service[iShortID].iDataStream].iLenPartA;
			iLenB = Stream[Service[iShortID].iDataStream].iLenPartB;
		}
	}

	const int iTotLen = iLenA + iLenB;

	if (iTotLen != 0)
		return (_REAL) iLenA / iTotLen;
	else
		return (_REAL) 0.0;
}

bool CParameter::SetWaveMode(const ERobMode eNewWaveMode)
{
	/* First check if spectrum occupancy and robustness mode pair is defined */
	if ((
		(eNewWaveMode == RM_ROBUSTNESS_MODE_C) ||
		(eNewWaveMode == RM_ROBUSTNESS_MODE_D)
		) && !(
		(eSpectOccup == SO_3) ||
		(eSpectOccup == SO_5)
		))
	{
		/* Set spectrum occupance to a valid parameter */
		eSpectOccup = SO_3;
	}

	/* Apply changes only if new parameter differs from old one */
	if (eRobustnessMode != eNewWaveMode)
	{
		/* Set new value */
		eRobustnessMode = eNewWaveMode;

		/* This parameter change provokes update of table */
		CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForWaveMode();

		/* Signal that parameter has changed */
		return true;
	}
	else
		return false;
}

void CParameter::SetSpectrumOccup(ESpecOcc eNewSpecOcc)
{
	/* First check if spectrum occupancy and robustness mode pair is defined */
	if ((
		(eRobustnessMode == RM_ROBUSTNESS_MODE_C) ||
		(eRobustnessMode == RM_ROBUSTNESS_MODE_D)
		) && !(
		(eNewSpecOcc == SO_3) ||
		(eNewSpecOcc == SO_5)
		))
	{
		/* Set spectrum occupance to a valid parameter */
		eNewSpecOcc = SO_3;
	}

	/* Apply changes only if new paramter differs from old one */
	if (eSpectOccup != eNewSpecOcc)
	{
		/* Set new value */
		eSpectOccup = eNewSpecOcc;

		/* This parameter change provokes update of table */
		CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForSpectrumOccup();
	}
}

void CParameter::SetStreamLen(const int iStreamID, const int iNewLenPartA,
							  const int iNewLenPartB)
{
	/* Apply changes only if parameters have changed */
	if ((Stream[iStreamID].iLenPartA != iNewLenPartA) ||
		(Stream[iStreamID].iLenPartB != iNewLenPartB))
	{
		/* Use new parameters */
		Stream[iStreamID].iLenPartA = iNewLenPartA;
		Stream[iStreamID].iLenPartB = iNewLenPartB;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSC();
	}
}

int CParameter::GetStreamLen(const int iStreamID)
{
	if(iStreamID != STREAM_ID_NOT_USED)
		return Stream[iStreamID].iLenPartA + Stream[iStreamID].iLenPartB;
	else
		return 0;
}

void CParameter::SetNumDecodedBitsMSC(const int iNewNumDecodedBitsMSC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumDecodedBitsMSC != iNumDecodedBitsMSC)
	{
		iNumDecodedBitsMSC = iNewNumDecodedBitsMSC;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}
}

void CParameter::SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumDecodedBitsSDC != iNumSDCBitsPerSFrame)
	{
		iNumSDCBitsPerSFrame = iNewNumDecodedBitsSDC;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForNoDecBitsSDC();
	}
}

void CParameter::SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumBitsHieraFrTot != iNumBitsHierarchFrameTotal)
	{
		iNumBitsHierarchFrameTotal = iNewNumBitsHieraFrTot;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}
}

void CParameter::SetMSCProtLev(const CMSCProtLev NewMSCPrLe,
							   const bool bWithHierarch)
{
	bool bParamersHaveChanged = false;

	if ((NewMSCPrLe.iPartA != MSCPrLe.iPartA) ||
		(NewMSCPrLe.iPartB != MSCPrLe.iPartB))
	{
		MSCPrLe.iPartA = NewMSCPrLe.iPartA;
		MSCPrLe.iPartB = NewMSCPrLe.iPartB;

		bParamersHaveChanged = true;
	}

	/* Apply changes only if parameters have changed */
	if (bWithHierarch == true)
	{
		if (NewMSCPrLe.iHierarch != MSCPrLe.iHierarch)
		{
			MSCPrLe.iHierarch = NewMSCPrLe.iHierarch;

			bParamersHaveChanged = true;
		}
	}

	/* In case parameters have changed, set init flags */
	if (bParamersHaveChanged == true)
		if(pDRMRec) pDRMRec->InitsForMSC();
}

void CParameter::SetServiceParameters(int iShortID, const CService& newService)
{
	Service[iShortID] = newService;
}

void CParameter::SetAudioParam(const int iShortID, const CAudioParam& NewAudParam)
{
	/* Apply changes only if parameters have changed */

	int iStreamID = Service[iShortID].iAudioStream;

	if(iStreamID == STREAM_ID_NOT_USED)
		return;

	if(AudioParam.size()<size_t(iStreamID+1))
		AudioParam.resize(iStreamID+1);

	if (AudioParam[iStreamID] != NewAudParam)
	{
		AudioParam[iStreamID] = NewAudParam;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForAudParam();
	}
}

CAudioParam CParameter::GetAudioParam(const int iShortID)
{
	if(Service[iShortID].iAudioStream != STREAM_ID_NOT_USED)
		return AudioParam[Service[iShortID].iAudioStream];

	CAudioParam default_param;
	return default_param;
}

void CParameter::SetDataParam(const int iShortID, const CDataParam& NewDataParam)
{
	size_t iStreamID = size_t(Service[iShortID].iDataStream);
	size_t iPacketID = size_t(Service[iShortID].iPacketID);

	if(DataParam.size()<(iStreamID+1))
		DataParam.resize(iStreamID+1);

	if(DataParam[iStreamID].size()<(iPacketID+1))
		DataParam[iStreamID].resize(iPacketID+1);

	CDataParam& OldDataParam = DataParam[iStreamID][iPacketID];

	/* Apply changes only if parameters have changed */
	if (OldDataParam != NewDataParam)
	{
		OldDataParam = NewDataParam;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForDataParam();
	}
}

CDataParam CParameter::GetDataParam(const int iShortID)
{
	int iStreamID = Service[iShortID].iDataStream;
	int iPacketID = Service[iShortID].iPacketID;

	if(iStreamID != STREAM_ID_NOT_USED)
		return DataParam[iStreamID][iPacketID];

	CDataParam default_param;
	return default_param;
}

void CParameter::SetInterleaverDepth(const ESymIntMod eNewDepth)
{
	if (eSymbolInterlMode != eNewDepth)
	{
		eSymbolInterlMode = eNewDepth;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForInterlDepth();
	}
}

void CParameter::SetMSCCodingScheme(const ECodScheme eNewScheme)
{
	if (eMSCCodingScheme != eNewScheme)
	{
		eMSCCodingScheme = eNewScheme;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCCodSche();
	}
}

void CParameter::SetSDCCodingScheme(const ECodScheme eNewScheme)
{
	if (eSDCCodingScheme != eNewScheme)
	{
		eSDCCodingScheme = eNewScheme;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForSDCCodSche();
	}
}

void CParameter::SetCurSelAudioService(const int iNewService)
{
	/* Change the current selected audio service ID only if the new ID does
	   contain an audio service. If not, keep the old ID. In that case it is
	   possible to select a "data-only" service and still listen to the audio of
	   the last selected service */
	if ((iCurSelAudioService != iNewService) &&
		(Service[iNewService].iAudioStream != STREAM_ID_NOT_USED))
	{
		iCurSelAudioService = iNewService;

		LastAudioService.Reset();

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForAudParam();
	}
}

void CParameter::SetCurSelDataService(const int iNewService)
{
	/* Change the current selected data service ID only if the new ID does
	   contain a data service. If not, keep the old ID. In that case it is
	   possible to select a "data-only" service and click back to an audio
	   service to be able to decode data service and listen to audio at the
	   same time */
	if ((iCurSelDataService != iNewService) &&
		(Service[iNewService].iDataStream != STREAM_ID_NOT_USED))
	{
		iCurSelDataService = iNewService;

		LastDataService.Reset();

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForDataParam();
	}
}

void CParameter::EnableMultimedia(const bool bFlag)
{
	if (bUsingMultimedia != bFlag)
	{
		bUsingMultimedia = bFlag;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}
}

void CParameter::SetNumOfServices(const size_t iNNumAuSe, const size_t iNNumDaSe)
{
	/* Check whether number of activated services is not greater than the
	   number of services signalled by the FAC because it can happen that
	   a false CRC check (it is only a 8 bit CRC) of the FAC block
	   initializes a wrong service */
	set<int> actServ;

	GetActiveServices(actServ);
	if (actServ.size() > iNNumAuSe + iNNumDaSe)
	{
		/* Reset services and streams and set flag for init modules */
		ResetServicesStreams();
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}

	if ((iNumAudioService != iNNumAuSe) || (iNumDataService != iNNumDaSe))
	{
		iNumAudioService = iNNumAuSe;
		iNumDataService = iNNumDaSe;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}
}

void CParameter::SetAudDataFlag(const int iShortID, const EStreamType iNewADaFl)
{
	if (Service[iShortID].eAudDataFlag != iNewADaFl)
	{
		Service[iShortID].eAudDataFlag = iNewADaFl;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSC();
	}
}

void CParameter::SetServiceID(const int iShortID, const uint32_t iNewServiceID)
{
	if (Service[iShortID].iServiceID != iNewServiceID)
	{
		/* JFBC - what is this for? */
		if ((iShortID == 0) && (Service[0].iServiceID > 0))
			ResetServicesStreams();

		Service[iShortID].iServiceID = iNewServiceID;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSC();


		/* If the receiver has lost the sync automatically restore
			last current service selected */

		if ((iShortID > 0) && (iNewServiceID > 0))
		{
			if(LastAudioService.iServiceID == iNewServiceID)
			{
				/* Restore last audio service selected */
				iCurSelAudioService = LastAudioService.iService;

				LastAudioService.Reset();

				/* Set init flags */
				if(pDRMRec) pDRMRec->InitsForAudParam();
			}

			if (LastDataService.iServiceID == iNewServiceID)
			{
				/* Restore last data service selected */
				iCurSelDataService = LastDataService.iService;

				LastDataService.Reset();

				/* Set init flags */
				if(pDRMRec) pDRMRec->InitsForDataParam();
			}
		}
	}
}


/* Implementaions for simulation -------------------------------------------- */
void CRawSimData::Add(uint32_t iNewSRS)
{
	/* Attention, function does not take care of overruns, data will be
	   lost if added to a filled shift register! */
	if (iCurWritePos < ciMaxDelBlocks)
		veciShRegSt[iCurWritePos++] = iNewSRS;
}

uint32_t CRawSimData::Get()
{
	/* We always use the first value of the array for reading and do a
	   shift of the other data by adding a arbitrary value (0) at the
	   end of the whole shift register */
	uint32_t iRet = veciShRegSt[0];
	veciShRegSt.AddEnd(0);
	iCurWritePos--;

	return iRet;
}

_REAL CParameter::GetSysSNRdBPilPos() const
{
/*
	Get system SNR in dB for the pilot positions. Since the average power of
	the pilots is higher than the data cells, the SNR is also higher at these
	positions compared to the total SNR of the DRM signal.
*/
	return (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSysSimSNRdB / 10) /
		CellMappingTable.rAvPowPerSymbol * CellMappingTable.rAvScatPilPow * (_REAL) CellMappingTable.iNumCarrier);
}

void
CParameter::SetSNR(const _REAL iNewSNR)
{
	SNRstat.addSample(iNewSNR);
}

_REAL
CParameter::GetSNR()
{
	return SNRstat.getCurrent();
}

_REAL CParameter::GetNominalSNRdB()
{
	/* Convert SNR from system bandwidth to nominal bandwidth */
	return (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSysSimSNRdB / 10) *
		GetSysToNomBWCorrFact());
}

void CParameter::SetNominalSNRdB(const _REAL rSNRdBNominal)
{
	/* Convert SNR from nominal bandwidth to system bandwidth */
	rSysSimSNRdB = (_REAL) 10.0 * log10(pow((_REAL) 10.0, rSNRdBNominal / 10) /
		GetSysToNomBWCorrFact());
}

_REAL CParameter::GetNominalBandwidth()
{
	_REAL rNomBW;

	/* Nominal bandwidth as defined in the DRM standard */
	switch (eSpectOccup)
	{
	case SO_0:
		rNomBW = (_REAL) 4500.0; /* Hz */
		break;

	case SO_1:
		rNomBW = (_REAL) 5000.0; /* Hz */
		break;

	case SO_2:
		rNomBW = (_REAL) 9000.0; /* Hz */
		break;

	case SO_3:
		rNomBW = (_REAL) 10000.0; /* Hz */
		break;

	case SO_4:
		rNomBW = (_REAL) 18000.0; /* Hz */
		break;

	case SO_5:
		rNomBW = (_REAL) 20000.0; /* Hz */
		break;

	default:
		rNomBW = (_REAL) 10000.0; /* Hz */
		break;
	}

	return rNomBW;
}

_REAL CParameter::GetSysToNomBWCorrFact()
{
	_REAL rNomBW = GetNominalBandwidth();

	/* Calculate system bandwidth (N / T_u) */
	const _REAL rSysBW = (_REAL) CellMappingTable.iNumCarrier / CellMappingTable.iFFTSizeN * SOUNDCRD_SAMPLE_RATE;

	return rSysBW / rNomBW;
}


void CParameter::SetIFSignalLevel(_REAL rNewSigStr)
{
	rIFSigStr = rNewSigStr;
}

_REAL CParameter::GetIFSignalLevel()
{
	return rIFSigStr;
}

void CRxStatus::SetStatus(const ETypeRxStatus OK)
{
	status = OK;
	iNum++;
	if(OK==RX_OK)
		iNumOK++;
}

EDemodulationType
CParameter::GetReceiverMode()
{
	return (pDRMRec)?pDRMRec->GetReceiverMode():DRM;
}

void CParameter::GenerateReceiverID()
{
	//Set receiver ID
	string sVer;
	unsigned int iImplementation = 0;;
	unsigned int iMajor = 0;
	unsigned int iMinor = 0;

	sReceiverID = "drea";

	sVer = dream_version;

	size_t pos;

	while((pos = sVer.find('.')) != string::npos)
		sVer.replace(pos, 1, " ");

	if ((pos = sVer.find("cvs")) != string::npos)
		sVer.replace(pos, 3, "   ");

	stringstream ssVer(sVer);
	ssVer >> iImplementation >> iMajor >> iMinor;

	stringstream ssInfoVer;
	ssInfoVer << setw(2) << setfill('0') << iImplementation << setw(2) << setfill('0') << iMajor << setw(2) << setfill('0') << iMinor;

	sReceiverID += ssInfoVer.str();

	while (sSerialNumber.length() < 6)
			sSerialNumber += "_";

	if (sSerialNumber.length() > 6)
		sSerialNumber.erase(6, pDRMRec->GetParameters()->sSerialNumber.length()-6);

	sReceiverID += pDRMRec->GetParameters()->sSerialNumber;
}

void CParameter::GenerateRandomSerialNumber()
{
	//seed random number generator
	srand((unsigned int)time(0));

	char randomChars[36];

	for (size_t q=0; q < 36; q++)
	{
		if (q < 26)
			randomChars[q] = char(q)+97;
		else
			randomChars[q] = (char(q)-26)+48;
	}

	char serialNumTemp[7];

	for (size_t i=0; i < 6; i++)
		serialNumTemp[i] = randomChars[(int) 35.0*rand()/RAND_MAX];

	serialNumTemp[6] = '\0';

	sSerialNumber = serialNumTemp;
}

CMinMaxMean::CMinMaxMean():CDumpable(),rSum(0.0),rCur(0.0),
rMin(numeric_limits<_REAL>::max()),rMax(numeric_limits<_REAL>::min()),iNum(0)
{
}

void CMinMaxMean::setInvalid()
{
	iNum = 0;
}

bool CMinMaxMean::isValid()
{
	return iNum>0;
}

void
CMinMaxMean::addSample(_REAL r)
{
	rCur = r;
	rSum += r;
	iNum++;
	if(r>rMax)
		rMax = r;
	if(r<rMin)
		rMin = r;
}

_REAL
CMinMaxMean::getCurrent()
{
	return rCur;
}

_REAL
CMinMaxMean::getMean()
{
	_REAL rMean = 0.0;
	if(iNum>0)
		rMean = rSum / iNum;
	rSum = 0.0;
	iNum = 0;
	return rMean;
}

void
CMinMaxMean::getMinMax(_REAL& rMinOut, _REAL& rMaxOut)
{
	if(rMin <= rMax)
	{
		rMinOut = rMin;
		rMaxOut = rMax;
	}
	else
	{
		rMinOut = 0.0;
		rMaxOut = 0.0;
	}
	rMin = numeric_limits<_REAL>::max();
	rMax = numeric_limits<_REAL>::min();
}

string CServiceDefinition::Frequency(size_t n) const
{
	if(n>=veciFrequencies.size())
		return ""; // not in the list

	stringstream ss;
	int iFrequency = veciFrequencies[n];

	switch (iSystemID)
	{
	case 0:
	case 1:
	case 2:
		/* AM or DRM */
		ss << iFrequency;
		break;

	case 3:
	case 4:
	case 5:
		/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
		ss << 87.5 + 0.1 * float(iFrequency);
		break;

	case 6:
	case 7:
	case 8:
		/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
		ss << 76.0 + 0.1 * float(iFrequency);
		break;

	case 9:
	case 10:
	case 11:
		if(iFrequency<=11) {
			int chan = iFrequency / 4;
			char subchan = 'A' + iFrequency % 4;
			ss << "Band I channel " << (chan+2) << subchan;
		} else if(64<= iFrequency && iFrequency <=95) {
			int chan = iFrequency / 4;
			char subchan = 'A' + iFrequency % 4;
			ss << "Band III channel " << (chan-11) << subchan;
		} else if(96<= iFrequency && iFrequency <=101) {
			int chan = iFrequency / 6;
			char subchan = 'A' + iFrequency % 6;
			ss << "Band III+ channel " << (chan-3) << subchan;
		} else if(128<= iFrequency && iFrequency <=143) {
			char chan = iFrequency - 128;
			double m = 1452.96+1.712*double(chan);
			ss << "European L-Band channel L" << ('A'+chan) << ", " << m << " MHz";
		} else if(160<= iFrequency && iFrequency <=182) {
			int chan = iFrequency - 159;
            double m = 1451.072+1.744*double(chan);
			ss << "Canadian L-Band channel " << chan << ", " << m << " MHz";
		} else {
			ss << "unknown channel " << iFrequency;
		}
		break;
	default:
		break;
	}
	return ss.str();
}

string CServiceDefinition::FrequencyUnits() const
{
	switch (iSystemID)
	{
	case 0:
	case 1:
	case 2:
		return "kHz";
		break;

	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		return "MHz";
		break;

	default:
		return "";
		break;
	}
}

string CServiceDefinition::System() const
{
	switch (iSystemID)
	{
	case 0:
		return "DRM";
		break;

	case 1:
	case 2:
		return "AM";
		break;

	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
		return "FM";
		break;
	case 9:
	case 10:
	case 11:
		return "DAB";
		break;

	default:
		return "";
		break;
	}
}

string COtherService::ServiceID() const
{
	stringstream ss;
	/*
	switch (iSystemID)
	{
	case 0:
	case 1:
		ss << "ID:" << hex << setw(6) << iServiceID;
		break;

	case 3:
	case 6:
		ss << "ECC+PI:" << hex << setw(6) << iServiceID;
		break;
	case 4:
	case 7:
		ss << "PI:" << hex << setw(4) << iServiceID;
		break;
	case 9:
		ss << "ECC+aud:" << hex << setw(6) << iServiceID;
		break;
	case 10:
		ss << "AUDIO:" << hex << setw(4) << iServiceID;
		break;
	case 11:
		ss << "DATA:" << hex << setw(8) << iServiceID;
		break;
		break;

	default:
		break;
	}
	*/
	return ss.str();
}

/* See ETSI ES 201 980 v2.1.1 Annex O */
bool
CAltFreqSched::IsActive(const time_t ltime)
{
	int iScheduleStart;
	int iScheduleEnd;
	int iWeekDay;

	/* Empty schedule is always active */
	if (iDuration == 0)
		return true;

	/* Calculate time in UTC */
	struct tm *gmtCur = gmtime(&ltime);

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0)
	   I must normalize so Monday = 0   */

	if (gmtCur->tm_wday == 0)
		iWeekDay = 6;
	else
		iWeekDay = gmtCur->tm_wday - 1;

	/* iTimeWeek minutes since last Monday 00:00 in UTC */
	/* the value is in the range 0 <= iTimeWeek < 60 * 24 * 7)   */

	const int iTimeWeek =
		(iWeekDay * 24 * 60) + (gmtCur->tm_hour * 60) + gmtCur->tm_min;

	/* Day Code: this field indicates which days the frequency schedule
	 * (the following Start Time and Duration) applies to.
	 * The msb indicates Monday, the lsb Sunday. Between one and seven bits may be set to 1.
	 */
	for (int i = 0; i < 7; i++)
	{
		/* Check if day is active */
		if ((1 << (6 - i)) & iDayCode)
		{
			/* Tuesday -> 1 * 24 * 60 = 1440 */
			iScheduleStart = (i * 24 * 60) + iStartTime;
			iScheduleEnd = iScheduleStart + iDuration;

			/* the normal check (are we inside start and end?) */
			if ((iTimeWeek >= iScheduleStart) && (iTimeWeek <= iScheduleEnd))
				return true;

			/* the wrap-around check */
			const int iMinutesPerWeek = 7 * 24 * 60;

			if (iScheduleEnd > iMinutesPerWeek)
			{
				/* our duration wraps into next Monday (or even later) */
				if (iTimeWeek < (iScheduleEnd - iMinutesPerWeek))
					return true;
			}
		}
	}
	return false;
}

ostream& operator<<(ostream& out, const CDumpable& d)
{
    d.dump(out);
    return out;
}

template<typename T>
void dump(ostream& out, T val)
{
    out << val;
}

template<typename T>
void dump(ostream& out, const vector<T>& vec)
{
    string sep = "";
    out << "[";
    for (size_t i = 0; i < vec.size(); i++)
    {
        out << sep; ::dump(out, vec[i]);
        sep = ", ";
    }
    out << "]";
}

//for_each(a.begin(); a.end(); dump);

template<typename T>
void dump(ostream& out, const CVector<T>& vec)
{
    out << "[ " << endl;
    string sep = "";
    for(int i=0; i<vec.Size(); i++)
    {
//        out << sep; ::dump(out, vec[i]); out << endl;
        out << sep << vec[i] << endl;
        sep = ", ";
    }
    out << "]" << endl;
}

template<typename T>
void dump(ostream& out, const CMatlibVector<T>& vec)
{
    out << "[ " << endl;
    string sep = "";
    for(int i=0; i<vec.Size(); i++)
    {
        out << sep << vec[i] << endl;
        sep = ", ";
    }
    out << "]" << endl;
}

void
CAudioParam::dump(ostream& out) const
{
    out << "{ textmessage: '" << strTextMessage << "'," << endl;
    out << "audioCoding: " << int(eAudioCoding) << "," << endl;
    out << "SBRFlag: " << int(eSBRFlag) << "," << endl;
    out << "SampleRate: " << int(eAudioSamplRate) << "," << endl;
    out << "Textflag: " << int(bTextflag) << "," << endl;
    out << "EnhanceFlag: " << int(bEnhanceFlag) << "," << endl;
    out << "AudioMode: " << int(eAudioMode) << "," << endl;
    out << "CELPIndex: " << int(iCELPIndex) << "," << endl;
    out << "CELPCRC: " << int(bCELPCRC) << "," << endl;
    out << "HVXCRate: " << int(eHVXCRate) << "," << endl;
    out << "HVXCCRC: " << int(bHVXCCRC) << "}" << endl;
}

void
CDataParam::dump(ostream& out) const
{
    out << "{ DataUnits: " << int(eDataUnitInd) << "," << endl;
    out << "AppDomain: " << int(eAppDomain) << "," << endl;
    out << "UserAppIdent: " << "0x" << hex << iUserAppIdent << dec << "}" << endl;
}

void
CService::dump(ostream& out) const
{
    out << "{ ServiceID: " << iServiceID << "," << endl;
    out << "CAIndication: " << int(eCAIndication) << "," << endl;
    out << "Language: " << iLanguage << "," << endl;
    out << "AudDataFlag: " << int(eAudDataFlag) << "," << endl;
    out << "ServiceDescr: " << iServiceDescr << "," << endl;
    out << "CountryCode: '" << strCountryCode << "'," << endl;
    out << "LanguageCode: '" << strLanguageCode << "'," << endl;
    out << "Label: '" << strLabel << "'," << endl;
    out << "AudioStream: " << iAudioStream << "," << endl;
    out << "DataStream: " << iDataStream << "," << endl;
    out << "PacketID: " << iPacketID << "}" << endl;
}

void
CStream::dump(ostream& out) const
{
    out << "{ iLenPartA: " << iLenPartA << "," << endl;
    out << "iLenPartB: " << iLenPartB << "," << endl;
    out << "AudDataFlag: " << int(eAudDataFlag) << "," << endl;
    out << "PacketMode: " << int(ePacketModInd) << "," << endl;
    out << "PacketLen: " << iPacketLen << "}" << endl;
}

void
CMSCProtLev::dump(ostream& out) const
{
    out << "{ iPartA: " << iPartA << "," << endl;
    out << "iPartB: " << iPartB << "," << endl;
    out << "Hierarch: " << iHierarch << "}" << endl;
}

void
CAltFreqSched::dump(ostream& out) const
{
    out << "{ iDayCode: " << iDayCode << "," << endl;
    out << "StartTime: " << iStartTime << "," << endl;
    out << "Duration: " << iDuration << "}" << endl;
}

void
CAltFreqRegion::dump(ostream& out) const
{
    out << "{ CIRAFZones: "; ::dump(out, veciCIRAFZones); out << endl;
    out << "Latitude: " << iLatitude << "," << endl;
    out << "LatitudeEx: " << iLatitudeEx << "," << endl;
    out << "LongitudeEx: " << iLongitudeEx << "}" << endl;
}

void
CServiceDefinition::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "}" << endl;
}

void
CMultiplexDefinition::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "," << endl;
    out << "{ ServiceRestriction: "; ::dump(out, veciServRestrict); out << endl;
    out << "IsSyncMultplx: " << bIsSyncMultplx << "}" << endl;
}

void
COtherService::dump(ostream& out) const
{
    out << "{ Frequencies: "; ::dump(out, veciFrequencies); out << endl;
    out << "RegionID: " << iRegionID << "," << endl;
    out << "ScheduleID: " << iScheduleID << "," << endl;
    out << "SystemID: " << iSystemID << "," << endl;
    out << "SameService: " << bSameService << "," << endl;
    out << "ShortID: " << iShortID << "," << endl;
    out << "ServiceID: " << iServiceID << "}" << endl;
}

void
CAltFreqSign::dump(ostream& out) const
{
    out << "{ Regions: "; ::dump(out, vecRegions); out << endl;
    out << "Schedules: "; ::dump(out, vecSchedules); out << endl;
    out << "Multiplexes: "; ::dump(out, vecMultiplexes); out << endl;
    out << "OtherServices: "; ::dump(out, vecOtherServices); out << endl;
    out << "RegionVersionFlag: " << bRegionVersionFlag << "," << endl;
    out << "ScheduleVersionFlag: " << bScheduleVersionFlag << "," << endl;
    out << "MultiplexVersionFlag: " << bMultiplexVersionFlag << "," << endl;
    out << "OtherServicesVersionFlag: " << bOtherServicesVersionFlag << "}" << endl;
}

void
CLastService::dump(ostream& out) const
{
    out << "{ Service: " << iService << ", ServiceID: " << iServiceID << "}" << endl;
}

void
CRxStatus::dump(ostream& out) const
{
    out << "{ status: " << int(status) << ", Num: " << iNum << ", NumOK: " << iNumOK << "}" << endl;
}

void
CReceiveStatus::dump(ostream& out) const
{
    out << "{ ";
    out << "FSync: "; FSync.dump(out); out << ", ";
    out << "TSync: "; TSync.dump(out); out << ", ";
    out << "Interface: "; Interface.dump(out); out << ", ";
    out << "FAC: "; FAC.dump(out); out << ", ";
    out << "Audio "; Audio.dump(out); out << ", ";
    out << "LLAudio: "; LLAudio.dump(out); out << ", ";
    out << "MOT: "; MOT.dump(out);
    out << "}" << endl;
}

void
CFrontEndParameters::dump(ostream& out) const
{
    out << "{ SMeterCorrectionType: " << int(eSMeterCorrectionType) << "," << endl;
    out << "SMeterBandwidth: " << rSMeterBandwidth << "," << endl;
    out << "DefaultMeasurementBandwidth: " << rDefaultMeasurementBandwidth << "," << endl;
    out << "AutoMeasurementBandwidth: " << bAutoMeasurementBandwidth << "," << endl;
    out << "CalFactorAM: " << rCalFactorAM << "," << endl;
    out << "CalFactorDRM: " << rCalFactorDRM << "," << endl;
    out << "IFCentreFreq: " << rIFCentreFreq << "}" << endl;
}

void
CMinMaxMean::dump(ostream& out) const
{
    out << "{ Sum: " <<  rSum << "," << endl;
    out << "Cur: " << rCur << "," << endl;
    out << "Min: " << rMin << "," << endl;
    out << "Max " << rMax << "," << endl;
    out << "Num: " << iNum << "}" << endl;
}

void
CParameter::dump(ostream& out) const
{
    out << "{ " << endl;

    out << "SymbolInterlMode: " <<  int(eSymbolInterlMode) << "," << endl;
    out << "MSCCodingScheme: " <<  int(eMSCCodingScheme) << "," << endl;
    out << "SDCCodingScheme: " <<  int(eSDCCodingScheme) << "," << endl;
    out << "NumAudioService: " <<  iNumAudioService << "," << endl;
    out << "NumDataService " <<  iNumDataService << "," << endl;
    out << "AMSSCarrierMode: " <<  iAMSSCarrierMode << "," << endl;
    out << "ReceiverID: '" <<  sReceiverID << "'," << endl;
    out << "SerialNumber '" <<  sSerialNumber << "'," << endl;
    out << "DataFilesDirectory: '" <<  sDataFilesDirectory << "'," << endl;
    out << "MSCPrLe: "; MSCPrLe.dump(out); out << "," << endl;
    out << "Stream: "; ::dump(out, Stream); out << "," << endl;
    string sep="";
    for(vector<CStream>::const_iterator i=Stream.begin();i!= Stream.end(); i++)
    {
        out << sep; i->dump(out); out << endl;
        sep = " ";
    }
    out << "]" << endl;
    out << "Service: "; ::dump(out, Service); out << "," << endl;
    out << "AudioParam: "; ::dump(out, AudioParam); out << "," << endl;
    out << "DataParam: "; ::dump(out, DataParam); out << "," << endl;
    out << "ServiceInformation: {" << endl;
    // TODO make CServiceInformation dumpable
    sep = "";
    for(map<uint32_t,CServiceInformation>::const_iterator i=ServiceInformation.begin();
        i!= ServiceInformation.end(); i++)
    {
        out << i->first << ": {";
        out << "id: " <<  i->second.id << "," << endl;
        out << "label: [" << endl;
        string sep2="";
        for(set<string>::const_iterator j=i->second.label.begin();
        j!=i->second.label.end(); j++)
        {
            out << sep2 << "'" << (*j) << "'";
        }
        out << "]}" << endl;
        sep = ", ";
    }
    out << "}" << endl;
    out << "NumBitsHierarchFrameTotal: " <<  iNumBitsHierarchFrameTotal << "," << endl;
    out << "NumDecodedBitsMSC: " <<  iNumDecodedBitsMSC << "," << endl;
    out << "NumSDCBitsPerSFrame: " <<  iNumSDCBitsPerSFrame << "," << endl;
    out << "NumAudioDecoderBits: " <<  iNumAudioDecoderBits << "," << endl;
    out << "NumDataDecoderBits: " <<  iNumDataDecoderBits << "," << endl;
    out << "Year: " <<  iYear << "," << endl;
    out << "Month: " <<  iMonth << "," << endl;
    out << "Day: " <<  iDay << "," << endl;
    out << "UTCHour: " <<  iUTCHour << "," << endl;
    out << "UTCMin: " <<  iUTCMin << "," << endl;
    out << "FrameIDTransm: " <<  iFrameIDTransm << "," << endl;
    out << "FrameIDReceiv: " <<  iFrameIDReceiv << "," << endl;
    out << "FreqOffsetAcqui: " <<  rFreqOffsetAcqui << "," << endl;
    out << "FreqOffsetTrack: " <<  rFreqOffsetTrack << "," << endl;
    out << "ResampleOffset: " <<  rResampleOffset << "," << endl;
    out << "TimingOffsTrack: " <<  iTimingOffsTrack << "," << endl;
    out << "AcquiState: " <<  eAcquiState << "," << endl;
    out << "NumAudioFrames: " <<  iNumAudioFrames << "," << endl;
    out << "AudioFrameStatus: "; ::dump(out, vecbiAudioFrameStatus); out << endl;
    out << "MeasurePSD: " <<  bMeasurePSD << "," << endl;
    out << "PIRStart: " <<  rPIRStart << "," << endl;
    out << "PIREnd: " <<  rPIREnd << "," << endl;
    out << "PSD: "; ::dump(out, vecrPSD); out << endl;
    out << "PIR: "; ::dump(out, vecrPIR); out << endl;
	//CMatrix <_COMPLEX> matcReceivedPilotValues;
    out << "CarOffset: " <<  rCarOffset << "," << endl;
    out << "OutputFormat: " <<  int(eOutputFormat) << "," << endl;
    out << "ReceiveStatus: "; ReceiveStatus.dump(out); out << endl;
    out << "FrontEndParameters: "; FrontEndParameters.dump(out); out << endl;
    out << "AltFreqSign: "; AltFreqSign.dump(out); out << endl;
    out << "MER: " <<  rMER << "," << endl;
    out << "WMERMSC: " <<  rWMERMSC << "," << endl;
    out << "WMERFAC: " <<  rWMERFAC << "," << endl;
    out << "SigmaEstimate: " <<  rSigmaEstimate << "," << endl;
    out << "MinDelay: " << rMinDelay << "," << endl;
    out << "MaxDelay: " <<  rMaxDelay << "," << endl;
    out << "MeasureDelay: " <<  bMeasureDelay << "," << endl;
    out << "Rdel: "; ::dump(out, vecrRdel); out << endl;
    out << "RdelThresholds: "; ::dump(out, vecrRdelThresholds); out << endl;
    out << "RdelIntervals: "; ::dump(out, vecrRdelIntervals); out << endl;
    out << "MeasureDoppler: " <<  bMeasureDoppler << "," << endl;
    out << "Rdop: " <<  rRdop << "," << endl;
    out << "MeasureInterference: " <<  bMeasureInterference << "," << endl;
    out << "IntFreq: " <<  rIntFreq << "," << endl;
    out << "INR: " <<  rINR << "," << endl;
    out << "ICR: " <<  rICR << "," << endl;
    out << "MaxPSDwrtSig: " <<  rMaxPSDwrtSig << "," << endl;
    out << "MaxPSDFreq: " <<  rMaxPSDFreq << "," << endl;
    out << "UsingMultimedia: " <<  bUsingMultimedia << "," << endl;
	//CCellMappingTable CellMappingTable;
	//CGPSData GPSData;
    out << "SNRstat: " << SNRstat << endl;
    out << "SigStrstat: " << SigStrstat << endl;
    out << "SysSimSNRdB: " <<  rSysSimSNRdB << "," << endl;
    out << "Frequency: " <<  iFrequency << "," << endl;
    out << "ValidSignalStrength: " <<  bValidSignalStrength << "," << endl;
    out << "SigStr: " <<  rSigStr << "," << endl;
    out << "IFSigStr: " <<  rIFSigStr << "," << endl;
    out << "CurSelAudioService: " <<  iCurSelAudioService << "," << endl;
    out << "CurSelDataService: " <<  iCurSelDataService << "," << endl;
    out << "RobustnessMode: " <<  int(eRobustnessMode) << "," << endl;
    out << "SpectOccup: " <<  int(eSpectOccup) << "," << endl;
    out << "LastAudioService: "; LastAudioService.dump(out); out << endl;
    out << "LastDataService: "; LastDataService.dump(out); out << endl;
    out << "}" << endl;
}
