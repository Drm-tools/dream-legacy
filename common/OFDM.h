/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See OFDM.cpp
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

#if !defined(OFDM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define OFDM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"

#ifdef HAVE_DFFTW_H
# include <dfftw.h>
#else
# include <fftw.h>
#endif


/* Classes ********************************************************************/
class COFDMModulation : public CTransmitterModul<_COMPLEX, _COMPLEX>
{
public:
	COFDMModulation() {FFTWPlan = NULL;}
	virtual ~COFDMModulation();

protected:
	fftw_plan				FFTWPlan;
	CVector<fftw_complex>	veccFFTWInput;
	CVector<fftw_complex>	veccFFTWOutput;

	int						iShiftedKmin;
	int						iShiftedKmax;
	int						iDFTSize;
	int						iGuardSize;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class COFDMDemodulation : public CReceiverModul<_REAL, _COMPLEX>
{
public:
	COFDMDemodulation() {FFTWPlan = NULL;}
	virtual ~COFDMDemodulation();

	void GetPowDenSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);

	_REAL GetSNREstdB() const {return 10 * log10(rSNREstimate);}

protected:
	CVector<_REAL>			vecrPDSResult;

	fftw_plan				FFTWPlan;
	CVector<fftw_complex>	veccFFTWInput;
	CVector<fftw_complex>	veccFFTWOutput;

	CVector<_REAL>			vecrPowSpec;
	int						iLenPowSpec;

	int						iShiftedKmin;
	int						iShiftedKmax;
	int						iDFTSize;
	int						iGuardSize;
	int						iNoCarrier;

	_COMPLEX				cCurExp;

	_REAL					rNoisePowAvLeft;
	_REAL					rNoisePowAvRight;
	_REAL					rSNREstimate;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};

class COFDMDemodSimulation : public CSimulationModul<_REAL, _COMPLEX>
{
public:
	COFDMDemodSimulation() {FFTWPlan = NULL;}
	virtual ~COFDMDemodSimulation();

protected:
	fftw_plan				FFTWPlan;
	CVector<fftw_complex>	veccFFTWInput;
	CVector<fftw_complex>	veccFFTWOutput;
	int						iStartPointGuardRemov;

	int						iShiftedKmin;
	int						iShiftedKmax;
	int						iDFTSize;
	int						iGuardSize;
	int						iNoCarrier;
	int						iSymbolBlockSize;
	int						iSymbolCounterTiSy;
	int						iNoSymPerFrame;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(OFDM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
