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

#if !defined(DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_)
#define DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_

#include <qwt_plot.h>
#include "../Vector.h"
#include "../Parameter.h"


/* Definitions ****************************************************************/
/* Define the plot color profile */
//#define FHG_DRM_SOFTWARE_RADIO_CP
#define MATLAB_LIKE_CP

#ifdef FHG_DRM_SOFTWARE_RADIO_CP
# define MAIN_PEN_COLOR_PLOT			green
# define MAIN_PEN_COLOR_CONSTELLATION	red
# define BCKGRD_COLOR_PLOT				black
# define MAIN_GRID_COLOR_PLOT			QColor(128, 0, 0)
# define SPEC_LINE1_COLOR_PLOT			yellow
# define SPEC_LINE2_COLOR_PLOT			blue
#endif

#ifdef MATLAB_LIKE_CP
# define MAIN_PEN_COLOR_PLOT			blue
# define MAIN_PEN_COLOR_CONSTELLATION	blue
# define BCKGRD_COLOR_PLOT				white
# define MAIN_GRID_COLOR_PLOT			gray
# define SPEC_LINE1_COLOR_PLOT			red
# define SPEC_LINE2_COLOR_PLOT			black
#endif


/* Classes ********************************************************************/
class CDRMPlot : public QwtPlot
{
    Q_OBJECT

public:
	CDRMPlot(QWidget *p = 0, const char *name = 0);
	virtual ~CDRMPlot() {}

	void SetAvIR(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale, 
				 _REAL rLowerB, _REAL rHigherB,
				 const _REAL rStartGuard, const _REAL rEndGuard, 
				 const _REAL rEndIR);
	void SetTranFct(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetPSD(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetInpSpec(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale);
	void SetFACConst(CVector<_COMPLEX>& veccData);
	void SetSDCConst(CVector<_COMPLEX>& veccData,
					 CParameter::ECodScheme eNewCoSc);
	void SetMSCConst(CVector<_COMPLEX>& veccData,
					 CParameter::ECodScheme eNewCoSc);

protected:
	void SetData(CVector<_REAL>& vecrData, CVector<_REAL>& vecrScale,
				 const int  size = 0);
	void SetData(CVector<_COMPLEX>& veccData, QColor color, const int  size);
	void SetQAM4Grid();
	void SetQAM16Grid();
	void SetQAM64Grid();
};


#endif // DRMPLOT_H__FD6B2345234523453_804E1606C2AC__INCLUDED_
