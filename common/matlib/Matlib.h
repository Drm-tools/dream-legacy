/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathamatic Library (Matlib)
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

#ifndef _MATLIB_H_
#define _MATLIB_H_

#include <math.h>
#include <complex>
using namespace std;
#include "../GlobalDefinitions.h"


/* Definitions ****************************************************************/
/* Two different types: constant and temporary buffer */
enum EVecTy {VTY_CONST, VTY_TEMP};


/* These definitions save a lot of redundant code */
#define _VECOP(TYPE, LENGTH, FCT) CMatlibVector<TYPE> vecRet(LENGTH, VTY_TEMP); for (int i = 0; i < LENGTH; i++) vecRet[i] = FCT; return vecRet

#define _VECOPCL(FCT)		for (int i = 0; i < iVectorLength; i++) operator[](i) FCT; return *this

#ifdef _DEBUG_
#define _TESTRNGR(POS)		if ((POS >= iVectorLength) || (POS < 0)) DebugError("MatLibrRead", "POS", POS, "iVectorLength", iVectorLength)
#define _TESTRNGW(POS)		if ((POS >= iVectorLength) || (POS < 0)) DebugError("MatLibrWrite", "POS", POS, "iVectorLength", iVectorLength)
#define _TESTSIZE(INP)		if (INP != iVectorLength) DebugError("MatLibOperator=()", "INP", INP, "iVectorLength", iVectorLength)
#else
#define _TESTRNGR(POS)
#define _TESTRNGW(POS)
#define _TESTSIZE(INP)
#endif


#define For(a, b, c)		for (a = (b); a <= (c); a++) {
#define End					}
#define If(a)				if (a) {
#define Else				} else {


/* Classes ********************************************************************/
template<class T> class			CMatlibVector; // Prototype for CMatlibVector<> class

typedef _REAL					CReal; // Here we can choose the precision of the Matlib calculations
typedef complex<CReal>			CComplex;
typedef CMatlibVector<CReal>	CRealVector;
typedef CMatlibVector<CComplex>	CComplexVector;

/* Include toolboxes after! the type declarations */
#include "MatlibStdToolbox.h"
#include "MatlibSigProToolbox.h"


/******************************************************************************/
/* CMatlibVector class ********************************************************/
/******************************************************************************/
template<class T>
class CMatlibVector
{
public:
	/* Construction, Destruction -------------------------------------------- */
	CMatlibVector() : iVectorLength(0), pData(NULL), eVType(VTY_CONST) {}
	CMatlibVector(const int iNLen, const EVecTy eNTy = VTY_CONST) : 
		iVectorLength(0), pData(NULL), eVType(eNTy) {Init(iNLen);}
	CMatlibVector(CMatlibVector<T>& vecI); // Copy constructor, VERY IMPORTANT!!!
	virtual ~CMatlibVector() {if (pData != NULL) delete[] pData;}

	CMatlibVector(const CMatlibVector<CReal> fvReal, const CMatlibVector<CReal> fvImag) : 
		iVectorLength(fvReal.GetSize()), pData(NULL), eVType(VTY_CONST/*VTY_TEMP*/)
	{
		/* Allocate data block for vector */
		pData = new CComplex[iVectorLength];

		/* Copy data from real-vectors in complex vector */
		for (int i = 0; i < iVectorLength; i++)
			pData[i] = CComplex(fvReal[i], fvImag[i]);
	}

	/* Operator[] (Regular indices!!!) */
	T operator[](int const iPos) const {_TESTRNGR(iPos); return pData[iPos];}
	T& operator[](int const iPos) {_TESTRNGW(iPos); return pData[iPos];} // For use as l value

	/* Operator() */
	T operator()(int const iPos) const {_TESTRNGR(iPos - 1); return pData[iPos - 1];}
	T& operator()(int const iPos) {_TESTRNGW(iPos - 1); return pData[iPos - 1];} // For use as l value

	CMatlibVector<T> operator()(const int iFrom, const int iTo) const;
	CMatlibVector<T> operator()(const int iFrom, const int iStep, const int iTo) const;

	int GetSize() const {return iVectorLength;}
	CMatlibVector<T>& Init(const int iIniLen);
	CMatlibVector<T>& PutIn(const int iFrom, const int iTo, CMatlibVector<T>& fvA);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, T& tB);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB);
	CMatlibVector<T>& Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB, const CMatlibVector<T>& vecC);


	/* operator= */
	CMatlibVector<T>&	operator=(const CMatlibVector<CReal>& vecI) {_TESTSIZE(vecI.GetSize()); _VECOPCL(= vecI[i]);}
	CMatlibVector<CComplex>& operator=(const CMatlibVector<CComplex>& vecI) {_TESTSIZE(vecI.GetSize()); _VECOPCL(= vecI[i]);}

	/* operator*= */
	CMatlibVector<T>&	operator*=(const CReal& tI) {_VECOPCL(*= tI);}
	CMatlibVector<T>&	operator*=(const CMatlibVector<CReal>& vecI) {_VECOPCL(*= vecI[i]);}
	CMatlibVector<CComplex>& operator*=(const CMatlibVector<CComplex>& vecI) {_VECOPCL(*= vecI[i]);}

	/* operator/= */
	CMatlibVector<T>&	operator/=(const CReal& tI) {_VECOPCL(/= tI);}
	CMatlibVector<T>&	operator/=(const CMatlibVector<CReal>& vecI) {_VECOPCL(/= vecI[i]);}
	CMatlibVector<CComplex>& operator/=(const CMatlibVector<CComplex>& vecI) {_VECOPCL(/= vecI[i]);}

	/* operator+= */
	CMatlibVector<T>&	operator+=(const CReal& tI) {_VECOPCL(+= tI);}
	CMatlibVector<T>&	operator+=(const CMatlibVector<CReal>& vecI) {_VECOPCL(+= vecI[i]);}
	CMatlibVector<CComplex>& operator+=(const CMatlibVector<CComplex>& vecI) {_VECOPCL(+= vecI[i]);}

	/* operator-= */
	CMatlibVector<T>&	operator-=(const CReal& tI) {_VECOPCL(-= tI);}
	CMatlibVector<T>&	operator-=(const CMatlibVector<CReal>& vecI) {_VECOPCL(-= vecI[i]);}
	CMatlibVector<CComplex>& operator-=(const CMatlibVector<CComplex>& vecI) {_VECOPCL(-= vecI[i]);}

protected:
	EVecTy	eVType;
	int		iVectorLength;
	T*		pData;
};


/* Help functions *************************************************************/
template<class T> 
inline int Size(const CMatlibVector<T>& vecI) {return vecI.GetSize();}
template<class T> 
inline int Length(const CMatlibVector<T>& vecI) {return vecI.GetSize();}

/* operator* */
inline CMatlibVector<CComplex> // cv, cv
	operator*(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] * cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator*(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.GetSize(), rvA[i] * rvB[i]);}


inline CMatlibVector<CComplex> // cv, rv
	operator*(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] * rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator*(const CMatlibVector<CReal>& rvB, const CMatlibVector<CComplex>& cvA)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] * rvB[i]);}

template<class T> 
inline CMatlibVector<T> // T, r
	operator*(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.GetSize(), vecA[i] * rB);}
template<class T> 
inline CMatlibVector<T> // r, T
	operator*(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.GetSize(), rA * vecB[i]);}


/* operator/ */
inline CMatlibVector<CComplex> // cv, cv
	operator/(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] / cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator/(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.GetSize(), rvA[i] / rvB[i]);}


inline CMatlibVector<CComplex> // cv, rv
	operator/(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] / rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator/(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, rvA.GetSize(), rvA[i] / cvB[i]);}

template<class T> 
inline CMatlibVector<T> // T, r
	operator/(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.GetSize(), vecA[i] / rB);}
template<class T> 
inline CMatlibVector<T> // r, T
	operator/(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.GetSize(), rA / vecB[i]);}


/* operator+ */
inline CMatlibVector<CComplex> // cv, cv
	operator+(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB) 
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] + cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator+(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB) 
	{_VECOP(CReal, rvA.GetSize(), rvA[i] + rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator+(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB) 
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] + rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator+(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB) 
	{_VECOP(CComplex, rvA.GetSize(), rvA[i] + cvB[i]);}

template<class T> 
inline CMatlibVector<T> // T, r
	operator+(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.GetSize(), vecA[i] + rB);}
template<class T> 
inline CMatlibVector<T> // r, T
	operator+(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.GetSize(), rA + vecB[i]);}


/* operator- */
inline CMatlibVector<CComplex> // cv, cv
	operator-(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] - cvB[i]);}
inline CMatlibVector<CReal> // rv, rv
	operator-(const CMatlibVector<CReal>& rvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CReal, rvA.GetSize(), rvA[i] - rvB[i]);}

inline CMatlibVector<CComplex> // cv, rv
	operator-(const CMatlibVector<CComplex>& cvA, const CMatlibVector<CReal>& rvB)
	{_VECOP(CComplex, cvA.GetSize(), cvA[i] - rvB[i]);}
inline CMatlibVector<CComplex> // rv, cv
	operator-(const CMatlibVector<CReal>& rvA, const CMatlibVector<CComplex>& cvB)
	{_VECOP(CComplex, rvA.GetSize(), rvA[i] - cvB[i]);}

template<class T> 
inline CMatlibVector<T> // T, r
	operator-(const CMatlibVector<T>& vecA, const CReal& rB)
	{_VECOP(T, vecA.GetSize(), vecA[i] - rB);}
template<class T> 
inline CMatlibVector<T> // r, T
	operator-(const CReal& rA, const CMatlibVector<T>& vecB)
	{_VECOP(T, vecB.GetSize(), rA - vecB[i]);}


/* Implementation **************************************************************
   (the implementation of template classes must be in the header file!) */
template<class T>
CMatlibVector<T>::CMatlibVector(CMatlibVector<T>& vecI) : iVectorLength(vecI.GetSize()), 
										pData(NULL), eVType(VTY_CONST/*VTY_TEMP*/)
{
	/* The copy constructor for the constant vector is a real copying
	   task. But in the case of a temporary buffer only the pointer
	   of the temporary buffer is used. The buffer of the temporary
	   vector is then destroyed!!! Therefore the usage of "VTY_TEMP"
	   should be done if the vector IS NOT USED IN A FUNCTION CALL, 
	   otherwise this vector will be destroyed afterwards (if the 
	   function argument is not declared with "&") */
	if (iVectorLength > 0)
	{
		if (vecI.eVType == VTY_CONST)
		{
			// Allocate data block for vector
			pData = new T[iVectorLength];

			// Copy vector
			for (int i = 0; i < iVectorLength; i++)
				pData[i] = vecI[i];
		}
		else
		{
			/* We can define the copy constructor as a destroying operator of
			   the input vector for performance reasons. This
			   saves us from always copy the whole vector */
			// Take data pointer from input vector (steal it)
			pData = vecI.pData;

			// Destroy other vector (temporary vectors only)
			vecI.pData = NULL;
		}
	}
}

template<class T>
CMatlibVector<T>& CMatlibVector<T>::Init(const int iIniLen)
{
	iVectorLength = iIniLen;

	if (pData != NULL)
		delete[] pData;

	/* Allocate data block for vector */
	if (iVectorLength > 0)
	{
		pData = new T[iVectorLength];

		// Init with zeros
		for (int i = 0; i < iVectorLength; i++)
			pData[i] = 0;
	}

	return *this;
}

template<class T>
CMatlibVector<T> CMatlibVector<T>::operator()(const int iFrom, const int iTo) const
{
	CMatlibVector<T> vecRet(iTo - iFrom + 1, VTY_TEMP);

	for (int i = iFrom - 1; i < iTo; i++)
		vecRet[i - iFrom + 1] = operator[](i);

	return vecRet;
}

template<class T>
CMatlibVector<T> CMatlibVector<T>::operator()(const int iFrom, const int iStep, const int iTo) const
{
	int iOutPos = 0;

	CMatlibVector<T> vecRet(abs(iTo - iFrom) / abs(iStep) + 1, VTY_TEMP);

	if (iFrom > iTo)
	{
		for (int i = iFrom - 1; i > iTo - 2; i += iStep)
		{
			vecRet[iOutPos] = operator[](i);
			iOutPos++;
		}
	}
	else
	{
		for (int i = iFrom - 1; i < iTo; i += iStep)
		{
			vecRet[iOutPos] = operator[](i);
			iOutPos++;
		}
	}

	return vecRet;
}

template<class T>
CMatlibVector<T>& CMatlibVector<T>::PutIn(const int iFrom, const int iTo, CMatlibVector<T>& vecI)
{
	for (int i = 0; i < (iTo - iFrom + 1); i++)
		operator[](i + iFrom - 1) = vecI[i];

	return *this;
}

template<class T>
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA, T& tB)
{
	for (int i = 0; i < vecA.GetSize(); i++)
		pData[i] = vecA[i];
	
	operator[](vecA.GetSize()) = tB;

	return *this;
}

template<class T>
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB)
{
	int i;
	int iSizeA = vecA.GetSize();

	/* Put first vector */
	for (i = 0; i < iSizeA; i++)
		operator[](i) = vecA[i];
	
	/* Put second vector behind the first one, both
	   together must have length of *this */
	for (i = 0; i < vecB.GetSize(); i++)
		operator[](i + iSizeA) = vecB[i];

	return *this;
}

template<class T>
CMatlibVector<T>& CMatlibVector<T>::Merge(const CMatlibVector<T>& vecA, const CMatlibVector<T>& vecB, const CMatlibVector<T>& vecC)
{
	int i;
	int iSizeA = vecA.GetSize();
	int iSizeB = vecB.GetSize();

	/* Put first vector */
	for (i = 0; i < iSizeA; i++)
		operator[](i) = vecA[i];
	
	/* Put second vector behind the first one */
	for (i = 0; i < iSizeB; i++)
		operator[](i + iSizeA) = vecB[i];

	/* Put third vector behind previous put vectors */
	for (i = 0; i < vecC.GetSize(); i++)
		operator[](i + iSizeA + iSizeB) = vecC[i];

	return *this;
}


#endif	/* _MATLIB_H_ */
