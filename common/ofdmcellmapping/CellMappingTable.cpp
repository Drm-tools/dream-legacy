/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Table of the mapping of OFDM cells.
 *	We build a table of one super-frame where we set flags for each cell to
 *	identify the symbol for this place. E.g. if the flag "CM_MSC" is set for
 *	one table entry this is the cell for a MSC-symbol. The name of the table
 *	is matiMapTab.
 *  We use the table "matcPilotCells" for storing the complex values for the
 *  pilots. For simplicity we allocate memory for all blocks but only the
 *  pilot positions are used.
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

#include "../GlobalDefinitions.h"
#include "CellMappingTable.h"


/* Implementation *************************************************************/
void CCellMappingTable::MakeTable(ERobMode eNewRobustnessMode, 
								  ESpecOcc eNewSpectOccup)
{
	int				iNoMSCDummyCells; /* Number of MSC dummy cells */
	int				iNoTimePilots; /* No of time pilots per frame */
	CScatPilots		ScatPilots;
	int				iSym;
	int				iFrameSym;
	int				iCar;
	int				iTimePilotsCounter;
	int				iFreqPilotsCounter;
	int				iScatPilotsCounter;
	int				iMSCCounter;
	int				iFACCounter;
	int				iScatPilPhase;
	int				iCarArrInd;
	int				iSpecOccArrayIndex;
	/* Tables */
	const int*		piTableFAC;
	const int*		piTableTimePilots;
	const int*		piTableFreqPilots;


	/* Set Parameters and pointers to the tables ******************************/
	switch (eNewSpectOccup)
	{
	case SO_0:
		iSpecOccArrayIndex = 0;
		break;

	case SO_1:
		iSpecOccArrayIndex = 1;
		break;

	case SO_2:
		iSpecOccArrayIndex = 2;
		break;

	case SO_3:
		iSpecOccArrayIndex = 3;
		break;

	case SO_4:
		iSpecOccArrayIndex = 4;
		break;

	case SO_5:
		iSpecOccArrayIndex = 5;
		break;
	}

	/* The robust mode defines all other parameters */
	switch (eNewRobustnessMode)
	{
	case RM_ROBUSTNESS_MODE_A:
		iCarrierKmin = iTableCarrierKmin[iSpecOccArrayIndex][0];
		iCarrierKmax = iTableCarrierKmax[iSpecOccArrayIndex][0];
		
		iFFTSizeN = RMA_FFT_SIZE_N;
		RatioTgTu.iEnum = RMA_ENUM_TG_TU;
		RatioTgTu.iDenom = RMA_DENOM_TG_TU;

		iNoSymPerFrame = RMA_NO_SYM_PER_FRAME;
		iNoSymbolsPerSuperframe = iNoSymPerFrame * NO_FRAMES_IN_SUPERFRAME;
		piTableFAC = &iTableFACRobModA[0][0];
		iNoTimePilots = RMA_NO_TIME_PIL;
		piTableTimePilots = &iTableTimePilRobModA[0][0];
		piTableFreqPilots = &iTableFreqPilRobModA[0][0];
		iScatPilTimeInt = RMA_SCAT_PIL_TIME_INT;
		iScatPilFreqInt = RMA_SCAT_PIL_FREQ_INT;

		/* Scattered pilots phase definition */
		ScatPilots.piConst = iTableScatPilConstRobModA;
		ScatPilots.iColSizeWZ = SIZE_COL_WZ_ROB_MOD_A;
		ScatPilots.piW = &iScatPilWRobModA[0][0];
		ScatPilots.piZ = &iScatPilZRobModA[0][0];
		ScatPilots.iQ = iScatPilQRobModA;

		ScatPilots.piGainTable = &iScatPilGainRobModA[iSpecOccArrayIndex][0];
		break;

	case RM_ROBUSTNESS_MODE_B:
		iCarrierKmin = iTableCarrierKmin[iSpecOccArrayIndex][1];
		iCarrierKmax = iTableCarrierKmax[iSpecOccArrayIndex][1];

		iFFTSizeN = RMB_FFT_SIZE_N;
		RatioTgTu.iEnum = RMB_ENUM_TG_TU;
		RatioTgTu.iDenom = RMB_DENOM_TG_TU;

		iNoSymPerFrame = RMB_NO_SYM_PER_FRAME;
		iNoSymbolsPerSuperframe = iNoSymPerFrame * NO_FRAMES_IN_SUPERFRAME;
		piTableFAC = &iTableFACRobModB[0][0];
		iNoTimePilots = RMB_NO_TIME_PIL;
		piTableTimePilots = &iTableTimePilRobModB[0][0];
		piTableFreqPilots = &iTableFreqPilRobModB[0][0];
		iScatPilTimeInt = RMB_SCAT_PIL_TIME_INT;
		iScatPilFreqInt = RMB_SCAT_PIL_FREQ_INT;

		/* Scattered pilots phase definition */
		ScatPilots.piConst = iTableScatPilConstRobModB;
		ScatPilots.iColSizeWZ = SIZE_COL_WZ_ROB_MOD_B;
		ScatPilots.piW = &iScatPilWRobModB[0][0];
		ScatPilots.piZ = &iScatPilZRobModB[0][0];
		ScatPilots.iQ = iScatPilQRobModB;

		ScatPilots.piGainTable = &iScatPilGainRobModB[iSpecOccArrayIndex][0];
		break;

	case RM_ROBUSTNESS_MODE_C:
		iCarrierKmin = iTableCarrierKmin[iSpecOccArrayIndex][2];
		iCarrierKmax = iTableCarrierKmax[iSpecOccArrayIndex][2];

		iFFTSizeN = RMC_FFT_SIZE_N;
		RatioTgTu.iEnum = RMC_ENUM_TG_TU;
		RatioTgTu.iDenom = RMC_DENOM_TG_TU;

		iNoSymPerFrame = RMC_NO_SYM_PER_FRAME;
		iNoSymbolsPerSuperframe = iNoSymPerFrame * NO_FRAMES_IN_SUPERFRAME;
		piTableFAC = &iTableFACRobModC[0][0];
		iNoTimePilots = RMC_NO_TIME_PIL;
		piTableTimePilots = &iTableTimePilRobModC[0][0];
		piTableFreqPilots = &iTableFreqPilRobModC[0][0];
		iScatPilTimeInt = RMC_SCAT_PIL_TIME_INT;
		iScatPilFreqInt = RMC_SCAT_PIL_FREQ_INT;

		/* Scattered pilots phase definition */
		ScatPilots.piConst = iTableScatPilConstRobModC;
		ScatPilots.iColSizeWZ = SIZE_COL_WZ_ROB_MOD_C;
		ScatPilots.piW = &iScatPilWRobModC[0][0];
		ScatPilots.piZ = &iScatPilZRobModC[0][0];
		ScatPilots.iQ = iScatPilQRobModC;

		ScatPilots.piGainTable = &iScatPilGainRobModC[iSpecOccArrayIndex][0];
		break;

	case RM_ROBUSTNESS_MODE_D:
		iCarrierKmin = iTableCarrierKmin[iSpecOccArrayIndex][3];
		iCarrierKmax = iTableCarrierKmax[iSpecOccArrayIndex][3];

		iFFTSizeN = RMD_FFT_SIZE_N;
		RatioTgTu.iEnum = RMD_ENUM_TG_TU;
		RatioTgTu.iDenom = RMD_DENOM_TG_TU;

		iNoSymPerFrame = RMD_NO_SYM_PER_FRAME;
		iNoSymbolsPerSuperframe = iNoSymPerFrame * NO_FRAMES_IN_SUPERFRAME;
		piTableFAC = &iTableFACRobModD[0][0];
		iNoTimePilots = RMD_NO_TIME_PIL;
		piTableTimePilots = &iTableTimePilRobModD[0][0];
		piTableFreqPilots = &iTableFreqPilRobModD[0][0];
		iScatPilTimeInt = RMD_SCAT_PIL_TIME_INT;
		iScatPilFreqInt = RMD_SCAT_PIL_FREQ_INT;

		/* Scattered pilots phase definition */
		ScatPilots.piConst = iTableScatPilConstRobModD;
		ScatPilots.iColSizeWZ = SIZE_COL_WZ_ROB_MOD_D;
		ScatPilots.piW = &iScatPilWRobModD[0][0];
		ScatPilots.piZ = &iScatPilZRobModD[0][0];
		ScatPilots.iQ = iScatPilQRobModD;

		ScatPilots.piGainTable = &iScatPilGainRobModD[iSpecOccArrayIndex][0];
		break;
	}

	/* Get No of carriers with DC */
	iNoCarrier = iCarrierKmax - iCarrierKmin + 1;

	/* Length of guard-interval measured in "time-bins". We do the calculation
	   with integer variables -> "/ RatioTgTu.iDenom" MUST be the last
	   operation! */
	iGuardSize = iFFTSizeN * RatioTgTu.iEnum / RatioTgTu.iDenom;

	/* Symbol block size is the guard-interval plus the useful part */
	iSymbolBlockSize = iFFTSizeN + iGuardSize;

	/* Calculate the index of the DC carrier in the shifted spectrum */
	iIndexDCFreq = (int) ((_REAL) VIRTUAL_INTERMED_FREQ * 
		iFFTSizeN / SOUNDCRD_SAMPLE_RATE);

	/* Index of minimum useful carrier (shifted) */
	iShiftedKmin = iIndexDCFreq + iCarrierKmin;

	/* Index. of maximum useful carrier (shifted) */
	iShiftedKmax = iIndexDCFreq + iCarrierKmax;

	/* Calculate number of time-interploated frequency pilots. Special case
	   with robustness mode D: pilots in all carriers! BUT: DC carrier is
	   counted as a pilot in that case!!! Be aware of that! */
	if (iScatPilFreqInt > 1)
		iNoIntpFreqPil = (int) ((_REAL) iNoCarrier / iScatPilFreqInt + 1);
	else
		iNoIntpFreqPil = iNoCarrier;


	/* Allocate memory for vectors and matrices ----------------------------- */
	/* Allocate memory for mapping table (Matrix) */
	matiMapTab.Init(iNoSymbolsPerSuperframe, iNoCarrier);

	/* Allocate memory for pilot cells definition and set it to zero */
	matcPilotCells.Init(iNoSymbolsPerSuperframe, iNoCarrier, 
		_COMPLEX((_REAL) 0.0, (_REAL) 0.0));

	/* Allocate memory for vectors with number of certain cells */
	veciNoMSCSym.Init(iNoSymbolsPerSuperframe);
	veciNoFACSym.Init(iNoSymbolsPerSuperframe);
	veciNoSDCSym.Init(iNoSymbolsPerSuperframe);


	/* Build table ************************************************************/
	/* Some of the definitions at the beginning are overwritten by successive
	   definitions! E.g., first define all carriers as MSC cells */
	iFreqPilotsCounter = 0;
	iTimePilotsCounter = 0;
	for (iSym = 0; iSym < iNoSymbolsPerSuperframe; iSym++)
	{
		/* Frame symbol: Counts symbols in one frame, not super frame! */
		iFrameSym = iSym % iNoSymPerFrame;

		/* Reset FAC counter at the beginning of each new frame */
		if (iFrameSym == 0)
			iFACCounter = 0;

		/* Calculate the start value of "p" in equation for gain reference 
		   cells in Table 90 (8.4.4.1) */
		iScatPilotsCounter = (int) ((_REAL) (iCarrierKmin - 
			(int) ((_REAL) iScatPilFreqInt / 2 + .5) - 
			iScatPilFreqInt * mod(iFrameSym, iScatPilTimeInt)
			) / (iScatPilFreqInt * iScatPilTimeInt));

		for (iCar = iCarrierKmin; iCar < iCarrierKmax + 1; iCar++)
		{
			/* Set carrier array index (since we do not have negative indices
			   in c++) */
			iCarArrInd = iCar - iCarrierKmin;


			/* MSC ---------------------------------------------------------- */
			/* First set all cells to MSC-cells */
			matiMapTab[iSym][iCarArrInd] = CM_MSC;


			/* SDC ---------------------------------------------------------- */
			/* No MSC-cells in the first two (or three) symbols -> SDC */
			switch (eNewRobustnessMode)
			{
			case RM_ROBUSTNESS_MODE_A:
			case RM_ROBUSTNESS_MODE_B:
				if ((iSym == 0) || (iSym == 1))
					matiMapTab[iSym][iCarArrInd] = CM_SDC;

				break;

			case RM_ROBUSTNESS_MODE_C:
			case RM_ROBUSTNESS_MODE_D:
				if ((iSym == 0) || (iSym == 1) || (iSym == 2))
					matiMapTab[iSym][iCarArrInd] = CM_SDC;

				break;
			}


			/* FAC ---------------------------------------------------------- */
			/* FAC positions are defined in a table */
			if (iFACCounter <= NO_FAC_CELLS)
			{
				/* piTableFAC[x * 2]: first column; piTableFAC[x * 2 + 1]: 
				   second column */
				if (piTableFAC[iFACCounter * 2] * iNoCarrier +
					piTableFAC[iFACCounter * 2 + 1] == iFrameSym * 
					iNoCarrier + iCar)
				{
					iFACCounter++;
					matiMapTab[iSym][iCarArrInd] = CM_FAC;
				}
			}


			/* Scattered pilots --------------------------------------------- */
			/* Standard: 8.4.4.3:
			   "In some cases gain references fall in locations which coincide 
			   with those already defined for either frequency or time 
			   references. In these cases, the phase definitions given in 
			   clauses 8.4.2 and 8.4.3 take precedence."
			   Therefore, Scattered pilots must be definded FIRST here! */

			/* The rule for calculating the scattered pilots is defined in the
			   specification in the following form: 
			   e.g.: k = 2 + 4 * (s mod 5) + 20 * p 
			   We define a "frequency-" (FreqInt) and "time-interpolation" 
			   (TimeInt). In this example, "4" is the FreqInt and "5" is the 
			   TimeInt. The first term "2" is the half of the FreqInt, rounded
			   towards infinity. The parameter "20" is FreqInt * TimeInt */
			if (iCar == (int) ((_REAL) iScatPilFreqInt / 2 + .5) + 
				iScatPilFreqInt * mod(iFrameSym, iScatPilTimeInt) + 
				iScatPilFreqInt * iScatPilTimeInt * iScatPilotsCounter)
			{
				iScatPilotsCounter++;

				/* Set flag in mapping table */
				matiMapTab[iSym][iCarArrInd] = CM_SCAT_PI;

				/* Set complex value for this pilot */
				/* Phase calculation ---------------------------------------- */
				int in, im, ip, i;

				/* Calculations as in drm-standard (8.4.4.3.1) */
				/* "in" is ROW No and "im" is COLUMN No! */
				in = mod(iFrameSym, ScatPilots.piConst[1] /* "y" */);
				im = (int) 
					((_REAL) iFrameSym / ScatPilots.piConst[1] /* "y" */);
				ip = (int) ((_REAL) (iCar - ScatPilots.piConst[2] /* "k_0" */ -
					in * ScatPilots.piConst[0] /* "x" */) / (
					ScatPilots.piConst[0] /* "x" */ * 
					ScatPilots.piConst[1] /* "y" */));

				/* Phase_1024[s,k] = 
				       (4Z_256[n,m]pW_1024[n,m] + p^2(1 + s)Q_1024) mod 1024 */
				iScatPilPhase = mod(4 * ScatPilots.piZ[in * 
					ScatPilots.iColSizeWZ + im] + ip *
					ScatPilots.piW[in * 
					ScatPilots.iColSizeWZ + im] + 
					ip * ip * (1 + iFrameSym) * ScatPilots.iQ, 1024);


				/* Gain calculation and applying of complex value ----------- */
				/* Test, if current carrier-index is one of the "boosted pilots"
				   position */
				_BOOLEAN bIsBoostedPilot = FALSE;
				for (i = 0; i < NO_BOOSTED_SCAT_PILOTS; i++)
				{
					/* In case of match set flag */
					if (ScatPilots.piGainTable[i] == iCar)
						bIsBoostedPilot = TRUE;
				}

				/* Boosted pilot: Gain = 2, Regular pilot: Gain = sqrt(2) */
				if (bIsBoostedPilot)
				{
					matcPilotCells[iSym][iCarArrInd] = 
						Polar2Cart(2, iScatPilPhase);

					/* Add flag for boosted pilot */
					matiMapTab[iSym][iCarArrInd] |= CM_BOOSTED_PI;
				}
				else
				{
					matcPilotCells[iSym][iCarArrInd] = 
						Polar2Cart(sqrt(2), iScatPilPhase);
				}
			}
			

			/* Time-reference pilots ---------------------------------------- */
			/* Time refs at the beginning of each frame, we use a table */
			if (iFrameSym == 0)
			{
				/* Use only the first column in piTableTimePilots */
				if (piTableTimePilots[iTimePilotsCounter * 2] == iCar)
				{
					/* Set flag in mapping table, consider case of both, 
					   scattered pilot and time pilot at same position */
					if (matiMapTab[iSym][iCarArrInd] & CM_SCAT_PI)
						matiMapTab[iSym][iCarArrInd] |= CM_TI_PI;
					else
						matiMapTab[iSym][iCarArrInd] = CM_TI_PI;

					/* Set complex value for this pilot */
					matcPilotCells[iSym][iCarArrInd] = 
						Polar2Cart(sqrt(2),
						piTableTimePilots[iTimePilotsCounter * 2 + 1]);

					if (iTimePilotsCounter == iNoTimePilots - 1)
						iTimePilotsCounter = 0;
					else
						iTimePilotsCounter++;
				}
			}


			/* Frequency-reference pilots ----------------------------------- */
			/* These pilots are in all symbols, the positions are stored in 
			   a table */
			/* piTableFreqPilots[x * 2]: first column; 
			   piTableFreqPilots[x * 2 + 1]: second column */
			if (piTableFreqPilots[iFreqPilotsCounter * 2] == iCar)
			{
				/* Set flag in mapping table, consider case of multiple
				   definitions of pilot-mapping */
				if ((matiMapTab[iSym][iCarArrInd] & CM_TI_PI) ||
					(matiMapTab[iSym][iCarArrInd] & CM_SCAT_PI))
				{
					matiMapTab[iSym][iCarArrInd] |= CM_FRE_PI;
				}
				else
					matiMapTab[iSym][iCarArrInd] = CM_FRE_PI;

				/* Set complex value for this pilot */
				/* Test for "special case" defined in drm-standard */
				_BOOLEAN bIsFreqPilSpeciCase = FALSE;
				if (eNewRobustnessMode == RM_ROBUSTNESS_MODE_D)
				{
					/* For robustness mode D, carriers 7 and 21 (Means: first
					   and second pilot, not No. 28 (NO_FREQ_PILOTS - 1) */
					if (iFreqPilotsCounter != NO_FREQ_PILOTS - 1)
					{
						/* Test for odd values of "s" (iSym) */
						if ((iFrameSym % 2) == 1)
							bIsFreqPilSpeciCase = TRUE;
					}
				}

				/* Apply complex value */
				if (bIsFreqPilSpeciCase)
					matcPilotCells[iSym][iCarArrInd] = 
						Polar2Cart(sqrt(2), mod(
						piTableFreqPilots[iFreqPilotsCounter * 2 + 1] + 
						512, 1024));
				else
					matcPilotCells[iSym][iCarArrInd] = 
						Polar2Cart(sqrt(2),
						piTableFreqPilots[iFreqPilotsCounter * 2 + 1]);

				/* Increase counter and wrap if needed */
				if (iFreqPilotsCounter == NO_FREQ_PILOTS - 1)
					iFreqPilotsCounter = 0;
				else
					iFreqPilotsCounter++;
			}


			/* DC-carrier (not used by DRM) --------------------------------- */
			/* Mark DC-carrier. Must be marked after scattered pilots, because
			   in one case (Robustness Mode D) some pilots must be overwritten!
			   */
			if (iCar == 0)
				matiMapTab[iSym][iCarArrInd] = CM_DC;

			/* In Robustness Mode A there are three "not used carriers" */
			if (eNewRobustnessMode == RM_ROBUSTNESS_MODE_A)
			{
				if ((iCar == -1) || (iCar == 1))
					matiMapTab[iSym][iCarArrInd] = CM_DC;
			} 
		}
	}


	/* Count individual cells *************************************************/
	/* We need to count the cells in a symbol for defining how many values from
	   each source is needed to generate one symbol in carrier-mapping */
	/* Init all counters */
	iMaxNoMSCSym = 0;
	iMaxNoFACSym = 0;
	iMaxNoSDCSym = 0;
	iNoSDCCellsPerSFrame = 0;
	iMSCCounter = 0;

	rAvPowPerSymbol = (_REAL) 0.0;

	for (iSym = 0; iSym < iNoSymbolsPerSuperframe; iSym++)
	{
		/* Init all counters */
		veciNoMSCSym[iSym] = 0;
		veciNoFACSym[iSym] = 0;
		veciNoSDCSym[iSym] = 0;
		
		for (iCar = 0; iCar < iNoCarrier; iCar++)
		{
			/* MSC */
			if (matiMapTab[iSym][iCar] & CM_MSC)
			{
				veciNoMSCSym[iSym]++;

				/* Count ALL MSC cells per super-frame */
				iMSCCounter++;
			}

			/* FAC */
			if (matiMapTab[iSym][iCar] & CM_FAC)
				veciNoFACSym[iSym]++;

			/* SDC */
			if (matiMapTab[iSym][iCar] & CM_SDC)
			{
				veciNoSDCSym[iSym]++;

				/* Count ALL SDC cells per super-frame */
				iNoSDCCellsPerSFrame++;
			}

			/* Calculations for average power per symbol (needed for SNR 
			   estimation and simulation). DC carrier is zero (contributes not
			   to the average power) */
			if (!(matiMapTab[iSym][iCar] & CM_DC))
			{
				if ((matiMapTab[iSym][iCar] & CM_FAC) ||
					(matiMapTab[iSym][iCar] & CM_SDC) ||
					(matiMapTab[iSym][iCar] & CM_MSC))
				{
					/* Data cells have average power of 1 */
					rAvPowPerSymbol += (_REAL) 1.0;
				}
				else
				{
					/* All pilots have power of 2 except of the boosted pilots
					   at the edges of the spectrum (they have power of 4) */
					if (matiMapTab[iSym][iCar] & CM_BOOSTED_PI)
						rAvPowPerSymbol += (_REAL) 4.0;
					else
						rAvPowPerSymbol += (_REAL) 2.0;
				}
			}
		}

		/* Set maximum for symbol */
		/* MSC */
		if (iMaxNoMSCSym < veciNoMSCSym[iSym])
			iMaxNoMSCSym = veciNoMSCSym[iSym];

		/* FAC */
		if (iMaxNoFACSym < veciNoFACSym[iSym])
			iMaxNoFACSym = veciNoFACSym[iSym];

		/* SDC */
		if (iMaxNoSDCSym < veciNoSDCSym[iSym])
			iMaxNoSDCSym = veciNoSDCSym[iSym];
	}

	/* Set number of useful MSC cells */
	iNoUsefMSCCellsPerFrame = 
		(int) (iMSCCounter / NO_FRAMES_IN_SUPERFRAME);

	/* Calculate dummy cells for MSC */
	iNoMSCDummyCells = iMSCCounter - iNoUsefMSCCellsPerFrame  * 
		NO_FRAMES_IN_SUPERFRAME;

	/* Correct last MSC count (because of dummy cells) */
	veciNoMSCSym[iNoSymbolsPerSuperframe - 1] -= iNoMSCDummyCells;

	/* Normalize the average power */
	rAvPowPerSymbol /= iNoSymbolsPerSuperframe;


/* ########################################################################## */
#ifdef _PRINT_TABLES_
/* Save table in file */
FILE* pFile = fopen("test/CarMapTable.dat", "w");
for (int i = 0; i < iNoSymbolsPerSuperframe; i++)
{
	for (int j = 0; j < iNoCarrier; j++)
	{
		if (matiMapTab[i][j] & CM_DC)
		{
			fprintf(pFile, ":");
			continue;
		}
		if (matiMapTab[i][j] & CM_MSC)
		{
			fprintf(pFile, ".");
			continue;
		}
		if (matiMapTab[i][j] & CM_SDC)
		{
			fprintf(pFile, "S");
			continue;
		}
		if (matiMapTab[i][j] & CM_FAC)
		{
			fprintf(pFile, "X");
			continue;
		}
		if (matiMapTab[i][j] & CM_TI_PI)
		{
			fprintf(pFile, "T");
			continue;
		}
		if (matiMapTab[i][j] & CM_FRE_PI)
		{
			fprintf(pFile, "f");
			continue;
		}
		if (matiMapTab[i][j] & CM_SCAT_PI)
		{
			/* Special mark for boosted pilots */
			if (matiMapTab[i][j] & CM_BOOSTED_PI)
				fprintf(pFile, "*");
			else
				fprintf(pFile, "0");
			continue;
		}
	}
	fprintf(pFile, "\n"); // New line
}
fclose(pFile);

/* Save pilot values in file */
/* Use following command to plot pilot complex values in Matlab:

	clear all;close all;load PilotCells.dat;subplot(211),mesh(abs(complex(PilotCells(:,1:2:end), PilotCells(:,2:2:end))));subplot(212),mesh(angle(complex(PilotCells(:,1:2:end), PilotCells(:,2:2:end))))

(It plots the absolute of the pilots in the upper plot and angle in 
the lower plot.)
*/
pFile = fopen("test/PilotCells.dat", "w");
for (int z = 0; z < iNoSymbolsPerSuperframe; z++)
{
	for (int v = 0; v < iNoCarrier; v++)
		fprintf(pFile, "%e %e ", matcPilotCells[z][v].real(), 
			matcPilotCells[z][v].imag());

	fprintf(pFile, "\n"); // New line
}
fclose(pFile);
#endif
/* ########################################################################## */
}

_COMPLEX CCellMappingTable::Polar2Cart(const _REAL rAbsolute, 
									   const int iPhase) const
{
/* 
	This function takes phases normalized to 1024 as defined in the drm-
	standard.
*/
	return _COMPLEX(rAbsolute * cos((_REAL) 2 * crPi * iPhase / 1024), 
		rAbsolute * sin((_REAL) 2 * crPi * iPhase / 1024));
}

int CCellMappingTable::mod(const int ix, const int iy) const
{
	/* Modulus definition for integer numbers */
	if (ix < 0)
		return ix % iy + iy;
	else
		return ix % iy;
}
