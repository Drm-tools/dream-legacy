/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Time synchronization tracking using information of scattered pilots
 *
 *	Algorithm proposed by Baoguo Yang in "Timing Recovery for OFDM
 *	Transmission", IEEE November 2000
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

#include "TimeSyncTrack.h"


/* Implementation *************************************************************/
void CTimeSyncTrack::Process(CParameter& Parameter, CComplexVector& veccChanEst,
							 int iNewTiCorr)
{
	int		i;
	int		iTiOffset;
	int		iCurTiCorr;
	int		iIntShiftVal;
	CReal	rPeakBound;
	CReal	rActShiftTiCor;
	CReal	rPropGain;

	/* Correct averaged estimate of impulse response for timing shifts ------ */
	/* Update timing correction history (shift register) */
	vecTiCorrHist.AddEnd(iNewTiCorr);

	/* Adapt vector to timing corrections */
	iCurTiCorr = -vecTiCorrHist[0];

	/* If new correction is out of range, do not use it */
	if ((iCurTiCorr >= iNoIntpFreqPil) || (iCurTiCorr <= -iNoIntpFreqPil))
		iCurTiCorr = 0;

	/* Calculate the actual shift of the timing correction. Since we do the 
	   timing correction at the sound card sample rate (48 kHz) and the 
	   estimated impulse response has a different sample rate (since the 
	   spectrum is only one little part of the sound card frequency range)
	   we have to correct the timing correction by a certain bandwidth factor */
	rActShiftTiCor = rFracPartTiCor + 
		(_REAL) iCurTiCorr * iNoCarrier / Parameter.iFFTSizeN;

	/* Extract the fractional part since we can only correct integer timing
	   shifts */
	rFracPartTiCor = rActShiftTiCor - (int) rActShiftTiCor;

	/* Shift the values in the vector storing the averaged impulse response. We
	   have to consider two cases for shifting (left and right shift) */
	if (rActShiftTiCor < 0)
		iIntShiftVal = (int) rActShiftTiCor + iNoIntpFreqPil;
	else
		iIntShiftVal = (int) rActShiftTiCor;

	vecrAvPoDeSp.Merge(vecrAvPoDeSp(iIntShiftVal + 1, iNoIntpFreqPil), 
		vecrAvPoDeSp(1, iIntShiftVal));


	/* New estimate for impulse response ------------------------------------ */
	/* Apply hamming window, Eq (15) */
	veccPilots = veccChanEst * vecrHammingWindow;

	/* Transform in time-domain to get estimate for delay power profile, 
	   Eq (15) */
	veccPilots = Ifft(veccPilots, FftPlan);


	/* Averaging and detection ---------------------------------------------- */
	/* Do not use averaging before channel estimation was initialized */
	if (iInitCnt > 0)
		iInitCnt--;
	else
	{
		/* Average result, Eq (16) (Should be a moving average function, for 
		   simplicity we have chosen an IIR filter here) */
		const CReal rLambda = 0.9;
		vecrAvPoDeSp = rLambda * vecrAvPoDeSp + 
			(1 - rLambda) * Abs(veccPilots) * Abs(veccPilots);
	}

	/* Lower and higher bound */
	rBoundHigher = Max(vecrAvPoDeSp) * rConst1;
	rBoundLower = Min(vecrAvPoDeSp) * rConst2;

	/* Calculate the peak bound, Eq (19) */
	rPeakBound = Max(rBoundHigher, rBoundLower);

	/* Rotate the averaged result vector to put the earlier peaks
	   (which can also detected in a certain amount) at the beginning of
	   the vector */
	vecrAvPoDeSpRot.Merge(vecrAvPoDeSp(iStPoRot, iNoIntpFreqPil), 
		vecrAvPoDeSp(1, iStPoRot - 1));

	/* Get final estimate, Eq (18) */
	_BOOLEAN bDelayFound = FALSE;
	for (i = 0; i < iNoIntpFreqPil - 1; i++)
	{
		if (bDelayFound == FALSE)
		{
			if ((vecrAvPoDeSpRot[i] > vecrAvPoDeSpRot[i + 1]) &&
				(vecrAvPoDeSpRot[i] > rPeakBound))
			{
				/* The first path delay is found */
				iFirstPathDelay = i;

				/* Set the flag */
				bDelayFound = TRUE;
			}
		}
	}

	/* Only apply timing correction if search was successful and tracking is
	   activated */
	if ((bDelayFound == TRUE) && (bTracking == TRUE))
	{
		/* Final offset is target position in comparision to the estimated first
		   path delay (rotation of vector must be considered here, too */
		iTiOffset = 
			iTargetTimingPos - iFirstPathDelay + iNoIntpFreqPil - iStPoRot;

		/* Adapt the linear control parameter to the region, where the peak was
		   found. The region left of the desired timing position is critical, 
		   because we imideately get ISI if a peak appers here. Therefore we
		   apply fast correction here. At the other positions, we 
		   smooth the controlling to improve the immunity against false peaks */
		if (iTiOffset > 0)
			rPropGain = CONT_PROP_BEFORE_GUARD_INT;
		else
			rPropGain = CONT_PROP_IN_GUARD_INT;


		/* Correct timing offset -------------------------------------------- */
		/* Direct apply of the estimate, no filtering! The correction must be 
		   delayed since we also have a delay in the system from timing
		   correction unit to this unit */
		iControlDelay++;
		if (iControlDelay == iSymDelay)
		{
			/* Reset the counter */
			iControlDelay = 0;

			/* Apply correction. We need to consider the sample grid difference
			   between measurement and correction domain with an additional 
			   factor */
			Parameter.rTimingOffsTrack = -(_REAL) iTiOffset * rPropGain *
				Parameter.iFFTSizeN / iNoCarrier;
		}
	}
}

void CTimeSyncTrack::Init(CParameter& Parameter, int iNewSymbDelay)
{
	bIsInInit = TRUE;

	/* This count prevents from using channel estimation information from time
	   interpolation before the init process hasn't yet finished */
	iInitCnt = INIT_CNT_BEF_CHAN_EST_USED;

	iNoCarrier = Parameter.iNoCarrier;
	iScatPilFreqInt = Parameter.iScatPilFreqInt;
	iNoIntpFreqPil = Parameter.iNoIntpFreqPil;
	iDFTSize = Parameter.iFFTSizeN;

	/* Timing correction history */
	iSymDelay = iNewSymbDelay;
	vecTiCorrHist.Init(iSymDelay, 0);

	/* Init vector for received data at pilot positions */
	veccPilots.Init(iNoIntpFreqPil);

	/* Vector for averaged power delay spread estimation */
	vecrAvPoDeSp.Init(iNoIntpFreqPil);
	vecrAvPoDeSp = Zeros(iNoIntpFreqPil);

	/* Vector for rotated result */
	vecrAvPoDeSpRot.Init(iNoIntpFreqPil);

	/* Length of guard-interval with respect to FFT-size! */
	iGuardSizeFFT = iNoCarrier * 
		Parameter.RatioTgTu.iEnum / Parameter.RatioTgTu.iDenom;

	/* Get the hamming window taps. The window is to reduce the leakage effect
	   of a DFT transformation */
	vecrHammingWindow.Init(iNoIntpFreqPil);
	vecrHammingWindow = Hamming(iNoIntpFreqPil);

	/* Weights for peak bound calculation, in Eq. (19) */
	rConst1 = exp((_REAL) -TETA1_DIST_FROM_MAX_DB / 10 * log(10));
	rConst2 = exp((_REAL) TETA2_DIST_FROM_MIN_DB / 10 * log(10));

	/* Init first path delay estimation */
	iFirstPathDelay = 0;

	/* Define start point for rotation of detection vector for acausal taps.
	   Per definition is this point somewhere in the region after the
	   actual guard-interval window */
	if (iGuardSizeFFT > iNoIntpFreqPil)
		iStPoRot = iNoIntpFreqPil;
	else
		iStPoRot = iGuardSizeFFT + (iNoIntpFreqPil - iGuardSizeFFT) / 2;

	/* Init fractional part of timing correction to zero */
	rFracPartTiCor = (CReal) 0.0;

	/* Init counter for control delay */
	iControlDelay = 0;

	/* Define target position for first path. Should be close to zero but not
	   exactely zero because even small estimation errors would lead to ISI */
	iTargetTimingPos = iGuardSizeFFT / TARGET_TI_POS_FRAC_GUARD_INT;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iNoIntpFreqPil);

	bIsInInit = FALSE;
}

void CTimeSyncTrack::GetAvPoDeSp(CVector<_REAL>& vecrData, 
								 CVector<_REAL>& vecrScale, 
								_REAL& rLowerBound, _REAL& rHigherBound,
								_REAL& rStartGuard, _REAL& rEndGuard)
{
	int		i;
	int		iHalfSpec;
	_REAL	rScaleIncr;
	_REAL	rScaleAbs;

	/* Init output vectors */
	vecrData.Init(iNoIntpFreqPil);
	vecrScale.Init(iNoIntpFreqPil);
	rHigherBound = (_REAL) 0.0;
	rLowerBound = (_REAL) 0.0;
	rStartGuard = 0;
	rEndGuard = 0;

	if (IsInInit() == FALSE)
	{
		/* "- iTargetTimingPos - 1" to get the "0" in the center of the graph */
		iHalfSpec = iNoIntpFreqPil / 2 - iTargetTimingPos - 1;


		/* Init scale (in "ms") */
		rScaleIncr = (_REAL) iDFTSize / 
			(SOUNDCRD_SAMPLE_RATE * iNoIntpFreqPil) * 1000 / 2;

		/* Let the target timing position be the "0" time */
		rScaleAbs = -(iHalfSpec + iTargetTimingPos) * rScaleIncr;

		/* Copy first part of data in output vector */
		for (i = 0; i < iHalfSpec; i++)
		{
			vecrData[i] = (_REAL) 10.0 * 
				log10(vecrAvPoDeSp[iNoIntpFreqPil - iHalfSpec + i]);

			/* Scale */
			vecrScale[i] = rScaleAbs;
			rScaleAbs += rScaleIncr;
		}

		/* Save scale point because this is the start point of guard-interval */
		rStartGuard = rScaleAbs;

		/* Copy second part of data in output vector */
		for (i = iHalfSpec; i < iNoIntpFreqPil; i++)
		{
			vecrData[i] = (_REAL) 10.0 * log10(vecrAvPoDeSp[i - iHalfSpec]);

			/* Scale */
			vecrScale[i] = rScaleAbs;
			rScaleAbs += rScaleIncr;
		}

		/* Return bounds */
		rHigherBound = (_REAL) 10.0 * log10(rBoundHigher);
		rLowerBound = (_REAL) 10.0 * log10(rBoundLower);

		/* End point of guard interval */
		rEndGuard = rScaleIncr * (iGuardSizeFFT - iTargetTimingPos);
	}
}
