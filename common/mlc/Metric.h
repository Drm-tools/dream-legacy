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

#if !defined(MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableQAMMapping.h"
#include "../Vector.h"
#include "../Parameter.h"


/* Classes ********************************************************************/
class CMLCMetric
{
public:
	CMLCMetric() {}
	virtual ~CMLCMetric() {}

	/* Return the number of used symbols for calculating one branch-metric */
	void	CalculateMetric(CVector<CEquSig>* pcInSymb, 
						    CVector<CDistance>& vecMetric, 
							CVector<_BINARY>& vecbiSubsetDef1, 
							CVector<_BINARY>& vecbiSubsetDef2,
							CVector<_BINARY>& vecbiSubsetDef3, 
							CVector<_BINARY>& vecbiSubsetDef4,
							CVector<_BINARY>& vecbiSubsetDef5,
							CVector<_BINARY>& vecbiSubsetDef6,
							int iLevel, _BOOLEAN bIteration);
	void	Init(int iNewInputBlockSize, CParameter::ECodScheme eNewCodingScheme);

protected:
	inline _REAL Minimum1(_REAL rA, _REAL rB) const;
	inline _REAL Minimum2(_REAL rA, _REAL rB1, _REAL rB2) const;
	inline _REAL Minimum4(_REAL rA, _REAL rB1, _REAL rB2, _REAL rB3, _REAL rB4) const;

	int						iInputBlockSize;
	CParameter::ECodScheme	eMapType;
};


#endif // !defined(MLC_METRIC_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
