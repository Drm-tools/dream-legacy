/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Alexander Kurpiers
 *
 * Description:
 *	DRM channel simulation
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

#include "ChannelSimulation.h"


/* Implementation *************************************************************/
void CDRMChannel::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i, j;

	/* Save old values from the end of the vector */
	for (i = 0; i < iMaxDelay; i++)
		veccHistory[i] = veccHistory[i + iInputBlockSize];

	/* Write new symbol in memory */
	for (i = iMaxDelay; i < iLenHist; i++)
		veccHistory[i] = (*pvecInputData)[i - iMaxDelay];

	/* Delay signal using history buffer, add tap gain (fading), multiply with
	   exp-function (optimized implementation, see below) to implement 
	   doppler shift */
	/* Direct path */
	for (i = 0; i < iInputBlockSize; i++)
	{
		veccOutput[i] = tap[0].Update() * 
			veccHistory[i + iMaxDelay /* - 0 */] * cCurExp[0];

		/* Rotate exp-pointer on step further by complex multiplication with 
		   precalculated rotation vector cExpStep. This saves us from
		   calling sin() and cos() functions all the time (iterative 
		   calculation of these functions) */
		cCurExp[0] *= cExpStep[0];
	}

	/* Echos */
	for (j = 1; j < iNoTaps; j++)
	{
		for (i = 0; i < iInputBlockSize; i++)
		{
			veccOutput[i] += tap[j].Update() * 
				veccHistory[i + iMaxDelay - tap[j].GetDelay()] * cCurExp[j];

			/* See above */
			cCurExp[j] *= cExpStep[j];
		}
	}

	/* Get real output vector and correct global gain */
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData)[i] = veccOutput[i].real() * 2 * rGainCorr;

	/* Additional white Gaussian noise (AWGN) */
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData)[i] += randn() * rNoisepwrFactor;


	/* Reference signals for channel estimation evaluation ------------------ */
// TODO
// In case of bit error rate simulation no additional outputs are needed, TODO
// nicer implementation of this!
if (pvecOutputData2 != NULL){

	/* Input reference signal */
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData2)[i] = (*pvecInputData)[i].real() * 2;

	/* Channel reference signal (without additional noise) */
	for (i = 0; i < iInputBlockSize; i++)
		(*pvecOutputData3)[i] = veccOutput[i].real() * 2 * rGainCorr;
}



}

void CDRMChannel::InitInternal(CParameter& ReceiverParam)
{
	int		i;
	_REAL	rSpecOcc;
	_REAL	rBWFactor;

	/* Set channel parameter according to selected channel number (table B.1) */
	switch (ReceiverParam.iDRMChannelNo)
	{
	case 1:
		/* AWGN */
		iNoTaps = 1;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 0.0);
		break;

	case 2:
		/* Rice with delay */
		iNoTaps = 2;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 0.0);

		tap[1].Init(/* Delay: */	(_REAL) 1.0, 
					/* Gain: */		(_REAL) 0.5, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 0.1);
		break;

	case 3:
		/* US Consortium */
		iNoTaps = 4;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.1, 
					/* Fd: */		(_REAL) 0.1);

		tap[1].Init(/* Delay: */	(_REAL) 0.7, 
					/* Gain: */		(_REAL) 0.7, 
					/* Fshift: */	(_REAL) 0.2, 
					/* Fd: */		(_REAL) 0.5);

		tap[2].Init(/* Delay: */	(_REAL) 1.5, 
					/* Gain: */		(_REAL) 0.5, 
					/* Fshift: */	(_REAL) 0.5, 
					/* Fd: */		(_REAL) 1.0);

		tap[3].Init(/* Delay: */	(_REAL) 2.2, 
					/* Gain: */		(_REAL) 0.25, 
					/* Fshift: */	(_REAL) 1.0, 
					/* Fd: */		(_REAL) 2.0);
		break;

	case 4:
		/* CCIR Poor */
		iNoTaps = 2;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 1.0);

		tap[1].Init(/* Delay: */	(_REAL) 2.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 1.0);
		break;
		
	case 5:
		/* Channel no 5 */
		iNoTaps = 2;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 2.0);

		tap[1].Init(/* Delay: */	(_REAL) 4.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 2.0);
		break;
		
	case 6:
		/* Channel no 6 */
		iNoTaps = 4;

		tap[0].Init(/* Delay: */	(_REAL) 0.0, 
					/* Gain: */		(_REAL) 0.5, 
					/* Fshift: */	(_REAL) 0.0, 
					/* Fd: */		(_REAL) 0.1);

		tap[1].Init(/* Delay: */	(_REAL) 2.0, 
					/* Gain: */		(_REAL) 1.0, 
					/* Fshift: */	(_REAL) 1.2, 
					/* Fd: */		(_REAL) 2.4);

		tap[2].Init(/* Delay: */	(_REAL) 4.0, 
					/* Gain: */		(_REAL) 0.25, 
					/* Fshift: */	(_REAL) 2.4, 
					/* Fd: */		(_REAL) 4.8);

		tap[3].Init(/* Delay: */	(_REAL) 6.0, 
					/* Gain: */		(_REAL) 0.0625, 
					/* Fshift: */	(_REAL) 3.6, 
					/* Fd: */		(_REAL) 7.2);
		break;
	}


	/* Init exponent steps (for doppler shift) and gain correction ---------- */
	rGainCorr = (_REAL) 0.0;
	for (i = 0; i < iNoTaps; i++)
	{
		/* Exponent function for shifting (doppler shift) */
		cCurExp[i] = (_REAL) 1.0;

		cExpStep[i] = 
			_COMPLEX(cos(tap[i].GetFShift()), sin(tap[i].GetFShift()));

		/* Gain correction denominator */
		rGainCorr += tap[i].GetGain() * tap[i].GetGain();
	}

	/* Final gain correction value */
	rGainCorr = (_REAL) 1.0 / sqrt(rGainCorr);


	/* Memory allocation ---------------------------------------------------- */
	/* Maximum delay */
	iMaxDelay = tap[iNoTaps - 1].GetDelay();

	/* Allocate memory for history, init vector with zeros. This history is used
	   for generating path delays */
	iLenHist = ReceiverParam.iSymbolBlockSize + iMaxDelay;
	veccHistory.Init(iLenHist, _COMPLEX((_REAL) 0.0, (_REAL) 0.0));

	/* Allocate memory for temporary output vector for complex interim values */
	veccOutput.Init(ReceiverParam.iSymbolBlockSize);


	/* Calculate noise power factors for a given SNR ------------------------ */
#if (USE_SYSTEM_BANDWIDTH)
	/* Spectrum width (N / T_u) */
	rSpecOcc = (_REAL) ReceiverParam.iNoCarrier / 
		ReceiverParam.iFFTSizeN * SOUNDCRD_SAMPLE_RATE;
#else
	switch (ReceiverParam.GetSpectrumOccup())
	{
	case SO_0:
		rSpecOcc = (_REAL) 4900.0; // Hz
		break;

	case SO_1:
		rSpecOcc = (_REAL) 5000.0; // Hz
		break;

	case SO_2:
		rSpecOcc = (_REAL) 9000.0; // Hz
		break;

	case SO_3:
		rSpecOcc = (_REAL) 10000.0; // Hz
		break;

	case SO_4:
		rSpecOcc = (_REAL) 18000.0; // Hz
		break;

	case SO_5:
		rSpecOcc = (_REAL) 20000.0; // Hz
		break;
	}
#endif

	/* Bandwidth correction factor for noise (f_s / (2 * B))*/
	rBWFactor = (_REAL) SOUNDCRD_SAMPLE_RATE / 2 / rSpecOcc;

	/* Calculation of the gain factor for noise generator */
	rNoisepwrFactor = sqrt(exp(-ReceiverParam.rSimSNRdB / 10 * log(10)) * 
		ReceiverParam.rAvPowPerSymbol * 2 * rBWFactor);


	/* Set seed of random noise generator */
	srand((unsigned) time(NULL));

	/* Define block-sizes for input and output */
	iInputBlockSize = ReceiverParam.iSymbolBlockSize;
	iOutputBlockSize = ReceiverParam.iSymbolBlockSize;

	/* For reference signals */
	iOutputBlockSize2 = ReceiverParam.iSymbolBlockSize;
	iOutputBlockSize3 = ReceiverParam.iSymbolBlockSize;
}
	
void CTapgain::Init(_REAL rNewDelay, _REAL rNewGain, _REAL rNewFshift, _REAL rNewFd)
{
	_REAL	s;
	int		k;

	/* Set internal parameters (with conversions) */
	delay = DelMs2Sam(rNewDelay);
	gain = rNewGain;
	fshift = NormShift(rNewFshift);
	fd = rNewFd;

	s = (_REAL) 0.5 * fd / SOUNDCRD_SAMPLE_RATE;

	/* If tap is not fading, return */
	if (s == (_REAL) 0.0)
		return;

	if (s > 0.03)
	{
		interpol = 0;
		polyinterpol = 1;
		phase = -1;
	} 
	else
		if (s > 0.017)
		{
			interpol = 0;
			polyinterpol = 2;
			phase = 0;

		} 
		else
			if (s > 0.0084)
			{
				interpol = 0;
				polyinterpol = 4;
				phase = 0;
			} 
			else
				if (s > 0.0042)
				{
					interpol = 0;
					polyinterpol = 8;
					phase = 0;

				} 
				else
				{
					interpol = (int) (0.0042 / s + 1);
					polyinterpol = 8;
					s = s * interpol;
					phase = 0;
				}

	gausstp(taps, s, polyinterpol);

	/* Initialize FIR buffer */
	for (k = 0; k < FIRLENGTH; k++)
	{
		fir_buff[k][0] = randn();
		fir_buff[k][1] = randn();
	}

	if (interpol)
	{
		/* Compute nextI and nextQ */
		nextI = (_REAL) 0.0;
		nextQ = (_REAL) 0.0;

		/* FIR filter */
		for (k = 0; k < FIRLENGTH; k++)
		{
			nextI += fir_buff[k][0] * taps[FIRLENGTH - k - 1];
			nextQ += fir_buff[k][1] * taps[FIRLENGTH - k - 1];
		}
	}

	over_cnt = 0;
	fir_index = 0;
}

_COMPLEX CTapgain::Update()
{
	int			k;
	_COMPLEX	in, out;

	/* If tap is not fading, just return gain */
	if (fd == (_REAL) 0.0) 
		return gain;

	/* Over_cnt is always zero if no interpolation is used */
	if (!over_cnt)
	{	
		lastI = nextI;
		lastQ = nextQ;

		/* Get new noise sample */
		if ((phase == -1) || (phase == 0)) 
		{
			fir_buff[fir_index][0] = randn();
			fir_buff[fir_index][1] = randn();

			fir_index = (fir_index - 1 + FIRLENGTH) % FIRLENGTH;
		}

		/* Compute new filter output */
		nextI = (_REAL) 0.0; 
		nextQ = (_REAL) 0.0;

		if (phase == -1)
		{
			for (k = 0; k < FIRLENGTH; k++)
			{
				nextI += 
					fir_buff[(k + fir_index) % FIRLENGTH][0] * 
					taps[FIRLENGTH - k - 1];
				nextQ += 
					fir_buff[(k + fir_index) % FIRLENGTH][1] * 
					taps[FIRLENGTH - k - 1];
			}
		}
		else
		{
			/* Polyphase FIR with interpolation by 8 */
			for (k = 0; k < FIRLENGTH; k++)
			{
				nextI +=
					fir_buff[(k + fir_index) % FIRLENGTH][0] *
					taps[polyinterpol * (FIRLENGTH - k) - phase - 1];
				nextQ += 
					fir_buff[(k + fir_index) % FIRLENGTH][1] * 
					taps[polyinterpol * (FIRLENGTH - k) - phase - 1];
			}

			phase = (phase + 1) % polyinterpol;
		}
	}

	if (interpol)
	{
		/* Linear interpolation */
		in  = _COMPLEX((nextI - lastI) * (_REAL) over_cnt / 
			interpol + lastI,
			(nextQ - lastQ) * (_REAL) over_cnt / 
			interpol + lastQ);

		/* Butterworth IIR filter to smooth interpolation */
		out = B1 * in + B2 * in_1 + B3 * in_2 - 
			A2 * out_1 - A3 * out_2;

		out_2 = out_1;
		out_1 = out;

		in_2 = in_1;
		in_1 = in;

		if (++over_cnt == interpol)
			over_cnt = 0;
	}
	else
		out = _COMPLEX(nextI, nextQ);

	/* Weight with gain */
	const _REAL ccGainCorr = sqrt(2);
	return out * gain / ccGainCorr;
}

void CTapgain::gausstp(_REAL taps[], _REAL& s, int& over) const
{
	/* Calculate impulse response of FIR filter to implement
	the Watterson modell (Gaussian PSD) */

	/* "2 * s" is the doppler spread */   
	for (int n = 0; n < (FIRLENGTH * over); n++)
	{
		taps[n] = sqrt(sqrt(crPi * (_REAL) 8.0) * s * over) * 
			exp(-fsqr((_REAL) 2.0 * crPi * s * (n - FIRLENGTH * over / 2)));
	}
}

int CTapgain::DelMs2Sam(const _REAL rDelay) const
{
	/* Delay in samples */
	return (int) (rDelay /* ms */ * SOUNDCRD_SAMPLE_RATE / 1000);	
}

_REAL CTapgain::NormShift(const _REAL rShift) const
{
	/* Normalize doppler shift */
	return (_REAL) 2.0 * crPi / SOUNDCRD_SAMPLE_RATE * rShift;
}

_REAL CChannelSim::randn() const
{
	const int iNoRand = 10;
	const _REAL rFactor = (_REAL) sqrt((_REAL) 12.0 / iNoRand) / RAND_MAX;
	const int iRandMaxHalf = RAND_MAX / 2;

	/* Add some constant distributed random processes to get Gaussian 
	   distribution */
	_REAL rNoise = 0;
	for (int i = 0; i < iNoRand; i++)
		rNoise += rand() - iRandMaxHalf;

	/* Apply amplification factor */
	return rNoise * rFactor;
}
