/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2008
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 *
 * Description:
 *	DrmTransmitter (almost) abstract interface
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

#ifndef _DRM_TRANSMITTERINTERFACE_H
#define _DRM_TRANSMITTERINTERFACE_H

class CParameter;

class CDRMTransmitterInterface
#ifdef USE_QT_GUI
	: public QThread
#endif
{
public:
	enum ETxOpMode { T_ENC, T_MOD, T_TX };

	CDRMTransmitterInterface() {}
	virtual ~CDRMTransmitterInterface() {}

#ifdef USE_QT_GUI
	void					run() { Start(); }
#else
	virtual void			start() {};
	virtual int				wait(int) {};
	bool					finished(){return true;}
#endif
	virtual void			Start()=0;
	virtual void			Stop()=0;

	virtual void			SetOperatingMode(const ETxOpMode)=0;
	virtual ETxOpMode		GetOperatingMode()=0;

	virtual void			CalculateChannelCapacities()=0;

	virtual _REAL 			GetLevelMeter()=0;

	/* Source Encoder Interface */
	virtual void			AddTextMessage(const string& strText)=0;
	virtual void			ClearTextMessages()=0;
	virtual void			GetTextMessages(vector<string>&)=0;
	virtual void			AddPic(const string& strFileName, const string& strFormat)=0;
	virtual void			ClearPics()=0;
	virtual _BOOLEAN		GetTransStat(string& strCPi, _REAL& rCPe)=0;

	virtual void			GetSoundInChoices(vector<string>&)=0;
	virtual void			SetSoundInInterface(int)=0;
	virtual int				GetSoundInInterface()=0;
	virtual void 			SetReadFromFile(const string& strNFN)=0;
	virtual string			GetReadFromFile()=0;

	virtual void			GetSoundOutChoices(vector<string>&)=0;
	virtual void			GetCOFDMOutputs(vector<string>&)=0;
	virtual void			SetCOFDMOutputs(const vector<string>&)=0;

	virtual void			SetMDIIn(const string& s)=0;
	virtual string			GetMDIIn()=0;

	virtual void			SetMDIOut(const vector<string>& v)=0;
	virtual void			GetMDIOut(vector<string>&)=0;

	virtual CParameter*		GetParameters()=0;

};

#endif
