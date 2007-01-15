/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden, Andrew Murphy
 *
 * Description:
  *	Implements Digital Radio Mondiale (DRM) Multiplex Distribution Interface
 *	(MDI), Receiver Status and Control Interface (RSCI)  
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 820,  ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
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

#include "MDIRSCI.h"
#include "../DrmReceiver.h"

/* Implementation *************************************************************/
CRSIMDIOutRCIIn::CRSIMDIOutRCIIn() : iLogFraCnt(0),
	bMDIOutEnabled(FALSE), bMDIInEnabled(FALSE),
	vecTagItemGeneratorStr(MAX_NUM_STREAMS), vecTagItemGeneratorRBP(MAX_NUM_STREAMS)
{
	/* Initialise all the generators for strx and rbpx tags */
	for (int i=0; i<MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].SetStreamNumber(i);
		vecTagItemGeneratorRBP[i].SetStreamNumber(i);
	}

	/* Reset all tags for initialization */
	ResetTags();

	/* Init constant tag */
	TagItemGeneratorProTyMDI.GenTag();
	TagItemGeneratorProTyRSCI.GenTag();


	/* Default settings for the "special protocol settings" ----------------- */
	/* Use CRC for AF packets */
	bUseAFCRC = TRUE;
	/* default profile, otherwise using --rsiout with no --rsioutprofile generates empty
	 * AF packets
	 */
	cProfile = 'A';

}

/******************************************************************************\
* MDI transmit                                                                 *
\******************************************************************************/
/* Access functions ***********************************************************/
void CRSIMDIOutRCIIn::SendLockedFrame(CParameter& Parameter,
						CSingleBuffer<_BINARY>& FACData,
						CSingleBuffer<_BINARY>& SDCData,
						vector<CSingleBuffer<_BINARY> >& vecMSCData
)
{
	TagItemGeneratorFAC.GenTag(Parameter, FACData);
	TagItemGeneratorSDC.GenTag(Parameter, SDCData);
	for (size_t i = 0; i < vecMSCData.size(); i++)
	{
		vecTagItemGeneratorStr[i].GenTag(Parameter, vecMSCData[i]);
	}
	TagItemGeneratorRobMod.GenTag(Parameter.GetWaveMode());
	TagItemGeneratorRxDemodMode.GenTag(Parameter.GetReceiverMode());

	/* SDC channel information tag must be created here because it must be sent
	   with each AF packet */
	TagItemGeneratorSDCChanInf.GenTag(Parameter);

	TagItemGeneratorInfo.GenTag(Parameter.sReceiverID);	/* rinf */

	/* RSCI tags ------------------------------------------------------------ */
	TagItemGeneratorRAFS.GenTag(Parameter);
	TagItemGeneratorRWMF.GenTag(TRUE, Parameter.rWMERFAC); /* WMER for FAC */
	TagItemGeneratorRWMM.GenTag(TRUE, Parameter.rWMERMSC); /* WMER for MSC */
	TagItemGeneratorRMER.GenTag(TRUE, Parameter.rMER); /* MER for MSC */
	TagItemGeneratorRDEL.GenTag(TRUE, Parameter.vecrRdelThresholds, Parameter.vecrRdelIntervals);
	TagItemGeneratorRDOP.GenTag(TRUE, Parameter.rRdop);
	TagItemGeneratorRINT.GenTag(TRUE,Parameter.rIntFreq, Parameter.rINR, Parameter.rICR);
	TagItemGeneratorRxService.GenTag(TRUE, Parameter.GetCurSelAudioService());
	TagItemGeneratorReceiverStatus.GenTag(Parameter);
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */


	/* Generate some other tags */
	_REAL rSigStr = 0.0;
	_BOOLEAN bValid = Parameter.GetSignalStrength(rSigStr);
	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr + S9_DBUV);

	if (Parameter.GPSInformation.GetGPSSource() == CParameter::CGPSInformation::GPS_SOURCE_MANUAL_ENTRY)
		Parameter.GPSInformation.SetPositionAvailable(Parameter.GPSInformation.SetLatLongDegreesMinutes(Parameter.ReceptLog.GetLatitude(), Parameter.ReceptLog.GetLongitude()));
	
	TagItemGeneratorGPSInformation.GenTag(Parameter.GPSInformation.GetUse(), Parameter.GPSInformation);	/* rgps */
	
	CVector<_BINARY> packet = GenMDIPacket();
   	TransmitPacket(packet);
}

void CRSIMDIOutRCIIn::SendUnlockedFrame(CParameter& Parameter)
{
	/* This is called once per frame if the receiver is unlocked */

	/* In the MDI profile, ignore this altogether since I assume there's no point */
	/* in generating empty packets with no reception monitoring information */
	if (cProfile == 'M')
		return;

	/* Send empty tags for most tag items */
	ResetTags();

	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

	TagItemGeneratorSDCChanInf.GenEmptyTag();

	TagItemGeneratorReceiverStatus.GenTag(Parameter);

	/* Generate some other tags */
	TagItemGeneratorInfo.GenTag(Parameter.sReceiverID);	/* rinf */
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */
	_REAL rSigStr = 0.0;
	_BOOLEAN bValid = Parameter.GetSignalStrength(rSigStr);
	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr + S9_DBUV);

	if (Parameter.GPSInformation.GetGPSSource() == CParameter::CGPSInformation::GPS_SOURCE_MANUAL_ENTRY)
		Parameter.GPSInformation.SetPositionAvailable(Parameter.GPSInformation.SetLatLongDegreesMinutes(Parameter.ReceptLog.GetLatitude(), Parameter.ReceptLog.GetLongitude()));
	
	TagItemGeneratorGPSInformation.GenTag(Parameter.GPSInformation.GetUse(), Parameter.GPSInformation);	/* rgps */

	TransmitPacket(GenMDIPacket());
}

void CRSIMDIOutRCIIn::SendAMFrame(CParameter&)
{
}

void CRSIMDIOutRCIIn::SetReceiver(CDRMReceiver *pReceiver)
{
	TagPacketDecoderRSCIControl.SetReceiver(pReceiver);
}

/* Actual MDI protocol implementation *****************************************/
CVector<_BINARY> CRSIMDIOutRCIIn::GenMDIPacket()
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

	/* *ptr tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorProTyMDI);
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorProTyRSCI);

	/* rinf taf */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorInfo);

	/* rgps tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorGPSInformation);

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

	/* sdc_ tag */
	TagPacketGenerator.AddTagItemIfInProfile(&TagItemGeneratorSDC);

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

void CRSIMDIOutRCIIn::ResetTags()
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
	TagItemGeneratorGPSInformation.Reset();	/* rgps */

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

	TagItemGeneratorSDC.GenEmptyTag();
}

void CRSIMDIOutRCIIn::GetNextPacket(CSingleBuffer<_BINARY>& buf)
{
	// TODO 
	(void)buf;
}

void CRSIMDIOutRCIIn::SetInAddr(const string& strAddr)
{
	/* If port number is set, MDI input will be enabled */
	bMDIInEnabled = TRUE;

#ifdef USE_QT_GUI
	// Delegate to socket
	_BOOLEAN bOK = PacketSocket.SetNetwInAddr(strAddr);

	if (bOK)
		// Connect socket to the MDI decoder
		PacketSocket.SetPacketSink(this);
#endif
}

void CRSIMDIOutRCIIn::SetOutAddr(const string& strArgument)
{
	_BOOLEAN bAddressOK = TRUE;
	// Delegate to socket
#ifdef USE_QT_GUI
	bAddressOK = PacketSocket.SetNetwOutAddr(strArgument);
#endif

	// If successful, set flag to enable MDI output
	if (bAddressOK)
		SetEnableMDIOut(TRUE);
}

void CRSIMDIOutRCIIn::SetProfile(const char c)
{
	cProfile = c;
}

#ifdef USE_QT_GUI
void CRSIMDIOutRCIIn::SendPacket(const vector<_BYTE>& vecbydata)
{
	CVectorEx<_BINARY> vecbidata;
	vecbidata.Init(vecbydata.size()*SIZEOF__BYTE);
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<vecbydata.size(); i++)
		vecbidata.Enqueue(vecbydata[i], SIZEOF__BYTE);
	CTagPacketDecoder::Error err = 
		TagPacketDecoderRSCIControl.DecodeAFPacket(vecbidata);
	if(err != CTagPacketDecoder::E_OK)
		cerr << "bad RSCI Control Packet Received" << endl;
}
#endif

void CRSIMDIOutRCIIn::TransmitPacket(CVector<_BINARY> vecbidata)
{
	vector<_BYTE> packet;
	vecbidata.ResetBitAccess();
	size_t bits = vecbidata.Size();
	size_t bytes = bits / SIZEOF__BYTE;
	packet.reserve(bytes);
	for(size_t i=0; i<bytes; i++)
	{
	 	_BYTE byte = (_BYTE)vecbidata.Separate(SIZEOF__BYTE);
		packet.push_back(byte);
	}
#ifdef USE_QT_GUI
	PacketSocket.SendPacket(packet);
#endif
}

/******************************************************************************\
* MDI receive                                                                  *
\******************************************************************************/
CRSIMDIInRCIOut::CRSIMDIInRCIOut() :
	bUseAFCRC(TRUE), bMDIOutEnabled(FALSE), bMDIInEnabled(FALSE)
{
	/* Init constant tag */
	TagItemGeneratorProTyRSCI.GenTag();
}

void CRSIMDIInRCIOut::SetInAddr(const string& strAddr)
{
	/* If port number is set, MDI input will be enabled */
	bMDIInEnabled = TRUE;
#ifdef USE_QT_GUI
	// Delegate to socket
	_BOOLEAN bOK = PacketSocket.SetNetwInAddr(strAddr);

	if (bOK)
		// Connect socket to the MDI decoder
		PacketSocket.SetPacketSink(this);
#endif
}

void CRSIMDIInRCIOut::SetOutAddr(const string& strArgument)
{
	_BOOLEAN bAddressOK = TRUE;
	// Delegate to socket
#ifdef USE_QT_GUI
	bAddressOK = PacketSocket.SetNetwOutAddr(strArgument);
#endif
	if(bAddressOK)
		bMDIOutEnabled = TRUE;
}

_BOOLEAN CRSIMDIInRCIOut::SetFrequency(int iNewFreqkHz)
{
	TagPacketGenerator.Reset();
	TagItemGeneratorCfre.GenTag(iNewFreqkHz);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCfre);
	CVector<_BINARY> packet = TagPacketGenerator.GenAFPacket(bUseAFCRC);
   	TransmitPacket(packet);
	return TRUE;
}

/* bits to bytes and send */

void CRSIMDIInRCIOut::TransmitPacket(CVector<_BINARY>& vecbidata)
{
	vector<_BYTE> packet;
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<size_t(vecbidata.Size()/SIZEOF__BYTE); i++)
		packet.push_back(vecbidata.Separate(SIZEOF__BYTE));
#ifdef USE_QT_GUI
	PacketSocket.SendPacket(packet);
#endif
}

#ifdef USE_QT_GUI
void CRSIMDIInRCIOut::SendPacket(const vector<_BYTE>& vecbydata)
{
	if(vecbydata[0]=='P')
	{
		vector<_BYTE> vecOut;
		if(Pft.DecodePFTPacket(vecbydata, vecOut))
		{
			queue.Put(vecOut);
		}
	}
	else
		queue.Put(vecbydata);
}
#endif

void CRSIMDIInRCIOut::InitInternal(CParameter&)
{
	iInputBlockSize = 1; /* anything is enough but not zero */
	iMaxOutputBlockSize = 2048*SIZEOF__BYTE; /* bigger than an ethernet packet */
}

void CRSIMDIInRCIOut::ProcessDataInternal(CParameter&)
{
	vector<_BYTE> vecbydata;
#ifdef USE_QT_GUI
	queue.Get(vecbydata);
#else
	// use a select here
#endif
	iOutputBlockSize = vecbydata.size()*SIZEOF__BYTE;
	pvecOutputData->Init(iOutputBlockSize);
	pvecOutputData->ResetBitAccess();
	for(size_t i=0; i<size_t(iOutputBlockSize/SIZEOF__BYTE); i++)
		pvecOutputData->Enqueue(vecbydata[i], SIZEOF__BYTE);
}
