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
#include "../chanest/ChanEstTime.h"


/* Definitions ****************************************************************/
#define HI_VALUE_FOR_MIN_SEARCH			3.4E38 /* Max value for float types */
#define CONTR_FREQ_OFF_INTEGRATION		((_REAL) 0.1)
#define CONTR_SAMP_OFF_INTEGRATION		((_REAL) 0.4)


/* Classes ********************************************************************/
class CSyncUsingPil : public CReceiverModul<_COMPLEX, _COMPLEX>, 
					  public CPilotModiClass
{
public:
	CSyncUsingPil() : bSyncInput(FALSE), bAquisition(FALSE), bTrackPil(FALSE),
		iSymbolCounterTiSy(0) {}
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

	int						iDFTSize;
	int						iShiftedKmin;


	/* Variables for frame synchronization */
	CVector<CPilotDiff>		vecDiffFact;
	int						iNoDiffFact;
	CShiftRegister<_REAL>	vecrCorrHistory;

	/* Variables for frequency pilot estimation */
	int						iPosFreqPil[NO_FREQ_PILOTS];
	_COMPLEX				cOldFreqPil[NO_FREQ_PILOTS];
	_REAL					rFreqPilotPhDiff[NO_FREQ_PILOTS];
	_REAL					rNormConstFOE;
	_REAL					rSampleFreqEst;

	int						iSymbolCounterTiSy;

	int						iNoSymPerFrame;

	_BOOLEAN				bBadFrameSync;

	_BOOLEAN				bSyncInput;

	_BOOLEAN				bAquisition;
	_BOOLEAN				bTrackPil;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(SYNCUSINGPIL_H__3B0BA660_CA63_434OBUVEE7A0D31912__INCLUDED_)
