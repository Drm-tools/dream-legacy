/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See ChannelEstimation.cpp
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

#if !defined(CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../Parameter.h"
#include "../Modul.h"
#include "../ofdmcellmapping/OFDMCellMapping.h"
#include "../matlib/Matlib.h"
#include "TimeLinear.h"
#include "TimeWiener.h"

#ifdef HAVE_DFFTW_H
# include <dfftw.h>
#else
# include <fftw.h>
#endif

#include "../sync/TimeSyncTrack.h"


/* Definitions ****************************************************************/
#define LEN_WIENER_FILT_FREQ_RMA		6
#define LEN_WIENER_FILT_FREQ_RMB		11
#define LEN_WIENER_FILT_FREQ_RMC		11
#define LEN_WIENER_FILT_FREQ_RMD		13

/* Time constant for IIR averaging of fast signal power estimation */
#define TICONST_SNREST_FAST				((CReal) 30.0) /* sec */

/* Time constant for IIR averaging of slow signal power estimation */
#define TICONST_SNREST_SLOW				((CReal) 100.0) /* sec */

/* Initial value for SNR */
#define INIT_VALUE_SNR_WIEN_FREQ_DB		((_REAL) 30.0) /* dB */

/* SNR estimation initial SNR value */
#define INIT_VALUE_SNR_ESTIM_DB			((_REAL) 20.0) /* dB */


/* Classes ********************************************************************/
class CChannelEstimation : public CReceiverModul<_COMPLEX, CEquSig>
{
public:
	CChannelEstimation() : iLenHistBuff(0), TypeIntFreq(FWIENER), 
		TypeIntTime(TWIENER), eDFTWindowingMethod(DFT_WIN_HAMM) {}
	virtual ~CChannelEstimation() {}

	enum ETypeIntFreq {FLINEAR, FDFTFILTER, FWIENER};
	enum ETypeIntTime {TLINEAR, TWIENER, TDECIDIR};

	void GetTransferFunction(CVector<_REAL>& vecrData, 
		CVector<_REAL>& vecrScale);
	void GetAvPoDeSp(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
					 _REAL& rLowerBound, _REAL& rHigherBound,
					 _REAL& rStartGuard, _REAL& rEndGuard, _REAL& rLenIR);


	CTimeLinear* GetTimeLinear() {return &TimeLinear;}
	CTimeWiener* GetTimeWiener() {return &TimeWiener;}
	CTimeSyncTrack* GetTimeSyncTrack() {return &TimeSyncTrack;}

	/* Set (get) frequency and time interpolation algorithm */
	void SetFreqInt(ETypeIntFreq eNewTy) {TypeIntFreq = eNewTy;}
	ETypeIntFreq GetFreqInt() {return TypeIntFreq;}
	void SetTimeInt(ETypeIntTime eNewTy) {TypeIntTime = eNewTy;
		SetInitFlag();}
	ETypeIntTime GetTimeInt() const {return TypeIntTime;}

	_REAL GetSNREstdB() const;
	_REAL GetSigma() {return TimeWiener.GetSigma();}
	_REAL GetDelay() const;

protected:
	enum EDFTWinType {DFT_WIN_RECT, DFT_WIN_HAMM};
	EDFTWinType			eDFTWindowingMethod;

	int					iNoSymPerFrame;

	CChanEstTime*		pTimeInt;

	CTimeLinear			TimeLinear;
	CTimeWiener			TimeWiener;

	CTimeSyncTrack		TimeSyncTrack;

	ETypeIntFreq		TypeIntFreq;
	ETypeIntTime		TypeIntTime;

	int					iNoCarrier;

	CMatrix<_COMPLEX>	matcHistory;

	int					iLenHistBuff;

	int					iScatPilFreqInt; /* Frequency interpolation */
	int					iScatPilTimeInt; /* Time interpolation */

	CComplexVector		veccChanEst;

	int					iGuardSizeFFT;
	int					iFFTSizeN;

	CRealVector			vecrDFTWindow;
	CRealVector			vecrDFTwindowInv;

	int					iLongLenFreq;
	CComplexVector		veccPilots;
	CComplexVector		veccIntPil;
	CFftPlans			FftPlanShort;
	CFftPlans			FftPlanLong;

	int					iNoIntpFreqPil;

	CReal				rLamSNREstFast;
	CReal				rLamSNREstSlow;

	_REAL				rNoiseEst;
	_REAL				rSignalEst;
	_REAL				rSNREstimate;
	_REAL				rSNRCorrectFact;
	int					iUpCntWienFilt;
	_REAL				rDelaySprEstInd;
	_REAL				rMaxDelaySprInFra;

	int					iStartZeroPadding;

	/* Wiener interpolation in frequency direction */
	void UpdateWienerFiltCoef(_REAL rNewSNR, _REAL rNewRatio);

	CComplexVector FreqOptimalFilter(int iFreqInt, int iDiff, _REAL rSNR, 
									 _REAL rRatGuarLen, int iLength);
	_COMPLEX FreqCorrFct(int iCurPos, _REAL rRatGuarLen);
	CMatrix<_COMPLEX>	matcFiltFreq;
	int					iLengthWiener;
	CVector<int>		veciPilOffTab;

	int					iDCPos;
	int					iPilOffset;
	int					iNoWienerFilt;
	CComplexMatrix		matcWienerFilter;

	int					iInitCnt;

	
	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


#endif // !defined(CHANEST_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
