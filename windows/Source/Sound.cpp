/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Soundcard interface for Windows operation systems
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

#include "Sound.h"


/* Implementation *************************************************************/
CSound::CSound()
{
	int i;

	m_WaveInEvent = NULL;

	m_WaveIn = NULL;
	m_WaveOut = NULL;

	/* Init buffer pointer to zero */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
		psSoundcardBuffer[i] = NULL;

	for (i = 0; i < NO_SOUND_BUFFERS_OUT; i++)
		psPlaybackBuffer[i] = NULL;

	/* Init wave-format structure */
	sWaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormatEx.nChannels = NO_IN_OUT_CHANNELS;
	sWaveFormatEx.wBitsPerSample = BITS_PER_SAMPLE;
	sWaveFormatEx.nSamplesPerSec = SOUNDCRD_SAMPLE_RATE;
	sWaveFormatEx.nBlockAlign = sWaveFormatEx.nChannels *
		sWaveFormatEx.wBitsPerSample / 8;
	sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign *
		sWaveFormatEx.nSamplesPerSec;
	sWaveFormatEx.cbSize = 0;
}

CSound::~CSound()
{
	int i;

	/* Delete allocated memory */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];
	}
}


/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/
void CSound::Read(CVector<short>& psData)
{
	int i;

	/* Wait, until data is available */
	if (!(m_WaveInHeader[iWhichBufferIn].dwFlags & WHDR_DONE))
		 WaitForSingleObject(m_WaveInEvent, INFINITE);

	/* Copy data from sound card in output buffer */
	for (i = 0; i < iBufferSizeIn; i++)
		psData[i] = psSoundcardBuffer[iWhichBufferIn]
			[NO_IN_OUT_CHANNELS * i + RECORDING_CHANNEL];

	/* Add the buffer so that it can be filled with new samples */
	AddInBuffer();

	/* In case more than one buffer was ready, reset event */
	ResetEvent(m_WaveInEvent);
}

void CSound::AddInBuffer()
{
	/* Send buffer to driver for filling with new data */
	waveInAddBuffer(m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

	/* Toggle buffers */
	iWhichBufferIn++;
	if (iWhichBufferIn == NO_SOUND_BUFFERS_IN)
		iWhichBufferIn = 0;
}

void CSound::InitRecording(int iNewBufferSize)
{
	int			i;
	MMRESULT	result;
	
	/* Set internal parameter */
	iBufferSizeIn = iNewBufferSize;

	/* Numerate the sound-devices. At least one must exist in the system */
	result = waveInGetNumDevs();
	if (result == 0)
		throw 0;
	
	/* Test for Mic available */
	result = waveInGetDevCaps (WAVE_MAPPER, &m_WaveInDevCaps,
		sizeof(WAVEINCAPS));
	if (result!= MMSYSERR_NOERROR)
		throw 0;
	
	/* We use an event controlled wave-in structure */
	/* Create an event */
	if (m_WaveInEvent != NULL)
		CloseHandle(m_WaveInEvent);
	m_WaveInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Open wave-input and set call-back mechanism to event handle */
	if (m_WaveIn != NULL)
	{
		waveInReset(m_WaveIn);
		waveInClose(m_WaveIn);
	}
	result = waveInOpen(&m_WaveIn, WAVE_MAPPER, &sWaveFormatEx,
		(DWORD) m_WaveInEvent, NULL, CALLBACK_EVENT);
	if (result != MMSYSERR_NOERROR)
		throw 0;
	
	/* Create memory for sound card buffer */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];
		psSoundcardBuffer[i] = new short[iBufferSizeIn * NO_IN_OUT_CHANNELS];
	}

	/* Set header data (prepare wave-header) */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		m_WaveInHeader[i].lpData = (LPSTR) &psSoundcardBuffer[i][0];
		m_WaveInHeader[i].dwBufferLength =
			iBufferSizeIn * BYTES_PER_SAMPLE * NO_IN_OUT_CHANNELS;
		m_WaveInHeader[i].dwFlags = 0;
	
		/* Call windows internal function to prepare wave-header */
		result = waveInPrepareHeader(m_WaveIn, &m_WaveInHeader[i],
			sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR)
			throw 0;
	}

	/* Define which buffer is the first to use */
	iWhichBufferIn = 0;
	
	/* Send all buffers to driver for filling the queue */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
		AddInBuffer();

	/* Notify that sound-capturing can start now */
	waveInStart(m_WaveIn);

	/* Reset event for initialization */
	ResetEvent(m_WaveInEvent);
}

void CSound::StopRecording()
{
	MMRESULT result;

	/* Reset audio driver */
	result = waveInReset(m_WaveIn);
	if (result != MMSYSERR_NOERROR)
		throw 0;

	/* Set event to ensure that thread leaves the waiting function */
	if (m_WaveInEvent)
		SetEvent(m_WaveInEvent);

	/* Wait for the thread to terminate */
	Sleep(500);

	/* Unprepare wave-headers */
	result =
		waveInUnprepareHeader(m_WaveIn, &m_WaveInHeader[0], sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
		throw 0;

	result =
		waveInUnprepareHeader(m_WaveIn, &m_WaveInHeader[1], sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
		throw 0;

	/* Close the sound-device */
	result = waveInClose(m_WaveIn);
	if (result != MMSYSERR_NOERROR)
		throw 0;

	/* Close the handle for the event */
	CloseHandle(m_WaveInEvent);
}


/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
void CSound::InitPlayback(int iNewBufferSize)
{
	int			i, j;
	MMRESULT	result;
	
	/* Set internal parameter */
	iBufferSizeOut = iNewBufferSize;

	/* Numerate the sound-devices. At least one must exist in the system */
	result = waveInGetNumDevs();
	if (result == 0)
		throw 0;
	
	/* Open wave-input and set call-back mechanism to event handle */
	if (m_WaveOut != NULL)
	{
		waveOutReset(m_WaveOut);
		waveOutClose(m_WaveOut);
	}
	result = waveOutOpen(&m_WaveOut, WAVE_MAPPER, &sWaveFormatEx, 0, 0,
		CALLBACK_NULL);
	if (result != MMSYSERR_NOERROR)
		throw 0;
	
	for (j = 0; j < NO_SOUND_BUFFERS_OUT; j++)
	{
		/* Create memory for playback buffer */
		if (psPlaybackBuffer[j] != NULL)
			delete[] psPlaybackBuffer[j];
		psPlaybackBuffer[j] = new short[iBufferSizeOut];

		/* Clear new buffer */
		for (i = 0; i < iBufferSizeOut; i++)
			psPlaybackBuffer[j][i] = 0;

		/* Set Header data */
		m_WaveOutHeader[j].lpData = (LPSTR) &psPlaybackBuffer[j][0];
		m_WaveOutHeader[j].dwBufferLength = iBufferSizeOut * BYTES_PER_SAMPLE;
		m_WaveOutHeader[j].dwFlags = 0;

		/* Prepare wave-header */
		result = waveOutPrepareHeader(m_WaveOut, &m_WaveOutHeader[j],
			sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR)
			throw 0;
		
		/* Initially send all buffers to the interface */
		waveOutWrite(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));
	}
}

void CSound::Write(CVector<short>& psData)
{
	int		i, j;
	int		iCntPrepBuf;
	int		iIndexDoneBuf;

	/* Get number of "done"-buffers and position of one of them */
	iCntPrepBuf = 0;
	for (i = 0; i < NO_SOUND_BUFFERS_OUT; i++)
	{
		if (m_WaveOutHeader[i].dwFlags & WHDR_DONE)
		{
			iCntPrepBuf++;
			iIndexDoneBuf = i;
		}
	}

	/* Now check special cases (Buffer is full or empty) */
	if (iCntPrepBuf == 0)
	{
		/* All buffers are filled, dump new block --------------------------- */
// It would be better to kill half of the buffer blocks to set the start
// back to the middle: TODO
		return;
	}
	else if (iCntPrepBuf == NO_SOUND_BUFFERS_OUT)
	{
		/* ---------------------------------------------------------------------
		   Buffer is empty -> send as many cleared blocks to the sound-
		   interface until half of the buffer size is reached */
		/* Send half of the buffer size blocks to the sound-interface */
		for (j = 0; j < NO_SOUND_BUFFERS_OUT / 2; j++)
		{
			/* First, clear these buffers */
			for (i = 0; i < iBufferSizeOut; i++)
				psPlaybackBuffer[j][i] = 0;

			/* Then send them to the interface */
			waveOutWrite(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));
		}

		/* Set index for done buffer */
		iIndexDoneBuf = NO_SOUND_BUFFERS_OUT / 2;
	}

	/* Copy stereo data from input in soundcard buffer */
	for (i = 0; i < iBufferSizeOut; i++)
		psPlaybackBuffer[iIndexDoneBuf][i] = psData[i];

	/* Now, send the current block */
	waveOutWrite(m_WaveOut, &m_WaveOutHeader[iIndexDoneBuf],
		sizeof(WAVEHDR));
}
