/******************************************************************************\
* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
* Copyright (c) 2001
*
* Author(s):
*	Alexander Kurpiers, Volker Fischer
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

#ifdef WITH_SOUND

#ifdef USE_DEVDSP

#include <linux/soundcard.h>
#include <errno.h>

static int fdSound = 0;

/* Implementation *************************************************************/
/******************************************************************************\
* Wave in																	   *
\******************************************************************************/
void CSound::Read(CVector<short>& psData)
{
	int size;
	int start;
	int ret;
	
	/* Reset start position of reading and set read block size */
	start = 0;
	size = iInBufferSize;

	while (size)
	{
		ret = read(fdSound, &tmprecbuf[start],
			(size > SSIZE_MAX) ? SSIZE_MAX : size);
		
		if (ret < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				printf(".");			
				continue;
			}
			perror("CSound:Read");
			exit(1);
		}
		size -= ret;
		start += ret;
	}
	
	/* Copy data from temporary buffer in output buffer */
	for (int i = 0; i < iInBufferSize; i++)
		psData[i] = tmprecbuf[NO_IN_OUT_CHANNELS * i + RECORDING_CHANNEL];


// TEST
static FILE* pFile = fopen("test/recsamples.dat", "w");
for (int i = 0; i < iInBufferSize; i++)
	fprintf(pFile, "%e ", psData[i]);		
fprintf(pFile, "\n");
fflush(pFile);
}

void CSound::InitIF(int & fdSound)
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
	fdSound = open("/dev/dsp", O_RDWR | O_NONBLOCK);
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


	/* Set sampling parameters ---------------------------------------------- */
	/* Number of buffers and buffer size
	   (If you need to set this parameter, you must set it directly after
	   opening the device. Executing another operation on the opened device can
	   cause the device driver to choose the settings itself after which it will
	   not allow you to change them) */
/*
	arg = 0xnnnnssss;
	status = ioctl(fdSound, SNDCTL_DSP_SETFRAGMENT, &arg);
	if (status == -1)
		perror("SNDCTL_DSP_SETFRAGMENT ioctl failed");
*/

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


	/* Print out capabilities of the sound card */
	printf("\nCapabilities:\n");
	status = ioctl(fdSound, SNDCTL_DSP_GETCAPS, &arg);
	if (status ==  -1)
		perror("SNDCTL_DSP_GETCAPS ioctl failed");
	printf(
		"  revision: %d\n"
		"  full duplex: %s\n"
		"  real-time: %s\n"
		"  batch: %s\n"
		"  coprocessor: %s\n"
		"  trigger: %s\n"
		"  mmap: %s\n",
		arg & DSP_CAP_REVISION,
		(arg & DSP_CAP_DUPLEX) ? "yes" : "no",
		(arg & DSP_CAP_REALTIME) ? "yes" : "no",
		(arg & DSP_CAP_BATCH) ? "yes" : "no",
		(arg & DSP_CAP_COPROC) ? "yes" : "no",
		(arg & DSP_CAP_TRIGGER) ? "yes" : "no",
		(arg & DSP_CAP_MMAP) ? "yes" : "no");
}

void CSound::InitRecording(int iNewBufferSize)
{
	printf("initrec\n");

	/* Save buffer size */
	iInBufferSize = iNewBufferSize;

	/* Allocate memory for temporary record buffer */
	if (tmprecbuf != NULL)
		delete[] tmprecbuf;
	tmprecbuf = new short int[iNewBufferSize * NO_IN_OUT_CHANNELS];
	
	InitIF(fdSound);
}

void CSound::StopRecording()
{
	printf("stoprec\n");

	if (fdSound >0)
		close(fdSound);

	fdSound = 0;
}


/******************************************************************************\
* Wave out																	   *
\******************************************************************************/
void CSound::InitPlayback(int iNewBufferSize)
{
	printf("initplay\n");
	
	/* Save buffer size */
	iBufferSize = iNewBufferSize;
	
	InitIF( fdSound );
}

void CSound::Write(CVector<short>& psData)
{
	int size = iBufferSize;
	int start = 0;
	int ret;
	
	while (size) {
		ret = write(fdSound, &psData[start],(size > SSIZE_MAX) ? SSIZE_MAX : size);
		if (ret < 0) {
			perror("CSound:Write");
			exit(1);
		}
		size -= ret;
		start += ret;
	}
}
#endif

#ifdef HAVE_ARTS

#include <artsc.h>

static arts_stream_t pstream = NULL;
static arts_stream_t rstream = NULL;

void CSound::InitPlayback(int iNewBufferSize)
{
	int arg;      /* argument for ioctl calls */
	int status;   /* return status of system calls */
	
	printf("\ninitplay %d\n", iNewBufferSize);
	
	/* Save buffer size */
	iBufferSize = iNewBufferSize;
	
	if (pstream != NULL)
		return;


	/* init arts */
	
	status = arts_init();
	
	if (status < 0)
	{
		fprintf(stderr, "arts_init error: %s\n", arts_error_text(status));
		return;
	}

	/* set sampling parameters */
	
	pstream = arts_play_stream( SOUNDCRD_SAMPLE_RATE, BITS_PER_SAMPLE, NO_IN_OUT_CHANNELS, "DRM");
	
	/* set to non-blocking */
	status = arts_stream_set( pstream, ARTS_P_BLOCKING, 0);
	if (status != 0)
		fprintf(stderr, "arts_stream_set: ARTS_P_BLOCKING error %s\n", arts_error_text(status));
	
}

void CSound::InitRecording(int iNewBufferSize)
{
	int arg;      /* argument for ioctl calls */
	int status;   /* return status of system calls */
	
	printf("\ninitrec %d\n",iNewBufferSize);
	
	/* Save buffer size */
	iInBufferSize = iNewBufferSize ;
	/* allocate memory for temporary record buffer */
	if ( tmprecbuf != NULL)
		delete[] tmprecbuf;
	tmprecbuf = new short int[ iNewBufferSize * NO_IN_OUT_CHANNELS ];
	
	if (rstream != NULL)
		return;
	
	/* init arts */
	
	status = arts_init();
	
	if (status < 0)
	{
		fprintf(stderr, "arts_init error: %s\n", arts_error_text(status));
		return;
	}
	
	/* set sampling parameters */
	
	rstream = arts_record_stream( SOUNDCRD_SAMPLE_RATE, BITS_PER_SAMPLE, NO_IN_OUT_CHANNELS, "DRM");
	
	/* set to blocking */
	status = arts_stream_set( rstream, ARTS_P_BLOCKING, 1);
	if (status != 1)
		fprintf(stderr, "arts_stream_set (InitRecording): ARTS_P_BLOCKING error %s\n", arts_error_text(status));
	
}

void CSound::Write(CVector<short>& psData)
{
	int size = iBufferSize;
	int start = 0;
	int ret;
	
	while (size) {
		ret = arts_write(pstream, &psData[start], size);
		if (ret < 0) {
			fprintf(stderr, "CSound:Write error %s\n", arts_error_text(ret));
			exit(1);
		}
		size -= ret;
		start += ret;
	}
}

void CSound::Read(CVector<short>& psData)
{
	int size = iInBufferSize;
	int start = 0;
	int ret;
	
	while (size) {
		ret = arts_read(rstream, &tmprecbuf[start], (size>32768) ? 32768 : size);
		if (ret < 0) {
			fprintf(stderr, "CSound:Read error %s\n", arts_error_text(ret));
			exit(1);
		}
		printf("ret: %d\n", ret);
		size -= ret;
		start += ret;
	}
	// we read stereo samples, but actually we want only one channel
	for (int i = 0; i < iInBufferSize; i++)
		psData[i] = tmprecbuf[NO_IN_OUT_CHANNELS * i + RECORDING_CHANNEL];
	
}
void CSound::StopRecording()
{
	printf("stoprec\n");
	if (rstream != NULL)
		arts_close_stream( rstream );
}

#endif
#endif
