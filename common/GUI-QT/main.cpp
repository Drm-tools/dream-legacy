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

		/* Call receiver main routine */
		DRMReceiver.Start();
	}
};


/* Implementation *************************************************************/
int main(int argc, char** argv)
{
// Activate this to read DRM data from file
//DRMReceiver.GetReceiver()->SetUseSoundcard(FALSE);


	/* Call simulation script. If simulation is activated, application is 
	   automatically exit in that routine. If in the script no simulation is
	   activated, this function will imedeately return */
	DRMSimulation.SimScript();

	QApplication	app(argc, argv); // Application object
	ReceiverThread	RecThread; // Working thread object
	FDRMDialog		MainDlg(0, 0, TRUE, Qt::WStyle_Minimize); // Main dialog

	/* Set main window */
	app.setMainWidget(&MainDlg);
	pApp = &app;
	
	/* First, initialize the working thread */
	DRMReceiver.Init();

	/* Start thread */
	RecThread.start();

	/* Show dialog, working thread must be initialized before starting the 
	   GUI! */
	MainDlg.exec();

	/* Stop working thread and wait until it is ready for terminating. We set
	   a time-out of 5 seconds. If thread was not ready in that time, the
	   program will terminate anyway */
	DRMReceiver.Stop();
	return RecThread.wait(5000);
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

void Data2File(const _REAL rInput)
{
	static FILE* pFile = fopen("test/test.dat", "w");
	fprintf(pFile, "%e\n", rInput);
	fflush(pFile);
}
