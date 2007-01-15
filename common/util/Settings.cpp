/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Tomi Manninen, Stephane Fillod, Robert Kesterson,
 *	Andrea Russo
 *
 * Description:
 *
 * 10/03/2003 Tomi Manninen / OH2BNS
 *  - Initial (basic) code for command line argument parsing (argv)
 * 04/15/2004 Tomi Manninen, Stephane Fillod
 *  - Hamlib
 * 07/27/2004
 *  - included stlini routines written by Robert Kesterson
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

#include "Settings.h"
#ifdef _WIN32
#include "windows.h"
#endif

#include "iostream"
#include "sstream"

/* Implementation *************************************************************/
void CSettings::Load(int argc, char** argv)
{
	/* Defaults */
	Put("Global", "role", "receiver");

	/* First load settings from init-file and then parse command line arguments.
	   The command line arguments overwrite settings in init-file! */
	ReadIniFile();

	ParseArguments(argc, argv);
#if 1
	for(INIFile::iterator i=iniFile.begin(); i!=iniFile.end(); i++)
	{
      cout << "[" << i->first << "]" << endl;
      for(INISection::iterator j=i->second.begin(); j!=i->second.end(); j++)
			   cout << j->first << "=" << j->second << endl;
    }
#endif	
}

void CSettings::Save()
{
	/* Just write settings in init-file */
	WriteIniFile();
}

string CSettings::Get(const string& sSection, const string& sKey, const string& sDefaultVal)
{
	INIFile::iterator s = iniFile.find(sSection);
	if(s == iniFile.end())
		return "";
	INISection::iterator k = s->second.find(sKey);
	if(k == s->second.end())
		return "";
	return k->second;
}

void CSettings::Put(const string& sSection, const string& sKey, const string& sValue)
{
 	 iniFile[sSection][sKey]=sValue;
}

void CSettings::Put(const string& sSection, const string& sKey, const int iValue)
{
 	 stringstream s;
 	 s << iValue;
 	 iniFile[sSection][sKey]=s.str();
}

/* Read and write init-file ***************************************************/
void CSettings::ReadIniFile()
{
	/* Load data from init-file */
	iniFile = LoadIni(DREAM_INIT_FILE_NAME);
}

void CSettings::WriteIniFile()
{
	/* Save settings in init-file */
	SaveIni(iniFile, DREAM_INIT_FILE_NAME);
}

_BOOLEAN CSettings::GetNumericIniSet(INIFile& theINI, string strSection,
									 string strKey, int iRangeStart,
									 int iRangeStop, int& iValue)
{
	/* Init return value */
	_BOOLEAN bReturn = FALSE;

	const string strGetIni =
		GetIniSetting(theINI, strSection.c_str(), strKey.c_str());

	/* Check if it is a valid parameter */
	if (!strGetIni.empty())
	{
		iValue = atoi(strGetIni.c_str());

		/* Check range */
		if ((iValue >= iRangeStart) && (iValue <= iRangeStop))
			bReturn = TRUE;
	}

	return bReturn;
}

void CSettings::SetNumericIniSet(INIFile& theINI, string strSection,
								 string strKey, int iValue)
{
	char cString[256];

	sprintf(cString, "%d", iValue);
	PutIniSetting(theINI, strSection.c_str(), strKey.c_str(), cString);
}

_BOOLEAN CSettings::GetFlagIniSet(INIFile& theINI, string strSection,
								  string strKey, _BOOLEAN& bValue)
{
	/* Init return value */
	_BOOLEAN bReturn = FALSE;

	const string strGetIni =
		GetIniSetting(theINI, strSection.c_str(), strKey.c_str());

	if (!strGetIni.empty())
	{
		if (atoi(strGetIni.c_str()))
			bValue = TRUE;
		else
			bValue = FALSE;

		bReturn = TRUE;
	}

	return bReturn;
}

void CSettings::SetFlagIniSet(INIFile& theINI, string strSection, string strKey,
							  _BOOLEAN bValue)
{
	if (bValue == TRUE)
		PutIniSetting(theINI, strSection.c_str(), strKey.c_str(), "1");
	else
		PutIniSetting(theINI, strSection.c_str(), strKey.c_str(), "0");
}


/* Command line argument parser ***********************************************/
void CSettings::ParseArguments(int argc, char** argv)
{
	string		strArgument;

	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	for (int i = 1; i < argc; i++)
	{
		/* DRM transmitter mode flag ---------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-t", "--transmitter") == TRUE)
		{
			Put("Global", "role", "transmitter");
			continue;
		}

		
		/* Flip spectrum flag ----------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-p", "--flipspectrum") == TRUE)
		{
			Put("Receiver", "flipspectrum", "1");
			continue;
		}

		/* Mute audio flag -------------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-m", "--muteaudio") == TRUE)
		{
			Put("Receiver", "muteaudio", "1");
			continue;
		}

		/* Bandpass filter flag --------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-F", "--filter") == TRUE)
		{
			Put("Receiver", "filter", "1");
			continue;
		}

		/* Modified metrics flag -------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-D", "--modmetric") == TRUE)
		{
			Put("Receiver", "modmetric", "1");
			continue;
		}

		/* Sound In device -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-I", "--snddevin", strArgument) == TRUE)
		{
			Put("Global", "snddevin", strArgument);
			continue;
		}

		/* Sound Out device ------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-O", "--snddevout", strArgument) == TRUE)
		{
			Put("Global", "snddevout", strArgument);
			continue;
		}

		/* Do not use sound card, read from file ---------------------------- */
		if (GetStringArgument(argc, argv, i, "-f", "--fileio", strArgument) == TRUE)
		{
			Put("Global", "filein", strArgument);
			continue;
		}

 		/* Do not use sound card, write COFDM to file ----------------------- */
 		if (GetStringArgument(argc, argv, i, "-x", "--fileout", strArgument) == TRUE)
 		{
			Put("Global", "fileout", strArgument);
 			continue;
 		}
 
 		/* set COFDM output format ------------------------------------------ */
 		if (GetStringArgument(argc, argv, i, "-z", "--outfmt", strArgument) == TRUE)
 		{
			Put("Transmitter", "outfmt", strArgument);
 			continue;
 		}

		/* Write output data to file as WAV --------------------------------- */
		if (GetStringArgument(argc, argv, i, "-w", "--writewav", strArgument) == TRUE)
		{
			Put("Global", "writewav", strArgument);
			continue;
		}
		
		/* Number of iterations for MLC setting ----------------------------- */
		if (GetStringArgument(argc, argv, i, "-i", "--mlciter", strArgument) == TRUE)
		{
			Put("Receiver", "mlciter", strArgument);
			continue;
		}

		/* Sample rate offset start value ----------------------------------- */
		if (GetStringArgument(argc, argv, i, "-s", "--sampleoff", strArgument) == TRUE)
		{
			Put("Receiver", "sampleoff", strArgument);
			continue;
		}

		/* Frequency acquisition search window size ------------------------- */
		if (GetStringArgument(argc, argv, i, "-S", "--fracwinsize", strArgument) == TRUE)
		{
			Put("Receiver", "fracwinsize", strArgument);
			continue;
		}

		/* Frequency acquisition search window center ----------------------- */
		if (GetStringArgument(argc, argv, i, "-E", "--fracwincent", strArgument) == TRUE)
		{
			Put("Receiver", "fracwincent", strArgument);
			continue;
		}

		/* Input channel selection ------------------------------------------ */
		if (GetStringArgument(argc, argv, i, "-c", "--inchansel", strArgument) == TRUE)
		{
			Put("Receiver", "inchansel", strArgument);
			continue;
		}

		/* Output channel selection ----------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-u", "--outchansel", strArgument) == TRUE)
		{
			Put("Receiver", "outchansel", strArgument);
			Put("Transmitter", "outchansel", strArgument);
			continue;
		}

		/* Wanted RF Frequency   ------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-r", "--frequency", strArgument) == TRUE)
		{
			Put("Receiver", "frequency", strArgument);
			continue;
		}

		/* Enable/Disable process priority flag */
		if (GetStringArgument(argc, argv, i, "-P", "--processpriority", strArgument) == TRUE)
		{
			Put("Receiver", "processpriority", strArgument);
			Put("Transmitter", "processpriority", strArgument);
			continue;
		}

		/* enable/disable epg decoding ----------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-e", "--decodeepg", strArgument) == TRUE)
		{
			Put("Receiver", "decodeepg", strArgument);
			continue;
		}

		/* log enable flag  ---------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-g", "--enablelog", strArgument) == TRUE)
		{
			Put("Logfile", "enablelog", strArgument);
			continue;
		}

		/* log file delay value  ---------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-l", "--logdelay", strArgument) == TRUE)
		{
			Put("Logfile", "delay", strArgument);
			continue;
		}

		/* Latitude string for log file ------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-a", "--latitude", strArgument) == TRUE)
		{
			Put("Logfile", "latitude", strArgument);
			continue;
		}

		/* Longitude string for log file ------------------------------------ */
		if (GetStringArgument(argc, argv, i, "-o", "--longitude", strArgument) == TRUE)
		{
			Put("Logfile", "longitude", strArgument);
			continue;
		}

		/* Color scheme main plot ------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-y", "--colorscheme", strArgument) == TRUE)
		{
			Put("GUI", "colorscheme", strArgument);
			continue;
		}

		/* MDI out address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-mdiout", "--mdiout", strArgument) == TRUE)
		{
			Put("Transmitter", "mdiout", strArgument);
			continue;
		}

		/* MDI in address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-mdiin", "--mdiin", strArgument) == TRUE)
		{
			Put("Transmitter", "mdiin", strArgument);
			continue;
		}

		/* RSCI status output profile */
		if (GetStringArgument(argc, argv, i, "--rsioutprofile", "--rsioutprofile",
			strArgument) == TRUE)
		{
			Put("Receiver", "rsioutprofile", strArgument);
			continue;
		}

		/* RSCI status out address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsiout", "--rsiout",
			strArgument) == TRUE)
		{
			Put("Receiver", "rsiout", strArgument);
			continue;
		}

		/* RSCI status in address -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "--rsiin", "--rsiin",
			strArgument) == TRUE)
		{
			Put("Receiver", "rsiin", strArgument);
			continue;
		}

		/* RSCI control out address */
		if (GetStringArgument(argc, argv, i, "--rciout", "--rciout",
			strArgument) == TRUE)
		{
			Put("Receiver", "rciout", strArgument);
			continue;
		}

		/* OPH: RSCI control in address */
		if (GetStringArgument(argc, argv, i, "--rciin", "--rciin",
			strArgument) == TRUE)
		{
			Put("Receiver", "rciin", strArgument);
			continue;
		}

#ifdef HAVE_LIBHAMLIB
		/* Hamlib config string --------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-C", "--hamlib-config",
			strArgument) == TRUE)
		{
			Put("Hamlib", "hamlib-config", strArgument);
			continue;
		}

		/* Hamlib Model ID -------------------------------------------------- */
		if (GetStringArgument(argc, argv, i, "-M", "--hamlib-model",
			strArgument) == TRUE)
		{
			Put("Hamlib", "hamlib-model", strArgument);
			continue;
		}

# ifdef USE_QT_GUI
		/* Enable s-meter flag ---------------------------------------------- */
		if (GetFlagArgument(argc, argv, i, "-T", "--ensmeter") == TRUE)
		{
			Put("Hamlib", "ensmeter", "1");
			continue;
		}
# endif
#endif

		/* Help (usage) flag ------------------------------------------------ */
		if ((!strcmp(argv[i], "--help")) ||
			(!strcmp(argv[i], "-h")) ||
			(!strcmp(argv[i], "-?")))
		{
			const string strHelp = UsageArguments(argv);

#if defined(_WIN32)
			MessageBox(NULL, strHelp.c_str(), "Dream",
				MB_SYSTEMMODAL | MB_OK | MB_ICONINFORMATION);
#else
			cerr << strHelp;
#endif

			exit(1);
		}


		/* Unknown option --------------------------------------------------- */
		cerr << argv[0] << ": ";
		cerr << "Unknown option '" << argv[i] << "' -- use '--help' for help"
			<< endl;

		exit(1);
	}
}

string CSettings::UsageArguments(char** argv)
{
// TODO: Use macro definitions for help text, too (instead of hard-coded numbers)!

	return
		"Usage: " + string(argv[0]) + " [option] [argument]\n"
		"Recognized options:\n"
		"  -t, --transmitter           DRM transmitter mode\n"
		"  -p, --flipspectrum          flip input spectrum\n"
		"  -i <n>, --mlciter <n>       number of MLC iterations (allowed range: 0...4 default: 1)\n"
		"  -s <r>, --sampleoff <r>     sample rate offset initial value [Hz] (allowed range: -200.0...200.0)\n"
		"  -m, --muteaudio             mute audio output\n"
		"  -f <s>, --fileio <s>        disable sound card input, use file <s> instead\n"
		"  -z <s>, --fileout <s>       disable sound card COFDM output, use file <s> instead\n"
		"  -x <s>, --outfmt <s>        disable sound card, use file <s> instead\n"
		"  -w <s>, --writewav <s>      write decoded audio output to wave file\n"
		"  -S <r>, --fracwinsize <r>   freq. acqu. search window size [Hz]\n"
		"  -E <r>, --fracwincent <r>   freq. acqu. search window center [Hz]\n"
		"  -F, --filter                apply bandpass filter\n"
		"  -D, --modmetric             enable modified metrics\n"
		"  -c <n>, --inchansel <n>     input channel selection\n"
		"                              0: left channel;   1: right channel;   2: mix both channels (default)\n"
		"                              3: I / Q input positive;   4: I / Q input negative\n"
		"                              5: I / Q input positive (0 Hz IF);   6: I / Q input negative (0 Hz IF)\n"
		"  -u <n>, --outchansel <n>    output channel selection\n"
		"                              0: L -> L, R -> R (default);   1: L -> L, R muted;   2: L muted, R -> R\n"
		"                              3: mix -> L, R muted;   4: L muted, mix -> R\n"
		"  -e <n>, --decodeepg <n>     enable/disable epg decoding (0: off; 1: on)\n"

#ifdef USE_QT_GUI
		"  -g <n>, --enablelog <n>     enable/disable logging (0: no logging; 1: logging\n"
		"  -r <n>, --frequency <n>     set frequency [kHz] for log file\n"
		"  -l <n>, --logdelay <n>      delay start of logging by <n> seconds, allowed range: 0...3600)\n"
		"  -y <n>, --colorscheme <n>   set color scheme for main plot\n"
		"                              0: blue-white (default);   1: green-black;   2: black-grey\n"
#endif
		"  --mdiout <s>                MDI out address format [IP#:]IP#:port (for Content Server)\n"
		"  --mdiin  <s>                MDI in address (for modulator) [[IP#:]IP:]port\n"
		"  --rsioutprofile <s>         MDI/RSCI output profile: A|B|C|D|Q|M\n"
		"  --rsiout <s>                MDI/RSCI output address format [IP#:]IP#:port\n"
		"  --rsiin <s>                 RSCI/MDI status input address format [[IP#:]IP#:]port|file\n"
		"  --rciout <s>                RSCI Control output format IP#:port\n"
		"  --rciin <s>                 RSCI Control input address number format [IP#:]port\n"

		"  -I <n>, --snddevin <n>      set sound in device\n"
		"  -O <n>, --snddevout <n>     set sound out device\n"

#ifdef HAVE_LIBHAMLIB
		"  -M <n>, --hamlib-model <n>  set Hamlib radio model ID\n"
		"  -C <s>, --hamlib-config <s> set Hamlib config parameter\n"
#endif
		"  -T, --ensmeter              enable S-Meter\n"
#ifdef WIN32
		"  -P, --processpriority <n>   enable/disable high priority for working thread\n"
#endif

		"  -h, -?, --help             this help text\n"
		"Example: " + string(argv[0]) +
		" -p --sampleoff -0.23 -i 2 "
#ifdef USE_QT_GUI
		"-r 6140 --rsiout 127.0.0.1:3002"
#endif
		"\n";
}

_BOOLEAN CSettings::GetFlagArgument(int argc, char** argv, int& i,
									string strShortOpt, string strLongOpt)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
		return TRUE;
	else
		return FALSE;
}

_BOOLEAN CSettings::GetStringArgument(int argc, char** argv, int& i,
									  string strShortOpt, string strLongOpt,
									  string& strArg)
{
	if ((!strShortOpt.compare(argv[i])) || (!strLongOpt.compare(argv[i])))
	{
		if (++i >= argc)
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a string argument" << endl;
			exit(1);
		}

		strArg = argv[i];

		return TRUE;
	}
	else
		return FALSE;
}

/* INI File routines using the STL ********************************************/
/* The following code was taken from "INI File Tools (STLINI)" written by
   Robert Kesterson in 1999. The original files are stlini.cpp and stlini.h.
   The homepage is http://robertk.com/source

   Copyright August 18, 1999 by Robert Kesterson */

#ifdef _MSC_VER
/* These pragmas are to quiet VC++ about the expanded template identifiers
   exceeding 255 chars. You won't be able to see those variables in a debug
   session, but the code will run normally */
#pragma warning (push)
#pragma warning (disable : 4786 4503)
#endif

string CSettings::GetIniSetting(CSettings::INIFile& theINI, const char* section,
								const char* key, const char* defaultval)
{
	string result(defaultval);
	INIFile::iterator iSection = theINI.find(string(section));

	if (iSection != theINI.end())
	{
		INISection::iterator apair = iSection->second.find(string(key));

		if (apair != iSection->second.end())
			result = apair->second;
	}

	return result;
}

void CSettings::PutIniSetting(CSettings::INIFile &theINI, const char *section,
							  const char *key, const char *value)
{
	INIFile::iterator		iniSection;
	INISection::iterator	apair;
	
	if ((iniSection = theINI.find(string(section))) == theINI.end())
	{
		/* No such section? Then add one */
		INISection newsection;
		if (key)
		{
			newsection.insert(
				std::pair<std::string, string>(string(key), string(value)));
		}

		theINI.insert(
			std::pair<string, INISection>(string(section), newsection));
	}
	else if (key)
	{	
		/* Found section, make sure key isn't in there already, 
		   if it is, just drop and re-add */
		apair = iniSection->second.find(string(key));
		if (apair != iniSection->second.end())
			iniSection->second.erase(apair);

		iniSection->second.insert(
			std::pair<string, string>(string(key), string(value)));
	}
}

CSettings::INIFile CSettings::LoadIni(const char* filename)
{
	INIFile			theINI;
	char			*value, *temp;
	string			section;
	char			buffer[MAX_INI_LINE];
	std::fstream	file(filename, std::ios::in);

	while (file.good())
	{
		memset(buffer, 0, sizeof(buffer));
		file.getline(buffer, sizeof(buffer));

		if ((temp = strchr(buffer, '\n')))
			*temp = '\0'; /* Cut off at newline */

		if ((temp = strchr(buffer, '\r')))
			*temp = '\0'; /* Cut off at linefeeds */

		if ((buffer[0] == '[') && (temp = strrchr(buffer, ']')))
		{   /* if line is like -->   [section name] */
			*temp = '\0'; /* Chop off the trailing ']' */
			section = &buffer[1];
			PutIniSetting(theINI, &buffer[1]); /* Start new section */
		}
		else if (buffer[0] && (value = strchr(buffer, '=')))
		{
			/* Assign whatever follows = sign to value, chop at "=" */
			*value++ = '\0';

			/* And add both sides to INISection */
			PutIniSetting(theINI, section.c_str(), buffer, value);
		}
		else if (buffer[0])
		{
			/* Must be a comment or something */
			PutIniSetting(theINI, section.c_str(), buffer, "");
		}
	}
	return theINI;
}

void CSettings::SaveIni(CSettings::INIFile &theINI, const char* filename)
{
	_BOOLEAN bFirstSection = TRUE; /* Init flag */

	std::fstream file(filename, std::ios::out);
	if(!file.good())
		return;
	
	/* Just iterate the hashes and values and dump them to a file */
	INIFile::iterator section = theINI.begin();
	while (section != theINI.end())
	{
		if (section->first > "")
		{
			if (bFirstSection == TRUE)
			{
				/* Don't put a newline at the beginning of the first section */
				file << "[" << section->first << "]" << std::endl;

				/* Reset flag */
				bFirstSection = FALSE;
			}
			else
				file << std::endl << "[" << section->first << "]" << std::endl;
		}

		INISection::iterator pair = section->second.begin();
	
		while (pair != section->second.end())
		{
			if (pair->second > "")
				file << pair->first << "=" << pair->second << std::endl;
			else
				file << pair->first << "=" << std::endl;
			pair++;
		}
		section++;
	}
	file.close();
}

/* Return true or false depending on whether the first string is less than the
   second */
bool CSettings::StlIniCompareStringNoCase::operator()(const string& x,
													  const string& y) const
{
#ifdef WIN32
	return (_stricmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
#ifdef strcasecmp
	return (strcasecmp(x.c_str(), y.c_str()) < 0) ? true : false;
#else
	unsigned	nCount = 0;
	int			nResult = 0;
	const char	*p1 = x.c_str();
	const char	*p2 = y.c_str();

	while (*p1 && *p2)
	{
		nResult = toupper(*p1) - toupper(*p2);
		if (nResult != 0)
			break;
		p1++;
		p2++;
		nCount++;
	}
	if (nResult == 0)
	{
		if (*p1 && !*p2)
			nResult = -1;
		if (!*p1 && *p2)
			nResult = 1;
	}
	if (nResult < 0)
		return true;
	return false;
#endif /* strcasecmp */
#endif
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
