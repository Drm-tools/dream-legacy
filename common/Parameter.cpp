/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM Parameters
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

#include "Parameter.h"
#include "Version.h"

void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam);

// To be replaced by something nicer!!! TODO
#include "DrmReceiver.h"

/* Implementation *************************************************************/
void CParameter::ResetServicesStreams()
{
	int i;

	/* Store informations about last services selected
		 this for select current service automatically after a resync */

	if (iCurSelAudioService > 0)
		LastAudioService.Save(iCurSelAudioService, Service[iCurSelAudioService].iServiceID);

	if (iCurSelDataService > 0)
		LastDataService.Save(iCurSelDataService, Service[iCurSelDataService].iServiceID);


	/* Reset everything to possible start values */
	for (i = 0; i < MAX_NUM_SERVICES; i++)
	{
		Service[i].AudioParam.strTextMessage = "";
		Service[i].AudioParam.iStreamID = STREAM_ID_NOT_USED;
		Service[i].AudioParam.eAudioCoding = AC_AAC;
		Service[i].AudioParam.eSBRFlag = SB_NOT_USED;
		Service[i].AudioParam.eAudioSamplRate = AS_24KHZ;
		Service[i].AudioParam.bTextflag = FALSE;
		Service[i].AudioParam.bEnhanceFlag = FALSE;
		Service[i].AudioParam.eAudioMode = AM_MONO;
		Service[i].AudioParam.iCELPIndex = 0;
		Service[i].AudioParam.bCELPCRC = FALSE;
		Service[i].AudioParam.eHVXCRate = HR_2_KBIT;
		Service[i].AudioParam.bHVXCCRC = FALSE;

		Service[i].DataParam.iStreamID = STREAM_ID_NOT_USED;
		Service[i].DataParam.ePacketModInd = PM_PACKET_MODE;
		Service[i].DataParam.eDataUnitInd = DU_SINGLE_PACKETS;
		Service[i].DataParam.iPacketID = 0;
		Service[i].DataParam.iPacketLen = 0;
		Service[i].DataParam.eAppDomain = AD_DRM_SPEC_APP;
		Service[i].DataParam.iUserAppIdent = 0;

		Service[i].iServiceID = SERV_ID_NOT_USED;
		Service[i].eCAIndication = CA_NOT_USED;
		Service[i].iLanguage = 0;
		Service[i].strCountryCode = "";
		Service[i].eAudDataFlag = SF_AUDIO;
		Service[i].iServiceDescr = 0;
		Service[i].strLabel = "";
	}

	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		Stream[i].iLenPartA = 0;
		Stream[i].iLenPartB = 0;
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

void CParameter::GetActiveServices(CVector<int>& veciActServ)
{
	CVector<int> vecbServices(MAX_NUM_SERVICES, 0);

	/* Init return vector */
	veciActServ.Init(0);

	/* Get active services */
	int iNumServices = 0;
	for (int i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
		{
			/* A service is active, add ID to vector */
			veciActServ.Add(i);
			iNumServices++;
		}
	}
}

void CParameter::GetActiveStreams(CVector<int>& veciActStr)
{
	int				i;
	int				iNumStreams;
	CVector<int>	vecbStreams(MAX_NUM_STREAMS, 0);

	/* Determine which streams are active */
	for (i = 0; i < MAX_NUM_SERVICES; i++)
	{
		if (Service[i].IsActive())
		{
			/* Audio stream */
			if (Service[i].AudioParam.iStreamID != STREAM_ID_NOT_USED)
				vecbStreams[Service[i].AudioParam.iStreamID] = 1;

			/* Data stream */
			if (Service[i].DataParam.iStreamID != STREAM_ID_NOT_USED)
				vecbStreams[Service[i].DataParam.iStreamID] = 1;
		}
	}

	/* Now, count streams */
	iNumStreams = 0;
	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		if (vecbStreams[i] == 1)
			iNumStreams++;
	}

	/* Now that we know how many streams are active, dimension vector */
	veciActStr.Init(iNumStreams);

	/* Store IDs of active streams */
	iNumStreams = 0;
	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		if (vecbStreams[i] == 1)
		{
			veciActStr[iNumStreams] = i;
			iNumStreams++;
		}
	}
}

_REAL CParameter::GetBitRateKbps(const int iServiceID, const _BOOLEAN bAudData)
{
	/* Init lengths to zero in case the stream is not yet assigned */
	int iLen = 0;

	/* First, check if audio or data service and get lengths */
	if (Service[iServiceID].eAudDataFlag == SF_AUDIO)
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
	if (Service[iServiceID].eAudDataFlag == SF_AUDIO)
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
	MakeTable(eRobustnessMode, eSpectOccup);
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

	/* Store new value in reception log */
	ReceptLog.SetRobMode(eNewWaveMode);

	/* Apply changes only if new paramter differs from old one */
	if (eRobustnessMode != eNewWaveMode)
	{
		/* Set new value */
		eRobustnessMode = eNewWaveMode;

		/* This parameter change provokes update of table */
		MakeTable(eRobustnessMode, eSpectOccup);

		SetInitFlags(I_ROBUSTNESS_MODE);

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
		MakeTable(eRobustnessMode, eSpectOccup);

		SetInitFlags(I_SPECTRUM_OCCUPANCY);
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
		SetInitFlags(I_MSC);		
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
		SetInitFlags(I_MSC_DEMUX);		
	}
}

void CParameter::SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumDecodedBitsSDC != iNumSDCBitsPerSFrame)
	{
		iNumSDCBitsPerSFrame = iNewNumDecodedBitsSDC;
		SetInitFlags(I_SDC);		
	}
}

void CParameter::SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumBitsHieraFrTot != iNumBitsHierarchFrameTotal)
	{
		iNumBitsHierarchFrameTotal = iNewNumBitsHieraFrTot;
		SetInitFlags(I_MSC_DEMUX);		
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
		SetInitFlags(I_MSC);		

	/* Set new protection levels in reception log file */
	ReceptLog.SetProtLev(MSCPrLe);

}

void CParameter::SetAudioParam(const int iShortID,
							   const CAudioParam NewAudParam)
{
	/* Apply changes only if parameters have changed */
	if (Service[iShortID].AudioParam != NewAudParam)
	{
		Service[iShortID].AudioParam = NewAudParam;
		SetInitFlags(I_AUDIO);		
	}
}

void CParameter::SetDataParam(const int iShortID, 
										const CDataParam NewDataParam)
{
	/* Apply changes only if parameters have changed */
	if (Service[iShortID].DataParam != NewDataParam)
	{
		Service[iShortID].DataParam = NewDataParam;
		SetInitFlags(I_DATA);		
	}
}

void CParameter::SetInterleaverDepth(const ESymIntMod eNewDepth)
{
	if (eSymbolInterlMode != eNewDepth)
	{
		eSymbolInterlMode = eNewDepth;
		SetInitFlags(I_INTERLEAVER);		
	}
}

void CParameter::SetMSCCodingScheme(const ECodScheme eNewScheme)
{
	if (eMSCCodingScheme != eNewScheme)
	{
		eMSCCodingScheme = eNewScheme;
		SetInitFlags(I_MSC_CODE);		
	}

	/* Set new coding scheme in reception log */
	ReceptLog.SetMSCScheme(eNewScheme);
}

void CParameter::SetSDCCodingScheme(const ECodScheme eNewScheme)
{
	if (eSDCCodingScheme != eNewScheme)
	{
		eSDCCodingScheme = eNewScheme;
		SetInitFlags(I_SDC_CODE);		
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
		SetInitFlags(I_AUDIO);
		LastAudioService.Reset();
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
		SetInitFlags(I_DATA);
		LastDataService.Reset();
	}
}

void CParameter::EnableMultimedia(const _BOOLEAN bFlag)
{
	if (bUsingMultimedia != bFlag)
	{
		bUsingMultimedia = bFlag;
		SetInitFlags(I_MSC_DEMUX);
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
		SetInitFlags(I_MSC_DEMUX);
	}

	if ((iNumAudioService != iNNumAuSe) || (iNumDataService != iNNumDaSe))
	{
		iNumAudioService = iNNumAuSe;
		iNumDataService = iNNumDaSe;
		SetInitFlags(I_MSC_DEMUX);
	}
}

void CParameter::SetAudDataFlag(const int iServID, const ETyOServ iNewADaFl)
{
	if (Service[iServID].eAudDataFlag != iNewADaFl)
	{
		Service[iServID].eAudDataFlag = iNewADaFl;
		SetInitFlags(I_MSC);
	}
}

void CParameter::SetServID(const int iServID, const uint32_t iNewServID)
{
	if (Service[iServID].iServiceID != iNewServID)
	{
		if ((iServID == 0) && (Service[0].iServiceID > 0))
			ResetServicesStreams();

		Service[iServID].iServiceID = iNewServID;

		SetInitFlags(I_MSC);

		/* If the receiver has lost the sync automatically restore 
			last current service selected */

		if ((iServID > 0) && (iNewServID > 0))
		{
			if (LastAudioService.iServiceID == iNewServID)
			{
				/* Restore last audio service selected */
				SetCurSelAudioService(LastAudioService.iService);
			}

			if (LastDataService.iServiceID == iNewServID)
			{
				/* Restore last data service selected */
				SetCurSelDataService(LastDataService.iService);
			}
		}
	}
}


/* Implementaions for simulation -------------------------------------------- */
void CParameter::CRawSimData::Add(uint32_t iNewSRS) 
{
	/* Attention, function does not take care of overruns, data will be
	   lost if added to a filled shift register! */
	if (iCurWritePos < ciMaxDelBlocks) 
		veciShRegSt[iCurWritePos++] = iNewSRS;
}

uint32_t CParameter::CRawSimData::Get() 
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
		rAvPowPerSymbol * rAvScatPilPow * (_REAL) iNumCarrier);
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

_REAL CParameter::GetSysToNomBWCorrFact()
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

	/* Calculate system bandwidth (N / T_u) */
	const _REAL rSysBW = (_REAL) iNumCarrier /
		iFFTSizeN * SOUNDCRD_SAMPLE_RATE;

	return rSysBW / rNomBW;
}


/* push from RSCI RX_STATUS */
void CParameter::SetSignalStrength(_BOOLEAN bValid, _REAL rNewSigStr)
{
	bValidSignalStrength = bValid;
	rSigStr = rNewSigStr;
}

_BOOLEAN CParameter::GetSignalStrength(_REAL rOutSigStr)
{
	if(bValidSignalStrength)
		rOutSigStr = rSigStr;

	return bValidSignalStrength;
}

void CParameter::CReceiveStatus::SetFrameSyncStatus(const ETypeRxStatus OK)
{ 
	FSyncOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_FRAME_SYNC,colour);
}
void CParameter::CReceiveStatus::SetTimeSyncStatus(const ETypeRxStatus OK)
{ 
	TSyncOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_TIME_SYNC,colour);
}
void CParameter::CReceiveStatus::SetInterfaceStatus(const ETypeRxStatus OK)
{ 
	InterfaceOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_IOINTERFACE,colour);
}
void CParameter::CReceiveStatus::SetFACStatus(const ETypeRxStatus OK)
{ 
	FACOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_FAC_CRC,colour);
}
void CParameter::CReceiveStatus::SetSDCStatus(const ETypeRxStatus OK)
{ 
	SDCOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_SDC_CRC,colour);
}
void CParameter::CReceiveStatus::SetAudioStatus(const ETypeRxStatus OK)
{ 
	AudioOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_MSC_CRC,colour);
}
void CParameter::CReceiveStatus::SetMOTStatus(const ETypeRxStatus OK)
{
	MOTOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	}
	PostWinMessage(MS_MOT_OBJ_STAT,colour);
}

ETypeRxStatus CParameter::CReceiveStatus::GetFrameSyncStatus()
{
	return FSyncOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetTimeSyncStatus()
{
	return TSyncOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetInterfaceStatus()
{
	return InterfaceOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetFACStatus()
{
	return FACOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetSDCStatus()
{
	return SDCOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetAudioStatus()
{
	return AudioOK;
}
ETypeRxStatus CParameter::CReceiveStatus::GetMOTStatus()
{
	return MOTOK;
}

const uint32_t CParameter::I_ROBUSTNESS_MODE = 1;
const uint32_t CParameter::I_SPECTRUM_OCCUPANCY = 2;
const uint32_t CParameter::I_INTERLEAVER = 4;
const uint32_t CParameter::I_MSC_CODE = 8;
const uint32_t CParameter::I_SDC_CODE = 16;
const uint32_t CParameter::I_SDC = 32;
const uint32_t CParameter::I_MSC = 64;
const uint32_t CParameter::I_MSC_DEMUX = 128;
const uint32_t CParameter::I_AUDIO = 256;
const uint32_t CParameter::I_DATA = 512;

void CParameter::SetInitFlags(uint32_t uMask)
{
	uInitFlags |= uMask;
}

void CParameter::ClearInitFlags(uint32_t uMask)
{
	uInitFlags &= (~uMask);
}

_BOOLEAN CParameter::TestInitFlag(uint32_t uMask)
{
	return (uInitFlags&uMask)?TRUE:FALSE;
}
