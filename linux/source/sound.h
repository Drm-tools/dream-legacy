/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Alexander Kurpiers, Volker Fischer
 *
 * Decription:
 * Linux sound interface
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

#if !defined(AFX_SOUNDIN_H__9518A621345F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621345F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include "../../common/GlobalDefinitions.h"
#include "../../common/Vector.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>


/* Definitions ****************************************************************/
#define	NO_IN_OUT_CHANNELS		2		/* Stereo recording (but we only
										   use one channel for recording) */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

#define RECORDING_CHANNEL		1		/* 0: Left, 1: Right

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NO_SOUND_BUFFERS_IN		30		/* Number of sound card buffers */


/* Classes ********************************************************************/
class CSound
{
public:
	CSound() {}
	virtual ~CSound() {}
#if WITH_SOUND
	void InitRecording(int iNewBufferSize);
	void InitPlayback(int iNewBufferSize);
	void Read(CVector<short>& psData);
	void Write(CVector<short>& psData);

	void StopRecording();
	
protected:
	int 	iBufferSize, iInBufferSize;
	void InitIF( int &fdSound );
	short int *tmpplaybuf, *tmprecbuf;
#else
	/* dummy definitions */
	void InitRecording(int iNewBufferSize){}
	void InitPlayback(int iNewBufferSize){}
	void Read(CVector<short>& psData){}
	void Write(CVector<short>& psData){}
	void StopRecording(){}
#endif
};


#endif // !defined(AFX_SOUNDIN_H__9518A621345F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
