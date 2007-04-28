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
#include <iomanip>
#include "util/LogPrint.h"

void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam);

/* Implementation *************************************************************/
CParameter::CParameter(CDRMReceiver *pRx):
 pDRMRec(pRx),
 eSymbolInterlMode(),
 eMSCCodingScheme(),	
 eSDCCodingScheme(),	
 iNumAudioService(0),
 iNumDataService(0),
 iAMSSCarrierMode(0),
 sReceiverID("                "),
 sSerialNumber(),
 sDataFilesDirectory("./"),
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
 eReceiverMode(RM_NONE),
 eAcquiState(AS_NO_SIGNAL),
 iNumAudioFrames(0),
 vecbiAudioFrameStatus(0),
 bMeasurePSD(),
 vecrPSD(0),
 matcReceivedPilotValues(),
 RawSimDa(),
 eSimType(),
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
 AltFreqOtherServicesSign(),
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
 bMeasureInterference(FALSE),
 rIntFreq(0.0),
 rINR(0.0),
 rICR(0.0),
 rMaxPSDwrtSig(0.0),
 rMaxPSDFreq(0.0),
 rSigStrengthCorrection(0.0),
 bRunThread(FALSE),
 bUsingMultimedia(FALSE),
 CellMappingTable(),
 GPSData(),
 rSysSimSNRdB(0.0),
 iFrequency(0),
 bValidSignalStrength(FALSE),
 rSigStr(0.0),
 rIFSigStr(0.0),
 iCurSelAudioService(0),
 iCurSelDataService(0),
 eRobustnessMode(RM_ROBUSTNESS_MODE_A),	
 eSpectOccup(SO_0),
 LastAudioService(),
 LastDataService(),
 Mutex()
{
	GenerateRandomSerialNumber();
	if(pDRMRec)
		eReceiverMode = pDRMRec->GetReceiverMode();
}

 CParameter::CParameter(CDRMReceiver *pRx, CParameter *pParameter):
 pDRMRec(pRx),
 eSymbolInterlMode(),
 eMSCCodingScheme(),	
 eSDCCodingScheme(),	
 iNumAudioService(0),
 iNumDataService(0),
 iAMSSCarrierMode(0),
 sReceiverID(pParameter->sReceiverID), // OPH
 sSerialNumber(pParameter->sSerialNumber), // OPH
 sDataFilesDirectory(pParameter->sDataFilesDirectory), // OPH
 MSCPrLe(),
 Stream(MAX_NUM_STREAMS), Service(MAX_NUM_SERVICES),
 iNumBitsHierarchFrameTotal(0),
 iNumDecodedBitsMSC(0),
 iNumSDCBitsPerSFrame(0),	
 iNumAudioDecoderBits(0),	
 iNumDataDecoderBits(0),	
 iYear(pParameter->iYear), // OPH
 iMonth(pParameter->iMonth), // OPH
 iDay(pParameter->iDay), // OPH
 iUTCHour(pParameter->iUTCHour), // OPH
 iUTCMin(pParameter->iUTCMin), // OPH
 iFrameIDTransm(0),
 iFrameIDReceiv(0),
 rFreqOffsetAcqui(0.0),
 rFreqOffsetTrack(0.0),
 rResampleOffset(0.0),
 iTimingOffsTrack(0),
 eReceiverMode(RM_NONE),
 eAcquiState(AS_NO_SIGNAL),
 vecbiAudioFrameStatus(0),
 bMeasurePSD(pParameter->bMeasurePSD), // OPH
 vecrPSD(0),
 matcReceivedPilotValues(),
 RawSimDa(),
 eSimType(),
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
 FrontEndParameters(pParameter->FrontEndParameters),  // OPH
 AltFreqSign(),
 AltFreqOtherServicesSign(),
 rMER(0.0),
 rWMERMSC(0.0),
 rWMERFAC(0.0),
 rSigmaEstimate(0.0),
 rMinDelay(0.0),
 rMaxDelay(0.0),
 bMeasureDelay(pParameter->bMeasureDelay), // OPH
 vecrRdel(0),
 vecrRdelThresholds(0),
 vecrRdelIntervals(0),
 bMeasureDoppler(pParameter->bMeasureDoppler), // OPH
 rRdop(0.0),
 bMeasureInterference(pParameter->bMeasureInterference), // OPH
 rIntFreq(0.0),
 rINR(0.0),
 rICR(0.0),
 rMaxPSDwrtSig(0.0),
 rMaxPSDFreq(0.0),
 rSigStrengthCorrection(pParameter->rSigStrengthCorrection), // OPH
 bRunThread(pParameter->bRunThread),  // OPH
 bUsingMultimedia(pParameter->bUsingMultimedia), // OPH
 CellMappingTable(),
 rSysSimSNRdB(0.0),
 iCurSelAudioService(0),
 iCurSelDataService(0),
 eRobustnessMode(RM_ROBUSTNESS_MODE_A),	
 eSpectOccup(SO_0),
 LastAudioService(),
 LastDataService(),
 Mutex()
{
	//GenerateRandomSerialNumber();  // OPH
	if(pDRMRec)
		eReceiverMode = pDRMRec->GetReceiverMode();
}


CParameter::~CParameter()
{
}

CParameter::CParameter(const CParameter& p):
 pDRMRec(p.pDRMRec),
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
 eReceiverMode(p.eReceiverMode),
 eAcquiState(p.eAcquiState),
 iNumAudioFrames(p.iNumAudioFrames),
 vecbiAudioFrameStatus(p.vecbiAudioFrameStatus),
 bMeasurePSD(p.bMeasurePSD),
 vecrPSD(p.vecrPSD),
 matcReceivedPilotValues(p.matcReceivedPilotValues),
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
 AltFreqOtherServicesSign(p.AltFreqOtherServicesSign),
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
 CellMappingTable(p.CellMappingTable),
 GPSData(p.GPSData),
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
	eReceiverMode = p.eReceiverMode;
	eAcquiState = p.eAcquiState;
	iNumAudioFrames = p.iNumAudioFrames;
	vecbiAudioFrameStatus = p.vecbiAudioFrameStatus;
	bMeasurePSD = p.bMeasurePSD;
	vecrPSD = p.vecrPSD;
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
	AltFreqOtherServicesSign = p.AltFreqOtherServicesSign;
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
	CellMappingTable = p.CellMappingTable;
	GPSData = p.GPSData;
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

void CParameter::ResetServicesStreams()
{
	int i;
	if(GetReceiverMode() == RM_DRM)
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
			Service[i].AudioParam.strTextMessage = "";
			Service[i].AudioParam.iStreamID = STREAM_ID_NOT_USED;
			Service[i].AudioParam.eAudioCoding = CAudioParam::AC_AAC;
			Service[i].AudioParam.eSBRFlag = CAudioParam::SB_NOT_USED;
			Service[i].AudioParam.eAudioSamplRate = CAudioParam::AS_24KHZ;
			Service[i].AudioParam.bTextflag = FALSE;
			Service[i].AudioParam.bEnhanceFlag = FALSE;
			Service[i].AudioParam.eAudioMode = CAudioParam::AM_MONO;
			Service[i].AudioParam.iCELPIndex = 0;
			Service[i].AudioParam.bCELPCRC = FALSE;
			Service[i].AudioParam.eHVXCRate = CAudioParam::HR_2_KBIT;
			Service[i].AudioParam.bHVXCCRC = FALSE;

			Service[i].DataParam.iStreamID = STREAM_ID_NOT_USED;
			Service[i].DataParam.ePacketModInd = CDataParam::PM_PACKET_MODE;
			Service[i].DataParam.eDataUnitInd = CDataParam::DU_SINGLE_PACKETS;
			Service[i].DataParam.iPacketID = 0;
			Service[i].DataParam.iPacketLen = 0;
			Service[i].DataParam.eAppDomain = CDataParam::AD_DRM_SPEC_APP;
			Service[i].DataParam.iUserAppIdent = 0;

			Service[i].iServiceID = SERV_ID_NOT_USED;
			Service[i].eCAIndication = CService::CA_NOT_USED;
			Service[i].iLanguage = 0;
			Service[i].strCountryCode = "";
			Service[i].strLanguageCode = "";
			Service[i].eAudDataFlag = CService::SF_AUDIO;
			Service[i].iServiceDescr = 0;
			Service[i].strLabel = "";
		}

		for (i = 0; i < MAX_NUM_STREAMS; i++)
		{
			Stream[i].iLenPartA = 0;
			Stream[i].iLenPartB = 0;
		}
	}
	else
	{

		// Set up encoded AM audio parameters
		Service[0].AudioParam.strTextMessage = "";
		Service[0].AudioParam.iStreamID = 0;
		Service[0].AudioParam.eAudioCoding = CAudioParam::AC_AAC;
		Service[0].AudioParam.eSBRFlag = CAudioParam::SB_NOT_USED;
		Service[0].AudioParam.eAudioSamplRate = CAudioParam::AS_24KHZ;
		Service[0].AudioParam.bTextflag = FALSE;
		Service[0].AudioParam.bEnhanceFlag = FALSE;
		Service[0].AudioParam.eAudioMode = CAudioParam::AM_MONO;
		Service[0].AudioParam.iCELPIndex = 0;
		Service[0].AudioParam.bCELPCRC = FALSE;
		Service[0].AudioParam.eHVXCRate = CAudioParam::HR_2_KBIT;
		Service[0].AudioParam.bHVXCCRC = FALSE;

		Stream[0].iLenPartA = 0;
		Stream[0].iLenPartB = 1044;
	}

	/* Reset alternative frequencies */
	AltFreqSign.Reset();
	AltFreqOtherServicesSign.Reset();

	/* Date, time */
	iDay = 0;
	iMonth = 0;
	iYear = 0;
	iUTCHour = 0;
	iUTCMin = 0;
}

int CParameter::GetNumActiveServices()
{
	int iNumAcServ = 0;

	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
			iNumAcServ++;
	}

	return iNumAcServ;
}

void CParameter::GetActiveServices(vector<int>& veciActServ)
{
	/* Init return vector */
	veciActServ.resize(0);

	/* Get active services */
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
			/* A service is active, add ID to vector */
			veciActServ.push_back(i);
	}
}

void CParameter::GetActiveStreams(vector<int>& veciActStr)
{
	int				i;

	veciActStr.resize(0);

	/* Determine which streams are active */
	for (i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
		{
			/* Audio stream */
			if (Service[i].AudioParam.iStreamID != STREAM_ID_NOT_USED)
				veciActStr.push_back(Service[i].AudioParam.iStreamID);

			/* Data stream */
			if (Service[i].DataParam.iStreamID != STREAM_ID_NOT_USED)
				veciActStr.push_back(Service[i].DataParam.iStreamID);
		}
	}
}

_REAL CParameter::GetBitRateKbps(const int iServiceID, const _BOOLEAN bAudData)
{
	/* Init lengths to zero in case the stream is not yet assigned */
	int iLen = 0;

	/* First, check if audio or data service and get lengths */
	if (Service[iServiceID].eAudDataFlag == CService::SF_AUDIO)
	{
		/* Check if we want to get the data stream connected to an audio
		   stream */
		if (bAudData == TRUE)
		{
			iLen = GetStreamLen( Service[iServiceID].DataParam.iStreamID);
		}
		else
		{
			iLen = GetStreamLen( Service[iServiceID].AudioParam.iStreamID);
		}
	}
	else
	{
		iLen = GetStreamLen( Service[iServiceID].DataParam.iStreamID);
	}

	/* We have 3 frames with time duration of 1.2 seconds. Bit rate should be
	   returned in kbps (/ 1000) */
	return (_REAL) iLen * SIZEOF__BYTE * 3 / (_REAL) 1.2 / 1000;
}

_REAL CParameter::PartABLenRatio(const int iServiceID)
{
	int iLenA = 0;
	int iLenB = 0;

	/* Get the length of protection part A and B */
	if (Service[iServiceID].eAudDataFlag == CService::SF_AUDIO)
	{
		/* Audio service */
		if (Service[iServiceID].AudioParam.iStreamID != STREAM_ID_NOT_USED)
		{
			iLenA = Stream[Service[iServiceID].AudioParam.iStreamID].iLenPartA;
			iLenB = Stream[Service[iServiceID].AudioParam.iStreamID].iLenPartB;
		}
	}
	else
	{
		/* Data service */
		if (Service[iServiceID].DataParam.iStreamID != STREAM_ID_NOT_USED)
		{
			iLenA = Stream[Service[iServiceID].DataParam.iStreamID].iLenPartA;
			iLenB = Stream[Service[iServiceID].DataParam.iStreamID].iLenPartB;
		}
	}

	const int iTotLen = iLenA + iLenB;

	if (iTotLen != 0)
		return (_REAL) iLenA / iTotLen;
	else
		return (_REAL) 0.0;
}

void CParameter::InitCellMapTable(const ERobMode eNewWaveMode,
								  const ESpecOcc eNewSpecOcc)
{
	/* Set new values and make table */
	eRobustnessMode = eNewWaveMode;
	eSpectOccup = eNewSpecOcc;
	CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);
}

_BOOLEAN CParameter::SetWaveMode(const ERobMode eNewWaveMode)
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

	/* Apply changes only if new paramter differs from old one */
	if (eRobustnessMode != eNewWaveMode)
	{
		/* Set new value */
		eRobustnessMode = eNewWaveMode;

		/* This parameter change provokes update of table */
		CellMappingTable.MakeTable(eRobustnessMode, eSpectOccup);

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForWaveMode();

		/* Signal that parameter has changed */
		return TRUE;
	}
	else
		return FALSE;
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

void CParameter::SetNumAudioDecoderBits(const int iNewNumAudioDecoderBits)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumAudioDecoderBits != iNumAudioDecoderBits)
	{
		iNumAudioDecoderBits = iNewNumAudioDecoderBits;
	}
}

void CParameter::SetNumDataDecoderBits(const int iNewNumDataDecoderBits)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumDataDecoderBits != iNumDataDecoderBits)
	{
		iNumDataDecoderBits = iNewNumDataDecoderBits;
	}
}

void CParameter::SetMSCProtLev(const CMSCProtLev NewMSCPrLe,
							   const _BOOLEAN bWithHierarch)
{
	_BOOLEAN bParamersHaveChanged = FALSE;

	if ((NewMSCPrLe.iPartA != MSCPrLe.iPartA) ||
		(NewMSCPrLe.iPartB != MSCPrLe.iPartB))
	{
		MSCPrLe.iPartA = NewMSCPrLe.iPartA;
		MSCPrLe.iPartB = NewMSCPrLe.iPartB;

		bParamersHaveChanged = TRUE;
	}

	/* Apply changes only if parameters have changed */
	if (bWithHierarch == TRUE)
	{
		if (NewMSCPrLe.iHierarch != MSCPrLe.iHierarch)
		{
			MSCPrLe.iHierarch = NewMSCPrLe.iHierarch;
		
			bParamersHaveChanged = TRUE;
		}
	}

	/* In case parameters have changed, set init flags */
	if (bParamersHaveChanged == TRUE)
		if(pDRMRec) pDRMRec->InitsForMSC();
}

void CParameter::SetServiceParameters(int iShortID, const CService& newService)
{
	Service[iShortID] = newService;
}

void CParameter::SetAudioParam(const int iShortID, const CAudioParam& NewAudParam)
{
	/* Apply changes only if parameters have changed */
	if (Service[iShortID].AudioParam != NewAudParam)
	{
		Service[iShortID].AudioParam = NewAudParam;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForAudParam();
	}
}

CAudioParam CParameter::GetAudioParam(const int iShortID)
{
	return Service[iShortID].AudioParam;
}

void CParameter::SetDataParam(const int iShortID, const CDataParam& NewDataParam)
{
	CDataParam& DataParam = Service[iShortID].DataParam;

	/* Apply changes only if parameters have changed */
	if (DataParam != NewDataParam)
	{
		DataParam = NewDataParam;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForDataParam();
	}
}

CDataParam CParameter::GetDataParam(const int iShortID)
{
	return Service[iShortID].DataParam;
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
		(Service[iNewService].AudioParam.iStreamID != STREAM_ID_NOT_USED))
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
		(Service[iNewService].DataParam.iStreamID != STREAM_ID_NOT_USED))
	{
		iCurSelDataService = iNewService;
		
		LastDataService.Reset();

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForDataParam();
	}
}

void CParameter::EnableMultimedia(const _BOOLEAN bFlag)
{
	if (bUsingMultimedia != bFlag)
	{
		bUsingMultimedia = bFlag;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSCDemux();
	}
}

void CParameter::SetNumOfServices(const int iNNumAuSe, const int iNNumDaSe)
{
	/* Check whether number of activated services is not greater than the
	   number of services signalled by the FAC because it can happen that
	   a false CRC check (it is only a 8 bit CRC) of the FAC block
	   initializes a wrong service */
	if (GetNumActiveServices() > iNNumAuSe + iNNumDaSe)
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

void CParameter::SetAudDataFlag(const int iServID, const CService::ETyOServ iNewADaFl)
{
	if (Service[iServID].eAudDataFlag != iNewADaFl)
	{
		Service[iServID].eAudDataFlag = iNewADaFl;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSC();
	}
}

void CParameter::SetServID(const int iServID, const uint32_t iNewServID)
{
	if (Service[iServID].iServiceID != iNewServID)
	{
		if ((iServID == 0) && (Service[0].iServiceID > 0))
			ResetServicesStreams();

		Service[iServID].iServiceID = iNewServID;

		/* Set init flags */
		if(pDRMRec) pDRMRec->InitsForMSC();


		/* If the receiver has lost the sync automatically restore 
			last current service selected */

		if ((iServID > 0) && (iNewServID > 0))
		{
			if (LastAudioService.iServiceID == iNewServID)
			{
				/* Restore last audio service selected */
				iCurSelAudioService = LastAudioService.iService;

				LastAudioService.Reset();

				/* Set init flags */
				if(pDRMRec) pDRMRec->InitsForAudParam();
			}

			if (LastDataService.iServiceID == iNewServID)
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
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	case NOT_PRESENT:  break;
	}
	PostWinMessage(ident,colour);
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

CMinMaxMean::CMinMaxMean():rSum(0.0),rCur(0.0),
rMin(numeric_limits<_REAL>::max()),rMax(numeric_limits<_REAL>::min()),iNum(0)
{
}

void
CMinMaxMean::addSample(_REAL r)
{
	rCur = r;
	rSum += r;
	iNum++;
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
