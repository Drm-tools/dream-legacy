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

/* cloned from DrmReceiver - TODO better solution */

class CSplitFAC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter&)
		{this->iInputBlockSize = NUM_FAC_BITS_PER_BLOCK;}
};

class CSplitSDC : public CSplitModul<_BINARY>
{
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.iNumSDCBitsPerSFrame;}
};

class CSplitMSC : public CSplitModul<_BINARY>
{
public:
	void SetStream(int iID) {iStreamID = iID;}

protected:
	void SetInputBlockSize(CParameter& p)
		{this->iInputBlockSize = p.GetStreamLen(iStreamID) * SIZEOF__BYTE;}

	int iStreamID;
};

/* Implementation *************************************************************/
CDRMTransmitter::CDRMTransmitter():
	TransmParam(NULL),
	strMDIinAddr(), strMDIoutAddr(),
	eOpMode(T_TX),
	bCOFDMout(FALSE), Encoder(), Modulator()
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

void
CDRMTransmitter::
SetSoundOutInterface(int i)
{
	Modulator.SetSoundOutInterface(i);
	bCOFDMout = TRUE;
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
CDRMTransmitter::SetWriteToFile(const string & strNFN, const string & strType)
{
	Modulator.SetWriteToFile(strNFN, strType);
	bCOFDMout = TRUE;
}

void
CDRMTransmitter::Stop()
{
	TransmParam.bRunThread = FALSE;
}

void CDRMTransmitter::Start()
{
	CSplitFAC				SplitFAC;
	CSplitSDC				SplitSDC;
	CSplitMSC				SplitMSC[MAX_NUM_STREAMS];
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

	SplitFAC.SetInitFlag();
	FACBuf.Clear();
	FACTxBuf.Clear();
	FACSendBuf.Clear();
	SplitSDC.SetInitFlag();
	SDCBuf.Clear();
	SDCTxBuf.Clear();
	SDCSendBuf.Clear();
	FACBuf.Init(72);
	SDCBuf.Init(10000);
	for(size_t i=0; i<MAX_NUM_STREAMS; i++)
	{
		SplitMSC[i].SetStream(i);
		SplitMSC[i].SetInitFlag();
		MSCBuf[i].Clear();
		MSCTxBuf[i].Clear();
		MSCSendBuf[i].Clear();
		MSCBuf[i].Init(10000);
	}

	if(bCOFDMout)
	{
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

	if(strMDIoutAddr!="")
	{
		/* set the output address */
		MDIOut.AddSubscriber(strMDIoutAddr, "", 'M');
		//MDIOut.SetInitFlag();
	}

	/* Set run flag */
	TransmParam.bRunThread = TRUE;

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


			if(bCOFDMout && MDIOut.GetOutEnabled())
			{
				SplitFAC.ProcessData(TransmParam, FACBuf, FACTxBuf, FACSendBuf);

				if(SDCBuf.GetFillLevel()==TransmParam.iNumSDCBitsPerSFrame)
					SplitSDC.ProcessData(TransmParam, SDCBuf, SDCTxBuf, SDCSendBuf);

				for(size_t i=0; i<MAX_NUM_STREAMS; i++)
					SplitMSC[i].ProcessData(TransmParam, MSCBuf[i], MSCTxBuf[i], MSCSendBuf[i]);

				Modulator.ProcessData(TransmParam, FACTxBuf, SDCTxBuf, MSCTxBuf);

				MDIOut.SendLockedFrame(TransmParam, FACSendBuf, SDCSendBuf, MSCSendBuf);
			}
			else
			{
				if(bCOFDMout)
					Modulator.ProcessData(TransmParam, FACBuf, SDCBuf, MSCBuf);
				else
					MDIOut.SendLockedFrame(TransmParam, FACBuf, SDCBuf, MSCBuf);
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
	eOpMode = ETxOpMode(s.Get("Transmitter", "mode", int(T_TX)));
	Encoder.LoadSettings(s, TransmParam);
	Modulator.LoadSettings(s, TransmParam);
}

void
CDRMTransmitter::SaveSettings(CSettings& s)
{
	s.Put("Transmitter", "mode", int(eOpMode));
	Encoder.SaveSettings(s, TransmParam);
	Modulator.SaveSettings(s, TransmParam);
}
