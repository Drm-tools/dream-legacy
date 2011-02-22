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

#include <qwidget.h>
#include <qdatetime.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qpixmap.h>
#include <map>
#if QT_VERSION < 0x040000
# include <qlistview.h>
# define Q3ListView QListView
# define Q3ListViewItem QListViewItem
#else
# include <q3listview.h>
#endif

#include "EPGDlgbase.h"
#include "../DrmReceiver.h"
#include "../datadecoding/epg/EPG.h"
#include "../util/Settings.h"

/* Definitions ****************************************************************/
#define COL_NAME	1

/* Define the timer interval of updating */
#define GUI_TIMER_EPG_UPDATE		1000 /* ms (1 second) */

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

	EPGDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);
	virtual ~EPGDlg();
	/* dummy assignment operator to help MSVC8 */
	EPGDlg& operator=(const EPGDlg&)
	{ throw "should not happen"; return *this;}

    void setDate();
    void select();

protected:

	virtual	void showEvent(QShowEvent *e);
	virtual void hideEvent(QHideEvent* pEvent);
	virtual void setActive(Q3ListViewItem*);

    class MyListViewItem : public Q3ListViewItem
    {
    	public:

    	MyListViewItem( Q3ListView * parent, QString a, QString b, QString c, QString d, QString e,
    	time_t s, int dr):Q3ListViewItem(parent,a,b,c,d,e),start(s),duration(dr){}


	_BOOLEAN IsActive();

	time_t start;
	int duration;
    };

	QPixmap			BitmCubeGreen;

    QDate date;
    bool do_updates;
    EPG epg;
	CDRMReceiver&	DRMReceiver;
	CSettings&		Settings;
	QTimer			Timer;
	map<QString,uint32_t> sids;
	Q3ListViewItem*		next;

signals:
	void NowNext(QString);

public slots:
    void nextDay();
    void previousDay();
    void selectChannel(const QString&);
    void setDay(int);
    void setMonth(int);
    void setYear(int);
	void OnTimer();
	void sendNowNext(QString);
};

#endif
