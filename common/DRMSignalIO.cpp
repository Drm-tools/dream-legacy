/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

	/* Filtering of output signal (FIR filter) ------------------------------ */
	/* Transfer input data in Matlib library vector */
	for (i = 0; i < iInputBlockSize; i++)
	{
		rvecDataReal[i] = (*pvecInputData)[i].real();
		rvecDataImag[i] = (*pvecInputData)[i].imag();
	}

	/* Actual filter routine (use saved state vector) */
	rvecDataReal = Filter(rvecB, rvecA, rvecDataReal, rvecZReal);

	if (eOutputFormat == OF_IQ)
		rvecDataImag = Filter(rvecB, rvecA, rvecDataImag, rvecZImag);


	/* Convert vector type. Fill vector with symbols (collect them) */
	const int iNs2 = iInputBlockSize * 2;
	for (i = 0; i < iNs2; i += 2)
	{
		const int iCurIndex = iBlockCnt * iNs2 + i;

		/* Imaginary, real */
		const short sCurOutReal = (short) (rvecDataReal[i / 2] * rNormFactor);
		const short sCurOutImag = (short) (rvecDataImag[i / 2] * rNormFactor);

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

		case OF_IQ:
			/* Send inphase and quadrature (I / Q) signal to stereo sound card
			   output. I: left channel, Q: right channel */
			vecsDataOut[iCurIndex] = sCurOutReal;
			vecsDataOut[iCurIndex + 1] = sCurOutImag;
			break;

		case OF_EP:
			/* Send envelope and phase signal to stereo sound card
			   output. Envelope: left channel, Phase: right channel */
			vecsDataOut[iCurIndex] = sCurOutEnv;
			vecsDataOut[iCurIndex + 1] = sCurOutPhase;
			break;
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
			/* Write data to file */
			for (i = 0; i < iBigBlockSize; i++)
			{
#ifdef FILE_DRM_USING_RAW_DATA
				const short sOut = vecsDataOut[i];

				/* Write 2 bytes, 1 piece */
				fwrite((const void*) &sOut, size_t(2), size_t(1),
					pFileTransmitter);
#else
				/* This can be read with Matlab "load" command */
				fprintf(pFileTransmitter, "%d\n", vecsDataOut[i]);
#endif
			}

			/* Flush the file buffer */
			fflush(pFileTransmitter);
		}
	}
}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
	float*	pCurFilt;
	int		iNumTapsTransmFilt;
	CReal	rNormCurFreqOffset;

	/* Init vector for storing a complete DRM frame number of OFDM symbols */
	iBlockCnt = 0;
	iNumBlocks = TransmParam.iNumSymPerFrame;
	iBigBlockSize = TransmParam.iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks;

	vecsDataOut.Init(iBigBlockSize);

	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface */
		pSound->InitPlayback(iBigBlockSize, TRUE);
	}
	else
	{
		/* Open file for writing data for transmitting */
#ifdef FILE_DRM_USING_RAW_DATA
		pFileTransmitter = fopen(strOutFileName.c_str(), "wb");
#else
		pFileTransmitter = fopen(strOutFileName.c_str(), "w");
#endif

		/* Check for error */
		if (pFileTransmitter == NULL)
			throw CGenErr("The file " + strOutFileName + " cannot be created.");
	}

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	switch (TransmParam.GetSpectrumOccup())
	{
	case SO_0:
		pCurFilt = fTransmFilt4_5;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_4_5;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 2190.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_1:
		pCurFilt = fTransmFilt5;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_5;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 2440.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_2:
		pCurFilt = fTransmFilt9;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_9;

		/* Centered */
		rNormCurFreqOffset = rDefCarOffset / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_3:
		pCurFilt = fTransmFilt10;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_10;

		/* Centered */
		rNormCurFreqOffset = rDefCarOffset / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_4:
		pCurFilt = fTransmFilt18;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_18;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 4500.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_5:
		pCurFilt = fTransmFilt20;
		iNumTapsTransmFilt = NUM_TAPS_TRANSMFILTER_20;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 5000.0) / SOUNDCRD_SAMPLE_RATE;
		break;
	}

	/* Init filter taps */
	rvecB.Init(iNumTapsTransmFilt);

	/* Modulate filter to shift it to the correct IF frequency */
	for (int i = 0; i < iNumTapsTransmFilt; i++)
	{
		rvecB[i] =
			pCurFilt[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);
	}

	/* Only FIR filter */
	rvecA.Init(1);
	rvecA[0] = (CReal) 1.0;

	/* State memory (init with zeros) and data vector */
	rvecZReal.Init(iNumTapsTransmFilt - 1, (CReal) 0.0);
	rvecZImag.Init(iNumTapsTransmFilt - 1, (CReal) 0.0);
	rvecDataReal.Init(TransmParam.iSymbolBlockSize);
	rvecDataImag.Init(TransmParam.iSymbolBlockSize);

	/* All robustness modes and spectrum occupancies should have the same output
	   power. Calculate the normaization factor based on the average power of
	   symbol (the number 3000 was obtained through output tests) */
	rNormFactor = (CReal) 3000.0 / Sqrt(TransmParam.rAvPowPerSymbol);

	/* Define block-size for input */
	iInputBlockSize = TransmParam.iSymbolBlockSize;
}

CTransmitData::~CTransmitData()
{
	/* Close file */
	if (pFileTransmitter != NULL)
		fclose(pFileTransmitter);
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
			PostWinMessage(MS_IOINTERFACE, 0); /* green light */
		else
			PostWinMessage(MS_IOINTERFACE, 2); /* red light */

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
				/* Mix left and right channel together. Prevent overflow! First,
				   copy recorded data from "short" in "int" type variables */
				const int iLeftChan = vecsSoundBuffer[2 * i];
				const int iRightChan = vecsSoundBuffer[2 * i + 1];

				(*pvecOutputData)[i] = (_REAL) ((iLeftChan + iRightChan) / 2);
			}
			break;
		}
	}
	else
	{
		/* Read data from file ---------------------------------------------- */
		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* If enf-of-file is reached, stop simulation */
#ifdef FILE_DRM_USING_RAW_DATA
			short tIn;

			/* Read 2 bytes, 1 piece */
			if (fread((void*) &tIn, size_t(2), size_t(1), pFileReceiver) ==
				size_t(0))
#else
			float tIn;

			if (fscanf(pFileReceiver, "%e\n", &tIn) == EOF)
#endif
			{
				Parameter.bRunThread = FALSE;

				/* Set output block size to zero to avoid writing invalid
				   data */
				iOutputBlockSize = 0;

				return;	
			}
			else
			{
				/* Write internal output buffer */
				(*pvecOutputData)[i] = (_REAL) tIn;
			}
		}
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

	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface. Set it to one symbol. The sound card interface
		   has to taken care about the buffering data of a whole MSC block.
		   Use stereo input (* 2) */
		pSound->InitRecording(Parameter.iSymbolBlockSize * 2);

		/* Init buffer size for taking stereo input */
		vecsSoundBuffer.Init(Parameter.iSymbolBlockSize * 2);
	}
	else
	{
		/* Open file for reading data from transmitter. Open file only once */
		if (pFileReceiver == NULL)
		{
#ifdef FILE_DRM_USING_RAW_DATA
			pFileReceiver = fopen(strInFileName.c_str(), "rb");
#else
			pFileReceiver = fopen(strInFileName.c_str(), "r");
#endif
		}

		/* Check for error */
		if (pFileReceiver == NULL)
			throw CGenErr("The file " + strInFileName + " must exist.");
	}

	/* Init signal meter */
	SignalLevelMeter.Init(0);

	/* Define output block-size */
	iOutputBlockSize = Parameter.iSymbolBlockSize;
}

CReceiveData::~CReceiveData()
{
	/* Close file (if opened) */
	if (pFileReceiver != NULL)
		fclose(pFileReceiver);
}

void CReceiveData::GetInputSpec(CVector<_REAL>& vecrData,
								CVector<_REAL>& vecrScale)
{
	int				i;
	CComplexVector	veccSpectrum;
	CRealVector		vecrFFTInput;

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

	/* Init vectors */
	vecrFFTInput.Init(NUM_SMPLS_4_INPUT_SPECTRUM);
	veccSpectrum.Init(iLenSpecWithNyFreq);

	/* Copy data from shift register in Matlib vector */
	for (i = 0; i < NUM_SMPLS_4_INPUT_SPECTRUM; i++)
		vecrFFTInput[i] = vecrInpData[i];

	/* Get spectrum */
	veccSpectrum = rfft(vecrFFTInput);

	/* Log power spectrum data */
	for (i = 0; i < iLenSpecWithNyFreq; i++)
	{
		const _REAL rNormSqMag = SqMag(veccSpectrum[i]) / rNormData;

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
	int			i;
	CRealVector	vecrSpectrum;
	CRealVector	vecrFFTInput;
	CRealVector	vecrHammWin;

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

	/* Init vectors and Hamming window */
	vecrSpectrum.Init(iLenSpecWithNyFreq, (CReal) 0.0);
	vecrFFTInput.Init(LEN_PSD_AV_EACH_BLOCK);
	vecrHammWin.Init(LEN_PSD_AV_EACH_BLOCK);
	vecrHammWin = Hamming(LEN_PSD_AV_EACH_BLOCK);

	/* Calculate FFT of each small block and average results (estimation
	   of PSD of input signal) */
	for (int j = 0; j < NUM_AV_BLOCKS_PSD; j++)
	{
		/* Copy data from shift register in Matlib vector */
		for (i = 0; i < LEN_PSD_AV_EACH_BLOCK; i++)
			vecrFFTInput[i] = vecrInpData[i + j * LEN_PSD_AV_EACH_BLOCK];

		/* Calculate squared magnitude of spectrum and average results */
		vecrSpectrum += SqMag(rfft(vecrFFTInput * vecrHammWin));
	}

	/* Log power spectrum data */
	for (i = 0; i < iLenSpecWithNyFreq; i++)
	{
		const _REAL rNormSqMag = vecrSpectrum[i] / rNormData;

		if (rNormSqMag > 0)
			vecrData[i] = (_REAL) 10.0 * log10(rNormSqMag);
		else
			vecrData[i] = RET_VAL_LOG_0;

		vecrScale[i] = (_REAL) i * rFactorScale;
	}

	/* Release resources */
	Unlock();
}
