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

#ifdef WRITE_TRNSM_TO_FILE
	/* Write data to file */
	/* Use only real-part. Since we use only the real part of the signal, we
	   have to double the amplitude (64 * 2 = 128) */
	for (i = 0; i < iInputBlockSize; i++)
	{
#ifdef FILE_DRM_USING_RAW_DATA
		const short sOut =
			(short) ((*pvecInputData)[i].real() * (_REAL) 128.0);

		/* Write 2 bytes, 1 piece */
		fwrite((const void*) &sOut, size_t(2), size_t(1), pFileTransmitter);
#else
		fprintf(pFileTransmitter, "%e\n",
			(float) (*pvecInputData)[i].real() * (_REAL) 128.0);
#endif
	}

	/* Flush the file buffer */
	fflush(pFileTransmitter);
#else


	/* Filtering of output signal (FIR filter) ------------------------------ */
	/* Transfer input data in Matlib library vector */
	for (i = 0; i < iInputBlockSize; i++)
	{
		rvecDataReal[i] = (*pvecInputData)[i].real();
		rvecDataImag[i] = (*pvecInputData)[i].imag();
	}

	/* Actual filter routine (use saved state vector) */
	rvecDataReal = Filter(rvecB, rvecA, rvecDataReal, rvecZ);

	if (eOutputFormat == OF_IQ)
		rvecDataImag = Filter(rvecB, rvecA, rvecDataImag, rvecZ);


	/* Convert vector type. Fill vector with symbols (collect them) */
	const int iNs2 = iInputBlockSize * 2;
	for (i = 0; i < iNs2; i += 2)
	{
		const int iCurIndex = iBlockCnt * iNs2 + i;

		/* Imaginary, real */
		const short sCurOutReal =
			(short) (rvecDataReal[i / 2] * (_REAL) 128.0);
		const short sCurOutImag =
			(short) (rvecDataImag[i / 2] * (_REAL) 128.0);

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

		/* Write data to sound card. Must be a blocking function */
		pSound->Write(vecsDataOut);
	}
#endif
}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
#ifdef WRITE_TRNSM_TO_FILE
	/* Open file for writing data for transmitting */
#ifdef FILE_DRM_USING_RAW_DATA
	pFileTransmitter = fopen("test/TransmittedData.txt", "wb");
#else
	pFileTransmitter = fopen("test/TransmittedData.txt", "w");
#endif

	/* Check for error */
	if (pFileTransmitter == NULL)
		throw CGenErr("The file test/TransmittedData.txt cannot be created.");
#else
	/* Init vector for storing a complete DRM frame number of OFDM symbols */
	iBlockCnt = 0;
	iNumBlocks = TransmParam.iNumSymPerFrame;
	const int iTotalSize =
		TransmParam.iSymbolBlockSize * 2 /* Stereo */ * iNumBlocks;

	vecsDataOut.Init(iTotalSize);

	/* Init sound interface */
	pSound->InitPlayback(iTotalSize, TRUE);
#endif

	/* Init filter taps */
	rvecB.Init(NUM_TAPS_TRANSMFILTER);

	/* Choose correct filter for chosen DRM bandwidth. Also, adjust offset
	   frequency for different modes. E.g., 5 kHz mode is on the right side
	   of the DC frequency */
	float* pCurFilt;
	CReal rNormCurFreqOffset;

	switch (TransmParam.GetSpectrumOccup())
	{
	case SO_0:
		pCurFilt = fTransmFilt4_5;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 2250.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_1:
		pCurFilt = fTransmFilt5;

		/* Completely on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 2500.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_2:
		pCurFilt = fTransmFilt9;

		/* Centered */
		rNormCurFreqOffset = rDefCarOffset / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_3:
		pCurFilt = fTransmFilt10;

		/* Centered */
		rNormCurFreqOffset = rDefCarOffset / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_4:
		pCurFilt = fTransmFilt18;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 4500.0) / SOUNDCRD_SAMPLE_RATE;
		break;

	case SO_5:
		pCurFilt = fTransmFilt20;

		/* Main part on the right side of DC */
		rNormCurFreqOffset =
			(rDefCarOffset + (CReal) 5000.0) / SOUNDCRD_SAMPLE_RATE;
		break;
	}

	/* Modulate filter to shift it to the correct IF frequency */
	for (int i = 0; i < NUM_TAPS_TRANSMFILTER; i++)
		rvecB[i] = pCurFilt[i] * Cos((CReal) 2.0 * crPi * rNormCurFreqOffset * i);

	/* Only FIR filter */
	rvecA.Init(1);
	rvecA[0] = (CReal) 1.0;

	/* State memory (init with zeros) and data vector */
	rvecZ.Init(NUM_TAPS_TRANSMFILTER, (CReal) 0.0);
	rvecDataReal.Init(TransmParam.iSymbolBlockSize);
	rvecDataImag.Init(TransmParam.iSymbolBlockSize);

	/* Define block-size for input */
	iInputBlockSize = TransmParam.iSymbolBlockSize;
}

CTransmitData::~CTransmitData()
{
#ifdef WRITE_TRNSM_TO_FILE
	/* Close file */
	if (pFileTransmitter != NULL)
		fclose(pFileTransmitter);
#endif
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

		/* Write data to output buffer */
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[i];
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


	/* Copy data in buffer for spectrum calculation ------------------------- */
	vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);


	/* Update level meter --------------------------------------------------- */
	LevelMeter();
}

void CReceiveData::InitInternal(CParameter& Parameter)
{
	if (bUseSoundcard == TRUE)
	{
		/* Init sound interface. Set it to one symbol. The sound card interface
		   has to taken care about the buffering data of a whole MSC block */
		pSound->InitRecording(Parameter.iSymbolBlockSize);

		vecsSoundBuffer.Init(Parameter.iSymbolBlockSize);
	}
	else
	{
		/* Open file for reading data from transmitter. Open file only once */
		if (pFileReceiver == NULL)
		{
#ifdef FILE_DRM_USING_RAW_DATA
			pFileReceiver = fopen("test/TransmittedData.txt", "rb");
#else
			pFileReceiver = fopen("test/TransmittedData.txt", "r");
#endif
		}

		/* Check for error */
		if (pFileReceiver == NULL)
			throw CGenErr("The file test/TransmittedData.txt must exist.");
	}

	/* Init vector for saving input data for spectrum */
	vecrInpData.Init(NUM_SMPLS_4_INPUT_SPECTRUM);

	/* Define output block-size */
	iOutputBlockSize = Parameter.iSymbolBlockSize;
}

CReceiveData::~CReceiveData()
{
	/* Close file (if opened) */
	if (pFileReceiver != NULL)
		fclose(pFileReceiver);
}

void CReceiveData::LevelMeter()
{
	/* Search for maximum. Decrease this max with time */
	for (int i = 0; i < iOutputBlockSize; i++)
	{
		/* Decrease max with time */
		if (iCurMicMeterLev >= METER_FLY_BACK)
			iCurMicMeterLev -= METER_FLY_BACK;
		else
		{
			if ((iCurMicMeterLev <= METER_FLY_BACK) && (iCurMicMeterLev > 1))
				iCurMicMeterLev -= 2;
		}

		/* Search for max */
		if (fabs((*pvecOutputData)[i]) > iCurMicMeterLev)
			iCurMicMeterLev = Real2Sample(fabs((*pvecOutputData)[i]));
	}
}

_REAL CReceiveData::GetLevelMeter()
{
	_REAL rNormMicLevel = (_REAL) iCurMicMeterLev / _MAXSHORT;

	/* Logarithmic measure */
	if (rNormMicLevel > 0)
		return (_REAL) 20.0 * log10(rNormMicLevel);
	else
		return RET_VAL_LOG_0;
}

void CReceiveData::GetInputSpec(CVector<_REAL>& vecrData,
								CVector<_REAL>& vecrScale)
{
	int				i;
	int				iLenSpecWithNyFreq;
	int				iLenInputVector;
	CComplexVector	veccSpectrum;
	_REAL			rFactorScale;
	_REAL			rNormData;
	CRealVector		vecrFFTInput;
	_REAL			rNormSqMag;

	iLenInputVector = vecrInpData.Size();

	/* Length of spectrum vector including Nyquist frequency */
	iLenSpecWithNyFreq = iLenInputVector / 2 + 1;

	/* Init input and output vectors */
	vecrData.Init(iLenSpecWithNyFreq, (_REAL) 0.0);
	vecrScale.Init(iLenSpecWithNyFreq, (_REAL) 0.0);

	/* Do copying of data only if vector is of non-zero length which means that
	   the module was already initialized */
	if (iLenInputVector != 0)
	{
		/* Lock resources */
		Lock();

		rFactorScale = (_REAL) SOUNDCRD_SAMPLE_RATE / iLenSpecWithNyFreq / 2000;
		rNormData = (_REAL) iLenInputVector * iLenInputVector * _MAXSHORT;

		veccSpectrum.Init(iLenSpecWithNyFreq);

		/* Copy data from shift register in Matlib vector */
		vecrFFTInput.Init(iLenInputVector);
		for (i = 0; i < iLenInputVector; i++)
			vecrFFTInput[i] = vecrInpData[i];

		/* Get spectrum */
		veccSpectrum = rfft(vecrFFTInput);

		/* Log power spectrum data */
		for (i = 0; i < iLenSpecWithNyFreq; i++)
		{
			rNormSqMag = SqMag(veccSpectrum[i]) / rNormData;

			if (rNormSqMag > 0)
				vecrData[i] = 
					(_REAL) 10.0 * log10(rNormSqMag);
			else
				vecrData[i] = RET_VAL_LOG_0;

			vecrScale[i] = (_REAL) i * rFactorScale;
		}

		/* Release resources */
		Unlock();
	}
}
