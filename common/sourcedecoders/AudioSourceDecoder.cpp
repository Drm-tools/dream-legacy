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
	int					i, j;
	int					iNumLowerProtectedBytes;
	short*				psDecOutSampleBuf;
	faacDecFrameInfo	DecFrameInfo;

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


	/* Extract audio data from stream *****************************************/
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();


	/* AAC super-frame-header ----------------------------------------------- */
	/* First border is always "0" */
	veciBorders[0] = 0;

	/* Parse middle borders. Frame borders in bytes (border number is stored in
	   12 bits). Subtract the porition for the higher protected bytes, because
	   we need the borders only for lower protected part. The lengths of higher
	   protected parts are all equal and must not be transmitted in these
	   borders */
	for (i = 1; i < iNumAACFrames; i++)
		veciBorders[i] =
			(*pvecInputData).Separate(12) - i * iNumHigherProtectedBytes;

	/* Last border (subtract higher protected part from all other audio
	   frames) */
	veciBorders[iNumAACFrames] =
		iAudioPayloadLen - iNumAACFrames * iNumHigherProtectedBytes;

	/* Byte-alignment (4 bits) in case of 10 audio frames */
	if (iNumAACFrames == 10)
		(*pvecInputData).Separate(4);


	/* Higher-protected part ------------------------------------------------ */
	for (i = 0; i < iNumAACFrames; i++)
	{
		/* Extract higher protected part bytes (8 bits per byte) */
		for (j = 0; j < iNumHigherProtectedBytes; j++)
			audio_frame[i][j] = (*pvecInputData).Separate(8);

		/* Extract CRC bits (8 bits) */
		aac_crc_bits[i] = (*pvecInputData).Separate(8);

		/* Init frame lengths */
		veciFrameLength[i] = iNumHigherProtectedBytes;
	}


	/* Lower-protected part ------------------------------------------------- */
	for (j = 0; j < iLenAudLow; j++)
	{
		/* Get current byte */
		_BYTE byCurData = (*pvecInputData).Separate(8);

		/* Check for each audio frame if it is in range by checking the
		   transmitted borders. In case one border was received with errors, it
		   can happen that the next border is lower than the first border. In
		   this case, the previous and next frame share some of the bytes. Of
		   course, one of these are incorrect but we do not know which one is
		   the correct one and which one is wrong, therefore we read both of
		   them */
		for (i = 0; i < iNumAACFrames; i++)
		{
			/* Check if we are in range for this audio frame */
			if ((j >= veciBorders[i]) && (j < veciBorders[i + 1]))
			{
				/* Always check the total size of this audio frame. In case of
				   wrong borders, it could cause an overflow */
				if (veciFrameLength[i] < iMaxLenOneAudFrame)
				{
					/* Assign new byte and increase counter */
					audio_frame[i][veciFrameLength[i]] = byCurData;

					veciFrameLength[i]++;
				}
			}
		}
	}


	/* AAC decoder ************************************************************/
	/* Init output block size to zero, this variable is also used for
	   determining the position for writing the output vector */
	iOutputBlockSize = 0;

	for (j = 0; j < iNumAACFrames; j++)
	{
		/* Prepare data vector with CRC at the beginning
		   (my definition with faad2) */
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
			if (iNoChannelsAAC == 1)
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
}

void CAudioSourceDecoder::InitInternal(CParameter& ReceiverParam)
{
	int iCurAudioStreamID;
	int iTotalNumInputBits;
	int iMaxLenResamplerOutput;
	int iCurSelServ;
	int iDRMchanMode;
	int iAudioSampleRate;
	int iAACSampleRate;
	int	iLenAudHigh;
	int	iNumHeaderBytes;

	/* Init error flag */
	DoNotProcessData = FALSE;

	/* Get current selected audio service */
	iCurSelServ = ReceiverParam.GetCurSelAudioService();

	/* Current audio stream ID */
	iCurAudioStreamID = ReceiverParam.Service[iCurSelServ].AudioParam.iStreamID;

	/* Get number of total input bits for this module */
	iTotalNumInputBits = ReceiverParam.iNumAudioDecoderBits;

	/* Check if current selected service is an audio service and check if a
	   stream is attached. Additionally, check if AAC (this is the only audio
	   decoding we offer right now) */
	if ((ReceiverParam.Service[iCurSelServ].
		eAudDataFlag == CParameter::SF_AUDIO) &&
		(iCurAudioStreamID != STREAM_ID_NOT_USED) &&
		(ReceiverParam.Service[iCurSelServ].AudioParam.
		eAudioCoding == CParameter::AC_AAC))
	{
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
			/* Some error ocurred, set parameters to valid values and set error
			   flag. TODO better solution, better error handling! */
			iNumAACFrames = 10;
			iNumHeaderBytes = 14;
			iAACSampleRate = 24000;
			DoNotProcessData = TRUE;
			break;
		}

		/* Set number of AAC frames for log file */
		ReceiverParam.ReceptLog.SetNumAAC(iNumAACFrames);

		/* If text message application is used or not */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.bTextflag)
		{
		case TRUE:
			bTextMessageUsed = TRUE;

			/* Get a pointer to the string */
			TextMessage.Init(&ReceiverParam.Service[iCurSelServ].AudioParam.
				strTextMessage);

			/* Total frame size is input block size minus the bytes for the text
			   message */
			iTotalFrameSize = iTotalNumInputBits -
				SIZEOF__BYTE * NO_BYTES_TEXT_MESS_IN_AUD_STR;

			/* Init vector for text message bytes */
			vecbiTextMessBuf.Init(SIZEOF__BYTE * NO_BYTES_TEXT_MESS_IN_AUD_STR);
			break;

		case FALSE:
			bTextMessageUsed = FALSE;

			/* All bytes are used for AAC data, no text message present */
			iTotalFrameSize = iTotalNumInputBits;
			break;
		}

		/* Number of channels for AAC: Mono, LC Stereo, Stereo */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioMode)
		{
		case CParameter::AM_MONO:
			iNoChannelsAAC = 1;

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
			iNoChannelsAAC = 1;
			iDRMchanMode = DRMCH_SBR_LC_STEREO;
			break;

		case CParameter::AM_STEREO:
			iNoChannelsAAC = 2;

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
		{
			iAudioPayloadLen = 0;
			DoNotProcessData = TRUE;
		}

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

		iResOutBlockSize =
			iLenDecOutPerChan * SOUNDCRD_SAMPLE_RATE / iAudioSampleRate;

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
			SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);
		ResampleObjR.Init(iLenDecOutPerChan,
			SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);


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

		/* Init storage for CRCs, frame lengths and borders */
		aac_crc_bits.Init(iNumAACFrames);
		veciFrameLength.Init(iNumAACFrames);
		veciBorders.Init(iNumAACFrames + 1);

		/* Init AAC-decoder */
		faacDecInitDRM(HandleAACDecoder, iAACSampleRate, iDRMchanMode);

		/* With this parameter we define the maximum lenght of the output
		   buffer. The cyclic buffer is only needed if we do a sample rate
		   correction due to a difference compared to the transmitter. But for
		   now we do not correct and we could stay with a single buffer
		   Maybe TODO: sample rate correction to avoid audio dropouts */
		iMaxOutputBlockSize = iMaxLenResamplerOutput;
	}
	else
		DoNotProcessData = TRUE;

	/* Define input block size of this module */
	iInputBlockSize = iTotalNumInputBits;

	/* Init output block size for the first time. This parameter is set in the
	   processing routine, too */
	iOutputBlockSize = 0;
}

CAudioSourceDecoder::CAudioSourceDecoder()
{
	/* Open AACEncoder instance */
	HandleAACDecoder = faacDecOpen();

	/* Decoder MUST be initialized at least once, therefore do it here in the
	   constructor with arbitrary values to be sure that this is satisfied */
	faacDecInitDRM(HandleAACDecoder, 24000, DRMCH_MONO);
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
	/* Close decoder handle */
	faacDecClose(HandleAACDecoder);
}
