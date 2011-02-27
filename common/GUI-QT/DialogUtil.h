/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
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

#if !defined(DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_)
#define DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_

#include "AboutDlgbase.h"
#include "../Parameter.h"
#include "../selectioninterface.h"
#ifdef HAVE_LIBHAMLIB
# include "../util/Utilities.h"
#endif

#include<map>

#if QT_VERSION < 0x040000
# include <qtextview.h>
# include <qpopupmenu.h>
# define Q3PopupMenu QPopupMenu
#else
# include <q3action.h>
# include <q3popupmenu.h>
# include <q3textview.h>
# include <q3whatsthis.h>
# include <QCustomEvent>
#endif
#include <qmenubar.h>
#include <qevent.h>
#include <qlabel.h>
#include <qthread.h>


#ifndef HAVE_LIBHAMLIB
typedef int rig_model_t;
#endif

/* Definitions ****************************************************************/

/* Definition for Courier font */
#ifdef _WIN32
	#define FONT_COURIER    "Courier New"
#else
	#define FONT_COURIER    "Courier"
#endif
/* Classes ********************************************************************/
/* DRM events --------------------------------------------------------------- */
class DRMEvent : public QCustomEvent
{
public:
	DRMEvent(const int iNewMeTy, const int iNewSt) :
		QCustomEvent(QEvent::User + 11), iMessType(iNewMeTy), iStatus(iNewSt) {}

	int iMessType;
	int iStatus;
};


/* About dialog ------------------------------------------------------------- */
class CAboutDlg : public CAboutDlgBase
{
	Q_OBJECT

public:
	CAboutDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		Qt::WFlags f = 0);
};


/* Help menu ---------------------------------------------------------------- */
class CDreamHelpMenu : public Q3PopupMenu
{
	Q_OBJECT

public:
	CDreamHelpMenu(QWidget* parent = 0);

protected:
	CAboutDlg AboutDlg;

public slots:
	void OnHelpWhatsThis();
	void OnHelpAbout() {AboutDlg.exec();}
};


/* Sound card selection menu ------------------------------------------------ */
class CSoundCardSelMenu : public Q3PopupMenu
{
	Q_OBJECT

public:
	CSoundCardSelMenu(CSelectionInterface* pNSIn,
						CSelectionInterface* pNSOut, QWidget* parent = 0);

protected:
	CSelectionInterface*	pSoundInIF;
	CSelectionInterface*	pSoundOutIF;
	vector<string>			vecSoundInNames;
	vector<string>			vecSoundOutNames;
	int						iNumSoundInDev;
	int						iNumSoundOutDev;
	Q3PopupMenu*				pSoundInMenu;
	Q3PopupMenu*				pSoundOutMenu;

public slots:
	void OnSoundInDevice(int id);
	void OnSoundOutDevice(int id);
};


/* GUI help functions ------------------------------------------------------- */
/* Converts from RGB to integer and back */
class CRGBConversion
{
public:
	static int RGB2int(const QColor newColor)
	{
		/* R, G and B are encoded as 8-bit numbers */
		int iReturn = newColor.red();
		iReturn <<= 8;
		iReturn |= newColor.green();
		iReturn <<= 8;
		iReturn |= newColor.blue();
		return iReturn;
	}

	static QColor int2RGB(const int iValue)
	{
		return QColor((iValue >> 16) & 255, (iValue >> 8) & 255, iValue & 255);
	}
};


inline void SetDialogCaption(QDialog* pDlg, const QString sCap)
{
	/* Under Windows it does seem that QT only sets the caption if a "Qt" is
	   present in the name. Make a little "trick" to display our desired
	   name without seeing the "Qt" (by Andrea Russo) */
	QString sTitle = "";

#ifdef _MSC_VER
# if QT_VERSION < 0x030000
	sTitle.fill(' ', 10000);
	sTitle += "Qt";
# endif
#endif

	pDlg->setCaption(sCap + sTitle);
}


class QAction;

class CRig :
#if QT_VERSION < 0x040000
	public QObject,
#endif
	public QThread
{
	Q_OBJECT
public:
	CRig(CParameter* np):
#ifdef HAVE_LIBHAMLIB
	Hamlib(),
#endif
	subscribers(0),pParameters(np)
	{ }
	void run();
	void subscribe();
	void unsubscribe();
#ifdef HAVE_LIBHAMLIB
	void GetRigList(map<rig_model_t,CHamlib::SDrRigCaps>& r) { Hamlib.GetRigList(r); }
	rig_model_t GetHamlibModelID() { return Hamlib.GetHamlibModelID(); }
	void SetHamlibModelID(rig_model_t r) { Hamlib.SetHamlibModelID(r); }
	void SetEnableModRigSettings(_BOOLEAN b) { Hamlib.SetEnableModRigSettings(b); }
	void GetPortList(map<string,string>& ports) { Hamlib.GetPortList(ports); }
	string GetComPort() { return Hamlib.GetComPort(); }
	void SetComPort(const string& s) { Hamlib.SetComPort(s); }
	_BOOLEAN GetEnableModRigSettings() { return Hamlib.GetEnableModRigSettings(); }
	CHamlib::ESMeterState GetSMeter(_REAL& r) { return Hamlib.GetSMeter(r); }
	void LoadSettings(CSettings& s) { Hamlib.LoadSettings(s);}
	void SaveSettings(CSettings& s) { Hamlib.SaveSettings(s); }
	CHamlib* GetRig() { return &Hamlib; }

protected:
	CHamlib Hamlib;
#endif
protected:
	int subscribers;
	CParameter* pParameters;

signals:
    void sigstr(double);
};

class RemoteMenu : public QObject
{
	Q_OBJECT

public:
	RemoteMenu(QWidget*, CRig&);
	Q3PopupMenu* menu(){ return pRemoteMenu; }

public slots:
	void OnModRigMenu(int iID);
	void OnRemoteMenu(int iID);
#if QT_VERSION < 0x040000
	void OnComPortMenu(QAction* action);
#else
	void OnComPortMenu(Q3Action* action);
#endif

signals:
	void SMeterAvailable();

protected:
#ifdef HAVE_LIBHAMLIB
	struct Rigmenu {std::string mfr; Q3PopupMenu* pMenu;};
	std::map<int,Rigmenu> rigmenus;
	std::vector<rig_model_t> specials;
	CRig&	rig;
#endif
	Q3PopupMenu* pRemoteMenu;
	Q3PopupMenu* pRemoteMenuOther;
};

#define OTHER_MENU_ID (666)
#define SMETER_MENU_ID (667)

#endif // DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_
