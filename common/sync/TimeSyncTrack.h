/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TimeSyncTrack.cpp
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

#if !defined(TIMESYNCTRACK_H__3B0BA6346234634554344_BBE7A0D31912__INCLUDED_)
#define TIMESYNCTRACK_H__3B0BA6346234634554344_BBE7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Parameter.h"
#include "../Vector.h"
#include "../matlib/Matlib.h"


/* Definitions ****************************************************************/
/* Define target position for first path in guard-interval. The number defines
   the fraction of the guard-interval */
#define TARGET_TI_POS_FRAC_GUARD_INT		9

/* Weights for bound calculation. First parameter is for peak distance and 
   second for distance from minimum value */
#define TETA1_DIST_FROM_MAX_DB				25
#define TETA2_DIST_FROM_MIN_DB				23

/* Control parameters */
#define CONT_PROP_IN_GUARD_INT				((_REAL) 0.08)
#define CONT_PROP_BEFORE_GUARD_INT			((_REAL) 0.1)


/* Classes ********************************************************************/
class CTimeSyncTrack
{
public:
	CTimeSyncTrack() : bTracking(FALSE), bIsInInit(FALSE) {}
	virtual ~CTimeSyncTrack() {}

	void Init(CParameter& Parameter, int iNewSymbDelay);

	int Process(CParameter& Parameter, CComplexVector& veccChanEst,
			    int iNewTiCorr);

	void GetAvPoDeSp(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
					 _REAL& rLowerBound, _REAL& rHigherBound,
					 _REAL& rStartGuard, _REAL& rEndGuard, _REAL& rLenIR);

	void StartTracking() {bTracking = TRUE;}
	void StopTracking() {bTracking = FALSE;}

protected:
	CComplexVector		veccPilots;
	int					iNoIntpFreqPil;
	CFftPlans			FftPlan;
	int					iScatPilFreqInt;
	int					iNoCarrier;
	CRealVector			vecrAvPoDeSp;
	CRealVector			vecrHammingWindow;
	CReal				rConst1;
	CReal				rConst2;
	int					iStPoRot;
	CRealVector			vecrAvPoDeSpRot;
	int					iSymDelay;
	CShiftRegister<int>	vecTiCorrHist;
	CShiftRegister<_REAL>	vecrNewMeasHist;
	
	CReal				rFracPartTiCor;
	int					iTargetTimingPos;

	_BOOLEAN			bTracking;

	int					iDFTSize;

	_REAL				rBoundLower;
	_REAL				rBoundHigher;
	int					iGuardSizeFFT;

	int					iInitCnt;

	int					iEstDelay;

	_BOOLEAN			IsInInit() const {return bIsInInit;}

private:
	_BOOLEAN			bIsInInit;
};


#endif // !defined(TIMESYNCTRACK_H__3B0BA6346234634554344_BBE7A0D31912__INCLUDED_)
