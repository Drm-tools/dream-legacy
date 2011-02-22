/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Tomi Manninen
 *
 * 5/15/2005 Andrea Russo
 *	- added preview
 * 5/25/2005 Andrea Russo
 *	- added "days" column in stations list view
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

#include "StationsDlg.h"
#include "DialogUtil.h"
#if QT_VERSION < 0x040000
# include <qheader.h>
# include <qftp.h>
# include <qwhatsthis.h>
# define Q3WhatsThis QWhatsThis
#else
# include <q3ftp.h>
# include <q3whatsthis.h>
# include <QHideEvent>
# include <QShowEvent>
# define CHECK_PTR(x) Q_CHECK_PTR(x)
#endif

/* Implementation *************************************************************/
QString MyListViewItem::key(int column, bool ascending) const
{
	/* Reimplement "key()" function to get correct sorting behaviour */
	if ((column == 2) || (column == 4))
	{
		/* These columns are filled with numbers. Some items may have numbers
		   after the comma, therefore multiply with 10000 (which moves the
		   numbers in front of the comma). Afterwards append zeros at the
		   beginning so that positive integer numbers are sorted correctly */
		return QString(QString().setNum((long int)
			(text(column).toFloat() * 10000.0))).rightJustify(20, '0');
	}
    else
		return Q3ListViewItem::key(column, ascending);
}

void CDRMSchedule::UpdateStringListForFilter(const CStationsItem StationsItem)
{
QStringList result;

QString strTarget = QString(StationsItem.strTarget.c_str());
QString strCountry = QString(StationsItem.strCountry.c_str());
QString strLanguage = QString(StationsItem.strLanguage.c_str());

   result = ListTargets.grep(strTarget);
   if (result.isEmpty())
     ListTargets.append(strTarget);

   result = ListCountries.grep(strCountry);
   if (result.isEmpty())
     ListCountries.append(strCountry);


   result = ListLanguages.grep(strLanguage);
   if (result.isEmpty())
     ListLanguages.append(strLanguage);
}

void StationsDlg::FilterChanged(const QString&)
{
	/* Update list view */
	SetStationsView();
}

_BOOLEAN StationsDlg::CheckFilter(const int iPos)
{
_BOOLEAN bCheck = TRUE;
QString sFilter = "";

	sFilter = ComboBoxFilterTarget->currentText();

	if ((sFilter != "") &&
		(QString(DRMSchedule.GetItem(iPos).strTarget.c_str()) != sFilter))
		bCheck = FALSE;

	sFilter = ComboBoxFilterCountry->currentText();

	if ((sFilter != "") &&
		(QString(DRMSchedule.GetItem(iPos).strCountry.c_str()) != sFilter))
		bCheck = FALSE;

	sFilter = ComboBoxFilterLanguage->currentText();

	if ((sFilter != "") &&
		(QString(DRMSchedule.GetItem(iPos).strLanguage.c_str()) != sFilter))
		bCheck = FALSE;

return bCheck;
}

void CDRMSchedule::ReadStatTabFromFile(const ESchedMode eNewSchM)
{
	const int	iMaxLenName = 256;
	char		cName[iMaxLenName];
	int			iFileStat;
	_BOOLEAN	bReadOK = TRUE;
	FILE*		pFile = NULL;

	/* Save new mode */
	SetSchedMode(eNewSchM);

	/* Open file and init table for stations */
	StationsTable.clear();

	switch (eNewSchM)
	{
	case SM_DRM:
		pFile = fopen(DRMSCHEDULE_INI_FILE_NAME, "r");
		break;

	case SM_ANALOG:
		pFile = fopen(AMSCHEDULE_INI_FILE_NAME, "r");
		break;
	}

	/* Check if opening of file was successful */
	if (pFile == 0)
		return;

	fgets(cName, iMaxLenName, pFile); /* Remove "[DRMSchedule]" */
	do
	{
		CStationsItem StationsItem;

		/* Start stop time */
		int iStartTime, iStopTime;
		iFileStat = fscanf(pFile, "StartStopTimeUTC=%04d-%04d\n",
			&iStartTime, &iStopTime);

		if (iFileStat != 2)
			bReadOK = FALSE;
		else
		{
			StationsItem.SetStartTimeNum(iStartTime);
			StationsItem.SetStopTimeNum(iStopTime);
		}

		/* Days */
		/* Init days with the "irregular" marker in case no valid string could
		   be read */
		string strNewDaysFlags = FLAG_STR_IRREGULAR_TRANSM;

		iFileStat = fscanf(pFile, "Days[SMTWTFS]=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
		{
			/* Check for length of input string (must be 7) */
			const string strTMP = cName;
			if (strTMP.length() == 7)
				strNewDaysFlags = strTMP;
		}

		/* Frequency */
		iFileStat = fscanf(pFile, "Frequency=%d\n", &StationsItem.iFreq);
		if (iFileStat != 1)
			bReadOK = FALSE;

		/* Target */
		iFileStat = fscanf(pFile, "Target=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strTarget = cName;

		/* Power */
		iFileStat = fscanf(pFile, "Power=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.rPower = QString(cName).toFloat();

		/* Name of the station */
		iFileStat = fscanf(pFile, "Programme=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strName = cName;

		/* Language */
		iFileStat = fscanf(pFile, "Language=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strLanguage = cName;

		/* Site */
		iFileStat = fscanf(pFile, "Site=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strSite = cName;

		/* Country */
		iFileStat = fscanf(pFile, "Country=%255[^\n|^\r]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strCountry = cName;

		iFileStat = fscanf(pFile, "\n");

		/* Check for error before applying data */
		if (bReadOK == TRUE)
		{
			/* Set "days flag string" and generate strings for displaying active
			   days */
			StationsItem.SetDaysFlagString(strNewDaysFlags);

			/* Add new item in table */
			StationsTable.push_back(StationsItem);

			UpdateStringListForFilter(StationsItem);
}
	} while (!((iFileStat == EOF) || (bReadOK == FALSE)));

	fclose(pFile);
}

CDRMSchedule::StationState CDRMSchedule::CheckState(const int iPos)
{
	/* Get system time */
	time_t ltime;
	time(&ltime);

	if (IsActive(iPos, ltime) == TRUE)
	{
		/* Check if the station soon will be inactive */
		if (IsActive(iPos, ltime + NUM_SECONDS_SOON_INACTIVE) == TRUE)
			return IS_ACTIVE;
		else
			return IS_SOON_INACTIVE;
	}
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

_BOOLEAN CDRMSchedule::IsActive(const int iPos, const time_t ltime)
{
	/* Calculate time in UTC */
	struct tm* gmtCur = gmtime(&ltime);
	const time_t lCurTime = mktime(gmtCur);

	/* Get stop time */
	struct tm* gmtStop = gmtime(&ltime);
	gmtStop->tm_hour = StationsTable[iPos].iStopHour;
	gmtStop->tm_min = StationsTable[iPos].iStopMinute;
	const time_t lStopTime = mktime(gmtStop);

	/* Get start time */
	struct tm* gmtStart = gmtime(&ltime);
	gmtStart->tm_hour = StationsTable[iPos].iStartHour;
	gmtStart->tm_min = StationsTable[iPos].iStartMinute;
	const time_t lStartTime = mktime(gmtStart);

	/* Check, if stop time is on next day */
	_BOOLEAN bSecondDay = FALSE;
	if (lStopTime < lStartTime)
	{
		/* Check, if we are at the first or the second day right now */
		if (lCurTime < lStopTime)
		{
			/* Second day. Increase day count */
			gmtCur->tm_wday++;

			/* Check that value is valid (range 0 - 6) */
			if (gmtCur->tm_wday > 6)
				gmtCur->tm_wday = 0;

			/* Set flag */
			bSecondDay = TRUE;
		}
	}

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0). "strDaysFlags" are coded with
	   pseudo binary representation. A one signalls that day is active. The most
	   significant 1 is the sunday, then followed the monday and so on. */
	if ((StationsTable[iPos].strDaysFlags[gmtCur->tm_wday] ==
		CHR_ACTIVE_DAY_MARKER) ||
		/* Check also for special case: days are 0000000. This is reserved for
		   DRM test transmissions or irregular transmissions. We define here
		   that these stations are transmitting every day */
		(StationsTable[iPos].strDaysFlags == FLAG_STR_IRREGULAR_TRANSM))
	{
		/* Check time interval */
		if (lStopTime > lStartTime)
		{
			if ((lCurTime >= lStartTime) && (lCurTime < lStopTime))
				return TRUE;
		}
		else
		{
			if (bSecondDay == FALSE)
			{
				/* First day. Only check if we are after start time */
				if (lCurTime >= lStartTime)
					return TRUE;
			}
			else
			{
				/* Second day. Only check if we are before stop time */
				if (lCurTime < lStopTime)
					return TRUE;
			}
		}
	}

	return FALSE;
}

void CStationsItem::SetDaysFlagString(const string strNewDaysFlags)
{
	/* Set internal "days flag" string and "show days" string */
	strDaysFlags = strNewDaysFlags;
	strDaysShow = "";

	/* Init days string vector */
	const QString strDayDef [] =
	{
		QObject::tr("Sun"),
		QObject::tr("Mon"),
		QObject::tr("Tue"),
		QObject::tr("Wed"),
		QObject::tr("Thu"),
		QObject::tr("Fri"),
		QObject::tr("Sat")
	};

	/* First test for day constellations which allow to apply special names */
	if (strDaysFlags == FLAG_STR_IRREGULAR_TRANSM)
		strDaysShow = QObject::tr("irregular").latin1();
	else if (strDaysFlags == "1111111")
		strDaysShow = QObject::tr("daily").latin1();
	else if (strDaysFlags == "1111100")
		strDaysShow = QObject::tr("from Sun to Thu").latin1();
	else if (strDaysFlags == "1111110")
		strDaysShow = QObject::tr("from Sun to Fri").latin1();
	else if (strDaysFlags == "0111110")
		strDaysShow = QObject::tr("from Mon to Fri").latin1();
	else if (strDaysFlags == "0111111")
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
}

StationsDlg::StationsDlg(CDRMReceiver& NDRMR, CSettings& NSettings, CRig& nrig,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	CStationsDlgBase(parent, name, modal, f),
	DRMReceiver(NDRMR),rig(nrig),
	Settings(NSettings),
	bReInitOnFrequencyChange(FALSE), vecpListItems(0)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Stations Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

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
	BitmCubePink.resize(iXSize, iYSize);
	BitmCubePink.fill(QColor(255, 128, 128));

	ProgrSigStrength->hide();
	TextLabelSMeter->hide();

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(0, tr("Station Name"));
	ListViewStations->addColumn(tr("Time [UTC]"));
	ListViewStations->addColumn(tr("Frequency [kHz]"));
	ListViewStations->addColumn(tr("Target"));
	ListViewStations->addColumn(tr("Power [kW]"));
	ListViewStations->addColumn(tr("Country"));
	ListViewStations->addColumn(tr("Site"));
	ListViewStations->addColumn(tr("Language"));
	ListViewStations->addColumn(tr("Days"));

	/* Set right alignment for numeric columns */
	ListViewStations->setColumnAlignment(2, Qt::AlignRight);
	ListViewStations->setColumnAlignment(4, Qt::AlignRight);

	/* Set up frequency selector control (QWTCounter control) */
	QwtCounterFrequency->setRange(0.0, MAX_RF_FREQ, 1.0);
	QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
	QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);
	QwtCounterFrequency->setValue(DRMReceiver.GetFrequency());

	/* Init UTC time shown with a label control */
	SetUTCTimeLabel();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	pViewMenu = new Q3PopupMenu(this);
	CHECK_PTR(pViewMenu);
	pViewMenu->insertItem(tr("Show &only active stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem(tr("Show &all stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Set stations in list view which are active right now */
	bShowAll = FALSE;
	pViewMenu->setItemChecked(0, TRUE);

	/* Stations Preview menu ------------------------------------------------ */
	pPreviewMenu = new Q3PopupMenu(this);
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
	/* Retrieve the setting saved into the .ini file */
	switch (Settings.Get("Stations Dialog", "preview", NUM_SECONDS_PREV_5MIN))
	{
	case NUM_SECONDS_PREV_5MIN:
		pPreviewMenu->setItemChecked(1, TRUE);
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
		break;

	case NUM_SECONDS_PREV_15MIN:
		pPreviewMenu->setItemChecked(2, TRUE);
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case NUM_SECONDS_PREV_30MIN:
		pPreviewMenu->setItemChecked(3, TRUE);
		DRMSchedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0, also takes care of out of value parameters */
		pPreviewMenu->setItemChecked(0, TRUE);
		DRMSchedule.SetSecondsPreview(0);
		break;
	}

	pViewMenu->insertSeparator();
	pViewMenu->insertItem(tr("Stations &preview"),pPreviewMenu);

	SetStationsView();

	/* Remote menu  --------------------------------------------------------- */
	pRemoteMenu = new RemoteMenu(this, rig);
	/* Separator */
	pRemoteMenu->menu()->insertSeparator();

	/* Enable s-meter */
	const int iSMeterMenuID = pRemoteMenu->menu()->insertItem(tr("Enable S-Meter"),
		this, SLOT(OnSMeterMenu(int)), 0, SMETER_MENU_ID);

	connect(pRemoteMenu, SIGNAL(SMeterAvailable()), this, SLOT(OnSMeterAvailable()));
#ifdef HAVE_LIBHAMLIB
	connect(&rig, SIGNAL(sigstr(double)), this, SLOT(OnSigStr(double)));
#endif

	/* Init progress bar for input s-meter */

	ProgrSigStrength->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
#if QT_VERSION < 0x040000
	ProgrSigStrength->setOrientation(QwtThermo::Horizontal, QwtThermo::Top);
#else
	ProgrSigStrength->setOrientation(Qt::Horizontal, QwtThermo::TopScale);
#endif
	ProgrSigStrength->setAlarmLevel(S_METER_THERMO_ALARM);
	ProgrSigStrength->setAlarmColor(QColor(255, 0, 0));
	ProgrSigStrength->setScale(S_METER_THERMO_MIN, S_METER_THERMO_MAX, 10.0);

	ProgrSigStrength->setAlarmEnabled(TRUE);
	ProgrSigStrength->setValue(S_METER_THERMO_MIN);
	ProgrSigStrength->setFillColor(QColor(0, 190, 0));

	/* Update menu ---------------------------------------------------------- */
	pUpdateMenu = new Q3PopupMenu(this);
	CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem(tr("&Get Update..."), this, SLOT(OnGetUpdate()), 0, 0);

	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), pViewMenu);
	pMenu->insertItem(tr("&Remote"), pRemoteMenu->menu());
	pMenu->insertItem(tr("&Update"), pUpdateMenu); /* String "Update" used below */
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
#if QT_VERSION < 0x040000
	CStationsDlgBaseLayout->setMenuBar(pMenu);
#else
//TODO
#endif


	/* Register the network protokol (ftp). This is needed for the DRMSchedule
	   download */
	Q3NetworkProtocol::registerNetworkProtocol("ftp",
		new Q3NetworkProtocolFactory<Q3Ftp>);

	/* Connections ---------------------------------------------------------- */

	connect(&TimerList, SIGNAL(timeout()),
		this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()),
		this, SLOT(OnTimerUTCLabel()));

	TimerList.stop();
	TimerUTCLabel.stop();

#if QT_VERSION < 0x040000
	connect(ListViewStations, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));
	connect(&UrlUpdateSchedule, SIGNAL(finished(QNetworkOperation*)),
		this, SLOT(OnUrlFinished(QNetworkOperation*)));
#else
	connect(ListViewStations, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnListItemClicked(Q3ListViewItem*)));
	connect(&UrlUpdateSchedule, SIGNAL(finished(Q3NetworkOperation*)),
		this, SLOT(OnUrlFinished(Q3NetworkOperation*)));
#endif

	connect(ListViewStations->header(), SIGNAL(clicked(int)),
		this, SLOT(OnHeaderClicked(int)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));

	connect(ComboBoxFilterTarget, SIGNAL(activated(const QString&)),
                  this, SLOT(FilterChanged(const QString&)));
	connect(ComboBoxFilterCountry, SIGNAL(activated(const QString&)),
                  this, SLOT(FilterChanged(const QString&)));
	connect(ComboBoxFilterLanguage, SIGNAL(activated(const QString&)),
                  this, SLOT(FilterChanged(const QString&)));
}

StationsDlg::~StationsDlg()
{
}

void StationsDlg::SetUTCTimeLabel()
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

void StationsDlg::OnShowStationsMenu(int iID)
{
	/* Show only active stations if ID is 0, else show all */
	if (iID == 0)
	{
		bShowAll = FALSE;
		/* clear all and reload. If the list is too big this increase the performance */
		ClearStationsView();
	}
	else
		bShowAll = TRUE;

	/* Update list view */
	SetStationsView();

	/* Taking care of checks in the menu */
	pViewMenu->setItemChecked(0, 0 == iID);
	pViewMenu->setItemChecked(1, 1 == iID);
}

void StationsDlg::OnShowPreviewMenu(int iID)
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

void StationsDlg::AddUpdateDateTime()
{
/*
	Set time and date of current DRM schedule in the menu text for
	querying a new schedule (online schedule update).
	If no schedule file exists, do not show any time and date.
*/
	/* make sure we check the correct file (DRM or AM schedule) */
	QString strFile;
	switch (DRMSchedule.GetSchedMode())
	{
		case CDRMSchedule::SM_DRM:
			strFile = DRMSCHEDULE_INI_FILE_NAME;
			pUpdateMenu->setItemEnabled(0, TRUE);
			break;

		case CDRMSchedule::SM_ANALOG:
			strFile = AMSCHEDULE_INI_FILE_NAME;
			pUpdateMenu->setItemEnabled(0, FALSE);
			break;
	}

	/* init with empty string in case there is no schedule file */
	QString s = "";

	/* get time and date information */
	QFileInfo f = QFileInfo(strFile);
	if (f.exists()) /* make sure the DRM schedule file exists */
	{
		/* use QT-type of data string for displaying */
		s = tr(" (last update: ")
			+ f.lastModified().date().toString() + ")";
	}

#if QT_VERSION < 0x040000
	pUpdateMenu->changeItem(tr("&Get Update") + s + "...", 0);
#else
	pUpdateMenu->changeItem(0, tr("&Get Update") + s + "...");
#endif
}

void StationsDlg::OnGetUpdate()
{
	if (QMessageBox::information(this, tr("Dream Schedule Update"),
		tr("Dream tries to download the newest DRM schedule\nfrom "
		"www.drm-dx.de (powered by Klaus Schneider).\nYour computer "
		"must be connected to the internet.\n\nThe current file "
		"DRMSchedule.ini will be overwritten.\nDo you want to "
		"continue?"),
		QMessageBox::Yes, QMessageBox::No) == 3 /* Yes */)
	{
		/* Try to download the current schedule. Copy the file to the
		   current working directory (which is "QDir().absFilePath(NULL)") */
		UrlUpdateSchedule.copy(QString(DRM_SCHEDULE_UPDATE_FILE),
			QString(QDir().absFilePath(NULL)));
	}
}

#if QT_VERSION < 0x040000
void StationsDlg::OnUrlFinished(QNetworkOperation* pNetwOp)
#else
void StationsDlg::OnUrlFinished(Q3NetworkOperation* pNetwOp)
#endif
{
	/* Check that pointer points to valid object */
	if (pNetwOp)
	{
		if (pNetwOp->state() == Q3NetworkProtocol::StFailed)
		{
			/* Something went wrong -> stop all network operations */
			UrlUpdateSchedule.stop();

			/* Notify the user of the failure */
			QMessageBox::information(this, "Dream",
				tr("Update failed. The following things may caused the "
				"failure:\n"
				"\t- the internet connection was not set up properly\n"
				"\t- the server www.drm-dx.de is currently not available\n"
				"\t- the file 'DRMSchedule.ini' could not be written"),
				QMessageBox::Ok);
		}

		/* We are interested in the state of the final put function */
		if (pNetwOp->operation() == Q3NetworkProtocol::OpPut)
		{
			if (pNetwOp->state() == Q3NetworkProtocol::StDone)
			{
				/* Notify the user that update was successful */
				QMessageBox::information(this, "Dream",
					tr("Update successful."), QMessageBox::Ok);
				/* Read updated ini-file */
				LoadSchedule(CDRMSchedule::SM_DRM);
			}
		}
	}
}

void StationsDlg::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timers */
	TimerList.stop();
	TimerUTCLabel.stop();
	DisableSMeter();

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	CWinGeom c;
	c.iXPos = WinGeom.x();
	c.iYPos = WinGeom.y();
	c.iHSize = WinGeom.height();
	c.iWSize = WinGeom.width();
	Settings.Put("Stations Dialog", c);

	/* Store preview settings */
	Settings.Put("Stations Dialog", "preview", DRMSchedule.GetSecondsPreview());

	/* Store sort settings */
	switch (DRMSchedule.GetSchedMode())
	{
	case CDRMSchedule::SM_DRM:
		Settings.Put("Stations Dialog", "sortcolumndrm", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendingdrm", bCurrentSortAscending);
		break;

	case CDRMSchedule::SM_ANALOG:
		Settings.Put("Stations Dialog", "sortcolumnanalog", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendinganalog", bCurrentSortAscending);
		break;
	}
}

void StationsDlg::showEvent(QShowEvent*)
{
	/* Load the schedule if necessary */
	if (DRMSchedule.GetStationNumber() == 0)
		LoadSchedule(DRMSchedule.GetSchedMode());

	/* If number of stations is zero, we assume that the ini file is missing */
	if (DRMSchedule.GetStationNumber() == 0)
	{
		if (DRMSchedule.GetSchedMode() == CDRMSchedule::SM_DRM)
		{
			QMessageBox::information(this, "Dream", tr("The file "
				DRMSCHEDULE_INI_FILE_NAME
				" could not be found or contains no data.\nNo "
				"stations can be displayed.\nTry to download this file by "
				"using the 'Update' menu."), QMessageBox::Ok);
		}
		else
		{
			QMessageBox::information(this, "Dream", tr("The file "
				AMSCHEDULE_INI_FILE_NAME
				" could not be found or contains no data.\nNo "
				"stations can be displayed."), QMessageBox::Ok);
		}
	}

	/* Update window */
	OnTimerUTCLabel();
	OnTimerList();

	/* Activate real-time timer when window is shown */
	TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);

	/* S-meter settings */
	if(Settings.Get("Hamlib", "ensmeter", int(0)))
	{
		if(pRemoteMenu) pRemoteMenu->menu()->setItemChecked(SMETER_MENU_ID, true);
		EnableSMeter();
	}
	else
	{
		if(pRemoteMenu) pRemoteMenu->menu()->setItemChecked(SMETER_MENU_ID, false);
		DisableSMeter();
	}

	/* add last update information on menu item */
	AddUpdateDateTime();
}

void StationsDlg::OnTimerList()
{
	/* frequency could be changed by evaluation dialog or RSCI */
	int iFrequency = DRMReceiver.GetFrequency();
	int iCurFrequency = QwtCounterFrequency->value();

	if (iFrequency != iCurFrequency)
	{
		QwtCounterFrequency->setValue(iFrequency);
	}

	/* Update list view */
	SetStationsView();
}

void StationsDlg::SetSortSettings(const CDRMSchedule::ESchedMode eNewSchM)
{
	/* Store the current sort settings before switching */
	if (eNewSchM != DRMSchedule.GetSchedMode())
	{
		switch (DRMSchedule.GetSchedMode())
		{
		case CDRMSchedule::SM_DRM:
			Settings.Put("Stations Dialog", "sortcolumndrm", iCurrentSortColumn);
			Settings.Put("Stations Dialog", "sortascendingdrm", bCurrentSortAscending);
			break;

		case CDRMSchedule::SM_ANALOG:
			Settings.Put("Stations Dialog", "sortcolumnanalog", iCurrentSortColumn);
			Settings.Put("Stations Dialog", "sortascendinganalog", bCurrentSortAscending);
			break;
		}
	}

	/* Set sorting behaviour of the list */
	switch (eNewSchM)
	{
	case CDRMSchedule::SM_DRM:
		iCurrentSortColumn = Settings.Get("Stations Dialog", "sortcolumndrm", 0);
		bCurrentSortAscending = Settings.Get("Stations Dialog", "sortascendingdrm", TRUE);
		break;

	case CDRMSchedule::SM_ANALOG:
		iCurrentSortColumn = Settings.Get("Stations Dialog", "sortcolumnanalog", 0);
		bCurrentSortAscending = Settings.Get("Stations Dialog", "sortascendinganalog", TRUE);
		break;
	}
	ListViewStations->setSorting(iCurrentSortColumn, bCurrentSortAscending);
}

void StationsDlg::SetCurrentSchedule(const CDRMSchedule::ESchedMode eNewSchM)
{
	SetSortSettings(eNewSchM);

	/* Save new mode */
	DRMSchedule.SetSchedMode(eNewSchM);
}

void StationsDlg::LoadSchedule(CDRMSchedule::ESchedMode eNewSchM)
{
	SetSortSettings(eNewSchM);

	ClearStationsView();
	/* Empty the string lists for combos filter */
	DRMSchedule.ListTargets = QStringList("");
	DRMSchedule.ListCountries = QStringList("");
	DRMSchedule.ListLanguages = QStringList("");

	ComboBoxFilterTarget->clear();
	ComboBoxFilterCountry->clear();
	ComboBoxFilterLanguage->clear();

	/* Read initialization file */
	DRMSchedule.ReadStatTabFromFile(eNewSchM);

	DRMSchedule.ListTargets.sort();
	DRMSchedule.ListCountries.sort();
	DRMSchedule.ListLanguages.sort();

	ComboBoxFilterTarget->insertStringList(DRMSchedule.ListTargets);
	ComboBoxFilterCountry->insertStringList(DRMSchedule.ListCountries);
	ComboBoxFilterLanguage->insertStringList(DRMSchedule.ListLanguages);

	/* Update list view */
	SetStationsView();

	/* Add last update information on menu item if the dialog is visible */
	if (this->isVisible())
		AddUpdateDateTime();
}

void StationsDlg::ClearStationsView()
{
	/* Delete all old list view items (it is important that the vector
	   "vecpListItems" was initialized to 0 at creation of the global object
	   otherwise this may cause an segmentation fault) */
	ListItemsMutex.lock();
	ListViewStations->clear();
	/*
	for (size_t i = 0; i < vecpListItems.size(); i++)
	{
		if (vecpListItems[i] != NULL)
			delete vecpListItems[i];
	}
	*/
	vecpListItems.clear();
	ListItemsMutex.unlock();
}

void StationsDlg::SetStationsView()
{
	size_t i;
	/* Stop the timer and disable the list */
	TimerList.stop();

	const _BOOLEAN bListFocus = ListViewStations->hasFocus();

	ListViewStations->setUpdatesEnabled(FALSE);
	ListViewStations->setEnabled(FALSE);

	/* Set lock because of list view items. These items could be changed
	   by another thread */
	ListItemsMutex.lock();

	const size_t iNumStations = DRMSchedule.GetStationNumber();
	_BOOLEAN bListHastChanged = FALSE;

	/* if the list got smaller, we need to free some memory */
	for (i = iNumStations; i < vecpListItems.size(); i++)
	{
		if (vecpListItems[i] != NULL)
			delete vecpListItems[i];
	}
	/* resize will leave all existing elements alone and add
	 * nulls in case the list needed to get bigger
	 */
	vecpListItems.resize(iNumStations, (MyListViewItem*) NULL);

	/* Add new item for each station in list view */
	for (i = 0; i < iNumStations; i++)
	{
		CDRMSchedule::StationState iState = DRMSchedule.CheckState(i);

		if (!(((bShowAll == FALSE) &&
			(iState == CDRMSchedule::IS_INACTIVE))
			|| (CheckFilter(i) == FALSE)))
		{
			/* Only insert item if it is not already in the list */
			if (vecpListItems[i] == NULL)
			{
				/* Get power of the station. We have to do a special treatment
				   here, because we want to avoid having a "0" in the list when
				   a "?" was in the schedule-ini-file */
				const _REAL rPower = DRMSchedule.GetItem(i).rPower;

				QString strPower;
				if (rPower == (_REAL) 0.0)
					strPower = "?";
				else
					strPower.setNum(rPower);

				/* Generate new list item with all necessary column entries */
				vecpListItems[i] = new MyListViewItem(ListViewStations,
					DRMSchedule.GetItem(i).strName.c_str()     /* name */,
					QString().sprintf("%04d-%04d",
					DRMSchedule.GetItem(i).GetStartTimeNum(),
					DRMSchedule.GetItem(i).GetStopTimeNum())   /* time */,
					QString().setNum(DRMSchedule.GetItem(i).iFreq) /* freq. */,
					DRMSchedule.GetItem(i).strTarget.c_str()   /* target */,
					strPower                                   /* power */,
					DRMSchedule.GetItem(i).strCountry.c_str()  /* country */,
					DRMSchedule.GetItem(i).strSite.c_str()     /* site */,
					DRMSchedule.GetItem(i).strLanguage.c_str() /* language */);

				/* Show list of days */
				vecpListItems[i]->setText(8,
					DRMSchedule.GetItem(i).strDaysShow.c_str());

				/* Insert this new item in list. The item object is destroyed by
				   the list view control when this is destroyed */
				ListViewStations->insertItem(vecpListItems[i]);

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}

			/* Check, if station is currently transmitting. If yes, set
			   special pixmap */
			switch (iState)
			{
				case CDRMSchedule::IS_ACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubeGreen);
					break;
				case CDRMSchedule::IS_PREVIEW:
					vecpListItems[i]->setPixmap(0, BitmCubeOrange);
					break;
				case CDRMSchedule::IS_SOON_INACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubePink);
					break;
				case CDRMSchedule::IS_INACTIVE:
					vecpListItems[i]->setPixmap(0, BitmCubeRed);
					break;
				default:
					vecpListItems[i]->setPixmap(0, BitmCubeRed);
					break;
			}
		}
		else
		{
			/* Delete this item since it is not used anymore */
			if (vecpListItems[i] != NULL)
			{
				/* If one deletes a item in QT list view, it is
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

	ListItemsMutex.unlock();

	/* Start the timer and enable the list */
	ListViewStations->setUpdatesEnabled(TRUE);
	ListViewStations->setEnabled(TRUE);

	/* to update the scrollbars */
	ListViewStations->triggerUpdate();

	if (bListFocus == TRUE)
		ListViewStations->setFocus();

	TimerList.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnFreqCntNewValue(double dVal)
{
	/* Set frequency to front-end */
	DRMReceiver.SetFrequency((int) dVal);
}

void StationsDlg::OnHeaderClicked(int c)
{
	/* Store the "direction" of sorting */
	if (iCurrentSortColumn == c)
		bCurrentSortAscending = !bCurrentSortAscending;
	else
		bCurrentSortAscending = TRUE;

	iCurrentSortColumn = c;
}

void StationsDlg::OnListItemClicked(Q3ListViewItem* item)
{
	/* Check that it is a valid item (!= 0) */
	if (item)
	{
		/* Third text of list view item is frequency -> text(2)
		   Set value in frequency counter control QWT. Setting this parameter
		   will emit a "value changed" signal which sets the new frequency.
		   Therefore, here is no call to "SetFrequency()" needed.*/
		QwtCounterFrequency->setValue(QString(item->text(2)).toInt());

		/* If the mode has changed re-initialise the receiver */
		ERecMode eCurrentMode = DRMReceiver.GetReceiverMode();

		/* if "bReInitOnFrequencyChange" is not true, initiate a reinit when
		 schedule mode is different from receiver mode */
		switch (DRMSchedule.GetSchedMode())
		{
		case CDRMSchedule::SM_DRM:
			if (eCurrentMode != RM_DRM)
				DRMReceiver.SetReceiverMode(RM_DRM);
			if (bReInitOnFrequencyChange)
				DRMReceiver.RequestNewAcquisition();
			break;

		case CDRMSchedule::SM_ANALOG:
			if (eCurrentMode != RM_AM)
				DRMReceiver.SetReceiverMode(RM_AM);
			if (bReInitOnFrequencyChange)
				DRMReceiver.RequestNewAcquisition();
			break;
		}
	}
}

void StationsDlg::OnSMeterAvailable()
{
	/* If model is changed, update s-meter because new rig might have support
	   for it. */
	EnableSMeter();
}

void StationsDlg::OnSMeterMenu(int iID)
{
	if (pRemoteMenu->menu()->isItemChecked(iID))
	{
		pRemoteMenu->menu()->setItemChecked(iID, FALSE);
		DisableSMeter();
		Settings.Put("Hamlib", "ensmeter", 0);
	}
	else
	{
		pRemoteMenu->menu()->setItemChecked(iID, TRUE);
		EnableSMeter();
		Settings.Put("Hamlib", "ensmeter", 1);
	}
}

void StationsDlg::EnableSMeter()
{
	ProgrSigStrength->setEnabled(TRUE);
	TextLabelSMeter->setEnabled(TRUE);
	ProgrSigStrength->show();
#ifdef HAVE_LIBHAMLIB
	rig.subscribe();
#endif
}

void StationsDlg::DisableSMeter()
{
	ProgrSigStrength->hide();
#ifdef HAVE_LIBHAMLIB
	rig.unsubscribe();
#endif
}

void StationsDlg::OnSigStr(double rCurSigStr)
{
	ProgrSigStrength->setValue(rCurSigStr);
}

void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	Q3WhatsThis::add(ListViewStations,
		tr("<b>Stations List:</b> In the stations list "
		"view all DRM stations which are stored in the DRMSchedule.ini file "
		"are shown. It is possible to show only active stations by changing a "
		"setting in the 'view' menu. The color of the cube on the left of a "
		"menu item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline, "
		"a pink cube shown that the transmission soon will be offline.<br>"
		"If the stations preview is active an orange box shows the stations "
		"that will be active.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column.<br>By clicking on a menu item, a remote front-end can "
		"be automatically switched to the current frequency and the "
		"Dream software is reset to a new acquisition (to speed up the "
		"synchronization process). Also, the log-file frequency edit "
		"is automatically updated."));

	/* Frequency Counter */
	Q3WhatsThis::add(QwtCounterFrequency,
		tr("<b>Frequency Counter:</b> The current frequency "
		"value can be changed by using this counter. The tuning steps are "
		"100 kHz for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically."));

	/* UTC time label */
	Q3WhatsThis::add(TextLabelUTCTime,
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is also known as Greenwich Mean Time "
		"(GMT)."));

#ifdef HAVE_LIBHAMLIB
	/* S-meter */
	const QString strSMeter =
		tr("<b>Signal-Meter:</b> Shows the signal strength "
		"level in dB relative to S9.<br>Note that not all "
		"front-ends controlled by hamlib support this feature. If the s-meter "
		"is not available, the controls are disabled.");

	Q3WhatsThis::add(TextLabelSMeter, strSMeter);
	Q3WhatsThis::add(ProgrSigStrength, strSMeter);
#endif
}
