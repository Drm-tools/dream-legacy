/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Data module (using multimedia information carried in DRM stream)
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

#include "DataDecoder.h"


/* Implementation *************************************************************/
void CDataDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	/* TODO: Implementation */
}

void CDataDecoder::InitInternal(CParameter& ReceiverParam)
{
// TODO : Implementation
	iInputBlockSize = 0;
#if 0
/* Length of higher and lower protected part of audio stream */
iLenAudHigh = ReceiverParam.Stream[iCurAudioStreamID].iLenPartA;
iLenAudLow = ReceiverParam.Stream[iCurAudioStreamID].iLenPartB;


/* Define input block size */
iInputBlockSize = (iLenAudHigh + iLenAudLow) * SIZEOF__BYTE;
#endif
}
