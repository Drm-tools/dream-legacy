/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Implements:
 *	- Hamlib interface
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

#ifndef _HAMLIB_H
#define _HAMLIB_H

#include "../GlobalDefinitions.h"
#include <map>

#ifdef HAVE_LIBHAMLIB
# include <hamlib/rig.h>
#endif

class CParameter;
class CSettings;

enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};
enum EMight { C_CAN, C_MUST, C_CANT };
enum ERigMode { DRM, DRM_MODIFIED, AM, USB, LSB, CW, NBFM, WBFM };

struct CRigModeSpecificSettings
{
	CRigModeSpecificSettings():modes(),levels(),functions(),parameters(),
	eOnboardDemod(C_CAN), eInChanSel(CS_MIX_CHAN), audioInput(-1)
	{}
	CRigModeSpecificSettings(const CRigModeSpecificSettings& c):
	modes(c.modes),levels(c.levels),functions(c.functions),parameters(c.parameters),
	eOnboardDemod(c.eOnboardDemod), eInChanSel(c.eInChanSel), audioInput(c.audioInput)
	{}
	CRigModeSpecificSettings& operator=(const CRigModeSpecificSettings& c)
	{
		modes = c.modes;
		levels = c.levels;
		functions = c.functions;
		parameters = c.parameters;
		eOnboardDemod = c.eOnboardDemod;
		eInChanSel = c.eInChanSel;
		audioInput = c.audioInput;
		return *this;
	}
	map<string,int> modes;
	map<string,value_t> levels;
	map<string,string> functions;
	map<string,value_t> parameters;
	EMight			eOnboardDemod;
	EInChanSel		eInChanSel;
	int				audioInput;
};

class CRigCaps
{
public:
	CRigCaps() : strManufacturer(""), strModelName(""),
	eRigStatus(RIG_STATUS_ALPHA),
	bHamlibDoesAudio(FALSE),
	iFreqOffs(0),settings()
	{}
	CRigCaps(const CRigCaps& nSDRC) : 
	strManufacturer(nSDRC.strManufacturer),
	strModelName(nSDRC.strModelName),
	eRigStatus(nSDRC.eRigStatus), 
	bHamlibDoesAudio(nSDRC.bHamlibDoesAudio),
	iFreqOffs(nSDRC.iFreqOffs),settings(nSDRC.settings)
	{}

	CRigCaps& operator=(const CRigCaps& cNew)
	{
		strManufacturer = cNew.strManufacturer;
		strModelName = cNew.strModelName;
		eRigStatus = cNew.eRigStatus;
		bHamlibDoesAudio = cNew.bHamlibDoesAudio;
		iFreqOffs = cNew.iFreqOffs;
		settings = cNew.settings;
		return *this;
	}

	string			strManufacturer;
	string			strModelName;
	rig_status_e	eRigStatus;
	_BOOLEAN		bHamlibDoesAudio;
	int				iFreqOffs; /* Frequency offset */
	map<ERigMode,CRigModeSpecificSettings>
					settings;
	map<string,string> config;
};

#ifdef HAVE_LIBHAMLIB
/* Hamlib interface --------------------------------------------------------- */
class CHamlib
#ifdef USE_QT_GUI
	: public QThread
#endif
{
public:
	CHamlib(CParameter& p);
	virtual ~CHamlib();

	virtual void	run();

	_BOOLEAN		SetFrequency(const int iFreqkHz);
	void 			SetEnableSMeter(const _BOOLEAN bStatus);
	_BOOLEAN		GetEnableSMeter();

	/* backend selection */
	void			GetRigList(map<rig_model_t,CRigCaps>&);
	void			SetHamlibModelID(const rig_model_t model);
	rig_model_t		GetHamlibModelID() const {return iHamlibModelID;}
	void			SetRigMode(ERigMode eNMod);
	ERigMode		GetRigMode() const {return eRigMode;}
	void			SetEnableModRigSettings(const _BOOLEAN bNSM);
	_BOOLEAN		GetEnableModRigSettings() { return bModRigSettings; }

	/* com port selection */
	void			GetPortList(map<string,string>&);
	void			SetComPort(const string&);
	string			GetComPort() const;

	string			GetInfo() const;
	string			GetConfig(const string& key);
	void			GetRigCaps(rig_model_t id, CRigCaps& caps) { caps = CapsHamlibModels[id]; }

	void			RigSpecialParameters(rig_model_t id, const string& sSet, int iFrOff, const string& sModSet);
	void			LoadSettings(CSettings& s);
	void			SaveSettings(CSettings& s);

	int				iFreqOffset;

protected:
	static int			PrintHamlibModelList(const struct rig_caps* caps, void* data);
	static int			token(const struct confparams *, rig_ptr_t);
	static int			level(RIG*, const struct confparams *, rig_ptr_t);
	static int			parm(RIG*, const struct confparams *, rig_ptr_t);
	void				SetRigModes();
	void				SetRigLevels();
	void				SetRigFuncs();
	void				SetRigParams();
	void				SetRigConfig();

	CParameter&			Parameters;
	RIG*				pRig;
	_BOOLEAN			bSMeterIsSupported;
	_BOOLEAN			bEnableSMeter;
	_BOOLEAN			bModRigSettings;
	rig_model_t			iHamlibModelID;
	ERigMode			eRigMode;
	map<rig_model_t,CRigCaps>
						CapsHamlibModels;
};
#else
struct CHamlib
{
	enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};
};
#endif

#endif 
