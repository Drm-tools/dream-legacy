/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TimeWiener.cpp
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

#if !defined(TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_)
#define TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Vector.h"
#include "../ofdmcellmapping/OFDMCellMapping.h"
#include "../matlib/Matlib.h"
#include "ChanEstTime.h"


/* Definitions ****************************************************************/
/* Define the number of averaged estimates of the correlation of the channel in
   time direction. This number comes from robustness mode B with 10 kHz 
   bandwidth -> after approx. 5000 symbols a good estimate was done: 
   N = 5000 * 206 / PilGrid (= 2) approx. 500000 */
#define NO_SIGMA_AVER_UNTIL_USE			500000


/* Classes ********************************************************************/
class CTimeWiener : public CChanEstTime
{
public:
	CTimeWiener() {}
	virtual ~CTimeWiener() {}

	virtual int Init(CParameter& ReceiverParam);
	virtual void Estimate(CVectorEx<_COMPLEX>* pvecInputData, 
						  CComplexVector& veccOutputData, 
						  CVector<int>& veciMapTab, 
						  CVector<_COMPLEX>& veccPilotCells);

	void SetSNR(_REAL rNewSNR) {rSNR = rNewSNR;}
	_REAL GetSigma() {return rSigma;}

	
protected:
	CRealVector TimeOptimalFilter(int iTimeInt, int iDiff, _REAL rNewSNR, 
								  _REAL rNewSigma, _REAL rTs, int iLength);
	inline int DisToNextPil(int iPiHiIndex, int iSymNo);
	void UpdateFilterCoef(_REAL rNewSNR, _REAL rNewSigma);
	_REAL ModLinRegr(CVector<_REAL>& vecrCorrEst);

	int					iNoCarrier;
	CVector<_COMPLEX>	veccChanEst;

	int					iLengthWiener;
	int					iNoFiltPhasTi;
	CMatrix<_REAL>		matrFiltTime;
	
	CMatrix<_COMPLEX>	matcChanAtPilPos;

	CVector<_REAL>		vecrTiCorrEst;

	int					iScatPilFreqInt; /* Frequency interpolation */
	int					iScatPilTimeInt; /* Time interpolation */
	int					iNoSymPerFrame;

	/* Number of first symbol with pilot at carrier-number 0 */
	int					iFirstSymbWithPi;

	int					iLenHistBuff;

	CShiftRegister<int>	vecTiCorrHist;
	int					iLenTiCorrHist;
	int					iAvCntSigmaEst;

	_REAL				Ts;
	_REAL				rSNR;
	_REAL				rSigma;
};


#endif // !defined(TIMEWIENER_LINEAR_H__3B0BA660_C345345_4344_D31912__INCLUDED_)
