/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See MLC.cpp
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

#if !defined(MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../util/ReceiverModul.h"
#include "../util/TransmitterModul.h"
#include "../Parameter.h"
#include "../tables/TableMLC.h"
#include "../tables/TableCarMap.h"
#include "ConvEncoder.h"
#include "ViterbiDecoder.h"
#include "Metric.h"
#include "BitInterleaver.h"
#include "QAMMapping.h"
#include "EnergyDispersal.h"


/* Classes ********************************************************************/
class CMLC
{
public:
	CMLC(EChanType ct) : iN_mux(0), eChannelType(ct) {}
	virtual ~CMLC() {}

	void CalculateParam(CParameter& Parameter, int iNewChannelType);

protected:

	int	iLevels;
	/* No input bits for each level. First index: Level, second index:
	   Protection level.
	   For three levels: [M_0,l  M_1,l  M2,l]
	   For six levels: [M_0,lRe  M_0,lIm  M_1,lRe  M_1,lIm  M_2,lRe  ...  ] */
	int	iM[MC_MAX_NUM_LEVELS][2];
	int iN[2];
	int iL[3];
	int iN_mux;
	int iCodeRate[MC_MAX_NUM_LEVELS][2];

	const int* piInterlSequ;

	int	iNumEncBits;

	EChanType	eChannelType;
	ECodScheme	eCodingScheme;

	void CalculateFACParam(CParameter& Parameter);
	void CalculateSDCParam(CParameter& Parameter);
	void CalculateMSCParam(CParameter& Parameter);

};

class CMLCEncoder : public CTransmitterModul<_BINARY, _COMPLEX>,
					public CMLC
{
public:
	CMLCEncoder(EChanType ct):CTransmitterModul<_BINARY, _COMPLEX>(),CMLC(ct),
	ConvEncoder(MC_MAX_NUM_LEVELS), BitInterleaver(2),
	QAMMapping(), EnergyDisp(),
	vecEncInBuffer(MC_MAX_NUM_LEVELS),vecEncOutBuffer(MC_MAX_NUM_LEVELS)
	{
	}
	virtual ~CMLCEncoder() {}

protected:
	vector<CConvEncoder>		ConvEncoder;
	/* Two different types of interleaver table */
	vector<CBitInterleaver>		BitInterleaver;
	CQAMMapping					QAMMapping;
	CEnergyDispersal			EnergyDisp;

	/* Internal buffers */
	vector<CVector<_DECISION> >	vecEncInBuffer;
	vector<CVector<_DECISION> >	vecEncOutBuffer;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& Parameter);
};

class CMLCDecoder : public CReceiverModul<CEquSig, _BINARY>,
					public CMLC
{
public:
	CMLCDecoder(EChanType ct):CReceiverModul<CEquSig, _BINARY>(),CMLC(ct),
	ViterbiDecoder(MC_MAX_NUM_LEVELS), MLCMetric(),
	BitDeinterleaver(2), BitInterleaver(2),
	ConvEncoder(MC_MAX_NUM_LEVELS), EnergyDisp(),
	vecMetric(),
	vecDecOutBits(MC_MAX_NUM_LEVELS), vecSubsetDef(MC_MAX_NUM_LEVELS),
	iNumOutBits(0), rAccMetric(0.0), iNumIterations(0),
	iInitNumIterations(MC_NUM_ITERATIONS), iIndexLastBranch(0)
	{
	}
	virtual ~CMLCDecoder() {}

	_REAL GetAccMetric() const {return 10 * log10(rAccMetric);}
	void putVectorSpace(CPointMeasure<vector<_COMPLEX> >&);
	void SetNumIterations(int iNewNumIterations)
		{iInitNumIterations = iNewNumIterations; SetInitFlag();}
	int GetInitNumIterations() const {return iInitNumIterations;}

protected:
	vector<CViterbiDecoder>		ViterbiDecoder;
	CMLCMetric					MLCMetric;
	/* Two different types of deinterleaver table */
	vector<CBitDeinterleaver>	BitDeinterleaver;
	vector<CBitInterleaver>		BitInterleaver;
	vector<CConvEncoder>		ConvEncoder;
	CEnergyDispersal			EnergyDisp;

	/* Internal buffers */
	CVector<CDistance>			vecMetric;

	vector<CVector<_DECISION> >	vecDecOutBits;
	vector<CVector<_DECISION> >	vecSubsetDef;
	int							iNumOutBits;

	/* Accumulated metric */
	_REAL						rAccMetric;

	int							iNumIterations;
	int							iInitNumIterations;
	int							iIndexLastBranch;

	virtual void InitInternal(CParameter&);
	virtual void ProcessDataInternal(CParameter&);
};

#endif // !defined(MLC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
