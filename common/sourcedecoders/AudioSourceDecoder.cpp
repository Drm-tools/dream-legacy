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
#include <iostream>


/* Definitions ****************************************************************/

// TEST -> for now only use CRC calculation
#define USE_CELP_DECODER



/* Elements of table for UEP parameters for CELP:
   - CELP bit rate index
   - Bit rate (bits / s)
   - Audio frame length (ms)
   - Higher protected part (bits/audio frame)
   - Lower protected part (bits/audio frame)
   - Higher protected part (bytes/audio super frame)
   - Lower protected part (bytes/audio super frame)
   - Audio super frame length (bytes) */
/* 8 kHz sampling rate */
#define LEN_CELP_8KHZ_UEP_PARAMS_TAB			27
const int iTableCELP8kHzUEPParams[LEN_CELP_8KHZ_UEP_PARAMS_TAB][8] = {
	{ 3850, 40, 36, 118,  45, 148, 193},
	{ 4250, 40, 36, 134,  45, 168, 213},
	{ 4650, 40, 36, 150,  45, 188, 233},
	{ 5700, 20, 24,  90,  60, 225, 285},
	{ 6000, 20, 24,  96,  60, 240, 300},
	{ 6300, 20, 24, 102,  60, 255, 315},
	{ 6600, 20, 24, 108,  60, 270, 330},
	{ 6900, 20, 24, 114,  60, 285, 345},
	{ 7100, 20, 24, 118,  60, 295, 355},
	{ 7300, 20, 24, 122,  60, 305, 365},
	{ 7700, 20, 36, 118,  90, 295, 385},
	{ 8300, 20, 36, 130,  90, 325, 415},
	{ 8700, 20, 36, 138,  90, 345, 435},
	{ 9100, 20, 36, 146,  90, 365, 455},
	{ 9500, 20, 36, 154,  90, 385, 475},
	{ 9900, 20, 36, 162,  90, 405, 495},
	{10300, 20, 36, 170,  90, 425, 515},
	{10500, 20, 36, 174,  90, 435, 525},
	{10700, 20, 36, 178,  90, 445, 535},
	{11000, 10, 24,  86, 120, 430, 550},
	{11400, 10, 24,  90, 120, 450, 570},
	{11800, 10, 24,  94, 120, 470, 590},
	{12000, 10, 24,  96, 120, 480, 600},
	{12200, 10, 24,  98, 120, 490, 610}
};

/* 16 kHz sampling rate */
#define LEN_CELP_16KHZ_UEP_PARAMS_TAB			32
const int iTableCELP16kHzUEPParams[LEN_CELP_16KHZ_UEP_PARAMS_TAB][8] = {
	{10900, 20, 64, 154, 160, 385, 545},
	{11500, 20, 64, 166, 160, 415, 575},
	{12100, 20, 64, 178, 160, 445, 605},
	{12700, 20, 64, 190, 160, 475, 635},
	{13300, 20, 64, 202, 160, 505, 665},
	{13900, 20, 64, 214, 160, 535, 695},
	{14300, 20, 64, 222, 160, 555, 715},
	{14700, 20, 92, 202, 230, 505, 735},
	{15900, 20, 92, 226, 230, 565, 795},
	{17100, 20, 92, 250, 230, 625, 855},
	{17900, 20, 92, 266, 230, 665, 895},
	{18700, 20, 92, 282, 230, 705, 935},
	{19500, 20, 92, 298, 230, 745, 975},
	{20300, 20, 92, 314, 230, 785, 1015},
	{21100, 20, 92, 330, 230, 825, 1055},
	{13600, 10, 50,  86, 250, 430, 680},
	{14200, 10, 50,  92, 250, 460, 710},
	{14800, 10, 50,  98, 250, 490, 740},
	{15400, 10, 50, 104, 250, 520, 770},
	{16000, 10, 50, 110, 250, 550, 800},
	{16600, 10, 50, 116, 250, 580, 830},
	{17000, 10, 50, 120, 250, 600, 850},
	{17400, 10, 64, 110, 320, 550, 870},
	{18600, 10, 64, 122, 320, 610, 930},
	{19800, 10, 64, 134, 320, 670, 990},
	{20600, 10, 64, 142, 320, 710, 1030},
	{21400, 10, 64, 150, 320, 750, 1070},
	{22200, 10, 64, 158, 320, 790, 1110},
	{23000, 10, 64, 166, 320, 830, 1150},
	{23800, 10, 64, 174, 320, 870, 1190}
};

/* Implementation ****************************************************************/

void CAudioSourceDecoder::ProcessDataInternal(CParameter& ReceiverParam)
{
	int					i, j;
	_BOOLEAN			bCurBlockOK;
	_BOOLEAN			bGoodValues;

#ifdef USE_FAAD2_LIBRARY
	faacDecFrameInfo	DecFrameInfo;
	short*				psDecOutSampleBuf;
#endif

	bGoodValues = FALSE;

	ReceiverParam.vecbiAudioFrameStatus.Init(0);
	ReceiverParam.vecbiAudioFrameStatus.ResetBitAccess();

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
	{
		return;
	}


	/* Text Message ***********************************************************/
	/* Total frame size depends on whether text message is used or not */
	if (bTextMessageUsed == TRUE)
	{
		/* Decode last for bytes of input block for text message */
		for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
			vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

		TextMessage.Decode(vecbiTextMessBuf);
	}


	/* Audio data header parsing **********************************************/
	/* Check if audio shall not be decoded */
	if (DoNotProcessAudDecoder == TRUE)
	{
		return;
	}
	
	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();


	/* Check which audio coding type is used */
	if (eAudioCoding == CParameter::AC_AAC)
	{
#ifdef USE_FAAD2_LIBRARY
		/* AAC super-frame-header ------------------------------------------- */
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
		for (i = 0; i < iNumAudioFrames; i++)
		{
			if ((veciFrameLength[i] < 0) ||
				(veciFrameLength[i] > iMaxLenOneAudFrame))
			{
				bGoodValues = FALSE;
			}
		}

		if (bGoodValues == TRUE)
		{
			/* Higher-protected part */
			for (i = 0; i < iNumAudioFrames; i++)
			{
				/* Extract higher protected part bytes (8 bits per byte) */
				for (j = 0; j < iNumHigherProtectedBytes; j++)
					audio_frame[i][j] = (*pvecInputData).Separate(8);

				/* Extract CRC bits (8 bits) */
				aac_crc_bits[i] = (*pvecInputData).Separate(8);
			}

			/* Lower-protected part */
			for (i = 0; i < iNumAudioFrames; i++)
			{
				/* First calculate frame length, derived from higher protected
				   part frame length and total size */
				const int iNumLowerProtectedBytes =
					veciFrameLength[i] - iNumHigherProtectedBytes;

				/* Extract lower protected part bytes (8 bits per byte) */
				for (j = 0; j < iNumLowerProtectedBytes; j++)
				{
					audio_frame[i][iNumHigherProtectedBytes + j] =
						(*pvecInputData).Separate(8);
				}
			}
		}
#endif
	}
	else if (eAudioCoding == CParameter::AC_CELP)
	{
		/* celp_super_frame(celp_table_ind) --------------------------------- */
		/* Higher-protected part */
		for (i = 0; i < iNumAudioFrames; i++)
		{
			celp_frame[i].ResetBitAccess();

			/* Extract higher protected part bits */
			for (j = 0; j < iNumHigherProtectedBits; j++)
				celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);

			/* Extract CRC bits (8 bits) if used */
			if (bCELPCRC == TRUE)
				celp_crc_bits[i] = (*pvecInputData).Separate(8);
		}

		/* Lower-protected part */
		for (i = 0; i < iNumAudioFrames; i++)
		{
			for (j = 0; j < iNumLowerProtectedBits; j++)
				celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
		}
	}


	/* Audio decoding *********************************************************/
	/* Init output block size to zero, this variable is also used for
	   determining the position for writing the output vector */
	iOutputBlockSize = 0;

	for (j = 0; j < iNumAudioFrames; j++)
	{
		if (eAudioCoding == CParameter::AC_AAC)
		{
#ifdef USE_FAAD2_LIBRARY
			if (bGoodValues == TRUE)
			{
				/* Prepare data vector with CRC at the beginning (the definition
				   with faad2 DRM interface) */
				vecbyPrepAudioFrame[0] = aac_crc_bits[j];

				for (i = 0; i < veciFrameLength[j]; i++)
					vecbyPrepAudioFrame[i + 1] = audio_frame[j][i];

#if 0
// Store AAC-data in file
string strAACTestFileName = "test/aac_";
if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
	AudioParam.eAudioSamplRate == CParameter::AS_12KHZ)
{
	strAACTestFileName += "12kHz_";
}
else
	strAACTestFileName += "24kHz_";

switch (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
	AudioParam.eAudioMode)
{
case CParameter::AM_MONO:
	strAACTestFileName += "mono";
	break;

case CParameter::AM_P_STEREO:
	strAACTestFileName += "pstereo";
	break;

case CParameter::AM_STEREO:
	strAACTestFileName += "stereo";
	break;
}

if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].AudioParam.
	eSBRFlag == CParameter::SB_USED)
{
	strAACTestFileName += "_sbr";
}
strAACTestFileName += ".dat";
static FILE* pFile2 = fopen(strAACTestFileName.c_str(), "wb");
int iNewFrL = veciFrameLength[j] + 1;
fwrite((void*) &iNewFrL, size_t(4), size_t(1), pFile2); // frame length
fwrite((void*) &vecbyPrepAudioFrame[0], size_t(1), size_t(iNewFrL), pFile2); // data
fflush(pFile2);
#endif

				/* Call decoder routine */
				psDecOutSampleBuf = (short*) NeAACDecDecode(HandleAACDecoder,
					&DecFrameInfo, &vecbyPrepAudioFrame[0],
					veciFrameLength[j] + 1);

				/* OPH: add frame status to vector for RSCI */
				ReceiverParam.vecbiAudioFrameStatus.Add(DecFrameInfo.error==0 ? 0 : 1);
				if (DecFrameInfo.error != 0)

					bCurBlockOK = FALSE; /* Set error flag */
				else
				{
					bCurBlockOK = TRUE;

					/* Conversion from _SAMPLE vector to _REAL vector for
					   resampling. ATTENTION: We use a vector which was
					   allocated inside the AAC decoder! */
					if (DecFrameInfo.channels == 1)
					{
						/* Change type of data (short -> real) */
						for (i = 0; i < iLenDecOutPerChan; i++)
							vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

						/* Resample data */
						ResampleObjL.Resample(vecTempResBufInLeft,
							vecTempResBufOutCurLeft);

						/* Mono (write the same audio material in both
						   channels) */
						for (i = 0; i < iResOutBlockSize; i++)
						{
							vecTempResBufOutCurRight[i] =
								vecTempResBufOutCurLeft[i];
						}
					}
					else
					{
						/* Stereo */
						for (i = 0; i < iLenDecOutPerChan; i++)
						{
							vecTempResBufInLeft[i] =
								psDecOutSampleBuf[i * 2];
							vecTempResBufInRight[i] =
								psDecOutSampleBuf[i * 2 + 1];
						}

						/* Resample data */
						ResampleObjL.Resample(vecTempResBufInLeft,
							vecTempResBufOutCurLeft);
						ResampleObjR.Resample(vecTempResBufInRight,
							vecTempResBufOutCurRight);
					}
				}
			}
			else
			{
				/* DRM AAC header was wrong, set flag to "bad block" */ 
				bCurBlockOK = FALSE;
				/* OPH: update audio status vector for RSCI */	
				ReceiverParam.vecbiAudioFrameStatus.Add(1);
			}
#endif
	}
	else if (eAudioCoding == CParameter::AC_CELP)
	{
		if (bCELPCRC == TRUE)
		{
			/* Prepare CRC object and data stream */
			CELPCRCObject.Reset(8);
			celp_frame[j].ResetBitAccess();

			for (i = 0; i < iNumHigherProtectedBits; i++)
				CELPCRCObject.AddBit((_BINARY) celp_frame[j].Separate(1));

			bCurBlockOK = CELPCRCObject.CheckCRC(celp_crc_bits[j]);
		}
		else
			bCurBlockOK = TRUE;
			
		/* OPH: update audio status vector for RSCI */	
        ReceiverParam.vecbiAudioFrameStatus.Add(bCurBlockOK == TRUE ? 0 : 1);

#if 0
// Store CELP-data in file
char cDummy[200];
string strCELPTestFileName = "test/celp_";
if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
	AudioParam.eAudioSamplRate == CParameter::AS_8_KHZ)
{
	strCELPTestFileName += "8kHz_";
	strCELPTestFileName += _itoa(iTableCELP8kHzUEPParams[ReceiverParam.Service[
		ReceiverParam.GetCurSelAudioService()].AudioParam.iCELPIndex][0], cDummy, 10);
}
else
{
	strCELPTestFileName += "16kHz_";
	strCELPTestFileName += _itoa(iTableCELP16kHzUEPParams[ReceiverParam.Service[
		ReceiverParam.GetCurSelAudioService()].AudioParam.iCELPIndex][0], cDummy, 10);
}
strCELPTestFileName += "bps";

if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].AudioParam.
	eSBRFlag == CParameter::SB_USED)
{
	strCELPTestFileName += "_sbr";
}
strCELPTestFileName += ".dat";

static FILE* pFile2 = fopen(strCELPTestFileName.c_str(), "wb");
int iTotNumBits = iNumHigherProtectedBits + iNumLowerProtectedBits;
int iNewFrL = (int) Ceil((CReal) iTotNumBits / 8);
fwrite((void*) &iNewFrL, size_t(4), size_t(1), pFile2); // frame length
celp_frame[j].ResetBitAccess();
for (i = 0; i < iNewFrL; i++)
{
	int iNumBits = Min(iTotNumBits - i * 8, 8);
	_BYTE bCurVal = (_BYTE) celp_frame[j].Separate(iNumBits);
	fwrite((void*) &bCurVal, size_t(1), size_t(1), pFile2); // data
}
fflush(pFile2);
#endif



#ifdef USE_CELP_DECODER

/* Write zeros in current output buffer since we do not have a decoder */
for (i = 0; i < iResOutBlockSize; i++)
{
	vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
	vecTempResBufOutCurRight[i] = (_REAL) 0.0;
}

#endif

	}
	else
		bCurBlockOK = FALSE;





// This code is independent of particular audio source type and should work
// fine with CELP and HVXC

		/* Postprocessing of audio blocks, status informations -------------- */
		if (bCurBlockOK == FALSE)
		{
			/* Set AAC CRC result in log file */
			ReceiverParam.ReceptLog.SetMSC(FALSE);

			if (bAudioWasOK == TRUE)
			{
				/* Post message to show that CRC was wrong (yellow light) */
				//PostWinMessage(MS_MSC_CRC, 1);
				ReceiverParam.ReceiveStatus.SetAudioStatus(DATA_ERROR);

				/* Fade-out old block to avoid "clicks" in audio. We use linear
				   fading which gives a log-fading impression */
				for (i = 0; i < iResOutBlockSize; i++)
				{
					/* Linear attenuation with time of OLD buffer */
					const _REAL rAtt =
						(_REAL) 1.0 - (_REAL) i / iResOutBlockSize;

					vecTempResBufOutOldLeft[i] *= rAtt;
					vecTempResBufOutOldRight[i] *= rAtt;

					if (bUseReverbEffect == TRUE)
					{
						/* Fade in input signal for reverberation to avoid
						   clicks */
						const _REAL rAttRev = (_REAL) i / iResOutBlockSize;

						/* Cross-fade reverberation effect */
						const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
							ProcessSample(vecTempResBufOutOldLeft[i] * rAttRev,
							vecTempResBufOutOldRight[i] * rAttRev);

						/* Mono reverbration signal */
						vecTempResBufOutOldLeft[i] += rRevSam;
						vecTempResBufOutOldRight[i] += rRevSam;
					}
				}

				/* Set flag to show that audio block was bad */
				bAudioWasOK = FALSE;
			}
			else
			{
				/* Post message to show that CRC was wrong (red light) */
				//PostWinMessage(MS_MSC_CRC, 2);
				ReceiverParam.ReceiveStatus.SetAudioStatus(CRC_ERROR);

				if (bUseReverbEffect == TRUE)
				{
					/* Add Reverberation effect */
					for (i = 0; i < iResOutBlockSize; i++)
					{
						/* Mono reverberation signal */
						vecTempResBufOutOldLeft[i] =
							vecTempResBufOutOldRight[i] = AudioRev.
							ProcessSample(0, 0);
					}
				}
			}

			/* Write zeros in current output buffer */
			for (i = 0; i < iResOutBlockSize; i++)
			{
				vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
				vecTempResBufOutCurRight[i] = (_REAL) 0.0;
			}
		}
		else
		{
			/* Set AAC CRC result in log file */
			ReceiverParam.ReceptLog.SetMSC(TRUE);

			/* Increment correctly decoded audio blocks counter */
			iNumCorDecAudio++;

			/* Post message to show that CRC was OK */
			//PostWinMessage(MS_MSC_CRC, 0);
			ReceiverParam.ReceiveStatus.SetAudioStatus(RX_OK);

			if (bAudioWasOK == FALSE)
			{
				if (bUseReverbEffect == TRUE)
				{
					/* Add "last" reverbration only to old block */
					for (i = 0; i < iResOutBlockSize; i++)
					{
						/* Mono reverberation signal */
						vecTempResBufOutOldLeft[i] =
							vecTempResBufOutOldRight[i] = AudioRev.
							ProcessSample(vecTempResBufOutOldLeft[i],
							vecTempResBufOutOldRight[i]);
					}
				}

				/* Fade-in new block to avoid "clicks" in audio. We use linear
				   fading which gives a log-fading impression */
				for (i = 0; i < iResOutBlockSize; i++)
				{
					/* Linear attenuation with time */
					const _REAL rAtt = (_REAL) i / iResOutBlockSize;

					vecTempResBufOutCurLeft[i] *= rAtt;
					vecTempResBufOutCurRight[i] *= rAtt;

					if (bUseReverbEffect == TRUE)
					{
						/* Cross-fade reverberation effect */
						const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
							ProcessSample(0, 0);

						/* Mono reverberation signal */
						vecTempResBufOutCurLeft[i] += rRevSam;
						vecTempResBufOutCurRight[i] += rRevSam;
					}
				}

				/* Reset flag */
				bAudioWasOK = TRUE;
			}
		}

		/* Conversion from _REAL to _SAMPLE with special function */
		for (i = 0; i < iResOutBlockSize; i++)
		{
			(*pvecOutputData)[iOutputBlockSize + i * 2] = 
				Real2Sample(vecTempResBufOutOldLeft[i]); /* Left channel */
			(*pvecOutputData)[iOutputBlockSize + i * 2 + 1] =
				Real2Sample(vecTempResBufOutOldRight[i]); /* Right channel */
		}

		/* Add new block to output block size ("* 2" for stereo output block) */
		iOutputBlockSize += iResOutBlockSize * 2;

		/* Store current audio block */
		for (i = 0; i < iResOutBlockSize; i++)
		{
			vecTempResBufOutOldLeft[i] = vecTempResBufOutCurLeft[i];
			vecTempResBufOutOldRight[i] = vecTempResBufOutCurRight[i];
		}
	}
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
	int iAudioSampleRate;

	/* Init error flags and output block size parameter. The output block
	   size is set in the processing routine. We must set it here in case
	   of an error in the initialization, this part in the processing
	   routine is not being called */
	DoNotProcessAudDecoder = FALSE;
	DoNotProcessData = FALSE;
	iOutputBlockSize = 0;

	try
	{
		/* Init counter for correctly decoded audio blocks */
		iNumCorDecAudio = 0;

		/* Init "audio was ok" flag */
		bAudioWasOK = TRUE;

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
				SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

			/* Init vector for text message bytes */
			vecbiTextMessBuf.Init(SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR);
			break;

		case FALSE:
			bTextMessageUsed = FALSE;

			/* All bytes are used for AAC data, no text message present */
			iTotalFrameSize = iInputBlockSize;
			break;
		}

		/* Get audio coding type */
		eAudioCoding =
			ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding;


		if (eAudioCoding == CParameter::AC_AAC)
		{
#ifdef USE_FAAD2_LIBRARY
			/* Init for AAC decoding ---------------------------------------- */
			int iAACSampleRate, iNumHeaderBytes, iDRMchanMode;

			/* Length of higher protected part of audio stream */
			const int iLenAudHigh =
				ReceiverParam.Stream[iCurAudioStreamID].iLenPartA;

			/* Set number of AAC frames in a AAC super-frame */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.
				eAudioSamplRate) /* Only 12 kHz and 24 kHz is allowed */
			{
			case CParameter::AS_12KHZ:
				iNumAudioFrames = 5;
				iNumHeaderBytes = 6;
				iAACSampleRate = 12000;
				break;

			case CParameter::AS_24KHZ:
				iNumAudioFrames = 10;
				iNumHeaderBytes = 14;
				iAACSampleRate = 24000;
				break;

			default:
				/* Some error occurred, throw error */
				throw CInitErr(ET_AUDDECODER);
				break;
			}

			/* Number of borders */
			iNumBorders = iNumAudioFrames - 1;

			/* Number of channels for AAC: Mono, PStereo, Stereo */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioMode)
			{
			case CParameter::AM_MONO:
				if (ReceiverParam.Service[iCurSelServ].AudioParam.
					eSBRFlag == CParameter::SB_USED)
				{
					iDRMchanMode = DRMCH_SBR_MONO;
				}
				else
					iDRMchanMode = DRMCH_MONO;
				break;

			case CParameter::AM_P_STEREO:
				/* Low-complexity only defined in SBR mode */
				iDRMchanMode = DRMCH_SBR_PS_STEREO;
				break;

			case CParameter::AM_STEREO:
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

			/* In case of SBR, AAC sample rate is half the total sample rate.
			   Length of output is doubled if SBR is used */
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
			   subtracting the audio super frame overhead (bytes used for the
			   audio super frame header() and for the aac_crc_bits)
			   (5.3.1.1, Table 5) */
			iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE -
				iNumHeaderBytes - iNumAudioFrames;

			/* Check iAudioPayloadLen value, only positive values make sense */
			if (iAudioPayloadLen < 0)
				throw CInitErr(ET_AUDDECODER);

			/* Calculate number of bytes for higher protected blocks */
			iNumHigherProtectedBytes = (iLenAudHigh - iNumHeaderBytes -
				iNumAudioFrames /* CRC bytes */) / iNumAudioFrames;

			if (iNumHigherProtectedBytes < 0)
				iNumHigherProtectedBytes = 0;

			/* The maximum length for one audio frame is "iAudioPayloadLen". The
			   regular size will be much shorter since all audio frames share
			   the total size, but we do not know at this time how the data is
			   split in the transmitter source coder */
			iMaxLenOneAudFrame = iAudioPayloadLen;
			audio_frame.Init(iNumAudioFrames, iMaxLenOneAudFrame);

			/* Init vector which stores the data with the CRC at the beginning
			   ("+ 1" for CRC) */
			vecbyPrepAudioFrame.Init(iMaxLenOneAudFrame + 1);

			/* Init storage for CRCs and frame lengths */
			aac_crc_bits.Init(iNumAudioFrames);
			veciFrameLength.Init(iNumAudioFrames);

			/* Init AAC-decoder */
			NeAACDecInitDRM(&HandleAACDecoder, iAACSampleRate, iDRMchanMode);
#else
			/* No AAC decoder available */
			throw CInitErr(ET_AUDDECODER);
#endif
		}
		else if (eAudioCoding == CParameter::AC_CELP)
		{
			/* Init for CELP decoding --------------------------------------- */
			int iCurCelpIdx, iCelpFrameLength;

			/* Set number of AAC frames in a AAC super-frame */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.
				eAudioSamplRate) /* Only 8000 and 16000 is allowed */
			{
			case CParameter::AS_8_KHZ:
				/* Check range */
				iCurCelpIdx =
					ReceiverParam.Service[iCurSelServ].AudioParam.iCELPIndex;

				if ((iCurCelpIdx > 0) &&
					(iCurCelpIdx < LEN_CELP_8KHZ_UEP_PARAMS_TAB))
				{
					/* CELP frame length */
					iCelpFrameLength = iTableCELP8kHzUEPParams[iCurCelpIdx][1];

					/* Number of bits for lower and higher protected parts */
					iNumHigherProtectedBits =
						iTableCELP8kHzUEPParams[iCurCelpIdx][2];
					iNumLowerProtectedBits =
						iTableCELP8kHzUEPParams[iCurCelpIdx][3];
				}
				else
					throw CInitErr(ET_AUDDECODER);

				/* Set audio sample rate */
				iAudioSampleRate = 8000;
				break;

			case CParameter::AS_16KHZ:
				/* Check range */
				iCurCelpIdx =
					ReceiverParam.Service[iCurSelServ].AudioParam.iCELPIndex;

				if ((iCurCelpIdx > 0) &&
					(iCurCelpIdx < LEN_CELP_16KHZ_UEP_PARAMS_TAB))
				{
					/* CELP frame length */
					iCelpFrameLength = iTableCELP16kHzUEPParams[iCurCelpIdx][1];

					/* Number of bits for lower and higher protected parts */
					iNumHigherProtectedBits =
						iTableCELP16kHzUEPParams[iCurCelpIdx][2];
					iNumLowerProtectedBits =
						iTableCELP16kHzUEPParams[iCurCelpIdx][3];
				}
				else
					throw CInitErr(ET_AUDDECODER);

				/* Set audio sample rate */
				iAudioSampleRate = 16000;
				break;

			default:
				/* Some error occurred, throw error */
				throw CInitErr(ET_AUDDECODER);
				break;
			}

			/* Check lengths of iNumHigherProtectedBits and
			   iNumLowerProtectedBits for overrun */
			const int iTotalNumCELPBits =
				iNumHigherProtectedBits + iNumLowerProtectedBits;

			if (iTotalNumCELPBits * SIZEOF__BYTE > iTotalFrameSize )
				throw CInitErr(ET_AUDDECODER);

			/* Calculate number of audio frames (one audio super frame is
			   always 400 ms long) */
			iNumAudioFrames = 400 /* ms */ / iCelpFrameLength /* ms */;

			/* Set CELP CRC flag */
			bCELPCRC = ReceiverParam.Service[iCurSelServ].AudioParam.bCELPCRC;

			/* Init vectors storing the CELP raw data and CRCs */
			celp_frame.Init(iNumAudioFrames, iTotalNumCELPBits);
			celp_crc_bits.Init(iNumAudioFrames);


// TEST
iLenDecOutPerChan = 0;


#ifdef USE_CELP_DECODER

// TODO put decoder initialization here

#else
			/* No CELP decoder available */
			throw CInitErr(ET_AUDDECODER);
#endif
		}
		else
		{
			/* Audio codec not supported */
			throw CInitErr(ET_AUDDECODER);
		}


		/* Set number of AAC frames for log file */
		ReceiverParam.ReceptLog.SetNumAAC(iNumAudioFrames);

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
		vecTempResBufInLeft.Init(iLenDecOutPerChan, (_REAL) 0.0);
		vecTempResBufInRight.Init(iLenDecOutPerChan, (_REAL) 0.0);
		vecTempResBufOutCurLeft.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutCurRight.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutOldLeft.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutOldRight.Init(iResOutBlockSize, (_REAL) 0.0);

		/* Init resample objects */
		ResampleObjL.Init(iLenDecOutPerChan,
			(_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);
		ResampleObjR.Init(iLenDecOutPerChan,
			(_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);

		/* Clear reverberation object */
		AudioRev.Clear();

		/* With this parameter we define the maximum lenght of the output
		   buffer. The cyclic buffer is only needed if we do a sample rate
		   correction due to a difference compared to the transmitter. But for
		   now we do not correct and we could stay with a single buffer
		   Maybe TODO: sample rate correction to avoid audio dropouts */
		iMaxOutputBlockSize = iMaxLenResamplerOutput;
	}

	catch (CInitErr CurErr)
	{
		switch (CurErr.eErrType)
		{
		case ET_ALL:
			/* An init error occurred, do not process data in this module */
			DoNotProcessData = TRUE;
			break;

		case ET_AUDDECODER:
			/* Audio part should not be decdoded, set flag */
			DoNotProcessAudDecoder = TRUE;
			break;

		default:
			DoNotProcessData = TRUE;
		}

		/* In all cases set output size to zero */
		iOutputBlockSize = 0;
	}
}

int CAudioSourceDecoder::GetNumCorDecAudio()
{
	/* Return number of correctly decoded audio blocks. Reset counter
	   afterwards */
	const int iRet = iNumCorDecAudio;

	iNumCorDecAudio = 0;

	return iRet;
}

CAudioSourceDecoder::CAudioSourceDecoder()
#ifdef USE_FAAD2_LIBRARY
	: AudioRev((CReal) 1.0 /* seconds delay */), bUseReverbEffect(TRUE)
#endif
{
#ifdef USE_FAAD2_LIBRARY
	/* Open AACEncoder instance */
	HandleAACDecoder = NeAACDecOpen();

	/* Decoder MUST be initialized at least once, therefore do it here in the
	   constructor with arbitrary values to be sure that this is satisfied */
	NeAACDecInitDRM(&HandleAACDecoder, 24000, DRMCH_MONO);
#endif
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
#ifdef USE_FAAD2_LIBRARY
	/* Close decoder handle */
	NeAACDecClose(HandleAACDecoder);
#endif
}

