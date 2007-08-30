/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable
 * 
 * Description:
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

#include "Hamlib.h"
#include "../Parameter.h"
#include <sstream>
#if defined(_WIN32)
# ifdef HAVE_SETUPAPI
#  ifndef INITGUID
#   define INITGUID 1
#  endif
#  include <windows.h>
#  include <setupapi.h>
#  if defined(_MSC_VER) && (_MSC_VER < 1400) || defined(__MINGW32__)
    DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86e0d1e0L, 0x8089, 
    0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);
#  endif
# endif
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#endif

/* Implementation *************************************************************/
#ifdef HAVE_LIBHAMLIB
/******************************************************************************\
* Hamlib interface                                                             *
\******************************************************************************/
/*
	This code is based on patches and example code from Tomi Manninen and
	Stephane Fillod (developer of hamlib)
*/
CHamlib::CHamlib(CParameter& p): iFreqOffset(0),
Parameters(p), pRig(NULL),
bSMeterIsSupported(FALSE), bEnableSMeter(FALSE), bModRigSettings(FALSE),
iHamlibModelID(0), CapsHamlibModels()
{
#ifdef RIG_MODEL_DUMMY
	CapsHamlibModels[RIG_MODEL_DUMMY].settings[DRM].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_DUMMY].settings[WBFM].levels["ATT"].i=5;
	CapsHamlibModels[RIG_MODEL_DUMMY].settings[DRM_MODIFIED].modes["AM"]=6;
	CapsHamlibModels[RIG_MODEL_DUMMY].settings[AM].parameters["BEEP"].i=6;
	CapsHamlibModels[RIG_MODEL_DUMMY].settings[USB].functions["TUNER"]="hello";
#endif

#ifdef RIG_MODEL_G303
	/* Winradio G3 */
	CapsHamlibModels[RIG_MODEL_G303].settings[DRM].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G303].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_G303].settings[DRM_MODIFIED].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G303].settings[DRM_MODIFIED].levels["AGC"].i=3;
#endif

#ifdef RIG_MODEL_G313
	/* Winradio G313 */
	CapsHamlibModels[RIG_MODEL_G313].settings[DRM].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G313].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_G313].settings[DRM_MODIFIED].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G313].settings[DRM_MODIFIED].levels["AGC"].i=3;
# ifdef __linux
	CapsHamlibModels[RIG_MODEL_G313].config["if_path"]="/dreamg3xxif";
	CapsHamlibModels[RIG_MODEL_G313].bHamlibDoesAudio = TRUE;
# endif
#endif

#ifdef RIG_MODEL_G315
	/* Winradio G315 */
	CapsHamlibModels[RIG_MODEL_G315].settings[DRM].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G315].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_G315].settings[DRM_MODIFIED].levels["ATT"].i=0;
	CapsHamlibModels[RIG_MODEL_G315].settings[DRM_MODIFIED].levels["AGC"].i=3;
# ifdef __linux
	CapsHamlibModels[RIG_MODEL_G315].config["if_path"]="/dreamg3xxif";
	CapsHamlibModels[RIG_MODEL_G315].bHamlibDoesAudio = TRUE;
# endif
	CapsHamlibModels[RIG_MODEL_G315].settings[WBFM].eOnboardDemod = C_MUST;
#endif

#ifdef RIG_MODEL_AR7030
	/* AOR 7030 */
	CapsHamlibModels[RIG_MODEL_AR7030].settings[DRM].modes["AM"]=3;
	CapsHamlibModels[RIG_MODEL_AR7030].settings[DRM].levels["AGC"].i=5;
	CapsHamlibModels[RIG_MODEL_AR7030].settings[DRM_MODIFIED].modes["AM"]=6;
	CapsHamlibModels[RIG_MODEL_AR7030].settings[DRM_MODIFIED].levels["AGC"].i=5;
#endif

#ifdef RIG_MODEL_NRD535
	/* JRC NRD 535 */
						 /* AGC=slow */ 
	CapsHamlibModels[RIG_MODEL_NRD535].settings[DRM].modes["CW"]=12000;
	CapsHamlibModels[RIG_MODEL_NRD535].settings[DRM].levels["CWPITCH"].i=-5000;
	CapsHamlibModels[RIG_MODEL_NRD535].settings[DRM].levels["IF"].i=-2000;
	CapsHamlibModels[RIG_MODEL_NRD535].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_NRD535].settings[DRM_MODIFIED].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_NRD535].iFreqOffs = 3; /* kHz frequency offset */
#endif

#ifdef RIG_MODEL_RX320
	/* TenTec RX320D */
	CapsHamlibModels[RIG_MODEL_RX320].settings[DRM].modes["AM"]=6000;
	CapsHamlibModels[RIG_MODEL_RX320].settings[DRM].levels["AF"].i=1;
	CapsHamlibModels[RIG_MODEL_RX320].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_RX320].settings[DRM_MODIFIED].levels["AGC"].i=3;
#endif

#ifdef RIG_MODEL_RX340
	/* TenTec RX340D */
	CapsHamlibModels[RIG_MODEL_RX340].settings[DRM].modes["USB"]=16000;
	CapsHamlibModels[RIG_MODEL_RX340].settings[DRM].levels["IF"].i=2000;
	CapsHamlibModels[RIG_MODEL_RX340].settings[DRM].levels["AF"].i=1;
	CapsHamlibModels[RIG_MODEL_RX340].settings[DRM].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_RX340].settings[DRM_MODIFIED].levels["AGC"].i=3;
	CapsHamlibModels[RIG_MODEL_RX340].iFreqOffs = -12;
#endif

	/* Load all available front-end remotes in hamlib library */
	rig_load_all_backends();

	/* Get all models which are available.
	 * A call-back function is called to return the different rigs */
	rig_list_foreach(PrintHamlibModelList, this);
}

CHamlib::~CHamlib()
{
	if (pRig != NULL)
	{
		/* close everything */
		rig_close(pRig);
		rig_cleanup(pRig);
	}
}

string CHamlib::GetConfig(const string& key)
{ 
	if(iHamlibModelID==0)
		return "";
	return CapsHamlibModels[iHamlibModelID].config[key];
}

void
CHamlib::GetRigList(map < rig_model_t, CRigCaps > &rigs)
{
	rigs = CapsHamlibModels;
}

void
CHamlib::GetPortList(map < string, string > &ports)
{
	ports.clear();
/* Config string for com-port selection is different in Windows and Linux */
#ifdef _WIN32
# ifdef HAVE_SETUPAPI
	GUID guid = GUID_DEVINTERFACE_COMPORT;
	HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&guid, NULL, NULL,
											   DIGCF_PRESENT |
											   DIGCF_DEVICEINTERFACE);
	if (hDevInfoSet != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA devInfo;
		devInfo.cbSize = sizeof(SP_DEVINFO_DATA);
		for (int i = 0; SetupDiEnumDeviceInfo(hDevInfoSet, i, &devInfo); i++)
		{
			HKEY hDeviceKey =
				SetupDiOpenDevRegKey(hDevInfoSet, &devInfo, DICS_FLAG_GLOBAL,
									 0, DIREG_DEV, KEY_QUERY_VALUE);
			if (hDeviceKey)
			{
				char szPortName[256];
				DWORD dwSize = sizeof(szPortName);
				DWORD dwType = 0;
				if ((RegQueryValueExA
					 (hDeviceKey, "PortName", NULL, &dwType,
					  reinterpret_cast < LPBYTE > (szPortName),
					  &dwSize) == ERROR_SUCCESS) && (dwType == REG_SZ))
				{
					char szFriendlyName[256];
					DWORD dwSize = sizeof(szFriendlyName);
					DWORD dwType = 0;
					if (SetupDiGetDeviceRegistryPropertyA
						(hDevInfoSet, &devInfo, SPDRP_DEVICEDESC, &dwType,
						 reinterpret_cast < PBYTE > (szFriendlyName), dwSize,
						 &dwSize) && (dwType == REG_SZ))
						ports[string(szFriendlyName) + " " + szPortName] = szPortName;
					else
						ports[szPortName] = szPortName;
				}

				RegCloseKey(hDeviceKey);
			}
		}

		SetupDiDestroyDeviceInfoList(hDevInfoSet);
	}
# endif
	if (ports.empty())
	{
		ports["COM1"] = "COM1";
		ports["COM2"] = "COM2";
		ports["COM3"] = "COM3 ";
		ports["COM4"] = "COM4 ";
		ports["COM5"] = "COM5 ";
	}
#elif defined(__linux)
	FILE *p =
		popen("hal-find-by-capability --capability serial", "r");
	_BOOLEAN bOK = FALSE;
	while (!feof(p))
	{
		char buf[1024];
		fgets(buf, sizeof(buf), p);
		if (strlen(buf) > 0)
		{
			string s =
				string("hal-get-property --key serial.device --udi ") +
				buf;
			FILE *p2 = popen(s.c_str(), "r");
			fgets(buf, sizeof(buf), p2);
			size_t n = strlen(buf);
			if (n > 0)
			{
				if (buf[n - 1] == '\n')
					buf[n - 1] = 0;
				ports[buf] = buf;
				bOK = TRUE;
				buf[0] = 0;
			}
			pclose(p2);
		}
	}
	pclose(p);
	if (!bOK)
	{
		ports["ttyS0"] = "/dev/ttyS0";
		ports["ttyS1"] = "/dev/ttyS1";
		ports["ttyUSB0"] = "/dev/ttyUSB0";
	}
#elif defined(__APPLE__)
	io_iterator_t serialPortIterator;
    kern_return_t			kernResult; 
    CFMutableDictionaryRef	classesToMatch;

    // Serial devices are instances of class IOSerialBSDClient
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else
	{
        CFDictionarySetValue(classesToMatch,
                             CFSTR(kIOSerialBSDTypeKey),
                             CFSTR(kIOSerialBSDRS232Type));
        
	}
    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &serialPortIterator);    
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }
        
    io_object_t		modemService;
    
    // Iterate across all modems found. In this example, we bail after finding the first modem.
    
    while ((modemService = IOIteratorNext(serialPortIterator)))
    {
        CFStringRef	bsdPathAsCFString;

		// Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
		// used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
		// incoming calls, e.g. a fax listener.
	
		bsdPathAsCFString = CFStringRef(IORegistryEntryCreateCFProperty(modemService,
                                                            CFSTR(kIOCalloutDeviceKey),
                                                            kCFAllocatorDefault,
                                                            0));
        if (bsdPathAsCFString)
        {
            Boolean result;
			char bsdPath[256];
            
            // Convert the path from a CFString to a C (NUL-terminated) string for use
			// with the POSIX open() call.
	    
			result = CFStringGetCString(bsdPathAsCFString,
                                        bsdPath,
                                        sizeof(bsdPath), 
                                        kCFStringEncodingUTF8);
            CFRelease(bsdPathAsCFString);
            
            if (result)
			{
				// make the name a bit more friendly for the menu
				string s,t=bsdPath;
				size_t p = t.find('.');
				if(p<string::npos)
					s = t.substr(p+1);
				else
					s = t;
				ports[s] = bsdPath;
            }
        }

        // Release the io_service_t now that we are done with it.
	
		(void) IOObjectRelease(modemService);
    }
#endif
}

void
CHamlib::SetComPort(const string & port)
{
	CapsHamlibModels[iHamlibModelID].config["rig_pathname"] = port;
	SetHamlibModelID(iHamlibModelID);
}

string CHamlib::GetComPort() const
{
	map < int, CRigCaps >::const_iterator m = CapsHamlibModels.find(iHamlibModelID);
	if(m==CapsHamlibModels.end())
		return "";

	map < string, string >::const_iterator c = m->second.config.find("rig_pathname");
	if (c == m->second.config.end())
		return "";

	return c->second;
}

int
CHamlib::PrintHamlibModelList(const struct rig_caps *caps, void *data)
{
	/* Access data members of class through pointer ((CHamlib*) data) */
	CHamlib & Hamlib = *((CHamlib *) data);

	/* Store new model in class. Use only relevant information */
	Hamlib.CapsHamlibModels[caps->rig_model].strManufacturer = caps->mfg_name;
	Hamlib.CapsHamlibModels[caps->rig_model].strModelName = caps->model_name;
	Hamlib.CapsHamlibModels[caps->rig_model].eRigStatus = caps->status;

	return 1;					/* !=0, we want them all! */
}

int CHamlib::level(RIG* rig, const struct confparams *parm, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	cout << parm->name << endl;
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

int CHamlib::parm(RIG* rig, const struct confparams *parm, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	cout << parm->name << endl;
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

int CHamlib::token(const struct confparams *parm, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	cout << parm->name << endl;
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

void
CHamlib::LoadSettings(CSettings & s)
{
	rig_model_t model = s.Get("Hamlib", "model", 0);
	bEnableSMeter = s.Get("Hamlib", "ensmeter", FALSE);
	iFreqOffset = s.Get("Hamlib", "freqoffset", 0);
	bModRigSettings = s.Get("Hamlib", "enmodrig", FALSE);

	/* extract config from -C command line arg */
	string command_line_config = s.Get("command", "hamlib-config", string(""));
	if(command_line_config!="")
	{
		istringstream params(command_line_config);
		while (!params.eof())
		{
			string name, value;
			getline(params, name, '=');
			getline(params, value, ',');
			s.Put("Hamlib-config", name, value);
		}
	}

	if (model != 0)
	{
		CRigCaps& caps = CapsHamlibModels[iHamlibModelID];
		INISection sec;
		s.Get("Hamlib-config", sec);
		INISection::iterator i;
		for(i=sec.begin(); i!=sec.end(); i++)
			caps.config[i->first] = i->second;
		for(size_t j=DRM; j<=WBFM; j++)
		{
			stringstream section;
			section << "Hamlib-" << j;
			sec.clear();
			s.Get(section.str()+"-modes", sec);
			for(i=sec.begin(); i!=sec.end(); i++)
				caps.settings[ERigMode(j)].modes[i->first] = atoi(i->second.c_str());
			sec.clear();
			s.Get(section.str()+"-levels", sec);
			for(i=sec.begin(); i!=sec.end(); i++)
			{
				setting_t setting = rig_parse_level(i->first.c_str());
				if (RIG_LEVEL_IS_FLOAT(setting))
					caps.settings[ERigMode(j)].levels[i->first].f = atof(i->second.c_str());
				else
					caps.settings[ERigMode(j)].levels[i->first].i = atoi(i->second.c_str());
			}
			sec.clear();
			s.Get(section.str()+"-functions", sec);
			for(i=sec.begin(); i!=sec.end(); i++)
				caps.settings[ERigMode(j)].functions[i->first] = i->second;
			sec.clear();
			s.Get(section.str()+"-parameters", sec);
			for(i=sec.begin(); i!=sec.end(); i++)
			{
				setting_t setting = rig_parse_parm(i->first.c_str());
				if (RIG_LEVEL_IS_FLOAT(setting))
					caps.settings[ERigMode(j)].levels[i->first].f = atof(i->second.c_str());
				else
					caps.settings[ERigMode(j)].levels[i->first].i = atoi(i->second.c_str());
			}
			caps.settings[ERigMode(j)].eOnboardDemod
				= EMight(s.Get(section.str(), "onboarddemod", int(C_CAN)));
			caps.settings[ERigMode(j)].eInChanSel
				= EInChanSel(s.Get(section.str(), "inchansel", int(CS_MIX_CHAN)));
			caps.settings[ERigMode(j)].audioInput = EMight(s.Get(section.str(), "audioinput", -1));
		}
		SetHamlibModelID(model);
	}

	s.Put("Hamlib", "model", model);
	s.Put("Hamlib", "ensmeter", bEnableSMeter);
	s.Put("Hamlib", "freqoffset", iFreqOffset);
	s.Put("Hamlib", "enmodrig", bModRigSettings);
}

void
CHamlib::SaveSettings(CSettings & s)
{
	/* Hamlib Model ID */
	s.Put("Hamlib", "model", iHamlibModelID);
	s.Put("Hamlib", "ensmeter", bEnableSMeter);

	/* Enable DRM modified receiver flag */
	s.Put("Hamlib", "enmodrig", bModRigSettings);
	
	const CRigCaps& caps = CapsHamlibModels[iHamlibModelID];

	for(map<string,string>::const_iterator i=caps.config.begin(); i!=caps.config.end(); i++)
	{
		s.Put("Hamlib-config", i->first, i->second);
	}

	for(map<ERigMode,CRigModeSpecificSettings>::const_iterator j=caps.settings.begin(); j!=caps.settings.end(); j++)
	{
		const CRigModeSpecificSettings& settings = j->second;
		map<string,int>::const_iterator k;
		map<string,string>::const_iterator l;
		map<string,value_t>::const_iterator v;

		stringstream section;
		section << "Hamlib-" << size_t(j->first);

		for(k=settings.modes.begin(); k!=settings.modes.end(); k++)
			s.Put(section.str()+"-modes", k->first, k->second);

		for(v=settings.levels.begin(); v!=settings.levels.end(); v++)
		{
			setting_t setting = rig_parse_level(v->first.c_str());
			if (RIG_LEVEL_IS_FLOAT(setting))
				s.Put(section.str()+"-levels", v->first, v->second.f);
			else
				s.Put(section.str()+"-levels", v->first, v->second.i);
		}

		for(l=settings.functions.begin(); l!=settings.functions.end(); l++)
			s.Put(section.str()+"-functions", l->first, l->second);

		for(v=settings.parameters.begin(); v!=settings.parameters.end(); v++)
		{
			setting_t setting = rig_parse_parm(v->first.c_str());
			if (RIG_PARM_IS_FLOAT(setting))
				s.Put(section.str()+"-parameters", v->first, v->second.f);
			else
				s.Put(section.str()+"-parameters", v->first, v->second.i);
		}
	}
	s.Put("Hamlib", "freqoffset", iFreqOffset);
}

_BOOLEAN
CHamlib::SetFrequency(const int iFreqkHz)
{
	/* Set frequency (consider frequency offset and conversion
		   from kHz to Hz by " * 1000 ") */
	if (pRig && rig_set_freq(pRig, RIG_VFO_CURR, (iFreqkHz + iFreqOffset) * 1000) == RIG_OK)
		return TRUE;
	return FALSE;
}

void CHamlib::SetEnableSMeter(const _BOOLEAN bStatus)
{
cout << "CHamlib::SetEnableSMeter(" << bStatus << ")" << endl;
	if(bStatus)
	{
#ifdef USE_QT_GUI
		if(bEnableSMeter==FALSE)
			start();
#endif
		bEnableSMeter = TRUE;
	}
	else
	{
		Parameters.Lock();
		Parameters.SigStrstat.setInvalid();
		Parameters.Unlock();
		bEnableSMeter = FALSE;
	}
}

_BOOLEAN CHamlib::GetEnableSMeter()
{
	return bEnableSMeter;
}

void CHamlib::run()
{
	while(bEnableSMeter && bSMeterIsSupported && pRig)
	{
		value_t val;
		switch(rig_get_level(pRig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &val))
		{
		case 0:
			Parameters.Lock();
			// Apply any correction
			Parameters.SigStrstat.addSample(val.f + Parameters.rSigStrengthCorrection);
			Parameters.Unlock();
			break;
		case -RIG_ETIMEOUT:
			/* If a time-out happened, do not update s-meter anymore (disable it) */
			bSMeterIsSupported = FALSE;
			break;
		}
#ifdef USE_QT_GUI
		msleep(400);
#endif
	}
}

void
CHamlib::SetRigModes()
{
	ERigMode em = bModRigSettings?DRM_MODIFIED:DRM;

	const map<string,int>& modes = CapsHamlibModels[iHamlibModelID].settings[em].modes;

	for (map < string, int >::const_iterator i = modes.begin(); i != modes.end(); i++)
	{
		rmode_t mode = rig_parse_mode(i->first.c_str());
		if (mode != RIG_MODE_NONE)
		{
			int ret = rig_set_mode(pRig, RIG_VFO_CURR, mode, i->second);
			if (ret != RIG_OK)
				cerr << "Rig set mode failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigLevels()
{
	ERigMode em = bModRigSettings?DRM_MODIFIED:DRM;

	const map<string,value_t>& levels = CapsHamlibModels[iHamlibModelID].settings[em].levels;

	for (map < string, value_t >::const_iterator i = levels.begin(); i != levels.end(); i++)
	{
		setting_t setting = rig_parse_level(i->first.c_str());
		if (setting != RIG_LEVEL_NONE)
		{
			int ret = rig_set_level(pRig, RIG_VFO_CURR, setting, i->second);
			if (ret != RIG_OK)
				cerr << "Rig set level failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigFuncs()
{
	ERigMode em = bModRigSettings?DRM_MODIFIED:DRM;

	const map<string,string>& functions = CapsHamlibModels[iHamlibModelID].settings[em].functions;

	for (map < string, string >::const_iterator i = functions.begin();
		 i != functions.end(); i++)
	{
		setting_t setting = rig_parse_func(i->first.c_str());
		if (setting != RIG_FUNC_NONE)
		{
			int ret =
				rig_set_func(pRig, RIG_VFO_CURR, setting,
							 atoi(i->second.c_str()));
			if (ret != RIG_OK)
				cerr << "Rig set func failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigParams()
{
	ERigMode em = bModRigSettings?DRM_MODIFIED:DRM;

	const map<string,value_t>& parameters = CapsHamlibModels[iHamlibModelID].settings[em].parameters;

	for (map < string, value_t >::const_iterator i = parameters.begin(); i != parameters.end(); i++)
	{
		setting_t setting = rig_parse_parm(i->first.c_str());
		if (setting != RIG_PARM_NONE)
		{
			int ret = rig_set_parm(pRig, setting, i->second);
			if (ret != RIG_OK)
				cerr << "Rig set parm failed: " << rigerror(ret) << endl;
		}
	}
}

void
CHamlib::SetRigConfig()
{
	const map<string,string>& config = CapsHamlibModels[iHamlibModelID].config;

	for (map < string, string >::const_iterator i = config.begin();
		 i != config.end(); i++)
	{
		int ret =
			rig_set_conf(pRig, rig_token_lookup(pRig, i->first.c_str()),
						 i->second.c_str());
		if (ret != RIG_OK)
		{
			rig_cleanup(pRig);
			pRig = NULL;
			throw CGenErr(string("Rig set conf failed: ")+i->first+"="+i->second);
		}
	}
}

void
CHamlib::SetEnableModRigSettings(const _BOOLEAN bNSM)
{
	if (bModRigSettings != bNSM)
	{
		/* Set internal parameter */
		bModRigSettings = bNSM;

		/* Hamlib must be re-initialized with new parameter */
		SetHamlibModelID(iHamlibModelID);
	}
}

void
CHamlib::SetHamlibModelID(const rig_model_t model)
{
	int ret;

	/* Set value for current selected model ID */
	iHamlibModelID = model;

	/* Init frequency offset */
	iFreqOffset = 0;

	try
	{
		/* If rig was already open, close it first */
		if (pRig != NULL)
		{
			/* Close everything */
			rig_close(pRig);
			rig_cleanup(pRig);
			pRig = NULL;
			bSMeterIsSupported = FALSE;
		}

		if (iHamlibModelID == 0)
			throw CGenErr("No rig model ID selected.");

		map < rig_model_t, CRigCaps >::const_iterator m = CapsHamlibModels.find(iHamlibModelID);
		if (m == CapsHamlibModels.end())
			throw CGenErr("invalid rig model ID selected.");

		const CRigCaps& caps = m->second;

		/* Check for special DRM front-end selection */
		if (bModRigSettings)
		{
			iFreqOffset = caps.iFreqOffs;
		}

		/* Init rig */
		pRig = rig_init(iHamlibModelID);
		if (pRig == NULL)
			throw CGenErr("Initialization of hamlib failed.");

/*
cout << "extra levels:" << endl;
		rig_ext_level_foreach (pRig, level, this);
cout << "extra parameters:" << endl;
		rig_ext_parm_foreach(pRig, parm, this);
cout << "tokens:" << endl;
		rig_token_foreach (pRig, token, this);
*/

		SetRigConfig();

		/* Open rig */
		ret = rig_open(pRig);
		if (ret != RIG_OK)
		{
			/* Fail! */
			rig_cleanup(pRig);
			pRig = NULL;

			throw CGenErr("Rig open failed.");
		}

		/* Ignore result, some rigs don't have support for this */
		rig_set_powerstat(pRig, RIG_POWER_ON);

		SetRigModes();
		SetRigLevels();
		SetRigFuncs();
		SetRigParams();
		/* Check if s-meter capabilities are available */
		if (pRig != NULL)
		{
			/* Check if s-meter can be used. Disable GUI control if not */
			bSMeterIsSupported = rig_has_get_level(pRig, RIG_LEVEL_STRENGTH);
			if(bSMeterIsSupported && bEnableSMeter)
			{
				bEnableSMeter = FALSE;
				SetEnableSMeter(TRUE);
			}
		}
	}
	catch(CGenErr GenErr)
	{
		/* Print error message */
		cerr << GenErr.strError << endl;

		/* Disable s-meter */
		bSMeterIsSupported = FALSE;
	}
}
#endif
