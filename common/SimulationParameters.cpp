/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM simulation script
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

#include "DrmSimulation.h"


/* Implementation *************************************************************/
void CDRMSimulation::SimScript()
{
	int				i;
	_REAL			rStartSNR, rEndSNR, rStepSNR;
	_REAL			rSNRCnt;
	FILE*			pFileSimRes;
	CVector<_REAL>	vecrMSE;
	string			strSimFile;
	string			strSpecialRemark;


	/**************************************************************************\
	* Simulation settings vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*
	\**************************************************************************/
	/* Choose which type of simulation, if you choose "ST_NONE", the regular
	   application will be started */
	Param.eSimType = CParameter::ST_BITERROR;
	Param.eSimType = CParameter::ST_MSECHANEST;
	Param.eSimType = CParameter::ST_SYNC_PARAM; /* Check "SetSyncInput()"s! */
	Param.eSimType = CParameter::ST_BER_IDEALCHAN;
	Param.eSimType = CParameter::ST_NONE; /* No simulation, regular GUI */

	if (Param.eSimType != CParameter::ST_NONE)
	{
		Param.iDRMChannelNum = 5;

		rStartSNR = (_REAL) 20.0;
		rEndSNR = (_REAL) 20.0;
		rStepSNR = (_REAL) 0.5;
		strSpecialRemark = "test";

		/* Length of simulation */
//		iSimTime = 200;
		iSimNumErrors = 1000000;


		if (Param.iDRMChannelNum < 3)
		{
			Param.InitCellMapTable(RM_ROBUSTNESS_MODE_A, SO_2);
			Param.eSymbolInterlMode = CParameter::SI_SHORT;
		}
		else
		{
			Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);
			Param.eSymbolInterlMode = CParameter::SI_LONG;
		}

		/* The associated code rate is R = 0,6 and the modulation is 64-QAM */
		Param.MSCPrLe.iPartB = 1;
		Param.eMSCCodingScheme = CParameter::CS_3_SM;


ChannelEstimation.SetFreqInt(CChannelEstimation::FWIENER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FDFTFILTER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FLINEAR);

ChannelEstimation.SetTimeInt(CChannelEstimation::TWIENER);
//ChannelEstimation.SetTimeInt(CChannelEstimation::TLINEAR);


		/* Init the modules to adapt to the new parameters. We need to do that
		   because the following routines call modul internal functions which
		   need correctly initialized modules */
		Init();

		MSCMLCDecoder.SetNumIterations(1);

		/* Define which synchronization algorithms we want to use */
		/* In case of bit error simulations, a synchronized DRM data stream is
		   used. Set corresponding modules to synchronized mode */
		InputResample.SetSyncInput(TRUE);
		FreqSyncAcq.SetSyncInput(TRUE);
		SyncUsingPil.SetSyncInput(TRUE);
		TimeSync.SetSyncInput(TRUE);
	/**************************************************************************\
	* Simulation settings ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*
	\**************************************************************************/







		/* Set the simulation priority to lowest possible value */
#ifdef _WIN32
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#else
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
		nice(19);
# endif
#endif


		/* Set simulation time or number of errors */
		if (iSimTime != 0)
			GenSimData.SetSimTime(iSimTime, 
				SimFileName(Param, strSpecialRemark));
		else
			GenSimData.SetNumErrors(iSimNumErrors, 
				SimFileName(Param, strSpecialRemark));

		/* Open file for storing simulation results */
		strSimFile = string(SIM_OUT_FILES_PATH) +
			SimFileName(Param, strSpecialRemark) + string(".dat");
		pFileSimRes = fopen(strSimFile.c_str(), "w");
		printf("%s\n", strSimFile.c_str()); /* Show name directly */

		/* Main SNR loop */
		for (rSNRCnt = rStartSNR; rSNRCnt <= rEndSNR; rSNRCnt += rStepSNR)
		{
			/* Set SNR in global struct and run simulation */
			Param.rSimSNRdB = rSNRCnt;
			Run();

			/* Store results in file */
			switch (Param.eSimType)
			{
			case CParameter::ST_MSECHANEST:
				/* After the simulation get results */
				IdealChanEst.GetResults(vecrMSE);

				/* Store results in a file */
				for (i = 0; i < vecrMSE.Size(); i++)
					fprintf(pFileSimRes, "%e ", vecrMSE[i]);
				fprintf(pFileSimRes, "\n"); /* New line */
				break;

			case CParameter::ST_SYNC_PARAM:
				/* Show results directly and save them in the file */
				printf("%e %e\n", rSNRCnt, Param.rSyncTestParam);
				fprintf(pFileSimRes, "%e %e\n", rSNRCnt, Param.rSyncTestParam);
				break;

			default:
				/* Show results directly and save them in the file */
				printf("%e %e\n", rSNRCnt, Param.rBitErrRate);
				fprintf(pFileSimRes, "%e %e\n", rSNRCnt, Param.rBitErrRate);
				break;
			}

			/* Make sure results are actually written in the file. In case the
			   simulation machine crashes, at least the last results are
			   preserved */
			fflush(pFileSimRes);
		}

		/* Close simulation results file afterwards */
		fclose(pFileSimRes);
	}


	/* At the end of the simulation, exit the application */
	if (Param.eSimType != CParameter::ST_NONE)
		exit(1);
}

string CDRMSimulation::SimFileName(CParameter& Param, string strAddInf)
{
/* 
	File naming convention:
	BER: Bit error rate simulation
	MSE: MSE for channel estimation
	BERIDEAL: Bit error rate simulation with ideal channel estimation
	SYNC: Simulation for a synchronization paramter (usually variance)

	B3: Robustness mode and spectrum occupancy
	Ch5: Which channel was used

	10k: Value set by "SetNumErrors()"

	example: BER_B3_Ch5_10k_NoSync
*/
	char chNumTmpLong[10];
	string strFileName = "";

	/* What type of simulation */
	switch (Param.eSimType)
	{
	case CParameter::ST_BITERROR:
		strFileName += "BER_";
		break;
	case CParameter::ST_MSECHANEST:
		strFileName += "MSE_";
		break;
	case CParameter::ST_BER_IDEALCHAN:
		strFileName += "BERIDEAL_";
		break;
	case CParameter::ST_SYNC_PARAM:
		strFileName += "SYNC_";
		break;
	}

	/* Channel number */
	strFileName += "CH";
	char chNumTmp;
	sprintf(&chNumTmp, "%d", Param.iDRMChannelNum);
	strFileName += chNumTmp;
	strFileName += "_";

	/* Robustness mode and spectrum occupancy */
	switch (Param.GetWaveMode())
	{
	case RM_ROBUSTNESS_MODE_A:
		strFileName += "A";
		break;
	case RM_ROBUSTNESS_MODE_B:
		strFileName += "B";
		break;
	case RM_ROBUSTNESS_MODE_C:
		strFileName += "C";
		break;
	case RM_ROBUSTNESS_MODE_D:
		strFileName += "D";
		break;
	}
	switch (Param.GetSpectrumOccup())
	{
	case SO_0:
		strFileName += "0_";
		break;
	case SO_1:
		strFileName += "1_";
		break;
	case SO_2:
		strFileName += "2_";
		break;
	case SO_3:
		strFileName += "3_";
		break;
	case SO_4:
		strFileName += "4_";
		break;
	case SO_5:
		strFileName += "5_";
		break;
	}

	/* Number of iterations in MLC decoder */
	strFileName += "It";
	sprintf(chNumTmpLong, "%d", MSCMLCDecoder.GetInitNumIterations());
	strFileName += chNumTmpLong;
	strFileName += "_";

	/* Protection level part B */
	strFileName += "PL";
	sprintf(chNumTmpLong, "%d", Param.MSCPrLe.iPartB);
	strFileName += chNumTmpLong;
	strFileName += "_";

	/* MSC coding scheme */
	switch (Param.eMSCCodingScheme)
	{
	case CParameter::CS_2_SM:
		strFileName += "16SM_";
		break;

	case CParameter::CS_3_SM:
		strFileName += "64SM_";
		break;

	case CParameter::CS_3_HMMIX:
		strFileName += "64MIX_";
		break;

	case CParameter::CS_3_HMSYM:
		strFileName += "64SYM_";
		break;
	}

	/* Number of error events or simulation time */
	int iCurNum;
	string strMultPl = "";
	if (iSimTime != 0)
	{
		strFileName += "T"; /* T -> time */
		iCurNum = iSimTime;
	}
	else
	{
		strFileName += "E"; /* E -> errors */
		iCurNum = iSimNumErrors;
	}
	
	if (iCurNum / 1000 > 0)
	{
		strMultPl = "K";
		iCurNum /= 1000;

		if (iCurNum / 1000 > 0)
		{
			strMultPl = "M";
			iCurNum /= 1000;
		}
	}

	sprintf(chNumTmpLong, "%d", iCurNum);
	strFileName += chNumTmpLong;
	strFileName += strMultPl;


	/* Special remark */
	if (!strAddInf.empty())
	{
		strFileName += "_";
		strFileName += strAddInf;
	}

	return strFileName;
}
