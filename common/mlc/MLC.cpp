/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Multi-level-channel (de)coder (MLC)
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

#include "MLC.h"


/* Implementation *************************************************************/
/******************************************************************************\
* MLC-encoder                                                                  *
\******************************************************************************/
void CMLCEncoder::ProcessDataInternal(CParameter& Parameter)
{
	int	i, j;
	int	iNoBitsOutDec;
	int iElementCounter;

	/* Energy dispersal ----------------------------------------------------- */
	/* VSPP is treated as a separate part for energy dispersal */
	EnergyDisp.ProcessData(pvecInputData);


	/* Partitioning of input-stream ----------------------------------------- */
	iElementCounter = 0;

	if (iL[2] == 0)
	{
		/* Standard departitioning */
		/* Protection level A */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				vecbiEncInBuffer[j][i] = (*pvecInputData)[iElementCounter];
				iElementCounter++;
			}
		}

		/* Protection level B */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				vecbiEncInBuffer[j][iM[j][0] + i] = 
					(*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}
	}
	else
	{
		/* Special departitioning with hierarchical modulation. First set 
		   hierarchical bits at the beginning, then append the rest */
		/* Hierarchical frame (always "iM[0][1]"). "iM[0][0]" is always "0" in
		   this case */
		for (i = 0; i < iM[0][1]; i++)
		{
			vecbiEncInBuffer[0][i] = (*pvecInputData)[iElementCounter];
	
			iElementCounter++;
		}


		/* Protection level A (higher protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				vecbiEncInBuffer[j][i] = (*pvecInputData)[iElementCounter];
				iElementCounter++;
			}
		}

		/* Protection level B  (lower protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				vecbiEncInBuffer[j][iM[j][0] + i] = 
					(*pvecInputData)[iElementCounter];

				iElementCounter++;
			}
		}
	}


	/* Convolutional encoder ------------------------------------------------ */
	for (j = 0; j < iLevels; j++)
	{
		iNoBitsOutDec = 
			ConvEncoder.Encode(vecbiEncInBuffer[j], vecbiEncOutBuffer[j],
			iM[j][0], iM[j][1], iCodeRate[j][0], iCodeRate[j][1], j);
#ifdef _DEBUG_
if (iNoBitsOutDec != iNoEncBits)
{
	DebugError("MLC decoded bits test", "No bits decoder out",
		iNoBitsOutDec, "No encoded bits", iNoEncBits);
}
#endif
	}


	/* Bit interleaver ------------------------------------------------------ */
	for (j = 0; j < iLevels; j++)
		if (piInterlSequ[j] != -1)
			BitInterleaver[piInterlSequ[j]].Interleave(vecbiEncOutBuffer[j]);


	/* QAM mapping ---------------------------------------------------------- */
	QAMMapping.Map(vecbiEncOutBuffer[0], 
				   vecbiEncOutBuffer[1], 
				   vecbiEncOutBuffer[2], 
				   vecbiEncOutBuffer[3], 
				   vecbiEncOutBuffer[4], 
				   vecbiEncOutBuffer[5], pvecOutputData);
}

void CMLCEncoder::InitInternal(CParameter& TransmParam)
{
	int i;

	CalculateParam(TransmParam, eChannelType);
	
	iNoInBits = iL[0] + iL[1] + iL[2];


	/* Init modules --------------------------------------------------------- */
	/* Energy dispersal */
	EnergyDisp.Init(iNoInBits, iL[2]);

	/* Encoder */
	ConvEncoder.Init(eCodingScheme, iN[0], iN[1], eChannelType);

	/* Bit interleaver */
	/* First init all possible interleaver (According table "TableMLC.h" ->
	   "Interleaver sequence") */
	if (eCodingScheme == CParameter::CS_3_HMMIX)
	{
		BitInterleaver[0].Init(iN[0], iN[1], 13);
		BitInterleaver[1].Init(iN[0], iN[1], 21);
	}
	else
	{
		BitInterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitInterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
	}

	/* QAM-mapping */
	QAMMapping.Init(iN_mux, eCodingScheme);


	/* Allocate memory for internal bit-buffers ----------------------------- */
	for (i = 0; i < iLevels; i++)
	{
		/* Buffers for each encoder on all different levels */
		/* Add bits from higher protected and lower protected part */
		vecbiEncInBuffer[i].Init(iM[i][0] + iM[i][1]);
	
		/* Encoder output buffers for all levels. Must have the same length */
		vecbiEncOutBuffer[i].Init(iNoEncBits);
	}

	/* Define block-size for input and output */
	iInputBlockSize = iNoInBits;
	iOutputBlockSize = iN_mux;
}


/******************************************************************************\
* MLC-decoder                                                                  *
\******************************************************************************/
void CMLCDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int			i, j, k;
	int			iElementCounter;
	_BOOLEAN	bIteration;

	/* Save input signal for signal constellation. We cannot use the copy
	   operator of vector because the input vector is not of the same size as
	   our intermediate buffer, therefore the "for"-loop */
	for (i = 0; i < iInputBlockSize; i++)
		vecSigSpacBuf[i] = (*pvecInputData)[i].cSig;

	/* Iteration loop */
	for (k = 0; k < iNoIterations + 1; k++)
	{
		for (j = 0; j < iLevels; j++)
		{
			/* Metric ------------------------------------------------------- */
			if (k > 0)
				bIteration = TRUE;
			else
				bIteration = FALSE;

			MLCMetric.CalculateMetric(pvecInputData, vecMetric, 
				vecbiSubsetDef[0], vecbiSubsetDef[1], vecbiSubsetDef[2], 
				vecbiSubsetDef[3], vecbiSubsetDef[4], vecbiSubsetDef[5], 
				j, bIteration);


			/* Bit deinterleaver -------------------------------------------- */
			if (piInterlSequ[j] != -1)
				BitDeinterleaver[piInterlSequ[j]].Deinterleave(vecMetric);


			/* Viterbi decoder ---------------------------------------------- */
			rAccMetric = ViterbiDecoder.Decode(vecMetric, vecbiDecOutBits[j], 
				iM[j][0], iM[j][1], iCodeRate[j][0], iCodeRate[j][1], j);

			/* The last branch of encoding and interleaving must not be used at 
			   the very last loop */
			/* "iLevels - 1" for iLevels = 1, 2, 3
			   "iLevels - 2" for iLevels = 6 */
			if ((k < iNoIterations) ||
				((k == iNoIterations) && !(j >= iIndexLastBranch)))
			{
				/* Convolutional encoder ------------------------------------ */
				ConvEncoder.Encode(vecbiDecOutBits[j], vecbiSubsetDef[j],
					iM[j][0], iM[j][1], iCodeRate[j][0], iCodeRate[j][1], j);
			

				/* Bit interleaver ------------------------------------------ */
				if (piInterlSequ[j] != -1)
					BitInterleaver[piInterlSequ[j]].
						Interleave(vecbiSubsetDef[j]);
			}
		}
	}


	/* De-partitioning of input-stream -------------------------------------- */
	iElementCounter = 0;

	if (iL[2] == 0)
	{
		/* Standard departitioning */
		/* Protection level A (higher protected part) */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[j][i]; 
				iElementCounter++;
			}
		}

		/* Protection level B (lower protected part) */
		for (j = 0; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				(*pvecOutputData)[iElementCounter] = 
					vecbiDecOutBits[j][iM[j][0] + i];

				iElementCounter++;
			}
		}
	}
	else
	{
		/* Special departitioning with hierarchical modulation. First set 
		   hierarchical bits at the beginning, then append the rest */
		/* Hierarchical frame (always "iM[0][1]"). "iM[0][0]" is always "0" in
		   this case */
		for (i = 0; i < iM[0][1]; i++)
		{
			(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[0][i];
			iElementCounter++;
		}

		/* Protection level A (higher protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][0]; i++)
			{
				(*pvecOutputData)[iElementCounter] = vecbiDecOutBits[j][i]; 
				iElementCounter++;
			}
		}

		/* Protection level B (lower protected part) */
		for (j = 1; j < iLevels; j++)
		{
			/* Bits */
			for (i = 0; i < iM[j][1]; i++)
			{
				(*pvecOutputData)[iElementCounter] = 
					vecbiDecOutBits[j][iM[j][0] + i];

				iElementCounter++;
			}
		}
	}


	/* Energy dispersal ----------------------------------------------------- */
	/* VSPP is treated as a separate part for energy dispersal (7.2.2) */
	EnergyDisp.ProcessData(pvecOutputData);
}

void CMLCDecoder::InitInternal(CParameter& ReceiverParam)
{
	int i;

	/* First, calculate all necessary parameters for decoding process */
	CalculateParam(ReceiverParam, eChannelType);

	/* Coding scheme could have be changed, update number of iterations */
	SetNoIterations(iNoIterations);

	/* Set this parameter to identify the last level of coder (important for 
	   very last loop */
	if (eCodingScheme == CParameter::CS_3_HMMIX)
		iIndexLastBranch = iLevels - 2;
	else
		iIndexLastBranch = iLevels - 1;

	iNoOutBits = iL[0] + iL[1] + iL[2];

	/* Reset accumulated metric for reliability test of transmission */
	rAccMetric = (_REAL) 0.0;


	/* Init modules --------------------------------------------------------- */
	/* Energy dispersal */
	EnergyDisp.Init(iNoOutBits, iL[2]);

	/* Viterby decoder */
	ViterbiDecoder.Init(eCodingScheme, iN[0], iN[1], eChannelType);

	/* Encoder */
	ConvEncoder.Init(eCodingScheme, iN[0], iN[1], eChannelType);

	/* Bit interleaver */
	/* First init all possible interleaver (According table "TableMLC.h" ->
	   "Interleaver sequence") */
	if (eCodingScheme == CParameter::CS_3_HMMIX)
	{
		BitDeinterleaver[0].Init(iN[0], iN[1], 13);
		BitDeinterleaver[1].Init(iN[0], iN[1], 21);
		BitInterleaver[0].Init(iN[0], iN[1], 13);
		BitInterleaver[1].Init(iN[0], iN[1], 21);
	}
	else
	{
		BitDeinterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitDeinterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
		BitInterleaver[0].Init(2 * iN[0], 2 * iN[1], 13);
		BitInterleaver[1].Init(2 * iN[0], 2 * iN[1], 21);
	}
	
	/* Metric */
	MLCMetric.Init(iN_mux, eCodingScheme);


	/* Allocate memory for internal bit (metric) -buffers ------------------- */
	vecMetric.Init(iNoEncBits);

	/* Decoder output buffers for all levels. Have different length */
	for (i = 0; i < iLevels; i++)
		vecbiDecOutBits[i].Init(iM[i][0] + iM[i][1]);

	/* Buffers for subset definition (always number of encoded bits long) */
	for (i = 0; i < MC_MAX_NO_LEVELS; i++)
		vecbiSubsetDef[i].Init(iNoEncBits);

	/* Init buffer for signal space */
	vecSigSpacBuf.Init(iN_mux);

	/* Define block-size for input and output */
	iInputBlockSize = iN_mux;
	iOutputBlockSize = iNoOutBits;
}

void CMLCDecoder::SetNoIterations(int iNewNoIterations)
{
	/* Reasonable number of iterations depends on coding scheme. With a 
	   4-QAM no iteration is possible */
	if (eCodingScheme == CParameter::CS_1_SM)
		iNoIterations = 0;
	else
		iNoIterations = iNewNoIterations;
}


/******************************************************************************\
* MLC base class                                                               *
\******************************************************************************/
void CMLC::CalculateParam(CParameter& Parameter, int iNewChannelType)
{
	int i;
	int iMSCDataLenPartA;

	switch (iNewChannelType)
	{
	/* FAC ********************************************************************/ 
	case CParameter::CT_FAC:
		eCodingScheme = CParameter::CS_1_SM;
		iN_mux = NO_FAC_CELLS;

		iNoEncBits = NO_FAC_CELLS * 2;

		iLevels = 1;

		/* Code rates for prot.-Level A and B for each level */
		/* Protection Level A */
		iCodeRate[0][0] = 0;

		/* Protection Level B */
		iCodeRate[0][1] = iCodRateCombFDC4SM;

		/* Define interleaver sequence for all levels */
		piInterlSequ = iInterlSequ4SM;


		/* iN: No of OFDM-cells of each protection level ---------------- */
		iN[0] = 0;
		iN[1] = iN_mux;


		/* iM: No of bits each level ---------------------------------------- */
		iM[0][0] = 0;
		iM[0][1] = 72;


		/* iL: No of bits each protection level ----------------------------- */
		/* Higher protected part */
		iL[0] = 0; 

		/* Lower protected part */
		iL[1] = iM[0][1]; 

		/* Very strong protected part (VSPP) */
		iL[2] = 0; 
		break;


	/* SDC ********************************************************************/ 
	case CParameter::CT_SDC:
		eCodingScheme = Parameter.eSDCCodingScheme;
		iN_mux = Parameter.iNoSDCCellsPerSFrame;

		iNoEncBits = iN_mux * 2;

		switch (eCodingScheme)
		{
		case CParameter::CS_1_SM:
			iLevels = 1;

			/* Code rates for prot.-Level A and B for each level */
			/* Protection Level A */
			iCodeRate[0][0] = 0;

			/* Protection Level B */
			iCodeRate[0][1] = iCodRateCombSDC4SM;

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ4SM;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			iN[0] = 0;
			iN[1] = iN_mux;


			/* iM: No of bits each level ------------------------------------ */
			iM[0][0] = 0;

			/* M_0,2 = RX_0 * floor((2 * N_SDC - 12) / RY_0) */
			iM[0][1] = iPuncturingPatterns[iCodRateCombSDC4SM][0] * 
				(int) ((_REAL) (2 * iN_mux - 12) / 
				iPuncturingPatterns[iCodRateCombSDC4SM][1]);


			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = 0; 

			/* Lower protected part */
			iL[1] = iM[0][1]; 

			/* Very strong protected part (VSPP) */
			iL[2] = 0; 
			break;

		case CParameter::CS_2_SM:
			iLevels = 2;

			/* Code rates for prot.-Level A and B for each level */
			for (i = 0; i < 2; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 0;

				/* Protection Level B */
				iCodeRate[i][1] = iCodRateCombSDC16SM[i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ16SM;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			iN[0] = 0;
			iN[1] = iN_mux;


			/* iM: No of bits each level ------------------------------------ */
			/* M_p,2 = RX_p * floor((N_2 - 6) / RY_p) */
			for (i = 0; i < 2; i++)
			{
				iM[i][0] = 0;

				/* M_p,2 = RX_p * floor((2 * N_SDC - 12) / RY_p) */
				iM[i][1] = iPuncturingPatterns[iCodRateCombSDC16SM[i]][0] * 
					(int) ((_REAL) (2 * iN[1] - 12) / 
					iPuncturingPatterns[iCodRateCombSDC16SM[i]][1]);
			}
			

			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = 0; 

			/* Lower protected part */
			iL[1] = iM[0][1] + iM[1][1]; 

			/* Very strong protected part (VSPP) */
			iL[2] = 0; 
			break;
		}

		/* Set number of bits for one SDC-block */
		Parameter.SetNoDecodedBitsSDC(iL[1]);
		break;


	/* MSC ********************************************************************/ 
	case CParameter::CT_MSC:
		eCodingScheme = Parameter.eMSCCodingScheme;
		iN_mux = Parameter.iNoUsefMSCCellsPerFrame;

		/* Data length for part A is the sum of all lengths of the streams */
		iMSCDataLenPartA = Parameter.Stream[0].iLenPartA +
						   Parameter.Stream[1].iLenPartA +
						   Parameter.Stream[2].iLenPartA +
						   Parameter.Stream[3].iLenPartA;

		switch (eCodingScheme)
		{
		case CParameter::CS_2_SM:
			iLevels = 2;

			/* Code rates for prot.-Level A and B for each level */
			for (i = 0; i < 2; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 
					iCodRateCombMSC16SM[Parameter.MSCPrLe.iPartA][i];

				/* Protection Level B */
				iCodeRate[i][1] = 
					iCodRateCombMSC16SM[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ16SM;

			iNoEncBits = iN_mux * 2;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			/* N_1 = ceil(8 * X / (2 * RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = (int) ceil(8 * (_REAL) iMSCDataLenPartA / (2 * 
				/* RY_Icm */
				(_REAL) iCodRateCombMSC16SM[Parameter.MSCPrLe.iPartA][2] *
				(
				/* R_0 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][0]][0] / 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][0]][1] +
				/* R_1 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][1]][0] / 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][1]][1]))) *
				/* RY_Icm */
				iCodRateCombMSC16SM[Parameter.MSCPrLe.iPartA][2];

			/* Check if result can be possible, if not -> correct. This can 
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;

			iN[1] = iN_mux - iN[0];


			/* iM: No of bits each level ------------------------------------ */
			for (i = 0; i < 2; i++)
			{
				/* M_p,1 = 2 * N_1 * R_p */
				iM[i][0] = (int) (2 * iN[0] *
					(_REAL) iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][i]][0] / 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartA][i]][1]);

				/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartB][i]][0] * 
					(int) ((_REAL) (2 * iN[1] - 12) / 
					iPuncturingPatterns[iCodRateCombMSC16SM[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}


			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = iM[0][0] + iM[1][0]; 

			/* Lower protected part */
			iL[1] = iM[0][1] + iM[1][1]; 

			/* Very strong protected part (VSPP) */
			iL[2] = 0; 
			break;

		case CParameter::CS_3_SM:
			iLevels = 3;
	
			/* Code rates for prot.-Level A and B for each level */
			for (i = 0; i < 3; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 
					iCodRateCombMSC64SM[Parameter.MSCPrLe.iPartA][i];

				/* Protection Level B */
				iCodeRate[i][1] = 
					iCodRateCombMSC64SM[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ64SM;
	
			iNoEncBits = iN_mux * 2;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			/* N_1 = ceil(8 * X / (2 * RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = (int) ceil(8 * (_REAL) iMSCDataLenPartA / (2 * 
				/* RY_Icm */
				(_REAL) iCodRateCombMSC64SM[Parameter.MSCPrLe.iPartA][3] *
				(
				/* R_0 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][0]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][0]][1] +
				/* R_1 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][1]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][1]][1] +
				/* R_2 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][2]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][2]][1]))) *
				/* RY_Icm */
				iCodRateCombMSC64SM[Parameter.MSCPrLe.iPartA][3];

			/* Check if result can be possible, if not -> correct. This can 
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;
			
			iN[1] = iN_mux - iN[0];
	

			/* iM: No of bits each level ------------------------------------ */
			for (i = 0; i < 3; i++)
			{
				/* M_p,1 = 2 * N_1 * R_p */
				iM[i][0] = (int) (2 * iN[0] *
					(_REAL) iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][i]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartA][i]][1]);

				/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartB][i]][0] * 
					(int) ((_REAL) (2 * iN[1] - 12) / 
					iPuncturingPatterns[iCodRateCombMSC64SM[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}


			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */ 
			iL[0] = iM[0][0] + iM[1][0] + iM[2][0]; 

			/* Lower protected part */
			iL[1] = iM[0][1] + iM[1][1] + iM[2][1]; 

			/* Very strong protected part (VSPP) */
			iL[2] = 0; 
			break;

		case CParameter::CS_3_HMSYM:
			iLevels = 3;
	
			/* Code rates for prot.-Level A and B for each level */
			/* VSPP (Hierachical) */
			iCodeRate[0][0] = 0;
			iCodeRate[0][1] = 
				iCodRateCombMSC64HMsym[Parameter.MSCPrLe.iHierarch][0];

			for (i = 1; i < 3; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 
					iCodRateCombMSC64HMsym[Parameter.MSCPrLe.iPartA][i];

				/* Protection Level B */
				iCodeRate[i][1] = 
					iCodRateCombMSC64HMsym[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ64HMsym;
	
			iNoEncBits = iN_mux * 2;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			/* N_1 = ceil(8 * X / (2 * RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = (int) ceil(8 * (_REAL) iMSCDataLenPartA / (2 * 
				/* RY_Icm */
				(_REAL) iCodRateCombMSC64HMsym[Parameter.MSCPrLe.iPartA][3] *
				(
				/* R_1 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][1]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][1]][1] +
				/* R_2 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][2]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][2]][1]))) *
				/* RY_Icm */
				iCodRateCombMSC64HMsym[Parameter.MSCPrLe.iPartA][3];

			/* Check if result can be possible, if not -> correct. This can 
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;
			
			iN[1] = iN_mux - iN[0];


			/* iM: No of bits each level ------------------------------------ */
			/* Level 0, contains the VSPP, treated differently */
			/* M_0,1 */
			iM[0][0] = 0;

			/* M_0,2 = RX_0 * floor((2 * (N_1 + N_2) - 12) / RY_0) */
			iM[0][1] = 
				iPuncturingPatterns[iCodRateCombMSC64HMsym[
				Parameter.MSCPrLe.iHierarch][0]][0] *
				(int) ((_REAL) (2 * (iN[0] + iN[1]) - 12) /
				iPuncturingPatterns[iCodRateCombMSC64HMsym[
				Parameter.MSCPrLe.iHierarch][0]][1]);

			for (i = 1; i < 3; i++)
			{
				/* M_p,1 = 2 * N_1 * R_p */
				iM[i][0] = (int) (2 * iN[0] *
					(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][i]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartA][i]][1]);

				/* M_p,2 = RX_p * floor((2 * N_2 - 12) / RY_p) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartB][i]][0] * 
					(int) ((_REAL) (2 * iN[1] - 12) / 
					iPuncturingPatterns[iCodRateCombMSC64HMsym[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}


			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = iM[1][0] + iM[2][0];
				
			/* Lower protected part */
			iL[1] = iM[1][1] + iM[2][1];

			/* Very strong protected part (VSPP) */
			iL[2] = iM[0][1];
			break;
	
		case CParameter::CS_3_HMMIX:
			iLevels = 6;
	
			/* Code rates for prot.-Level A and B for each level */
			/* VSPP (Hierachical) */
			iCodeRate[0][0] = 0;
			iCodeRate[0][1] = 
				iCodRateCombMSC64HMmix[Parameter.MSCPrLe.iHierarch][0];

			for (i = 1; i < 6; i++)
			{
				/* Protection Level A */
				iCodeRate[i][0] = 
					iCodRateCombMSC64HMmix[Parameter.MSCPrLe.iPartA][i];

				/* Protection Level B */
				iCodeRate[i][1] = 
					iCodRateCombMSC64HMmix[Parameter.MSCPrLe.iPartB][i];
			}

			/* Define interleaver sequence for all levels */
			piInterlSequ = iInterlSequ64HMmix;
	
			iNoEncBits = iN_mux;


			/* iN: No of OFDM-cells of each protection level ---------------- */
			/* N_1 = ceil(8 * X / (RY_Icm * sum(R_p)) * RY_Icm */
			iN[0] = (int) ceil(8 * (_REAL) iMSCDataLenPartA / ( 
				/* RY_Icm */
				(_REAL) iCodRateCombMSC64HMmix[Parameter.MSCPrLe.iPartA][6] *
				(
				/* R_1 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][1]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][1]][1] +
				/* R_2 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][2]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][2]][1] +
				/* R_3 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][3]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][3]][1] +
				/* R_4 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][4]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][4]][1] +
				/* R_5 */
				(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][5]][0] / 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][5]][1]))) *
				/* RY_Icm */
				iCodRateCombMSC64HMmix[Parameter.MSCPrLe.iPartA][6];

			/* Check if result can be possible, if not -> correct. This can 
			   happen, if a wrong number is in "Param.Stream[x].iLenPartA" */
			if (iN[0] > iN_mux)
				iN[0] = 0;

			iN[1] = iN_mux - iN[0];


			/* iM: No of bits each level ------------------------------------ */
			/* Real-parts of level 0, they contain the VSPP and treated 
			   differently */
			/* M_0,1Re */
			iM[0][0] = 0;

			/* M_0,2Re = RX_0Re * floor((N_1 + N_2 - 12) / RY_0Re) */
			iM[0][1] = 
				iPuncturingPatterns[iCodRateCombMSC64HMmix[
				Parameter.MSCPrLe.iHierarch][0]][0] *
				(int) ((_REAL) (iN[0] + iN[1] - 12) /
				iPuncturingPatterns[iCodRateCombMSC64HMmix[
				Parameter.MSCPrLe.iHierarch][0]][1]);

			for (i = 1; i < 6; i++)
			{
				/* M_p,1Re;Im = 2 * N_1 * R_pRe;Im */
				iM[i][0] = (int) (iN[0] * 
					(_REAL) iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][i]][0] /
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartA][i]][1]);

				/* M_p,2Re;Im = 
				   RX_pRe;Im * floor((2 * N_2 - 12) / RY_pRe;Im) */
				iM[i][1] = 
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartB][i]][0] *
					(int) ((_REAL) (iN[1] - 12) /
					iPuncturingPatterns[iCodRateCombMSC64HMmix[
					Parameter.MSCPrLe.iPartB][i]][1]);
			}
	

			/* iL: No of bits each protection level ------------------------- */
			/* Higher protected part */
			iL[0] = iM[1][0] + iM[2][0] + iM[3][0] + iM[4][0] + iM[5][0];
				
			/* Lower protected part */
			iL[1] = iM[1][1] + iM[2][1] + iM[3][1] + iM[4][1] + iM[5][1];

			/* Very strong protected part (VSPP) */
			iL[2] = iM[0][1];
			break;
		}

		/* Set No of output bits for next module */
		Parameter.SetNoDecodedBitsMSC(iL[0] + iL[1] + iL[2]);

		/* Set total number of bits for hiearchical frame (needed for MSC 
		   demultiplexer module) */
		Parameter.iNoBitsHierarchFrameTotal = iL[2];
		break;
	}
}
