/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	The common functionality for reading and writing the transfer-buffers is
 *	implemented here.
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

#ifndef _MODUL_IMPL_H
#define _MODUL_IMPL_H

#include "TransmitterModul.h"

template<class TData>
void InitTransferBuffer(CBuffer<TData>& buffer, int max, int n)
{
	if (max != 0)
		buffer.Init(max);
	else
	{
		buffer.Init(n);
	}
}

/******************************************************************************\
* Transmitter modul (CTransmitterModul)                                        *
\******************************************************************************/
template<class TInput, class TOutput>
CTransmitterModul<TInput, TOutput>::CTransmitterModul()
:CModul<TInput, TOutput>(),vecpvecInputData(),vecpvecOutputData()
{
	/* Initialize all member variables with zeros */
	this->iInputBlockSize = 0;
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;
	this->pvecInputData = NULL;
	pvecInputData2 = NULL;
	pvecInputData3 = NULL;
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	this->iInputBlockSize = 0;
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;
	this->pvecInputData = NULL;
	pvecInputData2 = NULL;
	pvecInputData3 = NULL;
	/* Init base-class */
	//CModul<TInput, TOutput>::Init(Parameter); we dont want the "thread safe" ness
	this->InitInternal(Parameter);
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter, 
											  CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	this->iInputBlockSize = 0;
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;
	this->pvecInputData = NULL;
	pvecInputData2 = NULL;
	pvecInputData3 = NULL;
	veciInputBlockSize.clear();

	/* Init base-class */
	//CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
	this->InitInternal(Parameter);

	/* Init output transfer buffer */
	InitTransferBuffer(OutputBuffer, this->iMaxOutputBlockSize, this->iOutputBlockSize);
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::Init(CParameter& Parameter, 
	CBuffer<TOutput>& OutputBuffer,
	CBuffer<TOutput>& OutputBuffer2,
	vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Init some internal variables */
	this->iInputBlockSize = 0;
	iInputBlockSize2 = 0;
	iInputBlockSize3 = 0;
	veciInputBlockSize.clear();

	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;

	size_t i;

	size_t size = vecOutputBuffer.size();

	veciMaxOutputBlockSize.resize(size);
	veciOutputBlockSize.resize(size);
	for(i=0; i<size; i++)
	{
		veciMaxOutputBlockSize[i] = 0;
		veciOutputBlockSize[i] = 0;
	}

	/* Init base-class */
	//CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
	this->InitInternal(Parameter);

	InitTransferBuffer(OutputBuffer, this->iMaxOutputBlockSize, this->iOutputBlockSize);
	InitTransferBuffer(OutputBuffer2, this->iMaxOutputBlockSize2, this->iOutputBlockSize2);
	for(i=0; i<size; i++)
	{
		InitTransferBuffer(vecOutputBuffer[i], this->veciMaxOutputBlockSize[i], this->veciOutputBlockSize[i]);
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Check, if enough input data is available */
		if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);

			return;
		}

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TInput>& InputBuffer2,
				CBuffer<TInput>& InputBuffer3,
				CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		bool bAllInputsReady = true;

		/* Check, if enough input data is available from all sources */
		if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);
			bAllInputsReady = false;
		}
		if (InputBuffer2.GetFillLevel() < iInputBlockSize2)
		{
			/* Set request flag */
			InputBuffer2.SetRequestFlag(TRUE);
			bAllInputsReady = false;
		}
		if (InputBuffer3.GetFillLevel() < iInputBlockSize3)
		{
			/* Set request flag */
			InputBuffer3.SetRequestFlag(TRUE);
			bAllInputsReady = false;
		}
	
		if(bAllInputsReady == false)
			return;

		/* Get vectors from transfer-buffers */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);
		pvecInputData3 = InputBuffer3.Get(iInputBlockSize3);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer,
									CBuffer<TOutput>& OutputBuffer2,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffers if data is requested */

	size_t i=0;

	_BOOLEAN bAllOutputsReady = TRUE;

	if (OutputBuffer.GetRequestFlag() == FALSE)
	{
		bAllOutputsReady = FALSE;
	}
		
	if (OutputBuffer2.GetRequestFlag() == FALSE)
	{
		bAllOutputsReady = FALSE;
	}

	for(i=0; i<vecOutputBuffer.size(); i++)
	{
		if (vecOutputBuffer[i].GetRequestFlag() == FALSE)
		{
			bAllOutputsReady = FALSE;
		}
	}

	if(bAllOutputsReady)
	{
		/* Check, if enough input data is available */
		if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
		{
			/* Set request flag */
			InputBuffer.SetRequestFlag(TRUE);
			return;
		}

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

		/* Query vectors from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		this->pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		for(i=0; i<vecOutputBuffer.size(); i++)
			this->vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer2.Put(this->iOutputBlockSize2);

		/* Data was provided, clear data request */
		OutputBuffer2.SetRequestFlag(FALSE);
	
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			/* Write processed data from internal memory in transfer-buffer */
			vecOutputBuffer[i].Put(this->veciOutputBlockSize[i]);

			/* Data was provided, clear data request */
			vecOutputBuffer[i].SetRequestFlag(FALSE);
		}
	}
}

template<class TInput, class TOutput>
void CTransmitterModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter ---------------- */
	/* Look in output buffer if data is requested */
	if (OutputBuffer.GetRequestFlag() == TRUE)
	{
		/* Read data and write it in the transfer-buffer.
		   Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);
		
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);

		/* Data was provided, clear data request */
		OutputBuffer.SetRequestFlag(FALSE);
	}
}

template<class TInput, class TOutput>
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter */
	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(TRUE);

		return FALSE;
	}

	/* Get vector from transfer-buffer */
	this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	return TRUE;
}

template<class TInput, class TOutput>
_BOOLEAN CTransmitterModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, 
				CBuffer<TInput>& InputBuffer,
				CBuffer<TInput>& InputBuffer2, 
				vector<CSingleBuffer<TInput> >& vecInputBuffer)
{
	/* OUTPUT-DRIVEN modul implementation in the transmitter */

	size_t i;

	bool bAllInputsReady = true;

	/* Check, if enough input data is available */
	if (InputBuffer.GetFillLevel() < this->iInputBlockSize)
	{
		/* Set request flag */
		InputBuffer.SetRequestFlag(TRUE);

		bAllInputsReady = false;

	}

	/* Check, if enough input data is available */
	if (InputBuffer2.GetFillLevel() < this->iInputBlockSize2)
	{
		/* Set request flag */
		InputBuffer2.SetRequestFlag(TRUE);

		bAllInputsReady = false;

	}

	for(i=0; i<vecInputBuffer.size(); i++)
	{
		/* Check, if enough input data is available */
		if (vecInputBuffer[i].GetFillLevel() < this->veciInputBlockSize[i])
		{
			/* Set request flag */
			vecInputBuffer[i].SetRequestFlag(TRUE);

			bAllInputsReady = false;
		}

	}

	if(bAllInputsReady == false)
		return FALSE;

	/* Get vector from transfer-buffer */
	this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	this->pvecInputData2 = InputBuffer2.Get(this->iInputBlockSize2);
	for(i=0; i<vecInputBuffer.size(); i++)
		this->vecpvecInputData[i] = vecInputBuffer[i].Get(this->veciInputBlockSize[i]);

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	return TRUE;
}

#endif
