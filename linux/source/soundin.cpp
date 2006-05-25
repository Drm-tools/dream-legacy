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
#if 0
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif
#include <iostream>
#include <fstream>
#include <sstream>

/*****************************************************************************/

#ifdef USE_DEVDSP

#include <sys/soundcard.h>
#include <errno.h>

CSoundIn::CSoundIn():iCurrentDevice(0),fdSound(0),names()
{
	RecThread.pSoundIn = this;
	ifstream sndstat("/dev/sndstat");
	if(sndstat.is_open()){
		while(!sndstat.eof()){
			char s[80];
			sndstat.getline(s,sizeof(s));
			if(string(s)=="Audio devices:") {
				char sep[20];
				while(true){
					sndstat.getline(s,sizeof(s));
					if(string(s) != "")
						names.push_back(s);
					else
						break;
				}
			}
		}
		sndstat.close();
	}
}

void CSoundIn::SetDev(int iNewDevice)
{
	iCurrentDevice = iNewDevice;
}

int CSoundIn::GetDev()
{
	return iCurrentDevice;
}

void CSoundIn::Init_HW()
{
	int arg;      /* argument for ioctl calls */
	int status;   /* return status of system calls */
	
	if (fdSound >0) 
	{
#ifdef USE_QT_GUI
//		qDebug("already open");
#endif
		return;	// already open
	}

	/* Open sound device (Use O_RDWR only when writing a program which is
	   going to both record and play back digital audio) */
	string dev = "/dev/dsp";
	if(iCurrentDevice>0)
		dev += ('0'+iCurrentDevice);
	fdSound = open(dev.c_str(), O_RDONLY );
	if (fdSound < 0) 
		throw CGenErr("open of "+dev+" failed");
	
	/* Get ready for us.
	   ioctl(audio_fd, SNDCTL_DSP_SYNC, 0) can be used when application wants 
	   to wait until last byte written to the device has been played (it doesn't
	   wait in recording mode). After that the call resets (stops) the device
	   and returns back to the calling program. Note that this call may take
	   several seconds to execute depending on the amount of data in the 
	   buffers. close() calls SNDCTL_DSP_SYNC automaticly */
	ioctl(fdSound, SNDCTL_DSP_SYNC, 0);

	/* Set sampling parameters always so that number of channels (mono/stereo) 
	   is set before selecting sampling rate! */
	/* Set number of channels (0=mono, 1=stereo) */
	arg = NUM_IN_CHANNELS - 1;
	status = ioctl(fdSound, SNDCTL_DSP_STEREO, &arg);
	if (status == -1) 
		throw CGenErr("SNDCTL_DSP_CHANNELS ioctl failed");		

	if (arg != (NUM_IN_CHANNELS - 1))
		throw CGenErr("unable to set number of channels");		
	

	/* Sampling rate */
	arg = SOUNDCRD_SAMPLE_RATE;
	status = ioctl(fdSound, SNDCTL_DSP_SPEED, &arg);
	if (status == -1)
		throw CGenErr("SNDCTL_DSP_SPEED ioctl failed");
	if (arg != SOUNDCRD_SAMPLE_RATE)
		throw CGenErr("unable to set sample rate");
	

	/* Sample size */
	arg = (BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8;      
	status = ioctl(fdSound, SNDCTL_DSP_SAMPLESIZE, &arg);
	if (status == -1)
		throw CGenErr("SNDCTL_DSP_SAMPLESIZE ioctl failed");
	if (arg != ((BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8))
		throw CGenErr("unable to set sample size");
}


int CSoundIn::read_HW( void * recbuf, int size) {
	
	int ret = read(fdSound, recbuf, size * NUM_IN_CHANNELS * BYTES_PER_SAMPLE );

	if (ret < 0) {
		if ( (errno != EINTR) && (errno != EAGAIN))
			throw CGenErr("CSound:Read");
		else
			return 0;
	} else
		return ret / (NUM_IN_CHANNELS * BYTES_PER_SAMPLE);
}

void CSoundIn::close_HW( void ) {

	if (fdSound >0)
		close(fdSound);
	fdSound = 0;

}
#endif

/*****************************************************************************/

#ifdef USE_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

CSoundIn::CSoundIn():iCurrentDevice(0),handle(NULL),devices(),names()
{
	RecThread.pSoundIn = this;
	ifstream sndstat("/proc/asound/pcm");
	if(sndstat.is_open()){
		vector<string> capture_devices;
		while(!sndstat.eof()){
			char s[200];
			sndstat.getline(s,sizeof(s));
			if(strlen(s)==0)
				break;
			if(strstr(s, "capture")!=NULL)
				capture_devices.push_back(s);
		}
		sndstat.close();
		sort(capture_devices.begin(), capture_devices.end());
		for(size_t i=0; i<capture_devices.size(); i++)
		{
			stringstream o(capture_devices[i]);
			char p,n[200],d[200],cap[80];
			int maj,min;
			o >> maj >> p >> min;
			o >> p;
			o.getline(n, sizeof(n), ':');
			o.getline(d, sizeof(d), ':');
			o.getline(cap, sizeof(cap));
			char dd[20];
			stringstream dev;
			dev << "plughw:" << maj << "," << min;
			devices.push_back(dev.str());
			names.push_back(n);
		}
		names.push_back("Default Capture Device");
		devices.push_back("dsnoop");
	}
}

void CSoundIn::SetDev(int iNewDevice)
{
	iCurrentDevice = iNewDevice;
}

int CSoundIn::GetDev()
{
	return iCurrentDevice;
}

void CSoundIn::Init_HW(){

	int err, dir=0;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	unsigned int rrate;
	snd_pcm_uframes_t period_size = FRAGSIZE * NUM_IN_CHANNELS/2;
	snd_pcm_uframes_t buffer_size;
	
	
	/* playback/record device */
	string recdevice = devices[iCurrentDevice];
	
	if (handle != NULL)
		return;
		
	err = snd_pcm_open( &handle, recdevice.c_str(), SND_PCM_STREAM_CAPTURE, 0 );
	if ( err != 0) 
	{
#ifdef USE_QT_GUI
		qDebug("open error: %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW record, can't open "+recdevice+" ("+names[iCurrentDevice]);	
	}
	
	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	
	/* Choose all parameters */
	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Broken configuration : no configurations available: %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");	
	}
	/* Set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);	

	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Access type not available : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");	
		
	}
	/* Set the sample format */
	err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Sample format not available : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");	
	}
	/* Set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_IN_CHANNELS);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Channels count (%i) not available s: %s", NUM_IN_CHANNELS, snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Set the stream rate */
	rrate = SOUNDCRD_SAMPLE_RATE;
	err = snd_pcm_hw_params_set_rate(handle, hwparams, rrate, dir);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Rate %iHz not available : %s dir %d", rrate, snd_strerror(err), dir);
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
		
	}
	unsigned int deltarate;
	if (rrate > SOUNDCRD_SAMPLE_RATE)
		deltarate = rrate - SOUNDCRD_SAMPLE_RATE;
	else
		deltarate = SOUNDCRD_SAMPLE_RATE - rrate;
	if(deltarate>2) {
#ifdef USE_QT_GUI
		qDebug("Rate doesn't match (requested %iHz, get %iHz)", SOUNDCRD_SAMPLE_RATE, rrate);
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	
	dir=0;
	unsigned int buffer_time = 500000;              /* ring buffer length in us */
        /* set the buffer time */
        err = snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, &dir);
        if (err < 0) {
#ifdef USE_QT_GUI
                qDebug("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
        }
        err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size);
        if (err < 0) {
#ifdef USE_QT_GUI
                qDebug("Unable to get buffer size for playback: %s\n", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
        }
#ifdef USE_QT_GUI
	// qDebug("buffer size %d", buffer_size);
#endif
        /* set the period time */
	unsigned int period_time = 100000;              /* period time in us */
        err = snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, &dir);
        if (err < 0) {
#ifdef USE_QT_GUI
                qDebug("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
        }
        err = snd_pcm_hw_params_get_period_size_min(hwparams, &period_size, &dir);
        if (err < 0) {
#ifdef USE_QT_GUI
                qDebug("Unable to get period size for playback: %s\n", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
        }
#ifdef USE_QT_GUI
	// qDebug("period size %d", period_size);
#endif

	/* Write the parameters to device */
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to set hw params : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Get the current swparams */
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to determine current swparams : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Start the transfer when the buffer immediately */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to set start threshold mode : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Allow the transfer when at least period_size samples can be processed */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to set avail min : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Align all transfers to 1 sample */
	err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to set transfer align : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	/* Write the parameters to the record/playback device */
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
#ifdef USE_QT_GUI
		qDebug("Unable to set sw params : %s", snd_strerror(err));
#endif
		throw CGenErr("alsa CSoundIn::Init_HW ");
	}
	snd_pcm_start(handle);
#ifdef USE_QT_GUI
	qDebug("alsa init done");
#endif

}

int CSoundIn::read_HW( void * recbuf, int size) {

	int ret = snd_pcm_readi(handle, recbuf, size);


	if (ret < 0) 
	{
		if (ret == -EPIPE) 
		{    
#ifdef USE_QT_GUI
			qDebug("rpipe");
			/* Under-run */
			qDebug("rprepare");
#endif
			ret = snd_pcm_prepare(handle);

#ifdef USE_QT_GUI
			if (ret < 0)
				qDebug("Can't recover from underrun, prepare failed: %s", snd_strerror(ret));
#endif

			ret = snd_pcm_start(handle);

#ifdef USE_QT_GUI
			if (ret < 0)
				qDebug("Can't recover from underrun, start failed: %s", snd_strerror(ret));
#endif
			return 0;

		} 
		else if (ret == -ESTRPIPE) 
		{
#ifdef USE_QT_GUI
			qDebug("strpipe");
#endif

			/* Wait until the suspend flag is released */
			while ((ret = snd_pcm_resume(handle)) == -EAGAIN)
				sleep(1);

			if (ret < 0) 
			{
				ret = snd_pcm_prepare(handle);

				if (ret < 0)
#ifdef USE_QT_GUI
					qDebug("Can't recover from suspend, prepare failed: %s", snd_strerror(ret));
#endif
					throw CGenErr("CSound:Read");
			}
			return 0;
		} 
		else 
		{
#ifdef USE_QT_GUI
			qDebug("CSoundIn::Read: %s", snd_strerror(ret));
#endif
			throw CGenErr("CSound:Read");
		}
	} else 
		return ret;
			
}

void CSoundIn::close_HW( void ) {

	if (handle != NULL)
		snd_pcm_close( handle );

	handle = NULL;
}

#endif


/* ************************************************************************* */


void
CSoundIn::CRecThread::run()
{
	while (SoundBuf.keep_running) {

		int fill;

		SoundBuf.lock();
		fill = SoundBuf.GetFillLevel();
		SoundBuf.unlock();
			
		if (  (SOUNDBUFLEN - fill) > (FRAGSIZE * NUM_IN_CHANNELS) ) {
			// enough space in the buffer
			
			int size = pSoundIn->read_HW( tmprecbuf, FRAGSIZE);

			// common code
			if (size > 0) {
				CVectorEx<_SAMPLE>*	ptarget;

				/* Copy data from temporary buffer in output buffer */
				SoundBuf.lock();

				ptarget = SoundBuf.QueryWriteBuffer();

				for (int i = 0; i < size * NUM_IN_CHANNELS; i++)
					(*ptarget)[i] = tmprecbuf[i];

				SoundBuf.Put( size * NUM_IN_CHANNELS );
				SoundBuf.unlock();
			}
		} else {
			msleep( 1 );
		}
	}
#ifdef USE_QT_GUI
	qDebug("Rec Thread stopped");
#endif
}


/* Wave in ********************************************************************/

void CSoundIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
#ifdef USE_QT_GUI
	qDebug("initrec %d", iNewBufferSize);
#endif

	/* Save < */
	RecThread.SoundBuf.lock();
	iInBufferSize = iNewBufferSize;
	bBlockingRec = bNewBlocking;
	RecThread.SoundBuf.unlock();
	
	Init_HW( );

	if ( RecThread.running() == FALSE ) {
		RecThread.SoundBuf.Init( SOUNDBUFLEN );
		RecThread.SoundBuf.unlock();
		RecThread.start();
	}

}


_BOOLEAN CSoundIn::Read(CVector< _SAMPLE >& psData)
{
	CVectorEx<_SAMPLE>*	p;

	RecThread.SoundBuf.lock();	// we need exclusive access
	
	
	while ( RecThread.SoundBuf.GetFillLevel() < iInBufferSize ) {
	
		
		// not enough data, sleep a little
		RecThread.SoundBuf.unlock();
		usleep(1000); //1ms
		RecThread.SoundBuf.lock();
	}
	
	// copy data
	
	p = RecThread.SoundBuf.Get( iInBufferSize );
	for (int i=0; i<iInBufferSize; i++)
		psData[i] = (*p)[i];
	
	RecThread.SoundBuf.unlock();

	return FALSE;
}

void CSoundIn::Close()
{
#ifdef USE_QT_GUI
	qDebug("stoprec");
#endif

	// stop the recording threads
	
	if (RecThread.running() ) {
		RecThread.SoundBuf.keep_running = FALSE;
		// wait 1sec max. for the threads to terminate
		RecThread.wait(1000);
	}
	
	close_HW();
}


#endif
