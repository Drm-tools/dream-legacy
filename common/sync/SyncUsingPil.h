/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See SyncUsingPil.cpp
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

#if !defined(SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_)
#define SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../Vector.h"
#include "../chanest/ChanEstTime.h"


/* Definitions ****************************************************************/
#define HI_VALUE_FOR_MIN_SEARCH			((_REAL) 3.4E38) /* Max value for float types */
#define CONTR_SAMP_OFF_INTEGRATION		((_REAL) 10.0)

/* Time constant for IIR averaging of frequency offset estimation */
#define TICONST_FREQ_OFF_EST			((CReal) 20.0) /* sec */

/* Time constant for IIR averaging of sample rate offset estimation */
#define TICONST_SAMRATE_OFF_EST			((CReal) 5.0) /* sec */


/* Classes ********************************************************************/
class CSyncUsingPil : public CReceiverModul<_COMPLEX, _COMPLEX>, 
					  public CPilotModiClass
{
public:
	CSyncUsingPil() : bSyncInput(FALSE), bAquisition(FALSE), bTrackPil(FALSE),
		iSymbCntFraSy(0), iNoSymPerFrame(0),
		iPosFreqPil(NO_FREQ_PILOTS), cOldFreqPil(NO_FREQ_PILOTS),
		cFreqPilotPhDiff(NO_FREQ_PILOTS) {}
	virtual ~CSyncUsingPil() {}

	/* To set the module up for synchronized DRM input data stream */
	void SetSyncInput(_BOOLEAN bNewS) {bSyncInput = bNewS;}

	void StartAcquisition();
	void StopAcquisition() {bAquisition = FALSE;}

	void StartTrackPil() {bTrackPil = TRUE;}
	void StopTrackPil() {bTrackPil = FALSE;}

protected:
	class CPilotDiff
	{
	public:
		_COMPLEX			cDiff;
		int					iNoCarrier;
	};


	/* Variables for frame synchronization */
	CVector<CPilotDiff>		vecDiffFact;
	int						iNoDiffFact;
	CShiftRegister<_REAL>	vecrCorrHistory;

	/* Variables for frequency pilot estimation */
	CVector<int>			iPosFreqPil;
	CVector<_COMPLEX>		cOldFreqPil;
	CVector<_COMPLEX>		cFreqPilotPhDiff;

	_REAL					rNormConstFOE;
	_REAL					rSampleFreqEst;

	int						iSymbCntFraSy;

	int						iNoSymPerFrame;

	_BOOLEAN				bBadFrameSync;

	_BOOLEAN				bSyncInput;

	_BOOLEAN				bAquisition;
	_BOOLEAN				bTrackPil;

	int						iMiddleOfInterval;

	_REAL					rLamSamRaOff;
	_REAL					rLamFreqOff;
	_COMPLEX				cFreqOffVec;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_)
