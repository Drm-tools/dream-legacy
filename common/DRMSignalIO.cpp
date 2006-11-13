/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Cesco (HB9TLK)
 *
 * Description:
 *	Transmit and receive data
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

#include "DRMSignalIO.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Transmitter                                                                  *
\******************************************************************************/
void CTransmitData::ProcessDataInternal(CParameter& Parameter)
{
	int i;

	/* Apply bandpass filter */
	BPFilter.Process(*pvecInputData);

	/* Convert vector type. Fill vector with symbols (collect them) */
	const int iNs2 = iInputBlockSize * 2;
	for (i = 0; i < iNs2; i += 2)
	{
		const int iCurIndex = iBlockCnt * iNs2 + i;

		/* Imaginary, real */
		const short sCurOutReal =
			(short) ((*pvecInputData)[i / 2].real() * rNormFactor);
		const short sCurOutImag =
			(short) ((*pvecInputData)[i / 2].imag() * rNormFactor);

		/* Envelope, phase */
		const short sCurOutEnv =
			(short) (Abs((*pvecInputData)[i / 2]) * (_REAL) 256.0);
		const short sCurOutPhase = /* 2^15 / pi / 2 -> approx. 5000 */
			(short) (Angle((*pvecInputData)[i / 2]) * (_REAL) 5000.0);

		switch (eOutputFormat)
		{
		case OF_REAL_VAL:
			/* Use real valued signal as output for both sound card channels */
			vecsDataOut[iCurIndex] = vecsDataOut[iCurIndex + 1] = sCurOutReal;
			break;

		case OF_IQ_POS:
			/* Send inphase and quadrature (I / Q) signal to stereo sound card
			   output. I: left channel, Q: right channel */
			vecsDataOut[iCurIndex] = sCurOutReal;
			vecsDataOut[iCurIndex + 1] = sCurOutImag;
			break;

		case OF_IQ_NEG:
			/* Send inphase and quadrature (I / Q) signal to stereo sound card
			   output. I: right channel, Q: left channel */
			vecsDataOut[iCurIndex] = sCurOutImag;
			vecsDataOut[iCurIndex + 1] = sCurOutReal;
			break;

		case OF_EP:
			/* Send envelope and phase signal to stereo sound card
			   output. Envelope: left channel, Phase: right channel */
			vecsDataOut[iCurIndex] = sCurOutEnv;
			vecsDataOut[iCurIndex + 1] = sCurOutPhase;
			break;
		}
		if (bUseSoundcard == FALSE)
		{
			/* Write data to file */
#ifdef HAVE_LIBSNDFILE
			if (eOutputFormat==OF_REAL_VAL)
			{
				(void)sf_writef_short(pFile, &vecsDataOut[iCurIndex], 1);
			}
			else
			{
				short buffer[2];
				buffer[0] = vecsDataOut[iCurIndex];
				buffer[1] = vecsDataOut[iCurIndex+1];
				(void)sf_writef_short(pFile, buffer, 1);
			}
#else
			switch(eOutFileMode)
			{
			case OFF_RAW:
				if (eOutputFormat==OF_REAL_VAL)
					fwrite((const void*) &vecsDataOut[iCurIndex], 2, 1, pFile);
				else
				{
					short buffer[2];
					buffer[0] = vecsDataOut[iCurIndex];
					buffer[1] = vecsDataOut[iCurIndex+1];
					fwrite((const void*) buffer, 2, 2, pFile);
				}
				break;
			case OFF_TXT:
				/* This can be read with Matlab "load" command */
				if (eOutputFormat==OF_REAL_VAL)
					fprintf(pFile, "%d\n", vecsDataOut[iCurIndex]);
				else
					fprintf(pFile, "%d\n %d\n", vecsDataOut[iCurIndex],
						vecsDataOut[iCurIndex+1]);
				break;
			case OFF_WAV:
				if (eOutputFormat==OF_REAL_VAL)
					WaveFile.AddStereoSample(vecsDataOut[iCurIndex], 0);
				else
					WaveFile.AddStereoSample(vecsDataOut[iCurIndex],
									vecsDataOut[iCurIndex+1]);
			}
#endif
		}
	}

	iBlockCnt++;
	if (iBlockCnt == iNumBlocks)
	{
		iBlockCnt = 0;

		if (bUseSoundcard == TRUE)
		{
			/* Write data to sound card. Must be a blocking function */
			pSound->Write(vecsDataOut);
		}
		else
		{
			if(eOutFileMode != OFF_WAV)
			{
				/* Flush the file buffer */
#ifdef HAVE_LIBSNDFILE
				sf_write_sync(pFile);
#else
				fflush(pFile);
#endif
			}
		}
	}
}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
	//float*	pCurFilt;
	//int		iNumTapsTransmFilt;
	//CReal	rNormCurFreqOffset;

	const int iSymbolBlockSize = TransmParam.iSymbolBlockSize;

	/* Init vector for storing a complete DRM frame number of OFDM symbols */
	iBlockCnt = 0;
	iNumBlocks = TransmParam.iNumSymPerFrame;
	iBigBlockSize = iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks;

	vecsDataOut.Init(iBigBlockSize);

	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface */
		pSound->Init(iBigBlockSize, TRUE);
	}
	else
	{
		/* Open file for writing data for transmitting */
#ifdef HAVE_LIBSNDFILE
		SF_INFO sfinfo;
		sfinfo.samplerate = 48000;
		sfinfo.channels = (eOutputFormat==OF_REAL_VAL)?1:2;
		switch(eOutFileMode)
		{
		case OFF_RAW:
			sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
			break;
		case OFF_TXT:
			sfinfo.format = SF_FORMAT_MAT4 | SF_FORMAT_PCM_16;
			break;
		case OFF_WAV:
			sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
			break;
		}
	    pFile = sf_open(strFileName.c_str(), SFM_WRITE, &sfinfo);		
#else
		switch(eOutFileMode)
		{
		case OFF_RAW:
			pFile = fopen(strFileName.c_str(), "wb");
			/* Check for error */
			if (pFile == NULL)
				throw CGenErr("The file " + strFileName + " cannot be created.");
			break;
		case OFF_TXT:
			pFile = fopen(strFileName.c_str(), "w");
			/* Check for error */
			if (pFile == NULL)
				throw CGenErr("The file " + strFileName + " cannot be created.");
			break;
		case OFF_WAV:
			WaveFile.Open(strFileName);
		}
#endif
	}


	/* Init bandpass filter object */
	BPFilter.Init(iSymbolBlockSize, rDefCarOffset,
		TransmParam.GetSpectrumOccup(), CDRMBandpassFilt::FT_TRANSMITTER);


	/* All robustness modes and spectrum occupancies should have the same output
	   power. Calculate the normalisation factor based on the average power of
	   symbol (the number 3000 was obtained through output tests) */
	rNormFactor = (CReal) 3000.0 / Sqrt(TransmParam.rAvPowPerSymbol);

	/* Define block-size for input */
	iInputBlockSize = iSymbolBlockSize;
}

void CTransmitData::Stop()
{
	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface */
		pSound->Close();
	}
	else
	{
		/* Close file */
#ifdef HAVE_LIBSNDFILE
		if (pFile != NULL)
			sf_close(pFile);
#else
		if(eOutFileMode==OFF_WAV)
			WaveFile.Close();
		else
			if (pFile != NULL)
				fclose(pFile);
#endif
	}
}

CTransmitData::~CTransmitData()
{
	Stop();
}


/******************************************************************************\
* Receive data from the sound card                                             *
\******************************************************************************/
void CReceiveData::ProcessDataInternal(CParameter& Parameter)
{
	int i;

	if (bUseSoundcard == TRUE)
	{
		/* Using sound card ------------------------------------------------- */
		/* Get data from sound interface. The read function must be a
		   blocking function! */
		if (pSound->Read(vecsSoundBuffer) == FALSE)
			Parameter.ReceiveStatus.SetInterfaceStatus(RX_OK);
		else
			Parameter.ReceiveStatus.SetInterfaceStatus(CRC_ERROR);
	}
	else
	{
		/* Read data from file ---------------------------------------------- */
#ifdef HAVE_LIBSNDFILE
		int channels = (eInChanSelection==CS_MONO)?1:2;
		int iSamples = iOutputBlockSize*channels;
		short *buffer = new short[iSamples];
		sf_count_t c = sf_readf_short(pFile, buffer, iOutputBlockSize);
		if(c==iOutputBlockSize)
		{
			for (i = 0; i < iSamples; i++)
				vecsSoundBuffer[i] = buffer[i];
		}
		else
		{
		   	sf_close(pFile);
		   	pFile=NULL;
			Parameter.bRunThread = FALSE;
			/* Set output block size to zero to avoid writing invalid data */
			iOutputBlockSize = 0;
			delete[] buffer;
			return;
		}
		delete[] buffer;
#else
		for (i = 0; i < iOutputBlockSize; i++)
		{
# ifdef FILE_DRM_USING_RAW_DATA
			short n;
			/* Read 2 bytes, 1 piece */
			if (fread((void*) &n, size_t(2), size_t(1), pFile) == size_t(0))
# else
			float n;
			if (fscanf(pFile, "%e\n", &n) == EOF)
# endif
			{
			   	fclose(pFile);
			   	pFile=NULL;
				Parameter.bRunThread = FALSE;
				/* Set output block size to zero to avoid writing invalid
				   data */
				iOutputBlockSize = 0;
				return;	
			}
			else
			{
				/* Write internal output buffer */
				(*pvecOutputData)[i] = (_REAL) n;
			}
		}
#endif
	}
	/* Write data to output buffer. Do not set the switch command inside
		   the for-loop for efficiency reasons */
	switch (eInChanSelection)
	{
	case CS_LEFT_CHAN:
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[2 * i];
		break;
	case CS_RIGHT_CHAN:
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[2 * i + 1];
		break;
	case CS_MIX_CHAN:
		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* Mix left and right channel together */
			const _REAL rLeftChan = vecsSoundBuffer[2 * i];
			const _REAL rRightChan = vecsSoundBuffer[2 * i + 1];
				(*pvecOutputData)[i] = (rLeftChan + rRightChan) / 2;
		}
		break;
		/* I / Q input */
	case CS_IQ_POS:
		for (i = 0; i < iOutputBlockSize; i++)
		{
			(*pvecOutputData)[i] =
				HilbertFilt((_REAL) vecsSoundBuffer[2 * i],
				(_REAL) vecsSoundBuffer[2 * i + 1]);
		}
		break;
	case CS_IQ_NEG:
		for (i = 0; i < iOutputBlockSize; i++)
		{
			(*pvecOutputData)[i] =
				HilbertFilt((_REAL) vecsSoundBuffer[2 * i + 1],
				(_REAL) vecsSoundBuffer[2 * i]);
		}
		break;

	case CS_IQ_POS_ZERO:
		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* Shift signal to vitual intermediate frequency before applying
			   the Hilbert filtering */
			_COMPLEX cCurSig = _COMPLEX((_REAL) vecsSoundBuffer[2 * i],
				(_REAL) vecsSoundBuffer[2 * i + 1]);
				cCurSig *= cCurExp;
				/* Rotate exp-pointer on step further by complex multiplication
			   with precalculated rotation vector cExpStep */
			cCurExp *= cExpStep;
				(*pvecOutputData)[i] =
				HilbertFilt(cCurSig.real(), cCurSig.imag());
		}
		break;

	case CS_IQ_NEG_ZERO:
		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* Shift signal to virtual intermediate frequency before applying
			   the Hilbert filtering */
			_COMPLEX cCurSig = _COMPLEX((_REAL) vecsSoundBuffer[2 * i + 1],
				(_REAL) vecsSoundBuffer[2 * i]);
				cCurSig *= cCurExp;
				/* Rotate exp-pointer one step further by complex multiplication
			   with precalculated rotation vector cExpStep */
			cCurExp *= cExpStep;
				(*pvecOutputData)[i] =
				HilbertFilt(cCurSig.real(), cCurSig.imag());
		}
		break;

	case CS_MONO: /* Only for file input !!! */
#ifdef HAVE_LIBSNDFILE
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[i];
#endif
		break;
	}
	/* Flip spectrum if necessary ------------------------------------------- */
	if (bFippedSpectrum == TRUE)
	{
		static _BOOLEAN bFlagInv = FALSE;

		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* We flip the spectrum by using the mirror spectrum at the negative
			   frequencys. If we shift by half of the sample frequency, we can
			   do the shift without the need of a Hilbert transformation */
			if (bFlagInv == FALSE)
			{
				(*pvecOutputData)[i] = -(*pvecOutputData)[i];
				bFlagInv = TRUE;
			}
			else
				bFlagInv = FALSE;
		}
	}

	/* Copy data in buffer for spectrum calculation */
	vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);

	/* Update level meter */
	SignalLevelMeter.Update((*pvecOutputData));
}

void CReceiveData::InitInternal(CParameter& Parameter)
{
	/* Check if "new" flag for sound card usage has changed */
	if (bNewUseSoundcard != bUseSoundcard)
		bUseSoundcard = bNewUseSoundcard;

	/* Init buffer size for taking stereo input */
	vecsSoundBuffer.Init(Parameter.iSymbolBlockSize * 2);

	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface. Set it to one symbol. The sound card interface
		   has to taken care about the buffering data of a whole MSC block.
		   Use stereo input (* 2) */
		pSound->Init(Parameter.iSymbolBlockSize * 2);

	}
	else
	{
		/* Open file for reading data from transmitter. Open file only once */
		if (pFile == NULL)
		{
#ifdef HAVE_LIBSNDFILE
			SF_INFO sfinfo;
			memset(&sfinfo, 0, sizeof(SF_INFO));
			pFile = sf_open(strInFileName.c_str(), SFM_READ, &sfinfo);
			if(sfinfo.channels==1)
				eInChanSelection = CS_MONO;
#else
# ifdef FILE_DRM_USING_RAW_DATA
			pFile = fopen(strInFileName.c_str(), "rb");
# else
			pFile = fopen(strInFileName.c_str(), "r");
# endif
			eInChanSelection = CS_MONO;
#endif
		}

		/* Check for error */
		if (pFile == NULL)
			throw CGenErr("The file " + strInFileName + " must exist.");
	}

	/* Init signal meter */
	SignalLevelMeter.Init(0);

	/* Inits for I / Q input */
	vecrReHist.Init(NUM_TAPS_IQ_INPUT_FILT, (_REAL) 0.0);
	vecrImHist.Init(NUM_TAPS_IQ_INPUT_FILT, (_REAL) 0.0);

	/* Start with phase null (can be arbitrarily chosen) */
	cCurExp = (_REAL) 1.0;

	/* Set rotation vector to mix signal from zero frequency to virtual
	   intermediate frequency */
	const _REAL rNormCurFreqOffsetIQ =
		(_REAL) 2.0 * crPi * ((_REAL) VIRTUAL_INTERMED_FREQ / SOUNDCRD_SAMPLE_RATE);

	cExpStep = _COMPLEX(cos(rNormCurFreqOffsetIQ), sin(rNormCurFreqOffsetIQ));


	/* Define output block-size */
	iOutputBlockSize = Parameter.iSymbolBlockSize;
}

_REAL CReceiveData::HilbertFilt(const _REAL rRe, const _REAL rIm)
{
/*
	Hilbert filter for I / Q input data. This code is based on code written
	by Cesco (HB9TLK)
*/
    int i;

	/* Move old data */
    for (i = 0; i < NUM_TAPS_IQ_INPUT_FILT - 1; i++)
	{
		vecrReHist[i] = vecrReHist[i + 1];
		vecrImHist[i] = vecrImHist[i + 1];
	}

    vecrReHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rRe;
    vecrImHist[NUM_TAPS_IQ_INPUT_FILT - 1] = rIm;

	/* Filter */
    _REAL rSum = (_REAL) 0.0;
    for (i = 1; i < NUM_TAPS_IQ_INPUT_FILT; i += 2)
		rSum += fHilFiltIQ[i] * vecrImHist[i];

	return (rSum + vecrReHist[IQ_INP_HIL_FILT_DELAY]) / 2;
}

CReceiveData::~CReceiveData()
{
	/* Close file (if opened) */
	if (pFile != NULL)
#ifdef HAVE_LIBSNDFILE
		sf_close(pFile);
#else
		fclose(pFile);
#endif
}

void CReceiveData::GetInputSpec(CVector<_REAL>& vecrData,
								CVector<_REAL>& vecrScale)
{
	int i;

	/* Length of spectrum vector including Nyquist frequency */
	const int iLenSpecWithNyFreq = NUM_SMPLS_4_INPUT_SPECTRUM / 2 + 1;

	/* Init input and output vectors */
	vecrData.Init(iLenSpecWithNyFreq, (_REAL) 0.0);
	vecrScale.Init(iLenSpecWithNyFreq, (_REAL) 0.0);

	/* Lock resources */
	Lock();

	/* Init the constants for scale and normalization */
	const _REAL rFactorScale =
		(_REAL) SOUNDCRD_SAMPLE_RATE / iLenSpecWithNyFreq / 2000;

	const _REAL rNormData = (_REAL) _MAXSHORT * _MAXSHORT *
		NUM_SMPLS_4_INPUT_SPECTRUM * NUM_SMPLS_4_INPUT_SPECTRUM;

	/* Copy data from shift register in Matlib vector */
	CRealVector vecrFFTInput(NUM_SMPLS_4_INPUT_SPECTRUM);
	for (i = 0; i < NUM_SMPLS_4_INPUT_SPECTRUM; i++)
		vecrFFTInput[i] = vecrInpData[i];

	/* Get squared magnitude of spectrum */
	CRealVector vecrSqMagSpect(iLenSpecWithNyFreq);
	vecrSqMagSpect =
		SqMag(rfft(vecrFFTInput * Hann(NUM_SMPLS_4_INPUT_SPECTRUM)));

	/* Log power spectrum data */
	for (i = 0; i < iLenSpecWithNyFreq; i++)
	{
		const _REAL rNormSqMag = vecrSqMagSpect[i] / rNormData;

		if (rNormSqMag > 0)
			vecrData[i] = (_REAL) 10.0 * log10(rNormSqMag);
		else
			vecrData[i] = RET_VAL_LOG_0;

		vecrScale[i] = (_REAL) i * rFactorScale;
	}

	/* Release resources */
	Unlock();
}

void CReceiveData::GetInputPSD(CVector<_REAL>& vecrData,
							   CVector<_REAL>& vecrScale)
{
	int i;

	/* Length of spectrum vector including Nyquist frequency */
	const int iLenSpecWithNyFreq = LEN_PSD_AV_EACH_BLOCK / 2 + 1;

	/* Init input and output vectors */
	vecrData.Init(iLenSpecWithNyFreq, (_REAL) 0.0);
	vecrScale.Init(iLenSpecWithNyFreq, (_REAL) 0.0);

	/* Lock resources */
	Lock();

	/* Init the constants for scale and normalization */
	const _REAL rFactorScale =
		(_REAL) SOUNDCRD_SAMPLE_RATE / iLenSpecWithNyFreq / 2000;

	const _REAL rNormData = (_REAL) _MAXSHORT * _MAXSHORT *
		LEN_PSD_AV_EACH_BLOCK * LEN_PSD_AV_EACH_BLOCK *
		NUM_AV_BLOCKS_PSD * NUM_AV_BLOCKS_PSD;

	/* Init intermediate vectors */
	CRealVector vecrAvSqMagSpect(iLenSpecWithNyFreq, (CReal) 0.0);
	CRealVector vecrFFTInput(LEN_PSD_AV_EACH_BLOCK);

	/* Init Hamming window */
	CRealVector vecrHammWin(Hamming(LEN_PSD_AV_EACH_BLOCK));

	/* Calculate FFT of each small block and average results (estimation
	   of PSD of input signal) */
	for (int j = 0; j < NUM_AV_BLOCKS_PSD; j++)
	{
		/* Copy data from shift register in Matlib vector */
		for (i = 0; i < LEN_PSD_AV_EACH_BLOCK; i++)
			vecrFFTInput[i] = vecrInpData[i + j * LEN_PSD_AV_EACH_BLOCK];

		/* Apply Hamming window */
		vecrFFTInput *= vecrHammWin;

		/* Calculate squared magnitude of spectrum and average results */
		vecrAvSqMagSpect += SqMag(rfft(vecrFFTInput));
	}

	/* Log power spectrum data */
	for (i = 0; i < iLenSpecWithNyFreq; i++)
	{
		const _REAL rNormSqMag = vecrAvSqMagSpect[i] / rNormData;

		if (rNormSqMag > 0)
			vecrData[i] = (_REAL) 10.0 * log10(rNormSqMag);
		else
			vecrData[i] = RET_VAL_LOG_0;

		vecrScale[i] = (_REAL) i * rFactorScale;
	}

	/* Release resources */
	Unlock();
}
