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
	int	iCurDataStreamID;

	/* Get current data stream ID */
	iCurDataStreamID =
		ReceiverParam.Service[ReceiverParam.GetCurSelDataService()].
		DataParam.iStreamID;

	/* Check, if service is activated */
	if (iCurDataStreamID != STREAM_ID_NOT_USED)
	{
		/* Length of higher and lower protected part of data stream */
		iLenDataHigh = ReceiverParam.Stream[iCurDataStreamID].iLenPartA;
		iLenDataLow = ReceiverParam.Stream[iCurDataStreamID].iLenPartB;
	}
	else
	{
		/* Selected stream is not active, set everyting to zero */
		iLenDataHigh = 0;
		iLenDataLow = 0;
	}

	iTotalNoInputBits = (iLenDataHigh + iLenDataLow) * SIZEOF__BYTE;

	/* Set input block size */
	iInputBlockSize = iTotalNoInputBits;
}
