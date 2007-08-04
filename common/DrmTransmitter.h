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
#include "DataIO.h"
#include "DRMSignalIO.h"
#include "sourcedecoders/AudioSourceDecoder.h"

/* Classes ********************************************************************/
class CSettings;

class CDRMTransmitter
#ifdef USE_QT_GUI
	: public QThread
#endif
{
public:
							CDRMTransmitter();
	virtual 				~CDRMTransmitter() {}
	void					LoadSettings(CSettings&); // can write to settings to set defaults
	void					SaveSettings(CSettings&);

#ifdef USE_QT_GUI
	void					run() { Start(); }
#else
	void					start() {}
	int						wait(int) {return 1;}
	bool					finished(){return true;}
#endif
	void					Start();
	void					Stop();

	_REAL 					GetLevelMeter();

	/* Source Encoder Interface */
	void					AddTextMessage(const string& strText);
	void					ClearTextMessages();
	void					AddPic(const string& strFileName, const string& strFormat);
	void					ClearPics();
	_BOOLEAN				GetTransStat(string& strCPi, _REAL& rCPe);

	void					GetSoundInChoices(vector<string>&);
	void					SetSoundInInterface(int);
	void 					SetReadFromFile(const string& strNFN);

	void					DisableCOFDM() { bCOFDMout = FALSE; }
	void					GetSoundOutChoices(vector<string>&);
	void					SetSoundOutInterface(int);
	void					SetWriteToFile(const string& strNFN, const string& strType);

	/* Parameters */
	CParameter				TransmParam;
	string					strMDIinAddr;
	string					strMDIoutAddr;

protected:

	CReadData*				pReadData;
	CAudioSourceEncoder		AudioSourceEncoder;
	string					strInputFileName;
	string					strOutputFileName;
	string					strOutputFileType;
	vector<string>			vecstrTexts;
	vector<string>			vecstrPics;
	vector<string>			vecstrPicTypes;
	int						iSoundInDev;
	int						iSoundOutDev;
	_BOOLEAN				bCOFDMout;
	_BOOLEAN				bUseUEP;
};


#endif // !defined(DRMTRANSM_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
