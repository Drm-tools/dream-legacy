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

#include <qlabel.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qstring.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qwt_thermo.h>
#include <qevent.h>
#include <qcstring.h>
#include <qlayout.h>

#ifdef _WIN32
# include "../../Windows/moc/fdrmdialogbase.h"
# include "../../Windows/moc/AboutDlgbase.h"
#else
# include "moc/fdrmdialogbase.h"
# include "moc/AboutDlgbase.h"
#endif
#include "systemevalDlg.h"
#include "../DrmReceiver.h"
#include "../Vector.h"


extern CDRMReceiver	DRMReceiver;


/* Classes ********************************************************************/
class DRMEvent : public QCustomEvent
{
public:
	DRMEvent(int iNewMeTy, int iNewSt) : 
		QCustomEvent(QEvent::User + 11), iMessType(iNewMeTy), iStatus(iNewSt) {}

	int iMessType;
	int iStatus;
};


class FDRMDialog : public FDRMDialogBase
{
	Q_OBJECT

public:
	FDRMDialog(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		WFlags f = 0);

protected:
	systemevalDlg*	pSysEvalDlg;
    QMenuBar*		pMenu;
	QPopupMenu*		pSoundInMenu;
	QPopupMenu*		pSoundOutMenu;
	int				iCurSelServiceGUI;
	int				iOldNoServicesGUI;
	int				iNumSoundDev;
	QTimer			Timer;
	CAboutDlgBase	AboutDlg;

	QString			SetServParamStr(int iServiceID);
	QString			SetBitrIDStr(int iServiceID);
	virtual void	customEvent(QCustomEvent* Event);

public slots:
	void OnTimer();
	void OnButtonService1();
	void OnButtonService2();
	void OnButtonService3();
	void OnButtonService4();
	void OnViewEvalDlg();
	void OnHelpAbout();
	void OnSoundInDevice(int id);
	void OnSoundOutDevice(int id);
};

