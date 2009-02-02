/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DrmTransmitter.cpp
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

#if !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#include "Parameter.h"
#include "DrmEncoder.h"
#include "DrmModulator.h"
#include "DrmTransmitterInterface.h"

/* Classes ********************************************************************/
class CSettings;

class CDRMTransmitter: public CDRMTransmitterInterface
{
public:
							CDRMTransmitter();
	virtual 				~CDRMTransmitter() {}
	void					LoadSettings(CSettings&); // can write to settings to set defaults
	void					SaveSettings(CSettings&);

	void					Start();
	void					Stop();

	void					SetOperatingMode(const ETxOpMode);
	ETxOpMode				GetOperatingMode();

	void					CalculateChannelCapacities();

	_REAL 					GetLevelMeter();

	/* Source Encoder Interface */
	void					AddTextMessage(const string& strText);
	void					ClearTextMessages();
	void					GetTextMessages(vector<string>&);

	void					AddPic(const string& strFileName, const string& strFormat);
	void					ClearPics();
	void					GetPics(map<string,string>&);

	bool				GetTransStat(string& strCPi, _REAL& rCPe);

	void					GetSoundInChoices(vector<string>&);
	void					SetSoundInInterface(int);
	int						GetSoundInInterface() { return Encoder.GetSoundInInterface(); }
	void 					SetReadFromFile(const string& strNFN);
	string					GetReadFromFile() { return Encoder.GetReadFromFile(); }

	void					GetSoundOutChoices(vector<string>&);
	void					SetCOFDMOutputs(const vector<string>& o) { Modulator.SetOutputs(o); }
	void					GetCOFDMOutputs(vector<string>& o) { Modulator.GetOutputs(o); }

	void					SetMDIIn(const string& s) { strMDIinAddr = s; }
	string					GetMDIIn() { return strMDIinAddr; }

	void					SetMDIOut(const vector<string>& v) { MDIoutAddr = v; }
	void					GetMDIOut(vector<string>& v) { v = MDIoutAddr; }

	virtual CParameter*		GetParameters() { return &TransmParam; }

protected:

	CParameter				TransmParam;
	ETxOpMode				eOpMode;
	string					strMDIinAddr;
	vector<string>			MDIoutAddr;

	CDRMEncoder				Encoder;
	CDRMModulator			Modulator;
	CMDIIn					MDIIn;
	CDecodeMDI				DecodeMDI;
	CMDIOut*				pMDIOut;
};


#endif // !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
