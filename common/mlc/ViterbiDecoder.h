/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	
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

#if !defined(VITERBI_DECODER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define VITERBI_DECODER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableMLC.h"
#include "ConvEncoder.h"
#include "ChannelCode.h"


/* Definitions ****************************************************************/
/* Data type for Viterbi metric */
#define _VITMETRTYPE				float

/* We initialize each new block of data all branches-metrics with the following
   value exept of the zero-state. This can be done since we actually KNOW that
   the zero state MUST be the transmitted one. The initialization vaule should
   be fairly high. But we have to be careful choosing this parameter. We
   should not take the largest value possible of the data type of the metric
   variable since in the Viterbi-routine we add something to this value and
   in that case we would force an overrun! */
#define MC_METRIC_INIT_VALUE		((_VITMETRTYPE) 1e10)


/* Classes ********************************************************************/
class CTrellisData
{
public:
	CTrellisData() : rMetric((_VITMETRTYPE) 0.0) {}

	_VITMETRTYPE	rMetric;
};

class CViterbiDecoder : public CChannelCode
{
public:
	CViterbiDecoder();
	virtual ~CViterbiDecoder() {}

	_REAL	Decode(CVector<CDistance>& vecNewDistance,
				   CVector<_BINARY>& vecbiOutputBits);
	void	Init(CParameter::ECodScheme eNewCodingScheme,
				 CParameter::EChanType eNewChannelType, int iN1, int iN2,
			     int iNewNumOutBitsPartA, int iNewNumOutBitsPartB,
			     int iPunctPatPartA, int iPunctPatPartB, int iLevel);

protected:
	/* Two trellis data vectors are needed for current and old state */
	CTrellisData			vecTrelData1[MC_NO_STATES];
	CTrellisData			vecTrelData2[MC_NO_STATES];

	_VITMETRTYPE			vecrMetricSet[MC_NO_OUTPUT_COMBINATIONS];

	CVector<int>			veciTablePuncPat;

	int						iNumOutBits;
	int						iNumOutBitsWithMemory;

	CMatrix<_BINARY>		matbiDecisions;
};


#endif // !defined(VITERBI_DECODER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
