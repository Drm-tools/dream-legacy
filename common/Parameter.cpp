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


// To be replaced by something nicer!!! TODO
#include "DrmReceiver.h"
extern CDRMReceiver	DRMReceiver;


/* Implementation *************************************************************/
void CParameter::ResetServicesStreams()
{
	int i;

	/* Reset everything to possible start values */
	for (i = 0; i < MAX_NO_SERVICES; i++)
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
		Service[i].DataParam.ePacketModInd = PM_SYNCHRON_STR_MODE;
		Service[i].DataParam.eDataUnitInd = DU_SINGLE_PACKETS;
		Service[i].DataParam.iPacketID = 0;
		Service[i].DataParam.iPacketLen = 0;
		Service[i].DataParam.eAppDomain = AD_DRM_SPEC_APP;

		Service[i].iServiceID = SERV_ID_NOT_USED;
		Service[i].eCAIndication = CA_USED;
		Service[i].iLanguage = 0;
		Service[i].eAudDataFlag = SF_AUDIO;
		Service[i].iServiceDescr = 0;
		Service[i].strLabel = "";
	}

	for (i = 0; i < MAX_NO_STREAMS; i++)
	{
		Stream[i].iLenPartA = 0;
		Stream[i].iLenPartB = 0;
	}
}

void CParameter::GetActiveStreams(CVector<int>& veciActStr)
{
	int					i;
	int					iNoStreams;
	CVector<int>		vecbStreams(MAX_NO_STREAMS, 0);

	/* Determine which streams are active */
	for (i = 0; i < MAX_NO_SERVICES; i++)
	{
		if (Service[i].iServiceID != SERV_ID_NOT_USED)
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
	iNoStreams = 0;
	for (i = 0; i < MAX_NO_STREAMS; i++)
		if (vecbStreams[i] == 1)
			iNoStreams++;

	/* Now that we know how many streams are active, dimension vector */
	veciActStr.Init(iNoStreams);

	/* Store IDs of active streams */
	iNoStreams = 0;
	for (i = 0; i < MAX_NO_STREAMS; i++)
	{
		if (vecbStreams[i] == 1)
		{
			veciActStr[iNoStreams] = i;
			iNoStreams++;
		}
	}
}

void CParameter::InitCellMapTable(const ERobMode eNewWaveMode, const ESpecOcc eNewSpecOcc)
{
	/* Set new values and make table */
	eRobustnessMode = eNewWaveMode;
	eSpectOccup = eNewSpecOcc;
	MakeTable(eRobustnessMode, eSpectOccup);


// Should be done but is no good for simulation, TODO
///* Set init flags */
//DRMReceiver.InitsForAllModules();
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

void CParameter::SetStreamLenPartA(const int iStreamNo, const int iNewLenPartA)
{
	/* Apply changes only if parameters have changed */
	if (Stream[iStreamNo].iLenPartA != iNewLenPartA)
	{
		Stream[iStreamNo].iLenPartA = iNewLenPartA;

		/* Set init flags */
		DRMReceiver.InitsForMSC();
	}
}

void CParameter::SetStreamLenPartB(const int iStreamNo, const int iNewLenPartB)
{
	/* Apply changes only if parameters have changed */
	if (Stream[iStreamNo].iLenPartB != iNewLenPartB)
	{
		Stream[iStreamNo].iLenPartB = iNewLenPartB;

		/* Set init flags */
		DRMReceiver.InitsForMSC();
	}
}

void CParameter::SetNoDecodedBitsMSC(const int iNewNoDecodedBitsMSC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNoDecodedBitsMSC != iNoDecodedBitsMSC)
	{
		iNoDecodedBitsMSC = iNewNoDecodedBitsMSC;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetNoDecodedBitsSDC(const int iNewNoDecodedBitsSDC)
{
	/* Apply changes only if parameters have changed */
	if (iNewNoDecodedBitsSDC != iNoSDCBitsPerSFrame)
	{
		iNoSDCBitsPerSFrame = iNewNoDecodedBitsSDC;

		/* Set init flags */
		DRMReceiver.InitsForNoDecBitsSDC();
	}
}

void CParameter::SetMSCProtLev(const CMSCProtLev NewMSCPrLe, const _BOOLEAN bWithHierarch)
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
}

void CParameter::SetAudioParam(const int iShortID, const CAudioParam NewAudParam)
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
	if (iCurSelAudioService != iNewService)
	{
		iCurSelAudioService = iNewService;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetCurSelDataService(const int iNewService)
{
	if (iCurSelDataService != iNewService)
	{
		iCurSelDataService = iNewService;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetNoAudioServ(const int iNewNoAuSe)
{
	if (iNoAudioService != iNewNoAuSe)
	{
		iNoAudioService = iNewNoAuSe;

		/* Set init flags */
		DRMReceiver.InitsForMSCDemux();
	}
}

void CParameter::SetNoDataServ(const int iNewNoDaSe)
{
	if (iNoDataService != iNewNoDaSe)
	{
		iNoDataService = iNewNoDaSe;

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

void CParameter::SetServID(const int iServID, const _UINT32BIT iNewServID)
{
	if (Service[iServID].iServiceID != iNewServID)
	{
		Service[iServID].iServiceID = iNewServID;

		/* Set init flags */
		DRMReceiver.InitsForMSC();
	}
}


/* Implementaions for simulation -------------------------------------------- */
void CParameter::CRawSimData::Add(_UINT32BIT iNewSRS) 
{
	/* Attention, function does not take care of overruns, data will be
	   lost if added to a filled shift register! */
	if (iCurWritePos < ciMaxDelBlocks) 
		veciShRegSt[iCurWritePos++] = iNewSRS;
}

_UINT32BIT CParameter::CRawSimData::Get() 
{
	/* We always use the first value of the array for reading and do a
	   shift of the other data by adding a arbitrary value (0) at the
	   end of the whole shift register */
	_UINT32BIT iRet = veciShRegSt[0];
	veciShRegSt.AddEnd(0);
	iCurWritePos--;

	return iRet;
}


/* Reception log implementation --------------------------------------------- */
CParameter::CReceptLog::CReceptLog() : iNumAACFrames(10), pFile(NULL)
{
	ResetLog();
}

void CParameter::CReceptLog::ResetLog()
{
	iNumCRCOkFAC = 0;
	iNumCRCOkMSC = 0;
	iNumSNR = 0;
	rAvSNR = (_REAL) 0.0;
}

void CParameter::CReceptLog::SetFAC(_BOOLEAN bCRCOk)
{
	if (bLogActivated == TRUE)
		if (bCRCOk == TRUE)
			iNumCRCOkFAC++;
}

void CParameter::CReceptLog::SetMSC(_BOOLEAN bCRCOk)
{
	if (bLogActivated == TRUE)
		if (bCRCOk == TRUE)
			iNumCRCOkMSC++;
}

void CParameter::CReceptLog::SetSNR(_REAL rCurSNR)
{
	if (bLogActivated == TRUE)
	{
		iNumSNR++;

		/* Average SNR values */
		rAvSNR += rCurSNR;
	}
}

void CParameter::CReceptLog::SetNumAAC(int iNewNum)
{
	/* Set the number of AAC frames in one block */
	iNumAACFrames = iNewNum;

	ResetLog();
}

void CParameter::CReceptLog::SetLog(_BOOLEAN bLog)
{
	time_t		ltime;
	char		tmpbuf[128];
	struct tm*	today;

	bLogActivated = bLog;

	/* Open or close the file */
	if (bLogActivated == TRUE)
	{
		/* Get time and date */
		time(&ltime);
		today = gmtime(&ltime); /* Should be UTC time */

		pFile = fopen("DreamLog.txt", "a");

		/* Beginning of new table (similar to standard DRM log file) */
		fprintf(pFile, "\n>>>>\nDream\nSoftware Version %s\n",
			DREAM_VERSION_NUMBER);

		fprintf(pFile, "Starttime (UTC)  %d-%02d-%02d %02d:%02d:%02d\n",
			today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
			today->tm_hour, today->tm_min, today->tm_sec);

		fprintf(pFile, "Frequency        \nLatitude         \nLongitude        \n\n");

		fprintf(pFile, "MINUTE  SNR     SYNC    AUDIO     TYPE\n");
		fflush(pFile);

		ResetLog();

		/* Reset minute count */
		iMinuteCnt = 0;
	}
	else
		CloseFile();
}

void CParameter::CReceptLog::CloseFile()
{
	if (pFile != NULL)
	{
		fprintf(pFile, "\nCRC: \n<<<<\n\n");

		fclose(pFile);

		pFile = NULL;
	}
}

void CParameter::CReceptLog::WriteParameters()
{
	int iAverageSNR;
	int iTmpNumAAC;

	try
	{
		if (bLogActivated == TRUE)
		{
			/* Avoid division by zero */
			if (iNumSNR == 0)
				iAverageSNR = 0;
			else
				iAverageSNR = (int) (rAvSNR / iNumSNR + (_REAL) 0.5); /* Round */

			/* If no sync, do not print number of AAC frames. If the number of
			   correct FAC CRCs is lower than 10%, we assume that receiver is
			   not synchronized */
			if (iNumCRCOkFAC < 15)
				iTmpNumAAC = 0;
			else
				iTmpNumAAC = iNumAACFrames;

			fprintf(pFile, "  %04d   %2d      %3d  %4d/%02d        0\n",
				iMinuteCnt, iAverageSNR, iNumCRCOkFAC,
				iNumCRCOkMSC, iTmpNumAAC);
			fflush(pFile);
		}

		ResetLog();
		iMinuteCnt++;
	}

	catch (...)
	{
		/* To prevent errors if user views the file during reception */
	}
}
