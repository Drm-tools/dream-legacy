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
_REAL CTimeSyncTrack::Process(CParameter& Parameter,
							  CComplexVector& veccChanEst, int iNewTiCorr)
{
	int			i;
	_REAL		rTiOffset;
	int			iIntShiftVal;
	int			iFirstPathDelay;
	CReal		rPeakBound;
	CReal		rActShiftTiCor;
	CReal		rPropGain;
	_BOOLEAN	bDelayFound;
	CReal		rTotalEnergy;
	CReal		rCurEnergy;
	_BOOLEAN	bDelSprLenFound;

	/* Rotate the averaged PDP to follow the time shifts -------------------- */
	/* Update timing correction history (shift register) */
	vecTiCorrHist.AddEnd(iNewTiCorr);

	/* Calculate the actual shift of the timing correction. Since we do the 
	   timing correction at the sound card sample rate (48 kHz) and the 
	   estimated impulse response has a different sample rate (since the 
	   spectrum is only one little part of the sound card frequency range)
	   we have to correct the timing correction by a certain bandwidth factor */
	rActShiftTiCor = rFracPartTiCor - 
		(_REAL) vecTiCorrHist[0] * iNoCarrier / iDFTSize;

	/* Extract the fractional part since we can only correct integer timing
	   shifts */
	rFracPartTiCor = rActShiftTiCor - (int) rActShiftTiCor;

	/* Shift the values in the vector storing the averaged impulse response. We
	   have to consider two cases for shifting (left and right shift) */
	if (rActShiftTiCor < 0)
		iIntShiftVal = (int) rActShiftTiCor + iNoIntpFreqPil;
	else
		iIntShiftVal = (int) rActShiftTiCor;

	/* If new correction is out of range, do not use it */
	if ((iIntShiftVal >= iNoIntpFreqPil) || (iIntShiftVal < 0))
		iIntShiftVal = 0;

	/* Actual rotation of vector */
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
		IIR1(vecrAvPoDeSp, SqMag(veccPilots), (CReal) 0.9);
	}

	/* Rotate the averaged result vector to put the earlier peaks
	   (which can also detected in a certain amount) at the beginning of
	   the vector */
	vecrAvPoDeSpRot.Merge(vecrAvPoDeSp(iStPoRot, iNoIntpFreqPil), 
		vecrAvPoDeSp(1, iStPoRot - 1));


	/* Lower and higher bound */
	rBoundHigher = Max(vecrAvPoDeSpRot) * rConst1;
	rBoundLower = Min(vecrAvPoDeSpRot) * rConst2;

	/* Calculate the peak bound, Eq (19) */
	rPeakBound = Max(rBoundHigher, rBoundLower);

	/* Get final estimate, Eq (18) */
	bDelayFound = FALSE; /* Init flag */
	for (i = 0; i < iNoIntpFreqPil - 1; i++)
	{
		/* We are only interested in the first peak */
		if (bDelayFound == FALSE)
		{
			if ((vecrAvPoDeSpRot[i] > vecrAvPoDeSpRot[i + 1]) &&
				(vecrAvPoDeSpRot[i] > rPeakBound))
			{
				/* The first path delay is found. Consider the rotation
				   introduced for earlier peaks */
				iFirstPathDelay = i + iStPoRot;

				/* Set the flag */
				bDelayFound = TRUE;
			}
		}
	}


	/* Only apply timing correction if search was successful and tracking is
	   activated */
	if ((bDelayFound == TRUE) && (bTracking == TRUE))
	{
		/* Correct timing offset -------------------------------------------- */
		/* Final offset is target position in comparision to the estimated first
		   path delay. Since we have a delay from the channel estimation, the
		   previous correction is subtracted "- vecrNewMeasHist[0]". If the
		   "real" correction arrives after the delay, this correction is 
		   compensated. The delay of the history buffer (vecrNewMeasHist) must
		   be equal to the delay of the channel estimation */
		rTiOffset = (_REAL) iTargetTimingPos - iFirstPathDelay + iNoIntpFreqPil 
			- vecrNewMeasHist[0];

		/* Adapt the linear control parameter to the region, where the peak was
		   found. The region left of the desired timing position is critical, 
		   because we imideately get ISI if a peak appers here. Therefore we
		   apply fast correction here. At the other positions, we 
		   smooth the controlling to improve the immunity against false peaks */
		if (rTiOffset > 0)
			rPropGain = CONT_PROP_BEFORE_GUARD_INT;
		else
			rPropGain = CONT_PROP_IN_GUARD_INT;

		/* Apply proportional control */
		rTiOffset *= rPropGain;

		/* Manage correction history. The corrections stored in the history must
		   be quantized to the upsampled output sample rate and is back 
		   transformed to the low sample rate afterwards */
		vecrNewMeasHist.AddEnd(0);
		for (i = 0; i < iSymDelay - 1; i++)
			vecrNewMeasHist[i] += (int) (rTiOffset * iDFTSize / iNoCarrier) * 
				(_REAL) iNoCarrier / iDFTSize;

		/* Apply correction. We need to consider the sample grid difference
		   between measurement and correction domain with an additional 
		   factor */
		Parameter.rTimingOffsTrack = -rTiOffset * iDFTSize / iNoCarrier;
	}


	/* Delay spread length estimation --------------------------------------- */
	/* Total energy of estimated impulse response */
	rTotalEnergy = Sum(vecrAvPoDeSp * vecrAvPoDeSp);

	/* Calculate the point where "rEnergBound" part of the energy is included */
	const CReal rEnergBound = (CReal) 0.9999999;
	rCurEnergy = (CReal) 0.0;
	bDelSprLenFound = FALSE;
	rEstDelay = rGuardSizeFFT;
	
	for (i = 0; i < iNoIntpFreqPil; i++)
	{
		if (bDelSprLenFound == FALSE)
		{
			rCurEnergy += vecrAvPoDeSp[i] * vecrAvPoDeSp[i];

			if (rCurEnergy > rEnergBound * rTotalEnergy)
			{
				/* Delay index */
				rEstDelay = (_REAL) i;

				bDelSprLenFound = TRUE;
			}
		}
	}

	/* Bound the delay to the guard-interval length */
	if (rEstDelay > rGuardSizeFFT)
		return rGuardSizeFFT;
	else
		return rEstDelay;
}

void CTimeSyncTrack::Init(CParameter& Parameter, int iNewSymbDelay)
{
	bIsInInit = TRUE;

	/* This count prevents from using channel estimation information from time
	   interpolation before the init process hasn't yet finished. We use 2 * 
	   the symbol delay because we assume a symmetric filter for channel
	   estimation in time direction */
	iInitCnt = iNewSymbDelay * 2;

	iNoCarrier = Parameter.iNoCarrier;
	iScatPilFreqInt = Parameter.iScatPilFreqInt;
	iNoIntpFreqPil = Parameter.iNoIntpFreqPil;
	iDFTSize = Parameter.iFFTSizeN;

	/* Timing correction history */
	iSymDelay = iNewSymbDelay;
	vecTiCorrHist.Init(iSymDelay, 0);

	/* History for new measurements (corrections) */
	vecrNewMeasHist.Init(iSymDelay - 1, (_REAL) 0.0);

	/* Init vector for received data at pilot positions */
	veccPilots.Init(iNoIntpFreqPil);

	/* Vector for averaged power delay spread estimation */
	vecrAvPoDeSp.Init(iNoIntpFreqPil, (CReal) 0.0);

	/* Vector for rotated result */
	vecrAvPoDeSpRot.Init(iNoIntpFreqPil);

	/* Length of guard-interval with respect to FFT-size! */
	rGuardSizeFFT = (_REAL) iNoCarrier * 
		Parameter.RatioTgTu.iEnum / Parameter.RatioTgTu.iDenom;

	/* Get the hamming window taps. The window is to reduce the leakage effect
	   of a DFT transformation */
	vecrHammingWindow.Init(iNoIntpFreqPil);
	vecrHammingWindow = Hamming(iNoIntpFreqPil);

	/* Weights for peak bound calculation, in Eq. (19) */
	rConst1 = pow(10, (_REAL) -TETA1_DIST_FROM_MAX_DB / 10);
	rConst2 = pow(10, (_REAL) TETA2_DIST_FROM_MIN_DB / 10);

	/* Define start point for rotation of detection vector for acausal taps.
	   Per definition is this point somewhere in the region after the
	   actual guard-interval window */
	if ((int) rGuardSizeFFT > iNoIntpFreqPil)
		iStPoRot = iNoIntpFreqPil;
	else
		iStPoRot = (int) (rGuardSizeFFT + (iNoIntpFreqPil - rGuardSizeFFT) / 2);

	/* Init fractional part of timing correction to zero */
	rFracPartTiCor = (CReal) 0.0;

	/* Define target position for first path. Should be close to zero but not
	   exactely zero because even small estimation errors would lead to ISI */
	iTargetTimingPos = (int) (rGuardSizeFFT / TARGET_TI_POS_FRAC_GUARD_INT);

	/* Init estimation of length of impulse response with length of guard-
	   interval */
	rEstDelay = rGuardSizeFFT;

	/* Init plans for FFT (faster processing of Fft and Ifft commands) */
	FftPlan.Init(iNoIntpFreqPil);

	bIsInInit = FALSE;
}

void CTimeSyncTrack::GetAvPoDeSp(CVector<_REAL>& vecrData, 
								 CVector<_REAL>& vecrScale, 
								_REAL& rLowerBound, _REAL& rHigherBound,
								_REAL& rStartGuard, _REAL& rEndGuard,
								_REAL& rLenIR)
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
			if (vecrAvPoDeSp[iNoIntpFreqPil - iHalfSpec + i] > 0)
				vecrData[i] = (_REAL) 10.0 * 
					log10(vecrAvPoDeSp[iNoIntpFreqPil - iHalfSpec + i]);
			else
				vecrData[i] = RET_VAL_LOG_0;

			/* Scale */
			vecrScale[i] = rScaleAbs;
			rScaleAbs += rScaleIncr;
		}

		/* Save scale point because this is the start point of guard-interval */
		rStartGuard = rScaleAbs;

		/* Copy second part of data in output vector */
		for (i = iHalfSpec; i < iNoIntpFreqPil; i++)
		{
			if (vecrAvPoDeSp[i - iHalfSpec] > 0)
				vecrData[i] = (_REAL) 10.0 * log10(vecrAvPoDeSp[i - iHalfSpec]);
			else
				vecrData[i] = RET_VAL_LOG_0;

			/* Scale */
			vecrScale[i] = rScaleAbs;
			rScaleAbs += rScaleIncr;
		}

		/* Return bounds */
		if (rBoundHigher > 0)
			rHigherBound = (_REAL) 10.0 * log10(rBoundHigher);
		else
			rHigherBound = RET_VAL_LOG_0;

		if (rBoundLower > 0)
			rLowerBound = (_REAL) 10.0 * log10(rBoundLower);
		else
			rLowerBound = RET_VAL_LOG_0;

		/* End point of guard interval */
		rEndGuard = rScaleIncr * (rGuardSizeFFT - iTargetTimingPos);

		/* Estmiated impulse response length */
		rLenIR = rScaleIncr * (rEstDelay - iTargetTimingPos);
	}
}
