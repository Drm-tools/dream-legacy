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

#include <qlistview.h>
#include <qpixmap.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qtimer.h>
#include <qmessagebox.h>

#ifdef _WIN32
# include "../../Windows/moc/StationsDlgbase.h"
#else
# include "moc/StationsDlgbase.h"
#endif
#include "../DrmReceiver.h"
#include "../Vector.h"

extern CDRMReceiver	DRMReceiver;


/* Definitions ****************************************************************/
/* Define the timer interval of updating the list view */
#define GUI_TIMER_LIST_VIEW_STAT		1000 /* ms -> every second */


/* Classes ********************************************************************/
class CStationsItem
{
public:
	int GetStartTimeNum() {return iStartHour * 100 + iStartMinute;}
	int GetStopTimeNum() {return iStopHour * 100 + iStopMinute;}
	void SetStartTimeNum(const int iStartTime)
	{
		/* Recover hours and minutes */
		iStartHour = iStartTime / 100;
		iStartMinute = iStartTime - iStartHour * 100;
	}
	void SetStopTimeNum(const int iStopTime)
	{
		/* Recover hours and minutes */
		iStopHour = iStopTime / 100;
		iStopMinute = iStopTime - iStopHour * 100;
	}

	int		iStartHour;
	int		iStartMinute;
	int		iStopHour;
	int		iStopMinute;
	int		iFreq;
	int		iDays;
	string	strName;
	string	strTarget;
	string	strLanguage;
	string	strSite;
	string	strCountry;
	_REAL	rPower;
};

class CDRMSchedule
{
public:
	CDRMSchedule();
	virtual ~CDRMSchedule() {}

	void ReadStatTabFromFile(const string strFileName);
	void WriteStatTabToFile(const string strFileName);

	int GetStationNumber() {return StationsTable.Size();}
	CStationsItem& GetItem(int const iPos) {return StationsTable[iPos];}
	_BOOLEAN IsActive(int const iPos);

protected:
	CVector<CStationsItem> StationsTable;
};

class StationsDlg : public CStationsDlgBase
{
	Q_OBJECT

public:
	StationsDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		WFlags f = 0);
	virtual ~StationsDlg() {}

protected:
	void SetStationsView();

	CDRMSchedule	DRMSchedule;
	QPixmap			BitmCubeGreen;
	QPixmap			BitmCubeRed;
	QTimer			Timer;
	_BOOLEAN		bShowAll;
    virtual void	showEvent(QShowEvent* pEvent);

public slots:
	void OnTimer();
	void OnListItemClicked(QListViewItem* item);
	void OnRadioShowItems(int iID);
};
