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
	_REAL			rSNRCnt;
	FILE*			pFile;
	CVector<_REAL>	vecrMSE;

	/* Choose which type of simulation */
	eSimType = ST_CHANEST;
	eSimType = ST_BITERROR;
	eSimType = ST_NONE;


/* Set the simulation priority to lowest possible value */
#ifdef _WIN32
	if (eSimType != ST_NONE)
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#endif


	switch (eSimType)
	{
	case ST_BITERROR:
		/* Simulation: Bit error rate --------------------------------------- */
		pFile = fopen("test/BitErrors.dat", "w");

		/* The associated code rate is R = 0,6 and the modulation is 64-QAM */
		Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);
		Param.MSCPrLe.iPartB = 1;
		Param.eMSCCodingScheme = CParameter::CS_3_SM;

//Param.eMSCCodingScheme = CParameter::CS_3_HMMIX;//CS_3_HMSYM;

		Param.iDRMChannelNo = 4;

		/* Init the modules to adapt to the new parameters. We need to do that
		   because the following routines call modul internal functions which
		   need correcetly initialized modules */
		Init();


		/* Define which synchronization algorithms we want to use */
		/* In case of bit error simulations, a synchronized DRM data stream is
		   used. Set corresponding modules to synchronized mode */
		InputResample.SetSyncInput(TRUE);
		FreqSyncAcq.SetSyncInput(TRUE);
		SyncUsingPil.SetSyncInput(TRUE);
		TimeSync.SetSyncInput(TRUE);

//ChannelEstimation.SetTimeInt(CChannelEstimation::TLINEAR);

		/* No of blocks for simulation */
		GenSimData.SetSimTime(1000);
//		GenSimData.SetNoErrors(100);

		MSCMLCDecoder.SetNoIterations(0);

		for (rSNRCnt = 22; rSNRCnt <= 27; rSNRCnt += 1.0)
		{
			Param.rSimSNRdB = rSNRCnt;

			Run();

			/* Save results */
			fprintf(pFile, "%e %e\n", rSNRCnt, Param.rBitErrRate);
			fflush(pFile);

			/* Additionally, show results directly */
			printf("%e %e\n", rSNRCnt, Param.rBitErrRate);
		}
		fclose(pFile);
		break;




	case ST_CHANEST:
		/* Simulation: channel estimation ----------------------------------- */
		pFile = fopen("test/mse.dat", "w");

ChannelEstimation.SetFreqInt(CChannelEstimation::FWIENER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FDFTFILTER);
//ChannelEstimation.SetFreqInt(CChannelEstimation::FLINEAR);

ChannelEstimation.SetTimeInt(CChannelEstimation::TLINEAR);
//ChannelEstimation.SetTimeInt(CChannelEstimation::TWIENER);


		Param.InitCellMapTable(RM_ROBUSTNESS_MODE_B, SO_3);

		Param.iDRMChannelNo = 3;

		/* No of blocks for simulation */
		GenSimData.SetSimTime(1000);

		Param.rSimSNRdB = 20;//rSNRCnt;

		Run();

		/* After the simulation get results */
		EvalChanEst.GetResults(vecrMSE);

		/* Store results in a file */
		for (i = 0; i < vecrMSE.Size(); i++)
			fprintf(pFile, "%e ", vecrMSE[i]);
		fprintf(pFile, "\n");
		fflush(pFile);

		fclose(pFile);
		break;
	}

	/* At the end of the simulation, exit the application */
	if (eSimType != ST_NONE)
		exit(1);
}
