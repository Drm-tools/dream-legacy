/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathamatic Library (Matlib), standard toolbox
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

#ifndef _MATLIB_STD_TOOLBOX_H_
#define _MATLIB_STD_TOOLBOX_H_

#include "Matlib.h"

/* fftw (Homepage: http://www.fftw.org) */
#ifdef HAVE_DFFTW_H
# include <dfftw.h>
#else
# include <fftw.h>
#endif

#ifdef HAVE_DRFFTW_H
# include <drfftw.h>
#else
# include <rfftw.h>
#endif


/* Classes ********************************************************************/
class CFftPlans
{
public:
	CFftPlans() : RFFTPlForw(NULL), RFFTPlBackw(NULL), bInitialized(false) {}
	CFftPlans(const int iFftSize) {Init(iFftSize);}
	virtual ~CFftPlans() {if (bInitialized) {
		rfftw_destroy_plan(RFFTPlForw); rfftw_destroy_plan(RFFTPlBackw);
		fftw_destroy_plan(FFTPlForw); fftw_destroy_plan(FFTPlBackw);}}

	void Init(const int iFSi)
	{
		// Delete old plans
		if (bInitialized) {
			rfftw_destroy_plan(RFFTPlForw); rfftw_destroy_plan(RFFTPlBackw);
			fftw_destroy_plan(FFTPlForw); fftw_destroy_plan(FFTPlBackw);}

		RFFTPlForw = rfftw_create_plan(iFSi, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);
		RFFTPlBackw = rfftw_create_plan(iFSi, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE);
		FFTPlForw = fftw_create_plan(iFSi, FFTW_FORWARD, FFTW_ESTIMATE);
		FFTPlBackw = fftw_create_plan(iFSi, FFTW_BACKWARD, FFTW_ESTIMATE);

		bInitialized = true;
	}

	rfftw_plan	RFFTPlForw;
	rfftw_plan	RFFTPlBackw;
	fftw_plan	FFTPlForw;
	fftw_plan	FFTPlBackw;
	bool		bInitialized;
};


/* Helpfunctions **************************************************************/
CMatlibVector<CReal>		Min(const CMatlibVector<CReal>& fvA, const CMatlibVector<CReal>& fvB);
CReal						Min(const CReal& rA, const CReal& rB);
template<class T> T			Min(const CMatlibVector<T>& vecI);
CMatlibVector<CReal>		Max(const CMatlibVector<CReal>& fvA, const CMatlibVector<CReal>& fvB);
CReal						Max(const CReal& rA, const CReal& rB);
template<class T> T			Max(const CMatlibVector<T>& vecI);

CMatlibVector<CReal>		Sort(const CMatlibVector<CReal>& rvI);

template<class T> T			Sum(const CMatlibVector<T>& vecI);

CMatlibVector<CReal>		Ones(const int iLen);
CMatlibVector<CReal>		Zeros(const int iLen);

CMatlibVector<CReal>		Real(const CMatlibVector<CComplex>& cvI);
CReal						Real(const CComplex& cI);
CMatlibVector<CReal>		Imag(const CMatlibVector<CComplex>& cvI);
CReal						Imag(const CComplex& cI);
CMatlibVector<CComplex>		Conj(const CMatlibVector<CComplex>& cvI);
CComplex					Conj(const CComplex& cI);

CMatlibVector<CReal>		Abs(const CMatlibVector<CReal>& fvI);
CMatlibVector<CReal>		Abs(const CMatlibVector<CComplex>& cvI);
CReal						Abs(const CComplex& cI);
CReal						Abs(const CReal& rI);

CMatlibVector<CReal>		Angle(const CMatlibVector<CComplex>& cvI);
CReal						Angle(const CComplex& cI);

/* Trigonometric functions */
template<class T> 
CMatlibVector<T>			Sin(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), sin(vecI[i]));}
CReal						Sin(const CReal& fI);
template<class T> 
CMatlibVector<T>			Cos(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), cos(vecI[i]));}
CReal						Cos(const CReal& fI);
template<class T> 
CMatlibVector<T>			Tan(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), tan(vecI[i]));}
CReal						Tan(const CReal& fI);

/* Square root */
template<class T> 
CMatlibVector<T>			Sqrt(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), sqrt(vecI[i]));}
CReal						Sqrt(const CReal& fI);

/* Exponential function */
template<class T> 
CMatlibVector<T>			Exp(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), exp(vecI[i]));}
CReal						Exp(const CReal& fI);

/* Logarithm */
template<class T> 
CMatlibVector<T>			Log(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), log(vecI[i]));}
CReal						Log(const CReal& fI);
template<class T> 
CMatlibVector<T>			Log10(const CMatlibVector<T>& vecI) {_VECOP(T, vecI.GetSize(), log10(vecI[i]));}
CReal						Log10(const CReal& fI);

/* Mean, variance and standard deviation */
template<class T> T			Mean(const CMatlibVector<T>& vecI);
template<class T> T			Var(const CMatlibVector<T>& vecI);
template<class T> T			Std(CMatlibVector<T>& vecI) {return Sqrt(Var(vecI));}


/* Rounding functions */
CMatlibVector<CReal>		Fix(const CMatlibVector<CReal>& fvI);
CReal						Fix(const CReal& fI);
CMatlibVector<CReal>		Floor(const CMatlibVector<CReal>& fvI);
CReal						Floor(const CReal& fI);
CMatlibVector<CReal>		Ceil(const CMatlibVector<CReal>& fvI);
CReal						Ceil(const CReal& fI);


/* Fourier transformations (also included: real FFT) */
CMatlibVector<CComplex>		Fft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CComplex>		Ifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CComplex>		rfft(CMatlibVector<CReal>& fvI, const CFftPlans& FftPlans = CFftPlans());
CMatlibVector<CReal>		rifft(CMatlibVector<CComplex>& cvI, const CFftPlans& FftPlans = CFftPlans());


/* Implementation **************************************************************
   (the implementation of template classes must be in the header file!) */
template<class T>
T Min(const CMatlibVector<T>& vecI)
{
	T fMinRet = vecI[0];
	for (int i = 0; i < vecI.GetSize(); i++)
		if (vecI[i] < fMinRet)
			fMinRet = vecI[i];

	return fMinRet;
}

template<class T>
T Max(const CMatlibVector<T>& vecI)
{
	T fMaxRet = vecI[0];
	for (int i = 0; i < vecI.GetSize(); i++)
		if (vecI[i] > fMaxRet)
			fMaxRet = vecI[i];

	return fMaxRet;
}

template<class T>
T Sum(const CMatlibVector<T>& vecI)
{
	T SumRet = 0;
	for (int i = 0; i < vecI.GetSize(); i++)
		SumRet += vecI[i];

	return SumRet;
}

template<class T> 
T Mean(const CMatlibVector<T>& vecI)
{
	T tRet = 0;
	for (int i = 0; i < vecI.GetSize(); i++)
		tRet += vecI[i];

	return tRet / vecI.GetSize(); // Normalizing
}

template<class T> 
T Var(const CMatlibVector<T>& vecI)
{
	int i;

	/* First calculate mean */
	T tMean = 0;
	for (i = 0; i < vecI.GetSize(); i++)
		tMean += vecI[i];
	tMean /= vecI.GetSize(); // Normalizing

	/* Now variance (sum formula) */
	T tRet = 0;
	for (i = 0; i < vecI.GetSize(); i++)
		tRet += (vecI[i] - tMean) * (vecI[i] - tMean);

	return tRet / (vecI.GetSize() - 1); // Normalizing
}


#endif	/* _MATLIB_STD_TOOLBOX_H_ */
