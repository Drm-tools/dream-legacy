/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See TimeSync.cpp
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

#if !defined(TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_)
#define TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../Vector.h"
#include "../matlib/Matlib.h"
#include "TimeSyncFilter.h"


/* Definitions ****************************************************************/
#define LAMBDA_LOW_PASS_START			((_REAL) 0.99)
#define TIMING_BOUND_ABS				150

/* Non-linear correction of the timing if variation is too big */
#define NO_SYM_BEFORE_RESET				5

/* Definitions for robustness mode detection */
#define NO_BLOCKS_FOR_RM_CORR			16
#define THRESHOLD_RELI_MEASURE			((_REAL) 8.0)

/* Downsampling factor. We only use approx. 6 kHz for correlation, therefore
   we can use a decimation of 8 (i.e., 48 kHz / 8 = 6 kHz). Must be 8 since
   all symbol and guard-interval length at 48000 for all robustness modes are
   dividable by 8 */
#define GRDCRR_DEC_FACT					8

#define STEP_SIZE_GUARD_CORR			4


/* Classes ********************************************************************/
class CTimeSync : public CReceiverModul<_REAL, _REAL>
{
public:
	CTimeSync() : iTimeSyncPos(0), bSyncInput(FALSE), bTimingAcqu(FALSE),
		bAcqWasActive(FALSE), bRobModAcqu(FALSE),
		iLengthIntermCRes(NUM_ROBUSTNESS_MODES),
		iPosInIntermCResBuf(NUM_ROBUSTNESS_MODES),
		iLengthOverlap(NUM_ROBUSTNESS_MODES),
		iLenUsefPart(NUM_ROBUSTNESS_MODES),
		iLenGuardInt(NUM_ROBUSTNESS_MODES),
		cGuardCorr(NUM_ROBUSTNESS_MODES),
		rGuardPow(NUM_ROBUSTNESS_MODES),
		cGuardCorrBlock(NUM_ROBUSTNESS_MODES),
		rGuardPowBlock(NUM_ROBUSTNESS_MODES),
		rLambdaCoAv((_REAL) 1.0) {}
	virtual ~CTimeSync() {}

	/* To set the module up for synchronized DRM input data stream */
	void SetSyncInput(_BOOLEAN bNewS) {bSyncInput = bNewS;}

	void StartAcquisition();
	void StopTimingAcqu() {bTimingAcqu = FALSE;}
	void StopRMDetAcqu() {bRobModAcqu = FALSE;}
	void SetFilterTaps(_REAL rNewOffsetNorm);

protected:
	int							iCorrCounter;
	int							iAveCorr;
	int							iStepSizeGuardCorr;

	CShiftRegister<_REAL>		HistoryBuf;
	CShiftRegister<_COMPLEX>	HistoryBufCorr;
	CVector<_REAL>				vecrHistoryFilt;
	CVector<_REAL>				pMovAvBuffer;
	CShiftRegister<_REAL>		pMaxDetBuffer;

	CVector<_REAL>				vecCorrAvBuf;
	int							iCorrAvInd;

	int							iMaxDetBufSize;
	int							iCenterOfMaxDetBuf;

	int							iMovAvBufSize;
	int							iTotalBufferSize;
	int							iSymbolBlockSize;
	int							iDecSymBS;
	int							iGuardSize;
	int							iTimeSyncPos;
	int							iDFTSize;
	_REAL						rStartIndex;

	int							iPosInMovAvBuffer;
	_REAL						rGuardEnergy;

	int							iCenterOfBuf;

	_BOOLEAN					bSyncInput;

	_BOOLEAN					bTimingAcqu;
	_BOOLEAN					bRobModAcqu;
	_BOOLEAN					bAcqWasActive;

	int							iSelectedMode;

	CComplexVector				cvecB;
	CRealVector					rvecZ;
	CRealVector					rvecInpTmp;
	CVector<_COMPLEX>			cvecOutTmpInterm;

	_REAL						rLambdaCoAv;


	/* Intermediate correlation results and robustness mode detection */
	CVector<_COMPLEX>			veccIntermCorrRes[NUM_ROBUSTNESS_MODES];
	CVector<_REAL>				vecrIntermPowRes[NUM_ROBUSTNESS_MODES];
	CVector<int>				iLengthIntermCRes;
	CVector<int>				iPosInIntermCResBuf;
	CVector<int>				iLengthOverlap;
	CVector<int>				iLenUsefPart;
	CVector<int>				iLenGuardInt;

	CVector<_COMPLEX>			cGuardCorr;
	CVector<_COMPLEX>			cGuardCorrBlock;
	CVector<_REAL>				rGuardPow;
	CVector<_REAL>				rGuardPowBlock;

	CRealVector					vecrRMCorrBuffer[NUM_ROBUSTNESS_MODES];
	int							iRMCorrBufSize;
	CRealVector					vecrCos[NUM_ROBUSTNESS_MODES];


	int GetIndFromRMode(ERobMode eNewMode);
	ERobMode GetRModeFromInd(int iNewInd);

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_)
