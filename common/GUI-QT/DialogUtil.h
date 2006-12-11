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

#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qevent.h>
#include <qtextview.h>
#include <qlabel.h>
#include <qwhatsthis.h>

#ifdef _WIN32
# include "../../Windows/moc/AboutDlgbase.h"
#else
# include "moc/AboutDlgbase.h"
#endif
#include "../DrmReceiver.h"
#include "../util/Vector.h"

#ifdef USE_QT_GUI
# include <qwt_global.h> /* for extract the library version */
#endif

#ifdef HAVE_LIBFREEIMAGE
# include <FreeImage.h> /* for extract the library version */
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
		WFlags f = 0);
};


/* Help menu ---------------------------------------------------------------- */
class CDreamHelpMenu : public QPopupMenu
{
	Q_OBJECT

public:
	CDreamHelpMenu(QWidget* parent = 0);

protected:
	CAboutDlg AboutDlg;

public slots:
	void OnHelpWhatsThis() {QWhatsThis::enterWhatsThisMode();}
	void OnHelpAbout() {AboutDlg.exec();}
};


/* Sound card selection menu ------------------------------------------------ */
class CSoundCardSelMenu : public QPopupMenu
{
	Q_OBJECT

public:
	CSoundCardSelMenu(CSoundIn* pNSIn, CSoundOut* pNSOut, QWidget* parent = 0);

protected:
	CSoundIn*		pSoundInIF;
	CSoundOut*		pSoundOutIF;
	vector<string>	vecSoundInNames;
	vector<string>	vecSoundOutNames;
	int				iNumSoundInDev;
	int				iNumSoundOutDev;
	QPopupMenu*		pSoundInMenu;
	QPopupMenu*		pSoundOutMenu;

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
# if QT_VERSION == 230
	sTitle.fill(' ', 10000);
	sTitle += "Qt";
# endif
#endif

	pDlg->setCaption(sCap + sTitle);
}


#endif // DIALOGUTIL_H__FD6B23452398345OIJ9453_804E1606C2AC__INCLUDED_
