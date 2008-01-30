/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	High level class for all modules. The common functionality for reading
 *	and writing the transfer-buffers are implemented here.
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

#if !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
#define AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_

#include "Buffer.h"
#include "Vector.h"
#include "../Parameter.h"
#include <iostream>


/* Classes ********************************************************************/
template<class T>
struct CInputStruct
{
	CInputStruct(): pvecData(NULL), iBlockSize(0) {}
	void clear() { pvecData = NULL; iBlockSize = 0; }
	CVectorEx<T>*	pvecData;
	/* Actual read (or written) size of the data */
	int					iBlockSize;
};

template<class T>
struct COutputStruct : public CInputStruct<T>
{
	COutputStruct(): CInputStruct<T>(), iMaxBlockSize(0) {}
	void clear() { CInputStruct<T>::clear(); iMaxBlockSize = 0; }
	template<typename T2>
	void init(T2 OutputBuffer)
	{
		if (this->iMaxBlockSize != 0)
			OutputBuffer->Init(iMaxBlockSize);
		else
		{
			if (this->iBlockSize != 0)
				OutputBuffer->Init(this->iBlockSize);
		}
	}
	/* Max block-size is used to determine the required size of buffer */
	int					iMaxBlockSize;
};

/* CNewModul ------------------------------------------------------------------- */
template< typename TInput, typename TOutput, size_t iNumInputs=1, size_t iNumOutputs=1>
class CNewModul  
{
public:
	CNewModul();
	virtual ~CNewModul() {}

	virtual void Init(CParameter& Parameter, CBuffer<TOutput>* OutputBuffer=NULL);

protected:


	CInputStruct<TInput>		inputs[iNumInputs];
	COutputStruct<TOutput>		outputs[iNumOutputs];

	void 						Init();
	virtual void				InitInternal(CParameter& Parameter) = 0;
	virtual void				ProcessDataInternal(CParameter& Parameter) = 0;

};

/* CTransmitterModul -------------------------------------------------------- */
template< typename TInput, typename TOutput, size_t iNumInputs=1, size_t iNumOutputs=1>
class CTransmitterModul : public CNewModul<TInput, TOutput>
{
public:
	CTransmitterModul();
	virtual ~CTransmitterModul() {}
	/* start of pipeline */
	virtual void		ReadData(CParameter& Parameter, CBuffer<TOutput>* OutputBuffer);
	/* one - one processing */
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>* InputBuffer,
									CBuffer<TOutput>* OutputBuffer);
	/* end of pipeline */
	virtual _BOOLEAN	WriteData(CParameter& Parameter, CBuffer<TInput>* InputBuffer);
protected:
	int iConsumed[iNumInputs];
};


/* Implementation *************************************************************/
/******************************************************************************\
* CNewModul                                                                       *
\******************************************************************************/
template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
CNewModul<TInput, TOutput, iNumInputs, iNumOutputs>::CNewModul()
{
	Init();
}

template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
void CNewModul<TInput, TOutput, iNumInputs, iNumOutputs>::Init()
{
	size_t i;
	for(i=0; i<iNumInputs; i++)
		inputs[i].clear();
	for(i=0; i<iNumOutputs; i++)
		outputs[i].clear();
}

template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
void CNewModul<TInput, TOutput, iNumInputs, iNumOutputs>::
Init(CParameter& Parameter, CBuffer<TOutput>* OutputBuffer)
{
	/* Init some internal variables */

	Init();

	/* Call init of derived module */
	InitInternal(Parameter);

	/* Init output transfer buffer */
	if(OutputBuffer)
		for(size_t i=0; i<iNumOutputs; i++)
			outputs[i].init(OutputBuffer);
}

/******************************************************************************\
* Transmitter module (CTransmitterModul)                                        *
\******************************************************************************/
template<class T>
bool checkInput(CBuffer<T>& Buffer, int wanted)
{
	if (Buffer.GetFillLevel() < wanted)
	{
		/* Set request flag */
		Buffer.SetRequestFlag(TRUE);
		return false;
	}
	return true;
}

template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
CTransmitterModul<TInput, TOutput, iNumInputs, iNumOutputs>::
CTransmitterModul(): CNewModul<TInput, TOutput>()
{
}

/* this handles multiplexing, 1-1 and demultiplexing
 * but demultiplexing assumes that all outputs are ready for input
 * at the same time and complete full blocks
 */
template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
_BOOLEAN CTransmitterModul<TInput, TOutput, iNumInputs, iNumOutputs>::
ProcessData(CParameter& Parameter, CBuffer<TInput>* InputBuffer,
				CBuffer<TOutput>* OutputBuffer)
{
	/* OUTPUT-DRIVEN module implementation in the transmitter ---------------- */
	bool bAllOutputsReady = true;
	size_t i=0;

	/* Look in output buffer if data is requested */
	for(i=0; i<iNumOutputs; i++)
		bAllOutputsReady &= (OutputBuffer[i].GetRequestFlag()==TRUE);

	if (bAllOutputsReady)
	{
		bool bAllInputsOK = true;
		/* Check, if enough input data is available from all sources */

		for(i=0; i<iNumInputs; i++)
			bAllInputsOK &= checkInput(InputBuffer[i], this->inputs[i].iBlockSize);

		if(bAllInputsOK == false)
			return FALSE;
	
		/* Get vectors from transfer-buffers */
		for(i=0; i<iNumInputs; i++)
		{
			this->inputs[i].pvecData = InputBuffer[i].Get(this->inputs[i].iBlockSize);
			this->iConsumed[i] = 0;
		}

		/* Query vector from output transfer-buffer for writing */
		this->outputs[0].pvecData = OutputBuffer[0].QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* consume additional data */
		for(i=0; i<iNumInputs; i++)
			(void)InputBuffer[i].Get(this->iConsumed[i]);
	
		/* Write processed data from internal memory in transfer-buffer */
		for(i=0; i<iNumOutputs; i++)
		{
			OutputBuffer[i].Put(this->outputs[0].iBlockSize);
			/* Data was provided, clear data request */
			OutputBuffer[i].SetRequestFlag(FALSE);
		}
		return TRUE;
	}
	return FALSE;
}

template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
void CTransmitterModul<TInput, TOutput, iNumInputs, iNumOutputs>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>* OutputBuffer)
{
	/* OUTPUT-DRIVEN module implementation in the transmitter ---------------- */
	bool bAllOutputsReady = true;
	size_t i=0;

	/* Look in output buffer if data is requested */
	for(i=0; i<iNumOutputs; i++)
		bAllOutputsReady &= (OutputBuffer[i].GetRequestFlag()==TRUE);

	if (bAllOutputsReady)
	{

		/* Read data and write it in the transfer-buffer.
		 * Query vector from output transfer-buffer for writing */
		for(i=0; i<iNumOutputs; i++)
			this->outputs[i].pvecData = OutputBuffer[i].QueryWriteBuffer();

			/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);
		
		for(i=0; i<iNumOutputs; i++)
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer[i].Put(this->outputs[i].iBlockSize);

			/* Data was provided, clear data request */
			OutputBuffer[i].SetRequestFlag(FALSE);
		}
	}
}

template< typename TInput, typename TOutput, size_t iNumInputs, size_t iNumOutputs>
_BOOLEAN CTransmitterModul<TInput, TOutput, iNumInputs, iNumOutputs>::
	WriteData(CParameter& Parameter, CBuffer<TInput>* InputBuffer)
{
	/* OUTPUT-DRIVEN module implementation in the transmitter */

	bool bReady = true;
	for(size_t i=0; i<iNumInputs; i++)
	{
		/* Check, if enough input data is available */
		int iBlockSize = this->inputs[i].iBlockSize;
		if (InputBuffer[i].GetFillLevel() < iBlockSize)
		{
			/* Set request flag */
			InputBuffer[i].SetRequestFlag(TRUE);
			bReady = false;
			/* Get vector from transfer-buffer */
			this->inputs[i].pvecData = InputBuffer[i].Get(iBlockSize);
		}
	}

	if(!bReady)
		return FALSE;

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	return TRUE;
}

/* Classes ********************************************************************/
/* CModul ------------------------------------------------------------------- */
template<class TInput, class TOutput>
class CModul  
{
public:
	CModul();
	virtual ~CModul() {}

	virtual void Init(CParameter& Parameter);
	virtual void Init(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer);

protected:
	CVectorEx<TInput>*	pvecInputData;
	CVectorEx<TOutput>*	pvecOutputData;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iInputBlockSize;
	int					iOutputBlockSize;

	void				Lock() {Mutex.Lock();}
	void				Unlock() {Mutex.Unlock();}

	void				InitThreadSave(CParameter& Parameter);
	virtual void		InitInternal(CParameter& Parameter) = 0;
	void				ProcessDataThreadSave(CParameter& Parameter);
	virtual void		ProcessDataInternal(CParameter& Parameter) = 0;

private:
	CMutex				Mutex;
};

/* CReceiverModul ----------------------------------------------------------- */
template<class TInput, class TOutput>
class CReceiverModul : public CModul<TInput, TOutput>
{
public:
	CReceiverModul();
	virtual ~CReceiverModul() {}

	void				SetInitFlag() {bDoInit = TRUE;}
	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter, 
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2);
	virtual void		Init(CParameter& Parameter, 
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 CBuffer<TOutput>& OutputBuffer3);
	virtual void		Init(CParameter& Parameter, 
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TOutput>& OutputBuffer2,
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		Init(CParameter& Parameter, 
							 vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual void		ReadData(CParameter& Parameter, 
								 CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer, 
									CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer, 
									CBuffer<TOutput>& OutputBuffer2);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer, 
									CBuffer<TOutput>& OutputBuffer2, 
									CBuffer<TOutput>& OutputBuffer3);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer,
									CBuffer<TOutput>& OutputBuffer, 
									CBuffer<TOutput>& OutputBuffer2, 
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual _BOOLEAN	ProcessData(CParameter& Parameter, 
									CBuffer<TInput>& InputBuffer,
									vector< CSingleBuffer<TOutput> >& vecOutputBuffer);
	virtual _BOOLEAN	WriteData(CParameter& Parameter, 
								  CBuffer<TInput>& InputBuffer);

protected:
	void SetBufReset1() {bResetBuf = TRUE;}
	void SetBufReset2() {bResetBuf2 = TRUE;}
	void SetBufReset3() {bResetBuf3 = TRUE;}
	void SetBufResetN() {for(size_t i=0; i<vecbResetBuf.size(); i++)
     vecbResetBuf[i] = TRUE;}

	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TOutput>*	pvecOutputData2;
	CVectorEx<TOutput>*	pvecOutputData3;
	vector<CVectorEx<TOutput>*>	vecpvecOutputData;

	/* Max block-size are used to determine the size of the required buffer */
	int					iMaxOutputBlockSize2;
	int					iMaxOutputBlockSize3;
	vector<int>			veciMaxOutputBlockSize;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;
	int					iOutputBlockSize3;
	vector<int>			veciOutputBlockSize;

private:
	/* Init flag */
	_BOOLEAN			bDoInit;

	/* Reset flags for output cyclic-buffers */
	_BOOLEAN			bResetBuf;
	_BOOLEAN			bResetBuf2;
	_BOOLEAN			bResetBuf3;
	vector<_BOOLEAN>	vecbResetBuf;
};


/* CSimulationModul --------------------------------------------------------- */
template<class TInput, class TOutput, class TInOut2>
class CSimulationModul : public CModul<TInput, TOutput>
{
public:
	CSimulationModul();
	virtual ~CSimulationModul() {}

	virtual void		Init(CParameter& Parameter);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer);
	virtual void		Init(CParameter& Parameter,
							 CBuffer<TOutput>& OutputBuffer,
							 CBuffer<TInOut2>& OutputBuffer2);
	virtual void		TransferData(CParameter& Parameter, 
									 CBuffer<TInput>& InputBuffer, 
									 CBuffer<TOutput>& OutputBuffer);


// TEST "ProcessDataIn" "ProcessDataOut"
	virtual _BOOLEAN	ProcessDataIn(CParameter& Parameter, 
									  CBuffer<TInput>& InputBuffer,
									  CBuffer<TInOut2>& InputBuffer2,
									  CBuffer<TOutput>& OutputBuffer);
	virtual _BOOLEAN	ProcessDataOut(CParameter& Parameter, 
									   CBuffer<TInput>& InputBuffer,
									   CBuffer<TOutput>& OutputBuffer, 
									   CBuffer<TInOut2>& OutputBuffer2);


protected:
	/* Additional buffers if the derived class has multiple output streams */
	CVectorEx<TInOut2>*	pvecOutputData2;

	/* Max block-size are used to determine the size of the requiered buffer */
	int					iMaxOutputBlockSize2;
	/* Actual read (or written) size of the data */
	int					iOutputBlockSize2;

	/* Additional buffers if the derived class has multiple input streams */
	CVectorEx<TInOut2>*	pvecInputData2;

	/* Actual read (or written) size of the data */
	int					iInputBlockSize2;
};


/* Implementation *************************************************************/
/******************************************************************************\
* CNewModul                                                                       *
\******************************************************************************/
template<class TInput, class TOutput>
CModul<TInput, TOutput>::CModul()
{
	/* Initialize everything with zeros */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;
	pvecInputData = NULL;
	pvecOutputData = NULL;
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::ProcessDataThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	/* Call processing routine of derived modul */
	ProcessDataInternal(Parameter);

	/* Unlock resources */
	Unlock();
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::InitThreadSave(CParameter& Parameter)
{
	/* Get a lock for the resources */
	Lock();

	try
	{
		/* Call init of derived modul */
		InitInternal(Parameter);

		/* Unlock resources */
		Unlock();
	}

	catch (CGenErr)
	{
		/* Unlock resources */
		Unlock();

		/* Throws the same error again which was send by the function */
		throw;
	}
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init some internal variables */
	iInputBlockSize = 0;

	/* Call init of derived modul */
	InitThreadSave(Parameter);
}

template<class TInput, class TOutput>
void CModul<TInput, TOutput>::Init(CParameter& Parameter, 
								   CBuffer<TOutput>& OutputBuffer)
{
	/* Init some internal variables */
	iMaxOutputBlockSize = 0;
	iInputBlockSize = 0;
	iOutputBlockSize = 0;

	/* Call init of derived modul */
	InitThreadSave(Parameter);

	/* Init output transfer buffer */
	if (iMaxOutputBlockSize != 0)
		OutputBuffer.Init(iMaxOutputBlockSize);
	else
	{
		if (iOutputBlockSize != 0)
			OutputBuffer.Init(iOutputBlockSize);
	}
}


/******************************************************************************\
* Receiver modul (CReceiverModul)                                              *
\******************************************************************************/
template<class TInput, class TOutput>
CReceiverModul<TInput, TOutput>::CReceiverModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	pvecOutputData2 = NULL;
	pvecOutputData3 = NULL;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;
	bDoInit = FALSE;
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer)
{
	/* Init flag */
	bResetBuf = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  CBuffer<TOutput>& OutputBuffer3)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

	if (iMaxOutputBlockSize3 != 0)
		OutputBuffer3.Init(iMaxOutputBlockSize3);
	else
	{
		if (iOutputBlockSize3 != 0)
			OutputBuffer3.Init(iOutputBlockSize3);
	}
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
									  CBuffer<TOutput>& OutputBuffer,
									  CBuffer<TOutput>& OutputBuffer2,
									  vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iMaxOutputBlockSize3 = 0;
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	iOutputBlockSize2 = 0;
	iOutputBlockSize3 = 0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	bResetBuf = FALSE;
	bResetBuf2 = FALSE;
	bResetBuf3 = FALSE;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}

    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput> void
CReceiverModul<TInput, TOutput>::Init(CParameter& Parameter,
					vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	size_t i;
	/* Init some internal variables */
	veciMaxOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
		veciMaxOutputBlockSize[i]=0;
	veciOutputBlockSize.resize(vecOutputBuffer.size());
    for(i=0; i<veciOutputBlockSize.size(); i++)
		veciOutputBlockSize[i]=0;
	vecbResetBuf.resize(vecOutputBuffer.size());
    for(i=0; i<vecbResetBuf.size(); i++)
		vecbResetBuf[i]=FALSE;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);

	/* Init output transfer buffers */
    for(i=0; i<veciMaxOutputBlockSize.size(); i++)
    {
		if (veciMaxOutputBlockSize[i] != 0)
			vecOutputBuffer[i].Init(veciMaxOutputBlockSize[i]);
		else
		{
			if (veciOutputBlockSize[i] != 0)
				vecOutputBuffer[i].Init(veciOutputBlockSize[i]);
		}
    }
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* Special case if input block size is zero */
	if (this->iInputBlockSize == 0)
	{
		InputBuffer.Clear();

		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from vectors */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	
		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		
		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	
		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				CBuffer<TOutput>& OutputBuffer3)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, OutputBuffer3);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		pvecOutputData3 = OutputBuffer3.QueryWriteBuffer();
		
		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	
		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		if (bResetBuf3 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf3 = FALSE;
			OutputBuffer3.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer3.Put(iOutputBlockSize3);
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				CBuffer<TOutput>& OutputBuffer,
				CBuffer<TOutput>& OutputBuffer2,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer, OutputBuffer2, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}
		
		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	
		/* Reset output-buffers if flag was set by processing routine */
		if (bResetBuf == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf = FALSE;
			OutputBuffer.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer.Put(this->iOutputBlockSize);
		}

		if (bResetBuf2 == TRUE)
		{
			/* Reset flag and clear buffer */
			bResetBuf2 = FALSE;
			OutputBuffer2.Clear();
		}
		else
		{
			/* Write processed data from internal memory in transfer-buffer */
			OutputBuffer2.Put(iOutputBlockSize2);
		}

		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == TRUE)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = FALSE;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	ProcessData(CParameter& Parameter, CBuffer<TInput>& InputBuffer,
				vector< CSingleBuffer<TOutput> >& vecOutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, vecOutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		size_t i;
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		vecpvecOutputData.resize(vecOutputBuffer.size());
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			vecpvecOutputData[i] = vecOutputBuffer[i].QueryWriteBuffer();
		}
		
		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	
		/* Reset output-buffers if flag was set by processing routine */
		for(i=0; i<vecOutputBuffer.size(); i++)
		{
			if (vecbResetBuf[i] == TRUE)
			{
				/* Reset flag and clear buffer */
				vecbResetBuf[i] = FALSE;
				vecOutputBuffer[i].Clear();
			}
			else
			{
				/* Write processed data from internal memory in transfer-buffer */
				vecOutputBuffer[i].Put(veciOutputBlockSize[i]);
			}
		}
	}

	return bEnoughData;
}

template<class TInput, class TOutput>
void CReceiverModul<TInput, TOutput>::
	ReadData(CParameter& Parameter, CBuffer<TOutput>& OutputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter, OutputBuffer);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* Query vector from output transfer-buffer for writing */
	this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

	/* Call the underlying processing-routine */
	this->ProcessDataThreadSave(Parameter);

	/* Reset output-buffers if flag was set by processing routine */
	if (bResetBuf == TRUE)
	{
		/* Reset flag and clear buffer */
		bResetBuf = FALSE;
		OutputBuffer.Clear();
	}
	else
	{
		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}
}

template<class TInput, class TOutput>
_BOOLEAN CReceiverModul<TInput, TOutput>::
	WriteData(CParameter& Parameter, CBuffer<TInput>& InputBuffer)
{
	/* Check initialization flag. The initialization must be done OUTSIDE
	   the processing routine. This is ensured by doing it here, where we
	   have control of calling the processing routine. Therefore we
	   introduced the flag */
	if (bDoInit == TRUE)
	{
		/* Call init routine */
		Init(Parameter);

		/* Reset init flag */
		bDoInit = FALSE;
	}

	/* Special case if input block size is zero and buffer, too */
	if ((InputBuffer.GetFillLevel() == 0) && (this->iInputBlockSize == 0))
	{
		InputBuffer.Clear();
		return FALSE;
	}

	/* INPUT-DRIVEN modul implementation in the receiver -------------------- */
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Call the underlying processing-routine */
		this->ProcessDataThreadSave(Parameter);
	}

	return bEnoughData;
}


/******************************************************************************\
* Simulation modul (CSimulationModul)                                          *
\******************************************************************************/
template<class TInput, class TOutput, class TInOut2>
CSimulationModul<TInput, TOutput, TInOut2>::CSimulationModul()
{
	/* Initialize all member variables with zeros */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;
	iInputBlockSize2 = 0;
	pvecOutputData2 = NULL;
	pvecInputData2 = NULL;
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::Init(CParameter& Parameter)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer)
{
	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	Init(CParameter& Parameter,
		 CBuffer<TOutput>& OutputBuffer,
		 CBuffer<TInOut2>& OutputBuffer2)
{
	/* Init some internal variables */
	iMaxOutputBlockSize2 = 0;
	iOutputBlockSize2 = 0;

	/* Init base-class */
	CModul<TInput, TOutput>::Init(Parameter, OutputBuffer);

	/* Init output transfer buffers */
	if (iMaxOutputBlockSize2 != 0)
		OutputBuffer2.Init(iMaxOutputBlockSize2);
	else
	{
		if (iOutputBlockSize2 != 0)
			OutputBuffer2.Init(iOutputBlockSize2);
	}
}

template<class TInput, class TOutput, class TInOut2>
void CSimulationModul<TInput, TOutput, TInOut2>::
	TransferData(CParameter& Parameter,
				 CBuffer<TInput>& InputBuffer,
				 CBuffer<TOutput>& OutputBuffer)
{
	/* TransferData needed for simulation */
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

	/* Call the underlying processing-routine */
	this->ProcessDataInternal(Parameter);

	/* Write processed data from internal memory in transfer-buffer */
	OutputBuffer.Put(this->iOutputBlockSize);
}

template<class TInput, class TOutput, class TInOut2>
_BOOLEAN CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataIn(CParameter& Parameter,
				  CBuffer<TInput>& InputBuffer,
				  CBuffer<TInOut2>& InputBuffer2,
				  CBuffer<TOutput>& OutputBuffer)
{
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if ((InputBuffer.GetFillLevel() >= this->iInputBlockSize) &&
		(InputBuffer2.GetFillLevel() >= iInputBlockSize2))
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
		pvecInputData2 = InputBuffer2.Get(iInputBlockSize2);

		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();

		/* Copy extended data from FIRST input vector (definition!) */
		(*(this->pvecOutputData)).
			SetExData((*(this->pvecInputData)).GetExData());

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffer */
		OutputBuffer.Put(this->iOutputBlockSize);
	}

	return bEnoughData;
}

template<class TInput, class TOutput, class TInOut2>
_BOOLEAN CSimulationModul<TInput, TOutput, TInOut2>::
	ProcessDataOut(CParameter& Parameter,
				   CBuffer<TInput>& InputBuffer,
				   CBuffer<TOutput>& OutputBuffer,
				   CBuffer<TInOut2>& OutputBuffer2)
{
	/* This flag shows, if enough data was in the input buffer for processing */
	_BOOLEAN bEnoughData = FALSE;

	/* Check if enough data is available in the input buffer for processing */
	if (InputBuffer.GetFillLevel() >= this->iInputBlockSize)
	{
		bEnoughData = TRUE;

		/* Get vector from transfer-buffer */
		this->pvecInputData = InputBuffer.Get(this->iInputBlockSize);
	
		/* Query vector from output transfer-buffer for writing */
		this->pvecOutputData = OutputBuffer.QueryWriteBuffer();
		pvecOutputData2 = OutputBuffer2.QueryWriteBuffer();

		/* Call the underlying processing-routine */
		this->ProcessDataInternal(Parameter);

		/* Write processed data from internal memory in transfer-buffers */
		OutputBuffer.Put(this->iOutputBlockSize);
		OutputBuffer2.Put(iOutputBlockSize2);
	}

	return bEnoughData;
}

/* Take an input buffer and split it 2 ways */

template<class TInput>
class CSplitModul: public CReceiverModul<TInput, TInput>
{
protected:
	virtual void SetInputBlockSize(CParameter& ReceiverParam) = 0;

	virtual void InitInternal(CParameter& ReceiverParam)
	{
		this->SetInputBlockSize(ReceiverParam);
		this->iOutputBlockSize = this->iInputBlockSize;
		this->iOutputBlockSize2 = this->iInputBlockSize;
	}

	virtual void ProcessDataInternal(CParameter&)
	{
		for (int i = 0; i < this->iInputBlockSize; i++)
		{
			TInput n = (*(this->pvecInputData))[i];
			(*this->pvecOutputData)[i] = n;
			(*this->pvecOutputData2)[i] = n;
		}
	}
};

#endif // !defined(AFX_MODUL_H__41E39CD3_2AEC_400E_907B_148C0EC17A43__INCLUDED_)
