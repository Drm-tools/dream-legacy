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
#include "TransmitterFilter.h"
#include "util/Utilities.h"

#ifdef _WIN32
# include "../../Windows/source/sound.h"
#else
# include "source/sound.h"
#endif


/* Definitions ****************************************************************/
/* Length of vector for input spectrum. We use approx. 0.2 sec
   of sampled data for spectrum calculation, this is 2^13 = 8192 to 
   make the FFT work more efficient. Make sure that this number is not smaller
   than the symbol lenght in 48 khz domain of longest mode (which is mode A/B:
   1280) */
#define NUM_SMPLS_4_INPUT_SPECTRUM	8192

/* Use raw 16 bit data or in text form for file format for DRM data. Defining
   the following macro will enable the raw data option */
#define FILE_DRM_USING_RAW_DATA


/* Classes ********************************************************************/
class CTransmitData : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	enum EOutFormat {OF_REAL_VAL /* real valued */, OF_IQ /* I / Q */,
		OF_EP /* envelope / phase */};

	CTransmitData(CSound* pNS) : pSound(pNS), eOutputFormat(OF_REAL_VAL),
		rDefCarOffset((_REAL) VIRTUAL_INTERMED_FREQ), bUseSoundcard(TRUE),
		strOutFileName("test/TransmittedData.txt"), pFileTransmitter(NULL) {}
	virtual ~CTransmitData();

	void SetIQOutput(const EOutFormat eFormat) {eOutputFormat = eFormat;}
	EOutFormat GetIQOutput() {return eOutputFormat;}

	void SetCarOffset(const CReal rNewCarOffset)
		{rDefCarOffset = rNewCarOffset;}

	void SetWriteToFile(const string strNFN)
	{
		strOutFileName = strNFN;
		bUseSoundcard = FALSE;
	}

protected:
	FILE*			pFileTransmitter;
	CSound*			pSound;
	CVector<short>	vecsDataOut;
	int				iBlockCnt;
	int				iNumBlocks;
	EOutFormat		eOutputFormat;

	CReal			rDefCarOffset;
	CRealVector		rvecA;
	CRealVector		rvecB;
	CRealVector		rvecZReal; /* State memory real part */
	CRealVector		rvecZImag; /* State memory imaginary part */
	CRealVector		rvecDataReal;
	CRealVector		rvecDataImag;

	CReal			rNormFactor;

	int				iBigBlockSize;

	string			strOutFileName;
	_BOOLEAN		bUseSoundcard;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CReceiveData : public CReceiverModul<_REAL, _REAL>
{
public:
	enum EInChanSel {CS_LEFT_CHAN, CS_RIGHT_CHAN, CS_MIX_CHAN};

	CReceiveData(CSound* pNS) : strInFileName("test/TransmittedData.txt"),
		bUseSoundcard(TRUE), bNewUseSoundcard(TRUE), pSound(pNS),
		pFileReceiver(NULL), bFippedSpectrum(FALSE),
		vecrInpData(NUM_SMPLS_4_INPUT_SPECTRUM, (_REAL) 0.0),
		eInChanSelection(CS_MIX_CHAN) {}
	virtual ~CReceiveData();

	_REAL GetLevelMeter() {return SignalLevelMeter.Level();}
	void GetInputSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

	void SetFlippedSpectrum(const _BOOLEAN bNewF) {bFippedSpectrum = bNewF;}
	_BOOLEAN GetFlippedSpectrum() {return bFippedSpectrum;}

	void SetReadFromFile(const string strNFN)
		{bNewUseSoundcard = FALSE; strInFileName = strNFN; SetInitFlag();}

	void SetInChanSel(const EInChanSel eNS) {eInChanSelection = eNS;}
	EInChanSel GetInChanSel() {return eInChanSelection;}

protected:
	CSignalLevelMeter		SignalLevelMeter;
	
	FILE*					pFileReceiver;

	CSound*					pSound;
	CVector<_SAMPLE>		vecsSoundBuffer;

	CShiftRegister<_REAL>	vecrInpData;

	_BOOLEAN				bFippedSpectrum;
	_BOOLEAN				bUseSoundcard;
	_BOOLEAN				bNewUseSoundcard;

	string					strInFileName;

	EInChanSel				eInChanSelection;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(DRMSIGNALIO_H__3B0BA660_CA63_4344_B_23E7A0D31912__INCLUDED_)
