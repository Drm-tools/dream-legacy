/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See Data.cpp
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

#if !defined(DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "Modul.h"
#include "FAC/FAC.h"
#include "SDC/SDC.h"
#include <time.h>
#ifdef _WIN32
# include "../../Windows/source/sound.h"
#else
# include "source/sound.h"
#endif


/* Definitions ****************************************************************/
/* In case of random-noise, define number of blocks */
#define DEFAULT_NUM_SIM_BLOCKS		50


/* Classes ********************************************************************/
/* MSC ---------------------------------------------------------------------- */
class CReadData : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CReadData() : iNumTransBlocks(DEFAULT_NUM_SIM_BLOCKS) {}
	virtual ~CReadData() {}

	void SetNumTransBlocks(int iNewNum) {iNumTransBlocks = iNewNum;}

protected:
	int iNumTransBlocks;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CWriteData : public CReceiverModul<_SAMPLE, _SAMPLE>
{
public:
	CWriteData(CSound* pNS) : bMuteAudio(FALSE) {pSound = pNS;}
	virtual ~CWriteData() {}

	void MuteAudio(_BOOLEAN bNewMA) {bMuteAudio = bNewMA;}

protected:
	CSound*		pSound;
	_BOOLEAN	bMuteAudio;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};

class CGenSimData : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CGenSimData() : iNumSimBlocks(DEFAULT_NUM_SIM_BLOCKS), eCntType(CT_TIME),
		iCounter(0), iNumErrors(0), strFileName("SimTime.dat"), tiStartTime(0) {}
	virtual ~CGenSimData() {}

	void SetSimTime(int iNewTi, string strNewFileName);
	void SetNumErrors(int iNewNE, string strNewFileName);
	int GetNumErrors() {return iNumErrors;}

protected:
	enum ECntType {CT_TIME, CT_ERRORS};
	ECntType	eCntType;
	int			iNumSimBlocks;
	int			iNumErrors;
	int			iCounter;
	int			iMinNumBlocks;
	string		strFileName;
	time_t		tiStartTime;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CEvaSimData : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CEvaSimData() {}
	virtual ~CEvaSimData() {}

protected:
	int		iIniCnt;
	int		iNumAccBitErrRate;
	_REAL	rAccBitErrRate;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


/* FAC ---------------------------------------------------------------------- */
class CGenerateFACData : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CGenerateFACData() {}
	virtual ~CGenerateFACData() {}

protected:
	CFACTransmit FACTransmit;

	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CUtilizeFACData : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CUtilizeFACData() : bSyncInput(FALSE), bCRCOk(FALSE) {}
	virtual ~CUtilizeFACData() {}

	/* To set the module up for synchronized DRM input data stream */
	void SetSyncInput(_BOOLEAN bNewS) {bSyncInput = bNewS;}

	_BOOLEAN GetCRCOk() const {return bCRCOk;}

protected:
	CFACReceive FACReceive;

	_BOOLEAN	bCRCOk;
	_BOOLEAN	bSyncInput;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


/* SDC ---------------------------------------------------------------------- */
class CGenerateSDCData : public CTransmitterModul<_BINARY, _BINARY>
{
public:
	CGenerateSDCData() {}
	virtual ~CGenerateSDCData() {}

protected:
	CSDCTransmit SDCTransmit;
		
	virtual void InitInternal(CParameter& TransmParam);
	virtual void ProcessDataInternal(CParameter& TransmParam);
};

class CUtilizeSDCData : public CReceiverModul<_BINARY, _BINARY>
{
public:
	CUtilizeSDCData() {}
	virtual ~CUtilizeSDCData() {}

protected:
	CSDCReceive SDCReceive;

	virtual void InitInternal(CParameter& ReceiverParam);
	virtual void ProcessDataInternal(CParameter& ReceiverParam);
};


/******************************************************************************\
* Data type conversion classes needed for simulation						   *
\******************************************************************************/
/* Conversion from channel output to resample module input */
class CDataConvChanResam : public CReceiverModul<CChanSimDataMod, _REAL>
{
protected:
	virtual void InitInternal(CParameter& ReceiverParam)
	{
		iInputBlockSize = ReceiverParam.iSymbolBlockSize;
		iOutputBlockSize = ReceiverParam.iSymbolBlockSize;
	}
	virtual void ProcessDataInternal(CParameter& ReceiverParam)
	{
		for (int i = 0; i < iOutputBlockSize; i++)
			(*pvecOutputData)[i] = (*pvecInputData)[i].tOut;
	}
};


#endif // !defined(DATA_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
