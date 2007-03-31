/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 * 
 * Decription:
 * ALSA sound interface
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

#include "alsa.h"
#include <iostream>
#include <sstream>
#include <fstream>

CAlsaSoundIn::CAlsaSoundIn():handle(NULL),names(),devices(),dev(-1)
{
	vector<string> choices;
	Enumerate(choices);
}

CAlsaSoundIn::~CAlsaSoundIn()
{
	if (handle)
		snd_pcm_close(handle);
}

void
CAlsaSoundIn::Enumerate(vector<string>& choices)
{
	vector < string > tmp;
	names.clear();
	devices.clear();
	ifstream sndstat("/proc/asound/pcm");
	if (sndstat.is_open())
	{
		while (!(sndstat.eof() || sndstat.fail()))
		{
			char s[200];
			sndstat.getline(s, sizeof(s));
			if (strlen(s) == 0)
				break;
			if (strstr(s, "capture") != NULL)
				tmp.push_back(s);
		}
		sndstat.close();
	}
	if (tmp.size() > 0)
	{
		sort(tmp.begin(), tmp.end());
		for (size_t i = 0; i < tmp.size(); i++)
		{
			stringstream o(tmp[i]);
			char p, n[200], d[200], cap[80];
			int maj, min;
			o >> maj >> p >> min;
			o >> p;
			o.getline(n, sizeof(n), ':');
			o.getline(d, sizeof(d), ':');
			o.getline(cap, sizeof(cap));
			stringstream dev;
			dev << "plughw:" << maj << "," << min;
			devices.push_back(dev.str());
			names.push_back(n);
		}
	}
	names.push_back("Default Capture Device");
	devices.push_back("dsnoop");
	choices = names;
}

void
CAlsaSoundIn::SetDev(int iNewDevice)
{
	if(dev != iNewDevice)
		Close();
	dev = iNewDevice;
}

int
CAlsaSoundIn::GetDev()
{
	return dev;
}

void
CAlsaSoundIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	int err;

	if(dev<0)
		throw CGenErr("Capture open error: device not selected");

	if(dev>=int(devices.size()))
	{
		vector<string> choices;
		Enumerate(choices);
	}

	if(dev>=int(devices.size()))
	{
		ostringstream s;
		s << "Capture open error: device " << dev << " out of range";
		throw CGenErr(s.str());
	}

	if ((err = snd_pcm_open(&handle, devices[dev].c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		throw CGenErr(string("Capture open error: ") + snd_strerror(err));
	}
	if ((err =
		 snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE,
							SND_PCM_ACCESS_RW_INTERLEAVED, 2, 48000, 0,
							400000)) < 0)
	{							/* 0.4sec */
		throw CGenErr(string("Capture open error: ") + snd_strerror(err));
	}
}

_BOOLEAN
CAlsaSoundIn::Read(CVector < short >&psData)
{
	int err = snd_pcm_readi(handle, &psData[0], psData.Size()/2);
	if (err < 0)
	{
		if (err == -EPIPE)
		{
			err = snd_pcm_prepare(handle);
			err = snd_pcm_start(handle);
			return TRUE;
		}
		else if (err == -ESTRPIPE)
		{
			/* Wait until the suspend flag is released */
			while ((err = snd_pcm_resume(handle)) == -EAGAIN)
				sleep(1);
			if (err < 0)
			{
				err = snd_pcm_prepare(handle);
				if(err < 0)
					throw CGenErr(string("Capture read error: ") + snd_strerror(err));
			}
			return TRUE;
		}
		else
			throw CGenErr(string("Capture read error: ") + snd_strerror(err));
	}
	else
		return FALSE;
}

void
CAlsaSoundIn::Close()
{
	if(handle)
	{
		snd_pcm_close(handle);
		handle = NULL;
	}
}

CAlsaSoundOut::CAlsaSoundOut():handle(NULL),names(),devices(),dev(-1)
{
	vector<string> choices;
	Enumerate(choices);
}

CAlsaSoundOut::~CAlsaSoundOut()
{
	if (handle)
		snd_pcm_close(handle);
}

void
CAlsaSoundOut::Enumerate(vector<string>& choices)
{
	vector < string > tmp;
	names.clear();
	devices.clear();
	ifstream sndstat("/proc/asound/pcm");
	if (sndstat.is_open())
	{
		while (!(sndstat.eof() || sndstat.fail()))
		{
			char s[200];
			sndstat.getline(s, sizeof(s));
			if (strlen(s) == 0)
				break;
			if (strstr(s, "playback") != NULL)
				tmp.push_back(s);
		}
		sndstat.close();
	}
	if (tmp.size() > 0)
	{
		sort(tmp.begin(), tmp.end());
		for (size_t i = 0; i < tmp.size(); i++)
		{
			stringstream o(tmp[i]);
			char p, n[200], d[200], cap[80];
			int maj, min;
			o >> maj >> p >> min;
			o >> p;
			o.getline(n, sizeof(n), ':');
			o.getline(d, sizeof(d), ':');
			o.getline(cap, sizeof(cap));
			stringstream dev;
			dev << "plughw:" << maj << "," << min;
			devices.push_back(dev.str());
			names.push_back(n);
		}
	}
	names.push_back("Default Playback Device");
	devices.push_back("dmix");
	choices = names;
}

void
CAlsaSoundOut::SetDev(int iNewDevice)
{
	if(dev != iNewDevice)
		Close();
	dev = iNewDevice;
}

int
CAlsaSoundOut::GetDev()
{
	return dev;
}
void
CAlsaSoundOut::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	int err;

	if(handle)
		Close();

	if(dev<0)
		throw CGenErr("Playback open error: device not selected");

	if(dev>=int(devices.size()))
	{
		vector<string> choices;
		Enumerate(choices);
	}

	if(dev>=int(devices.size()))
	if(dev>=int(devices.size()))
	{
		ostringstream s;
		s << "Playback open error: device " << dev << " out of range";
		throw CGenErr(s.str());
	}

	if ((err = snd_pcm_open(&handle, devices[dev].c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0)
	{
		throw CGenErr(string("Playback open error: ") + snd_strerror(err));
	}
	if ((err =
		 snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE,
							SND_PCM_ACCESS_RW_INTERLEAVED, 2, 48000, 1,
							400000)) < 0)
	{							/* 0.4sec */
		throw CGenErr(string("Playback open error: ") + snd_strerror(err));
	}
}

_BOOLEAN
CAlsaSoundOut::Write(CVector < short >&psData)
{
	if(handle==NULL)
		throw CGenErr("Playback write error: device not open");

	snd_pcm_sframes_t frames, nframes;
	nframes = psData.Size()/2;
	frames = snd_pcm_writei(handle, &psData[0], nframes);
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0)
		throw CGenErr(string("Playback write error: ") + snd_strerror(frames));
	if (frames > 0 && frames < nframes)
		printf("Short write (expected %li, wrote %li)\n", nframes, frames);
	return TRUE;
}

void
CAlsaSoundOut::Close()
{
	if(handle)
	{
		snd_pcm_close(handle);
		handle = NULL;
	}
}
