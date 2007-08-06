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
	MLCEncBuf(), IntlBuf(),
	FACMapBuf(), SDCMapBuf(), CarMapBuf(), OFDMModBuf(),
	pSoundOutInterface(NULL), pTransmitData(NULL),
	MSCMLCEncoder(), SymbInterleaver(), FACMLCEncoder(), SDCMLCEncoder(),
	OFDMCellMapping(), OFDMModulation(),
	strOutputFileName(), strOutputFileType(), iSoundOutDev(-1)
{
}

void
CDRMModulator::GetSoundOutChoices(vector<string>& v)
{
	CSoundOut s;
	s.Enumerate(v);
}

void
CDRMModulator::SetSoundOutInterface(int i)
{
	iSoundOutDev = i;
	strOutputFileName = "";
	strOutputFileType = "";
}

void
CDRMModulator::SetWriteToFile(const string & strNFN, const string & strType)
{
	strOutputFileName = strNFN;
	strOutputFileType = strType;
}

void
CDRMModulator::Init(CParameter& Parameters, CBuffer<_BINARY>& FACBuf,
	CBuffer<_BINARY>& SDCBuf, vector< CSingleBuffer<_BINARY> >& MSCBuf)
{

	/* Defines number of cells, important! */
	OFDMCellMapping.Init(Parameters, CarMapBuf);

	/* Defines number of SDC bits per super-frame */
	SDCMLCEncoder.Init(Parameters, SDCMapBuf);

	MSCMLCEncoder.Init(Parameters, MLCEncBuf);
	SymbInterleaver.Init(Parameters, IntlBuf);
	FACMLCEncoder.Init(Parameters, FACMapBuf);
	OFDMModulation.Init(Parameters, OFDMModBuf);

	if(strOutputFileName=="")
	{
		pSoundOutInterface = new CSoundOut;
		pTransmitData = new CTransmitData(pSoundOutInterface);
	}
	else
	{
		pTransmitData = new CTransmitData(NULL);
		pTransmitData->SetWriteToFile(strOutputFileName, strOutputFileType);
	}

	pTransmitData->Init(Parameters);
}

void
CDRMModulator::ProcessData(CParameter& Parameters,
	CBuffer<_BINARY>& FACBuf, CBuffer<_BINARY>& SDCBuf, vector< CSingleBuffer<_BINARY> >& MSCBuf)
{

	/* MLC-encoder */
	MSCMLCEncoder.ProcessData(Parameters, MSCBuf[0], MLCEncBuf);

	/* Convolutional interleaver */
	SymbInterleaver.ProcessData(Parameters, MLCEncBuf, IntlBuf);

	/* FAC *************************************************************** */
	FACMLCEncoder.ProcessData(Parameters, FACBuf, FACMapBuf);

	/* SDC *************************************************************** */
	SDCMLCEncoder.ProcessData(Parameters, SDCBuf, SDCMapBuf);

	/* Mapping of the MSC, FAC, SDC and pilots on the carriers *********** */
	OFDMCellMapping.ProcessData(Parameters, IntlBuf, FACMapBuf, SDCMapBuf, CarMapBuf);

	/* OFDM-modulation *************************************************** */
	OFDMModulation.ProcessData(Parameters, CarMapBuf, OFDMModBuf);

	/* Transmit the signal *********************************************** */
	pTransmitData->WriteData(Parameters, OFDMModBuf);

}

void
CDRMModulator::Cleanup(CParameter&)
{
	delete pTransmitData;
	if(pSoundOutInterface)
		delete pSoundOutInterface;
}

void
CDRMModulator::LoadSettings(CSettings& s, CParameter& Parameters)
{

	/* Set desired intermediate frequency (IF) in Hertz */
	Parameters.rCarOffset = s.Get("Modulator", "if", 12000.0);

	/* default output format - REAL */
	Parameters.eOutputFormat = EOutFormat(s.Get("Modulator", "output_format", OF_REAL_VAL));

	iSoundOutDev = s.Get("Modulator", "snddevout", -1);
	strOutputFileName = s.Get("Modulator", "outputfile", string(""));
	strOutputFileType = s.Get("Modulator", "outputfiletype", string(""));
}

void
CDRMModulator::SaveSettings(CSettings& s, CParameter& Parameters)
{
	s.Put("Modulator", "if", Parameters.rCarOffset);
	s.Put("Modulator", "output_format", Parameters.eOutputFormat);
	s.Put("Modulator", "snddevout", iSoundOutDev);
	s.Put("Modulator", "outputfile", strOutputFileName);
	s.Put("Modulator", "outputfiletype", strOutputFileType);
}
