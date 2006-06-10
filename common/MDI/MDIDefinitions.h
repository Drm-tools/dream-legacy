/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	Definitions used by the other MDI classes
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



#ifndef MDI_DEFINITIONS_H_INCLUDED
#define MDI_DEFINITIONS_H_INCLUDED

#include "../GlobalDefinitions.h"
#include "../util/Vector.h"


/* Definitions ****************************************************************/
/* Major revision: Currently 0000_[16] */
#define MDI_MAJOR_REVISION			0

/* Minor revision: Currently 0000_[16] */
#define MDI_MINOR_REVISION			0

/* Major revision: Currently 0003_[16] */
#define RSCI_MAJOR_REVISION			3

/* Minor revision: Currently 0000_[16] */
#define RSCI_MINOR_REVISION			0

/* Major revision of the AF protocol in use: Currently 01_[16] */
#define AF_MAJOR_REVISION			1

/* Minor revision of the AF protocol in use: Currently 00_[16] */
#define AF_MINOR_REVISION			0

/* MDI input */

/* Length of the MDI in buffer */
#define MDI_IN_BUF_LEN				4


	/* Class for keeping track of status flags for RSCI rsta tag */
class CRSCIStatusFlags
{
public:
	CRSCIStatusFlags(): bSyncOK(FALSE), bFACOK(FALSE), bSDCOK(FALSE), bNextSDCOK(FALSE), bAudioOK(FALSE) {}

	void SetSyncStatus(const _BOOLEAN bOK) { bSyncOK = bOK;}
	void SetFACStatus(const _BOOLEAN bOK) { bFACOK = bOK;}
	void SetSDCStatus(const _BOOLEAN bOK) { bNextSDCOK = bOK;} /* Delay SDC by one frame */
	void SetAudioStatus(const _BOOLEAN bOK) { bAudioOK = bOK;}
	void Update(void) {bSyncOK = FALSE; bSDCOK = bNextSDCOK, bFACOK = FALSE; bAudioOK = FALSE;} /* SDC keeps its value if no SDC is present */

	_BOOLEAN GetFACStatus() const {return bFACOK;}
	_BOOLEAN GetSDCStatus() const {return bSDCOK;}
	_BOOLEAN GetAudioStatus() const {return bAudioOK;}
	_BOOLEAN GetSyncStatus() const {return bSyncOK;}

private:
	_BOOLEAN bFACOK;
	_BOOLEAN bSDCOK;
	_BOOLEAN bNextSDCOK;
	_BOOLEAN bAudioOK;
	_BOOLEAN bSyncOK;
};

#endif
