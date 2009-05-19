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
#include "../Parameter.h"
#ifdef HAVE_QT
# include <qthread.h>
# include <qmutex.h>
#endif

#ifdef HAVE_LIBHAMLIB

#include <string>
#include <map>

#include <hamlib/rig.h>

class CParameter;
class CSettings;

enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};

struct CRigModeSpecificSettings
{
	CRigModeSpecificSettings():modes(),levels(),functions(),parameters(),
	attributes()
	{}
	CRigModeSpecificSettings(const CRigModeSpecificSettings& c):
	modes(c.modes),levels(c.levels),functions(c.functions),parameters(c.parameters),
	attributes(c.attributes)
	{}
	CRigModeSpecificSettings& operator=(const CRigModeSpecificSettings& c)
	{
		modes = c.modes;
		levels = c.levels;
		functions = c.functions;
		parameters = c.parameters;
		attributes = c.attributes;
		return *this;
	}
	map<string,int> modes;
	map<string,value_t> levels;
	map<string,string> functions;
	map<string,value_t> parameters;
	map<string,string> attributes;
};

class CRigCaps
{
public:

	CRigCaps() : settings(), config(), hamlib_caps()
	{
		hamlib_caps.mfg_name = NULL;
	}
	CRigCaps(const CRigCaps& c) : settings(c.settings), config(c.config)
	{
	    memcpy(&hamlib_caps, &c.hamlib_caps, sizeof(rig_caps));
	}
	const CRigCaps& operator=(const CRigCaps& c)
	{
	    settings = c.settings;
	    config = c.config;
	    memcpy(&hamlib_caps, &c.hamlib_caps, sizeof(rig_caps));
	    return *this;
	}

	string get_config(const string& key) const
	{
		map<string,string>::const_iterator s = config.find(key);
		if(s==config.end())
			return string("");
		return s->second;
	}
	void	set_config(const string& key, const string& val) { config[key] = val; }
	int	mode(EModulationType m, const string& key) const;
	void	get_level(EModulationType m, const string& key, int& val) const;
	void	get_level(EModulationType m, const string& key, float& val) const;
	string	function(EModulationType m, const string& key) const;
	void	get_parameter(EModulationType m, const string& key, int& val) const;
	void	get_parameter(EModulationType m, const string& key, float& val) const;
	string	attribute(EModulationType m, const string& key) const;

	void	set_mode(EModulationType m, const string& key, const string& val);
	void	set_level(EModulationType m, const string& key, const string&al);
	void	set_function(EModulationType m, const string& key, const string& val);
	void	set_parameter(EModulationType m, const string& key, const string& val);
	void	set_attribute(EModulationType m, const string& key, const string& val);

	void	SetRigModes(RIG*, EModulationType);
	void	SetRigLevels(RIG*, EModulationType);
	void	SetRigFuncs(RIG*, EModulationType);
	void	SetRigParams(RIG*, EModulationType);
	int	SetRigConfig(RIG*);

	void	LoadSettings(const CSettings&, const string& sec);
	void	SaveSettings(CSettings&, const string& sec) const;

protected:

	map<EModulationType,CRigModeSpecificSettings> settings;
	map<string,string> config;
public:
	rig_caps hamlib_caps;
};

struct CRigMap
{
    map<string,map<string,rig_model_t> > rigs;
};

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

	bool			SetFrequency(const int iFreqkHz);
	void 			SetEnableSMeter(const bool bStatus); // sets/clears wanted flag and starts/stops
	bool			GetEnableSMeter(); // returns wanted flag
	void 			StopSMeter(); // stops (clears run flag) but leaves wanted flag alone

	/* backend selection */
	void			GetRigList(CRigMap&);
	void			SetRigModel(rig_model_t model);
	void			SetModulation(EModulationType);

	/* com port selection */
	void			GetPortList(map<string,string>&) const;
	void			SetComPort(const string&);
	string			GetComPort() const;
	string			GetInfo() const { if(pRig) return rig_get_info(pRig); return ""; }
	void			GetRigCaps(rig_model_t model, CRigCaps& caps) const;
	void			set_attribute(rig_model_t r, EModulationType e, const string& key, const string& val)
					{
						CapsHamlibModels[r].set_attribute(e, key, val);
					}
	void			LoadSettings(CSettings& s);
	void			SaveSettings(CSettings& s) const;

protected:

	static int		rig_enumerator(const rig_caps* caps, void* data);
	static int		token(const struct confparams *, rig_ptr_t);
	static int		level(RIG*, const struct confparams *, rig_ptr_t);
	static int		parm(RIG*, const struct confparams *, rig_ptr_t);
	void			OpenRig(rig_model_t);
	void			CloseRig();

#ifdef HAVE_QT
	QMutex			mutex;
#endif
	CParameter&		Parameters;
	RIG*			pRig;
	bool			bSMeterWanted, bEnableSMeter;
	map<rig_model_t,CRigCaps>
				CapsHamlibModels;
	int			iFrequencykHz;
	int			iFrequencyOffsetkHz;
};
#  else
struct CHamlib
{
	enum ESMeterState {SS_VALID, SS_NOTVALID, SS_TIMEOUT};
};

# endif

#endif
