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
	/* Write data to file */
	/* Use only real-part. Since we use only the real part of the signal, we
	   have to double the amplitude */
	for (int y = 0; y < iInputBlockSize; y++)
		fprintf(pFileTransmitter, "%e\n",
			(float) (*pvecInputData)[y].real() * 2 * (_REAL) 64.0);

	/* Flush the buffer instantly because the receiver is called right
	   after finishing the transmitter. If the buffer is not flushed it can
	   happen that some data is not written in the file */
	fflush(pFileTransmitter);
}

void CTransmitData::InitInternal(CParameter& TransmParam)
{
	/* Define block-size for input */
	iInputBlockSize = TransmParam.iFFTSizeN + TransmParam.iGuardSize;

	/* Open file for writing data for transmitting */
	pFileTransmitter = fopen("test/TransmittedData.txt", "w");
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
		Sound.Read(vecsSoundBuffer);

		/* Write data to output buffer */
		for (i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (_REAL) vecsSoundBuffer[i];
	}
	else
	{
		/* Read data from file ---------------------------------------------- */
		float rTemp;
		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* If enf-of-file is reached, stop simulation */
			if (fscanf(pFileReceiver, "%e\n", &rTemp) == EOF)
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
				(*pvecOutputData)[i] = rTemp;
			}
		}
	}

	/* Copy data in buffer for spectrum calculation */
	vecrInpData.AddEnd((*pvecOutputData), iOutputBlockSize);


	/* Flip spectrum if necessary ------------------------------------------- */
	if (bFippedSpectrum == TRUE)
	{
		static _BOOLEAN bFlagInv = FALSE;

		for (i = 0; i < iOutputBlockSize; i++)
		{
			/* We flip the spectrum by using the mirror spectrum at the negative
			   frequencys. If we shift by half of the sample frequency, we can
			   do the shift without need of Hilbert transformation */
			if (bFlagInv == FALSE)
			{
				(*pvecOutputData)[i] = (_REAL) -1.0 * (*pvecOutputData)[i];
				bFlagInv = TRUE;
			}
			else
				bFlagInv = FALSE;
		}
	}


	/* Update level meter --------------------------------------------------- */
	LevelMeter();
}

void CReceiveData::InitInternal(CParameter& Parameter)
{
	int iSpecificOutBlockSize;

	if (bUseSoundcard == TRUE)
	{
		/* Set it to one symbol. The sound card interface has to taken care
		   about the buffering data of a whole MSC block */
		iSpecificOutBlockSize = Parameter.iSymbolBlockSize;

		/* Init sound interface */
		try
		{
			Sound.InitRecording(iSpecificOutBlockSize);
		}

		catch (...)
		{
			ErrorMessage("Sound card initialization failure.");
		}

		vecsSoundBuffer.Init(iSpecificOutBlockSize);
	}
	else
	{
		/* Open file for reading data from transmitter. Open file only once */
		if (pFileReceiver == NULL)
			pFileReceiver = fopen("test/TransmittedData.txt", "r");

		iSpecificOutBlockSize = Parameter.iSymbolBlockSize;
	}

	/* Init vector for saving input data for spectrum */
	vecrInpData.Init(NO_SMPLS_4_INPUT_SPECTRUM);

	/* Define output block-size */
	iOutputBlockSize = iSpecificOutBlockSize;

	/* Define maximum size of output cyclic buffer */
	iMaxOutputBlockSize = 2 * iSpecificOutBlockSize;
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
		return 20 * log10(rNormMicLevel);
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

	if (IsInInit() == FALSE)
	{
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
	}
}
