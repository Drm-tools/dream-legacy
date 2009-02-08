/*****************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy, Julian Cable
 *
 * Description:
 *	See Measurements.cpp
 *
 *******************************************************************************
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
\*******************************************************************************/

#ifndef _MEASUREMENTS_H
#define _MEASUREMENTS_H

#include "GlobalDefinitions.h"
#include "matlib/Matlib.h"
#include <vector>

class CMinMaxMean : public CDumpable
{
public:
    CMinMaxMean();

    void addSample(_REAL);
    _REAL getCurrent();
    _REAL getMean();
    void getMinMax(_REAL&, _REAL&);
    void setInvalid();
    bool isValid();
    void dump(ostream&) const;
protected:
    _REAL rSum, rCur, rMin, rMax;
    int iNum;
};

class CMeasurements
{
public:

    enum eMeasurementType
    {
        PSD,
        DOPPLER,
        DELAY,
        CHANNEL,
        INTERFERENCE,
        INPUT_SPECTRUM,
        MAX_MEASUREMENT_TYPE
    };

    CMeasurements();

    void subscribe(eMeasurementType e) { Subscriptions[e]++; }
    void unsubscribe(eMeasurementType e) { --Subscriptions[e]; }
    bool wanted(eMeasurementType e) { return Subscriptions[e]>0; }

    void SetRSCIDefaults(bool); // TODO - take profile into account

	CMinMaxMean SNRstat, SigStrstat;

	_REAL rMER;
	_REAL rWMERMSC;
	_REAL rWMERFAC;

    /* Doppler */
	_REAL rSigmaEstimate;
	_REAL rMinDelay;
	_REAL rMaxDelay;
	_REAL rRdop;
	vector<_REAL> vecrRdel;
	vector<_REAL> vecrRdelThresholds;
	vector<_REAL> vecrRdelIntervals;
	/* interference (constellation-based measurement rnic)*/
	_REAL rIntFreq, rINR, rICR;

	/* peak of PSD - for PSD-based interference measurement rnip */
	_REAL rMaxPSDwrtSig;
	_REAL rMaxPSDFreq;
	bool bETSIPSD; // ETSI PSD scale or old Dream ?
	vector <_REAL> vecrPSD;
	vector<_REAL> vecrInpSpec;

	_REAL rSigStr;
	_REAL rIFSigStr;
    vector<CComplex> veccChanEst;
	vector <bool> vecAudioFrameStatus;
	_REAL rPIRStart;
	_REAL rPIREnd;
	// vector to hold impulse response values for (proposed) rpir tag
	vector <_REAL> vecrPIR;
	vector<vector<_COMPLEX> > matcReceivedPilotValues;

    _REAL rStartGuard, rEndGuard, rLowerBound, rHigherBound, rPDSBegin, rPDSEnd;

protected:
    int Subscriptions[MAX_MEASUREMENT_TYPE];

	bool bValidSignalStrength;

};

#endif
