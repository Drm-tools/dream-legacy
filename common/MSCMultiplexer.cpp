/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	MSC audio/data demultiplexer
 *
 *
 * - (6.2.3.1) Multiplex frames (DRM standard):
 * The multiplex frames are built by placing the logical frames from each 
 * non-hierarchical stream together. The logical frames consist, in general, of 
 * two parts each with a separate protection level. The multiplex frame is 
 * constructed by taking the data from the higher protected part of the logical 
 * frame from the lowest numbered stream (stream 0 when hierarchical modulation 
 * is not used, or stream 1 when hierarchical modulation is used) and placing 
 * it at the start of the multiplex frame. Next the data from the higher 
 * protected part of the logical frame from the next lowest numbered stream is 
 * appended and so on until all streams have been transferred. The data from 
 * the lower protected part of the logical frame from the lowest numbered 
 * stream (stream 0 when hierarchical modulation is not used, or stream 1 when
 * hierarchical modulation is used) is then appended, followed by the data from 
 * the lower protected part of the logical frame from the next lowest numbered 
 * stream, and so on until all streams have been transferred. The higher 
 * protected part is designated part A and the lower protected part is 
 * designated part B in the multiplex description. The multiplex frame will be
 * larger than or equal to the sum of the logical frames from which it is 
 * formed. The remainder, if any, of the multiplex frame shall be filled with 
 * 0s. These bits shall be ignored by the receiver.
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

#include "MSCMultiplexer.h"


/* Implementation *************************************************************/
void CMSCDemultiplexer::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i;

	/* Extract audio data from input-stream */
	/* Higher protected part */
	for (i = 0; i < iLenAudHigh; i++)
		(*pvecOutputData)[i] = (*pvecInputData)[i + iOffsetAudHigh];

	/* Lower protected part */
	for (i = 0; i < iLenAudLow; i++)
		(*pvecOutputData)[i + iLenAudHigh] = 
			(*pvecInputData)[i + iOffsetAudLow];



// TODO extract data streams for DataDecoder

}

void CMSCDemultiplexer::InitInternal(CParameter& ReceiverParam)
{
	int				i;
	int				iNoServices;
	int				iCurAudioStreamID;
	int				iCurSelServ;
	CVector<int>	veciActStreams;

	/* First, init values so that nothing is done in this module (in case we do
	   not have an audio stream or the parameters are wrong */
	iLenAudHigh = 0;
	iLenAudLow = 0;
	iOutputBlockSize = 0;

	/* Get current selected service */
	iCurSelServ = ReceiverParam.GetCurSelectedService();

	/* Current audio stream ID */
	iCurAudioStreamID = ReceiverParam.Service[iCurSelServ].AudioParam.iStreamID;

	/* Check if current selected service is an audio service and check if 
	   a stream is attached */
	if ((ReceiverParam.Service[iCurSelServ].
		eAudDataFlag == CParameter::SF_AUDIO) && 
		(iCurAudioStreamID != STREAM_ID_NOT_USED))
	{
		/* Init number of services */
		iNoServices = 
			ReceiverParam.iNoAudioService + ReceiverParam.iNoDataService;

		/* Byte-offset of higher and lower protected part of audio stream --- */
		/* Get active streams */
		ReceiverParam.GetActiveStreams(veciActStreams);

		/* Init offsets */
		iOffsetAudHigh = 0;
		iOffsetAudLow = 0;

		/* Get start offset for lower protected parts in stream. Since lower 
		   protected part comes after the higher protected part, the offset
		   must be shifted initially by all higher protected part lengths
		   (iLenPartA of all streams are added) 6.2.3.1 */
		for (i = 0; i < veciActStreams.Size(); i++)
			iOffsetAudLow += ReceiverParam.Stream[veciActStreams[i]].
				iLenPartA * SIZEOF__BYTE;

		/* Real start position of the streams */
		for (i = 0; i < veciActStreams.Size(); i++)
		{
			if (veciActStreams[i] < iCurAudioStreamID)
			{
				iOffsetAudHigh += ReceiverParam.Stream[i].iLenPartA * 
					SIZEOF__BYTE;
				iOffsetAudLow += ReceiverParam.Stream[i].iLenPartB * 
					SIZEOF__BYTE;
			}
		}

		/* Length of higher and lower protected part of audio stream (number
		   of bits) */
		iLenAudHigh = ReceiverParam.Stream[iCurAudioStreamID].iLenPartA * 
			SIZEOF__BYTE;
		iLenAudLow = ReceiverParam.Stream[iCurAudioStreamID].iLenPartB * 
			SIZEOF__BYTE;

		/* Special case if hierarchical modulation is used */
		if (((ReceiverParam.eMSCCodingScheme == CParameter::CS_3_HMSYM) || 
			(ReceiverParam.eMSCCodingScheme == CParameter::CS_3_HMMIX)))
		{
			if (iCurAudioStreamID == 0)
			{
				/* Hierarchical channel is selected. Data is at the beginning
				   of incoming data block */
				iOffsetAudLow = 0;
			}
			else
			{
				/* Shift all offsets by the length of the hierarchical frame. We
				   cannot use the information about the length in 
				   "Stream[0].iLenPartB", because the real length of the frame
				   is longer or equal the length in "Stream[0].iLenPartB" */
				iOffsetAudHigh += ReceiverParam.iNoBitsHierarchFrameTotal;
				iOffsetAudLow += ReceiverParam.iNoBitsHierarchFrameTotal -
					/* We have to subtract this because we added it in the
					   for loop above which we do not need here */
					ReceiverParam.Stream[0].iLenPartB * SIZEOF__BYTE;
			}
		}


		/* Possibility check ------------------------------------------------ */
		/* Test, if parameters have possible values */
		if ((iOffsetAudHigh + iLenAudHigh > ReceiverParam.iNoDecodedBitsMSC) ||
			(iLenAudLow + iOffsetAudLow > ReceiverParam.iNoDecodedBitsMSC))
		{
			/* Something is wrong, do not use the parameters and do nothing in 
			   this module */
			iLenAudHigh = 0;
			iLenAudLow = 0;
			iOutputBlockSize = 0;
		}
		else
		{
			/* Parameters are ok, set output block sizes for this module */
			iOutputBlockSize = iLenAudHigh + iLenAudLow; /* No of bits */
		}
	}

	/* Set input block size */
	iInputBlockSize = ReceiverParam.iNoDecodedBitsMSC;


// For data stream, currently no data stream is used -> TODO
iOutputBlockSize2 = 0;
}
