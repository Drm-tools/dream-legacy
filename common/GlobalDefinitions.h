/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Global definitions
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

#if !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include <complex>
using namespace std;	// Because of the library: "complex"
#include <string>
#include <stdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "tables/TableDRMGlobal.h"


/* Definitions ****************************************************************/
/* When you define the following flag, a directory called
   "test" MUST EXIST in the windows directory (or linux directory if you use
   Linux)! */
#undef _DEBUG_


/* Define the application specific data-types ------------------------------- */
typedef	double							_REAL;
typedef	complex<_REAL>					_COMPLEX;
typedef short							_SAMPLE;
typedef unsigned char					_BYTE;
typedef bool							_BOOLEAN;

// bool seems not to work with linux, therefore "int" TODO: Fix Me!
typedef int/*bool*/						_BINARY;

#if defined(_WIN32)
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
#else
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# else
typedef unsigned long long uint64_t;
typedef unsigned long uint32_t;
# endif
#endif
#endif
typedef uint64_t						_UINT64BIT;
typedef uint32_t						_UINT32BIT;

/* Define type-specific information */
#define SIZEOF__BYTE					8
#define _MAXSHORT						32767


/* Definitions for window message system ------------------------------------ */
#define _MESSAGE_IDENT					unsigned int
#define MS_FAC_CRC						1	// MS: Message
#define MS_SDC_CRC						2
#define MS_MSC_CRC						3
#define MS_FRAME_SYNC					4
#define MS_TIME_SYNC					5

#define GUI_CONTROL_UPDATE_TIME			500	// Milliseconds


/* Global enumerations ------------------------------------------------------ */
enum ESpecOcc {SO_0, SO_1, SO_2, SO_3, SO_4, SO_5}; // SO: Spectrum Occupancy
enum ERobMode {RM_ROBUSTNESS_MODE_A, RM_ROBUSTNESS_MODE_B,
		RM_ROBUSTNESS_MODE_C, RM_ROBUSTNESS_MODE_D}; // RM: Robustness Mode


/* Constants ---------------------------------------------------------------- */
const _REAL crPi = ((_REAL) 3.14159265358979323846);

/* Standard definitions */
#define	TRUE							1
#define FALSE							0


/* Classes ********************************************************************/
/* For metric */
class CDistance
{
public:
	/* Distance towards 0, 1 */
	_REAL rTow0;
	_REAL rTow1;
};

/* Viterbi needs information of equalized received signal and channel */
class CEquSig
{
public:
	_COMPLEX	cSig; /* Actual signal */
	_REAL		rChan; /* Channel power at this cell */
};


/* Prototypes for global functions ********************************************/
/* Posting a window message */
void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam);

/* Debug error handling */
void DebugError(const char* pchErDescr, const char* pchPar1Descr,
				const double dPar1, const char* pchPar2Descr,
				const double dPar2);


/* Global functions ***********************************************************/
/* min() max() functions */
/* Non-const version */
template<class T> inline T& _max(T& a, T& b) {return a > b ? a : b;}
template<class T> inline T& _min(T& a, T& b) {return a < b ? a : b;}

/* Const version */
template<class T> inline const T& _max(const T& a, const T& b) {return a > b ? a : b;}
template<class T> inline const T& _min(const T& a, const T& b) {return a < b ? a : b;}

/* Converting _REAL to _SAMPLE */
inline _SAMPLE Real2Sample(const _REAL rInput)
{
	/* Lower bound */
	if (rInput < -_MAXSHORT)
		return -_MAXSHORT;

	/* Upper bound */
	if (rInput > _MAXSHORT)
		return _MAXSHORT;

	return (_SAMPLE) rInput;
}


#endif // !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
