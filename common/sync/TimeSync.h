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


/* Definitions ****************************************************************/
#define STEP_SIZE_GUARD_CORR			40
#define LAMBDA_LOW_PASS_START			((_REAL) 0.99)
#define TIMING_BOUND_ABS				150

/* Non-linear correction of the timing if variation is too big */
#define NO_SYM_BEFORE_RESET				5


/* Classes ********************************************************************/
class CTimeSync : public CReceiverModul<_REAL, _REAL>
{
public:
	CTimeSync() : iTimeSyncPos(0), bSyncInput(FALSE), bAquisition(FALSE),
		bAcqWasActive(FALSE) {}
	virtual ~CTimeSync() {}

	/* To set the module up for synchronized DRM input data stream */
	void SetSyncInput(_BOOLEAN bNewS) {bSyncInput = bNewS;}

	void StartAcquisition();
	void StopAcquisition() {bAquisition = FALSE;}

protected:
	int						iCorrCounter;
	int						iAveCorr;
	int						iStepSizeGuardCorr;

	CShiftRegister<_REAL>	HistoryBuf;
	CVector<_REAL>			vecrHistoryFilt;
	CVector<_REAL>			pMovAvBuffer;
	CShiftRegister<_REAL>	pMaxDetBuffer;

	int						iMaxDetBufSize;
	int						iCenterOfMaxDetBuf;

	int						iMovAvBufSize;
	int						iTotalBufferSize;
	int						iSymbolBlockSize;
	int						iGuardSize;
	int						iTimeSyncPos;
	int						iDFTSize;
	_REAL					rStartIndex;

	int						iPosInMovAvBuffer;
	_REAL					rGuardEnergy;

	int						iCenterOfBuf;

	/* Intermediate correlation results */
	CVector<_REAL>			vecrIntermCorrRes;
	CVector<_REAL>			vecrIntermPowRes;
	int						iLengthIntermCRes;
	int						iPosInIntermCResBuf;
	int						iLengthOverlap;

	_REAL					rGuardCorr;
	_REAL					rGuardPow;
	_REAL					rGuardCorrBlock;
	_REAL					rGuardPowBlock;

	_BOOLEAN				bSyncInput;

	_BOOLEAN				bAquisition;
	_BOOLEAN				bAcqWasActive;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(TIMESYNC_H__3B0BEVJBN872345NBEROUEBGF4344_BB27912__INCLUDED_)
