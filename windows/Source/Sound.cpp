/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Sound card interface for Windows operating systems
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
/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/
void CSound::Read(CVector<short>& psData)
{
	/* Check if device must be opened or reinitialized */
	if (bChangDevIn == TRUE)
	{
		OpenInDevice();

		/* Reinit sound interface */
		InitRecording(iBufferSizeIn);

		/* Reset flag */
		bChangDevIn = FALSE;
	}

	/* Wait until data is available */
	if (!(m_WaveInHeader[iWhichBufferIn].dwFlags & WHDR_DONE))
		 WaitForSingleObject(m_WaveInEvent, INFINITE);

	/* Copy data from sound card in output buffer */
	for (int i = 0; i < iBufferSizeIn; i++)
		psData[i] = psSoundcardBuffer[iWhichBufferIn]
			[NO_IN_OUT_CHANNELS * i + RECORDING_CHANNEL];

	/* Add the buffer so that it can be filled with new samples */
	AddInBuffer();

	/* In case more than one buffer was ready, reset event */
	ResetEvent(m_WaveInEvent);
}

void CSound::AddInBuffer()
{
	/* Unprepare wave-header */
	waveInUnprepareHeader(
		m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

	/* Reset struct entries */
	m_WaveInHeader[iWhichBufferIn].lpData = 
		(LPSTR) &psSoundcardBuffer[iWhichBufferIn][0];
	m_WaveInHeader[iWhichBufferIn].dwBufferLength =
		iBufferSizeIn * BYTES_PER_SAMPLE * NO_IN_OUT_CHANNELS;
	m_WaveInHeader[iWhichBufferIn].dwFlags = 0;
		
	/* Prepare wave-header */
	waveInPrepareHeader(
		m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

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
	
	/* Check if device must be opened or reinitialized */
	if (bChangDevIn == TRUE)
	{
		OpenInDevice();

		/* Reset flag */
		bChangDevIn = FALSE;
	}

	/* Set internal parameter */
	iBufferSizeIn = iNewBufferSize;

	/* Reset interface so that all buffers are returned from the interface */
	waveInReset(m_WaveIn);
	waveInStop(m_WaveIn);
	
	/* Create memory for sound card buffer */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];

		psSoundcardBuffer[i] = new short[iBufferSizeIn * NO_IN_OUT_CHANNELS];
	}

	/* Reset current buffer ID */
	iWhichBufferIn = 0;
	
	/* Send all buffers to driver for filling the queue */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
		AddInBuffer();
		
	/* Notify that sound capturing can start now */
	waveInStart(m_WaveIn);

	/* This reset event is very important for initialization, otherwise we will
	   get errors! */
	ResetEvent(m_WaveInEvent);
}

void CSound::OpenInDevice()
{
	/* Open wave-input and set call-back mechanism to event handle */
	if (m_WaveIn != NULL)
	{
		waveInReset(m_WaveIn);
		waveInClose(m_WaveIn);
	}

	MMRESULT result = waveInOpen(&m_WaveIn, iCurInDev, &sWaveFormatEx,
		(DWORD) m_WaveInEvent, NULL, CALLBACK_EVENT);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface Start, waveInOpen() failed. This error \
			usually occurs if another application uses the sound in.");
}

void CSound::SetInDev(int iNewDev)
{
	/* Set device to wave mapper if iNewDev is greater that the number of
	   sound devices in the system */
	if (iNewDev >= iNumDevs)
		iNewDev = WAVE_MAPPER;

	/* Change only in case new device id is not already active */
	if (iNewDev != iCurInDev)
	{
		iCurInDev = iNewDev;
		bChangDevIn = TRUE;
	}
}


/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
void CSound::Write(CVector<short>& psData)
{
	int		i, j;
	int		iCntPrepBuf;
	int		iIndexDoneBuf;

	/* Check if device must be opened or reinitialized */
	if (bChangDevOut == TRUE)
	{
		OpenOutDevice();

		/* Reinit sound interface */
		InitPlayback(iBufferSizeOut);

		/* Reset flag */
		bChangDevOut = FALSE;
	}

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

void CSound::InitPlayback(int iNewBufferSize)
{
	int			i, j;
	MMRESULT	result;
	
	/* Check if device must be opened or reinitialized */
	if (bChangDevOut == TRUE)
	{
		OpenOutDevice();

		/* Reset flag */
		bChangDevOut = FALSE;
	}

	/* Set internal parameter */
	iBufferSizeOut = iNewBufferSize;

	/* Reset interface */
	waveOutReset(m_WaveOut);

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
			throw CGenErr("Sound Interface Init Playback, \
				waveOutPrepareHeader() failed.");
		
		/* Initially, send all buffers to the interface */
		waveOutWrite(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));
	}
}

void CSound::OpenOutDevice()
{
	if (m_WaveOut != NULL)
	{
		waveOutReset(m_WaveOut);
		waveOutClose(m_WaveOut);
	}

	MMRESULT result = waveOutOpen(&m_WaveOut, iCurOutDev, &sWaveFormatEx, 0, 0,
		CALLBACK_NULL);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface Start, waveOutOpen() failed.");
}

void CSound::SetOutDev(int iNewDev)
{
	/* Set device to wave mapper if iNewDev is greater that the number of
	   sound devices in the system */
	if (iNewDev >= iNumDevs)
		iNewDev = WAVE_MAPPER;

	/* Change only in case new device id is not already active */
	if (iNewDev != iCurOutDev)
	{
		iCurOutDev = iNewDev;
		bChangDevOut = TRUE;
	}
}


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
void CSound::Close()
{
	int			i;
	MMRESULT	result;

	/* Reset audio driver */
	result = waveOutReset(m_WaveOut);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface, waveOutReset() failed.");

	result = waveInReset(m_WaveIn);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface, waveInReset() failed.");

	/* Set event to ensure that thread leaves the waiting function */
	if (m_WaveInEvent)
		SetEvent(m_WaveInEvent);

	/* Wait for the thread to terminate */
	Sleep(500);

	/* Unprepare wave-headers */
	for (i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		result = waveInUnprepareHeader(
			m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveInUnprepareHeader() failed.");
	}

	for (i = 0; i < NO_SOUND_BUFFERS_OUT; i++)
	{
		result = waveOutUnprepareHeader(
			m_WaveOut, &m_WaveOutHeader[i], sizeof(WAVEHDR));

		if (result != MMSYSERR_NOERROR)
			throw CGenErr("Sound Interface, waveOutUnprepareHeader() failed.");
	}

	/* Close the sound device */
	result = waveOutClose(m_WaveOut);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface, waveOutClose() failed.");

	result = waveInClose(m_WaveIn);
	if (result != MMSYSERR_NOERROR)
		throw CGenErr("Sound Interface, waveInClose() failed.");

	/* Close the handle for the event */
	CloseHandle(m_WaveInEvent);
}

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

	/* Get the number of digital audio devices in this computer, check range */
	iNumDevs = waveInGetNumDevs();

	if (iNumDevs > MAX_NUMBER_SOUND_CARDS)
		iNumDevs = MAX_NUMBER_SOUND_CARDS;

	/* At least one device must exist in the system */
	if (iNumDevs == 0)
		throw CGenErr("No audio device found.");

	/* Get info about the devices and store the names */
	for (i = 0; i < iNumDevs; i++)
		if (!waveInGetDevCaps(i, &m_WaveInDevCaps, sizeof(WAVEINCAPS)))
			pstrDevices[i] = m_WaveInDevCaps.szPname;

	/* We use an event controlled wave-in structure */
	/* Create an event */
	m_WaveInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	/* Set flag to open devices */
	bChangDevIn = TRUE;
	bChangDevOut = TRUE;

	/* Default device number (first device in system) */
	iCurInDev = 0;
	iCurOutDev = 0;
}

CSound::~CSound()
{
	/* Delete allocated memory */
	for (int i = 0; i < NO_SOUND_BUFFERS_IN; i++)
	{
		if (psSoundcardBuffer[i] != NULL)
			delete[] psSoundcardBuffer[i];
	}
}
