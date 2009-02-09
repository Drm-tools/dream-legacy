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
#include <vector>
#include <deque>
#include <limits>

class CMeasure
{
public:
    CMeasure() { reset(); }
    virtual ~CMeasure() {}
    void subscribe() { subscriptions++; }
    void unsubscribe() { --subscriptions; }
    bool wanted() const { return subscriptions>0; }
    void setAvailable() { available=true; }
    bool getAvailable() { return available; }
    virtual void reset() { subscriptions=0; available=false; validdata=false;}
    virtual bool valid() const { return validdata; }
    void invalidate() { validdata=false; }
protected:
    bool available, validdata;
    unsigned int subscriptions;
};

template<typename T>
class CPointMeasure : public CMeasure
{
public:
    CPointMeasure():CMeasure(),value(){}
    virtual ~CPointMeasure(){}
    void set(const T& v) { value = v; validdata=true; }
    T get() const { return value; }
    bool get(T& v) const
    {
        if(valid())
        {
            v=value;
            return true;
        }
        return false;
    }
protected:
    T value;
};

template<typename T>
class CTimeSeries : public CMeasure
{
public:
    CTimeSeries():CMeasure(),history(),max(0),interval(0.0){}
    void configure(size_t m, _REAL d) { max=m; interval=d;}
    virtual void reset() { CMeasure::reset(); history.clear(); max=0;interval=0.0;}
    void set(const T& v)
    {
        history.push_front(v);
        if(history.size()>max)
            history.pop_back();
        validdata=true;
    }
    T get_newest() const { return history.front();}
    T get_oldest() const { return history.back();}
    bool get(vector<T>& v)
    {
        if(valid())
        {
            v.resize(history.size());
            v.assign(history.begin(),history.end());
            return true;
        }
        return false;
    }
protected:
    deque<T> history;
    size_t max;
    _REAL interval;
};

template<typename T>
class CMinMaxMean : public CDumpable, public CMeasure
{
public:
    CMinMaxMean();

    void addSample(T);
    T getCurrent() const;
    T getMean();
    bool getCurrent(T&) const;
    bool getMean(T&);
    bool getMinMax(T&, T&);
    virtual void reset();
    void dump(ostream&) const;
protected:
    T sum, cur, min, max;
    int num;
};

class CMeasurements
{
public:

    CMeasurements();

	CMinMaxMean<_REAL> SNRstat, SigStrstat;

	CPointMeasure<_REAL> MER;
	CPointMeasure<_REAL> WMERMSC;
	CPointMeasure<_REAL> WMERFAC;

    /* Doppler */
    CPointMeasure<_REAL> Doppler;
    CPointMeasure<pair<_REAL,_REAL> > Delay;

	_REAL rRdop;
	vector<_REAL> vecrRdel;
	vector<_REAL> vecrRdelThresholds;
	vector<_REAL> vecrRdelIntervals;
	/* interference (constellation-based measurement rnic)*/
	struct CInterferer { _REAL rIntFreq, rINR, rICR; };
	CPointMeasure<CInterferer> interference;

	/* peak of PSD - for PSD-based interference measurement rnip */
	_REAL rMaxPSDwrtSig;
	_REAL rMaxPSDFreq;
	bool bETSIPSD; // ETSI PSD scale or old Dream ?

	CPointMeasure<vector<_REAL> > PSD;
	CPointMeasure<vector<_REAL> > inputSpectrum;

	CPointMeasure<_REAL> SigStr;
	CPointMeasure<_REAL> IFSigStr;

    CPointMeasure<vector<_COMPLEX> > ChannelEstimate;

	CTimeSeries<bool> audioFrameStatus;

	_REAL rPIRStart, rPIREnd;
	// vector to hold impulse response values for (proposed) rpir tag
	vector <_REAL> vecrPIR;
	CPointMeasure<vector<vector<_COMPLEX> > > Pilots;

    _REAL rStartGuard, rEndGuard, rLowerBound, rHigherBound, rPDSBegin, rPDSEnd;

protected:

};

template<typename T>
CMinMaxMean<T>::CMinMaxMean():CMeasure(),CDumpable(),sum(0),cur(),
min(numeric_limits<T>::max()),max(numeric_limits<T>::min()),num(0)
{
}

template<typename T>
void CMinMaxMean<T>::reset()
{
    CMeasure::reset();
    sum = 0;
	num = 0;
	min = numeric_limits<T>::max();
	max = numeric_limits<T>::min();
}

template<typename T>
void CMinMaxMean<T>::addSample(T val)
{
	cur = val;
	sum += val;
	num++;
	if(val>max)
		max = val;
	if(val<min)
		min = val;
    validdata=true;
}

template<typename T>
T CMinMaxMean<T>::getCurrent() const
{
	return cur;
}

template<typename T>
bool CMinMaxMean<T>::getCurrent(T&) const
{
}

template<typename T>
T CMinMaxMean<T>::getMean()
{
    if(!valid())
        return 0;
	T mean = 0;
	if(num>0)
		mean = sum / T(num);
    reset();
	return mean;
}

template<typename T>
bool CMinMaxMean<T>::getMean(T& val)
{
    if(valid())
    {
        val=getMean();
        return true;
    }
    return false;
}

template<typename T>
bool CMinMaxMean<T>::getMinMax(T& minOut, T& maxOut)
{
    if(!valid())
        return false;
    minOut = min;
    maxOut = max;
    reset();
    return true;
}

template<typename T>
void
CMinMaxMean<T>::dump(ostream& out) const
{
    out << "{ Sum: " <<  sum << "," << endl;
    out << "Cur: " << cur << "," << endl;
    out << "Min: " << min << "," << endl;
    out << "Max " << max << "," << endl;
    out << "Num: " << num << "}" << endl;
}

#endif
