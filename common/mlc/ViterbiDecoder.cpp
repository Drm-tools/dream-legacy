/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Alexander Kurpiers
 *
 * Description:
 *
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

#include "ViterbiDecoder.h"


/* Implementation *************************************************************/
_REAL CViterbiDecoder::Decode(CVector<CDistance>& vecNewDistance,
							  CVector<_BINARY>& vecbiOutputBits)
{
	int				i, j, k;
	int				iMinMetricIndex;
	int				iDistCnt;
	int				iDistCntGlob;
	_VITMETRTYPE	rAccMetricPrev0;
	_VITMETRTYPE	rAccMetricPrev1;
	_VITMETRTYPE	rMinMetric;
	_VITMETRTYPE	rCurMetric;
	_UINT32BIT		iPuncPatShiftRegShifted;
	_UINT32BIT		iPuncPatShiftReg;
	_UINT32BIT		iPunctCounter;
	int				iBitMask;
	CTrellisData*	pCurTrelData;
	CTrellisData*	pOldTrelData;
	CTrellisData*	pTMPTrelData;
	CTrellisData*	pCurTrelDataState;
	CTrellis*		pCurTrelState;

	/* Init pointers for old and new trellis state */
	pCurTrelData = vecTrelData1;
	pOldTrelData = vecTrelData2;

	/* Reset all metrics in the trellis. We initialize all states exept of
	   the zero state with a high metric, because we KNOW that the state "0"
	   is the transmitted state */
	pOldTrelData[0].rMetric = (_VITMETRTYPE) 0.0;
	for (i = 1; i < MC_NO_STATES; i++)
		pOldTrelData[i].rMetric = MC_METRIC_INIT_VALUE;

	/* Reset counter for puncturing and distance (metric) */
	iDistCntGlob = 0;
	iPunctCounter = 0;


	for (i = 0; i < iNumOutBitsWithMemory; i++)
	{
		/* ------------------------------------------------------------------ */
		/* Get all possible metrics for one transition in the trellis ------- */
		/* ------------------------------------------------------------------ */
		/* Prepare puncturing pattern --------------------------------------- */
		if (i < iNumOutBitsPartA)
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
			/* In case of FAC do not use special tailbit-pattern! */
			if ((i < iNumOutBits) || (eChannelType == CParameter::CT_FAC))
			{
				/* Puncturing patterns part B */
				/* Reset counter when beginning part B */
				if (i == iNumOutBitsPartA)
					iPunctCounter = 0;

				/* Refill shift register after a wrap */
				if (iPunctCounter == 0)
					iPuncPatShiftReg = iPartBPat;

				/* Increase puncturing-counter and manage wrap */
				iPunctCounter++;
				if (iPunctCounter == iPartBPatLen)
					iPunctCounter = 0;
			}
			else
			{
				/* Tailbits */
				/* Set tailbit pattern (no counter needed since there ist
				   only one cycle of this pattern) */
				if (i == iNumOutBits)
					iPuncPatShiftReg = iTailBitPat;
			}
		}


		/* Calculate all possible metrics ----------------------------------- */
		/* Try all possible output combinations of the encoder */
		for (k = 0; k < MC_NO_OUTPUT_COMBINATIONS; k++)
		{
			/* For each metric we need the same distance count and puncturing
			   pattern */
			iDistCnt = iDistCntGlob;
			iPuncPatShiftRegShifted = iPuncPatShiftReg;

			rCurMetric = (_VITMETRTYPE) 0.0;
			iBitMask = 1;

			for (j = 0; j < MC_NO_OUTPUT_BITS_PER_STEP; j++)
			{
				/* Mask first bit (LSB) */
				if (iPuncPatShiftRegShifted & 1)
				{
					/* We define the metrics order: [b_3, b_2, b_1, b_0] */
					if (k & iBitMask)
						rCurMetric += vecNewDistance[iDistCnt].rTow1; /* "1" */
					else
						rCurMetric += vecNewDistance[iDistCnt].rTow0; /* "0" */

					iDistCnt++;
				}

				/* Shift puncturing mask for next output bit */
				iPuncPatShiftRegShifted >>= 1;

				/* Shift bit mask */
				iBitMask <<= 1;
			}

			/* Set new value in vector */
			vecrMetricSet[k] = rCurMetric;
		}

		/* Save shifted register and distance count for next loop */
		iPuncPatShiftReg = iPuncPatShiftRegShifted;
		iDistCntGlob = iDistCnt;


		/* ------------------------------------------------------------------ */
		/* Update trellis --------------------------------------------------- */
		/* ------------------------------------------------------------------ */
		/* Init minium metric with large value */
		rMinMetric = MC_METRIC_INIT_VALUE;
		iMinMetricIndex = 0;


#if 0
		for (j = 0; j < MC_NO_STATES; j++)
		{
			/* Get pointer to current state in the trellis */
			pCurTrelState = &vecTrellis[j];
			pCurTrelDataState = &pCurTrelData[j];

			/* Calculate metrics from the two previous states, use the old
			   metric from the previous states plus the "transition-metric" - */
			/* "0" */
			rAccMetricPrev0 =
				pOldTrelData[pCurTrelState->iPrev0Index].rMetric +
				vecrMetricSet[pCurTrelState->iMetricPrev0];

			/* "1" */
			rAccMetricPrev1 =
				pOldTrelData[pCurTrelState->iPrev1Index].rMetric +
				vecrMetricSet[pCurTrelState->iMetricPrev1];


			/* Take path with smallest metric ------------------------------- */
			if (rAccMetricPrev0 < rAccMetricPrev1)
			{
				/* Save minimum metric for this state */
				pCurTrelDataState->rMetric = rAccMetricPrev0;

				/* Save decoded bits from surviving path */
				pCurTrelDataState->lDecodedBits =
					pOldTrelData[pCurTrelState->iPrev0Index].lDecodedBits;

				/* Shift lDecodedBits vector since we want to add a new bit,
				   in this case a "0", but we dont need to add this, it is
				   already there */
				pCurTrelDataState->lDecodedBits <<= 1;
			}
			else
			{
				/* Save minimum metric for this state */
				pCurTrelDataState->rMetric = rAccMetricPrev1;

				/* Save decoded bits from surviving path */
				pCurTrelDataState->lDecodedBits =
					pOldTrelData[pCurTrelState->iPrev1Index].lDecodedBits;

				/* Shift lDecodedBits vector since we want to add a new bit */
				pCurTrelDataState->lDecodedBits <<= 1;

				/* Write resulting "1" in lDecodedBits-vector */
				pCurTrelDataState->lDecodedBits |= 1;
			}


			/* Get minimum metric and index --------------------------------- */
			if (pCurTrelDataState->rMetric < rMinMetric)
			{
				rMinMetric = pCurTrelDataState->rMetric;
				iMinMetricIndex = j;
			}
		}
#else
#define BUTTERFLY( cur, prev0, prev1, met0, met1 ) \
		rAccMetricPrev0 = \
			pOldTrelData[ prev0 ].rMetric + \
			vecrMetricSet[ met0 ]; \
		rAccMetricPrev1 = \
			pOldTrelData[ prev1 ].rMetric + \
			vecrMetricSet[ met1 ]; \
		if (rAccMetricPrev0 < rAccMetricPrev1) \
		{ \
			pCurTrelData[ cur ].rMetric = rAccMetricPrev0; \
			pCurTrelData[ cur ].lDecodedBits = \
				pOldTrelData[ prev0 ].lDecodedBits << 1; \
		} \
		else \
		{ \
			pCurTrelData[ cur ].rMetric = rAccMetricPrev1; \
			pCurTrelData[ cur ].lDecodedBits = \
				( pOldTrelData[ prev1 ].lDecodedBits << 1) | 1; \
		} \
		if (pCurTrelData[ cur ].rMetric < rMinMetric) \
		{ \
			rMinMetric = pCurTrelData[ cur ].rMetric; \
			iMinMetricIndex = cur; \
		}

		BUTTERFLY( 0, 0, 32, 0, 15 )
		BUTTERFLY( 1, 0, 32, 15, 0 )
		BUTTERFLY( 2, 1, 33, 6, 9 )
		BUTTERFLY( 3, 1, 33, 9, 6 )
		BUTTERFLY( 4, 2, 34, 11, 4 )
		BUTTERFLY( 5, 2, 34, 4, 11 )
		BUTTERFLY( 6, 3, 35, 13, 2 )
		BUTTERFLY( 7, 3, 35, 2, 13 )
		BUTTERFLY( 8, 4, 36, 11, 4 )
		BUTTERFLY( 9, 4, 36, 4, 11 )
		BUTTERFLY( 10, 5, 37, 13, 2 )
		BUTTERFLY( 11, 5, 37, 2, 13 )
		BUTTERFLY( 12, 6, 38, 0, 15 )
		BUTTERFLY( 13, 6, 38, 15, 0 )
		BUTTERFLY( 14, 7, 39, 6, 9 )
		BUTTERFLY( 15, 7, 39, 9, 6 )
		BUTTERFLY( 16, 8, 40, 4, 11 )
		BUTTERFLY( 17, 8, 40, 11, 4 )
		BUTTERFLY( 18, 9, 41, 2, 13 )
		BUTTERFLY( 19, 9, 41, 13, 2 )
		BUTTERFLY( 20, 10, 42, 15, 0 )
		BUTTERFLY( 21, 10, 42, 0, 15 )
		BUTTERFLY( 22, 11, 43, 9, 6 )
		BUTTERFLY( 23, 11, 43, 6, 9 )
		BUTTERFLY( 24, 12, 44, 15, 0 )
		BUTTERFLY( 25, 12, 44, 0, 15 )
		BUTTERFLY( 26, 13, 45, 9, 6 )
		BUTTERFLY( 27, 13, 45, 6, 9 )
		BUTTERFLY( 28, 14, 46, 4, 11 )
		BUTTERFLY( 29, 14, 46, 11, 4 )
		BUTTERFLY( 30, 15, 47, 2, 13 )
		BUTTERFLY( 31, 15, 47, 13, 2 )
		BUTTERFLY( 32, 16, 48, 9, 6 )
		BUTTERFLY( 33, 16, 48, 6, 9 )
		BUTTERFLY( 34, 17, 49, 15, 0 )
		BUTTERFLY( 35, 17, 49, 0, 15 )
		BUTTERFLY( 36, 18, 50, 2, 13 )
		BUTTERFLY( 37, 18, 50, 13, 2 )
		BUTTERFLY( 38, 19, 51, 4, 11 )
		BUTTERFLY( 39, 19, 51, 11, 4 )
		BUTTERFLY( 40, 20, 52, 2, 13 )
		BUTTERFLY( 41, 20, 52, 13, 2 )
		BUTTERFLY( 42, 21, 53, 4, 11 )
		BUTTERFLY( 43, 21, 53, 11, 4 )
		BUTTERFLY( 44, 22, 54, 9, 6 )
		BUTTERFLY( 45, 22, 54, 6, 9 )
		BUTTERFLY( 46, 23, 55, 15, 0 )
		BUTTERFLY( 47, 23, 55, 0, 15 )
		BUTTERFLY( 48, 24, 56, 13, 2 )
		BUTTERFLY( 49, 24, 56, 2, 13 )
		BUTTERFLY( 50, 25, 57, 11, 4 )
		BUTTERFLY( 51, 25, 57, 4, 11 )
		BUTTERFLY( 52, 26, 58, 6, 9 )
		BUTTERFLY( 53, 26, 58, 9, 6 )
		BUTTERFLY( 54, 27, 59, 0, 15 )
		BUTTERFLY( 55, 27, 59, 15, 0 )
		BUTTERFLY( 56, 28, 60, 6, 9 )
		BUTTERFLY( 57, 28, 60, 9, 6 )
		BUTTERFLY( 58, 29, 61, 0, 15 )
		BUTTERFLY( 59, 29, 61, 15, 0 )
		BUTTERFLY( 60, 30, 62, 13, 2 )
		BUTTERFLY( 61, 30, 62, 2, 13 )
		BUTTERFLY( 62, 31, 63, 11, 4 )
		BUTTERFLY( 63, 31, 63, 4, 11 )

#undef BUTTERFLY
#endif

		/* Save decoded bit, but not before tailbits are used. Mask bit at
		   the defined position in "lDecodedBits" by "MC_DECODING_DEPTH" */
		if (i >= iTotalDecDepth)
		{
			/* Mask bit */
			if ((pCurTrelData[iMinMetricIndex].lDecodedBits &
				lOutBitMask) > 0)
			{
				vecbiOutputBits[i - iTotalDecDepth] = TRUE;
			}
			else
				vecbiOutputBits[i - iTotalDecDepth] = FALSE;
		}

		/* Swap trellis data pointers (old -> new, new -> old) */
		pTMPTrelData = pCurTrelData;
		pCurTrelData = pOldTrelData;
		pOldTrelData = pTMPTrelData;
	}

	/* Save last "MC_DECODING_DEPTH" bits. We use trellis state "0", because we
	   KNOW that this is our last state (shift registers in the coder are filled
	   with zeros at the end) */
	for (i = 0; i < MC_DECODING_DEPTH; i++)
	{
		if ((pOldTrelData[0].lDecodedBits &
			(_UINT64BIT(1) << MC_DECODING_DEPTH - i - 1)) /* Mask bit */ > 0)
		{
			vecbiOutputBits[iNumOutBits - MC_DECODING_DEPTH + i] = TRUE;
		}
		else
			vecbiOutputBits[iNumOutBits - MC_DECODING_DEPTH + i] = FALSE;
	}

	/* Return normalized accumulated minimum metric */
	return rMinMetric / iDistCntGlob;
}

void CViterbiDecoder::Init(CParameter::ECodScheme eNewCodingScheme,
						   CParameter::EChanType eNewChannelType, int iN1,
						   int iN2, int iNewNumOutBitsPartA,
						   int iNewNumOutBitsPartB, int iPunctPatPartA,
						   int iPunctPatPartB, int iLevel)
{
	int	iTailbitPattern;
	int	iTailbitParamL0;
	int	iTailbitParamL1;

	/* Set number of out bits and save channel type */
	iNumOutBitsPartA = iNewNumOutBitsPartA;
	eChannelType = eNewChannelType;

	/* Number of bits out is the sum of all protection levels */
	iNumOutBits = iNumOutBitsPartA + iNewNumOutBitsPartB;

	/* Number of out bits including the state memory */
	iNumOutBitsWithMemory = iNumOutBits + MC_CONSTRAINT_LENGTH - 1;

	/* Set output mask for bits (constant "1" must be casted to 64 bit interger,
	   otherwise we would get "0") */
	lOutBitMask = _UINT64BIT(1) << MC_DECODING_DEPTH;


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

CViterbiDecoder::CViterbiDecoder()
{
	CConvEncoder ConvEncoder; /* Needed for convolution method */

	/* Total decoder depth */
	iTotalDecDepth = MC_CONSTRAINT_LENGTH - 1 + MC_DECODING_DEPTH;


	/* Create trellis *********************************************************/
	for (int i = 0; i < MC_NO_STATES; i++)
	{
		/* Define previous states ------------------------------------------- */
		/* We define in this trellis that we shift the bits from right to
		   the left. We use the transition-bits which "fall" out of the
		   shift-register */
		vecTrellis[i].iPrev0Index = (i >> 1); /* Old state, Leading "0"
												 (automatically inserted by
												 shifting */
		vecTrellis[i].iPrev1Index = (i >> 1)	 /* Old state */
			| (1 << (MC_CONSTRAINT_LENGTH - 2)); /* New "1", must be on position
													MC_CONSTRAINT_LENGTH - 1 */


		/* Assign metrics to the transitions from both paths ---------------- */
		/* We define with the metrics the order: [b_3, b_2, b_1, b_0] */
		vecTrellis[i].iMetricPrev0 = 0;
		vecTrellis[i].iMetricPrev1 = 0;
		for (int j = 0; j < MC_NO_OUTPUT_BITS_PER_STEP; j++)
		{
			/* Calculate respective metrics from convolution of state and
			   transition bit */
			/* "0" */
			vecTrellis[i].iMetricPrev0 |=
				ConvEncoder.Convolution(
				/* Set all states in the shift-register for encoder. Use
				   current state with a leading "0" (which is automatically
				   there */
				i
				/* Use generator-polynomial j */
				, j) 
				/* Shift generated bit to the correct position */
				<< j;

			/* "1" */
			vecTrellis[i].iMetricPrev1 |=
				ConvEncoder.Convolution(
				/* Set all states in the shift-register for encoder. Use
				   current state with a leading "1". The position of this
				   bit is "MC_CONSTRAINT_LENGTH" (shifting from position 1:
				   "MC_CONSTRAINT_LENGTH - 1") */
				i | (1 << (MC_CONSTRAINT_LENGTH - 1))
				/* Use generator-polynomial j */
				, j)
				/* Shift generated bit to the correct position */
				<< j;
		}
	}
}
