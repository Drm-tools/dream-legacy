/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	DRM-transmitter
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

#include "MDI/MDIRSCI.h" /* OPH: need this near the top so winsock2 is included before winsock */
#include "MDI/MDIDecode.h"
#include "DrmTransmitter.h"
#include "mlc/MLC.h"
#include <queue>

#define MIN_MDI_INPUT_BUFFERED_FRAMES 0

/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter():
	TransmParam(),
	strMDIinAddr(), MDIoutAddr(), COFDMOutputs(),
	eOpMode(T_TX),
	Encoder(), Modulator(),
	MDIIn(), DecodeMDI(), MDIOut()
{
	/* Init streams */
	TransmParam.ResetServicesStreams();

	/* Init frame ID counter (index) */
	TransmParam.iFrameIDTransm = 0;

	/* Date, time. TODO: use computer system time... */
	TransmParam.iDay = 0;
	TransmParam.iMonth = 0;
	TransmParam.iYear = 0;
	TransmParam.iUTCHour = 0;
	TransmParam.iUTCMin = 0;
}

void
CDRMTransmitter::CalculateChannelCapacities(CParameter& Parameters)
{
	CSingleBuffer<_COMPLEX>	DummyBuf[1];
	CMSCMLCEncoder			MSCMLCEncoder;
	CSDCMLCEncoder			SDCMLCEncoder;
	SDCMLCEncoder.Init(Parameters, DummyBuf);
	MSCMLCEncoder.Init(Parameters, DummyBuf);
}

void
CDRMTransmitter::SetOperatingMode(const ETxOpMode eNewOpMode)
{
	eOpMode = eNewOpMode;
}

CDRMTransmitter::ETxOpMode
CDRMTransmitter::GetOperatingMode()
{
	return eOpMode;
}

void
CDRMTransmitter::
GetSoundInChoices(vector<string>& v)
{
	Encoder.GetSoundInChoices(v);
}

void
CDRMTransmitter::
GetSoundOutChoices(vector<string>& v)
{
	Modulator.GetSoundOutChoices(v);
}

void
CDRMTransmitter::
SetSoundInInterface(int i)
{
	Encoder.SetSoundInInterface(i);
}

_REAL CDRMTransmitter::GetLevelMeter()
{
	return Encoder.GetLevelMeter();
}

void
CDRMTransmitter::AddTextMessage(const string& strText)
{
	Encoder.AddTextMessage(strText);
}

void
CDRMTransmitter::ClearTextMessages()
{
	Encoder.ClearTextMessages();
}

void
CDRMTransmitter::AddPic(const string& strFileName, const string& strFormat)
{
	Encoder.AddPic(strFileName, strFormat);
}

void
CDRMTransmitter::ClearPics()
{
	Encoder.ClearPics();
}

_BOOLEAN
CDRMTransmitter::GetTransStat(string& strCPi, _REAL& rCPe)
{
	return Encoder.GetTransStat(strCPi, rCPe);
}

void
CDRMTransmitter::SetReadFromFile(const string & strNFN)
{
	Encoder.SetReadFromFile(strNFN);
}

void
CDRMTransmitter::Stop()
{
	TransmParam.bRunThread = FALSE;
}

void CDRMTransmitter::Start()
{

	bool bInSync = true;

	switch(eOpMode)
	{
	case T_TX:
		TransmParam.ReceiveStatus.FAC.SetStatus(RX_OK);
		TransmParam.ReceiveStatus.SDC.SetStatus(RX_OK);
		Encoder.Init(TransmParam, MDIBuf);
		Modulator.Init(TransmParam, COFDMOutputs);
		break;
	case T_ENC:
		if(MDIoutAddr.size()==0)
			throw CGenErr("Encoder with no outputs");
		TransmParam.ReceiveStatus.FAC.SetStatus(RX_OK);
		TransmParam.ReceiveStatus.SDC.SetStatus(RX_OK);
		Encoder.Init(TransmParam, MDIBuf);
		{
			/* set the output address */
			for(vector<string>::const_iterator s = MDIoutAddr.begin(); s!=MDIoutAddr.end(); s++)
				MDIOut.AddSubscriber(*s, "", 'M');
		}
		break;
	case T_MOD:
		if(strMDIinAddr != "")
		{
			MDIIn.SetOrigin(strMDIinAddr);
		}
		else
			throw CGenErr("Modulator with no input");
		bInSync = false;
	}

	/* Set run flag */
	TransmParam.bRunThread = TRUE;
    cout << "Tx: starting, in:" << (MDIIn.GetInEnabled()?"MDI":"Encoder")
         << ", out: " << (MDIOut.GetOutEnabled()?"MDI":"")
         << ", " << ((COFDMOutputs.size()>0)?"COFDM":"")
         << endl; cout.flush();

	try
	{
		while (TransmParam.bRunThread)
		{
			if(bInSync)
			{
				if(eOpMode == T_MOD)
				{
					MDIPacketBuf.Clear();
					MDIIn.ReadData(TransmParam, &MDIPacketBuf);
					cerr << "MDIPacketBuf.GetFillLevel " << MDIPacketBuf.GetFillLevel() << endl;
					DecodeMDI.ProcessData(TransmParam, &MDIPacketBuf, MDIBuf);
				}
				else
				{
					Encoder.ReadData(TransmParam, MDIBuf);
				}

				if(eOpMode == T_ENC)
				{
					MDIOut.WriteData(TransmParam, MDIBuf);
				}
				else
				{
					Modulator.WriteData(TransmParam, MDIBuf);
					/* TODO - set bInSync false on error */
				}
			}
			else
			{
				SyncWithMDI(TransmParam);
				if(TransmParam.bRunThread == FALSE)
					break;
				cerr << "Got SDCI & FAC" << endl;
				MDIIn.Init(TransmParam, &MDIPacketBuf);
				DecodeMDI.Init(TransmParam, MDIBuf);
				Modulator.Init(TransmParam, COFDMOutputs);
				cerr << "Modulator initialised from MDI with"
					<< " MSC mode " << int(TransmParam.eMSCCodingScheme)
					<< " SDC mode " << int(TransmParam.eSDCCodingScheme)
					<< " robm " << int(TransmParam.GetWaveMode())
					<< " spectrum occupancy " << int(TransmParam.GetSpectrumOccup()) << endl;
				bInSync = true;
			}
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	if(eOpMode != T_ENC)
		Modulator.Cleanup(TransmParam);
	if(eOpMode != T_MOD)
		Encoder.Cleanup(TransmParam);
}

void
CDRMTransmitter::SyncWithMDI(CParameter& TransmParam)
{
	MDIIn.Init(TransmParam, &MDIPacketBuf);
	DecodeMDI.Init(TransmParam, MDIBuf);
	while (TransmParam.bRunThread)
	{
		MDIPacketBuf.SetRequestFlag(TRUE); // should we need this? TODO
		MDIIn.ReadData(TransmParam, &MDIPacketBuf);
		cerr << "MDIPacketBuf.Fill " << MDIPacketBuf.GetFillLevel() << endl;
		MDIBuf[0].SetRequestFlag(TRUE); MDIBuf[1].SetRequestFlag(TRUE); MDIBuf[2].SetRequestFlag(TRUE);
		DecodeMDI.ProcessData(TransmParam, &MDIPacketBuf, MDIBuf);
		cerr << "MDIBuf Fill " << MDIBuf[0].GetFillLevel() << " " << MDIBuf[1].GetFillLevel() << " " << MDIBuf[2].GetFillLevel() << endl;
		if(MDIBuf[0].GetFillLevel()>0)
		{
			CFACReceive FACReceive;
			_BOOLEAN bCRCOk = FACReceive.FACParam(MDIBuf[0].QueryWriteBuffer(), TransmParam);
			if(bCRCOk)
			{
				return;
			}
			else
				cerr << "bad FAC CRC" << endl;
		}
	}
}

void
CDRMTransmitter::LoadSettings(CSettings& s)
{
	string mode = s.Get("0", "mode", string("DRMTX"));
	if(mode == "DRMTX")
		eOpMode = T_TX;
	if(mode == "DRMENC")
		eOpMode = T_ENC;
	if(mode == "DRMMOD")
		eOpMode = T_MOD;
	Encoder.LoadSettings(s, TransmParam);
	Modulator.LoadSettings(s, TransmParam);
}

void
CDRMTransmitter::SaveSettings(CSettings& s)
{
	switch(eOpMode)
	{
	case T_TX:
		s.Put("0", "mode", string("DRMTX"));
		break;
	case T_ENC:
		s.Put("0", "mode", string("DRMENC"));
		break;
	case T_MOD:
		s.Put("0", "mode", string("DRMMOD"));
		break;
	}
	Encoder.SaveSettings(s, TransmParam);
	Modulator.SaveSettings(s, TransmParam);
}
