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
	FILE*			pFileBitEr;
	FILE*			pFileMSE;
	CVector<_REAL>	vecrMSE;
	string			strSimFile;
	string			strSpecialRemark;
	int				iSimTime = 0;
	int				iSimNumErrors = 0;


	/**************************************************************************\
	* Simulation settings vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*
	\**************************************************************************/
	/* Choose which type of simulation, if you choose "ST_NONE", the regular
	   application will be started */
	eSimType = ST_MSECHANEST;
	eSimType = ST_BITERROR;
	eSimType = ST_BER_IDEALCHAN;
	eSimType = ST_NONE;
	
	if (eSimType != ST_NONE)
	{
		/* The associated code rate is R = 0,6 and the modulation is 64-QAM */


Param.InitCellMapTable(RM_ROBUSTNESS_MODE_A, SO_2); // SO_0 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!



		Param.MSCPrLe.iPartB = 1;
		Param.eSymbolInterlMode = CParameter::SI_LONG;//SI_SHORT;//
		Param.eMSCCodingScheme = CParameter::CS_3_SM;//CS_3_HMMIX;//CS_3_HMSYM;//

		Param.iDRMChannelNo = 2;

		rStartSNR = (_REAL) 12.0;
		rEndSNR = (_REAL) 18.0;
		rStepSNR = (_REAL) 0.3;
		strSpecialRemark = "odin";

		/* Length of simulation */
//		iSimTime = 20;
		iSimNumErrors = 200000;


ChannelEstimation.SetFreqInt(CChannelEstimation::FWIENER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FDFTFILTER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FLINEAR);

ChannelEstimation.SetTimeInt(CChannelEstimation::TWIENER);
//ChannelEstimation.SetTimeInt(CChannelEstimation::TLINEAR);


		/* Init the modules to adapt to the new parameters. We need to do that
		   because the following routines call modul internal functions which
		   need correctly initialized modules */
		Init();

		MSCMLCDecoder.SetNoIterations(1);

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
#endif


		/* Set simulation time or number of errors */
		if (iSimTime != 0)
			GenSimData.SetSimTime(iSimTime, 
				SimFileName(Param, eSimType, strSpecialRemark));
		else
			GenSimData.SetNoErrors(iSimNumErrors, 
				SimFileName(Param, eSimType, strSpecialRemark));

		if (eSimType == ST_MSECHANEST)
		{
			/* Open simulation file */
			strSimFile = SimFileName(Param, eSimType, strSpecialRemark) + ".dat";
			pFileMSE = fopen(strSimFile.c_str(), "w");

			Param.rSimSNRdB = rStartSNR;

			Run();

			/* After the simulation get results */
			IdealChanEst.GetResults(vecrMSE);

			/* Store results in a file */
			for (i = 0; i < vecrMSE.Size(); i++)
				fprintf(pFileMSE, "%e ", vecrMSE[i]);
			fprintf(pFileMSE, "\n");
			fclose(pFileMSE);
		}
		else
		{
			/* Open simulation file */
			strSimFile = SimFileName(Param, eSimType, strSpecialRemark) + ".dat";
			pFileBitEr = fopen(strSimFile.c_str(), "w");

			for (rSNRCnt = rStartSNR; rSNRCnt <= rEndSNR; rSNRCnt += rStepSNR)
			{
				Param.rSimSNRdB = rSNRCnt;

				Run();

				/* Save results */
				fprintf(pFileBitEr, "%e %e\n", rSNRCnt, Param.rBitErrRate);
				fflush(pFileBitEr);
// clear all;close all;load FileName.dat;semilogy(FileName(:,1), FileName(:,2));grid on

				/* Additionally, show results directly */
				printf("%e %e\n", rSNRCnt, Param.rBitErrRate);
			}

			fclose(pFileBitEr);
		}
	}


	/* At the end of the simulation, exit the application */
	if (eSimType != ST_NONE)
		exit(1);
}

string CDRMSimulation::SimFileName(CParameter& Param, ESimType eNewType,
								   string strAddInf)
{
	/* File naming convention:
	   BER: Bit error rate simulation
	   MSE: MSE for channel estimation

	   B3: Robustness mode and spectrum occupancy
	   Ch5: Which channel was used

	   10k: Value set by "SetNoErrors()"

	   example: BER_B3_Ch5_10k_NoSync */
	string strFileName = "test/";

	/* What type of simulation */
	switch (eNewType)
	{
	case ST_BITERROR:
		strFileName += "BER_";
		break;
	case ST_MSECHANEST:
		strFileName += "MSE_";
		break;
	case ST_BER_IDEALCHAN:
		strFileName += "BERIDEAL_";
		break;
	}

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

	/* Channel number */
	strFileName += "CH";
	char chNumTmp;
	sprintf(&chNumTmp, "%d", Param.iDRMChannelNo);
	strFileName += chNumTmp;
	strFileName += "_";

	/* Number of error events */
	char chNumTmpLong[10];
	sprintf(chNumTmpLong, "%d", GenSimData.GetNoErrors() / 1000);
	strFileName += chNumTmpLong;
	strFileName += "k";

	/* Special remark */
	if (!strAddInf.empty())
	{
		strFileName += "_";
		strFileName += strAddInf;
	}

	return strFileName;
}
