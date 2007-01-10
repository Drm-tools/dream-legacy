/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Reception Log
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

#include "ReceptionLog.h"
#include "Version.h"
#include "matlib/MatlibStdToolbox.h"
#include <time.h>

/*  implementation --------------------------------------------- */
CReceptLog::CReceptLog() : iNumAACFrames(10), pFileLong(NULL),
	pFileShort(NULL), iFrequency(0), strAdditText(""),
	strLatitude(""), strLongitude(""), 
	bLogEnabled(FALSE), bLogActivated(FALSE), iSecDelLogStart(0)
{
	ResetLog(TRUE);
	ResetLog(FALSE);
}

void CReceptLog::ResetLog(const _BOOLEAN bIsLong)
{
	if (bIsLong == TRUE)
	{
		bSyncOK = TRUE;
		bFACOk = TRUE;
		bMSCOk = TRUE;

		/* Invalidate flags for initialization */
		bSyncOKValid = FALSE;
		bFACOkValid = FALSE;
		bMSCOkValid = FALSE;

		/* Reset total number of checked CRCs and number of CRC ok */
		iNumCRCMSCLong = 0;
		iNumCRCOkMSCLong = 0;

		rCurSNR = (_REAL) 0.0;
	}
	else
	{
		iNumCRCOkFAC = 0;
		iNumCRCOkMSC = 0;
		iNumSNR = 0;
		rAvSNR = (_REAL) 0.0;
	}
}

void CReceptLog::ResetTransParams()
{
	/* Reset transmission parameters */
	eCurMSCScheme = CS_3_SM;
	eCurRobMode = RM_NO_MODE_DETECTED;
	CurProtLev.iPartA = 0;
	CurProtLev.iPartB = 0;
	CurProtLev.iHierarch = 0;
}

void CReceptLog::SetSync(const _BOOLEAN bCRCOk)
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

void CReceptLog::SetFAC(const _BOOLEAN bCRCOk)
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

void CReceptLog::SetMSC(const _BOOLEAN bCRCOk)
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

void CReceptLog::SetSNR(const _REAL rNewCurSNR)
{
	if (bLogActivated == TRUE)
	{
		Mutex.Lock();

		/* Set parameter for long log file version */
		rCurSNR = rNewCurSNR;

		iNumSNR++;

		/* Average SNR values */
		rAvSNR += rNewCurSNR;

		/* Set minimum and maximum of SNR */
		if (rNewCurSNR > rMaxSNR)
			rMaxSNR = rNewCurSNR;
		if (rNewCurSNR < rMinSNR)
			rMinSNR = rNewCurSNR;

		Mutex.Unlock();
	}
}

void CReceptLog::SetNumAAC(const int iNewNum)
{
	if (iNumAACFrames != iNewNum)
	{
		/* Set the number of AAC frames in one block */
		iNumAACFrames = iNewNum;

		ResetLog(TRUE);
		ResetLog(FALSE);
	}
}

void CReceptLog::StartLogging()
{
	bLogActivated = TRUE;
	bLogEnabled = TRUE;

	Mutex.Lock();

	/* Init long and short version of log file. Open output file, write
	   header and reset log file parameters */
	/* Short */
	pFileShort = fopen("DreamLog.txt", "a");
	SetLogHeader(pFileShort, FALSE);
	ResetLog(FALSE);
	iTimeCntShort = 0;

	/* Long */
	pFileLong = fopen("DreamLogLong.csv", "a");
	SetLogHeader(pFileLong, TRUE);
	ResetLog(TRUE);

	/* Init time with current time. The time function returns the number of
	   seconds elapsed since midnight (00:00:00), January 1, 1970,
	   coordinated universal time, according to the system clock */
	time(&TimeCntLong);

	/* Init maximum and mininum value of SNR */
	rMaxSNR = 0;
	rMinSNR = 1000; /* Init with high value */

	Mutex.Unlock();
}

void CReceptLog::StopLogging()
{
	bLogActivated = FALSE;
	bLogEnabled = FALSE;
	/* Close both types of log files */
	CloseFile(pFileLong, TRUE);
	CloseFile(pFileShort, FALSE);
}


void CReceptLog::SetLogHeader(FILE* pFile, const _BOOLEAN bIsLong)
{
	time_t		ltime;
	struct tm*	today;

	/* Get time and date */
	time(&ltime);
	today = gmtime(&ltime); /* Should be UTC time */

	if (pFile != NULL)
	{
		if (bIsLong != TRUE)
		{
			/* Beginning of new table (similar to standard DRM log file) */
			fprintf(pFile, "\n>>>>\nDream\nSoftware Version %s\n", dream_version);

			fprintf(pFile, "Starttime (UTC)  %d-%02d-%02d %02d:%02d:%02d\n",
				today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
				today->tm_hour, today->tm_min, today->tm_sec);

			fprintf(pFile, "Frequency        ");
			if (iFrequency != 0)
				fprintf(pFile, "%d kHz", iFrequency);
			
			fprintf(pFile, "\nLatitude         %7s", strLatitude.c_str());
			fprintf(pFile, "\nLongitude        %7s", strLongitude.c_str());

			/* Write additional text */
			if (strAdditText != "")
				fprintf(pFile, "\n%s\n\n", strAdditText.c_str());
			else
				fprintf(pFile, "\n\n");

			fprintf(pFile, "MINUTE  SNR     SYNC    AUDIO     TYPE\n");
		}
		else
		{
#ifdef _DEBUG_
			/* In case of debug mode, use more paramters */
			fprintf(pFile, "FREQ/MODE/QAM PL:ABH,       DATE,       TIME,    "
				"SNR, SYNC, FAC, MSC, AUDIO, AUDIOOK, DOPPLER, DELAY,  "
				"DC-FREQ, SAMRATEOFFS\n");
#else
			/* The long version of log file has different header */
			fprintf(pFile, "FREQ/MODE/QAM PL:ABH,       DATE,       TIME,    "
				"SNR, SYNC, FAC, MSC, AUDIO, AUDIOOK, DOPPLER, DELAY\n");
#endif
		}

		fflush(pFile);
	}
}

void CReceptLog::CloseFile(FILE* pFile, const _BOOLEAN bIsLong)
{
	if (pFile != NULL)
	{
		if (bIsLong == TRUE)
		{
			/* Long log file ending */
			fprintf(pFile, "\n\n");
		}
		else
		{
			/* Set min and max values of SNR. Check values first */
			if (rMaxSNR < rMinSNR)
			{
				/* It seems that no SNR value was set, set both max and min
				   to 0 */
				rMaxSNR = 0;
				rMinSNR = 0;
			}
			fprintf(pFile, "\nSNR min: %4.1f, max: %4.1f\n", rMinSNR, rMaxSNR);

			/* Short log file ending */
			fprintf(pFile, "\nCRC: \n");
			fprintf(pFile, "<<<<\n\n");
		}

		fclose(pFile);

		pFile = NULL;
	}
}

void CReceptLog::WriteParameters(const _BOOLEAN bIsLong)
{
	try
	{
		if (bLogActivated == TRUE)
		{
			Mutex.Lock();

			if (bIsLong == TRUE)
			{
				/* Log LONG ------------------------------------------------- */
				int			iSyncInd, iFACInd, iMSCInd;
				struct tm*	TimeNow;

				if ((bSyncOK == TRUE) && (bSyncOKValid == TRUE))
					iSyncInd = 1;
				else
					iSyncInd = 0;

				if ((bFACOk == TRUE) && (bFACOkValid == TRUE))
					iFACInd = 1;
				else
					iFACInd = 0;

				if ((bMSCOk == TRUE) && (bMSCOkValid == TRUE))
					iMSCInd = 1;
				else
					iMSCInd = 0;

				TimeNow = gmtime(&TimeCntLong); /* Should be UTC time */

				/* Get robustness mode string */
				char chRobMode;
				switch (eCurRobMode)
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
				int iCurMSCSc;
				switch (eCurMSCScheme)
				{
				case CS_3_SM:
					iCurMSCSc = 0;
					break;

				case CS_3_HMMIX:
					iCurMSCSc = 1;
					break;

				case CS_3_HMSYM:
					iCurMSCSc = 2;
					break;

				case CS_2_SM:
					iCurMSCSc = 3;
					break;
				}

				/* Copy protection levels */
				int iCurProtLevPartA = CurProtLev.iPartA;
				int iCurProtLevPartB = CurProtLev.iPartB;
				int iCurProtLevPartH = CurProtLev.iHierarch;

				/* Only show mode if FAC CRC was ok */
				if (iFACInd == 0)
				{
					chRobMode = 'X';
					iCurMSCSc = 0;
					iCurProtLevPartA = 0;
					iCurProtLevPartB = 0;
					iCurProtLevPartH = 0;
				}

#ifdef _DEBUG_
				/* Some more parameters in debug mode */
				fprintf(pFileLong,
					" %5d/%c%d%d%d%d        , %d-%02d-%02d, %02d:%02d:%02d.0, "
					"%6.2f,    %1d,   %1d,   %1d,   %3d,     %3d,   %5.2f, "
					"%5.2f, %8.2f,       %5.2f\n",
					iFrequency,	chRobMode, iCurMSCSc, iCurProtLevPartA,
					iCurProtLevPartB, iCurProtLevPartH,
					TimeNow->tm_year + 1900, TimeNow->tm_mon + 1,
					TimeNow->tm_mday, TimeNow->tm_hour, TimeNow->tm_min,
					TimeNow->tm_sec, rCurSNR, iSyncInd, iFACInd, iMSCInd,
					iNumCRCMSCLong, iNumCRCOkMSCLong,
					rDoppler, rDelay,
					GetDCFrequency(),
					GetSampFreqEst());
#else
				/* This data can be read by Microsoft Excel */
				fprintf(pFileLong,
					" %5d/%c%d%d%d%d        , %d-%02d-%02d, %02d:%02d:%02d.0, "
					"%6.2f,    %1d,   %1d,   %1d,   %3d,     %3d,   %5.2f, "
					"%5.2f\n",
					iFrequency,	chRobMode, iCurMSCSc, iCurProtLevPartA,
					iCurProtLevPartB, iCurProtLevPartH,
					TimeNow->tm_year + 1900, TimeNow->tm_mon + 1,
					TimeNow->tm_mday, TimeNow->tm_hour, TimeNow->tm_min,
					TimeNow->tm_sec, rCurSNR, iSyncInd, iFACInd, iMSCInd,
					iNumCRCMSCLong, iNumCRCOkMSCLong,
					rDoppler, rDelay);
#endif
			}
			else
			{
				/* Log SHORT ------------------------------------------------ */ 
				int iAverageSNR, iTmpNumAAC;

				/* Avoid division by zero */
				if (iNumSNR == 0)
					iAverageSNR = 0;
				else
					iAverageSNR = (int) Round(rAvSNR / iNumSNR);

				/* If no sync, do not print number of AAC frames. If the number
				   of correct FAC CRCs is lower than 10%, we assume that
				   receiver is not synchronized */
				if (iNumCRCOkFAC < 15)
					iTmpNumAAC = 0;
				else
					iTmpNumAAC = iNumAACFrames;

				fprintf(pFileShort, "  %04d   %2d      %3d  %4d/%02d        0",
					iTimeCntShort, iAverageSNR, iNumCRCOkFAC,
					iNumCRCOkMSC, iTmpNumAAC);

				fprintf(pFileShort, "\n"); /* New line */
			}

			fflush(pFileLong);
			fflush(pFileShort);

			ResetLog(bIsLong);

			if (bIsLong == TRUE)
			{
				/* This is a time_t type variable. It contains the number of
				   seconds from a certain defined date. We simply increment
				   this number for the next second instance */
				TimeCntLong++;
			}
			else
				iTimeCntShort++;

			Mutex.Unlock();
		}
	}

	catch (...)
	{
		/* To prevent errors if user views the file during reception */
	}
}
