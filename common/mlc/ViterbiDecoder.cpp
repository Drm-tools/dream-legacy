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
	int				i;
	int				iDistCnt;
	int				iCurDecState;
	_VITMETRTYPE	rAccMetricPrev0;
	_VITMETRTYPE	rAccMetricPrev1;
	_VITMETRTYPE*	pCurTrelMetric;
	_VITMETRTYPE*	pOldTrelMetric;

#ifdef USE_MMX
	/* Fields for storing the reodered metrics for MMX trellis */
	unsigned char	chMet1[MC_NUM_STATES / 2];
	unsigned char	chMet2[MC_NUM_STATES / 2];


	/* -------------------------------------------------------------------------
	   Since the metric is 8-bit fixed-point type, we need to scale the input
	   metrics to avoid overflows */
	/* Calculate average value of input metrics */
	_REAL rAverage = (_REAL) 0.0;
	for (i = 0; i < iNumOutBitsWithMemory; i++)
	{
		rAverage += vecNewDistance[i].rTow0;
		rAverage += vecNewDistance[i].rTow1;
	}

	/* Scale input metrics */
	const _REAL rAmp = rAverage / (2 * iNumOutBitsWithMemory) / 10;
	for (i = 0; i < iNumOutBitsWithMemory; i++)
	{
		vecNewDistance[i].rTow0 /= rAmp;
		vecNewDistance[i].rTow1 /= rAmp;
	}
#endif

	/* Init pointers for old and new trellis state */
	pCurTrelMetric = vecTrelMetric1;
	pOldTrelMetric = vecTrelMetric2;

	/* Reset all metrics in the trellis. We initialize all states exept of
	   the zero state with a high metric, because we KNOW that the state "0"
	   is the transmitted state */
	pOldTrelMetric[0] = (_VITMETRTYPE) 0;
	for (i = 1; i < MC_NUM_STATES; i++)
		pOldTrelMetric[i] = MC_METRIC_INIT_VALUE;

	/* Reset counter for puncturing and distance (from metric) */
	iDistCnt = 0;


	/* Main loop over all bits ---------------------------------------------- */
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
			_REAL rIRxx00 =
				vecNewDistance[iPos1].rTow0 + vecNewDistance[iPos0].rTow0;
			_REAL rIRxx10 =
				vecNewDistance[iPos1].rTow1 + vecNewDistance[iPos0].rTow0;
			_REAL rIRxx01 =
				vecNewDistance[iPos1].rTow0 + vecNewDistance[iPos0].rTow1;
			_REAL rIRxx11 =
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
					_REAL rIR00xx = vecNewDistance[iPos3].rTow0 +
						vecNewDistance[iPos2].rTow0;
					_REAL rIR10xx = vecNewDistance[iPos3].rTow1 +
						vecNewDistance[iPos2].rTow0;
					_REAL rIR01xx = vecNewDistance[iPos3].rTow0 +
						vecNewDistance[iPos2].rTow1;
					_REAL rIR11xx = vecNewDistance[iPos3].rTow1 +
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


		/* Update trellis --------------------------------------------------- */
#ifdef USE_MMX
		/* Use the butterfly unroll for reordering the metrics for MMX
		   trellis */
#define BUTTERFLY(cur, next, prev0, prev1, met0, met1) \
		{ \
			/* At this point we convert from float to char! No overflow-check
			   is done here */ \
			chMet1[prev0] = (_VITMETRTYPE) vecrMetricSet[met0]; \
			chMet2[prev0] = (_VITMETRTYPE) vecrMetricSet[met1]; \
		}
#else
		/* c++ version of trellis update */
#define BUTTERFLY(cur, next, prev0, prev1, met0, met1) \
		{ \
			/* First state in this set ------------------------------------ */ \
			/* Calculate metrics from the two previous states, use the old
			   metric from the previous states plus the "transition-metric" */ \
			rAccMetricPrev0 = pOldTrelMetric[prev0] + vecrMetricSet[met0]; \
			rAccMetricPrev1 = pOldTrelMetric[prev1] + vecrMetricSet[met1]; \
			\
			/* Take path with smallest metric */ \
			if (rAccMetricPrev0 < rAccMetricPrev1) \
			{ \
				/* Save minimum metric for this state and store decision */ \
				pCurTrelMetric[cur] = rAccMetricPrev0; \
				matbiDecisions[i][cur] = 0; \
			} \
			else \
			{ \
				/* Save minimum metric for this state and store decision */ \
				pCurTrelMetric[cur] = rAccMetricPrev1; \
				matbiDecisions[i][cur] = 1; \
			} \
			\
			/* Second state in this set ----------------------------------- */ \
			/* The only difference is that we swapped the matric sets */ \
			rAccMetricPrev0 = pOldTrelMetric[prev0] + vecrMetricSet[met1]; \
			rAccMetricPrev1 = pOldTrelMetric[prev1] + vecrMetricSet[met0]; \
			\
			/* Take path with smallest metric */ \
			if (rAccMetricPrev0 < rAccMetricPrev1) \
			{ \
				/* Save minimum metric for this state and store decision */ \
				pCurTrelMetric[next] = rAccMetricPrev0; \
				matbiDecisions[i][next] = 0; \
			} \
			else \
			{ \
				/* Save minimum metric for this state and store decision */ \
				pCurTrelMetric[next] = rAccMetricPrev1; \
				matbiDecisions[i][next] = 1; \
			} \
		}
#endif

		/* Unroll butterflys to avoid loop overhead. For c++ version, the
		   actual calculation of the trellis update is done here, for MMX
		   version, only the reordering of the new metrics is done here */
		BUTTERFLY(0, 1, 0, 32, 0, 15)
		BUTTERFLY(2, 3, 1, 33, 6, 9)
		BUTTERFLY(4, 5, 2, 34, 11, 4)
		BUTTERFLY(6, 7, 3, 35, 13, 2)
		BUTTERFLY(8, 9, 4, 36, 11, 4)
		BUTTERFLY(10, 11, 5, 37, 13, 2)
		BUTTERFLY(12, 13, 6, 38, 0, 15)
		BUTTERFLY(14, 15, 7, 39, 6, 9)
		BUTTERFLY(16, 17, 8, 40, 4, 11)
		BUTTERFLY(18, 19, 9, 41, 2, 13)
		BUTTERFLY(20, 21, 10, 42, 15, 0)
		BUTTERFLY(22, 23, 11, 43, 9, 6)
		BUTTERFLY(24, 25, 12, 44, 15, 0)
		BUTTERFLY(26, 27, 13, 45, 9, 6)
		BUTTERFLY(28, 29, 14, 46, 4, 11)
		BUTTERFLY(30, 31, 15, 47, 2, 13)
		BUTTERFLY(32, 33, 16, 48, 9, 6)
		BUTTERFLY(34, 35, 17, 49, 15, 0)
		BUTTERFLY(36, 37, 18, 50, 2, 13)
		BUTTERFLY(38, 39, 19, 51, 4, 11)
		BUTTERFLY(40, 41, 20, 52, 2, 13)
		BUTTERFLY(42, 43, 21, 53, 4, 11)
		BUTTERFLY(44, 45, 22, 54, 9, 6)
		BUTTERFLY(46, 47, 23, 55, 15, 0)
		BUTTERFLY(48, 49, 24, 56, 13, 2)
		BUTTERFLY(50, 51, 25, 57, 11, 4)
		BUTTERFLY(52, 53, 26, 58, 6, 9)
		BUTTERFLY(54, 55, 27, 59, 0, 15)
		BUTTERFLY(56, 57, 28, 60, 6, 9)
		BUTTERFLY(58, 59, 29, 61, 0, 15)
		BUTTERFLY(60, 61, 30, 62, 13, 2)
		BUTTERFLY(62, 63, 31, 63, 11, 4)

#undef BUTTERFLY

#ifdef USE_MMX
		/* Get pointer to current decisions vector */
		unsigned char* pCurDec = &matbiDecisions[i][0];

		/* MMX fixed-point implementation of trellis update */
		__asm
		{
		/*
			This code is based on a Viterbi sample code from
			Phil Karn, KA9Q (Dec 2001)
			simd-viterbi-2.0.3.zip -> viterbi27.c, mmxbfly27.s, ssebfly27.s
			homepage: http://www.ka9q.net

			Some comments to this MMX code:
			- To compare two 8-bit sized unsigned char, we need to apply a
			  special strategy:
			  psubusb mm5, mm1 // mm5 - mm1
			  pcmpeqb mm5, mm3 // mm3 = 0
			  We subtract unsigned with saturation and afterwards compare for
			  equal to zero. If value in mm1 is larger than the value in mm5, we
			  always get 0 as the result
			- Defining __asm Blocks as C Macros: Put the __asm keyword in front
			  of each assembly instruction
			- If we want to use c-pointers to arrays (like "pOldTrelMetric"), we
			  first have to copy it to a register (like edx), otherwise we get
			  errors
		*/
			/* Each invocation of BFLY() will do 8 butterflies in parallel */
#			define BFLY(GROUP) \
			{ \
				/* Compute branch metrics */ \
				__asm mov edx, pOldTrelMetric /* Incoming path metric */ \
				__asm movq mm4, [edx + (8 * GROUP)] /* high bit = 0 */ \
				__asm movq mm5, [edx + ((8 * GROUP) + 32)] /* high bit = 1 */ \
				__asm movq mm0, [chMet1 + (8 * GROUP)] \
				__asm movq mm3, [chMet2 + (8 * GROUP)] \
				\
				__asm movq mm1, mm4 /* first set (mm1, mm2) */ \
				__asm paddusb mm1, mm0 /* first set: decision for bit = 0 (mm1) */ \
				__asm movq mm2, mm5 \
				__asm paddusb mm2, mm3 /* first set: decision for bit = 1 (mm2) */ \
				__asm movq mm6, mm4 /* second set (mm6, mm7) */ \
				__asm paddusb mm6, mm3 /* second set: decision for bit = 0 (mm6) */ \
				__asm movq mm7, mm5 \
				__asm paddusb mm7, mm0 /* second set: decision for bit = 1 (mm7) */ \
				\
				/* live registers 1 2 6 7. Compare mm1 and mm2; mm6 and mm7 */ \
				__asm movq mm5, mm2 \
				__asm movq mm4, mm7 \
				__asm psubusb mm5, mm1 /* mm5 = mm2 - mm1 */ \
				__asm psubusb mm4, mm6 /* mm4 = mm7 - mm6 */ \
				__asm pxor mm3, mm3 /* zero mm3 register, needed for comparison */ \
				__asm pcmpeqb mm5, mm3 /* mm5 = first set of decisions */ \
				__asm pcmpeqb mm4, mm3 /* mm4 = second set of decisions */ \
				\
				/* live registers 1 2 4 5 6 7. Select survivors. Avoid jumps
				   -> mask results with AND and ANDN. then OR */ \
				__asm movq mm3, mm5 \
				__asm movq mm0, mm4 \
				__asm pand mm2, mm5 \
				__asm pand mm7, mm4 \
				__asm pandn mm3, mm1 \
				__asm pandn mm0, mm6 \
				__asm por mm2, mm3 /* mm2: first set survivors (decisions in mm5) */ \
				__asm por mm7, mm0 /* mm7: second set survivors (decisions in mm4) */ \
				\
				/* live registers 2 4 5 7 */ \
				/* interleave & store decisions in mm4, mm5 */ \
				/* interleave & store new branch metrics in mm2, mm7 */ \
				__asm movq mm3, mm5 \
				__asm movq mm0, mm2 \
				__asm punpcklbw mm3, mm4 /* interleave first 8 decisions */ \
				__asm punpckhbw mm5, mm4 /* interleave second 8 decisions */ \
				__asm punpcklbw mm0, mm7 /* interleave first 8 new metrics */ \
				__asm punpckhbw mm2, mm7 /* interleave second 8 new metrics */ \
				__asm mov edx, pCurDec \
				__asm movq [edx + (16 * GROUP)], mm3 \
				__asm movq [edx + (16 * GROUP + 8)], mm5 \
				__asm mov edx, pCurTrelMetric \
				__asm movq [edx + (16 * GROUP)], mm0 /* new metrics */ \
				__asm movq [edx + (16 * GROUP + 8)], mm2 \
			}

			/* Invoke macro 4 times for a total of 32 butterflies */
			BFLY(0)
			BFLY(1)
			BFLY(2)
			BFLY(3)


			/* -----------------------------------------------------------------
			   Normalize by finding smallest metric and subtracting it
			   from all metrics */

#if 1 // TEST if 0, always normalize
			/* See if we have to normalize */
			mov eax, [edx] /* Extract first output metric */
			and eax, 255
			cmp eax, 150 /* Is it greater than 150? */
			mov eax, 0
			jle done /* No, no need to normalize */
#endif

			/* Search for the minimum metric. Result ist stored in mm0 */
#			define PMINUB_MM0_MM1 \
			{ \
				__asm movq mm2, mm0 \
				__asm psubusb mm2, mm1 /* mm2 = mm0 - mm1 */ \
				__asm pxor mm3, mm3 /* zero mm3 register, needed for comparison */ \
				__asm pcmpeqb mm2, mm3 /* decisions */ \
				\
				__asm pand mm0, mm2 \
				__asm pandn mm2, mm1 \
				__asm por mm0, mm2 \
			}

			/* Search for minimum, byte-wise for whole register */
			mov edx, pCurTrelMetric
			movq mm0, [edx]
			movq mm1, [edx + 8]
			PMINUB_MM0_MM1
			movq mm1, [edx + 16]
			PMINUB_MM0_MM1
			movq mm1, [edx + 24]
			PMINUB_MM0_MM1
			movq mm1, [edx + 32]
			PMINUB_MM0_MM1
			movq mm1, [edx + 40]
			PMINUB_MM0_MM1
			movq mm1, [edx + 48]
			PMINUB_MM0_MM1
			movq mm1, [edx + 56]
			PMINUB_MM0_MM1

			/* mm0 contains 8 smallest metrics
			   crunch down to single lowest metric */
			movq mm1, mm0
			psrlq mm0, 32 /* Compare lowest 4 bytes with highest 4 bytes */
			PMINUB_MM0_MM1 /* -> results are in lowest 4 bytes */
			movq mm1, mm0
			psrlq mm0, 16 /* Compare lowest 2 bytes with mext 2 bytes */
			PMINUB_MM0_MM1 /* -> results are in lowest 2 bytes */
			movq mm1, mm0
			psrlq mm0, 8 /* Compare lowest byte with second lowest byte */
			PMINUB_MM0_MM1 /* -> resulting minium metric is in lowest byte */

			/* Expand value in lowest byte to all 8 bytes */
			punpcklbw mm0,mm0 /* First 2 bytes have same value */
			punpcklbw mm0,mm0 /* First 4 bytes have same value */
			punpcklbw mm0,mm0 /* All bytes are the same */


			/* mm0 now contains lowest metric in all 8 bytes
			   subtract it from every output metric. Trashes mm7 */
#			define PSUBUSBM(MEM, REG) \
			{ \
				__asm movq mm7, MEM \
				__asm psubusb mm7, REG \
				__asm movq MEM, mm7 \
			}

			PSUBUSBM([edx], mm0)
			PSUBUSBM([edx + 8], mm0)
			PSUBUSBM([edx + 16], mm0)
			PSUBUSBM([edx + 24], mm0)
			PSUBUSBM([edx + 32], mm0)
			PSUBUSBM([edx + 40], mm0)
			PSUBUSBM([edx + 48], mm0)
			PSUBUSBM([edx + 56], mm0)


		done:
			/* Needed, when we have used mmx registers and want to use floating
			   point operations afterwards */
			emms

#undef BFLY
#undef MINIMUM
#undef PSUBUSBM
		}
#endif

		/* Swap trellis data pointers (old -> new, new -> old) */
		_VITMETRTYPE* pTMPTrelMetric = pCurTrelMetric;
		pCurTrelMetric = pOldTrelMetric;
		pOldTrelMetric = pTMPTrelMetric;
	}


	/* Chainback the decoded bits from trellis ------------------------------ */
	/* The end-state is defined by the DRM standard as all-zeros (shift register
	   in the encoder is padded with zeros at the end */
	iCurDecState = 0;

	for (i = 0; i < iNumOutBits; i++)
	{
		/* Read out decisions "backwards". Mask only first bit, because in MMX
		   implementation, all 8 bits of a "char" are set to the decision */
		_BINARY biCurBit =
			matbiDecisions[iNumOutBitsWithMemory - i - 1][iCurDecState] & 1;

		/* Calculate next state from previous decoded bit -> shift old data
		   and add new bit */
		iCurDecState = (iCurDecState >> 1) | (biCurBit << 5);

		/* Set decisions "backwards" in actual result vector */
		vecbiOutputBits[iNumOutBits - i - 1] = biCurBit;
	}

	/* Return normalized accumulated minimum metric */
	return pOldTrelMetric[0] / iDistCnt;
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

	/* Init vector, storing table for puncturing pattern and generate pattern */
	veciTablePuncPat.Init(iNumOutBitsWithMemory);

	veciTablePuncPat = GenPuncPatTable(eNewCodingScheme, eNewChannelType, iN1,
		iN2, iNewNumOutBitsPartA, iNewNumOutBitsPartB, iPunctPatPartA,
		iPunctPatPartB, iLevel);

	/* Init vector for storing the decided bits */
	matbiDecisions.Init(iNumOutBitsWithMemory, MC_NUM_STATES);
}

CViterbiDecoder::CViterbiDecoder()
{
#if 0
	/* Create trellis *********************************************************/
	/* Activate this code to generate the table needed for the butterfly calls
	   in the processing routine */

	/* We need to analyze 2^(MC_CONSTRAINT_LENGTH - 1) states in the trellis */
	int				i;
	int				iPrev0Index[MC_NUM_STATES];
	int				iPrev1Index[MC_NUM_STATES];
	int				iMetricPrev0[MC_NUM_STATES];
	int				iMetricPrev1[MC_NUM_STATES];

	for (i = 0; i < MC_NUM_STATES; i++)
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
		for (int j = 0; j < MC_NUM_OUTPUT_BITS_PER_STEP; j++)
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
	for (i = 0; i < MC_NUM_STATES; i += 2)
		fprintf(pFile, "BUTTERFLY(%d, %d, %d, %d, %d, %d)\n", i, i + 1,
			iPrev0Index[i], iPrev1Index[i],
			iMetricPrev0[i], iMetricPrev1[i]);
	fflush(pFile);
	exit(1);
#endif
}
