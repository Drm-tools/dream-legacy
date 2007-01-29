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
#include <windows.h>
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

	interval = uint64_t(1e7 * double(iNewBufferSize) / double(SOUNDCRD_SAMPLE_RATE)/ 2.0);
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	timekeeper = *(uint64_t*)&ft;
	timekeeper += interval;
}

_BOOLEAN
CAudioFileIn::Read(CVector<short>& psData)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	uint64_t now = *(uint64_t*)&ft;
	double delay_ms = (double(timekeeper) - double(now))/10000.0;
	timekeeper += interval;
	if(delay_ms > 10.0)
	{
		Sleep(uint32_t(delay_ms)-10);
	}

	if (pFileReceiver == NULL)
		return TRUE;

	/* Read data from file ---------------------------------------------- */
	for (int i = 0; i < psData.Size()/2; i++)
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
	/* Close file (if opened) */
	if (pFileReceiver != NULL)
		fclose(pFileReceiver);
}
