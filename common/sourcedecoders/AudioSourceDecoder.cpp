/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source decoder
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
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 1111
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "AudioSourceDecoder.h"


/* Implementation *************************************************************/
void CAudioSourceDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int i;

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
		return;


	/* Text Message ***********************************************************/
	/* Total frame size depends on whether text message is used or not */
	if (bTextMessageUsed == TRUE)
	{
		/* Decode last for bytes of input block for text message */
		for (i = 0; i < SIZEOF__BYTE * 4; i++)
			vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

		TextMessage.Decode(vecbiTextMessBuf);
	}


#ifdef USE_FAAD2_LIBRARY
	faacDecFrameInfo	DecFrameInfo;
	_BOOLEAN			bGoodValues;
	short*				psDecOutSampleBuf;
	int					j;

	/* Check if AAC should not be decoded */
	if (DoNotProcessAAC == TRUE)
		return;


	/* Extract audio data from stream *****************************************/
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();


	/* AAC super-frame-header ----------------------------------------------- */
	int iPrevBorder = 0;
	for (i = 0; i < iNumBorders; i++)
	{
		/* Frame border in bytes (12 bits) */
		const int iFrameBorder = (*pvecInputData).Separate(12);

		/* The lenght is difference between borders */
		veciFrameLength[i] = iFrameBorder - iPrevBorder;
		iPrevBorder = iFrameBorder;
	}

	/* Byte-alignment (4 bits) in case of 10 audio frames */
	if (iNumBorders == 9)
		(*pvecInputData).Separate(4); 

	/* Frame length of last frame */
	veciFrameLength[iNumBorders] = iAudioPayloadLen - iPrevBorder;

	/* Check if frame length entries represent possible values */
	bGoodValues = TRUE;
	for (i = 0; i < iNumAACFrames; i++)
	{
		if ((veciFrameLength[i] < 0) ||
			(veciFrameLength[i] > iMaxLenOneAudFrame))
		{
			bGoodValues = FALSE;
		}
	}

	if (bGoodValues == TRUE)
	{
		/* Higher-protected part -------------------------------------------- */
		for (i = 0; i < iNumAACFrames; i++)
		{
			/* Extract higher protected part bytes (8 bits per byte) */
			for (j = 0; j < iNumHigherProtectedBytes; j++)
				audio_frame[i][j] = (*pvecInputData).Separate(8);

			/* Extract CRC bits (8 bits) */
			aac_crc_bits[i] = (*pvecInputData).Separate(8);
		}


		/* Lower-protected part --------------------------------------------- */
		for (i = 0; i < iNumAACFrames; i++)
		{
			/* First calculate frame length, derived from higher protected part
			   frame length and total size */
			const int iNumLowerProtectedBytes = 
				veciFrameLength[i] - iNumHigherProtectedBytes;

			/* Extract lower protected part bytes (8 bits per byte) */
			for (j = 0; j < iNumLowerProtectedBytes; j++)
				audio_frame[i][iNumHigherProtectedBytes + j] = 
					(*pvecInputData).Separate(8);
		}
	}


	/* AAC decoder ************************************************************/
	/* Init output block size to zero, this variable is also used for
	   determining the position for writing the output vector */
	iOutputBlockSize = 0;

	for (j = 0; j < iNumAACFrames; j++)
	{
		if (bGoodValues == TRUE)
		{
			/* Prepare data vector with CRC at the beginning (the definition
			   with faad2 DRM interface) */
			vecbyPrepAudioFrame[0] = aac_crc_bits[j];

			for (i = 0; i < veciFrameLength[j]; i++)
				vecbyPrepAudioFrame[i + 1] = audio_frame[j][i];

#if 0
// Store AAC-data in file
static FILE* pFile2 = fopen("test/aac.dat", "wb");

int iNewFrL = veciFrameLength[j] + 1;

// Frame length
fwrite((void*) &iNewFrL, size_t(4), size_t(1), pFile2);

size_t count = veciFrameLength[j] + 1; /* Number of bytes to write */
fwrite((void*) &vecbyPrepAudioFrame[0], size_t(1), count, pFile2);

fflush(pFile2);
#endif

			/* Call decoder routine */
			psDecOutSampleBuf = (short*) faacDecDecode(HandleAACDecoder,
				&DecFrameInfo, &vecbyPrepAudioFrame[0], veciFrameLength[j] + 1);
		}
		else
		{
			/* DRM AAC header was wrong, set decoder error code */
			DecFrameInfo.error = 1;
		}

		if (DecFrameInfo.error != 0)
		{
			/* Set AAC CRC result in log file */
			ReceiverParam.ReceptLog.SetMSC(FALSE);

			if (bAudioWasOK == TRUE)
			{
				bAudioWasOK = FALSE;

				/* Post message to show that CRC was wrong (yellow light) */
				PostWinMessage(MS_MSC_CRC, 1);
			}
			else
			{
				/* Post message to show that CRC was wrong (red light) */
				PostWinMessage(MS_MSC_CRC, 2);
			}

			/* Average block with flipped block for smoother transition */
			for (i = 0; i < iResOutBlockSize / 2; i++)
			{
				vecTempResBufOutLeft[i] = (vecTempResBufOutLeft[i] +
					vecTempResBufOutLeft[iResOutBlockSize - i - 1]) / 2;

				vecTempResBufOutRight[i] = (vecTempResBufOutRight[i] +
					vecTempResBufOutRight[iResOutBlockSize - i - 1]) / 2;
			}

			/* Attenuate the gain exponentially */
			for (i = 0; i < iResOutBlockSize; i++)
			{
				/* Make a lower bound of signal because the floating point
				   variable can get as small as it reaches the lower precision
				   and this can cause instability! */
				if (fabs(vecTempResBufOutLeft[i]) > (_REAL) 1.0)
					vecTempResBufOutLeft[i] *= FORFACT_AUD_BL_BAD_CRC;
				else
					vecTempResBufOutLeft[i] = (_REAL) 0.0;

				if (fabs(vecTempResBufOutRight[i]) > (_REAL) 1.0)
					vecTempResBufOutRight[i] *= FORFACT_AUD_BL_BAD_CRC;
				else
					vecTempResBufOutRight[i] = (_REAL) 0.0;
			}
		}
		else
		{
			/* Set AAC CRC result in log file */
			ReceiverParam.ReceptLog.SetMSC(TRUE);

			/* Post message to show that CRC was OK and reset flag */
			PostWinMessage(MS_MSC_CRC, 0);
			bAudioWasOK = TRUE;

			/* Conversion from _SAMPLE vector to _REAL vector for resampling.
			   ATTENTION: We use a vector which was allocated inside
			   the AAC decoder! */
			if (iNumChannelsAAC == 1)
			{
				/* Change type of data (short -> real) */
				for (i = 0; i < iLenDecOutPerChan; i++)
					vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

				/* Resample data */
				ResampleObjL.Resample(vecTempResBufInLeft,
					vecTempResBufOutLeft);

				/* Mono (write the same audio material in both channels) */
				for (i = 0; i < iResOutBlockSize; i++)
					vecTempResBufOutRight[i] = vecTempResBufOutLeft[i];
			}
			else
			{
				/* Stereo */
				for (i = 0; i < iLenDecOutPerChan; i++)
				{
					vecTempResBufInLeft[i] = psDecOutSampleBuf[i * 2];
					vecTempResBufInRight[i] = psDecOutSampleBuf[i * 2 + 1];
				}

				/* Resample data */
				ResampleObjL.Resample(vecTempResBufInLeft,
					vecTempResBufOutLeft);
				ResampleObjR.Resample(vecTempResBufInRight,
					vecTempResBufOutRight);
			}
		}

		/* Conversion from _REAL to _SAMPLE with special function */
		for (i = 0; i < iResOutBlockSize; i++)
		{
			(*pvecOutputData)[iOutputBlockSize + i * 2] = 
				Real2Sample(vecTempResBufOutLeft[i]); /* Left channel */
			(*pvecOutputData)[iOutputBlockSize + i * 2 + 1] =
				Real2Sample(vecTempResBufOutRight[i]); /* Right channel */
		}

		/* Add new block to output block size ("* 2" for stereo output block) */
		iOutputBlockSize += iResOutBlockSize * 2;
	}
#endif
}

void CAudioSourceDecoder::InitInternal(CParameter& ReceiverParam)
{
/*
	Since we use the exception mechanism in this init routine, the sequence of
	the individual initializations is very important!
	Requirement for text message is "stream is used" and "audio service".
	Requirement for AAC decoding are the requirements above plus "audio coding
	is AAC"
*/
	int iCurAudioStreamID;
	int iMaxLenResamplerOutput;
	int iCurSelServ;
	int iDRMchanMode;
	int iAudioSampleRate;
	int iAACSampleRate;
	int	iLenAudHigh;
	int	iNumHeaderBytes;

	try
	{
		/* Init error flags and output block size parameter. The output block
		   size is set in the processing routine. We must set it here in case
		   of an error in the initialization, this part in the processing
		   routine is not being called */
		DoNotProcessAAC = FALSE;
		DoNotProcessData = FALSE;
		iOutputBlockSize = 0;

		/* Get number of total input bits for this module */
		iInputBlockSize = ReceiverParam.iNumAudioDecoderBits;

		/* Get current selected audio service */
		iCurSelServ = ReceiverParam.GetCurSelAudioService();

		/* Current audio stream ID */
		iCurAudioStreamID =
			ReceiverParam.Service[iCurSelServ].AudioParam.iStreamID;

		/* The requirement for this module is that the stream is used and the
		   service is an audio service. Check it here */
		if ((ReceiverParam.Service[iCurSelServ].
			eAudDataFlag != CParameter::SF_AUDIO) ||
			(iCurAudioStreamID == STREAM_ID_NOT_USED))
		{
			throw CInitErr(ET_ALL);
		}


		/* Init text message application ------------------------------------ */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.bTextflag)
		{
		case TRUE:
			bTextMessageUsed = TRUE;

			/* Get a pointer to the string */
			TextMessage.Init(&ReceiverParam.Service[iCurSelServ].AudioParam.
				strTextMessage);

			/* Total frame size is input block size minus the bytes for the text
			   message */
			iTotalFrameSize = iInputBlockSize -
				SIZEOF__BYTE * NO_BYTES_TEXT_MESS_IN_AUD_STR;

			/* Init vector for text message bytes */
			vecbiTextMessBuf.Init(SIZEOF__BYTE * NO_BYTES_TEXT_MESS_IN_AUD_STR);
			break;

		case FALSE:
			bTextMessageUsed = FALSE;

			/* All bytes are used for AAC data, no text message present */
			iTotalFrameSize = iInputBlockSize;
			break;
		}


#ifdef USE_FAAD2_LIBRARY
		/* Init for AAC decoding -------------------------------------------- */
		/* Check, if AAC is used */
		if (ReceiverParam.Service[iCurSelServ].AudioParam.
			eAudioCoding != CParameter::AC_AAC)
		{
			throw CInitErr(ET_AAC);
		}

		/* Init "audio was ok" flag */
		bAudioWasOK = TRUE;

		/* Lengths of higher and lower protected part of audio stream */
		iLenAudHigh = ReceiverParam.Stream[iCurAudioStreamID].iLenPartA;
		iLenAudLow = ReceiverParam.Stream[iCurAudioStreamID].iLenPartB;

		/* Set number of AAC frames in a AAC super-frame */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioSamplRate)
		{ /* only 12 kHz and 24 kHz is allowed */
		case CParameter::AS_12KHZ:
			iNumAACFrames = 5;
			iNumHeaderBytes = 6;
			iAACSampleRate = 12000;
			break;

		case CParameter::AS_24KHZ:
			iNumAACFrames = 10;
			iNumHeaderBytes = 14;
			iAACSampleRate = 24000;
			break;

		default:
			/* Some error occurred, throw error */
			throw CInitErr(ET_AAC);
			break;
		}

		/* Number of borders */
		iNumBorders = iNumAACFrames - 1;

		/* Set number of AAC frames for log file */
		ReceiverParam.ReceptLog.SetNumAAC(iNumAACFrames);

		/* Number of channels for AAC: Mono, LC Stereo, Stereo */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioMode)
		{
		case CParameter::AM_MONO:
			iNumChannelsAAC = 1;

			if (ReceiverParam.Service[iCurSelServ].AudioParam.
				eSBRFlag == CParameter::SB_USED)
			{
				iDRMchanMode = DRMCH_SBR_MONO;
			}
			else
				iDRMchanMode = DRMCH_MONO;
			break;

		case CParameter::AM_LC_STEREO:
			/* Low-complexity only defined in SBR mode */
			iNumChannelsAAC = 1;
			iDRMchanMode = DRMCH_SBR_LC_STEREO;
			break;

		case CParameter::AM_STEREO:
			iNumChannelsAAC = 2;

			if (ReceiverParam.Service[iCurSelServ].AudioParam.
				eSBRFlag == CParameter::SB_USED)
			{
				iDRMchanMode = DRMCH_SBR_STEREO;
			}
			else
			{
				iDRMchanMode = DRMCH_STEREO;
			}
			break;
		}

		/* In case of SBR, AAC sample rate is half the total sample rate. Length
		   of output is doubled if SBR is used */
		if (ReceiverParam.Service[iCurSelServ].AudioParam.
			eSBRFlag == CParameter::SB_USED)
		{
			iAudioSampleRate = iAACSampleRate * 2;
			iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH * 2;
		}
		else
		{
			iAudioSampleRate = iAACSampleRate;
			iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH;
		}

		/* The audio_payload_length is derived from the length of the audio
		   super frame (data_length_of_part_A + data_length_of_part_B)
		   subtracting the audio super frame overhead (bytes used for the audio
		   super frame header() and for the aac_crc_bits) (5.3.1.1, Table 5) */
		iAudioPayloadLen =
			iTotalFrameSize / SIZEOF__BYTE - iNumHeaderBytes - iNumAACFrames;

		/* Check iAudioPayloadLen value, only positive values make sense */
		if (iAudioPayloadLen < 0)
			throw CInitErr(ET_AAC);

		/* Calculate number of bytes for higher protected blocks */
		iNumHigherProtectedBytes =
			(iLenAudHigh - iNumHeaderBytes - iNumAACFrames /* CRC bytes */) /
			iNumAACFrames;

		if (iNumHigherProtectedBytes < 0)
			iNumHigherProtectedBytes = 0;

		/* Since we do not correct for sample rate offsets here (yet), we do not
		   have to consider larger buffers. An audio frame always corresponds
		   to 400 ms */
		iMaxLenResamplerOutput = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
			(_REAL) 0.4 /* 400ms */ * 2 /* for stereo */);

		iResOutBlockSize = (int) ((_REAL) iLenDecOutPerChan *
			SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);

		/* Additional buffers needed for resampling since we need conversation
		   between _REAL and _SAMPLE. We have to init the buffers with
		   zeros since it can happen, that we have bad CRC right at the
		   start of audio blocks */
		vecTempResBufInLeft.Init(iLenDecOutPerChan);
		vecTempResBufInRight.Init(iLenDecOutPerChan);
		vecTempResBufOutLeft.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutRight.Init(iResOutBlockSize, (_REAL) 0.0);

		/* Init resample objects */
		ResampleObjL.Init(iLenDecOutPerChan,
			(_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);
		ResampleObjR.Init(iLenDecOutPerChan,
			(_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);


		/* AAC decoder ------------------------------------------------------ */
		/* The maximum length for one audio frame is "iAudioPayloadLen". The
		   regular size will be much shorter since all audio frames share the
		   total size, but we do not know at this time how the data is 
		   split in the transmitter source coder */
		iMaxLenOneAudFrame = iAudioPayloadLen;
		audio_frame.Init(iNumAACFrames, iMaxLenOneAudFrame);

		/* Init vector which stores the data with the CRC at the beginning
		   ("+ 1" for CRC) */
		vecbyPrepAudioFrame.Init(iMaxLenOneAudFrame + 1);

		/* Init storage for CRCs and frame lengths */
		aac_crc_bits.Init(iNumAACFrames);
		veciFrameLength.Init(iNumAACFrames);

		/* Init AAC-decoder */
		faacDecInitDRM(HandleAACDecoder, iAACSampleRate, iDRMchanMode);

		/* With this parameter we define the maximum lenght of the output
		   buffer. The cyclic buffer is only needed if we do a sample rate
		   correction due to a difference compared to the transmitter. But for
		   now we do not correct and we could stay with a single buffer
		   Maybe TODO: sample rate correction to avoid audio dropouts */
		iMaxOutputBlockSize = iMaxLenResamplerOutput;
#endif
	}

	catch (CInitErr CurErr)
	{
		switch (CurErr.eErrType)
		{
		case ET_ALL:
			/* An init error occurred, do not process data in this module */
			DoNotProcessData = TRUE;
			break;

		case ET_AAC:
			/* AAC part should not be decdoded, set flag */
			DoNotProcessAAC = TRUE;
			break;

		default:
			DoNotProcessData = TRUE;
		}
	}
}

CAudioSourceDecoder::CAudioSourceDecoder()
{
#ifdef USE_FAAD2_LIBRARY
	/* Open AACEncoder instance */
	HandleAACDecoder = faacDecOpen();

	/* Decoder MUST be initialized at least once, therefore do it here in the
	   constructor with arbitrary values to be sure that this is satisfied */
	faacDecInitDRM(HandleAACDecoder, 24000, DRMCH_MONO);
#endif
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
#ifdef USE_FAAD2_LIBRARY
	/* Close decoder handle */
	faacDecClose(HandleAACDecoder);
#endif
}
