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

#include "sound.h"

#include "../../common/GlobalDefinitions.h"
#include "../../common/Buffer.h"
#include "../../common/Vector.h"

#ifdef WITH_SOUND
#include <qthread.h>
#include <string.h>


#define RECORD 0
#define PLAY   1


#define SOUNDBUFLEN 102400

#define FRAGSIZE 1024



/*****************************************************************************/

#ifdef USE_DEVDSP

#include <linux/soundcard.h>
#include <errno.h>

static int fdSound = 0;

void CSound::Init_HW( int mode )
{
	int arg;      /* argument for ioctl calls */
	int status;   /* return status of system calls */
	
	printf("fdsound: %d\n", fdSound);
	if (fdSound >0) 
	{
		printf("already open\n");
		return;	// already open
	}

	/* Open sound device (Use O_RDWR only when writing a program which is
	   going to both record and play back digital audio) */
	fdSound = open("/dev/dsp", O_RDWR );
	if (fdSound < 0) 
    {   
		perror("open of /dev/dsp failed");
		exit(1);
    }
	
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
	arg = NO_IN_OUT_CHANNELS - 1;
	status = ioctl(fdSound, SNDCTL_DSP_STEREO, &arg);
	if (status == -1)
		perror("SNDCTL_DSP_CHANNELS ioctl failed");
	if (arg != (NO_IN_OUT_CHANNELS - 1))
		perror("unable to set number of channels");
	

	/* Sampling rate */
	arg = SOUNDCRD_SAMPLE_RATE;
	status = ioctl(fdSound, SNDCTL_DSP_SPEED, &arg);
	if (status == -1)
		perror("SNDCTL_DSP_SPEED ioctl failed");
	if (arg != SOUNDCRD_SAMPLE_RATE)
		perror("unable to set sample rate");
	

	/* Sample size */
	arg = (BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8;      
	status = ioctl(fdSound, SNDCTL_DSP_SAMPLESIZE, &arg);
	if (status == -1)
		perror("SNDCTL_DSP_SAMPLESIZE ioctl failed");
	if (arg != ((BITS_PER_SAMPLE == 16) ? AFMT_S16_LE : AFMT_U8))
		perror("unable to set sample size");


	/* Check capabilities of the sound card */
	status = ioctl(fdSound, SNDCTL_DSP_GETCAPS, &arg);
	if (status ==  -1)
		perror("SNDCTL_DSP_GETCAPS ioctl failed");
	if ((arg & DSP_CAP_DUPLEX) == 0)
		perror("Soundcard not full duplex capable!");
}


int read_HW( void * recbuf, int size) {
	
	int ret = read(fdSound, recbuf, size * NO_IN_OUT_CHANNELS * BYTES_PER_SAMPLE );
//printf("%d ", ret); fflush(stdout);

	if (ret < 0) {
		if ( (errno != EINTR) && (errno != EAGAIN)) 
		{
			perror("CSound:Read");
			exit(1);
		} else
			return 0;
	} else
		return ret / (NO_IN_OUT_CHANNELS * BYTES_PER_SAMPLE);
}

int write_HW( _SAMPLE *playbuf, int size ){

	int start = 0;
	int ret;

	size *= BYTES_PER_SAMPLE * NO_IN_OUT_CHANNELS;

	while (size) {

		ret = write(fdSound, &playbuf[start], size);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN) 
			{
				continue;
			}
			perror("CSound:Write");
			exit(1);
		}
		size -= ret;
		start += ret / BYTES_PER_SAMPLE;
	}
	return 0;
}
#endif

/*****************************************************************************/

#ifdef USE_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#include <sys/asoundlib.h>

static snd_pcm_t *rhandle = NULL;
static snd_pcm_t *phandle = NULL;


void CSound::Init_HW( int mode ){
 	
	int err, dir;
        snd_pcm_hw_params_t *hwparams;
        snd_pcm_sw_params_t *swparams;
	unsigned int rrate;
	snd_pcm_uframes_t period_size = FRAGSIZE;
	snd_pcm_t *  handle;
	
	
	/* playback/record device directly to the kernel without sample rate conversion 
   	- we do it on our own	*/
	static const char *device = "hw:0,0";	
	
	if (mode == RECORD) {

		if (rhandle != NULL)
			return;
			
		err = snd_pcm_open( &rhandle, device, SND_PCM_STREAM_CAPTURE, 0 );
		if ( err != 0) 
		{
			printf("open error: %s\n", snd_strerror(err));
			return;	
		}
		handle = rhandle;
	} else {
		if (phandle != NULL)
			return;

		err = snd_pcm_open( &phandle, device, SND_PCM_STREAM_PLAYBACK, 0 );
		if ( err != 0) 
		{
			printf("open error: %s\n", snd_strerror(err));
			return;	
		}
		handle = phandle;
	}
	
	
	snd_pcm_hw_params_alloca(&hwparams);
	snd_pcm_sw_params_alloca(&swparams);
	
	
	/* Choose all parameters */
	err = snd_pcm_hw_params_any(handle, hwparams);
	if (err < 0) {
		printf("Broken configuration : no configurations available: %s\n", snd_strerror(err));
		return;
	}
	/* Set the interleaved read/write format */
	err = snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);	

	if (err < 0) {
		printf("Access type not available : %s\n", snd_strerror(err));
		return;
		
	}
	/* Set the sample format */
	err = snd_pcm_hw_params_set_format(handle, hwparams, SND_PCM_FORMAT_S16);
	if (err < 0) {
		printf("Sample format not available : %s\n", snd_strerror(err));
		return;
	}
	/* Set the count of channels */
	err = snd_pcm_hw_params_set_channels(handle, hwparams, NO_IN_OUT_CHANNELS);
	if (err < 0) {
		printf("Channels count (%i) not available s: %s\n", NO_IN_OUT_CHANNELS, snd_strerror(err));
		return;
	}
	/* Set the stream rate */
	rrate = SOUNDCRD_SAMPLE_RATE;
	err = snd_pcm_hw_params_set_rate_near(handle, hwparams, &rrate, &dir);
	if (err < 0) {
		printf("Rate %iHz not available : %s dir %d\n", rrate, snd_strerror(err), dir);
		return;
		
	}
	if (rrate != SOUNDCRD_SAMPLE_RATE) {
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", rrate, err);
		return;
	}
	
	/* Set the period size */
	err = snd_pcm_hw_params_set_period_size_min(handle, hwparams, &period_size, &dir);
	if (err < 0) {
		printf("Unable to set period time %i : %s dir: %d\n", period_size, snd_strerror(err), dir);
		return;
	}
	err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, &dir);
	if (err > 0) {
		printf("Unable to get period size : %s\n", snd_strerror(err));
		return;
	}

	/* Write the parameters to device */
	err = snd_pcm_hw_params(handle, hwparams);
	if (err < 0) {
		printf("Unable to set hw params : %s\n", snd_strerror(err));
		return;
	}
	/* Get the current swparams */
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams : %s\n", snd_strerror(err));
		return;
	}
	/* Start the transfer when the buffer immediately */
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0);
	if (err < 0) {
		printf("Unable to set start threshold mode : %s\n", snd_strerror(err));
		return;
	}
	/* Allow the transfer when at least period_size samples can be processed */
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
	if (err < 0) {
		printf("Unable to set avail min : %s\n", snd_strerror(err));
		return;
	}
	/* Align all transfers to 1 sample */
	err = snd_pcm_sw_params_set_xfer_align(handle, swparams, 1);
	if (err < 0) {
		printf("Unable to set transfer align : %s\n", snd_strerror(err));
		return;
	}
	/* Write the parameters to the record/playback device */
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params : %s\n", snd_strerror(err));
		return;
	}
	snd_pcm_start(handle);
	printf("alsa init done\n");

}

int read_HW( void * recbuf, int size) {

//printf("r"); fflush(stdout);
	int ret = snd_pcm_readi(rhandle, recbuf, size);

//printf("ret: %d", ret); fflush(stdout);

	if (ret < 0) 
	{
		if (ret == -EPIPE) 
		{    
			printf("rpipe\n");
			/* Under-run */
			printf("rprepare\n");
			ret = snd_pcm_prepare(rhandle);

			if (ret < 0)
				printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(ret));

			ret = snd_pcm_start(rhandle);

			if (ret < 0)
				printf("Can't recover from underrun, start failed: %s\n", snd_strerror(ret));
			return 0;

		} 
		else if (ret == -ESTRPIPE) 
		{
			printf("strpipe\n");

			/* Wait until the suspend flag is released */
			while ((ret = snd_pcm_resume(rhandle)) == -EAGAIN)
				sleep(1);       

			if (ret < 0) 
			{
				ret = snd_pcm_prepare(rhandle);

				if (ret < 0)
					printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(ret));
			}
			return 0;
		} 
		else 
		{
			printf("CSound::Read: %s\n", snd_strerror(ret));
						exit(1);
		}
	} else 
		return ret;
			
}

int write_HW( _SAMPLE *playbuf, int size ){

	int start = 0;
	int ret;
//printf("w"); fflush(stdout);
	while (size) {

		ret = snd_pcm_writei(phandle, &playbuf[start], size );
		if (ret < 0) {
			if (ret ==  -EAGAIN) {
				if ((ret = snd_pcm_wait (phandle, 100)) < 0) {
			        	printf ("poll failed (%s)\n", snd_strerror (ret));
			        	break;
				}	           
				continue;
			} else 
			if (ret == -EPIPE) {    /* under-run */
printf("prepare\n");
        			ret = snd_pcm_prepare(phandle);
        			if (ret < 0)
                			printf("Can't recover from underrun, prepare failed: %s\n", snd_strerror(ret));
        			continue;
			} else if (ret == -ESTRPIPE) {
printf("strpipe\n");
        			while ((ret = snd_pcm_resume(phandle)) == -EAGAIN)
                			sleep(1);       /* wait until the suspend flag is released */
        			if (ret < 0) {
                			ret = snd_pcm_prepare(phandle);
                			if (ret < 0)
                        			printf("Can't recover from suspend, prepare failed: %s\n", snd_strerror(ret));
        			}
        			continue;
			} else {
                                printf("Write error: %s\n", snd_strerror(ret));
                               	exit(1);
                        }
                        break;  /* skip one period */
		}
		size -= ret;
		start += ret;
	}
	return 0;
}
#endif



/* ARTS code seems to work, unfortunatly arts recording is broken :-( A sine 
   wave gets completely distorted */
#ifdef USE_ARTS

#include <artsc.h>

static arts_stream_t pstream = NULL;
static arts_stream_t rstream = NULL;

void CSound::Init_HW( int mode)
{
	int arg;      /* argument for ioctl calls */
	int status;   /* return status of system calls */
		
	
	if (mode == RECORD) {
		if (rstream != NULL)
			return;
	} else {
		if (pstream != NULL)
			return;
	}	

	
	/* Init arts */
	status = arts_init();
	
	if (status < 0) 
	{   
		fprintf(stderr, "arts_init error: %s\n", arts_error_text(status));
		return;
	}
	
	/* Set sampling parameters */
	if (mode == RECORD ) {
		rstream = arts_record_stream(SOUNDCRD_SAMPLE_RATE, BITS_PER_SAMPLE, 
			NO_IN_OUT_CHANNELS, "DRM");
		/* Set to blocking */
		status = arts_stream_set( rstream, ARTS_P_BLOCKING, 1);
	} else {
		pstream = arts_play_stream( SOUNDCRD_SAMPLE_RATE, BITS_PER_SAMPLE, 
			NO_IN_OUT_CHANNELS, "DRM");
		/* Set to blocking */
		status = arts_stream_set( pstream, ARTS_P_BLOCKING, 1);
	}
	
	if (status != 1)
		fprintf(stderr, "arts_stream_set: ARTS_P_BLOCKING error %s\n", 
		arts_error_text(status));
			
}

int read_HW( void * recbuf, int size) {

	int ret = arts_read(rstream, recbuf, FRAGSIZE * NO_IN_OUT_CHANNELS * BYTES_PER_SAMPLE );
	if (ret < 0) {
		fprintf(stderr, "CSound:Read error %s\n", arts_error_text(ret));
					exit(1);
	} else
		return ret / (NO_IN_OUT_CHANNELS * BYTES_PER_SAMPLE);
}

int write_HW( _SAMPLE *playbuf, int size ){

	int start = 0;
	int ret;

	size *= BYTES_PER_SAMPLE * NO_IN_OUT_CHANNELS;

	while (size) 
	{
		ret = arts_write(pstream, &playbuf[start], size);
		if (ret < 0) 
		{
			fprintf(stderr, "CSound:Write error %s\n", arts_error_text(ret));
			exit(1);
		}
//printf("w%d\n", ret);
		size -= ret;
		start += ret / BYTES_PER_SAMPLE;
	}
	return 0;
}
#endif


/* ************************************************************************* */

class CSoundBuf : public CCyclicBuffer<_SAMPLE> {

public:
	void lock (void){ data_accessed.lock(); }
	void unlock (void){ data_accessed.unlock(); }
	
protected:
	QMutex	data_accessed;

} SoundBufP, SoundBufR;


class RecThread : public QThread {
public:
	virtual void run() {
	
	
		while (1) {

			int fill;

			SoundBufR.lock();
			fill = SoundBufR.GetFillLevel();
			SoundBufR.unlock();
				
			if (  (SOUNDBUFLEN - fill) > FRAGSIZE ) {
//printf("f %d ", fill); fflush(stdout);
				// enough space in the buffer
				
				int size = read_HW( tmprecbuf, FRAGSIZE);

				// common code
				if (size > 0) {
					CVectorEx<_SAMPLE>*	ptarget;
					
					/* Copy data from temporary buffer in output buffer */
					SoundBufR.lock();
		 			
					ptarget = SoundBufR.QueryWriteBuffer();
					
					for (int i = 0; i < size; i++)
						(*ptarget)[i] = tmprecbuf[NO_IN_OUT_CHANNELS * i + RECORDING_CHANNEL];
		 			
					SoundBufR.Put( size );
					SoundBufR.unlock();
				}
			} else
				usleep( 1000 );
		}
	}

protected:
	_SAMPLE	tmprecbuf[NO_IN_OUT_CHANNELS * FRAGSIZE];
} RecThread1;



/* Wave in ********************************************************************/

void CSound::InitRecording(int iNewBufferSize)
{
	printf("initrec %d\n", iNewBufferSize);

	printf("initrec %d\n", iNewBufferSize);

	/* Save buffer size */
	SoundBufR.lock();
	iInBufferSize = iNewBufferSize;
	SoundBufR.unlock();
	
	Init_HW( RECORD );

	if ( RecThread1.running() == FALSE ) {
		SoundBufR.Init( SOUNDBUFLEN );
		SoundBufR.unlock();
		RecThread1.start();
	}

}

void CSound::StopRecording()
{
	printf("stoprec\n");
#ifdef USE_DSP
	if (fdSound >0)
		close(fdSound);
	fdSound = 0;
#endif
#ifdef USE_ALSA
	if (rhandle != NULL)
		snd_pcm_close( rhandle );

	rhandle = NULL;
#endif
#ifdef HAVE_ARTS
	if (rstream != NULL)
		arts_close_stream( rstream );
if (rstream != NULL) printf("ups\n");
#endif
}


void CSound::Read(CVector< _SAMPLE >& psData)
{
	CVectorEx<_SAMPLE>*	p;

	SoundBufR.lock();	// we need exclusive access
	
//printf("r"); fflush(stdout);
	
	while ( SoundBufR.GetFillLevel() < iInBufferSize ) {
	
		
		// not enough data, sleep a little
		SoundBufR.unlock();
//printf("rw"); fflush(stdout);
		usleep(1000); //1ms
		SoundBufR.lock();
	}
	
	// copy data
	
	p = SoundBufR.Get( iInBufferSize );
	for (int i=0; i<iInBufferSize; i++)
		psData[i] = (*p)[i];
	
	SoundBufR.unlock();
}


/* Wave out *******************************************************************/


class PlayThread : public QThread {
public:
	virtual void run() {
	
	
		while (1) {

			int fill;

			SoundBufP.lock();
			fill = SoundBufP.GetFillLevel();
			SoundBufP.unlock();
				
			if ( fill > (FRAGSIZE * NO_IN_OUT_CHANNELS) ) {

//printf("f%d ", fill); fflush(stdout);		 
				// enough data in the buffer

				CVectorEx<_SAMPLE>*	p;
				
				SoundBufP.lock();
				p = SoundBufP.Get( FRAGSIZE * NO_IN_OUT_CHANNELS );

				for (int i=0; i < FRAGSIZE * NO_IN_OUT_CHANNELS; i++)
					tmpplaybuf[i] = (*p)[i];

				SoundBufP.unlock();
				
				write_HW( tmpplaybuf, FRAGSIZE );

			} else {
			
				do {			
//printf("h %d", fill); fflush(stdout);
					usleep( 1000 );
					
					SoundBufP.lock();
					fill = SoundBufP.GetFillLevel();
					SoundBufP.unlock();

				} while ( fill < SOUNDBUFLEN/2 );	// wait until buffer is at least half full
			}
		}
	}

protected:
	_SAMPLE	tmpplaybuf[NO_IN_OUT_CHANNELS * FRAGSIZE];
} PlayThread1;


void CSound::InitPlayback(int iNewBufferSize)
{
	printf("initplay %d\n", iNewBufferSize);
	
	/* Save buffer size */
	SoundBufP.lock();
	iBufferSize = iNewBufferSize;
	SoundBufP.unlock();

	Init_HW( PLAY );

	if ( PlayThread1.running() == FALSE ) {
		SoundBufP.Init( SOUNDBUFLEN );
		SoundBufP.unlock();
		PlayThread1.start();
	}
}


void CSound::Write(CVector< _SAMPLE >& psData)
{

#if 0
// blocking write
while(1){
	SoundBufP.lock();
	int fill = SOUNDBUFLEN - SoundBufP.GetFillLevel();
	SoundBufP.unlock();
	if ( fill > iBufferSize) break;
}
#endif	
	SoundBufP.lock();	// we need exclusive access

	if ( ( SOUNDBUFLEN - SoundBufP.GetFillLevel() ) > iBufferSize) {
		 
		CVectorEx<_SAMPLE>*	ptarget;
		 
		 // data fits, so copy
//printf("n"); fflush(stdout);		 
		 ptarget = SoundBufP.QueryWriteBuffer();

		 for (int i=0; i < iBufferSize; i++)
		 	(*ptarget)[i] = psData[i];

		 SoundBufP.Put( iBufferSize );
	}
	
	SoundBufP.unlock();
}

#endif
