/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 *	Thanks to Tomi Manninen / OH2BNS / KP20ME04 for the command line argument
 *		parsing (argv)
 *
 * Description:
 *	
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

#include "main.h"

/* The receiver, transmitter and simulation are global objects */
CDRMReceiver	DRMReceiver; 
CDRMTransmitter	DRMTransmitter;
CDRMSimulation	DRMSimulation;


/* Implementation *************************************************************/
#ifdef USE_QT_GUI
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/

/* This pointer is only used for the post-event routine */
QApplication*	pApp = NULL;	

/* Thread class for the receiver */
class ReceiverThread : public QThread 
{
public:
	virtual void run() 
	{
		/* Set thread priority (The working thread should have a higher priority
		   than the GUI) */
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

		try
		{
			/* Call receiver main routine */
			DRMReceiver.Start();
		}

		catch (CGenErr GenErr)
		{
			ErrorMessage(GenErr.strError);
		}
	}
};

int main(int argc, char** argv)
{
try
{
	/* Call simulation script. If simulation is activated, application is 
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	QApplication	app(argc, argv); // Application object

	/* Parse arguments */
	ParseArguments(app);

	ReceiverThread	RecThread; // Working thread object
	FDRMDialog		MainDlg(0, 0, TRUE, Qt::WStyle_MinMax); // Main dialog

// Activate this to read DRM data from file
//DRMReceiver.GetReceiver()->SetUseSoundcard(FALSE);

// Activate this to start the transmitter and generate a DRM stream
//DRMTransmitter.StartTransmitter();

	/* First, initialize the working thread. This should be done in an extra
	   routine since we cannot 100% assume that the working thread is ealier
	   ready than the GUI thread */
	DRMReceiver.Init();

	/* Start thread */
	RecThread.start();

	/* Set main window */
	app.setMainWidget(&MainDlg);
	pApp = &app;

	/* Show dialog, working thread must be initialized before starting the 
	   GUI! */
	MainDlg.exec();

	/* Stop working thread and wait until it is ready for terminating. We set
	   a time-out of 5 seconds. If thread was not ready in that time, the
	   program will terminate anyway, but this can lead to an error 
	   message */
	DRMReceiver.Stop();

	RecThread.wait(5000);
}

catch (CGenErr GenErr)
{
	ErrorMessage(GenErr.strError);
}

	return 0;
}


/* Implementation of global functions *****************************************/
void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam)
{
	/* In case of simulation no events should be generated */
	if (pApp != NULL)
	{
		DRMEvent* DRMEv = new DRMEvent(MessID, iMessageParam);

		/* Qt will delete the event object when done */
		QThread::postEvent(pApp->mainWidget(), DRMEv);
	}
}

void ErrorMessage(string strErrorString)
{
	/* Workaround for the QT problem */
#ifdef _WIN32
	string strError = "The following error occured:\n";
	strError += strErrorString.c_str();
	strError += "\n\nThe application will exit now.";

	MessageBox(NULL, strError.c_str(), "Dream",
		MB_SYSTEMMODAL | MB_OK | MB_ICONEXCLAMATION);
#else
	perror(strErrorString.c_str());
#endif

/*
// Does not work correctly. If it is called by a different thread, the application
// hangs! FIXME
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
int main(int argc, char** argv)
{
	DRMSimulation.SimScript();
	DRMReceiver.Start();
	return 0;
}

void PostWinMessage(const _MESSAGE_IDENT MessID, const int iMessageParam) {}
#endif /* USE_QT_GUI */


void DebugError(const char* pchErDescr, const char* pchPar1Descr, 
				const double dPar1, const char* pchPar2Descr,
				const double dPar2)
{
	FILE* pFile = fopen("test/DebugError.dat", "a");
	fprintf(pFile, pchErDescr); fprintf(pFile, " ### ");
	fprintf(pFile, pchPar1Descr); fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, pchPar2Descr); fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	printf("\nDebug error, exit! For more information see DebugError.dat\n");
	exit(1);
}


/* Command line argument parser ***********************************************/
void ParseArguments(QApplication& app)
{
	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	for (int i = 1; i < app.argc(); i++)
	{
		/* Flip spectrum flag ----------------------------------------------- */
		if ((!strcmp(app.argv()[i], "--flipspectrum")) ||
			(!strcmp(app.argv()[i], "-p")))
		{
			DRMReceiver.GetReceiver()->SetFlippedSpectrum(TRUE);
			continue;
		}


		/* Mute audio flag -------------------------------------------------- */
		if ((!strcmp(app.argv()[i], "--muteaudio")) ||
			(!strcmp(app.argv()[i], "-m")))
		{
			DRMReceiver.GetWriteData()->MuteAudio(TRUE);
			continue;
		}


		/* Do not use sound card, read from file ---------------------------- */
		if ((!strcmp(app.argv()[i], "--fromfile")) ||
			(!strcmp(app.argv()[i], "-f")))
		{
			DRMReceiver.GetReceiver()->SetUseSoundcard(FALSE);
			continue;
		}


		/* Number of iterations for MLC setting ----------------------------- */
		if ((!strcmp(app.argv()[i], "--mlciter")) ||
			(!strcmp(app.argv()[i], "-i")))
		{
			if (++i >= app.argc())
			{
				cerr << app.argv()[0] << ": ";
				cerr << "'--mlciter' needs a numeric argument between 0 and 4." << endl;
				exit(1);
			}

			char *p;
			int n = strtol(app.argv()[i], &p, 10);
			if (*p || n < 0 || n > 4)
			{
				cerr << app.argv()[0] << ": ";
				cerr << "'--mlciter' needs a numeric argument between 0 and 4." << endl;
				exit(1);
			}

			DRMReceiver.GetMSCMLC()->SetNumIterations(n);
			continue;
		}


		/* Sample rate offset start value ----------------------------------- */
		if ((!strcmp(app.argv()[i], "--sampleoff")) ||
			(!strcmp(app.argv()[i], "-s")))
		{
			if (++i >= app.argc())
			{
				cerr << app.argv()[0] << ": ";
				cerr << "'--sampleoff' needs a numeric argument between -200.0 and 200.0." << endl;
				exit(1);
			}

			char *p;
			_REAL r = strtod(app.argv()[i], &p);
			if (*p || r < (_REAL) -200.0 || r > (_REAL) 200.0)
			{
				cerr << app.argv()[0] << ": ";
				cerr << "'--sampleoff' needs a numeric argument between -200.0 and 200.0." << endl;
				exit(1);
			}

			DRMReceiver.SetInitResOff(r);
			continue;
		}


		/* Help (usage) flag ------------------------------------------------ */
		if ((!strcmp(app.argv()[i], "--help")) ||
			(!strcmp(app.argv()[i], "-h")) ||
			(!strcmp(app.argv()[i], "-?")))
		{
			UsageArguments();
			exit(1);
		}


		/* Unknown option --------------------------------------------------- */
		cerr << app.argv()[0] << ": ";
		cerr << "Unknown option '" << app.argv()[i] << "' -- use '--help' for help" << endl;
		exit(1);
	}
}

void UsageArguments(void)
{
	cerr << "Usage: " << qApp->argv()[0] << " [option] [argument]" << endl;
	cerr << endl;
	cerr << "Recognized options:" << endl;
	cerr << endl;
	cerr << "  -p, --flipspectrum         flip input spectrum" << endl;
	cerr << "  -i <n>, --mlciter <n>      number of MLC iterations" << endl;
	cerr << "                             allowed range: 0...4" << endl;
	cerr << "                             default: 1" << endl;
	cerr << "  -s <r>, --sampleoff <r>    sample rate offset initial value [Hz]" << endl;
	cerr << "                             allowed range: -200.0...200.0" << endl;
	cerr << "  -m, --muteaudio            mute audio output" << endl;
	cerr << "  -f, --fromfile             disable sound card," << endl;
	cerr << "                             read from file instead" << endl;
	cerr << "  -h, -?, --help             this help text" << endl;
	cerr << endl;
	cerr << "Example: " << qApp->argv()[0] << " -p --sampleoff -0.23 -i 2" << endl;
	cerr << endl;
}
