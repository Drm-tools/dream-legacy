/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See RobModeDetection.cpp
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

#if !defined(ROBMODEDETECTION_H__3B0BA660EDOINBERO912__INCLUDED_)
#define ROBMODEDETECTION_H__3B0BA660EDOINBERO912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../matlib/Matlib.h"


/* Definitions ****************************************************************/
/* Step size for guard-interval correlation for robust mode detection */
#define STEP_SIZE_GUARD_CORR_RMD		40
#define NO_BLOCKS_FOR_FFT				16
#define THRESHOLD_RELI_MEASURE			((_REAL) 8.0)//((_REAL) 10.0)


/* Classes ********************************************************************/
class CRobModDet : public CReceiverModul<_REAL, _REAL>
{
public:
	CRobModDet() : bAquisition(FALSE) {}
	virtual ~CRobModDet() {}

	void StartAcquisition();
	void StopAcquisition() {bAquisition = FALSE;}

protected:
	CShiftRegister<CReal>	vecrHistory;
	CRealVector				vecrFFTBufferModA;
	CRealVector				vecrFFTBufferModB;
	CRealVector				vecrFFTBufferModC;
	CRealVector				vecrFFTBufferModD;

	CRealVector				vecrCosA;
	CRealVector				vecrCosB;
	CRealVector				vecrCosC;
	CRealVector				vecrCosD;

	int						iGuardSizeA;
	int						iGuardSizeB;
	int						iGuardSizeC;
	int						iGuardSizeD;
	int						iObservedFreqBinA;
	int						iObservedFreqBinB;
	int						iObservedFreqBinC;
	int						iObservedFreqBinD;
	int						iTotalBufferSize;
	int						iTimeSyncPos;
	int						iFFTBufSize;

	_BOOLEAN				bAquisition;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(ROBMODEDETECTION_H__3B0BA660EDOINBERO912__INCLUDED_)
