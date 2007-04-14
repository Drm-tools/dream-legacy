/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Oliver Haffenden
 *
 * Description:
 *  
 * See PacketSinkFile.h
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

#include "PacketSinkFile.h"

CPacketSinkRawFile::CPacketSinkRawFile()
: pFile(0), bIsRecording(0), bReopenFile(FALSE)
{
}

void CPacketSinkRawFile::SendPacket(const vector<_BYTE>& vecbydata, uint32_t, uint16_t)
{
       if (!bIsRecording) // not recording
       {
               if (pFile != 0) // close file if one is open
               {
                       fclose(pFile);
                       pFile = 0;
               }
               return;
       }

       if (bReopenFile) // file is open but we want to start a new one
       {
               bReopenFile = FALSE;
               if (pFile)
               {
                       fclose(pFile);
               }
               pFile = 0;
       }

       if (!pFile) // either wasn't open, or we just closed it
       {
               pFile = fopen(strFileName.c_str(), "wb");
               if (!pFile)
             {
                       // Failed to open
                       bIsRecording = FALSE;
                       return;
               }
       }

	_BYTE b;
	for (size_t i = 0; i < vecbydata.size (); i++)
	{
		b = vecbydata[i];
		fwrite(&b,1,1,pFile);
	}

}

_BOOLEAN CPacketSinkRawFile::SetDestination(const string& strFName)
{
	strFileName = strFName;
	return TRUE;
}

void CPacketSinkRawFile::StartRecording()
{
	if (bIsRecording) // file already open: close it and open new file
		bReopenFile = TRUE;
	bIsRecording = TRUE;
}

void CPacketSinkRawFile::StopRecording()
{
	bIsRecording = FALSE;
}
