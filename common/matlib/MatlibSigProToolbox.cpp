/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathamatic Library (Matlib), signal processing toolbox
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

#include "MatlibSigProToolbox.h"


/* Implementation *************************************************************/
CMatlibVector<CReal> Hann(const int iLen)
{
	int iHalf, i;
	CMatlibVector<CReal> fvRet(iLen, VTY_TEMP);

	if (iLen % 2)
	{
		/* Odd length window */
		iHalf = (iLen + 1) / 2;

		/* Hanning window */
		CMatlibVector<CReal> vecTemp(iHalf);
		CMatlibVector<CReal> w(iHalf);
		for (i = 0; i < iHalf; i++) vecTemp[i] = (CReal) i;
		w = (CReal) 0.5 * (1 - Cos((CReal) 2.0 * crPi * vecTemp / (iLen - 1)));

		/* [w; w(end - 1:-1:1)] */
		return fvRet.Merge(w, w(iHalf - 1, -1, 1));
	}
	else
	{
		/* Even length window */
		iHalf = iLen / 2;

		/* Hanning window */
		CMatlibVector<CReal> vecTemp(iHalf);
		CMatlibVector<CReal> w(iHalf);
		for (i = 0; i < iHalf; i++) vecTemp[i] = (CReal) i;
		w = (CReal) 0.5 * (1 - Cos((CReal) 2.0 * crPi * vecTemp / (iLen - 1)));

		/* [w; w(end:-1:1)] */
		return fvRet.Merge(w, w(iHalf, -1, 1));
	}
}

CMatlibVector<CReal> Hamming(const int iLen)
{
	int iHalf, i;
	CMatlibVector<CReal> fvRet(iLen, VTY_TEMP);

	if (iLen % 2)
	{
		/* Odd length window */
		iHalf = (iLen + 1) / 2;

		/* Hanning window */
		CMatlibVector<CReal> vecTemp(iHalf);
		CMatlibVector<CReal> w(iHalf);
		for (i = 0; i < iHalf; i++) vecTemp[i] = (CReal) i;
		w = (CReal) 0.54 - (CReal) 0.46 * 
			Cos((CReal) 2.0 * crPi * vecTemp / (iLen - 1));

		/* [w; w(end - 1:-1:1)] */
		return fvRet.Merge(w, w(iHalf - 1, -1, 1));
	}
	else
	{
		/* Even length window */
		iHalf = iLen / 2;

		/* Hanning window */
		CMatlibVector<CReal> vecTemp(iHalf);
		CMatlibVector<CReal> w(iHalf);
		for (i = 0; i < iHalf; i++) vecTemp[i] = (CReal) i;
		w = (CReal) 0.54 - (CReal) 0.46 * 
			Cos((CReal) 2.0 * crPi * vecTemp / (iLen - 1));

		/* [w; w(end:-1:1)] */
		return fvRet.Merge(w, w(iHalf, -1, 1));
	}
}

CMatlibVector<CReal> Sinc(const CMatlibVector<CReal>& fvI)
{
	CMatlibVector<CReal> fvRet(fvI.GetSize(), VTY_TEMP);

	for (int i = 0; i < fvI.GetSize(); i++)
	{
		if (fvI[i] == (CReal) 0.0)
			fvRet[i] = (CReal) 1.0;
		else
			fvRet[i] = sin(crPi * fvI[i]) / (crPi * fvI[i]);
	}

	return fvRet;
}

CMatlibVector<CReal> Randn(const int iLength)
{
	/* Add some constant distributed random processes together */
	_VECOP(CReal, iLength, 
		(CReal) ((((CReal) 
		rand() + rand() + rand() + rand() + rand() + rand() + rand()) 
		/ RAND_MAX - 0.5) * /* sqrt(3) * 2 / sqrt(7) */ 1.3093));
}

CMatlibVector<CReal> Filter(const CMatlibVector<CReal>& fvB, 
							const CMatlibVector<CReal>& fvA, 
							const CMatlibVector<CReal>& fvX, 
							CMatlibVector<CReal>& fvZ)
{
	int						m, n, iLenCoeff;
	CMatlibVector<CReal>	fvY(fvX.GetSize(), VTY_TEMP);
	CMatlibVector<CReal>	fvANew, fvBNew;

	/* Length of coefficiants */
	iLenCoeff = Max((CReal) fvB.GetSize(), (CReal) fvA.GetSize());

	/* Make fvB and fvA the same length (zero padding) */
	if (fvB.GetSize() > fvA.GetSize())
	{
		fvBNew.Init(fvB.GetSize());
		fvANew.Init(fvB.GetSize());

		fvBNew = fvB;
		fvANew.Merge(fvA, Zeros(fvB.GetSize() - fvA.GetSize()));
	}
	else
	{
		fvBNew.Init(fvA.GetSize());
		fvANew.Init(fvA.GetSize());

		fvANew = fvA;
		fvBNew.Merge(fvB, Zeros(fvA.GetSize() - fvB.GetSize()));
	}

	/* Filter is implemented as a transposed direct form II structure */
	for (m = 0; m < fvX.GetSize(); m++)
	{
		/* y(m) = (b(1) x(m) + z_1(m - 1)) / a(1) */
		fvY[m] = (fvBNew[0] * fvX[m] + fvZ[0]) / fvANew[0];

		for (n = 1; n < iLenCoeff; n++)
		{
			/* z_{n - 2}(m) = b(n - 1) x(m) + z_{n - 1}(m - 1) -
			   a(n - 1) y(m) */
			fvZ[n - 1] = fvBNew[n] * fvX[m] + fvZ[n] - fvANew[n] * fvY[m];
		}
	}

	return fvY;
}

CMatlibVector<CReal> Levinson(const CMatlibVector<CReal> vecrRx, 
							  const CMatlibVector<CReal> vecrB)
{
/* 
	The levinson recursion [S. Haykin]

	This algorithm solves the following equations:
	Rp ap = ep u1,
	Rp Xp = b, where Rp is a Toepliz-matrix of vector prRx and b = prB 
	is an arbitrary correlation-vector. The Result is ap = prA.

	Parts of the following code are taken from Ptolemy
	(http://ptolemy.eecs.berkeley.edu/)
*/
	const int	iLength = vecrRx.GetSize();
	CRealVector vecrX(iLength, VTY_TEMP);

	CReal		rGamma;
	CReal		rGammaCap;
	CReal		rDelta;
	CReal		rE;
	CReal		rQ;
	int			i, j;
	CRealVector vecraP(iLength);
	CRealVector vecrA(iLength);

	/* Initialize the recursion --------------------------------------------- */
	// (a) First coefficient is always unity
	vecrA[0] = (CReal) 1.0;
	vecraP[0] = (CReal) 1.0;

	// (b) 
	vecrX[0] = vecrB[0] / vecrRx[0];

	// (c) Initial prediction error is simply the zero-lag of
	// of the autocorrelation, or the signal power estimate.
	rE = vecrRx[0];


	/* Main loop ------------------------------------------------------------ */
	// The order recurrence
	for (j = 0; j < iLength - 1; j++)
	{
		// (a) Compute the new gamma
		rGamma = vecrRx[j + 1];
		for (i = 1; i < j + 1; i++) 
			rGamma += vecrA[i] * vecrRx[j - i + 1];

		// (b), (d) Compute and output the reflection coefficient
		// (which is also equal to the last AR parameter)
		vecrA[j + 1] = rGammaCap = - rGamma / rE;

		// (c)
		for (i = 1; i < j + 1; i++) 
			vecraP[i] = vecrA[i] + rGammaCap * vecrA[j - i + 1];

		// Swap a and aP for next order recurrence
		for (i = 1; i < j + 1; i++)
			vecrA[i] = vecraP[i];

		// (e) Update the prediction error power
		rE = rE * ((CReal) 1.0 - rGammaCap * rGammaCap);

		// (f)
		rDelta = (CReal) 0.0;
		for (i = 0; i < j + 1; i++) 
			rDelta += vecrX[i] * vecrRx[j - i + 1];

		// (g), (i) 
		vecrX[j + 1] = rQ = (vecrB[j + 1] - rDelta) / rE;

		// (h)
		for (i = 0; i < j + 1; i++) 
			vecrX[i] = vecrX[i] + rQ * vecrA[j - i + 1];
	}

	return vecrX;
}

CMatlibVector<CComplex> Levinson(const CMatlibVector<CComplex> veccRx, 
								 const CMatlibVector<CComplex> veccB)
{
/* 
	The levinson recursion [S. Haykin]
	COMPLEX version!

	This algorithm solves the following equations:
	Rp ap = ep u1,
	Rp Xp = b, where Rp is a Toepliz-matrix of vector prRx and b = prB 
	is an arbitrary correlation-vector. The Result is ap = prA.

	Parts of the following code are taken from Ptolemy
	(http://ptolemy.eecs.berkeley.edu/)
*/
	const int		iLength = veccRx.GetSize();
	CComplexVector	veccX(iLength, VTY_TEMP);

	CComplex		cGamma;
	CComplex		cGammaCap;
	CComplex		cDelta;
	CReal			rE;
	CComplex		cQ;
	int				i, j;
	CComplexVector	veccaP(iLength);
	CComplexVector	veccA(iLength);

	/* Initialize the recursion --------------------------------------------- */
	// (a) First coefficient is always unity
	veccA[0] = (CReal) 1.0;
	veccaP[0] = (CReal) 1.0;

	// (b) 
	veccX[0] = veccB[0] / veccRx[0];

	// (c) Initial prediction error is simply the zero-lag of
	// of the autocorrelation, or the signal power estimate.
	rE = Real(veccRx[0]);


	/* Main loop ------------------------------------------------------------ */
	// The order recurrence
	for (j = 0; j < iLength - 1; j++)
	{
		// (a) Compute the new gamma
		cGamma = veccRx[j + 1];
		for (i = 1; i < j + 1; i++) 
			cGamma += veccA[i] * veccRx[j - i + 1];

		// (b), (d) Compute and output the reflection coefficient
		// (which is also equal to the last AR parameter)
		veccA[j + 1] = cGammaCap = - cGamma / rE;

		// (c)
		for (i = 1; i < j + 1; i++) 
			veccaP[i] = veccA[i] + cGammaCap * Conj(veccA[j - i + 1]);

		// Swap a and aP for next order recurrence
		for (i = 1; i < j + 1; i++)
			veccA[i] = veccaP[i];

		// (e) Update the prediction error power
		rE = rE * ((CReal) 1.0 - Abs(cGammaCap) * Abs(cGammaCap));

		// (f)
		cDelta = (CReal) 0.0;
		for (i = 0; i < j + 1; i++) 
			cDelta += veccX[i] * veccRx[j - i + 1];

		// (g), (i) 
		veccX[j + 1] = cQ = (veccB[j + 1] - cDelta) / rE;

		// (h)
		for (i = 0; i < j + 1; i++) 
			veccX[i] = veccX[i] + cQ * Conj(veccA[j - i + 1]);
	}

	return veccX;
}

void GetIIRTaps(CMatlibVector<CReal>& vecrB, CMatlibVector<CReal>& vecrA, 
				const CReal rFreqOff)
{
/* 
	Use a prototype IIR filter (calculated with Matlab(TM)) and shift
	the spectrum
*/
	CReal rWu, rWl, rAlpha, rK, rA1, rA2;

	/* Coefficiants of prototype filter */
	/* [b, a] = butter(2, 0.08333); */
	const CReal rProtB[3] = {(CReal) 0.01440038190639, 
		(CReal) 0.02880076381278, (CReal) 0.01440038190639};
	const CReal rProtA[3] = 
		{(CReal) 1.0, (CReal) -1.63300761704957, (CReal) 0.69060914467514};


	/* 4.5 kHz 3-dB-bandwidth at a sample rate of 48 kHz */
	const CReal rWc = crPi * 0.09375;

	/* New lower and upper frequency bound */
	rWu = (CReal) crPi * (1.0 - rFreqOff);
	rWl = rWu - 2 * rWc;

	/* Alpha and K as defined in Proakis, Digital Signal Processing */
	rAlpha = cos((rWu + rWl) / 2) / cos((rWu - rWl) / 2);
	rK = tan(rWc / 2) / tan((rWu - rWl) / 2);

	/* A1, A2 */
	rA1 = -2 * rAlpha * rK / (rK + 1);
	rA2 = (rK - 1) / (rK + 1);

	/* Init input vectors */
	vecrB.Init(5);
	vecrA.Init(5);

	/* Calculate new coefficiants */
	vecrB[0] = rProtB[0] - rA2 * rProtB[1] + rA2 * rA2 * rProtB[2];
	vecrB[1] = (CReal) 0.0;
	vecrB[2] = 2 * rA2 * rProtB[0] + rA1 * rA1 * rProtB[0] - 
		rA2 * rA2 * rProtB[1] - rA1 * rA1 * rProtB[1] - rProtB[1] + 
		2 * rA2 * rProtB[2] + rA1 * rA1 * rProtB[2];
	vecrB[3] = (CReal) 0.0;
	vecrB[4] = vecrB[0];

	vecrA[0] = rProtA[0] - rA2 * rProtA[1] + rA2 * rA2 * rProtA[2];
	vecrA[1] = -2 * rA1 * rProtA[0] + rA1 * rA2 * rProtA[1] + rA1 * rProtA[1] -
		2 * rA1 * rA2 * rProtA[2];
	vecrA[2] = 2 * rA2 * rProtA[0] + rA1 * rA1 * rProtA[0] - 
		rA2 * rA2 * rProtA[1] - rA1 * rA1 * rProtA[1] - rProtA[1] + 
		2 * rA2 * rProtA[2] + rA1 * rA1 * rProtA[2];
	vecrA[3] = -2 * rA1 * rA2 * rProtA[0] + rA1 * rA2 * rProtA[1] + 
		rA1 * rProtA[1] - 2 * rA1 * rProtA[2];
	vecrA[4] = rA2 * rA2 * rProtA[0] - rA2 * rProtA[1] + rProtA[2];
}
