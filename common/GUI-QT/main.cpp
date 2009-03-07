/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod
 *
 * Description:
 *
 * 11/10/2004 Stephane Fillod
 *	- QT translation
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

#ifdef _WIN32
# ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
# endif
# define _WIN32_WINNT 0x0400
# include <windows.h>
#endif

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "../DrmSimulation.h"
#include "../util/Settings.h"
#ifdef HAVE_LIBHAMLIB
# include "../util/Hamlib.h"
#endif
#ifdef USE_QT_GUI
# include <QApplication>
# include <QThread>
# include <QMessageBox>
# include <QTranslator>
# include "DRMMainWindow.h"
# include "TransmDlg.h"
#endif
#include <iostream>

/* Implementation *************************************************************/
#ifdef USE_QT_GUI
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/

int
main(int argc, char **argv)
{
	/* create app before running Settings.Load to consume platform/QT parameters */
	QApplication app(argc, argv);

#if defined(__APPLE__)
	/* find plugins on MacOs when deployed in a bundle */
	app.setLibraryPaths(app.applicationDirPath()+"../PlugIns");
#endif

	/* Load and install multi-language support (if available) */
	QTranslator translator(0);
	if (translator.load("dreamtr"))
		app.installTranslator(&translator);

	CDRMSimulation DRMSimulation;

	/* Call simulation script. If simulation is activated, application is
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	CSettings Settings;
	/* Parse arguments and load settings from init-file */
	Settings.Load(argc, argv);

	try
	{

#ifdef _WIN32
		/* works for both transmit and receive. GUI is low, working is normal.
		 * the working thread does not need to know what the setting is.
		 */
			if (Settings.Get("GUI", "processpriority", 1) != 0)
			{
				/* Set priority class for this application */
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

				/* Low priority for GUI thread */
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
				Settings.Put("GUI", "processpriority", 1);
			}
			else
			{
				Settings.Put("GUI", "processpriority", 0);
			}
#endif

		string strMode = Settings.Get("0", "mode", string("RX"));

		bool bShowHelp = Settings.Get("command", "help", 0);
		if(bShowHelp)
		{
			string strError = Settings.Get("command", "error", string(""));

			string strHelp = Settings.UsageArguments(argv);
			if(strError != "")
			{
				strHelp = strError + " is not a valid argument\n" + strHelp;
			}

			QMessageBox::information(0, "Dream", strHelp.c_str());
			exit(0);
		}

		if (strMode == "RX")
		{
			CDRMReceiver DRMReceiver;
			DRMReceiver.LoadSettings(Settings);

			/* First, initialize the working thread. This should be done in an extra
			   routine since we cannot 100% assume that the working thread is
			   ready before the GUI thread */

#ifdef HAVE_LIBHAMLIB
			/* initialise Hamlib first, so that when the Receiver is
			 * initialised it can tune the front end
			 */

            CHamlib *pHamlib = NULL;
			string rsi = Settings.Get("command", "rsiin", string(""));
            string fio = Settings.Get("command", "fileio", string(""));
			if(rsi == "" && fio == "") /* don't initialise hamlib if RSCI or file input is requested */
			{
				pHamlib = new CHamlib(*DRMReceiver.GetParameters());
				pHamlib->LoadSettings(Settings);
				DRMReceiver.SetHamlib(pHamlib);
			}
#endif

			DRMMainWindow MainDlg(DRMReceiver, Settings, 0, 0, Qt::WStyle_MinMax);

			/* Start working thread */
			DRMReceiver.start();

			/* Set main window */
			app.setMainWidget(&MainDlg);

			app.exec();

#ifdef HAVE_LIBHAMLIB
			if(pHamlib)
			{
				pHamlib->StopSMeter();
				if (pHamlib->wait(1000) == false)
					cout << "error terminating rig polling thread" << endl;
				pHamlib->SaveSettings(Settings);
			}
#endif
			DRMReceiver.SaveSettings(Settings);
		}
		else if (strMode == "TX" || strMode == "ENC" || strMode == "MOD")
		{
			CDRMTransmitter DRMTransmitter;

			DRMTransmitter.LoadSettings(Settings);

			TransmDialog MainDlg(DRMTransmitter, Settings, NULL, NULL, Qt::WStyle_MinMax);
			/* Set main window */
			app.setMainWidget(&MainDlg);

			/* Show dialog */
			MainDlg.show();
			app.exec();

			DRMTransmitter.SaveSettings(Settings);
		}
		else
		{
			QMessageBox::information(0, "Dream", Settings.UsageArguments(argv).c_str());
		}
	}

	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}
	catch(string strError)
	{
		ErrorMessage(strError);
	}
	catch(char *Error)
	{
		ErrorMessage(Error);
	}

	/* Save settings to init-file */
	Settings.Save();

	return 0;
}

/* Implementation of global functions *****************************************/

void
ErrorMessage(string strErrorString)
{
	/* Workaround for the QT problem */
	string strError = "The following error occured:\n";
	strError += strErrorString.c_str();
	strError += "\n\nThe application will exit now.";

#ifdef _WIN32
	MessageBoxA(NULL, strError.c_str(), "Dream",
			   MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
#else
	perror(strError.c_str());
#endif

/*
// Does not work correctly. If it is called by a different thread, the
// application hangs! FIXME
	QMessageBox::critical(0, "Dream",
		QString("The following error occured:<br><b>") +
		QString(strErrorString.c_str()) +
		"</b><br><br>The application will exit now.");
*/
	exit(1);
}
#else /* USE_QT_GUI */
/******************************************************************************\
* No GUI                                                                       *
\******************************************************************************/

int
main(int argc, char **argv)
{
	try
	{
		CSettings Settings;
		Settings.Load(argc, argv);
		if (Settings.Get("command", "isreceiver", true))
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver;

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings(Settings);
			DRMReceiver.SetReceiverMode(DRM);
			DRMReceiver.Start();
			DRMReceiver.SaveSettings(Settings);
		}
		else
		{
			CDRMTransmitter DRMTransmitter;
			DRMTransmitter.LoadSettings(Settings);
			DRMTransmitter.Start();
			DRMTransmitter.SaveSettings(Settings);
		}
	}
	catch(CGenErr GenErr)
	{
		ErrorMessage(GenErr.strError);
	}

	return 0;
}

void
ErrorMessage(string strErrorString)
{
	perror(strErrorString.c_str());
}
#endif /* USE_QT_GUI */

void
DebugError(const char *pchErDescr, const char *pchPar1Descr,
		   const double dPar1, const char *pchPar2Descr, const double dPar2)
{
	FILE *pFile = fopen("test/DebugError.dat", "a");
	fprintf(pFile, pchErDescr);
	fprintf(pFile, " ### ");
	fprintf(pFile, pchPar1Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, pchPar2Descr);
	fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	printf("\nDebug error! For more information see test/DebugError.dat\n");
	exit(1);
}
