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

#ifdef USE_QT_GUI
# include <qapplication.h>
# include <qthread.h>
# include <qmessagebox.h>
# include "fdrmdialog.h"
# include "TransmDlg.h"
# include "../GPSReceiver.h"
#endif
#include <iostream>

/* Implementation *************************************************************/
#ifdef USE_QT_GUI
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/

/* This pointer is only used for the post-event routine */
QApplication *pApp = NULL;

int
main(int argc, char **argv)
{
	CDRMSimulation DRMSimulation;

	/* Call simulation script. If simulation is activated, application is
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	try
	{
		/* Application object must be initialized before the DRMReceiver object
		 * because of the QT functions used in the MDI module. TODO: better solution
		 */
		QApplication app(argc, argv);

		/* Parse arguments and load settings from init-file */
		CSettings Settings;
		Settings.Load(argc, argv);

		/* Load and install multi-language support (if available) */
		QTranslator translator(0);
		if (translator.load("dreamtr"))
			app.installTranslator(&translator);

		string mode = Settings.Get("command", "mode", string("receive"));
		cout << "mode:" << mode << endl;
		if (mode == "receive")
		{
			CDRMReceiver DRMReceiver;
#ifdef _WIN32
			if (DRMReceiver.GetEnableProcessPriority())
			{
				/* Set priority class for this application */
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

				/* Low priority for GUI thread */
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
			}
#endif

			/* First, initialize the working thread. This should be done in an extra
			   routine since we cannot 100% assume that the working thread is
			   ready before the GUI thread */

			DRMReceiver.LoadSettings(Settings);

			if (Settings.Get("AM Dialog", "visible", false))
				DRMReceiver.SetReceiverMode(RM_AM);
			else
				DRMReceiver.SetReceiverMode(RM_DRM);

			DRMReceiver.Init();

			CGPSReceiver *pGPSReceiver = NULL;
			if (DRMReceiver.GetParameters()->ReceptLog.GPSData.
				GetGPSSource() != CGPSData::GPS_SOURCE_MANUAL_ENTRY)
			{
				pGPSReceiver =
					new CGPSReceiver(DRMReceiver.GetParameters()->ReceptLog.
									 GPSData);
			}

			FDRMDialog MainDlg(DRMReceiver, Settings, 0, 0, FALSE, Qt::WStyle_MinMax);

			/* Start working thread */
			DRMReceiver.start();

			/* Set main window */
			app.setMainWidget(&MainDlg);
			pApp = &app;		/* Needed for post-event routine */

			app.exec();

			/* GUI has exited, stop everything else */
			if (pGPSReceiver)
				delete pGPSReceiver;

			DRMReceiver.SaveSettings(Settings);
		}
		else if(mode == "transmit")
		{
			TransmDialog MainDlg(Settings, 0, 0, FALSE, Qt::WStyle_MinMax);

			/* Set main window */
			app.setMainWidget(&MainDlg);
			pApp = &app;		/* Needed for post-event routine */

			/* Show dialog */
			MainDlg.show();
			app.exec();
		}
		else
		{
# if QT_VERSION >= 0x030000
			QTextEdit help;
			help.setTextFormat( Qt::PlainText );
			help.setGeometry(200, 200, 480, 320);
			help.append(Settings.UsageArguments(argv).c_str());
			// Set main window */
			app.setMainWidget(&help);
			help.show();
			app.exec();
#else
			cout << Settings.UsageArguments(argv) << end;
#endif
			exit(0);
		}

		/* Save settings to init-file */
		Settings.Save();
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

	return 0;
}

/* Implementation of global functions *****************************************/
void
PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam)
{
	/* In case of simulation no events should be generated */
	if (pApp != NULL)
	{
		DRMEvent *DRMEv = new DRMEvent(MessID, iMessageParam);

		/* Qt will delete the event object when done */
		QThread::postEvent(pApp->mainWidget(), DRMEv);
	}
}

void
ErrorMessage(string strErrorString)
{
	/* Workaround for the QT problem */
	string strError = "The following error occured:\n";
	strError += strErrorString.c_str();
	strError += "\n\nThe application will exit now.";

#ifdef _WIN32
	MessageBox(NULL, strError.c_str(), "Dream",
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
		CSettings Settings(&DRMReceiver);
		Settings.Load(argc, argv);
		if (Settings.Get("command", "isreceiver", TRUE))
		{
			CDRMSimulation DRMSimulation;
			CDRMReceiver DRMReceiver;

			DRMSimulation.SimScript();
			DRMReceiver.LoadSettings(Settings);
			DRMReceiver.SetReceiverMode(RM_DRM);
			DRMReceiver.Start();
		}
		else
		{
			CDRMTransmitter DRMTransmitter;
			DRMTransmitter.Start();
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

void
PostWinMessage(const _MESSAGE_IDENT, const int)
{
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
