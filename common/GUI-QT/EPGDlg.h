/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide Viewer
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

#ifndef _EPGDLG_H
#define _EPGDLG_H

#ifdef _WIN32
# include "../../Windows/moc/EPGDlgbase.h"
#else
# include "moc/EPGDlgbase.h"
#endif
#include <qwidget.h>
#include <qdatetime.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qtextbrowser.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qpixmap.h>

#include "../DrmReceiver.h"
#include "../datadecoding/epg/EPG.h"

/* Definitions ****************************************************************/
#define COL_NAME	1

/* Define the timer interval of updating */
#define GUI_TIMER_EPG_UPDATE		60000 /* ms (1 minute) */

/* list view columns */
#define COL_START		0
#define COL_NAME		1
#define	COL_GENRE		2
#define	COL_DESCRIPTION	3
#define COL_DURATION	4

/* Classes ********************************************************************/
class EPGDlg : public CEPGDlgbase
{
	Q_OBJECT
    
public:

	EPGDlg(CDRMReceiver* pNDRMR, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, WFlags f = 0);
		
	virtual ~EPGDlg();

    void setDate();
    void select();
    	
protected:
	
    virtual	void showEvent(QShowEvent *e);
	virtual void hideEvent(QHideEvent* pEvent);

	_BOOLEAN IsActive(const QString start, const QString duration, const time_t ltime);

	QPixmap			BitmCubeGreen;

    QDate date;
    bool do_updates;
    EPG epg;
	CDRMReceiver*	pDRMRec;
	QTimer			Timer;

public slots:
    void nextDay();
    void previousDay();
    void selectChannel(const QString&);
    void setDay(int);
    void setMonth(int);
    void setYear(int);
	void OnTimer();
};

#endif
