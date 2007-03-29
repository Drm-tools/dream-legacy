/******************************************************************************\
* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
* Copyright (c) 2001
*
* Author(s):
*	Alexander Kurpiers
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

#include "soundin.h"

#ifdef WITH_SOUND
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <iostream>
#include <fstream>
#include <sstream>

/*****************************************************************************/

# ifdef USE_DEVDSP

#include <sys/soundcard.h>
#include <errno.h>

map < string, COSSDev::devdata > COSSDev::dev;

void
COSSDev::devdata::open(const string & name, int mode)
{
#if 1
	if (fd == 0)
	{
		fd =::open(name.c_str(), O_RDWR);
		int
			arg,
			status;
		/* Get ready for us.
		   ioctl(audio_fd, SNDCTL_DSP_SYNC, 0) can be used when application wants 
		   to wait until last byte written to the device has been played (it doesn't
		   wait in recording mode). After that the call resets (stops) the device
		   and returns back to the calling program. Note that this call may take
		   several seconds to execute depending on the amount of data in the 
		   buffers. close() calls SNDCTL_DSP_SYNC automaticly */
		ioctl(fd, SNDCTL_DSP_SYNC, 0);

		/* Set sampling parameters always so that number of channels (mono/stereo) 
		   is set before selecting sampling rate! */
		/* Set number of channels (0=mono, 1=stereo) */
		arg = NUM_OUT_CHANNELS - 1;
		status = ioctl(fd, SNDCTL_DSP_STEREO, &arg);
		if (status == -1)
			throw
			CGenErr(string("SNDCTL_DSP_CHANNELS ioctl failed: ") +
					strerror(errno));

		if (arg != (NUM_OUT_CHANNELS - 1))
			throw
			CGenErr("unable to set number of channels");

		/* Sampling rate */
		arg = SOUNDCRD_SAMPLE_RATE;
		status = ioctl(fd, SNDCTL_DSP_SPEED, &arg);
		if (status == -1)
			throw
			CGenErr("SNDCTL_DSP_SPEED ioctl failed");
		if (arg != SOUNDCRD_SAMPLE_RATE)
			throw
			CGenErr("unable to set sample rate");

		/* Sample size */
		arg = (BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8;
		status = ioctl(fd, SNDCTL_DSP_SAMPLESIZE, &arg);
		if (status == -1)
			throw
			CGenErr("SNDCTL_DSP_SAMPLESIZE ioctl failed");
		if (arg != ((BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8))
			throw
			CGenErr("unable to set sample size");
#if 1
		/* Check capabilities of the sound card */
		status = ioctl(fd, SNDCTL_DSP_GETCAPS, &arg);
		if (status == -1)
			throw
			CGenErr("SNDCTL_DSP_GETCAPS ioctl failed");
		if ((arg & DSP_CAP_DUPLEX) == 0)
			throw
			CGenErr("Soundcard not full duplex capable!");
#endif
	}
#else
	if (fd == 0)
	{
		fd =::open(name.c_str(), mode);
	}
	else
	{
		::close(fd);
		/* this might work if there was a way to wait for the card to
		   be ready to open again */
		fd =::open(name.c_str(), O_RDWR);
	}
#endif
	if (fd > 0)
		count++;
	else
	{
		fd = 0;
	}
}

void
COSSDev::devdata::close()
{
	if (fd && ((--count) == 0))
	{
		::close(fd);
		fd = 0;
	}
}

int
COSSDev::devdata::fildes()
{
	return fd;
}

void
COSSDev::open(const string & devname, int mode)
{
	name = devname;
	dev[name].open(name, mode);
}

void
COSSDev::close()
{
	dev[name].close();
}

void
getdevices(vector < string > &names, vector < string > &devices,
		   bool playback)
{
	vector < string > tmp;
	names.clear();
	devices.clear();
	ifstream sndstat("/dev/sndstat");
	if (!sndstat.is_open())
	{
		sndstat.close();
		sndstat.clear();
		sndstat.open("/proc/asound/oss/sndstat");
	}

	if (sndstat.is_open())
	{
		while (!(sndstat.eof() || sndstat.fail()))
		{
			char s[80];
			sndstat.getline(s, sizeof(s));

			if (strstr(s, "Audio devices:") != NULL)
			{
				while (true)
				{
					sndstat.getline(s, sizeof(s));
					if (strlen(s) > 0)
						tmp.push_back(s);
					else
						break;
				}
			}
		}
	}
	sndstat.close();

	/* if there is more than one device, let the user chose */
	if (tmp.size() > 0)
	{
		names.resize(tmp.size());
		devices.resize(tmp.size());
		size_t i;
		for (i = 0; i < tmp.size(); i++)
		{
			stringstream o(tmp[i]);
			char p, name[200];
			int n;
			o >> n >> p;
			o.getline(name, sizeof(name), ':');
			names[n] = name;
		}
		devices[0] = "/dev/dsp";
		for (i = 1; i < names.size(); i++)
		{
			devices[i] = "/dev/dsp";
			devices[i] += '0' + i;
		}
	}
	else
	{
		if (playback)
			names.push_back("Default Playback Device");
		else
			names.push_back("Default Capture Device");
		devices.push_back("/dev/dsp");
	}
}

# endif

/*****************************************************************************/

# ifdef USE_ALSA

void
getdevices(vector < string > &names, vector < string > &devices,
		   bool playback)
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
			if (strstr(s, playback ? "playback" : "capture") != NULL)
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
	if (playback)
	{
		names.push_back("Default Playback Device");
		devices.push_back("dmix");
	}
	else
	{
		names.push_back("Default Capture Device");
		devices.push_back("dsnoop");
	}
}

# endif

#endif
