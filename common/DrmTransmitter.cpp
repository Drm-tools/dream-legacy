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

/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter():
	TransmParam(),
	strMDIinAddr(), MDIoutAddr(), COFDMOutputs(),
	eOpMode(T_TX),
	Encoder(), Modulator()
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
	CSingleBuffer<_COMPLEX>	DummyBuf;
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
	CUpstreamDI				MDIIn;
	CDecodeRSIMDI			DecodeMDI;
	CDownstreamDI			MDIOut;

	vector<CSingleBuffer<_BINARY> >	MSCBuf(MAX_NUM_STREAMS);
	vector<CSingleBuffer<_BINARY> >	MSCTxBuf(MAX_NUM_STREAMS);
	vector<CSingleBuffer<_BINARY> >	MSCSendBuf(MAX_NUM_STREAMS);
	CSingleBuffer<_BINARY>			MDIPacketBuf;
	CSingleBuffer<_BINARY>	FACBuf;
	CSingleBuffer<_BINARY>	FACTxBuf;
	CSingleBuffer<_BINARY>	FACSendBuf;

	CSingleBuffer<_BINARY>	SDCBuf;
	CSingleBuffer<_BINARY>	SDCTxBuf;
	CSingleBuffer<_BINARY>	SDCSendBuf;

	int sdc_counter = 0;

	FACBuf.Clear();
	FACTxBuf.Clear();
	FACSendBuf.Clear();
	SDCBuf.Clear();
	SDCTxBuf.Clear();
	SDCSendBuf.Clear();
	FACBuf.Init(72);
	SDCBuf.Init(10000);
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		MSCBuf[i].Clear();
		MSCTxBuf[i].Clear();
		MSCSendBuf[i].Clear();
		MSCBuf[i].Init(10000);
	}

	if(COFDMOutputs.size()>0)
	{
		Modulator.SetOutputs(COFDMOutputs);
		Modulator.Init(TransmParam, FACTxBuf, SDCTxBuf, MSCTxBuf);
	}

	if(strMDIinAddr != "")
	{
		MDIIn.SetOrigin(strMDIinAddr);
		MDIIn.SetInitFlag();
		DecodeMDI.SetInitFlag();
	}
	else
	{
		Encoder.Init(TransmParam, FACBuf, SDCBuf, MSCBuf);
	}

	for(vector<string>::const_iterator s = MDIoutAddr.begin(); s!=MDIoutAddr.end(); s++)
	{
		/* set the output address */
		MDIOut.AddSubscriber(*s, "", 'M');
		//MDIOut.SetInitFlag();
	}

	//if(MDIOut.GetOutEnabled())
	{
		TransmParam.ReceiveStatus.FAC.SetStatus(RX_OK);
		TransmParam.ReceiveStatus.SDC.SetStatus(RX_OK);
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
			if(MDIIn.GetInEnabled())
			{
				MDIPacketBuf.Clear();
				MDIIn.ReadData(TransmParam, MDIPacketBuf);
				if(MDIPacketBuf.GetFillLevel()>0)
				{
					DecodeMDI.ProcessData(TransmParam, MDIPacketBuf, FACBuf, SDCBuf, MSCBuf);
				}
			}
			else
			{
				Encoder.ProcessData(TransmParam, FACBuf, SDCBuf, MSCBuf);
			}


			if(COFDMOutputs.size()>0)
				Modulator.ProcessData(TransmParam, FACBuf, SDCBuf, MSCBuf);
			else
			{
				MDIOut.SendLockedFrame(TransmParam, FACBuf, SDCBuf, MSCBuf);
				FACBuf.SetRequestFlag(TRUE);
				switch(sdc_counter)
				{
				case 0:
					SDCBuf.SetRequestFlag(TRUE);
					sdc_counter = 1;
					break;
				case 1:
					sdc_counter = 2;
					break;
				case 2:
					sdc_counter = 0;
				}
				for(size_t i=0; i<MAX_NUM_STREAMS; i++)
					MSCBuf[i].SetRequestFlag(TRUE);
			}
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	Encoder.Cleanup(TransmParam);
	Modulator.Cleanup(TransmParam);
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
