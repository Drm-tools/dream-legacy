/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2005
 *
 * Author(s):
 *	Andrea Russo
 *
 * Description:
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

#include "LiveScheduleDlg.h"

/* Implementation *************************************************************/

string CDRMLiveSchedule::Binary2String(const int iVal)
{
int p;
string s = "";
int iTempVal = iVal;

while (iTempVal != 0)
{
	p = iTempVal % 2;

	if (p == 1)
		s = s + "1";
	else
		s = s + "0";
	
	iTempVal = iTempVal / 2;
}

return s;
}

QString LiveScheduleDlg::ExtractTime(const int iTime)
{
string sHours = "";
string sMinutes = "";
string sDays = "";
string sResult = "";

	int iMinutes = iTime % 60;
	int iHours = iTime / 60;
		
	if (iMinutes < 10)
		sMinutes = "0";

	if (iHours>24)
	{
		int iDays = iHours / 24;
		
		if (iDays > 1)
		{
			sDays += " (";
			sDays += QString::number(iDays).latin1(); 
			sDays += " days)";
		}
		
		iHours = iHours % 24;
	}
	
	if (iHours < 10)
		sHours = "0";

	sHours += QString::number(iHours).latin1();
	sMinutes += QString::number(iMinutes).latin1();

	sResult = sHours + ":" + sMinutes + sDays;
	return QString(sResult.c_str());
}

QString LiveScheduleDlg::ExtractDaysFlagString(const string strDaysFlags)
{
string strDaysShow = "";

	/* Init days string vector */
	const QString strDayDef [] =
	{
		QObject::tr("Mon"),
		QObject::tr("Tue"),
		QObject::tr("Wed"),
		QObject::tr("Thu"),
		QObject::tr("Fri"),
		QObject::tr("Sat"),
		QObject::tr("Sun")
	};

	/* First test for day constellations which allow to apply special names */
	/* 1111111 = Mon Tue Wed Thu Fri Sat Sun = 1234567 (1 = Monday, 7 = Sunday) */
	if (strDaysFlags == "1111111")
		strDaysShow = QObject::tr("daily").latin1();
	else if (strDaysFlags == "1111100")
		strDaysShow = QObject::tr("from Mon to Fri").latin1();
	else if (strDaysFlags == "1111110")
		strDaysShow = QObject::tr("from Mon to Sat").latin1();
	else
	{
		/* No special name could be applied, just list all active days */
		for (int i = 0; i < 7; i++)
		{
			/* Check if day is active */
			if (strDaysFlags[i] == CHR_ACTIVE_DAY_MARKER)
			{
				/* Set commas in between the days, to not set a comma at
				   the beginning */
				if (strDaysShow != "") 
					strDaysShow += ",";

				/* Add current day */
				strDaysShow += strDayDef[i].latin1();
			}
		}
	}
return QString(strDaysShow.c_str());
}

void CDRMLiveSchedule::LoadAFSInformations(CParameter::CAltFreqSign AltFreqSign)
{
int k;
_BOOLEAN bFound;
QString strRegions = "";

/* Init table for stations */
StationsTable.Init(0);

const int iSize = AltFreqSign.vecAltFreq.Size();

for (int z = 0; z < iSize; z++)
{
	/* TODO multiplex and restrictions */
	//AltFreqSign.vecAltFreq[z].bIsSyncMultplx);
	
	//for ( k = 0; k < 4; k++)
	//	AltFreqSign.vecAltFreq[z].veciServRestrict[k]);
	
	if (AltFreqSign.vecAltFreq[z].bRegionSchedFlag == TRUE)
	{
		k = 0;
		strRegions = "";
	
		while (k < AltFreqSign.vecAltFreqRegions.Size())
		{
			if (AltFreqSign.vecAltFreqRegions[k].iRegionID == AltFreqSign.vecAltFreq[z].iRegionID)
			{
				/* Targets */				
				int iCIRAF = 0;
		
				for (int kk = 0; kk < AltFreqSign.vecAltFreqRegions[k].veciCIRAFZones.Size(); kk++)
				{
					iCIRAF = AltFreqSign.vecAltFreqRegions[k].veciCIRAFZones[kk];

					if (strRegions != "")
						strRegions += ", ";
	
					strRegions += QString(strTableCIRAFzones[iCIRAF].c_str());
				}
			}
			k++;
		}

		k = 0;
		bFound = FALSE;

		while (k < AltFreqSign.vecAltFreqSchedules.Size())
		{
			/* Schedules */
			if (AltFreqSign.vecAltFreqSchedules[k].iScheduleID == AltFreqSign.vecAltFreq[z].iScheduleID)
			{
				bFound = TRUE;
			
				/* For all frequencies */
				for ( int j = 0; j < AltFreqSign.vecAltFreq[z].veciFrequencies.Size(); j++)
				{	
					CLiveScheduleItem LiveScheduleItem;
	
					/* Frequency */
					LiveScheduleItem.iFreq = AltFreqSign.vecAltFreq[z].veciFrequencies[j];

					/* Set start time and duration */
					LiveScheduleItem.iStartTime = AltFreqSign.vecAltFreqSchedules[k].iStartTime;
					LiveScheduleItem.iDuration = AltFreqSign.vecAltFreqSchedules[k].iDuration;

					/* Days flags */
					LiveScheduleItem.strDaysFlags = Binary2String(AltFreqSign.vecAltFreqSchedules[k].iDayCode);

					/* Add the target */
					LiveScheduleItem.strTarget = strRegions.latin1();

					/* Add new item in table */
					StationsTable.Add(LiveScheduleItem);
				}
			}
			k++;
		}

		/* If not schedule found then add frequency and regions */
		if (bFound == FALSE)
		{
			/* For all frequencies */
			for (int j = 0; j < AltFreqSign.vecAltFreq[z].veciFrequencies.Size(); j++)
			{	
				CLiveScheduleItem LiveScheduleItem;

				/* Frequency */
				LiveScheduleItem.iFreq = AltFreqSign.vecAltFreq[z].veciFrequencies[j];

				/* Add the target */
				LiveScheduleItem.strTarget = strRegions.latin1();

				/* Add new item in table */
				StationsTable.Add(LiveScheduleItem);
			}
		}
	}
	else
	{
		/* For all frequencies */
		for ( int j = 0; j < AltFreqSign.vecAltFreq[z].veciFrequencies.Size(); j++)
		{	
			CLiveScheduleItem LiveScheduleItem;
	
			/* Frequency */
			LiveScheduleItem.iFreq = AltFreqSign.vecAltFreq[z].veciFrequencies[j];

			/* Add new item in table */
			StationsTable.Add(LiveScheduleItem);
		}
	}
}
}

LiveScheduleDlg::LiveScheduleDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) : vecpListItems(0),
	CLiveScheduleDlgBase(parent, name, modal, f), pDRMRec(pNDRMR)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomLiveScheduleDlg.iXPos,
		pDRMRec->GeomLiveScheduleDlg.iYPos,
		pDRMRec->GeomLiveScheduleDlg.iWSize,
		pDRMRec->GeomLiveScheduleDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#else /* Under Linux only restore the size */
	resize(pDRMRec->GeomLiveScheduleDlg.iWSize,
		pDRMRec->GeomLiveScheduleDlg.iHSize);
#endif

	/* Set sorting behaviour of the list */
	ListViewStations->setSorting(pDRMRec->SortParamLiveSched.iColumn,
		pDRMRec->SortParamLiveSched.bAscending);

	iCurrentSortColumn = pDRMRec->SortParamLiveSched.iColumn;
	bCurrentSortAscending = pDRMRec->SortParamLiveSched.bAscending;

	/* Define size of the bitmaps */
	const int iXSize = 13;
	const int iYSize = 13;

	/* Create bitmaps */
	BitmCubeGreen.resize(iXSize, iYSize);
	BitmCubeGreen.fill(QColor(0, 255, 0));
	BitmCubeYellow.resize(iXSize, iYSize);
	BitmCubeYellow.fill(QColor(255, 255, 0));
	BitmCubeRed.resize(iXSize, iYSize);
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeOrange.resize(iXSize, iYSize);
	BitmCubeOrange.fill(QColor(255, 128, 0));

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(COL_FREQ, tr("Frequency [kHz]"));
	ListViewStations->addColumn(tr("Time [UTC]"));
	ListViewStations->addColumn(tr("Target"));
	ListViewStations->addColumn(tr("Days"));

	/* Set right alignment for numeric columns */
	ListViewStations->setColumnAlignment(COL_FREQ, Qt::AlignRight);

	/* this for add spaces into the column and show all the header caption */
	vecpListItems.Init(1);
	vecpListItems[0] = new MyListLiveViewItem(ListViewStations,
		"0000000000000");

	/* Init UTC time shown with a label control */
	SetUTCTimeLabel();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	pViewMenu = new QPopupMenu(this);
	CHECK_PTR(pViewMenu);
	pViewMenu->insertItem(tr("Show &only active stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem(tr("Show &all stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Retrieve the setting saved into the .ini file */
	SetCurrentSavePath(pDRMRec->strStoragePathLiveScheduleDlg.c_str());

	/* Set stations in list view which are active right now */
	bShowAll = 	pDRMRec->bShowAllStations;

	if (bShowAll)
		pViewMenu->setItemChecked(1, TRUE);
	else
		pViewMenu->setItemChecked(0, TRUE);

	/* Stations Preview menu ------------------------------------------------ */
	pPreviewMenu = new QPopupMenu(this);
	CHECK_PTR(pPreviewMenu);
	pPreviewMenu->insertItem(tr("&Disabled"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 0);
	pPreviewMenu->insertItem(tr("&5 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 1);
	pPreviewMenu->insertItem(tr("&15 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 2);
	pPreviewMenu->insertItem(tr("&30 minutes"), this,
		SLOT(OnShowPreviewMenu(int)), 0, 3);

	/* Set stations preview */
	/* Retrive the setting saved into the .ini file */
	DRMSchedule.SetSecondsPreview(pDRMRec->iSecondsPreviewLiveSched);
	switch (DRMSchedule.GetSecondsPreview())
	{
	case NUM_SECONDS_PREV_5MIN:
		pPreviewMenu->setItemChecked(1, TRUE);
		break;

	case NUM_SECONDS_PREV_15MIN:
		pPreviewMenu->setItemChecked(2, TRUE);
		break;

	case NUM_SECONDS_PREV_30MIN:
		pPreviewMenu->setItemChecked(3, TRUE);
		break;

	default: /* case 0: */
		pPreviewMenu->setItemChecked(0, TRUE);
		break;
	}

	pViewMenu->insertSeparator();
	pViewMenu->insertItem(tr("Stations &preview"),pPreviewMenu);

	SetStationsView();

	/* File menu ------------------------------------------------------------ */
	pFileMenu = new QPopupMenu(this);
	CHECK_PTR(pFileMenu);
	pFileMenu->insertItem(tr("&Save..."), this, SLOT(OnSave()), CTRL+Key_S, 0);

	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&File"), pFileMenu);
	pMenu->insertItem(tr("&View"), pViewMenu);

	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* disable save menu */
	pFileMenu->setItemEnabled(0, FALSE);

	/* Now tell the layout about the menu */
	CLiveScheduleDlgBaseLayout->setMenuBar(pMenu);

	/* Connections ---------------------------------------------------------- */
	connect(&TimerList, SIGNAL(timeout()),
		this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()),
		this, SLOT(OnTimerUTCLabel()));

	connect(ListViewStations->header(), SIGNAL(clicked(int)),
		this, SLOT(OnHeaderClicked(int)));
}

LiveScheduleDlg::~LiveScheduleDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomLiveScheduleDlg.iXPos = WinGeom.x();
	pDRMRec->GeomLiveScheduleDlg.iYPos = WinGeom.y();
	pDRMRec->GeomLiveScheduleDlg.iHSize = WinGeom.height();
	pDRMRec->GeomLiveScheduleDlg.iWSize = WinGeom.width();

	/* Store preview settings */
	pDRMRec->iSecondsPreviewLiveSched = DRMSchedule.GetSecondsPreview();

	/* Store sort settings */
	pDRMRec->SortParamLiveSched.iColumn = iCurrentSortColumn;
	pDRMRec->SortParamLiveSched.bAscending = bCurrentSortAscending;

	/* Store preview settings */
	pDRMRec->bShowAllStations = bShowAll;

	/* Store save path */
	pDRMRec->strStoragePathLiveScheduleDlg = strCurrentSavePath.latin1();
}

void LiveScheduleDlg::SetUTCTimeLabel()
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	/* Generate time in format "UTC 12:00" */
	QString strUTCTime = QString().sprintf("%02d:%02d UTC",
		gmtCur->tm_hour, gmtCur->tm_min);

	/* Only apply if time label does not show the correct time */
	if (TextLabelUTCTime->text().compare(strUTCTime))
		TextLabelUTCTime->setText(strUTCTime);
}

void LiveScheduleDlg::OnShowStationsMenu(int iID)
{
	/* Show only active stations if ID is 0, else show all */
	if (iID == 0)
		bShowAll = FALSE;
	else
		bShowAll = TRUE;

	/* Update list view */
	SetStationsView();

	/* Taking care of checks in the menu */
	pViewMenu->setItemChecked(0, 0 == iID);
	pViewMenu->setItemChecked(1, 1 == iID);
}

void LiveScheduleDlg::OnShowPreviewMenu(int iID)
{
	switch (iID)
	{
	case 1:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
		break;

	case 2:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case 3:
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0: */
		DRMSchedule.SetSecondsPreview(0);
		break;
	}

	/* Update list view */
	SetStationsView();

	/* Taking care of checks in the menu */
	pPreviewMenu->setItemChecked(0, 0 == iID);
	pPreviewMenu->setItemChecked(1, 1 == iID);
	pPreviewMenu->setItemChecked(2, 2 == iID);
	pPreviewMenu->setItemChecked(3, 3 == iID);
}

void LiveScheduleDlg::OnTimerList()
{
	/* Update schedule and list view */
		LoadSchedule();
}

QString MyListLiveViewItem::key(int column, bool ascending) const
{
	/* Reimplement "key()" function to get correct sorting behaviour */
	if (column == COL_FREQ)
	{
		/* These columns are filled with numbers. Some items may have numbers
		   after the comma, therefore multiply with 10000 (which moves the
		   numbers in front of the comma). Afterwards append zeros at the
		   beginning so that positive integer numbers are sorted correctly */
		return QString(QString().setNum((long int)
			(text(column).toFloat() * 10000.0))).rightJustify(20, '0');
	}
    else
		return QListViewItem::key(column, ascending);
}

void LiveScheduleDlg::LoadSchedule()
{
	/* Lock mutex for modifying the vecpListItems */
	ListItemsMutex.Lock();

	/* Delete all old list view items (it is important that the vector
	   "vecpListItems" was initialized to 0 at creation of the global object
	   otherwise this may cause an segmentation fault) */
	for (int i = 0; i < vecpListItems.Size(); i++)
	{
		if (vecpListItems[i] != NULL)
			delete vecpListItems[i];
	}

	DRMSchedule.LoadAFSInformations(pDRMRec->GetParameters()->AltFreqSign);

	/* Init vector for storing the pointer to the list view items */
	const int intNumStations = DRMSchedule.GetStationNumber();

	vecpListItems.Init(intNumStations, NULL);

	/* Enable disable save menu item */
	if (intNumStations > 0)
		pFileMenu->setItemEnabled(0, TRUE);
	else
		pFileMenu->setItemEnabled(0, FALSE);

	/* Unlock BEFORE calling the stations view update because in this function
	   the mutex is locked, too! */
	ListItemsMutex.Unlock();

	/* Update list view */
	SetStationsView();
}

void LiveScheduleDlg::showEvent(QShowEvent* pEvent)
{
	/* Update window */
	OnTimerUTCLabel();
	OnTimerList();

	/* Activate real-time timer when window is shown */
	TimerList.start(GUI_TIMER_LIST_VIEW_UPDATE); /* Stations list */
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);
}

void LiveScheduleDlg::hideEvent(QHideEvent* pEvent)
{
	/* Deactivate real-time timers */
	TimerList.stop();
	TimerUTCLabel.stop();
}

void LiveScheduleDlg::SetStationsView()
{
	/* Set lock because of list view items. These items could be changed
	   by another thread */
	ListItemsMutex.Lock();

	const int iNumStations = DRMSchedule.GetStationNumber();

	_BOOLEAN bListHastChanged = FALSE;

	/* Add new item for each station in list view */
	for (int i = 0; i < iNumStations; i++)
	{
		if (!((bShowAll == FALSE) &&
			(DRMSchedule.CheckState(i) == CDRMLiveSchedule::IS_INACTIVE)))
		{
			/* Only insert item if it is not already in the list */
			if (vecpListItems[i] == NULL)
			{
				/* Generate new list item with all necessary column entries */
				vecpListItems[i] = new MyListLiveViewItem(ListViewStations,
					QString().setNum(DRMSchedule.GetItem(i).iFreq) /* freq. */,

					ExtractTime(DRMSchedule.GetItem(i).iStartTime) + "-" +
					ExtractTime(DRMSchedule.GetItem(i).iStartTime + DRMSchedule.GetItem(i).iDuration) /* time */,

					QString(DRMSchedule.GetItem(i).strTarget.c_str())   /* target */,
					ExtractDaysFlagString(DRMSchedule.GetItem(i).strDaysFlags) /* Show list of days */);

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}

			/* Check, if station is currently transmitting. If yes, set
			   special pixmap */
			if (DRMSchedule.CheckState(i) == CDRMLiveSchedule::IS_ACTIVE)
			{
				vecpListItems[i]->setPixmap(0, BitmCubeGreen);
			}
			else
			{
				if (DRMSchedule.CheckState(i) == CDRMLiveSchedule::IS_PREVIEW)
					vecpListItems[i]->setPixmap(0, BitmCubeOrange);
				else
					vecpListItems[i]->setPixmap(0, BitmCubeRed);
			}

		}
		else
		{
			/* Delete this item since it is not used anymore */
			if (vecpListItems[i] != NULL)
			{
				/* If one deletes a menu item in QT list view, it is
				   automaticall removed from the list and the list gets
				   repainted */
				delete vecpListItems[i];

				/* Reset pointer so we can distinguish if it is used or not */
				vecpListItems[i] = NULL;

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}
		}
	}

	/* Sort the list if items have changed */
	if (bListHastChanged == TRUE)
		ListViewStations->sort();

	ListItemsMutex.Unlock();
}

void LiveScheduleDlg::OnHeaderClicked(int c)
{
	/* Store the "direction" of sorting */
	if (iCurrentSortColumn == c)
		bCurrentSortAscending = !bCurrentSortAscending;
	else
		bCurrentSortAscending = TRUE;

	iCurrentSortColumn = c;
}

void LiveScheduleDlg::SetCurrentSavePath(const QString strFileName)
{
	strCurrentSavePath = QFileInfo(strFileName).dirPath();

	if (strCurrentSavePath.right(1).latin1() != QString("/"))
		strCurrentSavePath += "/";
}

QString ColValue(const QString strValue)
{
	if (strValue == "")
		return "&nbsp;";
	else
		return strValue;
}

void LiveScheduleDlg::OnSave()
{
	QString strFileName;
	QString strSchedule = "";
	QString strValue = "";

	/* Force the sort for all items */ 
 	ListViewStations->firstChild()
		->sortChildItems(iCurrentSortColumn,bCurrentSortAscending);

	/* Extract values from the list */
    QListViewItem * myItem = ListViewStations->firstChild();

    while( myItem )
	{
			strSchedule += "<tr>"
				"<td>" + myItem->text(COL_FREQ) + "</td>" /* freq */
				"<td>" + ColValue(myItem->text(1)) + "</td>" /* time */
				"<td>" + ColValue(myItem->text(2)) + "</td>" /* target */
				"<td>" + ColValue(myItem->text(3)) + "</td>" /* days */
				"</tr>\n";
        myItem = myItem->nextSibling();
    }

	if (strSchedule != "")
	{
		/* Save to file current schedule  */
		QString strTitle(tr("AFS Live Schedule"));
		QString strItems("");

		/* Prepare HTML page for storing the content */
		QString strText = "<html>\n<head>\n"
			"<meta http-equiv=\"content-Type\" "
			"content=\"text/html; charset=utf-8\">\n<title>" + strTitle +
			"</title>\n</head>\n\n<body>\n"
			"<h3>" + strTitle + "</h3>"
			"\n<table border=\"1\"><tr>\n"
			"<th>" + tr("Frequency [kHz]") + "</th>"
			"<th>" + tr("Time [UTC]") + "</th>"
			"<th>" + tr("Target") + "</th>"
			"<th>" + tr("Days") + "</th>\n"
			"</tr>\n"
			+ strSchedule +
			"</table>\n"
			/* Add current date and time */
			"<br><p align=right><font size=-2><i>" +
			QDateTime().currentDateTime().toString() + "</i></font></p>"
			"</body>\n</html>";

		strFileName = QFileDialog::getSaveFileName(strCurrentSavePath +
			"LiveSchedule.html", "*.html", this);

		if (!strFileName.isNull())
		{
			SetCurrentSavePath(strFileName);

			/* Save as a text stream */
			QFile FileObj(strFileName);

			if (FileObj.open(IO_WriteOnly))
			{
				QTextStream TextStream(&FileObj);
				TextStream << strText ; /* Actual writing */
				FileObj.close();
			}
		}
	}
}

void LiveScheduleDlg::AddWhatsThisHelp()
{
	/* Stations List */
	QWhatsThis::add(ListViewStations,
		tr("<b>Live Schedule List:</b> In the live schedule list "
		"view AFS (Alternative Frequency Signalling) informations trasmitted "
            "into the current DRM signal."
		"It is possible to show only active stations by changing a "
		"setting in the 'view' menu. The color of the cube on the left of a "
		"menu item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline.<br>"
		"If the stations preview is active an orange box shows the stations "
		"that will be active.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column."));

	/* UTC time label */
	QWhatsThis::add(TextLabelUTCTime,
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is also known as Greenwich Mean Time "
		"(GMT)."));
}


CDRMLiveSchedule::StationState CDRMLiveSchedule::CheckState(const int iPos)
{
	/* Get system time */
	time_t ltime;
	time(&ltime);

	if (IsActive(iPos, ltime) == TRUE)
		return IS_ACTIVE;
	else
	{
		/* Station is not active, check preview condition */
		if (iSecondsPreview > 0)
		{
			if (IsActive(iPos, ltime + iSecondsPreview) == TRUE)
				return IS_PREVIEW;
			else
				return IS_INACTIVE;
		}
		else
			return IS_INACTIVE;
	}
}

_BOOLEAN CDRMLiveSchedule::IsActive(const int iPos, const time_t ltime)
{
int iScheduleStart;
int iScheduleEnd;
int iWeekDay;

/* See ETSI ES 201 980 v2.1.1 Annex O */

	/* Empty schedule is always active */
	if (StationsTable[iPos].iDuration == 0)
		 return true;

	/* Calculate time in UTC */
	struct tm* gmtCur = gmtime(&ltime);
	const time_t lCurTime = mktime(gmtCur);

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0) 
	   I must normalize so Monday = 0   */
	
	if (gmtCur->tm_wday == 0)
		iWeekDay = 6;
	else
		iWeekDay = gmtCur->tm_wday - 1;

	/* iTimeWeek minutes since last Monday 00:00 in UTC */
	/* the value is in the range 0 <= iTimeWeek < 60 �24 �7)   */
	
	const int iTimeWeek = (iWeekDay * 24 * 60) + (gmtCur->tm_hour * 60) + gmtCur->tm_min;
	
	/* DaysFlag structure 1111111 = 1234567 (1 = Monday, 7 = Sunday) */
	for (int i = 0; i < 7; i++)
	{
		/* Check if day is active */
		if (StationsTable[iPos].strDaysFlags[i] == CHR_ACTIVE_DAY_MARKER)
		{
			/* Tuesday -> 1 �24 �60 = 1 440 */
			iScheduleStart = (i * 24 * 60) + StationsTable[iPos].iStartTime;
			iScheduleEnd = iScheduleStart + StationsTable[iPos].iDuration;

			/* the normal check (are we inside start and end?) */
			if ((iTimeWeek >= iScheduleStart) && (iTimeWeek <= iScheduleEnd))
				return true;

			/* the wrap-around check */
			const int iMinutesPerWeek = 7 * 24 * 60;

			if (iScheduleEnd > iMinutesPerWeek)
			{
				/* our duration wraps into next Monday (or even later) */
				if (iTimeWeek < (iScheduleEnd - iMinutesPerWeek))
					return true;
			}
		}
	}
	return false;
}
