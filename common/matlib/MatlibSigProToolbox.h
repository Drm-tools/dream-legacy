/******************************************************************************\
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	c++ Mathamatic Library (Matlib), signal processing toolbox
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

#ifndef _MATLIB_SIGNAL_PROC_TOOLBOX_H_
#define _MATLIB_SIGNAL_PROC_TOOLBOX_H_

#include "Matlib.h"
#include "MatlibStdToolbox.h"


/* Classes ********************************************************************/
CMatlibVector<CReal>	Randn(const int iLength);
CMatlibVector<CReal>	Sinc(const CMatlibVector<CReal>& fvI);
CMatlibVector<CReal>	Hann(const int iLen);
CMatlibVector<CReal>	Hamming(const int iLen);

/* Filter data with a recursive (IIR) or nonrecursive (FIR) filter */
CMatlibVector<CReal>	Filter(const CMatlibVector<CReal>& fvB, 
							   const CMatlibVector<CReal>& fvA, 
							   const CMatlibVector<CReal>& fvX, 
							   CMatlibVector<CReal>& fvZ);

/* Levinson durbin recursion */
CMatlibVector<CReal>	Levinson(const CMatlibVector<CReal> vecrRx, 
								 const CMatlibVector<CReal> vecrB);
CMatlibVector<CComplex>	Levinson(const CMatlibVector<CComplex> veccRx, 
								 const CMatlibVector<CComplex> veccB);

void GetIIRTaps(CMatlibVector<CReal>& vecrB, CMatlibVector<CReal>& vecrA, 
				const CReal rFreqOff);


#endif	/* _MATLIB_SIGNAL_PROC_TOOLBOX_H_ */
