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
	int				iDistCnt;
	int				iMinMetricIndex;
	_VITMETRTYPE	rAccMetricPrev0;
	_VITMETRTYPE	rAccMetricPrev1;
	_VITMETRTYPE	rMinMetric;
	_VITMETRTYPE	rCurMetric;
	CTrellisData*	pCurTrelData;
	CTrellisData*	pOldTrelData;

	/* Init pointers for old and new trellis state */
	pCurTrelData = vecTrelData1;
	pOldTrelData = vecTrelData2;

	/* Reset all metrics in the trellis. We initialize all states exept of
	   the zero state with a high metric, because we KNOW that the state "0"
	   is the transmitted state */
	pOldTrelData[0].rMetric = (_VITMETRTYPE) 0;
	for (i = 1; i < MC_NO_STATES; i++)
		pOldTrelData[i].rMetric = MC_METRIC_INIT_VALUE;

	/* Reset counter for puncturing and distance (from metric) */
	iDistCnt = 0;


	for (i = 0; i < iNumOutBitsWithMemory; i++)
	{
		/* Calculate all possible metrics ----------------------------------- */
		/* There are only a small set of possible puncturing patterns used for
		   DRM: 0001, 0101, 0011, 0111, 1111. These need different numbers of
		   input bits (increment of "iDistCnt" is dependent on pattern!). To
		   optimize the calculation of the metrics, a "subset" of bits are first
		   calculated which are used to get the final result. In this case,
		   redundancy can be avoided.
		   Note, that not all possible bit-combinations are used in the coder,
		   only a subset of numbers: 0, 2, 4, 6, 9, 11, 13, 15 (compare numbers
		   in the BUTTERFLY( ) calls) */

		/* Get first position in input vector (is needed for all cases) */
		const int iPos0 = iDistCnt;
		iDistCnt++;

		if (veciTablePuncPat[i] == PP_TYPE_0001)
		{
			/* Pattern 0001 */
			vecrMetricSet[ 0] = vecNewDistance[iPos0].rTow0;
			vecrMetricSet[ 2] = vecNewDistance[iPos0].rTow0;
			vecrMetricSet[ 4] = vecNewDistance[iPos0].rTow0;
			vecrMetricSet[ 6] = vecNewDistance[iPos0].rTow0;
			vecrMetricSet[ 9] = vecNewDistance[iPos0].rTow1;
			vecrMetricSet[11] = vecNewDistance[iPos0].rTow1;
			vecrMetricSet[13] = vecNewDistance[iPos0].rTow1;
			vecrMetricSet[15] = vecNewDistance[iPos0].rTow1;
		}
		else
		{
			/* The following patterns need one more bit */
			const int iPos1 = iDistCnt;
			iDistCnt++;

			/* Calculate "subsets" of bit-combinations. "rIRxx00" means that
			   the fist two bits are used, others are x-ed. "IR" stands for
			   "intermediate result" */
			_VITMETRTYPE rIRxx00 =
				vecNewDistance[iPos1].rTow0 + vecNewDistance[iPos0].rTow0;
			_VITMETRTYPE rIRxx10 =
				vecNewDistance[iPos1].rTow1 + vecNewDistance[iPos0].rTow0;
			_VITMETRTYPE rIRxx01 =
				vecNewDistance[iPos1].rTow0 + vecNewDistance[iPos0].rTow1;
			_VITMETRTYPE rIRxx11 =
				vecNewDistance[iPos1].rTow1 + vecNewDistance[iPos0].rTow1;

			if (veciTablePuncPat[i] == PP_TYPE_0101)
			{
				/* Pattern 0101 */
				vecrMetricSet[ 0] = rIRxx00;
				vecrMetricSet[ 2] = rIRxx00;
				vecrMetricSet[ 4] = rIRxx10;
				vecrMetricSet[ 6] = rIRxx10;
				vecrMetricSet[ 9] = rIRxx01;
				vecrMetricSet[11] = rIRxx01;
				vecrMetricSet[13] = rIRxx11;
				vecrMetricSet[15] = rIRxx11;
			}
			else if (veciTablePuncPat[i] == PP_TYPE_0011)
			{
				/* Pattern 0011 */
				vecrMetricSet[ 0] = rIRxx00;
				vecrMetricSet[ 2] = rIRxx10;
				vecrMetricSet[ 4] = rIRxx00;
				vecrMetricSet[ 6] = rIRxx10;
				vecrMetricSet[ 9] = rIRxx01;
				vecrMetricSet[11] = rIRxx11;
				vecrMetricSet[13] = rIRxx01;
				vecrMetricSet[15] = rIRxx11;
			}
			else
			{
				/* The following patterns need one more bit */
				const int iPos2 = iDistCnt;
				iDistCnt++;

				if (veciTablePuncPat[i] == PP_TYPE_0111)
				{
					/* Pattern 0111 */
					vecrMetricSet[ 0] = vecNewDistance[iPos2].rTow0 + rIRxx00;
					vecrMetricSet[ 2] = vecNewDistance[iPos2].rTow0 + rIRxx10;
					vecrMetricSet[ 4] = vecNewDistance[iPos2].rTow1 + rIRxx00;
					vecrMetricSet[ 6] = vecNewDistance[iPos2].rTow1 + rIRxx10;
					vecrMetricSet[ 9] = vecNewDistance[iPos2].rTow0 + rIRxx01;
					vecrMetricSet[11] = vecNewDistance[iPos2].rTow0 + rIRxx11;
					vecrMetricSet[13] = vecNewDistance[iPos2].rTow1 + rIRxx01;
					vecrMetricSet[15] = vecNewDistance[iPos2].rTow1 + rIRxx11;
				}
				else
				{
					/* Pattern 1111 */
					/* This pattern needs all four bits */
					const int iPos3 = iDistCnt;
					iDistCnt++;

					/* Calculate "subsets" of bit-combinations. "rIRxx00" means
					   that the last two bits are used, others are x-ed.
					   "IR" stands for "intermediate result" */
					_VITMETRTYPE rIR00xx = vecNewDistance[iPos3].rTow0 +
						vecNewDistance[iPos2].rTow0;
					_VITMETRTYPE rIR10xx = vecNewDistance[iPos3].rTow1 +
						vecNewDistance[iPos2].rTow0;
					_VITMETRTYPE rIR01xx = vecNewDistance[iPos3].rTow0 +
						vecNewDistance[iPos2].rTow1;
					_VITMETRTYPE rIR11xx = vecNewDistance[iPos3].rTow1 +
						vecNewDistance[iPos2].rTow1;

					vecrMetricSet[ 0] = rIR00xx + rIRxx00; /* 0 */
					vecrMetricSet[ 2] = rIR00xx + rIRxx10; /* 2 */
					vecrMetricSet[ 4] = rIR01xx + rIRxx00; /* 4 */
					vecrMetricSet[ 6] = rIR01xx + rIRxx10; /* 6 */
					vecrMetricSet[ 9] = rIR10xx + rIRxx01; /* 9 */
					vecrMetricSet[11] = rIR10xx + rIRxx11; /* 11 */
					vecrMetricSet[13] = rIR11xx + rIRxx01; /* 13 */
					vecrMetricSet[15] = rIR11xx + rIRxx11; /* 15 */
				}
			}
		}


		/* ------------------------------------------------------------------ */
		/* Update trellis --------------------------------------------------- */
		/* ------------------------------------------------------------------ */
		/* Init minium metric with large value */
		rMinMetric = MC_METRIC_INIT_VALUE;
		iMinMetricIndex = 0;

#define BUTTERFLY( cur, prev0, prev1, met0, met1 ) \
		{ \
			/* Calculate metrics from the two previous states, use the old
			   metric from the previous states plus the "transition-metric" */ \
			/* "0" */ \
			rAccMetricPrev0 = \
				pOldTrelData[ prev0 ].rMetric + \
				vecrMetricSet[ met0 ]; \
			\
			/* "1" */ \
			rAccMetricPrev1 = \
				pOldTrelData[ prev1 ].rMetric + \
				vecrMetricSet[ met1 ]; \
			\
			/* Take path with smallest metric ----------------------------- */ \
			if (rAccMetricPrev0 < rAccMetricPrev1) \
			{ \
				/* Save minimum metric for this state */ \
				pCurTrelData[ cur ].rMetric = rAccMetricPrev0; \
				\
				/* Save decoded bits from surviving path and shift lDecodedBits
				   vector (<< 1) since we want to add a new bit, in this case a
				   "0", but we dont need to add this, it is already there */ \
				pCurTrelData[ cur ].lDecodedBits = \
					pOldTrelData[ prev0 ].lDecodedBits << 1; \
			} \
			else \
			{ \
				/* Save minimum metric for this state */ \
				pCurTrelData[ cur ].rMetric = rAccMetricPrev1; \
				\
				/* Save decoded bits from surviving path and shift lDecodedBits
				   vector (<< 1) since we want to add a new bit. Then write
				   resulting "1" in lDecodedBits-vector (with | 1) */ \
				pCurTrelData[ cur ].lDecodedBits = \
					( pOldTrelData[ prev1 ].lDecodedBits << 1) | 1; \
			} \
			\
			/* Get minimum metric and index ------------------------------- */ \
			if (pCurTrelData[ cur ].rMetric < rMinMetric) \
			{ \
				rMinMetric = pCurTrelData[ cur ].rMetric; \
				iMinMetricIndex = cur; \
			} \
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
		CTrellisData* pTMPTrelData = pCurTrelData;
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
	return rMinMetric / iDistCnt;
}

void CViterbiDecoder::Init(CParameter::ECodScheme eNewCodingScheme,
						   CParameter::EChanType eNewChannelType, int iN1,
						   int iN2, int iNewNumOutBitsPartA,
						   int iNewNumOutBitsPartB, int iPunctPatPartA,
						   int iPunctPatPartB, int iLevel)
{
	/* Number of bits out is the sum of all protection levels */
	iNumOutBits = iNewNumOutBitsPartA + iNewNumOutBitsPartB;

	/* Number of out bits including the state memory */
	iNumOutBitsWithMemory = iNumOutBits + MC_CONSTRAINT_LENGTH - 1;

	/* Set output mask for bits (constant "1" must be casted to 64 bit interger,
	   otherwise we would get "0") */
	lOutBitMask = _UINT64BIT(1) << MC_DECODING_DEPTH;

	/* Init vector, storing table for puncturing pattern and generate pattern */
	veciTablePuncPat.Init(iNumOutBitsWithMemory);

	veciTablePuncPat = GenPuncPatTable(eNewCodingScheme, eNewChannelType, iN1,
		iN2, iNewNumOutBitsPartA, iNewNumOutBitsPartB, iPunctPatPartA,
		iPunctPatPartB, iLevel);
}

CViterbiDecoder::CViterbiDecoder()
{
	/* Total decoder depth */
	iTotalDecDepth = MC_CONSTRAINT_LENGTH - 1 + MC_DECODING_DEPTH;

#if 0
	/* Create trellis *********************************************************/
	/* Activate this code to generate the table needed for the butterfly calls
	   in the processing routine */

	/* We need to analyze 2^(MC_CONSTRAINT_LENGTH - 1) states in the trellis */
	int				i;
	int				iPrev0Index[MC_NO_STATES];
	int				iPrev1Index[MC_NO_STATES];
	int				iMetricPrev0[MC_NO_STATES];
	int				iMetricPrev1[MC_NO_STATES];

	for (i = 0; i < MC_NO_STATES; i++)
	{
		/* Define previous states ------------------------------------------- */
		/* We define in this trellis that we shift the bits from right to
		   the left. We use the transition-bits which "fall" out of the
		   shift-register */
		iPrev0Index[i] = (i >> 1);				/* Old state, Leading "0"
												   (automatically inserted by
												   shifting */
		iPrev1Index[i] = (i >> 1)				 /* Old state */
			| (1 << (MC_CONSTRAINT_LENGTH - 2)); /* New "1", must be on position
													MC_CONSTRAINT_LENGTH - 1 */


		/* Assign metrics to the transitions from both paths ---------------- */
		/* We define with the metrics the order: [b_3, b_2, b_1, b_0] */
		iMetricPrev0[i] = 0;
		iMetricPrev1[i] = 0;
		for (int j = 0; j < MC_NO_OUTPUT_BITS_PER_STEP; j++)
		{
			/* Calculate respective metrics from convolution of state and
			   transition bit */
			/* "0" */
			iMetricPrev0[i] |= Convolution(
				/* Set all states in the shift-register for encoder. Use
				   current state with a leading "0" (which is automatically
				   there */
				i
				/* Use generator-polynomial j */
				, j) 
				/* Shift generated bit to the correct position */
				<< j;

			/* "1" */
			iMetricPrev1[i] |= Convolution(
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

	/* Save trellis in file (for substituting number directly in the code) */
	static FILE* pFile = fopen("test/trellis.dat", "w");
	for (i = 0; i < MC_NO_STATES; i++)
		fprintf(pFile, "BUTTERFLY( %d, %d, %d, %d, %d )\n", i,
			iPrev0Index[i], iPrev1Index[i],
			iMetricPrev0[i], iMetricPrev1[i]);
	fflush(pFile);
	exit(1);
#endif
}
