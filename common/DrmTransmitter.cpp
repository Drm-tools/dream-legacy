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
#include <sstream>

/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter():
    CDRMTransmitterInterface(),
	TransmParam(), eOpMode(T_TX),
	strMDIinAddr(), MDIoutAddr(),
	Encoder(), Modulator(),
	MDIIn(), DecodeMDI(), MDIOut(*new CMDIOut())
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
CDRMTransmitter::CalculateChannelCapacities()
{
	CSingleBuffer<_COMPLEX>	DummyBuf;
	CMSCMLCEncoder			MSCMLCEncoder;
	CSDCMLCEncoder			SDCMLCEncoder;
	SDCMLCEncoder.Init(TransmParam, DummyBuf);
	MSCMLCEncoder.Init(TransmParam, DummyBuf);
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
CDRMTransmitter::GetTextMessages(vector<string>& v)
{
	Encoder.GetTextMessages(v);
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

void
CDRMTransmitter::GetPics(map<string,string>& m)
{
    Encoder.GetPics(m);
}

bool
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
	TransmParam.bRunThread = false;
}

void CDRMTransmitter::Start()
{

	bool bInSync = true;
	CSingleBuffer<_BINARY> MDIPacketBuf;
	CSingleBuffer<_BINARY> FACBuf, SDCBuf;
	vector<CSingleBuffer<_BINARY> > MSCBuf(MAX_NUM_STREAMS);

	try
	{
        switch(eOpMode)
        {
        case T_TX:
            TransmParam.ReceiveStatus.FAC.SetStatus(RX_OK);
            TransmParam.ReceiveStatus.SDC.SetStatus(RX_OK);
            Encoder.Init(TransmParam, FACBuf, SDCBuf, MSCBuf);
            Modulator.Init(TransmParam);
            break;
        case T_ENC:
            if(MDIoutAddr.size()==0)
                throw CGenErr("Encoder with no outputs");
            TransmParam.ReceiveStatus.FAC.SetStatus(RX_OK);
            TransmParam.ReceiveStatus.SDC.SetStatus(RX_OK);
            Encoder.Init(TransmParam, FACBuf, SDCBuf, MSCBuf);
            {
                // delete the old MDIOut and get a new one so that we discard the
                // old destinations
                CMDIOut* old = &MDIOut;
                delete old;
                MDIOut = *new CMDIOut();
                /* set the output address */
                for(vector<string>::const_iterator s = MDIoutAddr.begin(); s!=MDIoutAddr.end(); s++)
                    MDIOut.AddSubscriber(*s, "", 'M');
            }
            MDIOut.Init(TransmParam);
            break;
        case T_MOD:
            if(strMDIinAddr != "")
            {
                MDIIn.SetOrigin(strMDIinAddr);
            }
            else
                throw CGenErr("Modulator with no input");
            bInSync = false;
            MDIIn.Init(TransmParam, MDIPacketBuf);
            DecodeMDI.Init(TransmParam, FACBuf, SDCBuf, MSCBuf);

        }

        /* Set run flag */
        TransmParam.bRunThread = true;
        cout << "Tx: starting, in:" << (MDIIn.GetInEnabled()?"MDI":"Encoder")
             << ", out: " << (MDIOut.GetOutEnabled()?"MDI":"COFDM")
             << endl; cout.flush();

		while (TransmParam.bRunThread)
		{
			if(eOpMode == T_MOD)
			{
				if(bInSync==false)
				{
					FACBuf.SetRequestFlag(true);
					SDCBuf.SetRequestFlag(true);
					for(size_t i=0; i<MAX_NUM_STREAMS; i++)
						MSCBuf[i].SetRequestFlag(true);
					MDIPacketBuf.SetRequestFlag(true);
				}
				MDIPacketBuf.Clear();
				MDIIn.ReadData(TransmParam, MDIPacketBuf);
				if(MDIPacketBuf.GetFillLevel()>0)
				{
					//DecodeMDI.Expect(MDIPacketBuf.GetFillLevel());
					DecodeMDI.ProcessData(TransmParam, MDIPacketBuf, FACBuf, SDCBuf, MSCBuf);
				}
				if(bInSync==false && FACBuf.GetFillLevel()>0)
				{
					CFACReceive FACReceive;
					bool bCRCOk = FACReceive.FACParam(FACBuf.QueryWriteBuffer(), TransmParam);
					if(bCRCOk)
					{
						cerr << "Got SDCI & FAC" << endl;
						Modulator.Init(TransmParam);
						if(SDCBuf.GetFillLevel()>0)
						{
							cerr << "got SDC" << endl;
							bInSync = true;
						}
						else
						{
							/* consume the FAC & MSC and wait for an SDC frame */
							(void)FACBuf.Get(NUM_FAC_BITS_PER_BLOCK);
							for(size_t i=0; i<MAX_NUM_STREAMS; i++)
							{
								(void)MSCBuf[i].Get(MSCBuf[i].GetFillLevel());
                            }
						}
					}
					else
						cerr << "bad FAC CRC" << endl;
				}
			}
			else
			{
				Encoder.ReadData(TransmParam, FACBuf, SDCBuf, MSCBuf);
			}

			if(eOpMode == T_ENC)
			{
				MDIOut.WriteData(TransmParam, FACBuf, SDCBuf, MSCBuf);
			}
			else
			{
				if(bInSync)
					Modulator.WriteData(TransmParam, FACBuf, SDCBuf, MSCBuf);
				/* TODO - set bInSync false on error */
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
CDRMTransmitter::LoadSettings(CSettings& s)
{
	string mode = s.Get("0", "mode", string("DRMTX"));
	strMDIinAddr = s.Get("Transmitter", "MDIin", string(""));
	for(size_t i=0; i<100; i++)
	{
	    stringstream ss;
        ss << "MDIout" << i;
        string addr = s.Get("Transmitter", ss.str(), string("[none]"));
        if(addr == "[none]")
            break;
        MDIoutAddr.push_back(addr);
	}
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
	s.Put("Transmitter", "MDIin", strMDIinAddr);
	for(size_t i=0; i<MDIoutAddr.size(); i++)
	{
	    stringstream ss;
        ss << "MDIout" << i;
        s.Put("Transmitter", ss.str(), MDIoutAddr[i]);
	}
	Encoder.SaveSettings(s, TransmParam);
	Modulator.SaveSettings(s, TransmParam);
}
