/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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
							  CVector<_BINARY>& vecbiOutputBits,
							  int iNoOutBitsPartA, int iNoOutBitsPartB,
						      int iPunctPatPartA, int iPunctPatPartB, int iLevel)
{
	int				i, j, k;
	int				iMinMetricIndex;
	int				iDistCnt;
	int				iDistCntGlob;
	_REAL			rAccMetricPrev0;
	_REAL			rAccMetricPrev1;
	_REAL			rMinMetric;
	_REAL			rMetricSet[MC_NO_OUTPUT_COMBINATIONS];
	_UINT32BIT		iPuncPatShiftRegShifted;
	_UINT32BIT		iPuncPatShiftReg;
	_UINT32BIT		iPunctCounter;
	int				iTailbitPattern;
	int				iCur;
	int				iOld;
	int				iNoOutBits;

	/* No of bits out is the sum of all protection levels */
	iNoOutBits = iNoOutBitsPartA + iNoOutBitsPartB;

	/* Init indices for "old" and "current" buffers for metric and decoded
	   bits */
	iCur = 0;
	iOld = 1;

	/* Reset all metrics in the trellis. We initialize all states exept of
	   the zero state with a high metric, because we KNOW that the state "0"
	   is the transmitted state */
	TrelState[0].rMetric[iOld] = (_REAL) 0.0;
	for (i = 1; i < MC_NO_STATES; i++)
		TrelState[i].rMetric[iOld] = MC_METRIC_INIT_VALUE;

	/* Reset counter for puncturing and distance (metric) */
	iDistCntGlob = 0;
	iPunctCounter = 0;

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

	for (i = 0; i < iNoOutBits + MC_CONSTRAINT_LENGTH - 1; i++)
	{
		/* ------------------------------------------------------------------ */
		/* Get all possible metrics for one transition in the trellis ------- */
		/* ------------------------------------------------------------------ */
		/* Prepare puncturing pattern --------------------------------------- */
		if (i < iNoOutBitsPartA)
		{
			/* Puncturing patterns part A */
			/* Refill shift register after a wrap */
			if (iPunctCounter == 0)
				iPuncPatShiftReg = iPuncturingPatterns[iPunctPatPartA][2];

			/* Increase puncturing-counter and manage wrap */
			iPunctCounter++;
			if (iPunctCounter == iPuncturingPatterns[iPunctPatPartA][0])
				iPunctCounter = 0;
		}
		else
		{
			/* In case of FAC do not use special tailbit-pattern! */
			if ((i < iNoOutBits) || (eChannelType == CParameter::CT_FAC))
			{
				/* Puncturing patterns part B */
				/* Reset counter when beginning part B */
				if (i == iNoOutBitsPartA)
					iPunctCounter = 0;

				/* Refill shift register after a wrap */
				if (iPunctCounter == 0)
					iPuncPatShiftReg = iPuncturingPatterns[iPunctPatPartB][2];

				/* Increase puncturing-counter and manage wrap */
				iPunctCounter++;
				if (iPunctCounter == iPuncturingPatterns[iPunctPatPartB][0])
					iPunctCounter = 0;
			}
			else
			{
				/* Tailbits */
				/* Set tailbit pattern (no counter needed since there ist
				   only one cycle of this pattern) */
				if (i == iNoOutBits)
					iPuncPatShiftReg = iPunctPatTailbits[iTailbitPattern];
			}
		}

		/* Try all possible output combinations of the encoder */
		for (k = 0; k < MC_NO_OUTPUT_COMBINATIONS; k++)
		{
			/* For each metric we need same dis. count and puncturing pattern */
			iDistCnt = iDistCntGlob;
			iPuncPatShiftRegShifted = iPuncPatShiftReg;

			rMetricSet[k] = (_REAL) 0.0;
			for (j = 0; j < MC_NO_OUTPUT_BITS_PER_STEP; j++)
			{
				/* Calculate distance --------------------------------------- */
				/* Mask first bit (LSB) */
				if (iPuncPatShiftRegShifted & 1)
				{
					/* We define the metrics order: [b_3, b_2, b_1, b_0] */
					if ((k & (1 << j)) > 0)
						/* "1" */
						rMetricSet[k] += vecNewDistance[iDistCnt].rTow1;
					else
						/* "0" */
						rMetricSet[k] += vecNewDistance[iDistCnt].rTow0;

					iDistCnt++;
				}

				/* Shift puncturing mask for next output bit */
				iPuncPatShiftRegShifted >>= 1;
			}
		}

		/* Save shifted register and distance count for next loop */
		iPuncPatShiftReg = iPuncPatShiftRegShifted;
		iDistCntGlob = iDistCnt;


		/* ------------------------------------------------------------------ */
		/* Update trellis --------------------------------------------------- */
		/* ------------------------------------------------------------------ */
		/* We need minimum metric for normalizing the metric after adding to
		   avoid overrun. Next two lines: Initialization */
		iMinMetricIndex = 0;
		rMinMetric =
			TrelState[TrelState[iMinMetricIndex].iPrev0Index].rMetric[iOld] +
			rMetricSet[TrelState[iMinMetricIndex].iMetricPrev0];

		for (j = 0; j < MC_NO_STATES; j++)
		{
			/* Calculate metrics from the two previous states, use the old
			   metric from the previous states plus the "transition-metric" - */
			/* "0" */
			rAccMetricPrev0 =
				TrelState[TrelState[j].iPrev0Index].rMetric[iOld] +
				rMetricSet[TrelState[j].iMetricPrev0];
			/* "1" */
			rAccMetricPrev1 =
				TrelState[TrelState[j].iPrev1Index].rMetric[iOld] +
				rMetricSet[TrelState[j].iMetricPrev1];

			/* Take path with smallest metric ------------------------------- */
			if (rAccMetricPrev0 < rAccMetricPrev1)
			{
				/* Save minimum metric for this state */
				TrelState[j].rMetric[iCur] = rAccMetricPrev0;

				/* Save decoded bits from surviving path */
				TrelState[j].lDecodedBits[iCur] =
					TrelState[TrelState[j].iPrev0Index].lDecodedBits[iOld];

				/* Shift lDecodedBits vector since we want to add a new bit,
				   in this case a "0", but we dont need to add this, it is
				   already there */
				TrelState[j].lDecodedBits[iCur] <<= 1;
			}
			else
			{
				/* Save minimum metric for this state */
				TrelState[j].rMetric[iCur] = rAccMetricPrev1;

				/* Save decoded bits from surviving path */
				TrelState[j].lDecodedBits[iCur] =
					TrelState[TrelState[j].iPrev1Index].lDecodedBits[iOld];

				/* Shift lDecodedBits vector since we want to add a new bit */
				TrelState[j].lDecodedBits[iCur] <<= 1;

				/* Write resulting "1" in lDecodedBits-vector */
				TrelState[j].lDecodedBits[iCur] |= 1;
			}

			/* Get minimum metric and index */
			if (TrelState[j].rMetric[iCur] < rMinMetric)
			{
				rMinMetric = TrelState[j].rMetric[iCur];
				iMinMetricIndex = j;
			}
		}

		/* Save decoded bit, but not before tailbits are used. Mask bit at
		   the defined position in "lDecodedBits" by "MC_DECODING_DEPTH" */
		const int iTotalDecDepth = MC_CONSTRAINT_LENGTH - 1 + MC_DECODING_DEPTH;
		if (i >= iTotalDecDepth)
		{
			/* Mask bit */
			if ((TrelState[iMinMetricIndex].lDecodedBits[iCur] & lOutBitMask)
					> 0)
			{
				vecbiOutputBits[i - iTotalDecDepth] = TRUE;
			}
			else
				vecbiOutputBits[i - iTotalDecDepth] = FALSE;
		}

		/* Exchange indices for old and current buffers for metric and decoded
		   bits */
		iCur = 1 - iCur;
		iOld = 1 - iOld;
	}

	/* Save last "MC_DECODING_DEPTH" bits. We use trellis state "0", because we
	   KNOW that this is our last state (shift registers in the coder are filled
	   with zeros at the end */
	for (i = 0; i < MC_DECODING_DEPTH; i++)
	{
		if ((TrelState[0].lDecodedBits[iOld] &
			(_UINT64BIT(1) << MC_DECODING_DEPTH - i - 1)) /* Mask bit */
			> 0)
		{
			vecbiOutputBits[iNoOutBits - MC_DECODING_DEPTH + i] = TRUE;
		}
		else
			vecbiOutputBits[iNoOutBits - MC_DECODING_DEPTH + i] = FALSE;
	}

	/* Return normalized accumulated minimum metric */
	return rMinMetric / iDistCntGlob;
}

void CViterbiDecoder::Init(CParameter::ECodScheme eNewCodingScheme, int iN1,
						   int iN2, CParameter::EChanType eNewChannelType)
{
	/* Set output mask for bits (constant "1" must be casted to 64 bit interger,
	   otherwise we would get "0") */
	lOutBitMask = _UINT64BIT(1) << MC_DECODING_DEPTH;

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

	eChannelType = eNewChannelType;
}

CViterbiDecoder::CViterbiDecoder()
{
	/* Create trellis *********************************************************/
	for (int i = 0; i < MC_NO_STATES; i++)
	{
		/* Define previous states ------------------------------------------- */
		/* We define in this trellis that we shift the bits from right to
		   the left. We use the transition-bits which "fall" out of the
		   shift-register */
		TrelState[i].iPrev0Index = (i >> 1); /* Old state, Leading "0"
												(automatically inserted by
												shifting */
		TrelState[i].iPrev1Index = (i >> 1) /* Old state */
			| (1 << (MC_CONSTRAINT_LENGTH - 2)); /* New "1", must be on position
													MC_CONSTRAINT_LENGTH - 1 */


		/* Assign metrics to the transitions from both paths ---------------- */
		/* We define with the metrics the order: [b_3, b_2, b_1, b_0] */
		CConvEncoder ConvEncoder;
		TrelState[i].iMetricPrev0 = 0;
		TrelState[i].iMetricPrev1 = 0;
		for (int j = 0; j < MC_NO_OUTPUT_BITS_PER_STEP; j++)
		{
			/* Calculate respective metrics from convolution of state and
			   transition bit */
			/* "0" */
			TrelState[i].iMetricPrev0 |=
				ConvEncoder.Convolution(
				/* Set all states in the shift-register for encoder. Use
				   current state with a leading "0" (which is automatically
				   there */
				i
				/* Use generator-polynomial j */
				, j) 
				/* Shift generated bit to the right position */
				<< j;

			/* "1" */
			TrelState[i].iMetricPrev1 |=
				ConvEncoder.Convolution(
				/* Set all states in the shift-register for encoder. Use
				   current state with a leading "1". The position of this
				   bit is "MC_CONSTRAINT_LENGTH" (shifting from position 1:
				   "MC_CONSTRAINT_LENGTH - 1") */
				i | (1 << (MC_CONSTRAINT_LENGTH - 1))
				/* Use generator-polynomial j */
				, j)
				/* Shift generated bit to the right position */
				<< j;
		}
	}
}
