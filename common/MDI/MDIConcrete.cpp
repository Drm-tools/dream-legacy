/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
  *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)  
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module implements the abstract CMDI interface defined in MDI.h.
 *  This is the real implementation, as opposed to CMDINull which is a dummy - see MDINull.h.
 *  All modules that generate MDI information are given (normally at construction) a pointer to an MDI object.
 *  They call methods in this interface when they have MDI/RSCI data to impart.
 *
 *	Note that this previously needed QT, but now the QT socket is wrapped in an abstract class so 
 *  this class can be compiled without QT. (A null socket is instantiated instead in this case, so
 *  nothing will actually happen.) This could be developed further by using a factory class to make
 *  the socket, in which case this class would only need to know about the abstract interface
 *  CPacketSocket.
 *
 *  This class is now almost a facade for all of the DCP and TAG classes, designed to have the
 *  same interface as the old CMDI class had. It could be improved further by moving the
 *  MDI generation into a separate class.
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

#include "MDIConcrete.h"
#include "../DrmReceiver.h"

#define MAX_UNLOCKED_COUNT 2 /* number of frames without FAC data before generating free-running RSCI */

/* Implementation *************************************************************/
CMDIConcrete::CMDIConcrete() : pReceiver(NULL), iLogFraCnt(0),
	bMDIInEnabled(FALSE), bMDIOutEnabled(FALSE),
	vecTagItemGeneratorRBP(MAX_NUM_STREAMS), vecTagItemGeneratorStr(MAX_NUM_STREAMS),
	TagPacketDecoderMDI(&MDIInBuffer), iUnlockedCount(MAX_UNLOCKED_COUNT)
{
	/* Initialise all the generators for strx and rbpx tags */
	for (int i=0; i<MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].SetStreamNumber(i);
		vecTagItemGeneratorRBP[i].SetStreamNumber(i);
	}

	/* Reset all tags for initialization and reset flag for SDC tag */
	ResetTags(TRUE);
	bSDCOutWasSet = FALSE;
	bSDCInWasSet = FALSE;

	/* Init constant tag */
	TagItemGeneratorProTyMDI.GenTag();;
	TagItemGeneratorProTyRSCI.GenTag();;


	/* Default settings for the "special protocol settings" ----------------- */
	/* Use CRC for AF packets */
	bUseAFCRC = TRUE;

}

void CMDIConcrete::SetDRMReceiver(CDRMReceiver *pRx)
{
	/* Set the member variable holding pointer to the DRM receiver (for RSCI control functions)
	   I wanted to put this as a parameter to the constructor, but this gave a compiler warning 
	   because I wanted to pass the "this" pointer in the initialisation list for CDRMReceiver */
	pReceiver = pRx;

	// Also need to pass this onto the tag packet decoder so it can pass config changes to the receiver
	TagPacketDecoderRSCIControl.SetReceiver(pRx);
}
/******************************************************************************\
* MDI transmit                                                                 *
\******************************************************************************/
/* Access functions ***********************************************************/
void CMDIConcrete::SetFACData(_BOOLEAN bFACOK, CVectorEx<_BINARY>& vecbiFACData, CParameter& Parameter)
{
	/* Set unlocked count back to zero since we have received a FAC block */
	iUnlockedCount = 0;

	/* If an FAC block is ready, we send all OLD tags. The reason for that is
	   because an FAC block is always available if a DRM stream is connected
	   whereby the regular stream might not be present. Therefore we should use
	   the FAC blocks for synchronization of AF packets.
	   One problem is the SDC block. If we receive a new FAC block, the SDC of
	   the current DRM frame is already processed. Therefore it does not belong
	   to the OLD tags. To correct this, we introduced a flag showing that the
	   SDC block was received. In this case, delay the SDC tag and send it with
	   the next AF packet */

	/* Generate the rsta tag using the status flags collected during the frame */
	/* Note that the SDC flag is delayed by one frame by the CRSCIStatusFlags class */

	RSCIStatusFlags.SetSyncStatus(TRUE); /* If this function is called the Rx must be synchronised */
	TagItemGeneratorReceiverStatus.GenTag(RSCIStatusFlags);

	/* Generate some other tags */
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */

	if (bSDCOutWasSet == TRUE)
	{
		/* Do not send SDC packet with this AF packet, do not reset SDC tag
		   since we need it for the next AF packet */
		SendPacket(GenMDIPacket(FALSE));
		ResetTags(FALSE);

		/* Reset flag */
		bSDCOutWasSet = FALSE;
	}
	else
	{
		SendPacket(GenMDIPacket(TRUE));
		ResetTags(TRUE);
	}

	/* Now generate tags for the current DRM frame */
	TagItemGeneratorFAC.GenTag(bFACOK, vecbiFACData);

	TagItemGeneratorRobMod.GenTag(Parameter.GetWaveMode());

	/* TODO: move ERecMode to GlobalDefinitions.h so it can be passed around without needing */
	/* DRMReceiver.h. Then the line below can be included */

	//TagItemGeneratorRxDemodMode.GenTag(pReceiver->GetReceiverMode());

	/* SDC channel information tag must be created here because it must be sent
	   with each AF packet */
	TagItemGeneratorSDCChanInf.GenTag(Parameter);

	RSCIStatusFlags.Update(); /* Reset the status flags and delay the SDC flag by one frame */
	RSCIStatusFlags.SetFACStatus(bFACOK);

	/* RSCI tags ------------------------------------------------------------ */

	/* rser tag */
	TagItemGeneratorRxService.GenTag(TRUE, Parameter.GetCurSelAudioService());

#ifdef HAVE_LIBHAMLIB
	CReal rCurSigStr;
	pReceiver->GetHamlib()->GetSMeter(rCurSigStr);
	rCurSigStr += S9_DBUV;
	TagItemGeneratorSignalStrength.GenTag(TRUE, rCurSigStr);
#else
	TagItemGeneratorSignalStrength.GenTag(FALSE, 0);
#endif


// TODO

}

void CMDIConcrete::SendUnlockedFrame(CParameter& Parameter)
{
	/* This is called once per frame even if the receiver is unlocked */

	/* In the MDI profile, ignore this altogether since I assume there's no point */
	/* in generating empty packets with no reception monitoring information */
	if (cProfile == 'M')
		return;

	/* we will get one of these between each FAC block, and occasionally we */
	/* might get two, so don't start generating free-wheeling RSCI until we've. */
	/* had three in a row */
	if (iUnlockedCount < MAX_UNLOCKED_COUNT)
	{
		/* Increase the counter until we get to the limit */
		iUnlockedCount++;
		/* Don't generate an unlocked RSCI frame */
		return;
	}

	/* Send empty tags for most tag items */
	/* If we have only just lost lock, there might be some data from the previous frame to send */

	RSCIStatusFlags.SetSyncStatus(FALSE); /* If this function is called the Rx must be unlocked */
	TagItemGeneratorReceiverStatus.GenTag(RSCIStatusFlags);

	/* Generate some other tags */
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */

	if (bSDCOutWasSet == TRUE)
	{
		/* It's unlikely that we received the SDC in this frame, but send dummy just in case */
		SendPacket(GenMDIPacket(FALSE));

	}
	else
	{
		SendPacket(GenMDIPacket(TRUE));
	}
		/* Reset flag */
	bSDCOutWasSet = FALSE;

	/* This call will generate empty versions of most tags for next time */
	ResetTags(TRUE);

	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

	TagItemGeneratorSDCChanInf.GenEmptyTag();

	RSCIStatusFlags.Update(); /* Reset the status flags and delay the SDC flag by one frame */
	RSCIStatusFlags.SetFACStatus(FALSE);

	TagItemGeneratorReceiverStatus.GenTag(RSCIStatusFlags);

	RSCIStatusFlags.Update();

#ifdef HAVE_LIBHAMLIB
	CReal rCurSigStr;
	pReceiver->GetHamlib()->GetSMeter(rCurSigStr);
	// convert dB wrt S9 into dBuV
	rCurSigStr += S9_DBUV;
	TagItemGeneratorSignalStrength.GenTag(TRUE, rCurSigStr);

#else
	TagItemGeneratorSignalStrength.GenTag(FALSE, 0);
#endif

}


void CMDIConcrete::SetSDCData(_BOOLEAN bSDCOK, CVectorEx<_BINARY>& vecbiSDCData)
{
	TagItemGeneratorSDC.GenTag(bSDCOK, vecbiSDCData);
	RSCIStatusFlags.SetSDCStatus(bSDCOK);

	/* Set flag to identify special case */
	bSDCOutWasSet = TRUE;
}

void CMDIConcrete::SetStreamData(const int iStrNum, CVectorEx<_BINARY>& vecbiStrData)
{
	vecTagItemGeneratorStr[iStrNum].GenTag(vecbiStrData);
}

void CMDIConcrete::SetAudioFrameStatus(_BOOLEAN bIsValid, CVector<_BINARY>& vecbiAudioStatus)
{
	/* Generate rafs tag */
	TagItemGeneratorRAFS.GenTag(bIsValid, vecbiAudioStatus);

	/* set the audio flag to be used in the rsta tag */
	_BOOLEAN bAudioOK = TRUE;
	if (bIsValid)
	{
		vecbiAudioStatus.ResetBitAccess();
		
		for (int i=0; i<vecbiAudioStatus.Size(); i++)
			if (vecbiAudioStatus.Separate(1) == 1)
				bAudioOK = FALSE;
	}
	else
		bAudioOK = FALSE;

	RSCIStatusFlags.SetAudioStatus(bAudioOK);
}


void CMDIConcrete::SetMERs(const _REAL rRMER, const _REAL rRWMM, const _REAL rRWMF)
{
	TagItemGeneratorRWMF.GenTag(TRUE, rRWMF);
	TagItemGeneratorRWMM.GenTag(TRUE, rRWMM);
	TagItemGeneratorRMER.GenTag(TRUE, rRMER);
}


void CMDIConcrete::SetRDEL(const CRealVector &vecrThresholds, const CRealVector &vecrIntervals)
{
	TagItemGeneratorRDEL.GenTag(TRUE, vecrThresholds, vecrIntervals);
}

void CMDIConcrete::SetRDOP(const CReal rDoppler)
{
	TagItemGeneratorRDOP.GenTag(TRUE, rDoppler);
}

void CMDIConcrete::SetInterference(const CReal rIntFreq, const CReal rINR, const CReal rICR)
{
	TagItemGeneratorRINT.GenTag(TRUE, rIntFreq, rINR, rICR);
}

/* Network interface implementation *******************************************/
_BOOLEAN CMDIConcrete::SetNetwOutAddr(const string strNewIPPort)
{
	// Delegate to socket
	_BOOLEAN bAddressOK = PacketSocket.SetNetwOutAddr(strNewIPPort);

	// If successful, set flag to enable MDI output
	if (bAddressOK)
		SetEnableMDIOut(TRUE);

	return bAddressOK;
}

void CMDIConcrete::SendPacket(CVector<_BINARY> vecbiPacket)
{
	// Delegate to socket
	PacketSocket.SendPacket(vecbiPacket);
}

/* Actual MDI protocol implementation *****************************************/
CVector<_BINARY> CMDIConcrete::GenMDIPacket(const _BOOLEAN bWithSDC)
{
	/* Reset the tag packet generator */
	TagPacketGenerator.Reset();

	/* Set the RSCI/MDI profile */
	TagPacketGenerator.SetProfile(cProfile);

	/* Increment MDI packet counter and generate counter tag */
	TagItemGeneratorLoFrCnt.GenTag();

	/* Generate the rpro tag to indicate the current profile */
	TagItemGeneratorProfile.GenTag(cProfile);

	/* dlfc tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorLoFrCnt);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorLoFrCnt);

	/* *ptr tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorProTyMDI);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorProTyRSCI);

	/* dlfc tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorLoFrCnt);

	/* rpro tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorProfile);

	/* rdmo - note that this is currently empty */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRxDemodMode);

	/* ract */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRxActivated);

	/* rfre tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRxFrequency);

	/* fac_ tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorFAC);


	/* SDC tag must be delayed in some cases */
	if (bWithSDC == TRUE)
	{
		/* sdc_ tag */
		TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorSDC);
	}
	else
		/* empty sdc_ tag */
		TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorSDCEmpty);

	/* sdci tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorSDCChanInf);

	/* robm tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRobMod);

	/* strx tag */
	int j;
	for (j = 0; j < MAX_NUM_STREAMS; j++)
	{
		TagPacketGenerator.AddTagItemIfInProfile(&vecTagItemGeneratorStr[j]);
	}

	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorSignalStrength);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRAFS);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRMER);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRWMM);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRWMF);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRDEL);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRDOP);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorRINT);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorReceiverStatus);

	for (j = 0; j < MAX_NUM_STREAMS; j++)
	{
		TagPacketGenerator.AddTagItemIfInProfile(&vecTagItemGeneratorRBP[j]);
	}

	return TagPacketGenerator.GenAFPacket(bUseAFCRC);


}



void CMDIConcrete::ResetTags(const _BOOLEAN bResetSDC)
{
	/* Do not reset "*ptr" tag because this one is static */
	/* This group of tags are generated each time so don't need empty tags generated */
	TagItemGeneratorLoFrCnt.Reset(); /* dlfc tag */
	TagItemGeneratorFAC.Reset(); /* fac_ tag */
	TagItemGeneratorSDCChanInf.Reset(); /* sdci tag */
	TagItemGeneratorRobMod.Reset(); /* robm tag */
	TagItemGeneratorInfo.Reset(); /* info tag */
	TagItemGeneratorReceiverStatus.Reset(); /* rsta */

	TagItemGeneratorProfile.Reset(); /* rpro */

	/* This group of tags might not be generated, so make an empty version in case */
	
	TagItemGeneratorSignalStrength.GenEmptyTag(); /* rdbv tag */
	TagItemGeneratorRWMF.GenEmptyTag(); /* rwmf tag */
	TagItemGeneratorRWMM.GenEmptyTag(); /* rwmm tag */
	TagItemGeneratorRMER.GenEmptyTag(); /* rmer tag */
	TagItemGeneratorRDEL.GenEmptyTag(); /* rdel tag */
	TagItemGeneratorRDOP.GenEmptyTag(); /* rdop tag */
	TagItemGeneratorRINT.GenEmptyTag(); /* rint tag */
	TagItemGeneratorRAFS.GenEmptyTag(); /* rafs tag */

	/* Tags that are not fully implemented yet */
	TagItemGeneratorRxBandwidth.GenEmptyTag(); /* rbw_ */
	TagItemGeneratorRxService.GenEmptyTag(); /* rser */

	for (int i = 0; i < MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].Reset(); // strx tag 
		vecTagItemGeneratorRBP[i].GenEmptyTag(); // make empty version of mandatory tag that isn't implemented 
	}


	/* Make empty sdc_ tag to send while we delay the SDC */
	TagItemGeneratorSDCEmpty.GenEmptyTag();

	/* Special case with SDC block */
	if (bResetSDC == TRUE)
		//vecbiTagSDC.Init(0); /* sdc_ tag */
		TagItemGeneratorSDC.GenEmptyTag();
}


/******************************************************************************\
* MDI receive                                                                  *
\******************************************************************************/
/* Access functions ***********************************************************/
ERobMode CMDIConcrete::GetFACData(CVectorEx<_BINARY>& vecbiFACData)
{
	Mutex.Lock();

	/* Get new MDI data from buffer if it was not get by the SDC routine. Reset
	   the flag if it was set */
	if (bSDCInWasSet == FALSE)
		MDIInBuffer.Get(CurMDIPkt);
	else
		bSDCInWasSet = FALSE;

	if (CurMDIPkt.vecbiFACData.Size() > 0)
	{
		/* Copy incoming MDI FAC data */
		vecbiFACData.ResetBitAccess();
		CurMDIPkt.vecbiFACData.ResetBitAccess();

		/* FAC data is always 72 bits long which is 9 bytes, copy data
		   byte-wise */
		for (int i = 0; i < NUM_FAC_BITS_PER_BLOCK / SIZEOF__BYTE; i++)
		{
			vecbiFACData.Enqueue(CurMDIPkt.vecbiFACData.
				Separate(SIZEOF__BYTE), SIZEOF__BYTE);
		}
	}
	else
	{
		/* If no data is available, return cleared vector */
		vecbiFACData.Reset(0);
	}

	/* Set current robustness mode (should be done inside the mutex region) */
	const ERobMode eCurRobMode = CurMDIPkt.eRobMode;

	Mutex.Unlock();

	return eCurRobMode;
}

void CMDIConcrete::GetSDCData(CVectorEx<_BINARY>& vecbiSDCData)
{
	Mutex.Lock();

	/* If this is a DRM frame with SDC, get new MDI data from buffer and set
	   flag so that new data is not queried in FAC data routine */
	bSDCInWasSet = TRUE;
	MDIInBuffer.Get(CurMDIPkt);

	const int iLenBitsMDISDCdata = CurMDIPkt.vecbiSDCData.Size();
	if (iLenBitsMDISDCdata > 0)
	{
		/* If receiver is correctly initialized, the input vector should be
		   large enough for the SDC data */
		const int iLenSDCDataBits = vecbiSDCData.Size();

		if (iLenSDCDataBits >= iLenBitsMDISDCdata)
		{
			/* Copy incoming MDI SDC data */
			vecbiSDCData.ResetBitAccess();
			CurMDIPkt.vecbiSDCData.ResetBitAccess();

			/* We have to copy bits instead of bytes since the length of SDC
			   data is usually not a multiple of 8 */
			for (int i = 0; i < iLenBitsMDISDCdata; i++)
				vecbiSDCData.Enqueue(CurMDIPkt.vecbiSDCData.Separate(1), 1);
		}
	}
	else
	{
		/* If no data is available, return cleared vector */
		vecbiSDCData.Reset(0);
	}

	Mutex.Unlock();
}

void CMDIConcrete::GetStreamData(CVectorEx<_BINARY>& vecbiStrData, const int iLen,
						 const int iStrNum)
{
	Mutex.Lock();

	/* First check if stream ID is valid */
	if (iStrNum != STREAM_ID_NOT_USED)
	{
		/* Now check length of data vector */
		const int iStreamLen = CurMDIPkt.vecbiStr[iStrNum].Size();

		if (iLen >= iStreamLen)
		{
			/* Copy data */
			CurMDIPkt.vecbiStr[iStrNum].ResetBitAccess();
			vecbiStrData.ResetBitAccess();

			/* Data is always a multiple of 8 -> copy bytes */
			for (int i = 0; i < iStreamLen / SIZEOF__BYTE; i++)
			{
				vecbiStrData.Enqueue(CurMDIPkt.vecbiStr[iStrNum].
					Separate(SIZEOF__BYTE), SIZEOF__BYTE);
			}
		}
	}

	Mutex.Unlock();
}


/* Network interface implementation *******************************************/
_BOOLEAN CMDIConcrete::SetNetwInPort(const int iPort)
{
	/* If port number is set, MDI input will be enabled */
	SetEnableMDIIn(TRUE);

	// Delegate to socket
	_BOOLEAN bOK = PacketSocket.SetNetwInPort(iPort);

	if (bOK)
		// Connect socket to the MDI decoder
		PacketSocket.SetPacketSink(&TagPacketDecoderMDI);

	/* Initialize the listening socket. Host address is 0 -> "INADDR_ANY" */
	return bOK;
}

_BOOLEAN CMDIConcrete::SetNetwInMcast(const string strNewIPIP)
{
	// Delegate to socket
	return PacketSocket.SetNetwInMcast(strNewIPIP);

}


/* Network interface implementation for RSCI input *******************************************/
_BOOLEAN CMDIConcrete::SetNetwInPortRSCI(const int iPort)
{

	/* Don't enable MDI input: will still use analogue input */

	// Delegate to socket 
	_BOOLEAN bOK = PacketSocket.SetNetwInPort(iPort);
	if (bOK)
		PacketSocket.SetPacketSink(&TagPacketDecoderRSCIControl);
	return bOK;
}
