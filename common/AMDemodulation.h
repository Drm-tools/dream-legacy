/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See AMDemodulation.cpp
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

#if !defined(AMDEMOD_H__3B0BEVJBN8LKH2934BGF4344_BB27912__INCLUDED_)
#define AMDEMOD_H__3B0BEVJBN8LKH2934BGF4344_BB27912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"
#include "Vector.h"
#include "matlib/Matlib.h"
#include "AMDemodulationFilter.h"
#include "resample/Resample.h"

#ifdef HAVE_DRFFTW_H
# include <drfftw.h>
#else
# include <rfftw.h>
#endif


/* Definitions ****************************************************************/
/* Set the number of blocks used for carrier frequency acquisition */
#define NUM_BLOCKS_CARR_ACQUISITION			10

/* Percentage of aquisition search window half size relative to the useful
   spectrum bandwidth */
#define PERC_SEARCH_WIN_HALF_SIZE			((CReal) 5.0 /* % */ / 100)

/* Set value for desired amplitude for AM signal, controlled by the AGC. Maximum
   value is the range for short variables (16 bit) -> 32768 */
#define DES_AV_AMPL_AM_SIGNAL				((CReal) 8000.0)

/* Lower bound for estimated average amplitude. That is needed, since we 
   devide by this estimate so it must not be zero */
#define LOWER_BOUND_AMP_LEVEL				((CReal) 10.0)

/* Amplitude correction factor for demodulation */
#define AM_AMPL_CORR_FACTOR					((CReal) 5.0)


/* Classes ********************************************************************/
class CAMDemodulation : public CReceiverModul<_REAL, _SAMPLE>
{
public:
	CAMDemodulation() : bAcquisition(TRUE), bSearWinWasSet(FALSE),
		bNewDemodType(FALSE), eDemodType(DT_AM), iFilterBW(10000),
		rFiltCentOffsNorm((CReal) 0.0), rBandWidthNorm((CReal) 0.0),
		eAGCType(AT_MEDIUM) {}
	virtual ~CAMDemodulation() {}

	enum EDemodType {DT_AM, DT_LSB, DT_USB, DT_FM};
	enum EAGCType {AT_NO_AGC, AT_SLOW, AT_MEDIUM, AT_FAST};

	void SetAcqFreq(const CReal rNewNormCenter);

	EDemodType GetDemodType() {return eDemodType;}
	void SetDemodType(const EDemodType eNewType)
		{eDemodType = eNewType; bNewDemodType = TRUE;}

	int GetFilterBW() {return iFilterBW;}
	void SetFilterBW(const int iNewBW)
		{iFilterBW = iNewBW; bNewDemodType = TRUE;}

	EAGCType GetAGCType() {return eAGCType;}
	void SetAGCType(const EAGCType eNewType)
		{eAGCType = eNewType;}

	void GetBWParameters(CReal& rCenterFreq, CReal& rBW)
		{rCenterFreq = rFiltCentOffsNorm; rBW = rBandWidthNorm;}

protected:
	void SetCarrierFrequency(const CReal rNormCurFreqOffset);
	void GetBWFilter(const int iFiltBW, CReal& rFreq,
		CVector<CReal>& vecrFilter);
	CVector<CReal> ResampleFilterCoeff(const float* pfFilt, CReal rRatio);

	CRealVector					rvecA;
	CRealVector					rvecBReal;
	CRealVector					rvecBImag;
	CRealVector					rvecZReal;
	CRealVector					rvecZImag;
	CRealVector					rvecInpTmp;
	CComplexVector				cvecHilbert;

	CRealVector					rvecZAM;
	CRealVector					rvecADC;
	CRealVector					rvecBDC;
	CRealVector					rvecZFM;
	CRealVector					rvecAFM;
	CRealVector					rvecBFM;

	CComplex					cCurExp;
	CComplex					cExpStep;

	int							iSymbolBlockSize;

	_BOOLEAN					bAcquisition;
	CShiftRegister<fftw_real>	vecrFFTHistory;

	CFftPlans					FftPlan;
	CRealVector					vecrFFTInput;
	CComplexVector				veccFFTOutput;

	int							iTotalBufferSize;
	int							iAquisitionCounter;
	CRealVector					vecrPSD;
	int							iHalfBuffer;

	int							iSearchWinStart;
	int							iSearchWinEnd;
	_BOOLEAN					bSearWinWasSet;
	CReal						rNormCenter;

	CReal						rNormCurFreqOffset;

	_BOOLEAN					bNewDemodType;
	EDemodType					eDemodType;
	int							iFilterBW;

	CReal						rFiltCentOffsNorm;
	CReal						rBandWidthNorm;

	CReal						rAvAmplEst;
	EAGCType					eAGCType;

	CComplex					cOldVal;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(AMDEMOD_H__3B0BEVJBN8LKH2934BGF4344_BB27912__INCLUDED_)
