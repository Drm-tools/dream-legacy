/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
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

#include "Utilities.h"
#include <sstream>

#ifdef _WIN32
# ifndef NOMINMAX
#  define NOMINMAX
# endif
/* Always include winsock2.h before windows.h */
# include <winsock2.h>
# include <ws2tcpip.h>
# include <windows.h>
#else
# include <sys/ioctl.h>
# ifndef __linux__
#  include <sys/socket.h>
# endif
# include <net/if.h>
# include <netinet/in.h>
# include <arpa/inet.h>
typedef int SOCKET;
# define SOCKET_ERROR				(-1)
# define INVALID_SOCKET				(-1)
#endif

#ifdef HAVE_LIBPCAP
# include <pcap.h>
#endif

#include <limits>

/* Implementation *************************************************************/
/******************************************************************************\
* Signal level meter                                                           *
\******************************************************************************/
void
CSignalLevelMeter::doUpdate(const _REAL rVal)
{
	/* Search for maximum. Decrease this max with time */
	/* Decrease max with time */
	if (rCurLevel >= METER_FLY_BACK)
		rCurLevel -= METER_FLY_BACK;
	else
	{
		if ((rCurLevel <= METER_FLY_BACK) && (rCurLevel > 1))
			rCurLevel -= 2;
	}

	/* Search for max */
	const _REAL rCurAbsVal = Abs(rVal);
	if (rCurAbsVal > rCurLevel)
		rCurLevel = rCurAbsVal;
}

void
CSignalLevelMeter::Update(const _REAL rVal)
{
	//Mutex.Lock();
	doUpdate(rVal);
	//Mutex.Unlock();
}

void
CSignalLevelMeter::Update(const vector < _SAMPLE > vecsVal)
{
	//Mutex.Lock();
	/* Do the update for entire vector, convert to real */
	for (size_t i = 0; i < vecsVal.size(); i++)
		doUpdate((_REAL) vecsVal[i]);
	//Mutex.Unlock();
}

void
CSignalLevelMeter::Update(const CVector < _REAL > vecsVal)
{
	//Mutex.Lock();
	/* Do the update for entire vector, convert to real */
	const int iVecSize = vecsVal.Size();
	for (int i = 0; i < iVecSize; i++)
		doUpdate(vecsVal[i]);
	//Mutex.Unlock();
}

_REAL CSignalLevelMeter::Level() const
{
	//Mutex.Lock();
	const _REAL
		rNormMicLevel = rCurLevel / numeric_limits<_SAMPLE>::max();
	//fvMutex.Unlock();

	/* Logarithmic measure */
	if (rNormMicLevel > 0)
		return 20.0 * log10(rNormMicLevel);
	else
		return RET_VAL_LOG_0;
}

/******************************************************************************\
* Bandpass filter                                                              *
\******************************************************************************/
void
CDRMBandpassFilt::Process(CVector < _COMPLEX > &veccData)
{
	int i;

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		cvecDataTmp[i] = veccData[i];

	/* Apply FFT filter */
	cvecDataTmp =
		CComplexVector(FftFilt
					   (cvecB, Real(cvecDataTmp), rvecZReal, FftPlanBP),
					   FftFilt(cvecB, Imag(cvecDataTmp), rvecZImag,
							   FftPlanBP));

	/* Copy CVector data in CMatlibVector */
	for (i = 0; i < iBlockSize; i++)
		veccData[i] = cvecDataTmp[i];
}

void
CDRMBandpassFilt::Init(int iNewBlockSize, _REAL rOffsetHz,
                        CReal rSignalBW, CReal rMargin)
{
	/* Set internal parameter */
	iBlockSize = iNewBlockSize;

	/* Init temporary vector */
	cvecDataTmp.Init(iBlockSize);

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	CReal rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;

	/* Band-pass filter bandwidth */
	CReal rBPFiltBW = (rSignalBW + rMargin) / SOUNDCRD_SAMPLE_RATE;

	if(rSignalBW < 4600.0)
	{
		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 2190.0) / SOUNDCRD_SAMPLE_RATE;
	} else if(rSignalBW < 5100.0)
	{
		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 2440.0) / SOUNDCRD_SAMPLE_RATE;
	} else if(rSignalBW < 9100.0)
	{
		/* Centered */
		rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;
	} else if(rSignalBW < 1100.0)
	{
		/* Centered */
		rNormCurFreqOffset = rOffsetHz / SOUNDCRD_SAMPLE_RATE;
	} else if(rSignalBW < 1900.0)
	{
		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 4500.0) / SOUNDCRD_SAMPLE_RATE;
	} else
	{
		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rOffsetHz + (CReal) 5000.0) / SOUNDCRD_SAMPLE_RATE;
	}

	/* FFT plan is initialized with the long length */
	FftPlanBP.Init(iBlockSize * 2);

	/* State memory (init with zeros) and data vector */
	rvecZReal.Init(iBlockSize, (CReal) 0.0);
	rvecZImag.Init(iBlockSize, (CReal) 0.0);
	rvecDataReal.Init(iBlockSize);
	rvecDataImag.Init(iBlockSize);

	/* "+ 1" because of the Nyquist frequency (filter in frequency domain) */
	cvecB.Init(iBlockSize + 1);

	/* Actual filter design */
	CRealVector vecrFilter(iBlockSize);
	vecrFilter = FirLP(rBPFiltBW, Nuttallwin(iBlockSize));

	/* Copy actual filter coefficients. It is important to initialize the
	   vectors with zeros because we also do a zero-padding */
	CRealVector rvecB(2 * iBlockSize, (CReal) 0.0);

	/* Modulate filter to shift it to the correct IF frequency */
	for (int i = 0; i < iBlockSize; i++)
	{
		rvecB[i] =
			vecrFilter[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);
	}

	/* Transformation in frequency domain for fft filter */
	cvecB = rfft(rvecB, FftPlanBP);
}

/******************************************************************************\
* Modified Julian Date                                                         *
\******************************************************************************/
void
CModJulDate::Set(const uint32_t iModJulDate)
{
	uint32_t iZ, iA, iAlpha, iB, iC, iD, iE;
	_REAL rJulDate, rF;

	/* Definition of the Modified Julian Date */
	rJulDate = (_REAL) iModJulDate + 2400000.5;

	/* Get "real" date out of Julian Date
	   (Taken from "http://mathforum.org/library/drmath/view/51907.html") */
	// 1. Add .5 to the JD and let Z = integer part of (JD+.5) and F the
	// fractional part F = (JD+.5)-Z
	iZ = (uint32_t) (rJulDate + (_REAL) 0.5);
	rF = (rJulDate + (_REAL) 0.5) - iZ;

	// 2. If Z < 2299161, take A = Z
	// If Z >= 2299161, calculate alpha = INT((Z-1867216.25)/36524.25)
	// and A = Z + 1 + alpha - INT(alpha/4).
	if (iZ < 2299161)
		iA = iZ;
	else
	{
		iAlpha = (int) (((_REAL) iZ - (_REAL) 1867216.25) / (_REAL) 36524.25);
		iA = iZ + 1 + iAlpha - (int) ((_REAL) iAlpha / (_REAL) 4.0);
	}

	// 3. Then calculate:
	// B = A + 1524
	// C = INT( (B-122.1)/365.25)
	// D = INT( 365.25*C )
	// E = INT( (B-D)/30.6001 )
	iB = iA + 1524;
	iC = (int) (((_REAL) iB - (_REAL) 122.1) / (_REAL) 365.25);
	iD = (int) ((_REAL) 365.25 * iC);
	iE = (int) (((_REAL) iB - iD) / (_REAL) 30.6001);

	// The day of the month dd (with decimals) is:
	// dd = B - D - INT(30.6001*E) + F
	iDay = iB - iD - (int) ((_REAL) 30.6001 * iE);	// + rF;

	// The month number mm is:
	// mm = E - 1, if E < 13.5
	// or
	// mm = E - 13, if E > 13.5
	if ((_REAL) iE < 13.5)
		iMonth = iE - 1;
	else
		iMonth = iE - 13;

	// The year yyyy is:
	// yyyy = C - 4716   if m > 2.5
	// or
	// yyyy = C - 4715   if m < 2.5
	if ((_REAL) iMonth > 2.5)
		iYear = iC - 4716;
	else
		iYear = iC - 4715;
}

/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
	The following code is based on "JCRev: John Chowning's reverberator class"
	by Perry R. Cook and Gary P. Scavone, 1995 - 2004
	which is in "The Synthesis ToolKit in C++ (STK)"
	http://ccrma.stanford.edu/software/stk

	Original description:
	This class is derived from the CLM JCRev function, which is based on the use
	of networks of simple allpass and comb delay filters. This class implements
	three series allpass units, followed by four parallel comb filters, and two
	decorrelation delay lines in parallel at the output.
*/
CAudioReverb::CAudioReverb(const CReal rT60)
{
	/* Delay lengths for 44100 Hz sample rate */
	int lengths[9] = { 1777, 1847, 1993, 2137, 389, 127, 43, 211, 179 };
	const CReal scaler = (CReal) SOUNDCRD_SAMPLE_RATE / 44100.0;

	int delay, i;
	if (scaler != 1.0)
	{
		for (i = 0; i < 9; i++)
		{
			delay = (int) Floor(scaler * lengths[i]);

			if ((delay & 1) == 0)
				delay++;

			while (!isPrime(delay))
				delay += 2;

			lengths[i] = delay;
		}
	}

	for (i = 0; i < 3; i++)
		allpassDelays_[i].Init(lengths[i + 4]);

	for (i = 0; i < 4; i++)
		combDelays_[i].Init(lengths[i]);

	setT60(rT60);
	allpassCoefficient_ = (CReal) 0.7;
	Clear();
}

bool CAudioReverb::isPrime(const int number)
{
/*
	Returns true if argument value is prime. Taken from "class Effect" in
	"STK abstract effects parent class".
*/
	if (number == 2)
		return true;

	if (number & 1)
	{
		for (int i = 3; i < (int) Sqrt((CReal) number) + 1; i += 2)
		{
			if ((number % i) == 0)
				return false;
		}

		return true;			/* prime */
	}
	else
		return false;			/* even */
}

void
CAudioReverb::Clear()
{
	/* Reset and clear all internal state */
	allpassDelays_[0].Reset(0);
	allpassDelays_[1].Reset(0);
	allpassDelays_[2].Reset(0);
	combDelays_[0].Reset(0);
	combDelays_[1].Reset(0);
	combDelays_[2].Reset(0);
	combDelays_[3].Reset(0);
}

void
CAudioReverb::setT60(const CReal rT60)
{
	/* Set the reverberation T60 decay time */
	for (int i = 0; i < 4; i++)
	{
		combCoefficient_[i] = pow((CReal) 10.0, (CReal) (-3.0 *
														 combDelays_[i].
														 Size() / (rT60 *
																   SOUNDCRD_SAMPLE_RATE)));
	}
}

CReal CAudioReverb::ProcessSample(const CReal rLInput, const CReal rRInput)
{
	/* Compute one output sample */
	CReal
		temp,
		temp0,
		temp1,
		temp2;

	/* Mix stereophonic input signals to mono signal (since the maximum value of
	   the input signal is 0.5 * max due to the current implementation in
	   AudioSourceDecoder.cpp, we cannot get an overrun) */
	const CReal
		input = rLInput + rRInput;

	temp = allpassDelays_[0].Get();
	temp0 = allpassCoefficient_ * temp;
	temp0 += input;
	allpassDelays_[0].Add((int) temp0);
	temp0 = -(allpassCoefficient_ * temp0) + temp;

	temp = allpassDelays_[1].Get();
	temp1 = allpassCoefficient_ * temp;
	temp1 += temp0;
	allpassDelays_[1].Add((int) temp1);
	temp1 = -(allpassCoefficient_ * temp1) + temp;

	temp = allpassDelays_[2].Get();
	temp2 = allpassCoefficient_ * temp;
	temp2 += temp1;
	allpassDelays_[2].Add((int) temp2);
	temp2 = -(allpassCoefficient_ * temp2) + temp;

	const CReal
		temp3 = temp2 + (combCoefficient_[0] * combDelays_[0].Get());
	const CReal
		temp4 = temp2 + (combCoefficient_[1] * combDelays_[1].Get());
	const CReal
		temp5 = temp2 + (combCoefficient_[2] * combDelays_[2].Get());
	const CReal
		temp6 = temp2 + (combCoefficient_[3] * combDelays_[3].Get());

	combDelays_[0].Add((int) temp3);
	combDelays_[1].Add((int) temp4);
	combDelays_[2].Add((int) temp5);
	combDelays_[3].Add((int) temp6);

	return temp3 + temp4 + temp5 + temp6;
}

#ifdef _WIN32

void GetNetworkInterfaces(vector<CIpIf>& vecIpIf)
{
	vecIpIf.clear();
	CIpIf i;
	i.name = "any";
	i.addr = 0;
	vecIpIf.push_back(i);
#ifdef HAVE_LIBPCAP
	pcap_if_t *alldevs;
	pcap_if_t *d;
	char errbuf[PCAP_ERRBUF_SIZE+1];
	/* Retrieve the device list */
	if(pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		cerr << "Can't get network interfaces: " << errbuf << endl;
		return;
	}
	for(d=alldevs;d;d=d->next)
	{
		pcap_addr_t *a=d->addresses;
		i.addr = ntohl(((struct sockaddr_in *)a->addr)->sin_addr.s_addr);
		i.name = d->name;
		vecIpIf.push_back(i);
	}

	/* Free the device list */
	pcap_freealldevs(alldevs);
#endif
}

#else

void GetNetworkInterfaces(vector<CIpIf>& vecIpIf)
{
	char buff[1024];
	struct ifconf ifc;
	struct ifreq *ifr;
	int skfd;

	CIpIf i;
	i.name = "any";
	i.addr = 0;

	vecIpIf.clear();
	vecIpIf.push_back(i);

	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
	{
		perror("socket");
		return;
	}

	ifc.ifc_len = sizeof(buff);
	ifc.ifc_buf = buff;
	if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0)
	{
		perror("ioctl(SIOCGIFCONF)");
		return;
	}

	ifr = ifc.ifc_req;

	for (size_t j = 0; j < ifc.ifc_len / sizeof(struct ifreq); j++)
	{
		CIpIf i;
		i.addr = ntohl(((struct sockaddr_in *)&ifr[j].ifr_addr)->sin_addr.s_addr);
		i.name = ifr[j].ifr_name;
		vecIpIf.push_back(i);
	}
}
#endif
