/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 * 
 * Decription:
 * PortAudio sound interface
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

#ifndef _DRM_PORTAUDIO_H
#define _DRM_PORTAUDIO_H

#include "../soundinterface.h"
#include <portaudio.h>
#include "pa_ringbuffer.h"

class CPaCommon: public CSelectionInterface
{
public:
						CPaCommon(bool);
	virtual 			~CPaCommon();

	virtual void		Enumerate(vector<string>& choices);
	virtual void		SetDev(int iNewDevice);
	virtual int			GetDev();

	void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE, int iChannels=2);
	void		ReInit();
	_BOOLEAN	Read(vector<_SAMPLE>& data);
	_BOOLEAN	Write(vector<_SAMPLE>& data);
	void		Close();

	PaUtilRingBuffer ringBuffer;
	int xruns;

protected:

	PaSampleFormat fmt(short) { return paInt16; }
	PaSampleFormat fmt(float) { return paFloat32; }

	PaStream *stream;
	vector<string> names;
	vector<PaDeviceIndex> devices;
	int dev;
	bool is_capture,blocking,device_changed,xrun;
	int framesPerBuffer;
	int iBufferSize;
	char *ringBufferData;
	int channels;

	static int pa_count;
};

class CPaIn: public CSoundInInterface
{
public:
						CPaIn();
	virtual 			~CPaIn();
	virtual void		Enumerate(vector<string>& choices) { hw.Enumerate(choices); }
	virtual void		SetDev(int iNewDevice) { hw.SetDev(iNewDevice); }
	virtual int			GetDev() { return hw.GetDev(); }

	virtual void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE, int iChannels=2);
	virtual void		Close();
	virtual _BOOLEAN	Read(vector<_SAMPLE>& data);

protected:

	CPaCommon hw;
};

class CPaOut: public CSoundOutInterface
{
public:
						CPaOut();
	virtual 			~CPaOut();
	virtual void		Enumerate(vector<string>& choices) { hw.Enumerate(choices); }
	virtual void		SetDev(int iNewDevice) { hw.SetDev(iNewDevice); }
	virtual int			GetDev() { return hw.GetDev(); }

	virtual void		Init(int iNewBufferSize, _BOOLEAN bNewBlocking = TRUE, int iChannels=2);
	virtual void		Close();
	virtual _BOOLEAN	Write(vector<_SAMPLE>& data);

protected:

	CPaCommon hw;
};

#endif
