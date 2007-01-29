/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 * 
 * Decription:
 * Read a file at the correct rate
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

#include "audiofilein.h"
#include <time.h>
#include <iostream>
#define FILE_DRM_USING_RAW_DATA

CAudioFileIn::CAudioFileIn(): CSoundInInterface(), pFileReceiver(NULL),
			strInFileName(), interval(0), timekeeper(0)
{
}

CAudioFileIn::~CAudioFileIn()
{
	Close();
}

void
CAudioFileIn::SetFileName(const string& strFileName)
{
	strInFileName = strFileName;
}

void
CAudioFileIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	/* Check previously a file was being used */
	if (pFileReceiver != NULL)
	{
		fclose(pFileReceiver);
		pFileReceiver = NULL;
	}

	pFileReceiver = fopen(strInFileName.c_str(), "rb");
	/* Check for error */
	if (pFileReceiver == NULL)
		throw CGenErr("The file " + strInFileName + " must exist.");

	interval = uint64_t(1e9 * double(iNewBufferSize) / double(SOUNDCRD_SAMPLE_RATE) / 2ULL);
	timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	timekeeper = 1000000000ULL * uint64_t(now.tv_sec) + uint64_t(now.tv_nsec);
}

_BOOLEAN
CAudioFileIn::Read(CVector<short>& psData)
{
	timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	uint64_t now_ns = 1000000000ULL * uint64_t(now.tv_sec) + uint64_t(now.tv_nsec);
	int64_t delay_ns = timekeeper + interval - now_ns;
	timekeeper += interval;
	if(delay_ns > 20000000LL) /* don't expect too much resolution from nanosleep */
	{
		timespec delay;
		delay.tv_sec = delay_ns / 1000000000LL;
		delay.tv_nsec = delay_ns % 1000000000LL;
		nanosleep(&delay, NULL);
	}

	if (pFileReceiver == NULL)
		return TRUE;

	/* Read data from file ---------------------------------------------- */
	int iOutputBlockSize = psData.Size() / 2; /* psData is stereo ! */
	for (int i = 0; i < iOutputBlockSize; i++)
	{
#ifdef FILE_DRM_USING_RAW_DATA
		short tIn;

		/* Read 2 bytes, 1 piece */
		if (fread((void*) &tIn, size_t(2), size_t(1), pFileReceiver) == size_t(0))
		{
			rewind(pFileReceiver);
			fread((void*) &tIn, size_t(2), size_t(1), pFileReceiver);
		}
		psData[2*i] = (short)tIn;
		psData[2*i+1] = (short)tIn;
#else
		float tIn;

		if (fscanf(pFileReceiver, "%e\n", &tIn) == EOF)
		{
			/* If end-of-file is reached, stop simulation */
			return FALSE;
		}
		psData[2*i] = (short)tIn;
		psData[2*i+1] = (short)tIn;
#endif
	}
	return FALSE;
}

void
CAudioFileIn::Close()
{
	cout << "CAudioFileIn::Close" << endl;
	/* Close file (if opened) */
	if (pFileReceiver != NULL)
		fclose(pFileReceiver);
}
