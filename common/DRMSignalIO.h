/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DRMSignalIO.cpp
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

#if !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
#define DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"
#include <math.h>
#include "matlib/Matlib.h"

#ifdef _WIN32
# include "../../Windows/source/sound.h"
#else
# include "source/sound.h"
#endif


/* Definitions ****************************************************************/
#define	METER_FLY_BACK				15

/* Length of vector for input spectrum. We use approx. 0.2 sec
   of sampled data for spectrum calculation, this is 2^13 = 8192 to 
   make the FFT work more efficient */
#define NO_SMPLS_4_INPUT_SPECTRUM	8192


/* Classes ********************************************************************/
class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	CTransmitData() : pFileTransmitter(NULL) {}
	virtual ~CTransmitData();

protected:
	FILE*	pFileTransmitter;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
	CReceiveData(CSound* pNS) : bFippedSpectrum(FALSE), pFileReceiver(NULL),
		bUseSoundcard(TRUE) {pSound = pNS;}
	virtual ~CReceiveData();

	_REAL GetLevelMeter();
	void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

	void SetFlippedSpectrum(_BOOLEAN bNewF) {bFippedSpectrum = bNewF;}
	void SetUseSoundcard(_BOOLEAN bNewUS) {bUseSoundcard = bNewUS;
		SetInitFlag();}

protected:
	void LevelMeter();

	int						iCurMicMeterLev;
	
	FILE*					pFileReceiver;

	CSound*					pSound;
	CVector<_SAMPLE>		vecsSoundBuffer;

	CShiftRegister<_REAL>	vecrInpData;

	_BOOLEAN				bFippedSpectrum;
	_BOOLEAN				bUseSoundcard;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
