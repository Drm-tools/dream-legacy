/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Alexander Kurpiers, Julian Cable
 *
 * Description:
 *	See Sound.cpp
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

#ifndef __SOUND_H
#define __SOUND_H

#include "GlobalDefinitions.h"
#include "util/Vector.h"

/* Classes ********************************************************************/
class CSoundInterface
{
public:
    CSoundInterface():  vecstrDevices(),iCurDev(0),bChangDev(TRUE),iBufferSize(0){}
	virtual 			~CSoundInterface() { }

	/* return a string array of the user friendly names for each possible device */
	virtual void		Enumerate(vector<string>& names){ names = vecstrDevices; }
	/* select the nth device from the enumerated list.
	 * If negative or out of range, then the highest numbered device will be
	 * used, which will normally be the system default device
	 */
	virtual void		SetDev(int iNewDev) { iCurDev = iNewDev; bChangDev=TRUE; }
	/* return the currently selected device number */
	int					GetDev() {return iCurDev;}
	/* return the name of the device as handed to the OS to open the device */
	virtual string		GetDeviceName()=0;

	/*	(re-)initialise the device,
	 * set the number of sound samples to be read/written by each Read/Write call
	 * to iNewBufferSize, if stereo then 2 samples correspond to one sampling interval
	 */
	virtual void		Init(int iNewBufferSize)=0;
	/* gracefully close the device */
	virtual void		Close()=0;
	/* save a copy of the data to a wav file */
	virtual void		StartRecording(const string& strFileName)=0;
	/* stop saving */
	virtual	void		StopRecording()=0;
	virtual	_BOOLEAN	CurrentlyRecording()=0;

protected:
	vector<string>		vecstrDevices;
	int					iCurDev;
	_BOOLEAN			bChangDev;
	int					iBufferSize;
};

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NUM_SOUND_BUFFERS_IN	24		/* Number of sound card buffers */

#define NUM_SOUND_BUFFERS_OUT	3		/* Number of sound card buffers */

/* Classes ********************************************************************/
class CSoundInInterface : public CSoundInterface
{
public:
	CSoundInInterface():CSoundInterface(){}
	virtual ~CSoundInInterface(){}

	/* read iBufferSize 16 bit samples,
	 * half from the left channel, and half from the right, interleaved.
	 * If reading from a sound card, block until enough samples are available.
	 * If reading from a file, sleep if needed to run at the correct rate
	 */
	virtual _BOOLEAN	Read(CVector<_SAMPLE>& psData)=0;
	/* read from a file instead of from a sound card. When get to the end, finish
	 * if realtime is true, pretend to be a soundcard, otherwise go as fast as possible
	 */
	virtual _BOOLEAN	SetReadFromFile(const string& strFileName, _BOOLEAN bRealtime)=0;
};

class CSoundOutInterface : public CSoundInterface
{
public:
	CSoundOutInterface():CSoundInterface(){};
	virtual ~CSoundOutInterface(){}

	/* Write iBufferSize 16 bit samples,
	 * half to the left channel, and half to the right,
	 * interleaved.
	 */
	virtual _BOOLEAN	Write(CVector<_SAMPLE>& psData)=0;
	/* write to a file, instead of to a sound card */
	virtual _BOOLEAN	SetWriteToFile(const string& strFileName)=0;

};

#endif
