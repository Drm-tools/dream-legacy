/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2002
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Resample routine for arbitrary sample-rate conversions in a low range (for
 * frequency offset correction).
 * The algorithm is based on a polyphase structure. We upsample the input
 * signal with a factor INTERP_DECIM_I_D and calculate two successive samples
 * whereby we perform a linear interpolation between these two samples to get
 * an arbitraty sample grid. 
 * The polyphase filter is calculated with Matlab(TM), the associated file
 * is ResampleFilter.m.
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

#include "Resample.h"


/* Implementation *************************************************************/
int CResample::Resample(CVector<_REAL>* prInput, CVector<_REAL>* prOutput, 
						_REAL rRation)
{
	int i;
	int ik;
	int ip1, ip2;
	int in1, in2;
	int im;
	_REAL ry1;
	_REAL ry2;
	_REAL rxInt;
	_REAL rBlockDuration;

	/* Move old data from the end to the history part of the buffer */
	for (i = 0; i < iHistorySize; i++)
		vecrIntBuff[i] = vecrIntBuff[i + iInputBlockSize];

	/* Add the new data in internal buffer */
	for (i = 0; i < iInputBlockSize; i++)
		vecrIntBuff[i + iHistorySize] = (*prInput)[i];

	/* Sample-interval of new sample frequency in relation to interpolated 
	   sample-interval */
	rTStep = (_REAL) INTERP_DECIM_I_D / rRation;

	/* Init output counter */
	im = 0;

	/* Calculate block duration */
	rBlockDuration = (iInputBlockSize + iHistorySize - 1) * INTERP_DECIM_I_D;

	/* Main loop */
	do
	{
		/* Quantize output-time to interpolated time-index */
		ik = (int) rtOut;

		/* Calculate convolutions for the two interpolation-taps */
		/* Phase for the linear interpolation-taps */
		ip1 = ik % INTERP_DECIM_I_D;
		ip2 = (ik + 1) % INTERP_DECIM_I_D;

		/* Sample positions in the input vector */
		in1 = (int) (ik / INTERP_DECIM_I_D);
		in2 = (int) ((ik + 1) / INTERP_DECIM_I_D);

		/* Convolution ********************************************************/
		ry1 = (_REAL) 0.0;
		ry2 = (_REAL) 0.0;
		for (i = 0; i < NO_TAPS_PER_PHASE; i++)
		{
			ry1 += fResTaps1To1[ip1][i] * vecrIntBuff[in1 - i];
			ry2 += fResTaps1To1[ip2][i] * vecrIntBuff[in2 - i];
		}

		/* Linear interpolation ***********************************************/
		rxInt = rtOut - (int) rtOut; /* Get numbers after the comma */
		(*prOutput)[im] = (ry2 - ry1) * rxInt + ry1;

		/* Increase output counter */
		im++;
			
		/* Increase output-time and index one step */
		rtOut = rtOut + rTStep;
	} 
	while (rtOut < rBlockDuration);

	/* Set rtOut back */
	rtOut -= iInputBlockSize * INTERP_DECIM_I_D;

	return im;
}

void CResample::Init(int iNewInputBlockSize)
{
	iInputBlockSize = iNewInputBlockSize;

	/* History size must be one sample larger, because we use always TWO 
	   convolutions */
	iHistorySize = NO_TAPS_PER_PHASE + 1;

	/* Allocate memory for internal buffer, clear sample history */
	vecrIntBuff.Init(iInputBlockSize + iHistorySize, (_REAL) 0.0);

	/* Init absolute time for output stream (at the end of the history part */
	rtOut = (_REAL) (iHistorySize - 1) * INTERP_DECIM_I_D;
}
