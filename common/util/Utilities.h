/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Utilities.cpp
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

#if !defined(UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_)
#define UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../Vector.h"
#include "../matlib/Matlib.h"


/* Definitions ****************************************************************/
#define	METER_FLY_BACK				15


/* Classes ********************************************************************/
class CSignalLevelMeter
{
public:
	CSignalLevelMeter() : rCurLevel((_REAL) 0.0) {}
	virtual ~CSignalLevelMeter() {}

	void Init(_REAL rStartVal) {rCurLevel = Abs(rStartVal);}
	void Update(const _REAL rVal);
	void Update(const CVector<_REAL> vecrVal);
	void Update(const CVector<_SAMPLE> vecsVal);
	_REAL Level();

protected:
	_REAL rCurLevel;
};


#endif // !defined(UTILITIES_H__3B0BA660_CA63_4344_B3452345D31912__INCLUDED_)
