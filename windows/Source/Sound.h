/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#if !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include <windows.h>
#include <mmsystem.h>

#include "../../common/GlobalDefinitions.h"
#include "../../common/Vector.h"


/* Definitions ****************************************************************/
#define	NO_IN_OUT_CHANNELS		2		/* Stereo recording (but we only
										   use one channel for recording) */
#define	BITS_PER_SAMPLE			16		/* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE		2		/* Number of bytes per sample */

/* Set this number as high as we have to prebuffer symbols for one MSC block.
   In case of robustness mode D we have 24 symbols */
#define NO_SOUND_BUFFERS_IN		30		/* Number of sound card buffers */

#define NO_SOUND_BUFFERS_OUT	4		/* Number of sound card buffers */

#define RECORDING_CHANNEL		0		/* 0: Left, 1: Right
  
/* Maximum number of recognized sound cards installed in the system */
#define MAX_NUMBER_SOUND_CARDS	10


/* Classes ********************************************************************/
class CSound
{
public:
	CSound();
	virtual ~CSound();

	void	InitRecording(int iNewBufferSize);
	void	InitPlayback(int iNewBufferSize);
	void	Read(CVector<short>& psData);
	void	Write(CVector<short>& psData);

	int		GetNumDev() {return iNumDevs;}
	string	GetDeviceName(int iDiD) {return pstrDevices[iDiD];}
	void	SetOutDev(int iNewDev);
	void	SetInDev(int iNewDev);

	void	Close();

protected:
	void	OpenInDevice();
	void	OpenOutDevice();
	void	PrepareInBuffer(int iBufNum);
	void	PrepareOutBuffer(int iBufNum);
	void	AddInBuffer();
	void	AddOutBuffer(int iBufNum);

	WAVEFORMATEX	sWaveFormatEx;
	UINT			iNumDevs;
	string			pstrDevices[MAX_NUMBER_SOUND_CARDS];
	UINT			iCurInDev;
	UINT			iCurOutDev;
	BOOLEAN			bChangDevIn;
	BOOLEAN			bChangDevOut;

	/* Wave in */
	WAVEINCAPS		m_WaveInDevCaps;
	HWAVEIN			m_WaveIn;
	HANDLE			m_WaveInEvent;
	WAVEHDR			m_WaveInHeader[NO_SOUND_BUFFERS_IN];
	int				iBufferSizeIn;
	int				iWhichBufferIn;
	short*			psSoundcardBuffer[NO_SOUND_BUFFERS_IN];

	/* Wave out */
	int				iBufferSizeOut;
	HWAVEOUT		m_WaveOut;
	short*			psPlaybackBuffer[NO_SOUND_BUFFERS_OUT];
	WAVEHDR			m_WaveOutHeader[NO_SOUND_BUFFERS_OUT];
};


#endif // !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
