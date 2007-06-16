/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable
 *
 * Decription:
 * Sound in interface using POSIX shared memory
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

#define _POSIX_C_SOURCE 199309
#include <time.h>
#include <sys/types.h>
#include <iostream>
# include <sys/mman.h>
# include <fcntl.h>
# include <unistd.h>
# include <sys/stat.h>
# include "shmsoundin.h"

CShmSoundIn::CShmSoundIn():ringBuffer(NULL),
		shmid(-1),shm(NULL),shm_path(),name("shm input"),channels(2)
{
}

CShmSoundIn::~CShmSoundIn()
{
	Close();
}

void
CShmSoundIn::Init(int iNewBufferSize, _BOOLEAN bNewBlocking)
{
	shmid = shm_open(shm_path.c_str(), O_RDWR, 0666);
	if(shmid == -1)
	{
		perror("shm_open");
		return;
	}
	struct stat s;
	fstat(shmid, &s);
	shm = mmap(0, s.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, shmid, 0);
	if((void*)shm == (void*)-1)
	{
		perror("mmap");
		return;
	}
	ringBuffer = (PaUtilShmRingBuffer*)shm;
}

void
CShmSoundIn::Enumerate(vector < string > &choices)
{
	choices.clear();
	if(shmid==-1)
		return;
	choices.push_back(name);
}

void
CShmSoundIn::SetDev(int iNewDevice)
{
}

int
CShmSoundIn::GetDev()
{
	if(shmid==-1)
		return -1;
	return 0;
}

_BOOLEAN
CShmSoundIn::Read(CVector<short>& psData)
{
	if(ringBuffer==NULL)
		return FALSE;

	size_t samples = psData.Size();
	size_t bytes = sizeof(short)*samples;
	if(channels==1)
		bytes = bytes/2;
	
	while(PaUtil_GetShmRingBufferReadAvailable(ringBuffer)<int(bytes))
	{
		timespec ts;
		ts.tv_sec=0;
		ts.tv_nsec = 10000000; // 10 ms
		nanosleep(&ts, NULL);
	}

	if(channels==2)
	{
		PaUtil_ReadShmRingBuffer(ringBuffer, &psData[0], bytes);
	}
	else
	{
		vector<short> buf(bytes);
		PaUtil_ReadShmRingBuffer(ringBuffer, &buf[0], bytes);
		for(size_t i=0; i<buf.size(); i++)
		{
			psData[2*i] = buf[i];
			psData[2*i+1] = buf[i];
		}
	}
	
	return TRUE;
}

void
CShmSoundIn::Close()
{
	struct stat s;
	fstat(shmid, &s);
	munmap(shm, s.st_size);
	shm_unlink(shm_path.c_str());
	cout << "capture close" << endl;
}
