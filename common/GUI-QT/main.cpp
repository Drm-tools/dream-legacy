/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#include <qapplication.h>
#include <qthread.h>
#include <qmessagebox.h>
#include "fdrmdialog.h"

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../DrmTransmitter.h"
#include "../DrmSimulation.h"


/* The receiver, transmitter and simulation are global objects */
CDRMReceiver	DRMReceiver;
CDRMTransmitter	DRMTransmitter;
CDRMSimulation	DRMSimulation;

/* This pointer is only used for the post-event routine */
QApplication*	pApp;	


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


/* Implementation *************************************************************/
int main(int argc, char** argv)
{
	QApplication	app(argc, argv); // Application object
	ReceiverThread	RecThread; // Working thread object
	FDRMDialog		MainDlg(0, 0, TRUE, Qt::WStyle_Minimize); // Main dialog

// Activate this to read DRM data from file
//DRMReceiver.GetReceiver()->SetUseSoundcard(FALSE);

// Activate this to start the transmitter and generate a DRM stream
//DRMTransmitter.StartTransmitter();

	/* Set main window */
	app.setMainWidget(&MainDlg);
	pApp = &app;

try
{
	/* Call simulation script. If simulation is activated, application is 
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will immediately return */
	DRMSimulation.SimScript();
	
	/* First, initialize the working thread. This should be done in an extra
	   routine since we cannot 100% assume that the working thread is ealier
	   ready than the GUI thread */
	DRMReceiver.Init();

	/* Start thread */
	RecThread.start();

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
	DRMEvent* DRMEv = new DRMEvent(MessID, iMessageParam);

	/* Qt will delete the event object when done */
	QThread::postEvent(pApp->mainWidget(), DRMEv);
}

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

/* QT mutex wrapper */
void CMutex::Lock()
{
	Mutex.lock();
}

void CMutex::Unlock()
{
	Mutex.unlock();
}

_BOOLEAN CMutex::Locked()
{
	return Mutex.locked();
}
