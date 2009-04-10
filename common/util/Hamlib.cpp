/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Ollie Haffenden
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

#include <cstdlib>
#include "Hamlib.h"
#ifdef HAVE_LIBHAMLIB
#include "../Parameter.h"
#include "Settings.h"
#include <sstream>
#include <iostream>
#include "../Parameter.h"
#if defined(_WIN32)
# define NOMINMAX
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
#elif defined(__linux)
# ifdef HAVE_LIBHAL
#  include <hal/libhal.h>
# endif
#endif

/* Implementation *************************************************************/
/*
	This code is based on patches and example code from Tomi Manninen and
	Stephane Fillod (developer of hamlib)
*/
CHamlib::CHamlib(CParameter& p):
#ifdef HAVE_QT
    mutex(),
#endif
    Parameters(p), pRig(NULL),
bSMeterWanted(false), bEnableSMeter(false),
ModelID(), WantedModelID(), eRigMode(DRM), CapsHamlibModels(),
iFrequencykHz(0), iFrequencyOffsetkHz(0)
{
	for(size_t j=DRM; j<=WBFM; j++)
	{
		EModulationType e = EModulationType(j);
		ModelID[e] = WantedModelID[e] = 0;
	}

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
		CloseRig();
	}
}

int
CHamlib::PrintHamlibModelList(const rig_caps *caps, void *data)
{
	/* Access data members of class through pointer ((CHamlib*) data) */
	CHamlib* This = (CHamlib *)data;

	rig_model_t model = caps->rig_model;

	/* Store new model in class. */
	stringstream s;
	s << caps->rig_model;
	CRigCaps& r = This->CapsHamlibModels[model];
	r.hamlib_caps = *caps;
	return 1;					/* !=0, we want them all! */
}

void
CHamlib::GetRigList(CRigMap& rigs)
{
	rigs = CapsHamlibModels;
}

void
CHamlib::GetPortList(map < string, string > &ports) const
{
	ports.clear();
/* Config string for com-port selection is different for each platform */
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
	bool bOK = false;
# ifdef HAVE_LIBHAL
	int num_udis;
	char **udis;
	DBusError error;
	LibHalContext *hal_ctx;

	dbus_error_init (&error);
 	if ((hal_ctx = libhal_ctx_new ()) == NULL) {
 		LIBHAL_FREE_DBUS_ERROR (&error);
 		return ;
 	}
 	if (!libhal_ctx_set_dbus_connection (hal_ctx, dbus_bus_get (DBUS_BUS_SYSTEM, &error))) {
		fprintf (stderr, "error: libhal_ctx_set_dbus_connection: %s: %s\n", error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return ;
	}
	if (!libhal_ctx_init (hal_ctx, &error)) {
		if (dbus_error_is_set(&error)) {
			fprintf (stderr, "error: libhal_ctx_init: %s: %s\n", error.name, error.message);
			LIBHAL_FREE_DBUS_ERROR (&error);
		}
		fprintf (stderr, "Could not initialise connection to hald.\n"
		"Normally this means the HAL daemon (hald) is not running or not ready.\n");
		return ;
 	}

	udis = libhal_find_device_by_capability (hal_ctx, "serial", &num_udis, &error);

	if (dbus_error_is_set (&error)) {
		fprintf (stderr, "error: %s: %s\n", error.name, error.message);
		LIBHAL_FREE_DBUS_ERROR (&error);
		return ;
	}
	for (int i = 0; i < num_udis; i++) {
		/*
  		linux.device_file = '/dev/ttyS0'  (string)
  		info.product = '16550A-compatible COM port'  (string)
  		serial.type = 'platform'  (string)
  		serial.port = 0  (0x0)  (int)
		*/
		char *dev = libhal_device_get_property_string (hal_ctx, udis[i], "linux.device_file", &error);
		char *prod = libhal_device_get_property_string (hal_ctx, udis[i], "info.product", &error);
		char *type = libhal_device_get_property_string (hal_ctx, udis[i], "serial.type", &error);
		int port = libhal_device_get_property_int(hal_ctx, udis[i], "serial.port", &error);
		ostringstream s;
		s << port << " - " << type << " [" << prod << "]";
		ports[s.str()] = dev;
		libhal_free_string (dev);
		libhal_free_string (prod);
		libhal_free_string (type);
		bOK = true;
	}

	libhal_free_string_array (udis);
# else
	FILE *p = popen("hal-find-by-capability --capability serial", "r");
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
				bOK = true;
				buf[0] = 0;
			}
			pclose(p2);
		}
	}
	pclose(p);
# endif
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

    io_object_t		comPort;

    // Iterate across all ports found.

    while ((comPort = IOIteratorNext(serialPortIterator)))
    {
        CFStringRef	bsdPathAsCFString;

		// Get the callout device's path (/dev/cu.xxxxx). The callout device should almost always be
		// used: the dialin device (/dev/tty.xxxxx) would be used when monitoring a serial port for
		// incoming calls, e.g. a fax listener.

		bsdPathAsCFString = CFStringRef(IORegistryEntryCreateCFProperty(comPort,
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

		(void) IOObjectRelease(comPort);
    }
#endif
}

void
CHamlib::SetComPort(const string & port)
{
	rig_model_t model = ModelID[eRigMode];
	if(model!=0)
		CapsHamlibModels[model].set_config("rig_pathname", port);
	SetRigModel(eRigMode, model); // close and re-open
}

string CHamlib::GetComPort() const
{
	map<EModulationType,rig_model_t>::const_iterator e = ModelID.find(eRigMode);
	if(e==ModelID.end())
		return "";
	CRigMap::const_iterator m = CapsHamlibModels.find(e->second);
	if(m==CapsHamlibModels.end())
		return "";
	return m->second.get_config("rig_pathname");
}

int CHamlib::level(RIG* rig, const struct confparams *, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	(void)rig;
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

int CHamlib::parm(RIG* rig, const struct confparams *, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	(void)rig;
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

int CHamlib::token(const struct confparams *, rig_ptr_t data)
{
	CHamlib & Hamlib = *((CHamlib *) data);
	(void)Hamlib;
	return 1;					/* !=0, we want them all! */
}

static const char *enums[] = {"DRM", "AM", "USB", "LSB", "CW", "NBFM", "WBFM" };

void
CHamlib::LoadSettings(CSettings & s)
{
	CRigCaps rigcaps;

#ifdef RIG_MODEL_G303
	/* Winradio G303 */
	CapsHamlibModels[RIG_MODEL_G303].set_level(DRM, "ATT", "0");
	CapsHamlibModels[RIG_MODEL_G303].set_level(DRM, "AGC", "3");
#endif

#ifdef RIG_MODEL_G313
	/* Winradio G313 */
	CapsHamlibModels[RIG_MODEL_G313].set_level(DRM, "ATT", "0");
	CapsHamlibModels[RIG_MODEL_G313].set_level(DRM, "AGC", "3");
# ifdef __linux
	CapsHamlibModels[RIG_MODEL_G313].set_config("if_path", "/dreamg3xxif");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(DRM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(AM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(USB, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(LSB, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(NBFM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G313].set_attribute(WBFM, "audiotype", "shm");
# endif
#endif

#ifdef RIG_MODEL_G315
	/* Winradio G315 */
	CapsHamlibModels[RIG_MODEL_G315].set_level(DRM, "ATT", "0");
	CapsHamlibModels[RIG_MODEL_G315].set_level(DRM, "AGC", "3");
# ifdef __linux
	CapsHamlibModels[RIG_MODEL_G315].set_config("if_path", "/dreamg3xxif");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(DRM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(AM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(USB, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(LSB, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(NBFM, "audiotype", "shm");
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(WBFM, "audiotype", "shm");
# endif
	CapsHamlibModels[RIG_MODEL_G315].set_attribute(WBFM, "onboarddemod", "must");
#endif

#ifdef RIG_MODEL_AR7030
	/* AOR 7030 */
	CapsHamlibModels[RIG_MODEL_AR7030].set_mode(DRM, "AM", "6");
	CapsHamlibModels[RIG_MODEL_AR7030].set_level(DRM, "AGC", "5");

	CapsHamlibModels[-RIG_MODEL_AR7030] = CapsHamlibModels[RIG_MODEL_AR7030];
	CapsHamlibModels[RIG_MODEL_AR7030].set_mode(DRM, "AM", "3");

#endif

#ifdef RIG_MODEL_NRD535
	/* JRC NRD 535 */
						 /* AGC=slow */
	CapsHamlibModels[RIG_MODEL_NRD535].set_level(DRM, "AGC", "3");

	CapsHamlibModels[-RIG_MODEL_NRD535] = CapsHamlibModels[RIG_MODEL_NRD535];

	CapsHamlibModels[RIG_MODEL_NRD535].set_mode(DRM, "CW", "12000");
	CapsHamlibModels[RIG_MODEL_NRD535].set_level(DRM, "CWPITCH", "-5000");
	CapsHamlibModels[RIG_MODEL_NRD535].set_level(DRM, "IF", "-2000");
	CapsHamlibModels[RIG_MODEL_NRD535].set_config("offset", "3"); /* kHz frequency offset */
#endif

#ifdef RIG_MODEL_RX320
	/* TenTec RX320D */
	CapsHamlibModels[RIG_MODEL_RX320].set_level(DRM, "AGC", "3");

	CapsHamlibModels[-RIG_MODEL_RX320] = CapsHamlibModels[RIG_MODEL_RX320];

	CapsHamlibModels[RIG_MODEL_RX320].set_mode(DRM, "AM", "6000");
	CapsHamlibModels[RIG_MODEL_RX320].set_level(DRM, "AF", "1");

#endif

#ifdef RIG_MODEL_RX340
	/* TenTec RX340D */
	CapsHamlibModels[RIG_MODEL_RX340].set_level(DRM, "AGC", "3");

	CapsHamlibModels[-RIG_MODEL_RX340] = CapsHamlibModels[RIG_MODEL_RX340];

	CapsHamlibModels[RIG_MODEL_RX340].set_mode(DRM, "USB", "16000");
	CapsHamlibModels[RIG_MODEL_RX340].set_level(DRM, "IF", "2000");
	CapsHamlibModels[RIG_MODEL_RX340].set_level(DRM, "AF", "1");
	CapsHamlibModels[RIG_MODEL_RX340].set_config("offset", "-12");

#endif

#ifdef RIG_MODEL_ELEKTOR507
    /* Elektor USB SDR 5/07 */
	CapsHamlibModels[RIG_MODEL_ELEKTOR507].set_config("offset", "-12");
#endif

	eRigMode = EModulationType(s.Get("Hamlib", "mode", 0));
	bSMeterWanted = s.Get("Hamlib", "smeter", false);

	for(CRigMap::iterator r = CapsHamlibModels.begin(); r != CapsHamlibModels.end(); r++)
	{
		stringstream section;
		rig_model_t model = r->first;
		section << "Hamlib" << ((model<0)?"-Modified-":"-") << abs(model) << "-";
		r->second.LoadSettings(s, section.str());
	}

	/* Hamlib Model ID */
	for(size_t j=DRM; j<=WBFM; j++)
	{
		stringstream sec;
		sec << "Hamlib-" << enums[j];
		WantedModelID[EModulationType(j)] = s.Get(sec.str(), "model", 0);
	}
	/* Initial mode/band */
	eRigMode = EModulationType(s.Get("Hamlib", "mode", int(DRM)));

	rig_model_t model = WantedModelID[eRigMode];

	/* extract config from -C command line arg */
	string command_line_config = s.Get("command", "hamlib-config", string(""));
	if(command_line_config!="")
	{
		istringstream params(command_line_config);
		string name, value;
		while (getline(params, name, '='))
		{
			getline(params, value, ',');
			CapsHamlibModels[model].set_config(name, value);
				// TODO support levels, params, etc.
		}
	}
}

void
CHamlib::SaveSettings(CSettings & s) const
{
	/* Hamlib Model ID */
	for(size_t j=DRM; j<=WBFM; j++)
	{
		stringstream sec;
		sec << "Hamlib-" << enums[j];
		EModulationType e = EModulationType(j);

		map<EModulationType,rig_model_t>::const_iterator m = ModelID.find(e);
		if(m!=ModelID.end() && m->second != 0)
		{
			s.Put(sec.str(), "model", m->second);
		}
		else
		{
			map<EModulationType,rig_model_t>::const_iterator m = WantedModelID.find(e);
			if(m!=WantedModelID.end() && m->second != 0)
				s.Put(sec.str(), "model", m->second);
		}
	}

	s.Put("Hamlib", "smeter", bSMeterWanted);
	s.Get("Hamlib", "mode", int(eRigMode));

	for(CRigMap::const_iterator r = CapsHamlibModels.begin(); r != CapsHamlibModels.end(); r++)
	{
		if(r->first != 0)
		{
			stringstream section;
			rig_model_t model = r->first;
			section << "Hamlib" << ((model<0)?"-Modified-":"-") << abs(model) << "-";
			r->second.SaveSettings(s, section.str());
		}
	}
}

bool
CHamlib::SetFrequency(const int iFreqkHz)
{
    iFrequencykHz = iFreqkHz;

	int iFreqHz = (iFreqkHz + iFrequencyOffsetkHz) * 1000;
	//cout << "CHamlib::SetFrequency input: " << iFreqkHz << " offset: " << iFrequencyOffsetkHz << " Hz: " << iFreqHz << endl;
	if (pRig && rig_set_freq(pRig, RIG_VFO_CURR, iFreqHz) == RIG_OK)
		return true;
	return false;
}

void CHamlib::SetEnableSMeter(const bool bStatus)
{
	bSMeterWanted = bStatus;
	if(bStatus)
	{
#ifdef USE_QT_GUI
		if(bEnableSMeter==false)
		{
			start(); // don't do this except in GUI thread - see CReceiverSettings
		}
#endif
	}
	else
	{
		StopSMeter();
	}
}

bool CHamlib::GetEnableSMeter()
{
	return bEnableSMeter;
}

void CHamlib::StopSMeter()
{
	Parameters.Lock();
	Parameters.Measurements.SigStrstat.invalidate();
	Parameters.Unlock();
	bEnableSMeter = false;
}

void CHamlib::run()
{
	bEnableSMeter = true;
	while(bEnableSMeter)
	{
		value_t val;
		int r = -1;
		mutex.lock();
		if(pRig)
			r = rig_get_level(pRig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &val);
		mutex.unlock();
		if(r == 0)
		{
			Parameters.Lock();
			// Apply any correction
            const _REAL S9_DBuV = 34.0; // S9 in dBuV for converting HamLib S-meter readings
			Parameters.Measurements.SigStrstat.addSample(_REAL(val.i) + S9_DBuV + Parameters.rSigStrengthCorrection);
			Parameters.Unlock();
#ifdef USE_QT_GUI
			msleep(400);
#endif
		}
		else
		{
			/* If a time-out happened, do not update s-meter anymore (disable it) */
			bEnableSMeter = false;
		}
	}
}

void
CHamlib::OpenRig()
{
	rig_model_t iRig = ModelID[eRigMode];
	cerr << "CHamlib::OpenRig " << int(eRigMode) << " " << iRig << " " << abs(iRig) << endl;
	/* Init rig (negative rig numbers indicate modified rigs */
	pRig = rig_init(abs(iRig));
	if (pRig == NULL)
	{
		cerr << "Initialization of hamlib failed." << endl;
		return;
	}

	int ret = CapsHamlibModels[iRig].SetRigConfig(pRig);

	if(ret != RIG_OK)
	{
		cerr << "setrigconfig failed" << endl;
		return;
	}

	/* Open rig */
	ret = rig_open(pRig);
	if (ret != RIG_OK)
	{
		/* Fail! */
		cerr << "rig_open failed" << endl;
		CloseRig();
	}
}

void
CHamlib::CloseRig()
{
	if(bEnableSMeter)
	{
		bEnableSMeter = false;
#ifdef USE_QT_GUI
		if(wait(1000) == false)
			cerr << "error terminating rig polling thread" << endl;
#endif
	}
	/* If rig was already open, close it first */
	if (pRig != NULL)
	{
		/* Close everything */
		mutex.lock();
		rig_close(pRig);
		rig_cleanup(pRig);
		pRig = NULL;
		mutex.unlock();
	}
}

void
CHamlib::SetRigModelForAllModes(rig_model_t model)
{
	for(size_t j=DRM; j<=WBFM; j++)
	{
		WantedModelID[EModulationType(j)] = model;
	}
	SetRigModel(eRigMode,  WantedModelID[eRigMode]);
}

void
CHamlib::SetRigModel(EModulationType eNewMode, rig_model_t model)
{
	// close the rig
	rig_model_t old_model = ModelID[eRigMode];

	if(old_model!=0)
		CloseRig();

	/* Set value for current selected model ID */
	eRigMode = eNewMode;
	ModelID[eRigMode] = model;

	/* if no Rig is wanted we are done */
	if(model == 0)
		return;

	CRigMap::iterator m = CapsHamlibModels.find(model);
	if (m == CapsHamlibModels.end())
	{
		cerr << "invalid rig model ID selected: " << model;
		return;
	}

	CRigCaps& RigCaps = CapsHamlibModels[ModelID[eRigMode]];

	OpenRig();

	if (pRig == NULL)
	{
		cerr << "Open Rig failed" << endl;
		return;
	}

	/* Ignore result, some rigs don't have support for this */
	rig_set_powerstat(pRig, RIG_POWER_ON);

	RigCaps.SetRigModes(pRig, eRigMode);
	RigCaps.SetRigLevels(pRig, eRigMode);
	RigCaps.SetRigFuncs(pRig, eRigMode);
	RigCaps.SetRigParams(pRig, eRigMode);

	if(pRig==NULL)
		return; // something went wrong

	stringstream offset(RigCaps.get_config("offset"));
	offset >> iFrequencyOffsetkHz;

	if (iFrequencykHz != 0)
		  SetFrequency(iFrequencykHz);

	/* Check if s-meter capabilities are available */
	if(bSMeterWanted && rig_has_get_level(pRig, RIG_LEVEL_STRENGTH))
		SetEnableSMeter(true);
}

int
CRigCaps::mode(EModulationType m, const string& key) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return -1;
	map<string,int>::const_iterator k = s->second.modes.find(key);
	if(k==s->second.modes.end())
		return -1;
	return k->second;
}

void
CRigCaps::get_level(EModulationType m, const string& key, int& val) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return;
	map<string,value_t>::const_iterator k = s->second.levels.find(key);
	if(k==s->second.levels.end())
		return;
	val = k->second.i;
}

void
CRigCaps::get_level(EModulationType m, const string& key, float& val) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return;
	map<string,value_t>::const_iterator k = s->second.levels.find(key);
	if(k==s->second.levels.end())
		return;
	val = k->second.f;
}

string
CRigCaps::function(EModulationType m, const string& key) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return "";
	map<string,string>::const_iterator k = s->second.functions.find(key);
	if(k==s->second.functions.end())
		return "";
	return k->second;
}

string
CRigCaps::attribute(EModulationType m, const string& key) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return "";
	map<string,string>::const_iterator k = s->second.attributes.find(key);
	if(k==s->second.attributes.end())
		return "";
	return k->second;
}

void
CRigCaps::get_parameter(EModulationType m, const string& key, int& val) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return;
	map<string,value_t>::const_iterator k = s->second.parameters.find(key);
	if(k==s->second.parameters.end())
		return;
	val = k->second.i;
}

void
CRigCaps::get_parameter(EModulationType m, const string& key, float& val) const
{
	map<EModulationType,CRigModeSpecificSettings>::const_iterator s = settings.find(m);
	if(s==settings.end())
		return;
	map<string,value_t>::const_iterator k = s->second.parameters.find(key);
	if(k==s->second.parameters.end())
		return;
	val = k->second.f;
}

void
CRigCaps::set_mode(EModulationType m, const string& key, const string& val)
{
	settings[m].modes[key] = atoi(val.c_str());
}

void
CRigCaps::set_level(EModulationType m, const string& key, const string& val)
{
	setting_t setting = rig_parse_level(key.c_str());
	if (RIG_LEVEL_IS_FLOAT(setting))
		settings[m].levels[key].f = atof(val.c_str());
	else
		settings[m].levels[key].i = atoi(val.c_str());
}

void
CRigCaps::set_function(EModulationType m, const string& key, const string& val)
{
	settings[m].functions[key] = val;
}

void
CRigCaps::set_attribute(EModulationType m, const string& key, const string& val)
{
	settings[m].attributes[key] = val;
}

void
CRigCaps::set_parameter(EModulationType m, const string& key, const string& val)
{
	setting_t setting = rig_parse_parm(key.c_str());
	if (RIG_LEVEL_IS_FLOAT(setting))
		settings[m].parameters[key].f = atof(val.c_str());
	else
		settings[m].parameters[key].i = atoi(val.c_str());
}

void
CRigCaps::SetRigModes(RIG* pRig, EModulationType eRigMode)
{
	const map<string,int>& modes = settings[eRigMode].modes;

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
CRigCaps::SetRigLevels(RIG* pRig, EModulationType eRigMode)
{
	const map<string,value_t>& levels = settings[eRigMode].levels;

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
CRigCaps::SetRigFuncs(RIG* pRig, EModulationType eRigMode)
{
	const map<string,string>& functions = settings[eRigMode].functions;

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
CRigCaps::SetRigParams(RIG* pRig, EModulationType eRigMode)
{
	const map<string,value_t>& parameters = settings[eRigMode].parameters;

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

int
CRigCaps::SetRigConfig(RIG* pRig)
{
	for (map < string, string >::const_iterator i = config.begin();
		 i != config.end(); i++)
	{
		int ret =
			rig_set_conf(pRig, rig_token_lookup(pRig, i->first.c_str()),
						 i->second.c_str());
		if (ret != RIG_OK)
		{
			cerr << "Rig set conf failed: " << i->first << "=" << i->second;
			return ret;
		}
	}
	return RIG_OK;
}

void
CRigCaps::LoadSettings(const CSettings& s, const string& secpref)
{
	INISection sec;
	s.Get(secpref+"config", sec);
	INISection::iterator i;
	for(i=sec.begin(); i!=sec.end(); i++)
		set_config(i->first, i->second);
	for(size_t j=DRM; j<=WBFM; j++)
	{
		EModulationType m = EModulationType(j);
		string section = secpref + string(enums[j]) + string("-");
		sec.clear();
		s.Get(section+"modes", sec);
		for(i=sec.begin(); i!=sec.end(); i++)
			set_mode(m, i->first, i->second);
		sec.clear();
		s.Get(section+"levels", sec);
		for(i=sec.begin(); i!=sec.end(); i++)
			set_level(m, i->first, i->second);
		sec.clear();
		s.Get(section+"functions", sec);
		for(i=sec.begin(); i!=sec.end(); i++)
			set_function(m, i->first, i->second);
		sec.clear();
		s.Get(section+"parameters", sec);
		for(i=sec.begin(); i!=sec.end(); i++)
			set_parameter(m, i->first, i->second);
		s.Get(section+"attributes", sec);
		for(i=sec.begin(); i!=sec.end(); i++)
			set_attribute(m, i->first, i->second);
	}
}

void
CRigCaps::SaveSettings(CSettings& s, const string& sec) const
{
	for(map<string,string>::const_iterator i=config.begin(); i!=config.end(); i++)
	{
		s.Put(sec+"config", i->first, i->second);
	}

	for(map<EModulationType,CRigModeSpecificSettings>::const_iterator j=settings.begin();
		j!=settings.end(); j++)
	{
		const CRigModeSpecificSettings& settings = j->second;
		map<string,int>::const_iterator k;
		map<string,string>::const_iterator l;
		map<string,value_t>::const_iterator v;
		string section = sec + string(enums[j->first]) + string("-");

		for(k=settings.modes.begin(); k!=settings.modes.end(); k++)
			s.Put(section+"modes", k->first, k->second);

		for(v=settings.levels.begin(); v!=settings.levels.end(); v++)
		{
			setting_t setting = rig_parse_level(v->first.c_str());
			if (RIG_LEVEL_IS_FLOAT(setting))
				s.Put(section+"levels", v->first, v->second.f);
			else
				s.Put(section+"levels", v->first, v->second.i);
		}

		for(l=settings.functions.begin(); l!=settings.functions.end(); l++)
			s.Put(section+"functions", l->first, l->second);

		for(v=settings.parameters.begin(); v!=settings.parameters.end(); v++)
		{
			setting_t setting = rig_parse_parm(v->first.c_str());
			if (RIG_PARM_IS_FLOAT(setting))
				s.Put(section+"parameters", v->first, v->second.f);
			else
				s.Put(section+"parameters", v->first, v->second.i);
		}

		for(l=settings.attributes.begin(); l!=settings.attributes.end(); l++)
			s.Put(section+"attributes", l->first, l->second);
	}
}

#endif
