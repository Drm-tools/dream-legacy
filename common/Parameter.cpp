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
extern CDRMReceiver	DRMReceiver;


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
		Service[i].strLanguageCode = "";
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

		/* Set init flags */
		DRMReceiver.InitsForWaveMode();

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

		/* Set init flags */
		DRMReceiver.InitsForSpectrumOccup();
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
		DRMReceiver.InitsForMSC();
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
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetNumDecodedBitsSDC(const int iNewNumDecodedBitsSDC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumDecodedBitsSDC != iNumSDCBitsPerSFrame)
	{
		iNumSDCBitsPerSFrame = iNewNumDecodedBitsSDC;

		/* Set init flags */
		DRMReceiver.InitsForNoDecBitsSDC();
	}
}

void CParameter::SetNumBitsHieraFrTot(const int iNewNumBitsHieraFrTot)
{
	/* Apply changes only if parameters have changed */
	if (iNewNumBitsHieraFrTot != iNumBitsHierarchFrameTotal)
	{
		iNumBitsHierarchFrameTotal = iNewNumBitsHieraFrTot;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
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
		DRMReceiver.InitsForMSC();

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

		/* Set init flags */
		DRMReceiver.InitsForAudParam();
	}
}

void CParameter::SetDataParam(const int iShortID, const CDataParam NewDataParam)
{
	/* Apply changes only if parameters have changed */
	if (Service[iShortID].DataParam != NewDataParam)
	{
		Service[iShortID].DataParam = NewDataParam;

		/* Set init flags */
		DRMReceiver.InitsForDataParam();
	}
}

void CParameter::SetInterleaverDepth(const ESymIntMod eNewDepth)
{
	if (eSymbolInterlMode != eNewDepth)
	{
		eSymbolInterlMode = eNewDepth;

		/* Set init flags */
		DRMReceiver.InitsForInterlDepth();
	}
}

void CParameter::SetMSCCodingScheme(const ECodScheme eNewScheme)
{
	if (eMSCCodingScheme != eNewScheme)
	{
		eMSCCodingScheme = eNewScheme;

		/* Set init flags */
		DRMReceiver.InitsForMSCCodSche();
	}

	/* Set new coding scheme in reception log */
	ReceptLog.SetMSCScheme(eNewScheme);
}

void CParameter::SetSDCCodingScheme(const ECodScheme eNewScheme)
{
	if (eSDCCodingScheme != eNewScheme)
	{
		eSDCCodingScheme = eNewScheme;

		/* Set init flags */
		DRMReceiver.InitsForSDCCodSche();
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
		DRMReceiver.InitsForAudParam();
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
		DRMReceiver.InitsForDataParam();
	}
}

void CParameter::EnableMultimedia(const _BOOLEAN bFlag)
{
	if (bUsingMultimedia != bFlag)
	{
		bUsingMultimedia = bFlag;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
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
		DRMReceiver.InitsForMSCDemux();
	}

	if ((iNumAudioService != iNNumAuSe) || (iNumDataService != iNNumDaSe))
	{
		iNumAudioService = iNNumAuSe;
		iNumDataService = iNNumDaSe;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetAudDataFlag(const int iServID, const ETyOServ iNewADaFl)
{
	if (Service[iServID].eAudDataFlag != iNewADaFl)
	{
		Service[iServID].eAudDataFlag = iNewADaFl;

		/* Set init flags */
		DRMReceiver.InitsForMSC();
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
		DRMReceiver.InitsForMSC();


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
				DRMReceiver.InitsForAudParam();
			}

			if (LastDataService.iServiceID == iNewServID)
			{
				/* Restore last data service selected */
				iCurSelDataService = LastDataService.iService;

				LastDataService.Reset();

				/* Set init flags */
				DRMReceiver.InitsForDataParam();
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
	const _REAL rSysBW = (_REAL) iNumCarrier /
		iFFTSizeN * SOUNDCRD_SAMPLE_RATE;

	return rSysBW / rNomBW;
}

/* Reception log implementation --------------------------------------------- */
CParameter::CReceptLog::CReceptLog() : shortlog(), longlog(), iNumAACFrames(10),
	iFrequency(0), bLogActivated(FALSE),
	bLogEnabled(FALSE), bRxlEnabled(FALSE), bPositionEnabled(FALSE),
	strAdditText(""), 
	iSecDelLogStart(0)
{
	shortlog.setLog(this);
	longlog.setLog(this);
	shortlog.reset();
	longlog.reset();
}

void CParameter::CReceptLog::WriteParameters(_BOOLEAN bLong)
{
	Mutex.Lock();
	if(bLong)
		longlog.writeParameters();
	else
		shortlog.writeParameters();
	Mutex.Unlock();
}

void CParameter::CReceptLog::ResetTransParams()
{
	/* Reset transmission parameters */
	eCurMSCScheme = CParameter::CS_3_SM;
	eCurRobMode = RM_NO_MODE_DETECTED;
	CurProtLev.iPartA = 0;
	CurProtLev.iPartB = 0;
	CurProtLev.iHierarch = 0;
}

void CParameter::CReceptLog::SetSync(const _BOOLEAN bCRCOk)
{
	if (bLogActivated == TRUE)
	{
		Mutex.Lock();

		/* If one of the syncs were wrong in one second, set to false */
		if (bCRCOk == FALSE)
			bSyncOK = FALSE;

		/* Validate sync flag */
		bSyncOKValid = TRUE;

		Mutex.Unlock();
	}
}

void CParameter::CReceptLog::SetFAC(const _BOOLEAN bCRCOk)
{
	if (bLogActivated == TRUE)
	{
		Mutex.Lock();

		if (bCRCOk == TRUE)
			iNumCRCOkFAC++;
		else
			bFACOk = FALSE;

		/* Validate FAC flag */
		bFACOkValid = TRUE;

		Mutex.Unlock();
	}
}

void CParameter::CReceptLog::SetMSC(const _BOOLEAN bCRCOk)
{
	if (bLogActivated == TRUE)
	{
		Mutex.Lock();

		/* Count for total number of MSC cells in a certain period of time */
		iNumCRCMSCLong++;

		if (bCRCOk == TRUE)
		{
			iNumCRCOkMSC++;
			iNumCRCOkMSCLong++; /* Increase number of CRCs which are ok */
		}
		else
			bMSCOk = FALSE;

		/* Validate MSC flag */
		bMSCOkValid = TRUE;

		Mutex.Unlock();
	}
}

unsigned int CParameter::CReceptLog::ExtractMinutes(double dblDeg)
{
	unsigned int Degrees;

	/* Extract degrees */
	Degrees = (unsigned int) dblDeg;
	return (unsigned int) (((floor((dblDeg - Degrees) * 1000000) / 1000000) + 0.00005) * 60.0);
}

void CParameter::CShortLog::SetSNR(const _REAL rNewCurSNR)
{
	iNumSNR++;

	/* Average SNR values */
	rSumSNR += rNewCurSNR;

	/* Set minimum and maximum of SNR */
	if (rNewCurSNR > rMaxSNR)
		rMaxSNR = rNewCurSNR;
	if (rNewCurSNR < rMinSNR)
		rMinSNR = rNewCurSNR;
}

void CParameter::CShortLog::SetSignalStrength(const _REAL rNewCurSigStr)
{
	iNumSigStr++;

	/* Average SigStr values */
	rSumSigStr += rNewCurSigStr;

	/* Set minimum and maximum of SigStr */
	if (rNewCurSigStr > rMaxSigStr)
		rMaxSigStr = rNewCurSigStr;
	if (rNewCurSigStr < rMinSigStr)
		rMinSigStr = rNewCurSigStr;
}

void CParameter::CLongLog::SetSNR(const _REAL rNewCurSNR)
{
	rCurSNR = rNewCurSNR;
}

void CParameter::CLongLog::SetSignalStrength(const _REAL rNewCurSigStr)
{
	rCurSigStr = rNewCurSigStr;
}

void CParameter::SetSNR(const _REAL rNewCurSNR)
{
	Mutex.Lock();
	rSNREstimate = rNewCurSNR;
	ReceptLog.shortlog.SetSNR(rNewCurSNR);
	ReceptLog.longlog.SetSNR(rNewCurSNR);
	Mutex.Unlock();
}

void CParameter::CReceptLog::SetNumAAC(const int iNewNum)
{
	if (iNumAACFrames != iNewNum)
	{
		/* Set the number of AAC frames in one block */
		iNumAACFrames = iNewNum;

		longlog.reset();
		shortlog.reset();
	}
}

void CParameter::CLog::open(const char* filename, time_t now)
{
	pFile = fopen(filename, "a");
	writeHeader(now);
	fflush(pFile);
	reset();
}

void CParameter::CLog::close()
{
	if (pFile != NULL)
	{
		writeTrailer();
		fclose(pFile);
		pFile=NULL;
	}
}

void CParameter::CReceptLog::StartLogging()
{
	time_t		ltime;

	/* Get time and date */
	time(&ltime);

	bLogActivated = TRUE;
	bLogEnabled = TRUE;

	Mutex.Lock();

	/* Init long and short version of log file. Open output file, write
	   header and reset log file parameters */
	/* Short */
	shortlog.open("DreamLog.txt", ltime);

	/* Long */
	longlog.open("DreamLogLong.csv", ltime);

	Mutex.Unlock();
}

void CParameter::CReceptLog::StopLogging()
{
	bLogActivated = FALSE;
	bLogEnabled = FALSE;
	/* Close both types of log files */
	shortlog.close();
	longlog.close();
}

void CParameter::CShortLog::reset()
{
	pLog->iNumCRCOkFAC = 0;
	pLog->iNumCRCOkMSC = 0;

	iNumSNR = 0;
	rSumSNR = (_REAL) 0.0;
	rMaxSNR = 0;
	rMinSNR = 1000; /* Init with high value */

	iNumSigStr = 0;
	rSumSigStr = (_REAL) 0.0;
	rMaxSigStr = 0;
	rMinSigStr = 1000; /* Init with high value */
}

void CParameter::CLongLog::reset()
{
	pLog->bSyncOK = TRUE;
	pLog->bFACOk = TRUE;
	pLog->bMSCOk = TRUE;

	/* Invalidate flags for initialization */
	pLog->bSyncOKValid = FALSE;
	pLog->bFACOkValid = FALSE;
	pLog->bMSCOkValid = FALSE;

	/* Reset total number of checked CRCs and number of CRC ok */
	pLog->iNumCRCMSCLong = 0;
	pLog->iNumCRCOkMSCLong = 0;

	rCurSNR = (_REAL) 0.0;
}

void CParameter::CShortLog::writeHeader(time_t now)
{
	struct tm*	today;
	today = gmtime(&now); /* Should be UTC time */

	if (pFile != NULL)
	{
		/* Beginning of new table (similar to standard DRM log file) */
		fprintf(pFile, "\n>>>>\nDream\nSoftware Version %s\n", dream_version);

		fprintf(pFile, "Starttime (UTC)  %d-%02d-%02d %02d:%02d:%02d\n",
			today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
			today->tm_hour, today->tm_min, today->tm_sec);

		fprintf(pFile, "Frequency        ");
		if (pLog->iFrequency != 0)
			fprintf(pFile, "%d kHz\n", pLog->iFrequency);
		else
			fprintf(pFile, "\n");
			
		if(pLog->GPSData.GetPositionAvailable())
		{
			double latitude, longitude;
			pLog->GPSData.GetLatLongDegrees(latitude, longitude);

			char c;
			double val;
			if(latitude<0.0)
			{
				c='S';
				val = - latitude;
			}
			else
			{
				c='N';
				val = latitude;
			}
			fprintf(pFile, "Latitude          %2d\xb0%02d'%c\n", int(val), pLog->ExtractMinutes(val), c);

			if(longitude<0.0)
			{
				c='W';
				val = - longitude;
			}
			else
			{
				c='E';
				val = longitude;
			}
			fprintf(pFile, "Longitude        %3d\xb0%02d'%c\n", int(val), pLog->ExtractMinutes(val), c);
		}

		/* Write additional text */
		if (pLog->strAdditText != "")
			fprintf(pFile, "%s\n\n", pLog->strAdditText.c_str());
		else
			fprintf(pFile, "\n");

		fprintf(pFile, "MINUTE  SNR     SYNC    AUDIO     TYPE");
		if(pLog->bRxlEnabled)
			fprintf(pFile, "     RXL\n");
		fprintf(pFile, "\n");
	}
	iTimeCntShort = 0;
}

void CParameter::CLongLog::writeHeader(time_t)
{
	if (pFile != NULL)
	{
#ifdef _DEBUG_
		/* In case of debug mode, use more paramters */
		fprintf(pFile, "FREQ/MODE/QAM PL:ABH,       DATE,       TIME,    "
			"SNR, SYNC, FAC, MSC, AUDIO, AUDIOOK, DOPPLER, DELAY,  "
			"DC-FREQ, SAMRATEOFFS\n");
#else
		/* The long version of log file has different header */
		fprintf(pFile, "FREQ/MODE/QAM PL:ABH,       DATE,       TIME,    "
			"SNR, SYNC, FAC, MSC, AUDIO, AUDIOOK, DOPPLER, DELAY");
		if(pLog->bRxlEnabled)
			fprintf(pFile, ",     RXL");
		if(pLog->bPositionEnabled)
			fprintf(pFile, ",     LATITUDE,    LONGITUDE");
		fprintf(pFile, "\n");
#endif
	}

	/* Init time with current time. The time function returns the number of
	   seconds elapsed since midnight (00:00:00), January 1, 1970,
	   coordinated universal time, according to the system clock */
	time(&TimeCntLong);
}

void CParameter::CShortLog::writeTrailer()
{
	if (rMaxSNR > rMinSNR)
		fprintf(pFile, "\nSNR min: %4.1f, max: %4.1f\n", rMinSNR, rMaxSNR);
	else
		fprintf(pFile, "\nSNR min: %4.1f, max: %4.1f\n", 0.0, 0.0);

	if(pLog->bRxlEnabled)
	{
		if (rMaxSigStr > rMinSigStr)
			fprintf(pFile, "\nRXL min: %4.1f, max: %4.1f\n", rMinSigStr, rMaxSigStr);
		else
			fprintf(pFile, "\nRXL min: %4.1f, max: %4.1f\n", 0.0, 0.0);
	}

	/* Short log file ending */
	fprintf(pFile, "\nCRC: \n");
	fprintf(pFile, "<<<<\n\n");
}

void CParameter::CLongLog::writeTrailer()
{
	fprintf(pFile, "\n\n");
}

void CParameter::CShortLog::writeParameters()
{
	if (pLog->bLogActivated == FALSE)
		return;

	int iAverageSNR, iAverageSigStr, iTmpNumAAC;

	/* Avoid division by zero */
	if (iNumSNR > 0)
		iAverageSNR = (int) Round(rSumSNR / iNumSNR);
	else
		iAverageSNR = 0;

	/* Avoid division by zero */
	if (iNumSigStr > 0)
		iAverageSigStr = (int) Round(rSumSigStr / iNumSigStr);
	else
		iAverageSigStr = 0;

	/* If no sync, do not print number of AAC frames. If the number
	   of correct FAC CRCs is lower than 10%, we assume that
	   receiver is not synchronized */
	if (pLog->iNumCRCOkFAC < 15)
		iTmpNumAAC = 0;
	else
		iTmpNumAAC = pLog->iNumAACFrames;

	try
	{
		fprintf(pFile, "  %04d   %2d      %3d  %4d/%02d        0",
		iTimeCntShort, iAverageSNR, pLog->iNumCRCOkFAC, pLog->iNumCRCOkMSC, iTmpNumAAC);
		if(pLog->bRxlEnabled)
			fprintf(pFile, "      %02d", iAverageSigStr);
		fprintf(pFile, "\n");
		fflush(pFile);
	}
	catch (...)
	{
		/* To prevent errors if user views the file during reception */
	}

	reset();

	iTimeCntShort++;

}

void CParameter::CLongLog::writeParameters()
{
	if (pLog->bLogActivated == FALSE)
		return;

	int	iSyncInd, iFACInd, iMSCInd;

	if ((pLog->bSyncOK == TRUE) && (pLog->bSyncOKValid == TRUE))
		iSyncInd = 1;
	else
		iSyncInd = 0;

	if ((pLog->bFACOk == TRUE) && (pLog->bFACOkValid == TRUE))
		iFACInd = 1;
	else
		iFACInd = 0;

	if ((pLog->bMSCOk == TRUE) && (pLog->bMSCOkValid == TRUE))
		iMSCInd = 1;
	else
		iMSCInd = 0;

	struct tm* TimeNow = gmtime(&TimeCntLong); /* UTC */

	/* Get parameters for delay and Doppler. In case the receiver is
	   not synchronized, set parameters to zero */
	_REAL rDelay = (_REAL) 0.0;
	_REAL rDoppler = (_REAL) 0.0;
	if (DRMReceiver.GetReceiverState() == AS_WITH_SIGNAL)
	{
		rDelay = DRMReceiver.GetParameters()->rMinDelay;
		rDoppler = DRMReceiver.GetParameters()->rSigmaEstimate;
	}

	/* Get robustness mode string */
	char chRobMode='X';
	switch (pLog->eCurRobMode)
	{
	case RM_ROBUSTNESS_MODE_A:
		chRobMode = 'A';
		break;

	case RM_ROBUSTNESS_MODE_B:
		chRobMode = 'B';
		break;

	case RM_ROBUSTNESS_MODE_C:
		chRobMode = 'C';
		break;

	case RM_ROBUSTNESS_MODE_D:
		chRobMode = 'D';
		break;

	case RM_NO_MODE_DETECTED:
		chRobMode = 'X';
		break;
	}

	/* Get MSC scheme */
	int iCurMSCSc=-1;
	switch (pLog->eCurMSCScheme)
	{
	case CParameter::CS_3_SM:
		iCurMSCSc = 0;
		break;

	case CParameter::CS_3_HMMIX:
		iCurMSCSc = 1;
		break;

	case CParameter::CS_3_HMSYM:
		iCurMSCSc = 2;
		break;

	case CParameter::CS_2_SM:
		iCurMSCSc = 3;
		break;

	case CParameter::CS_1_SM:/* TODO */
		break;
	}

	/* Copy protection levels */
	int iCurProtLevPartA = pLog->CurProtLev.iPartA;
	int iCurProtLevPartB = pLog->CurProtLev.iPartB;
	int iCurProtLevPartH = pLog->CurProtLev.iHierarch;

	/* Only show mode if FAC CRC was ok */
	if (iFACInd == 0)
	{
		chRobMode = 'X';
		iCurMSCSc = 0;
		iCurProtLevPartA = 0;
		iCurProtLevPartB = 0;
		iCurProtLevPartH = 0;
	}

	try
	{
#ifdef _DEBUG_
		/* Some more parameters in debug mode */
		fprintf(pFile,
			" %5d/%c%d%d%d%d        , %d-%02d-%02d, %02d:%02d:%02d.0, "
			"%6.2f,    %1d,   %1d,   %1d,   %3d,     %3d,   %5.2f, "
			"%5.2f, %8.2f,       %5.2f\n",
			pLog->iFrequency,	chRobMode, iCurMSCSc, iCurProtLevPartA,
			iCurProtLevPartB, iCurProtLevPartH,
			TimeNow->tm_year + 1900, TimeNow->tm_mon + 1,
			TimeNow->tm_mday, TimeNow->tm_hour, TimeNow->tm_min,
			TimeNow->tm_sec, rCurSNR, iSyncInd, iFACInd, iMSCInd,
			iNumCRCMSCLong, iNumCRCOkMSCLong, rDoppler, rDelay,
			pLog->DRMReceiver.GetParameters()->GetDCFrequency(),
			pLog->DRMReceiver.GetParameters()->GetSampFreqEst()
			);
#else
		/* This data can be read by Microsoft Excel */
		fprintf(pFile,
			" %5d/%c%d%d%d%d        , %d-%02d-%02d, %02d:%02d:%02d.0, "
			"%6.2f,    %1d,   %1d,   %1d,   %3d,     %3d,   %5.2f, %5.2f",
			pLog->iFrequency,	chRobMode, iCurMSCSc, iCurProtLevPartA,
			iCurProtLevPartB, iCurProtLevPartH,
			TimeNow->tm_year + 1900, TimeNow->tm_mon + 1,
			TimeNow->tm_mday, TimeNow->tm_hour, TimeNow->tm_min,
			TimeNow->tm_sec, rCurSNR, iSyncInd, iFACInd, iMSCInd,
			pLog->iNumCRCMSCLong, pLog->iNumCRCOkMSCLong,
			rDoppler, rDelay);
		if(pLog->bRxlEnabled)
			fprintf(pFile, ",   %5.2f", rCurSigStr);
		if(pLog->bPositionEnabled)
		{
			double latitude, longitude;
			pLog->GPSData.GetLatLongDegrees(latitude, longitude);
			fprintf(pFile, ",   %10.6f,   %10.6f", latitude, longitude);
		}
		fprintf(pFile, "\n");
#endif
		fflush(pFile);
	}

	catch (...)
	{
		/* To prevent errors if user views the file during reception */
	}

	reset();
	/* This is a time_t type variable. It contains the number of
	   seconds from a certain defined date. We simply increment
	   this number for the next second instance */
	TimeCntLong++;
}

ERecMode CParameter::GetReceiverMode()
{
 return DRMReceiver.GetReceiverMode();		 
}

EAcqStat CParameter::GetReceiverState()
{
	return DRMReceiver.GetReceiverState();
}

void CParameter::SetIFSignalLevel(_REAL rNewSigStr)
{
	Mutex.Lock();
	ReceptLog.rIFSigStr = rNewSigStr;
	Mutex.Unlock();
}

_REAL CParameter::GetIFSignalLevel()
{
	_REAL r;
	Mutex.Lock();
	r = ReceptLog.rIFSigStr;
	Mutex.Unlock();
	return r;
}

void CParameter::SetSignalStrength(_BOOLEAN bValid, _REAL rNewSigStr)
{
	Mutex.Lock();
	ReceptLog.bValidSignalStrength = bValid;
	ReceptLog.rSigStr = rNewSigStr;
	ReceptLog.shortlog.SetSignalStrength(rNewSigStr);
	ReceptLog.longlog.SetSignalStrength(rNewSigStr);
	Mutex.Unlock();
}

_BOOLEAN CParameter::GetSignalStrength(_REAL &rOutSigStr)
{
	_BOOLEAN bValid;
	Mutex.Lock();
	bValid = ReceptLog.bValidSignalStrength;
	if(bValid)
		rOutSigStr = ReceptLog.rSigStr;
	Mutex.Unlock();
	return bValid;
}

void CParameter::CReceiveStatus::SetFrameSyncStatus(const ETypeRxStatus OK)
{ 
	FSyncOK = OK;
	int colour=2;
	switch(OK) {
	case CRC_ERROR: colour=2; break;
	case DATA_ERROR: colour=1; break;
	case RX_OK: colour=0; break;
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
	case NOT_PRESENT:  break;
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
