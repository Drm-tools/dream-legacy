/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	
 *	Note: We always shift the bits towards the MSB 
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

#include "ConvEncoder.h"


/* Implementation *************************************************************/
int CConvEncoder::Encode(CVector<_BINARY>& vecInputData, 
						 CVector<_BINARY>& vecOutputData)
{
	int				i, k;
	int				iOutputCounter;
	_UINT32BIT		iPunctCounter;
	int				iTailbitPattern;
	_UINT32BIT		iPuncPatShiftReg;
	_BYTE			byStateShiftReg;
	int				iNoBitsRegPunct;

	/* Set output size to zero, increment it each time a new bit is encoded */
	iOutputCounter = 0;

	/* Reset counter for puncturing and state-register */
	iPunctCounter = 0;
	byStateShiftReg = 0;

	/* FAC with no special tailbit pattern */
	if (eChannelType == CParameter::CT_FAC)
		iNoBitsRegPunct = iNumInBitsWithMemory;
	else
		iNoBitsRegPunct = iNumInBits;

	for (i = 0; i < iNoBitsRegPunct; i++)
	{
		/* Prepare puncturing pattern --------------------------------------- */
		if (i < iNumInBitsPartA)
		{
			/* Puncturing patterns part A */
			/* Refill shift register after a wrap */
			if (iPunctCounter == 0)
				iPuncPatShiftReg = iPartAPat;

			/* Increase puncturing-counter and manage wrap */
			iPunctCounter++;
			if (iPunctCounter == iPartAPatLen)
				iPunctCounter = 0;
		}
		else
		{
			/* Puncturing patterns part B */
			/* Reset counter when beginning part B */
			if (i == iNumInBitsPartA)
				iPunctCounter = 0;

			/* Refill shift register after a wrap */
			if (iPunctCounter == 0)
				iPuncPatShiftReg = iPartBPat;

			/* Increase puncturing-counter and manage wrap */
			iPunctCounter++;
			if (iPunctCounter == iPartBPatLen)
				iPunctCounter = 0;
		}


		/* Update shift-register (state information) ------------------------ */
		/* Shift bits in state-shift-register */
		byStateShiftReg <<= 1;

		/* In case of FAC, the tailbits must be calculated in this loop. Check
		   when end of vector is reached and no more bits must be added */
		if (i < iNumInBits)
		{
			/* Add new bit at the beginning */
			if (vecInputData[i] == TRUE)
				byStateShiftReg |= 1;
		}

		for (k = 0; k < MC_NO_OUTPUT_BITS_PER_STEP; k++)
		{
			/* Puncturing --------------------------------------------------- */
			/* Mask first bit (LSB) */
			if (iPuncPatShiftReg & 1)
			{
				/* Calculate convolution and put result in transitory 
				   output vector */
				vecOutputData[iOutputCounter] = Convolution(byStateShiftReg, k);

				/* Set new fill-level */
				iOutputCounter++;
			}

			/* Shift puncturing mask for next output bit */
			iPuncPatShiftReg >>= 1;
		}
	}


	/* Tailbits ***************************************************************/
	/* Tailbit patterns are NOT USED with FAC! */
	if (eChannelType != CParameter::CT_FAC)
	{
		/* Fill shift register for the first time */
		iPuncPatShiftReg = iTailBitPat;
	
		for (i = 0; i < MC_CONSTRAINT_LENGTH - 1; i++)
		{
			/* Update shift-register (state information) -------------------- */
			/* Shift bits in state-shift-register (implicitely a 0 is added) */
			byStateShiftReg <<= 1;
	
			for (k = 0; k < MC_NO_OUTPUT_BITS_PER_STEP; k++)
			{
				/* Puncturing for tailbits ---------------------------------- */
				/* Mask first bit (LSB) */
				if (iPuncPatShiftReg & 1)
				{
					/* Calculate convolution */
					vecOutputData[iOutputCounter] = 
						Convolution(byStateShiftReg, k);
	
					/* Set new fill-level */
					iOutputCounter++;
				}
	
				/* Shift puncturing mask for next output bit */
				iPuncPatShiftReg >>= 1;
			}
		}	
	}

	/* Return number of encoded bits */
	return iOutputCounter;
}

_BINARY CConvEncoder::Convolution(_BYTE byNewStateShiftReg, int iGenPolyn) const
{
	_BINARY	binResult;
	_BYTE	byResult;

	/* Mask bits with generator polynomial */
	byResult = byNewStateShiftReg & byGeneratorMatrix[iGenPolyn];

	/* XOR all bits in byResult.
	   We observe always the LSB by masking using operator "& 1". To get 
	   access to all bits in "byResult" we shift the current bit so long
	   until it reaches the mask (at zero) by using operator ">> i". The
	   actual XOR operation is done by "^=" */
	binResult = FALSE;
	for (int i = 0; i < MC_CONSTRAINT_LENGTH; i++)
		binResult ^= (byResult >> i) & 1;

	return binResult;
}

void CConvEncoder::Init(CParameter::ECodScheme eNewCodingScheme,
						CParameter::EChanType eNewChannelType, int iN1, 
						int iN2, int iNewNumInBitsPartA,
						int iNewNumInBitsPartB, int iPunctPatPartA,
						int iPunctPatPartB, int iLevel)
{
	int	iTailbitPattern;
	int	iTailbitParamL0;
	int	iTailbitParamL1;

	/* Set number of out bits and save channel type */
	iNumInBitsPartA = iNewNumInBitsPartA;
	eChannelType = eNewChannelType;

	/* Number of bits out is the sum of all protection levels */
	iNumInBits = iNumInBitsPartA + iNewNumInBitsPartB;

	/* Number of out bits including the state memory */
	iNumInBitsWithMemory = iNumInBits + MC_CONSTRAINT_LENGTH - 1;


	/* Set puncturing bit patterns and lengths ------------------------------ */
	iPartAPat = iPuncturingPatterns[iPunctPatPartA][2];
	iPartBPat = iPuncturingPatterns[iPunctPatPartB][2];
	iPartAPatLen = iPuncturingPatterns[iPunctPatPartA][0];
	iPartBPatLen = iPuncturingPatterns[iPunctPatPartB][0];

	
	/* Set tail-bit pattern ------------------------------------------------- */
	/* We have to consider two cases because in HSYM "N1 + N2" is used
	   instead of only "N2" to calculate the tailbit pattern */
	switch (eNewCodingScheme)
	{
	case CParameter::CS_3_HMMIX:
		iTailbitParamL0 = iN1 + iN2;
		iTailbitParamL1 = iN2;
		break;

	case CParameter::CS_3_HMSYM:
		iTailbitParamL0 = 2 * (iN1 + iN2);
		iTailbitParamL1 = 2 * iN2;
		break;

	default:
		iTailbitParamL0 = 2 * iN2;
		iTailbitParamL1 = 2 * iN2;
	}

	/* Tailbit pattern calculated according DRM-standard. We have to consider
	   two cases because in HSYM "N1 + N2" is used instead of only "N2" */
	if (iLevel == 0)
		iTailbitPattern =
			iTailbitParamL0 - 12 - iPuncturingPatterns[iPunctPatPartB][1] *
			(int) ((iTailbitParamL0 - 12) /
			iPuncturingPatterns[iPunctPatPartB][1]);
	else
		iTailbitPattern =
			iTailbitParamL1 - 12 - iPuncturingPatterns[iPunctPatPartB][1] *
			(int) ((iTailbitParamL1 - 12) /
			iPuncturingPatterns[iPunctPatPartB][1]);

	iTailBitPat = iPunctPatTailbits[iTailbitPattern];
}
