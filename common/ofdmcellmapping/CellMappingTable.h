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

#if !defined(CELLMAPPINGTABLE_H__3B0BA660_CA63_4347A0D31912__INCLUDED_)
#define CELLMAPPINGTABLE_H__3B0BA660_CA63_4347A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../tables/TableCarMap.h"
#include "../tables/TableFAC.h"
#include "../Vector.h"


/* Definitions ****************************************************************/
/* We define a bit for each flag to allow multiple assignments */
#define	CM_DC			1	/* Bit 0 */ // CM: Carrier Mapping
#define	CM_MSC			2	/* Bit 1 */
#define	CM_SDC			4	/* Bit 2 */
#define	CM_FAC			8	/* Bit 3 */
#define	CM_TI_PI		16	/* Bit 4 */
#define	CM_FRE_PI		32	/* Bit 5 */
#define	CM_SCAT_PI		64	/* Bit 6 */
#define	CM_BOOSTED_PI	128	/* Bit 7 */

/* Function for determining if this is a pilot */
#define _IsPilot(a) (((a) == CM_TI_PI) || ((a) == CM_FRE_PI) || ((a) == CM_SCAT_PI))


/* Classes ********************************************************************/
class CCellMappingTable
{
public:
	CCellMappingTable() : iNoSymbolsPerSuperframe(0) {}
	virtual ~CCellMappingTable() {}

	void MakeTable(ERobMode eNewRobustnessMode, ESpecOcc eNewSpectOccup);

	struct CRatio {int iEnum; int iDenom;};

	/* Mapping table and pilot cell matrix */
	CMatrix<int>		matiMapTab; 
	CMatrix<_COMPLEX>	matcPilotCells;

	int					iNoSymbolsPerSuperframe;
	int					iNoSymPerFrame; /* No of symbols per frame */
	int					iNoCarrier;
	int					iScatPilTimeInt; /* Time interpolation */
	int					iScatPilFreqInt; /* Frequency interpolation */

	int					iMaxNoMSCSym; /* Max no of MSC cells in a symbol */
	int					iMaxNoFACSym; /* Max no of FAC cells in a symbol */
	int					iMaxNoSDCSym; /* Max no of SDC cells in a symbol */

	/* No MSC in symbol */
	CVector<int>		veciNoMSCSym; 

	/* No FAC in symbol */
	CVector<int>		veciNoFACSym; 

	/* No SDC in symbol */
	CVector<int>		veciNoSDCSym;

	int					iFFTSizeN; /* FFT size of the OFDM modulation */
	int					iCarrierKmin; /* Carrier No of carrier with lowest frequency */
	int					iCarrierKmax; /* Carrier No of carrier with highest frequency */
	int					iIndexDCFreq; /* Index of DC carrier */
	int					iShiftedKmin; /* Shifted carrier min ("soundcard pass-band") */
	int					iShiftedKmax; /* Shifted carrier max ("soundcard pass-band") */
	CRatio				RatioTgTu; /* Ratio between guard-interval and useful part */
	int					iGuardSize; /* Length of guard-interval measured in "time-bins" */
	int					iSymbolBlockSize; /* Useful part plus guard-interval in "time-bins" */
	int					iNoIntpFreqPil; /* No of time-interploated frequency pilots */

	int					iNoUsefMSCCellsPerFrame; /* Number of MSC cells per multiplex frame N_{MUX} */
	int					iNoSDCCellsPerSFrame; /* Number of SDC cells per super-frame */

	/* Needed for SNR estimation and simulation */
	_REAL				rAvPowPerSymbol;

protected:
	/* Internal parameters for MakeTable function --------------------------- */
	struct CScatPilots
	{	
		/* For the pase */
		const int*  piConst;
		int			iColSizeWZ;
		const int*	piW;
		const int*	piZ;
		int			iQ;

		/* For the gain */
		const int*	piGainTable;
	};

private:
	_COMPLEX	Polar2Cart(const _REAL rAbsolute, const int iPhase) const;
	int			mod(const int ix, const int iy) const;
};


#endif // !defined(CELLMAPPINGTABLE_H__3B0BA660_CA63_4347A0D31912__INCLUDED_)
