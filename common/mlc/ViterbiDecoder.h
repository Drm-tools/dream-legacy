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


/* Definitions ****************************************************************/
/* Data type for Viterbi metric */
#define _VITMETRTYPE				float

/* We initialize each new block of data all branches-metrics with the following
   value exept of the zero-state. This can be done since we actually KNOW that
   the zero state MUST be the transmitted one. The initialization vaule should
   be fairly high */
#define MC_METRIC_INIT_VALUE		((_VITMETRTYPE) 1e10)

/* Decoding depth is MC_CONSTRAINT_LENGTH - 1 because we use the bits which
   are falling out of the shift register in the viterby algorithm. Be
   aware that the decoding depth must be shorter than "_UINT64BIT" has bits! */
#define MC_DECODING_DEPTH			(9 * MC_CONSTRAINT_LENGTH)


/* Classes ********************************************************************/
class CTrellisState
{
public:
	CTrellisState() {rMetric[0] = 0; rMetric[1] = 0;
					 lDecodedBits[0] = 0; lDecodedBits[1] = 0;}
	virtual ~CTrellisState() {}

	/* Index of previous state, Number stands for "0" or "1" transmitted */
	int		iPrev0Index;
	int		iPrev1Index;
	/* Metric for previous states */
	int		iMetricPrev0;
	int		iMetricPrev1;

	/* We need to split old and current metric since it is possible to come from
	   the same state (Same with the decoded bits) */
	_VITMETRTYPE	rMetric[2];
	_UINT64BIT		lDecodedBits[2];
};

class CViterbiDecoder
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
	/* We need to analyze 2^(MC_CONSTRAINT_LENGTH - 1) states in the trellis */
	CTrellisState			vecTrelState[MC_NO_STATES];
	_VITMETRTYPE			vecrMetricSet[MC_NO_OUTPUT_COMBINATIONS];

	int						iNumOutBitsPartA;
	int						iNumOutBits;
	int						iNumOutBitsWithMemory;
	_UINT32BIT				iTailBitPat;
	_UINT32BIT				iPartAPat;
	int						iPartAPatLen;
	_UINT32BIT				iPartBPat;
	int						iPartBPatLen;

	int						iTotalDecDepth;
	_UINT64BIT				lOutBitMask;
	CParameter::EChanType	eChannelType;
};


#endif // !defined(VITERBI_DECODER_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
