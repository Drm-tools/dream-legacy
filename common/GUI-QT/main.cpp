/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Tomi Manninen
 *
 * Description:
 *
 * 10/03/2003 Tomi Manninen / OH2BNS
 *  - Initial (basic) code for command line argument parsing (argv)
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


/* Implementation *************************************************************/
#ifdef USE_QT_GUI
/******************************************************************************\
* Using GUI with QT                                                            *
\******************************************************************************/
/* The receiver, transmitter and simulation are global objects */
CDRMReceiver	DRMReceiver; 
CDRMSimulation	DRMSimulation;

/* This pointer is only used for the post-event routine */
QApplication*	pApp = NULL;


/* Thread class for the receiver */
class CReceiverThread : public QThread 
{
public:
	void Stop()
	{
		/* Stop working thread and wait until it is ready for terminating. We
		   set a time-out of 5 seconds */
		DRMReceiver.Stop();

		if (wait(5000) == FALSE)
			ErrorMessage("Termination of sound interface thread failed.");
	}

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
	/* Parse arguments */
	_BOOLEAN bIsReceiver = ParseArguments(argc, argv);

	/* Call simulation script. If simulation is activated, application is 
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();

	QApplication app(argc, argv); /* Application object */

	if (bIsReceiver == FALSE)
	{
		TransmDialog MainDlg(0, 0, TRUE, Qt::WStyle_MinMax);

		/* Set main window */
		app.setMainWidget(&MainDlg);
		pApp = &app; /* Needed for post-event routine */

		/* Show dialog */
		MainDlg.exec();
	}
	else
	{
		CReceiverThread	RecThread; /* Working thread object */
		FDRMDialog		MainDlg(0, 0, TRUE, Qt::WStyle_MinMax);

		/* First, initialize the working thread. This should be done in an extra
		   routine since we cannot 100% assume that the working thread is ealier
		   ready than the GUI thread */
		DRMReceiver.Init();

		/* Start thread */
		RecThread.start();

		/* Set main window */
		app.setMainWidget(&MainDlg);
		pApp = &app; /* Needed for post-event routine */

		/* Show dialog, working thread must be initialized before starting the 
		   GUI! */
		MainDlg.exec();

		RecThread.Stop();
	}
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
CDRMReceiver DRMReceiver; /* Must be a global object */

int main(int argc, char** argv)
{
	CDRMTransmitter	DRMTransmitter;
	CDRMSimulation	DRMSimulation;

	_BOOLEAN bIsReceiver = ParseArguments(argc, argv);
	DRMSimulation.SimScript();

	if (bIsReceiver == TRUE)
		DRMReceiver.Start();
	else
		DRMTransmitter.Start();

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
_BOOLEAN ParseArguments(int argc, char** argv)
{
	_BOOLEAN bIsReceiver = TRUE;

	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	for (int i = 1; i < argc; i++)
	{
		/* DRM transmitter mode flag ---------------------------------------- */
		if ((!strcmp(argv[i], "--transmitter")) ||
			(!strcmp(argv[i], "-t")))
		{
			bIsReceiver = FALSE;
			continue;
		}

		
		/* Flip spectrum flag ----------------------------------------------- */
		if ((!strcmp(argv[i], "--flipspectrum")) ||
			(!strcmp(argv[i], "-p")))
		{
			DRMReceiver.GetReceiver()->SetFlippedSpectrum(TRUE);
			continue;
		}


		/* Mute audio flag -------------------------------------------------- */
		if ((!strcmp(argv[i], "--muteaudio")) ||
			(!strcmp(argv[i], "-m")))
		{
			DRMReceiver.GetWriteData()->MuteAudio(TRUE);
			continue;
		}


		/* Do not use sound card, read from file ---------------------------- */
		if ((!strcmp(argv[i], "--fromfile")) ||
			(!strcmp(argv[i], "-f")))
		{
			DRMReceiver.GetReceiver()->SetUseSoundcard(FALSE);
			continue;
		}


		/* Number of iterations for MLC setting ----------------------------- */
		if ((!strcmp(argv[i], "--mlciter")) ||
			(!strcmp(argv[i], "-i")))
		{
			if (++i >= argc)
			{
				cerr << argv[0] << ": ";
				cerr << "'--mlciter' needs a numeric argument between 0 and 4."
					<< endl;
				exit(1);
			}

			char *p;
			int n = strtol(argv[i], &p, 10);
			if (*p || n < 0 || n > 4)
			{
				cerr << argv[0] << ": ";
				cerr << "'--mlciter' needs a numeric argument between 0 and 4."
					<< endl;
				exit(1);
			}

			DRMReceiver.GetMSCMLC()->SetNumIterations(n);
			continue;
		}


		/* Start log file flag ---------------------------------------------- */
		if ((!strcmp(argv[i], "--startlog")) ||
			(!strcmp(argv[i], "-l")))
		{
			DRMReceiver.GetParameters()->ReceptLog.SetDelLogStart();
			continue;
		}


		/* Frequency for log file ------------------------------------------- */
		if ((!strcmp(argv[i], "--frequency")) ||
			(!strcmp(argv[i], "-r")))
		{
			if (++i >= argc)
			{
				cerr << argv[0] << ": ";
				cerr << "'--frequency' needs a numeric argument."
					<< endl;
				exit(1);
			}

			char *p;
			int n = strtol(argv[i], &p, 10);
			if (*p || n < 0)
			{
				cerr << argv[0] << ": ";
				cerr << "'--frequency' needs a numeric argument."
					<< endl;
				exit(1);
			}

			DRMReceiver.GetParameters()->ReceptLog.SetFrequency(n);
			continue;
		}


		/* Latitude string for log file ------------------------------------- */
		if ((!strcmp(argv[i], "--latitude")) ||
			(!strcmp(argv[i], "-a")))
		{
			if (++i >= argc)
			{
				cerr << argv[0] << ": ";
				cerr << "'--latitude' needs a string argument."
					<< endl;
				exit(1);
			}

			DRMReceiver.GetParameters()->ReceptLog.SetLatitude(argv[i]);
			continue;
		}


		/* Longitude string for log file ------------------------------------ */
		if ((!strcmp(argv[i], "--longitude")) ||
			(!strcmp(argv[i], "-o")))
		{
			if (++i >= argc)
			{
				cerr << argv[0] << ": ";
				cerr << "'--longitude' needs a string argument."
					<< endl;
				exit(1);
			}

			DRMReceiver.GetParameters()->ReceptLog.SetLongitude(argv[i]);
			continue;
		}


		/* Sample rate offset start value ----------------------------------- */
		if ((!strcmp(argv[i], "--sampleoff")) ||
			(!strcmp(argv[i], "-s")))
		{
			if (++i >= argc)
			{
				cerr << argv[0] << ": ";
				cerr << "'--sampleoff' needs a numeric argument between -200.0"
					" and 200.0." << endl;
				exit(1);
			}

			char *p;
			_REAL r = strtod(argv[i], &p);
			if (*p || r < (_REAL) -200.0 || r > (_REAL) 200.0)
			{
				cerr << argv[0] << ": ";
				cerr << "'--sampleoff' needs a numeric argument between -200.0"
					" and 200.0." << endl;
				exit(1);
			}

			DRMReceiver.SetInitResOff(r);
			continue;
		}


		/* Help (usage) flag ------------------------------------------------ */
		if ((!strcmp(argv[i], "--help")) ||
			(!strcmp(argv[i], "-h")) ||
			(!strcmp(argv[i], "-?")))
		{
			UsageArguments(argv);
			exit(1);
		}


		/* Unknown option --------------------------------------------------- */
		cerr << argv[0] << ": ";
		cerr << "Unknown option '" << argv[i] << "' -- use '--help' for help"
			<< endl;

		exit(1);
	}

	return bIsReceiver;
}

void UsageArguments(char** argv)
{
	cerr << "Usage: " << argv[0] << " [option] [argument]" << endl;
	cerr << endl;
	cerr << "Recognized options:" << endl;
	cerr << endl;
	cerr << "  -t, --transmitter          DRM transmitter mode" << endl;
	cerr << "  -p, --flipspectrum         flip input spectrum" << endl;
	cerr << "  -i <n>, --mlciter <n>      number of MLC iterations" << endl;
	cerr << "                             allowed range: 0...4" << endl;
	cerr << "                             default: 1" << endl;
	cerr << "  -s <r>, --sampleoff <r>    sample rate offset initial value [Hz]"
		<< endl;
	cerr << "                             allowed range: -200.0...200.0"
		<< endl;
	cerr << "  -m, --muteaudio            mute audio output" << endl;
	cerr << "  -f, --fromfile             disable sound card," << endl;
	cerr << "                             read from file instead" << endl;
	cerr << "  -r <n>, --frequency <n>    set frequency [kHz] for log file"
		<< endl;
	cerr << "  -a <s>, --latitude <s>     set latitude string for log file"
		<< endl;
	cerr << "  -o <s>, --longitude <s>    set longitude string for log file"
		<< endl;
	cerr << "  -l, --startlog             start log file (delayed)" << endl;
	cerr << endl;
	cerr << "  -h, -?, --help             this help text" << endl;
	cerr << endl;
	cerr << "Example: " << argv[0] <<
		" -p --sampleoff -0.23 -i 2 -r 6140 -a 50°13\\'N -o 8°34\\'E" << endl;
	cerr << endl;
}
