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


/* Implementation *************************************************************/
/******************************************************************************\
* Signal level meter                                                           *
\******************************************************************************/
void CSignalLevelMeter::Update(const _REAL rVal)
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

void CSignalLevelMeter::Update(const CVector<_REAL> vecrVal)
{
	/* Do the update for entire vector */
	const int iVecSize = vecrVal.Size();
	for (int i = 0; i < iVecSize; i++)
		Update(vecrVal[i]);
}

void CSignalLevelMeter::Update(const CVector<_SAMPLE> vecsVal)
{
	/* Do the update for entire vector, convert to real */
	const int iVecSize = vecsVal.Size();
	for (int i = 0; i < iVecSize; i++)
		Update((_REAL) vecsVal[i]);
}

_REAL CSignalLevelMeter::Level()
{
	const _REAL rNormMicLevel = rCurLevel / _MAXSHORT;

	/* Logarithmic measure */
	if (rNormMicLevel > 0)
		return (_REAL) 20.0 * log10(rNormMicLevel);
	else
		return RET_VAL_LOG_0;
}
