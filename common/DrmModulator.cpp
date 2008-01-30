/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DRM-Modulator
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

#include "DrmModulator.h"
#include <iostream>
#include "util/Settings.h"

#include "sound.h"

/* Implementation *************************************************************/
CDRMModulator::CDRMModulator():
	MLCEncBuf(), MapBuf(),
	CarMapBuf(), OFDMModBuf(), TransmitData(),
	MSCMLCEncoder(), SymbInterleaver(), FACMLCEncoder(), SDCMLCEncoder(),
	OFDMCellMapping(), OFDMModulation()
{
}

void
CDRMModulator::GetSoundOutChoices(vector<string>& v)
{
	CSoundOut s;
	s.Enumerate(v);
}

void
CDRMModulator::Init(CParameter& Parameter, const vector<string>& COFDMOutputs)
{
	/* Defines number of cells, important! */
	OFDMCellMapping.Init(Parameter, &CarMapBuf);

	/* Defines number of SDC bits per super-frame */
	SDCMLCEncoder.Init(Parameter, &MapBuf[2]);

	MSCMLCEncoder.Init(Parameter, &MLCEncBuf);
	SymbInterleaver.Init(Parameter, &MapBuf[0]);
	FACMLCEncoder.Init(Parameter, &MapBuf[1]);
	OFDMModulation.Init(Parameter, &OFDMModBuf);

	TransmitData.Init(Parameter);
	if(COFDMOutputs.size()>0)
	{
		SetOutputs(COFDMOutputs);
	}
	else
		throw CGenErr("Modulator with no outputs");
}

void
CDRMModulator::WriteData(CParameter& Parameters, CBuffer<_BINARY>* Buf)
{
	cerr << "MSC MLC Int SDC FAC Car FDM" << endl;
	cerr <<  " "  << Buf[2].GetRequestFlag();
	cerr << "   " << MLCEncBuf.GetRequestFlag() ;
	cerr << "   " << MapBuf[0].GetRequestFlag() ;
	cerr << "   " << MapBuf[1].GetRequestFlag() ;
	cerr << "   " << MapBuf[2].GetRequestFlag() ;
	cerr << "   " << CarMapBuf.GetRequestFlag();
	cerr << "   " << OFDMModBuf.GetRequestFlag();
	cerr << endl;
	/* MLC-encoder */
	MSCMLCEncoder.ProcessData(Parameters, &Buf[2], &MLCEncBuf);

	/* Convolutional interleaver */
	SymbInterleaver.ProcessData(Parameters, &MLCEncBuf, &MapBuf[0]);

	/* FAC *************************************************************** */
	FACMLCEncoder.ProcessData(Parameters, &Buf[0], &MapBuf[1]);

	/* SDC *************************************************************** */
	SDCMLCEncoder.ProcessData(Parameters, &Buf[1], &MapBuf[2]);

	/* Mapping of the MSC, FAC, SDC and pilots on the carriers *********** */
	OFDMCellMapping.ProcessData(Parameters, MapBuf, &CarMapBuf);
	cerr << "CarMapBuf " << CarMapBuf.GetFillLevel() << endl;

	/* OFDM-modulation *************************************************** */
	OFDMModulation.ProcessData(Parameters, &CarMapBuf, &OFDMModBuf);

	/* Transmit the signal *********************************************** */
	TransmitData.WriteData(Parameters, &OFDMModBuf);
}

void
CDRMModulator::Cleanup(CParameter&)
{
}

void
CDRMModulator::LoadSettings(CSettings& s, CParameter& Parameters)
{

	/* Set desired intermediate frequency (IF) in Hertz */
	Parameters.rCarOffset = s.Get("Modulator", "if", 12000.0);

	/* default output format - REAL */
	Parameters.eOutputFormat = EOutFormat(s.Get("Modulator", "output_format", OF_REAL_VAL));

	//iSoundOutDev = s.Get("Modulator", "snddevout", -1);
	//strOutputFileName = s.Get("Modulator", "outputfile", string(""));
	//strOutputFileType = s.Get("Modulator", "outputfiletype", string(""));
}

void
CDRMModulator::SaveSettings(CSettings& s, CParameter& Parameters)
{
	s.Put("Modulator", "if", Parameters.rCarOffset);
	s.Put("Modulator", "output_format", Parameters.eOutputFormat);
	//s.Put("Modulator", "snddevout", iSoundOutDev);
	//s.Put("Modulator", "outputfile", strOutputFileName);
	//s.Put("Modulator", "outputfiletype", strOutputFileType);
}

void
CDRMModulator::SetOutputs(const vector<string>& o)
{
	TransmitData.SetOutputs(o);
}

void
CDRMModulator::GetOutputs(vector<string>& o)
{
	TransmitData.GetOutputs(o);
}
