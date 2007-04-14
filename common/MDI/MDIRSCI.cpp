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
#ifdef USE_QT_GUI
#  include "PacketSocketQT.h"
#  include "PacketSourceFile.h"
#else
# include "PacketSocketNull.h"
#endif
#include <sstream>
#include <iomanip>
#include <qhostaddress.h>

/* Implementation *************************************************************/
CDownstreamDI::CDownstreamDI() : iLogFraCnt(0),
	bMDIOutEnabled(FALSE), bMDIInEnabled(FALSE),bIsRecording(FALSE),
	vecTagItemGeneratorStr(MAX_NUM_STREAMS), vecTagItemGeneratorRBP(MAX_NUM_STREAMS),
	RSISubscribers(),pRSISubscriberFile(new CRSISubscriberFile)
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

	/* Add the file subscriber to the list of subscribers */
	RSISubscribers[""] = pRSISubscriberFile;

}

CDownstreamDI::~CDownstreamDI() 
{
	for(map<string,CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
	{
		delete i->second;
	}
}

/******************************************************************************\
* DI send status, receive control                                             *
\******************************************************************************/
/* Access functions ***********************************************************/
void CDownstreamDI::SendLockedFrame(CParameter& Parameter,
						CSingleBuffer<_BINARY>& FACData,
						CSingleBuffer<_BINARY>& SDCData,
						vector<CSingleBuffer<_BINARY> >& vecMSCData
)
{
	TagItemGeneratorFAC.GenTag(Parameter, FACData);
	TagItemGeneratorSDC.GenTag(Parameter, SDCData);
	//for (size_t i = 0; i < vecMSCData.size(); i++)
	for (size_t i = 0; i < MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].GenTag(Parameter, vecMSCData[i]);
	}
	TagItemGeneratorRobMod.GenTag(Parameter.GetWaveMode());
	TagItemGeneratorRxDemodMode.GenTag(Parameter.GetReceiverMode());

	/* SDC channel information tag must be created here because it must be sent
	   with each AF packet */
	TagItemGeneratorSDCChanInf.GenTag(Parameter);

	TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */

	/* RSCI tags ------------------------------------------------------------ */
	TagItemGeneratorRAFS.GenTag(Parameter);
	TagItemGeneratorRWMF.GenTag(TRUE, Parameter.rWMERFAC); /* WMER for FAC */
	TagItemGeneratorRWMM.GenTag(TRUE, Parameter.rWMERMSC); /* WMER for MSC */
	TagItemGeneratorRMER.GenTag(TRUE, Parameter.rMER); /* MER for MSC */
	TagItemGeneratorRDEL.GenTag(TRUE, Parameter.vecrRdelThresholds, Parameter.vecrRdelIntervals);
	TagItemGeneratorRDOP.GenTag(TRUE, Parameter.rRdop);
	TagItemGeneratorRINT.GenTag(TRUE,Parameter.rIntFreq, Parameter.rINR, Parameter.rICR);
	TagItemGeneratorRNIP.GenTag(TRUE,Parameter.rMaxPSDFreq, Parameter.rMaxPSDwrtSig);
	TagItemGeneratorRxService.GenTag(TRUE, Parameter.GetCurSelAudioService());
	TagItemGeneratorReceiverStatus.GenTag(Parameter);
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */
	TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);
	TagItemGeneratorPilots.GenTag(Parameter);

	/* Generate some other tags */
	_REAL rSigStr = 0.0;
	_BOOLEAN bValid = Parameter.GetSignalStrength(rSigStr);
	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr + S9_DBUV);

	TagItemGeneratorGPS.GenTag(TRUE, Parameter.ReceptLog.GPSData);	// rgps
	
	GenDIPacket();
}

void CDownstreamDI::SendUnlockedFrame(CParameter& Parameter)
{
	/* This is called once per frame if the receiver is unlocked */

	/* In the MDI profile, we used to ignore this altogether since "I assume there's no point */
	/* in generating empty packets with no reception monitoring information" */
	/* But now there could be multiple profiles at the same time. TODO: decide what to do! */
/*	if (cProfile == 'M')
		return;*/

	/* Send empty tags for most tag items */
	ResetTags();

	TagItemGeneratorFAC.GenEmptyTag();
	TagItemGeneratorSDC.GenEmptyTag();

	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

	TagItemGeneratorRxDemodMode.GenTag(Parameter.GetReceiverMode());

	TagItemGeneratorSDCChanInf.GenEmptyTag();

	TagItemGeneratorReceiverStatus.GenTag(Parameter);

	TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);

	TagItemGeneratorPilots.GenEmptyTag();

	TagItemGeneratorRNIP.GenTag(TRUE,Parameter.rMaxPSDFreq, Parameter.rMaxPSDwrtSig);

	/* Generate some other tags */
	TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */
	_REAL rSigStr = 0.0;
	_BOOLEAN bValid = Parameter.GetSignalStrength(rSigStr);
	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr + S9_DBUV);

	TagItemGeneratorGPS.GenTag(TRUE, Parameter.ReceptLog.GPSData);	/* rgps */

	GenDIPacket();
}

void CDownstreamDI::SendAMFrame(CParameter& Parameter, CSingleBuffer<_BINARY>& CodedAudioData)
{
		/* This is called once per 400ms if the receiver is in AM mode */

	/* In the MDI profile, ignore this altogether since there's no DRM information */
	/*if (cProfile == 'M')
		return;*/

	/* Send empty tags for most tag items */
	ResetTags();

	TagItemGeneratorFAC.GenEmptyTag();
	TagItemGeneratorSDC.GenEmptyTag();
	/* mode is unknown - make empty robm tag */
	TagItemGeneratorRobMod.GenEmptyTag();

	/* demod mode */
	TagItemGeneratorRxDemodMode.GenTag(Parameter.GetReceiverMode());

	TagItemGeneratorSDCChanInf.GenEmptyTag();

	/* These will be set appropriately when the rx is put into AM mode */
	/* We need to decide what "appropriate" settings are */
	TagItemGeneratorReceiverStatus.GenTag(Parameter); 

	TagItemGeneratorPowerSpectralDensity.GenTag(Parameter);

	TagItemGeneratorPilots.GenEmptyTag();

	TagItemGeneratorRNIP.GenTag(TRUE, Parameter.rMaxPSDFreq, Parameter.rMaxPSDwrtSig);

	// Generate a rama tag with the encoded audio data
	TagItemGeneratorAMAudio.GenTag(Parameter, CodedAudioData);

	/* Generate some other tags */
	TagItemGeneratorRINF.GenTag(Parameter.sReceiverID);	/* rinf */
	TagItemGeneratorRxFrequency.GenTag(TRUE, Parameter.ReceptLog.GetFrequency()); /* rfre */
	TagItemGeneratorRxActivated.GenTag(TRUE); /* ract */
	_REAL rSigStr = 0.0;
	_BOOLEAN bValid = Parameter.GetSignalStrength(rSigStr);
	TagItemGeneratorSignalStrength.GenTag(bValid, rSigStr + S9_DBUV);

	TagItemGeneratorGPS.GenTag(TRUE, Parameter.ReceptLog.GPSData);	/* rgps */

	GenDIPacket();
}

void CDownstreamDI::SetReceiver(CDRMReceiver *pReceiver)
{
	/* use the File subscriber to handle RSCI commands */
	pRSISubscriberFile->SetReceiver(pReceiver);
}

/* Actual DRM DI protocol implementation *****************************************/
void CDownstreamDI::GenDIPacket()
{
	/* Reset the tag packet generator */
	TagPacketGenerator.Reset();

	/* Set the RSCI/MDI profile */
	TagPacketGenerator.SetProfile(cProfile);

	/* Increment MDI packet counter and generate counter tag */
	TagItemGeneratorLoFrCnt.GenTag();

	/* dlfc tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorLoFrCnt);

	/* *ptr tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyMDI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);

	/* rinf taf */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRINF);

	/* rgps tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorGPS);

	/* rpro tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProfile);

	/* rdmo - note that this is currently empty */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxDemodMode);

	/* ract */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxActivated);

	/* rfre tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRxFrequency);

	/* fac_ tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorFAC);

	/* sdc_ tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorSDC);

	/* sdci tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorSDCChanInf);

	/* robm tag */
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRobMod);

	/* strx tag */
	size_t i;
	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		TagPacketGenerator.AddTagItem(&vecTagItemGeneratorStr[i]);
	}
	TagPacketGenerator.AddTagItem(&TagItemGeneratorAMAudio);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorSignalStrength);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRAFS);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRMER);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRWMM);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRWMF);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRDEL);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRDOP);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRINT);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorRNIP);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorReceiverStatus);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorPowerSpectralDensity);

	TagPacketGenerator.AddTagItem(&TagItemGeneratorPilots);


	for (i = 0; i < MAX_NUM_STREAMS; i++)
	{
		TagPacketGenerator.AddTagItem(&vecTagItemGeneratorRBP[i]);
	}

	/*return TagPacketGenerator.GenAFPacket(bUseAFCRC);*/

	/* transmit a packet to each subscriber */
	for(map<string,CRSISubscriber*>::iterator s = RSISubscribers.begin();
			s!=RSISubscribers.end(); s++)
	{
		// re-generate the profile tag for each subscriber
		TagItemGeneratorProfile.GenTag(s->second->GetProfile());
		s->second->TransmitPacket(&TagPacketGenerator);
	}
}

void CDownstreamDI::ResetTags()
{
	/* Do not reset "*ptr" tag because this one is static */
	/* This group of tags are generated each time so don't need empty tags generated */
	TagItemGeneratorLoFrCnt.Reset(); /* dlfc tag */
	TagItemGeneratorFAC.Reset(); /* fac_ tag */
	TagItemGeneratorSDCChanInf.Reset(); /* sdci tag */
	TagItemGeneratorRobMod.Reset(); /* robm tag */
	TagItemGeneratorRINF.Reset(); /* info tag */
	TagItemGeneratorReceiverStatus.Reset(); /* rsta */

	TagItemGeneratorProfile.Reset(); /* rpro */
	TagItemGeneratorGPS.Reset();	/* rgps */

	TagItemGeneratorPowerSpectralDensity.Reset();
	TagItemGeneratorPilots.Reset();

	/* This group of tags might not be generated, so make an empty version in case */
	
	TagItemGeneratorSignalStrength.GenEmptyTag(); /* rdbv tag */
	TagItemGeneratorRWMF.GenEmptyTag(); /* rwmf tag */
	TagItemGeneratorRWMM.GenEmptyTag(); /* rwmm tag */
	TagItemGeneratorRMER.GenEmptyTag(); /* rmer tag */
	TagItemGeneratorRDEL.GenEmptyTag(); /* rdel tag */
	TagItemGeneratorRDOP.GenEmptyTag(); /* rdop tag */
	TagItemGeneratorRINT.GenEmptyTag(); /* rint tag */
	TagItemGeneratorRNIP.GenEmptyTag(); /* rnip tag */
	TagItemGeneratorRAFS.GenEmptyTag(); /* rafs tag */

	/* Tags that are not fully implemented yet */
	TagItemGeneratorRxBandwidth.GenEmptyTag(); /* rbw_ */
	TagItemGeneratorRxService.GenEmptyTag(); /* rser */

	for (int i = 0; i < MAX_NUM_STREAMS; i++)
	{
		vecTagItemGeneratorStr[i].Reset(); // strx tag 
		vecTagItemGeneratorRBP[i].GenEmptyTag(); // make empty version of mandatory tag that isn't implemented 
	}

	TagItemGeneratorAMAudio.Reset(); // don't make the tag in DRM mode

	TagItemGeneratorSDC.GenEmptyTag();
}

void CDownstreamDI::GetNextPacket(CSingleBuffer<_BINARY>&)
{
	// TODO 
}

_BOOLEAN CDownstreamDI::SetOrigin(const string& strAddr)
{
	/* only allow one listening address */
	if(bMDIInEnabled == TRUE)
		return FALSE;

	if(source == NULL)
	{
#ifdef USE_QT_GUI
		source = new CPacketSocketQT;
#endif
	}

	if(source == NULL)
		return FALSE;

	// Delegate to socket
	_BOOLEAN bOK = source->SetOrigin(strAddr);

	if (bOK)
	{
		source->SetPacketSink(this);
		bMDIInEnabled = TRUE;
		return TRUE;
	}
	return FALSE;
}

_BOOLEAN CDownstreamDI::SetDestination(const string& str)
{
	/* allow multiple destinations, allow destinations to send cpro instructions back */

	if (RSISubscribers.size() <= MAX_NUM_RSI_SUBSCRIBERS)
	{
		CRSISubscriberSocket* p = new CRSISubscriberSocket(NULL);
		_BOOLEAN bAddressOK = p->SetDestination(str);
		// If successful, set flag to enable MDI output
		if (bAddressOK)
		{
			string a;
			bMDIOutEnabled = TRUE;
			if(p->GetDestination(a))
			{
				/* TODO incoporate port */
				RSISubscribers[a] = p;
			}
			return TRUE;
		}
		else
			delete p;
	}
	return FALSE;
}

_BOOLEAN CDownstreamDI::GetDestination(string& str)
{
	return FALSE; // makes no sense 
}

void CDownstreamDI::SetProfile(char c)
{
	/* set profile for all subscribers with no profile */
	for(map<string,CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
	{
		if(i->second->GetProfile() == 0)
			i->second->SetProfile(c);
	}
	cProfile = c;
}

void CDownstreamDI::SetAFPktCRC(const _BOOLEAN bNAFPktCRC)
{
	for(map<string,CRSISubscriber*>::iterator i = RSISubscribers.begin();
			i!=RSISubscribers.end(); i++)
			i->second->SetAFPktCRC(bNAFPktCRC);
}

string CDownstreamDI::GetRSIfilename(CParameter& Parameter)
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	iFrequency = Parameter.ReceptLog.GetFrequency(); // remember this for later

	stringstream filename;
	filename << Parameter.sDataFilesDirectory;
	filename << Parameter.sReceiverID << "_";
	filename << setw(4) << setfill('0') << gmtCur->tm_year + 1900 << "-" << setw(2) << setfill('0')<< gmtCur->tm_mon + 1;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_mday << "_";
	filename << setw(2) << setfill('0') << gmtCur->tm_hour << "-" << setw(2) << setfill('0')<< gmtCur->tm_min;
	filename << "-" << setw(2) << setfill('0')<< gmtCur->tm_sec << "_";
	filename << setw(8) << setfill('0') << (iFrequency*1000) << ".rs" << char(toupper(cProfile));
	return filename.str();
}

void CDownstreamDI::SetRSIRecording(CParameter& Parameter, const _BOOLEAN bOn, char cPro)
{
	if (bOn)
	{
		pRSISubscriberFile->SetProfile(cPro);

		pRSISubscriberFile->SetDestination(GetRSIfilename(Parameter));
		pRSISubscriberFile->StartRecording();
		bMDIOutEnabled = TRUE;
		bIsRecording = TRUE;
	}
	else
	{
		pRSISubscriberFile->StopRecording();
		bIsRecording = FALSE;
	}
}

 /* needs to be called in case a new RSCI file needs to be started */
void CDownstreamDI::NewFrequency(CParameter& Parameter)
{
	/* Has it really changed? */
	int iNewFrequency = Parameter.ReceptLog.GetFrequency();

	if (iNewFrequency != iFrequency)
	{
		if (bIsRecording)
		{
			pRSISubscriberFile->StopRecording();
			pRSISubscriberFile->SetDestination(GetRSIfilename(Parameter));
			pRSISubscriberFile->StartRecording();
		}
	}
}

/* this gets called with incoming RSCI Control packets.
 * Send it on to the RSISubscriber. This probably doesn't have the receiver pointer
 * but it can use it to let the far end control the profile
 * so send it to the File subscriber as well which we know does have the pointer.
 */

void CDownstreamDI::SendPacket(const vector<_BYTE>& vecbydata, uint32_t addr, uint16_t port)
{
	stringstream key;
	QHostAddress a(addr);
	key << a.toString() << ":" << port;
	map<string,CRSISubscriber*>::iterator s = RSISubscribers.find(key.str());
	if(s != RSISubscribers.end())
		s->second->SendPacket(vecbydata);
	pRSISubscriberFile->SendPacket(vecbydata);
}

/******************************************************************************\
* DI receive status, send control                                             * 
\******************************************************************************/
CUpstreamDI::CUpstreamDI() : source(NULL), sink(NULL), bUseAFCRC(TRUE), bMDIOutEnabled(FALSE), bMDIInEnabled(FALSE)
{
	/* Init constant tag */
	TagItemGeneratorProTyRSCI.GenTag();
}

CUpstreamDI::~CUpstreamDI()
{
	if(source)
	{
		if(sink && (void*)sink != (void*)source)
			delete sink;
		delete source;
	}
}

_BOOLEAN CUpstreamDI::SetOrigin(const string& str)
{
	/* only allow one listening address */
	if(bMDIInEnabled == TRUE)
		return FALSE;

	if(source)
		return FALSE;

	strOrigin = str;

	// try a socket
#ifdef USE_QT_GUI
	source = new CPacketSocketQT;
#else
	source = new CPacketSocketNull;
#endif

	// Delegate to socket
	_BOOLEAN bOK = source->SetOrigin(str);

	if(!bOK)
	{
		// try a file
		delete source;
		source = NULL;
#ifdef USE_QT_GUI
		source = new CPacketSourceFile;
		bOK = source->SetOrigin(str);
#endif
	}
	if (bOK)
	{
		source->SetPacketSink(this);
		bMDIInEnabled = TRUE;
		return TRUE;
	}
	return FALSE;
}

_BOOLEAN CUpstreamDI::SetDestination(const string& str)
{
	strDestination = str;
	if(sink==NULL)
	{
#ifdef USE_QT_GUI
		sink = new CPacketSocketQT;
#endif
	}

	if(sink==NULL)
		return FALSE;

	bMDIOutEnabled = sink->SetDestination(strDestination);

	return bMDIOutEnabled;
}

_BOOLEAN CUpstreamDI::GetDestination(string& str)
{
	if(sink)
		return sink->GetDestination(str);
	return FALSE;
}

void
CUpstreamDI::doSendPacket(const vector<_BYTE>& vecbydata)
{
	if(bMDIOutEnabled)
	{
		sink->SendPacket(vecbydata);
	}
}

void CUpstreamDI::SetFrequency(int iNewFreqkHz)
{
	TagPacketGenerator.Reset();
	TagItemGeneratorCfre.GenTag(iNewFreqkHz);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCfre);
	CVector<_BYTE> packet = AFPacketGenerator.GenAFPacket(bUseAFCRC, &TagPacketGenerator);
	doSendPacket(packet);
}

void CUpstreamDI::SetReceiverMode(ERecMode eNewMode)
{
	TagPacketGenerator.Reset();
	TagItemGeneratorCdmo.GenTag(eNewMode);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorProTyRSCI);
	TagPacketGenerator.AddTagItem(&TagItemGeneratorCdmo);
	CVector<_BYTE> packet = AFPacketGenerator.GenAFPacket(bUseAFCRC, &TagPacketGenerator);
	doSendPacket(packet);
}

/* bits to bytes and send */

void CUpstreamDI::TransmitPacket(CVector<_BINARY>& vecbidata)
{
	vector<_BYTE> packet;
	vecbidata.ResetBitAccess();
	for(size_t i=0; i<size_t(vecbidata.Size()/SIZEOF__BYTE); i++)
		packet.push_back(_BYTE(vecbidata.Separate(SIZEOF__BYTE)));
	doSendPacket(packet);
}

/* we only support one upstream RSCI source, so ignore the source address */
void CUpstreamDI::SendPacket(const vector<_BYTE>& vecbydata, uint32_t, uint16_t)
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

void CUpstreamDI::InitInternal(CParameter&)
{
	iInputBlockSize = 1; /* anything is enough but not zero */
	iMaxOutputBlockSize = 2048*SIZEOF__BYTE; /* bigger than an ethernet packet */
}

void CUpstreamDI::ProcessDataInternal(CParameter&)
{
	vector<_BYTE> vecbydata;
	queue.Get(vecbydata);
	iOutputBlockSize = vecbydata.size()*SIZEOF__BYTE;
	pvecOutputData->Init(iOutputBlockSize);
	pvecOutputData->ResetBitAccess();
	for(size_t i=0; i<size_t(iOutputBlockSize/SIZEOF__BYTE); i++)
		pvecOutputData->Enqueue(vecbydata[i], SIZEOF__BYTE);
}
