/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Mark J. Fine, Markus Maerz, Tomi Manninen, Stephane Fillod
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

#include "StationsDlg.h"
#ifndef _WIN32
# include <sys/termios.h>
#endif

#if !defined(HAVE_RIG_PARSE_MODE) && defined(HAVE_LIBHAMLIB)
extern "C"
{
	extern rmode_t parse_mode(const char *);
	extern vfo_t parse_vfo(const char *);
	extern setting_t parse_func(const char *);
	extern setting_t parse_level(const char *);
	extern setting_t parse_parm(const char *);
	extern const char* strstatus(enum rig_status_e);
}
# define rig_parse_mode parse_mode
# define rig_parse_vfo parse_vfo
# define rig_parse_func parse_func
# define rig_parse_level parse_level
# define rig_parse_parm parse_parm
# define rig_strstatus strstatus
#endif


/* Implementation *************************************************************/
CDRMSchedule::CDRMSchedule()
{
	ReadStatTabFromFile("DRMSchedule.ini");
}

void CDRMSchedule::ReadStatTabFromFile(const string strFileName)
{
	const int	iMaxLenName = 256;
	char		cName[iMaxLenName];
	int			iFileStat;
	_BOOLEAN	bReadOK = TRUE;

	/* Open file and init table for stations */
	StationsTable.Init(0);
	FILE* pFile = fopen(strFileName.c_str(), "r");

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
		iFileStat = fscanf(pFile, "Days[SMTWTFS]=%d\n", &StationsItem.iDays);
		if (iFileStat != 1)
			bReadOK = FALSE;

		/* Frequency */
		iFileStat = fscanf(pFile, "Frequency=%d\n", &StationsItem.iFreq);
		if (iFileStat != 1)
			bReadOK = FALSE;

		/* Target */
		iFileStat = fscanf(pFile, "Target=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strTarget = cName;

		/* Power */
		iFileStat = fscanf(pFile, "Power=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.rPower = QString(cName).toFloat();

		/* Name of the station */
		iFileStat = fscanf(pFile, "Programme=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strName = cName;

		/* Language */
		iFileStat = fscanf(pFile, "Language=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strLanguage = cName;

		/* Site */
		iFileStat = fscanf(pFile, "Site=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strSite = cName;

		/* Country */
		iFileStat = fscanf(pFile, "Country=%255[^\n]\n", cName);
		if (iFileStat != 1)
			fscanf(pFile, "\n");
		else
			StationsItem.strCountry = cName;

		iFileStat = fscanf(pFile, "\n");


		/* Add new item in table */
		if (bReadOK == TRUE)
			StationsTable.Add(StationsItem);
	} while (!((iFileStat == EOF) || (bReadOK == FALSE)));

	fclose(pFile);
}

_BOOLEAN CDRMSchedule::IsActive(int const iPos)
{
	/* Get system time */
	time_t ltime;
	time(&ltime);

	/* Current UTC time */
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
	   tm_wday: day of week (0 - 6; Sunday = 0). "iDays" are coded with pseudo
	   binary representation. A one signalls that day is active. The most
	   significant 1 is the sunday, then followed the monday and so on. Dividing
	   by 10 moves the one to the right. "6 -" because we want to start on the
	   left with the sunday */
	if (((int) (StationsTable[iPos].iDays /
		pow((_REAL) 10.0, (6 - gmtCur->tm_wday))) % 2 == 1) ||
		/* Check also for special case: days are 0000000. This is reserved for
		   DRM test transmissions or irregular transmissions. We define here
		   that these stations are transmitting every day */
		(StationsTable[iPos].iDays == 0))
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

StationsDlg::StationsDlg(QWidget* parent, const char* name, bool modal,
	WFlags f) :	CStationsDlgBase(parent, name, modal, f)
#ifdef HAVE_LIBHAMLIB
	, pRig(NULL)
#endif
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(DRMReceiver.GeomStationsDlg.iXPos,
		DRMReceiver.GeomStationsDlg.iYPos,
		DRMReceiver.GeomStationsDlg.iWSize,
		DRMReceiver.GeomStationsDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#endif

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

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(0, "Station Name");
	ListViewStations->addColumn("Time [UTC]");
	ListViewStations->addColumn("Frequency [kHz]");
	ListViewStations->addColumn("Target");
	ListViewStations->addColumn("Power [KW]");
	ListViewStations->addColumn("Country");
	ListViewStations->addColumn("Site");
	ListViewStations->addColumn("Language");

	/* Init vector for storing the pointer to the list view items */
	vecpListItems.Init(DRMSchedule.GetStationNumber(), NULL);

	/* Set up frequency selector control (QWTCounter control) */
	QwtCounterFrequency->setRange(0.0, 30000.0, 1.0);
	QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
	QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

	/* Init with current setting in log file */
	QwtCounterFrequency->
		setValue(DRMReceiver.GetParameters()->ReceptLog.GetFrequency());

	/* Init UTC time shown with a label control */
	SetUTCTimeLabel();


	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	pViewMenu = new QPopupMenu(this);
	CHECK_PTR(pViewMenu);
	pViewMenu->insertItem("Show &only active stations", this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem("Show &all stations", this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Set stations in list view which are active right now */
	bShowAll = FALSE;
	pViewMenu->setItemChecked(0, TRUE);
	SetStationsView();

	/* Sort list by transmit power (5th column), most powerful on top */
	ListViewStations->setSorting(4, FALSE);
	ListViewStations->sort();


	/* Remote menu  --------------------------------------------------------- */
	pRemoteMenu = new QPopupMenu(this);
	CHECK_PTR(pRemoteMenu);


#ifndef HAVE_LIBHAMLIB
	pRemoteMenu->insertItem("None", this, SLOT(OnRemoteMenu(int)), 0, 0);
#ifdef _WIN32
	pRemoteMenu->insertItem("Winradio G3",
		this, SLOT(OnRemoteMenu(int)), 0, 1);
	pRemoteMenu->insertItem("AOR 7030",
		this, SLOT(OnRemoteMenu(int)), 0, 2);
#endif
	pRemoteMenu->insertItem("Elektor 3/04",
		this, SLOT(OnRemoteMenu(int)), 0, 3);
	pRemoteMenu->insertItem("JRC NRD-535",
		this, SLOT(OnRemoteMenu(int)), 0, 4);
	pRemoteMenu->insertItem("TenTec RX320D",
		this, SLOT(OnRemoteMenu(int)), 0, 5);
#endif


#ifdef HAVE_LIBHAMLIB
	pRemoteMenuOther = new QPopupMenu(this);
	CHECK_PTR(pRemoteMenuOther);

	/* Special DRM front-end list */
	vecSpecDRMRigs.Init(0);

	/* Winradio G3 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_G303,
		"l_ATT=0,l_AGC=3", 0));

	/* AOR 7030 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_AR7030,
		"m_CW=9500,l_IF=-4200,l_AGC=3", 5 /* kHz frequency offset */));

#ifdef RIG_MODEL_ELEKTOR304
	/* Elektor 3/04 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_ELEKTOR304, "", 0));
#endif

	/* JRC NRD 535 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_NRD535,
		"l_CWPITCH=-5000,m_CW=12000,l_IF=-2000,l_AGC=3" /* AGC=slow */,
		3 /* kHz frequency offset */));

	/* TenTec RX320D */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_RX320,
		"l_AF=1,l_AGC=3,m_USB=6000", 0));

	/* TenTec RX340D */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_RX340,
		"l_AF=1,m_USB=16000,l_AGC=3,l_IF=2000",
		-12 /* kHz frequency offset */));


	/* Load all possible front-end remotes in hamlib library */
	rig_load_all_backends();

	/* Get all models which are available. First, the vector for storing the
	   data has to be initialized with zero length! A call-back function is
	   called to return the different rigs */
	veccapsHamlibModels.Init(0);
	const int status = rig_list_foreach(PrintHamlibModelList, this);
	SortHamlibModelList(veccapsHamlibModels); /* Sort list */

	/* Init vector for storing the model IDs with zero length */
	veciModelID.Init(0);

	/* Add menu entry "none" */
	pRemoteMenu->insertItem("None", this, SLOT(OnRemoteMenu(int)), 0, 0);
	veciModelID.Add(0); /* ID 0 for "none" */

	/* Only add menu entries if list was correctly received */
	_BOOLEAN bCheckWasSet = FALSE;
	if (status == RIG_OK)
	{
		for (int j = 0; j < veccapsHamlibModels.Size(); j++)
		{
			/* Create menu objects which belong to an action group. We hope that
			   QT takes care of all the new objects and deletes them... */
			const int iCurModelID = veccapsHamlibModels[j].iModelID;
		
			/* Store model ID */
			veciModelID.Add(iCurModelID);

			int iDummy;
			if (CheckForSpecDRMFE(iCurModelID, iDummy) == TRUE)
			{
				pRemoteMenu->insertItem(
					/* Set menu string. Should look like:
					   [ID] Manuf. Model */
					"[" + QString().setNum(iCurModelID) + "] " +
					veccapsHamlibModels[j].strManufacturer + " " +
					veccapsHamlibModels[j].strModelName,
					this, SLOT(OnRemoteMenu(int)), 0, veciModelID.Size() - 1);

				/* Check for checking and init if necessary */
				if (DRMReceiver.GetHamlibModel() == iCurModelID)
				{
					pRemoteMenu->setItemChecked(veciModelID.Size() - 1, TRUE);
					bCheckWasSet = TRUE;
					InitHamlib(iCurModelID); /* Init hamlib */
				}
			}
			else
			{
				/* "Other" menu */
				pRemoteMenuOther->insertItem(
					/* Set menu string. Should look like:
					   [ID] Manuf. Model (status) */
					"[" + QString().setNum(iCurModelID) + "] " +
					veccapsHamlibModels[j].strManufacturer + " " +
					veccapsHamlibModels[j].strModelName +
					" (" + rig_strstatus(veccapsHamlibModels[j].eRigStatus) +
					")",
					this, SLOT(OnRemoteMenu(int)), 0, veciModelID.Size() - 1);

				/* Check for checking and init if necessary */
				if (DRMReceiver.GetHamlibModel() == iCurModelID)
				{
					pRemoteMenuOther->
						setItemChecked(veciModelID.Size() - 1, TRUE);
					bCheckWasSet = TRUE;
					InitHamlib(iCurModelID); /* Init hamlib */
				}
			}
		}
	}

	/* Only add "other" menu if front-ends are available */
	if (status == RIG_OK)
		pRemoteMenu->insertItem("Other", pRemoteMenuOther);

	if (bCheckWasSet == FALSE)
#endif
	{
		/* No remote as default */
		eWhichRemoteControl = RC_NOREMCNTR;
		pRemoteMenu->setItemChecked(0, TRUE);
	}

	/* Separator */
	pRemoteMenu->insertSeparator();


	/* COM port selection --------------------------------------------------- */
	/* Toggle action for com port selection menu entries */
	agCOMPortSel = new QActionGroup(this, "Com port", TRUE);

	pacMenuCOM1 = new QAction("COM1", "COM1", 0, agCOMPortSel, 0, TRUE);
	pacMenuCOM2 = new QAction("COM2", "COM2", 0, agCOMPortSel, 0, TRUE);
	pacMenuCOM3 = new QAction("COM3", "COM3", 0, agCOMPortSel, 0, TRUE);

	/* Add COM port selection menu group to remote menu */
	agCOMPortSel->addTo(pRemoteMenu);

	/* Default com number */
	eComNumber = CN_COM1;

#ifdef HAVE_LIBHAMLIB
	/* If a config string was set via the command line, we would have to parse
	   the string to identify the port number which was set (if any) which we
	   do not want to do. We simply do not set any check in this case */
	if (DRMReceiver.GetHamlibConf().empty())
#endif
	{
		pacMenuCOM1->setOn(TRUE);
	}


	/* Update menu ---------------------------------------------------------- */
	QPopupMenu* pUpdateMenu = new QPopupMenu(this);
	CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem("&Get Update...", this, SLOT(OnGetUpdate()));


	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&View", pViewMenu);
	pMenu->insertItem("&Remote", pRemoteMenu);
	pMenu->insertItem("&Update", pUpdateMenu); /* String "Udate" used below */
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CStationsDlgBaseLayout->setMenuBar(pMenu);


	/* Register the network protokol (ftp). This is needed for the DRMSchedule
	   download */
	QNetworkProtocol::registerNetworkProtocol("ftp",
		new QNetworkProtocolFactory<QFtp>);


	/* Connections ---------------------------------------------------------- */
	/* Action groups */
	connect(agCOMPortSel, SIGNAL(selected(QAction*)),
		this, SLOT(OnComPortMenu(QAction*)));

	connect(&TimerList, SIGNAL(timeout()),
		this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()),
		this, SLOT(OnTimerUTCLabel()));

	connect(ListViewStations, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));
	connect(&UrlUpdateSchedule, SIGNAL(finished(QNetworkOperation*)),
		this, SLOT(OnUrlFinished(QNetworkOperation*)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));


	/* Set up timer */
	TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
	TimerUTCLabel.start(1000); /* UTC label, every second */
}

StationsDlg::~StationsDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	DRMReceiver.GeomStationsDlg.iXPos = WinGeom.x();
	DRMReceiver.GeomStationsDlg.iYPos = WinGeom.y();
	DRMReceiver.GeomStationsDlg.iHSize = WinGeom.height();
	DRMReceiver.GeomStationsDlg.iWSize = WinGeom.width();

#ifdef HAVE_LIBHAMLIB
	if (pRig != NULL)
	{
		/* close everything */
		rig_close(pRig);
		rig_cleanup(pRig);
	}
#endif
}

void StationsDlg::SetUTCTimeLabel()
{
	/* Get current UTC time */
	time_t ltime;
	time(&ltime);
	struct tm* gmtCur = gmtime(&ltime);

	/* Generate time in format "UTC 12:00" */
	QString strUTCTime = QString().sprintf("UTC %2d:%02d",
		gmtCur->tm_hour, gmtCur->tm_min);

	/* Only apply if time label does not show the correct time */
	if (TextLabelUTCTime->text().compare(strUTCTime))
		TextLabelUTCTime->setText(strUTCTime);
}

void StationsDlg::OnShowStationsMenu(int iID)
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

void StationsDlg::OnGetUpdate()
{
	if (QMessageBox::information(this, "Dream Schedule Update",
		"Dream tries to download the newest DRM schedule\nfrom "
		"www.drm-dx.de (powered by Klaus Schneider).\nYour computer "
		"must be connected to the internet.\n\nThe current file "
		"DRMSchedule.ini will be overwritten.\nDo you want to "
		"continue?",
		QMessageBox::Yes, QMessageBox::No) == 3 /* Yes */)
	{
		/* Try to download the current schedule. Copy the file to the
		   current working directory (which is "QDir().absFilePath(NULL)") */
		UrlUpdateSchedule.copy(QString(DRM_SCHEDULE_UPDATE_FILE),
			QString(QDir().absFilePath(NULL)));
	}
}

void StationsDlg::OnUrlFinished(QNetworkOperation* pNetwOp)
{
	/* Check that pointer points to valid object */
	if (pNetwOp)
	{
		if (pNetwOp->state() == QNetworkProtocol::StFailed)
		{
			/* Something went wrong -> stop all network operations */
			UrlUpdateSchedule.stop();

			/* Notify the user of the failure */
			QMessageBox::information(this, "Dream",
				"Update failed. The following things may caused the failure:\n"
				"\t- the internet connection was not set up properly\n"
				"\t- the server www.drm-dx.de is currently not available\n"
				"\t- the file 'DRMSchedule.ini' could not be written",
				QMessageBox::Ok);
		}

		/* We are interested in the state of the final put function */
		if (pNetwOp->operation() == QNetworkProtocol::OpPut)
		{
			if (pNetwOp->state() == QNetworkProtocol::StDone)
			{
				/* Notify the user that update was successful */
#ifdef _WIN32
				QMessageBox::warning(this, "Dream", "Update successful.\n"
					"Due to network problems with the Windows version of QT, "
					"the Dream software must be restarted after a DRMSchedule "
					"update.\nPlease exit Dream now.",
					"Ok");
#else
				QMessageBox::information(this, "Dream", "Update successful.",
					QMessageBox::Ok);
#endif

				// FIXME: The following lines change the DRMSchedule object.
				// This operation is not thread safe! A mutex should be used!
				/* Read updated ini-file */
				DRMSchedule.ReadStatTabFromFile("DRMSchedule.ini");

				/* Delete all old list view items */
				for (int i = 0; i < vecpListItems.Size(); i++)
					if (vecpListItems[i] != NULL)
						delete vecpListItems[i];

				/* Init vector for storing the pointer to the new list view
				   items */
				vecpListItems.Init(DRMSchedule.GetStationNumber(), NULL);

				/* Update list view */
				SetStationsView();
			}
		}
	}
}

void StationsDlg::showEvent(QShowEvent* pEvent)
{
	/* If number of stations is zero, we assume that the ini file is missing */
	if (DRMSchedule.GetStationNumber() == 0)
	{
		QMessageBox::information(this, "Dream", "The file DRMSchedule.ini could "
			"not be found or contains no data.\nNo stations can be displayed.\n"
			"Try to download this file by using the 'Update' menu.",
			QMessageBox::Ok);
	}
}

void StationsDlg::OnTimerList()
{
	/* Update list view */
	SetStationsView();
}

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
		return QListViewItem::key(column, ascending);
}

void StationsDlg::SetStationsView()
{
	const int iNumStations = DRMSchedule.GetStationNumber();
	_BOOLEAN bListHastChanged = FALSE;

	/* Add new item for each station in list view */
	for (int i = 0; i < iNumStations; i++)
	{
		if (!((bShowAll == FALSE) && (DRMSchedule.IsActive(i) == FALSE)))
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

				/* Check, if station is currently transmitting. If yes, set
				   special pixmap */
				if (DRMSchedule.IsActive(i) == TRUE)
				{
					/* Check for "special case" transmissions */
					if (DRMSchedule.GetItem(i).iDays == 0)
						vecpListItems[i]->setPixmap(0, BitmCubeYellow);
					else
						vecpListItems[i]->setPixmap(0, BitmCubeGreen);
				}
				else
					vecpListItems[i]->setPixmap(0, BitmCubeRed);

				/* Insert this new item in list. The item object is destroyed by
				   the list view control when this is destroyed */
				ListViewStations->insertItem(vecpListItems[i]);

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
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
}

void StationsDlg::OnFreqCntNewValue(double dVal)
{
	/* Set frequency to front-end */
	SetFrequency((int) dVal);

	/* Set selected frequency in log file class */
	DRMReceiver.GetParameters()->ReceptLog.SetFrequency((int) dVal);
}

void StationsDlg::OnListItemClicked(QListViewItem* item)
{
	/* Check that it is a valid item (!= 0) */
	if (item)
	{
		/* Third text of list view item is frequency -> text(2)
		   Set value in frequency counter control QWT. Setting this parameter
		   will emit a "value changed" signal which sets the new frequency.
		   Therefore, here is no call to "SetFrequency()" needed. Also, the
		   frequency is set in the log file, therefore here is no
		   "ReceptLog.SetFrequency()" needed, too */
		QwtCounterFrequency->setValue(QString(item->text(2)).toInt());

		/* Now tell the receiver that the frequency has changed */
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);
	}
}

void StationsDlg::OnRemoteMenu(int iID)
{
#ifdef HAVE_LIBHAMLIB
	/* Set ID if valid */
	const int iNewModelID = veciModelID[iID];

	/* A rig was selected via the menu, delete all previous settings from
	   the command line (if any) */
	DRMReceiver.SetHamlibConf("");

	InitHamlib(iNewModelID);

	/* Take care of check */
	for (int i = 0; i < veciModelID.Size(); i++)
	{
		/* We don't care here that not all IDs are in each menu. If there is a
		   non-valid ID for the menu item, there is simply nothing done */
		pRemoteMenu->setItemChecked(i, i == iID);
		pRemoteMenuOther->setItemChecked(i, i == iID);
	}
#else
	switch (iID)
	{
	case 0:
		eWhichRemoteControl = RC_NOREMCNTR;
		break;

	case 1:
		eWhichRemoteControl = RC_WINRADIO;
		break;

	case 2:
		eWhichRemoteControl = RC_AOR7030;
		break;

	case 3:
		eWhichRemoteControl = RC_ELEKTOR304;
		break;

	case 4:
		eWhichRemoteControl = RC_JRC_NRD535;
		break;

	case 5:
		eWhichRemoteControl = RC_TT_RX320D;
		break;
	}

	/* Taking care of checks in the menu */
	pRemoteMenu->setItemChecked(0, 0 == iID);
	pRemoteMenu->setItemChecked(1, 1 == iID);
	pRemoteMenu->setItemChecked(2, 2 == iID);
	pRemoteMenu->setItemChecked(3, 3 == iID);
	pRemoteMenu->setItemChecked(4, 4 == iID);
	pRemoteMenu->setItemChecked(5, 5 == iID);

	/* Check which receiver require com number and which not. If not required,
	   disable com menu */
	switch (iID)
	{
	case 0:
	case 1:
		agCOMPortSel->setEnabled(FALSE);
		break;

	default:
		agCOMPortSel->setEnabled(TRUE);
		break;
	}
#endif
}

void StationsDlg::OnComPortMenu(QAction* action)
{
	/* We cannot use the switch command for the non constant expressions here */
	if (action == pacMenuCOM1)
		eComNumber = CN_COM1;

	if (action == pacMenuCOM2)
		eComNumber = CN_COM2;

	if (action == pacMenuCOM3)
		eComNumber = CN_COM3;

#ifdef HAVE_LIBHAMLIB
	/* A com port was selected via the menu, delete all previous settings from
	   the command line (if any) */
	DRMReceiver.SetHamlibConf("");

	/* Init hamlib, use current selected model ID */
	InitHamlib(iCurSelModelID);
#endif
}

void StationsDlg::SetFrequency(const int iFreqkHz)
{
#ifdef HAVE_LIBHAMLIB
	SetFrequencyHamlib(iFreqkHz);
#else
	switch (eWhichRemoteControl)
	{
	case RC_WINRADIO:
		SetFrequencyWinradio(iFreqkHz);
		break;

	case RC_AOR7030:
		SetFrequencyAOR7030(eComNumber, iFreqkHz);
		break;

	case RC_ELEKTOR304:
		SetFrequencyElektor304(eComNumber, iFreqkHz);
		break;

	case RC_JRC_NRD535:
		SetFrequencyNRD535(eComNumber, iFreqkHz);
		break;

	case RC_TT_RX320D:
		SetFrequencyRX320D(eComNumber, iFreqkHz);
		break;
	}
#endif
}


#ifdef HAVE_LIBHAMLIB
/******************************************************************************\
* HAMLIB                                                                       *
\******************************************************************************/
/*
	This code is based on patches and example code from Tomi Manninen and
	Stephane Fillod (developer of hamlib)
*/
int StationsDlg::PrintHamlibModelList(const struct rig_caps* caps, void* data)
{
	/* Access data members of class through pointer ((StationsDlg*) data).
	   Store new model in string vector. Use only relevant information */
	((StationsDlg*) data)->veccapsHamlibModels.Add(SDrRigCaps(caps->rig_model,
		caps->mfg_name, caps->model_name, caps->status));

	return 1; /* !=0, we want them all! */
}

void StationsDlg::SortHamlibModelList(CVector<SDrRigCaps>& veccapsHamlibModels)
{
	/* Loop through the array one less than its total cell count */
	const int iEnd = veccapsHamlibModels.Size() - 1;

	for (int i = 0; i < iEnd; i++)
	{
		for (int j = 0; j < iEnd; j++)
		{
			/* Compare the values and switch if necessary */
			if (veccapsHamlibModels[j].iModelID >
				veccapsHamlibModels[j + 1].iModelID)
			{
				const SDrRigCaps instSwap = veccapsHamlibModels[j];
				veccapsHamlibModels[j] = veccapsHamlibModels[j + 1];
				veccapsHamlibModels[j + 1] = instSwap;
			}
		}
	}
}

_BOOLEAN StationsDlg::CheckForSpecDRMFE(const rig_model_t iID, int& iIndex)
{
	_BOOLEAN bIsSpecialDRMrig = FALSE;
	const int iVecSize = vecSpecDRMRigs.Size();

	/* Check for special DRM front-end */
	for (int i = 0; i < iVecSize; i++)
	{
		if (vecSpecDRMRigs[i].iModelID == iID)
		{
			bIsSpecialDRMrig = TRUE;
			iIndex = i;
		}
	}

	return bIsSpecialDRMrig;
}

_BOOLEAN StationsDlg::SetFrequencyHamlib(const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

	/* Prepare actual frequency value for hamlib */
	int iActHamFreq = iFreqkHz;

	/* Check for special rig if there is a frequency offset */
	int iIndex;
	if (CheckForSpecDRMFE(iCurSelModelID, iIndex) == TRUE)
		iActHamFreq += vecSpecDRMRigs[iIndex].iFreqOffs;

	iActHamFreq *= 1000; /* Conversion from kHz to Hz */

	/* Check if rig was opend properly */
	if (pRig != NULL)
	{
		/* Set frequency */
		if (rig_set_freq(pRig, RIG_VFO_CURR, iActHamFreq) == RIG_OK)
			bSucceeded = TRUE;
	}

	return bSucceeded;
}

void StationsDlg::InitHamlib(const rig_model_t newModID)
{
	int ret;

	/* Set value for current selected model ID */
	iCurSelModelID = newModID;

	/* If rig was already open, close it first */
	if (pRig != NULL)
	{
		/* Close everything */
		rig_close(pRig);
		rig_cleanup(pRig);
		pRig = NULL;
	}

	/* Init rig */
	pRig = rig_init(newModID);
	if (pRig == NULL)
		return;

	/* Set config for hamlib. Check if config string was added with command line
	   argument */
	string strHamlibConfig = DRMReceiver.GetHamlibConf();

	if (strHamlibConfig.empty())
	{
		/* Generate string for port selection */
		switch (eComNumber)
		{
		case CN_COM1:
			strHamlibConfig = HAMLIB_CONF_COM1;
			break;

		case CN_COM2:
			strHamlibConfig = HAMLIB_CONF_COM2;
			break;

		case CN_COM3:
			strHamlibConfig = HAMLIB_CONF_COM3;
			break;
		}
	}

	/* Config setup */
	char *p_dup, *p, *q, *n;
	for (p = p_dup = strdup(strHamlibConfig.c_str()); p && *p != '\0'; p = n)
	{
		if ((q = strchr(p, '=')) == NULL)
		{
			/* Malformatted config string */
			cerr << "Malformatted config string" << endl;
			rig_cleanup(pRig);
			pRig = NULL;
			return;
		}
		*q++ = '\0';

		if ((n = strchr(q, ',')) != NULL)
			*n++ = '\0';

		ret = rig_set_conf(pRig, rig_token_lookup(pRig, p), q);
		if (ret != RIG_OK)
		{
			/* Rig set conf failed */
			cerr << "Rig set conf failed: " << rigerror(ret) << endl;
			rig_cleanup(pRig);
			pRig = NULL;
			return;
		}
	}
	if (p_dup)
		free(p_dup);

	/* Open rig */
	if (ret = rig_open(pRig) != RIG_OK)
	{
		/* Fail! */
		cerr << "Rig open failed: " << rigerror(ret) << endl;
		rig_cleanup(pRig);
		pRig = NULL;
	}

	/* Ignore result, some rigs don't have support for this */
	rig_set_powerstat(pRig, RIG_POWER_ON);

	/* Check for special DRM front-end selection */
	int iIndex;
	if (CheckForSpecDRMFE(newModID, iIndex) == TRUE)
	{
		/* Parse special settings */
		char *p_dup, *p, *q, *n;
		for (p = p_dup = strdup(vecSpecDRMRigs[iIndex].strDRMSet.c_str());+
			p && *p != '\0'; p = n)
		{
			if ((q = strchr(p, '=')) == NULL)
			{
				/* Malformatted config string */
				cerr << "Malformatted config string" << endl;
				rig_cleanup(pRig);
				pRig = NULL;
				return;
			}
			*q++ = '\0';

			if ((n = strchr(q, ',')) != NULL)
				*n++ = '\0';

			rmode_t mode;
			setting_t setting;
			value_t val;

			if (p[0] == 'm' && (mode = rig_parse_mode(p + 2)) != RIG_MODE_NONE)
			{
				ret = rig_set_mode(pRig, RIG_VFO_CURR, mode, atoi(q));
				if (ret != RIG_OK)
					cerr << "Rig set mode failed: " << rigerror(ret) << endl;
			}
			else if (p[0] == 'l' && (setting = rig_parse_level(p + 2)) !=
				RIG_LEVEL_NONE)
			{
				if (RIG_LEVEL_IS_FLOAT(setting))
					val.f = atof(q);
				else
					val.i = atoi(q);

				ret = rig_set_level(pRig, RIG_VFO_CURR, setting, val);
				if (ret != RIG_OK)
					cerr << "Rig set level failed: " << rigerror(ret) << endl;
			}
			else if (p[0] == 'f' && (setting = rig_parse_func(p + 2)) !=
				RIG_FUNC_NONE)
			{
				ret = rig_set_func(pRig, RIG_VFO_CURR, setting, atoi(q));
				if (ret != RIG_OK)
					cerr << "Rig set func failed: " << rigerror(ret) << endl;
			}
			else if (p[0] == 'p' && (setting = rig_parse_parm(p + 2)) !=
				RIG_PARM_NONE)
			{
				if (RIG_PARM_IS_FLOAT(setting))
					val.f = atof(q);
				else
					val.i = atoi(q);

				ret = rig_set_parm(pRig, setting, val);
				if (ret != RIG_OK)
					cerr << "Rig set parm failed: " << rigerror(ret) << endl;
			}
			else
				cerr << "Rig unknown setting: " << p << "=" << q << endl;
		}
		if (p_dup)
			free(p_dup);
	}

	/* Set new model ID in receiver object which is needed for init-file */
	DRMReceiver.SetHamlibModel(newModID);
}
#endif


/******************************************************************************\
* Winradio G3																   *
\******************************************************************************/
_BOOLEAN StationsDlg::SetFrequencyWinradio(const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

#ifdef _WIN32
	/* Some type definitions needed for dll access */
	typedef int (__stdcall *FNCOpenRadioDevice)(int iDeviceNum);
	typedef BOOL (__stdcall *FNCCloseRadioDevice)(int hRadio);
	typedef BOOL (__stdcall *FNCG3SetFrequency)(int hRadio, DWORD dwFreq);
	typedef BOOL (__stdcall *FNCSetPower)(int hRadio, BOOL rPower);

	/* Try to load required dll */
	HMODULE dll = LoadLibrary("wrg3api.dll");

	if (dll)
	{
		/* Get process addresses from dll for function access */
		FNCOpenRadioDevice OpenRadioDevice =
			(FNCOpenRadioDevice) GetProcAddress(dll, "OpenRadioDevice");
		FNCCloseRadioDevice CloseRadioDevice =
			(FNCCloseRadioDevice) GetProcAddress(dll, "CloseRadioDevice");
		FNCG3SetFrequency G3SetFrequency =
			(FNCG3SetFrequency) GetProcAddress(dll, "SetFrequency");
		FNCSetPower SetPower = (FNCSetPower) GetProcAddress(dll, "SetPower");

		/* Open Winradio receiver handle */
		int hRadio = OpenRadioDevice(0);

		if (hRadio != 0)
		{
			/* Make sure the receiver is switched on */
			SetPower(hRadio, 1);

			/* Set the frequency ("* 1000", because frequency is in kHz) */
			if (G3SetFrequency(hRadio, (DWORD) (iFreqkHz * 1000)) == TRUE)
				bSucceeded = TRUE;

			/* Close device */
			CloseRadioDevice(hRadio);
		}

		/* Clean up the dll access */
		FreeLibrary(dll);
	}
#endif

	return bSucceeded;
}


/******************************************************************************\
* AOR 7030, serial interface												   *
\******************************************************************************/
_BOOLEAN StationsDlg::SetFrequencyAOR7030(const ECOMNumber eCOMNumber,
										  const int iFreqkHz)
{
/*
	This code is based on a Delphi code by Carsten Knuetter. Special thanks for
	his help on testing the code.
*/
	_BOOLEAN bSucceeded = FALSE;

#ifdef _WIN32
	/* Open serial interface */
	FILE_HANDLE hCom;
	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM2:
		hCom = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM3:
		hCom = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;
	}

	if (hCom != INVALID_HANDLE_VALUE)
	{
		DCB dcb;

		GetCommState(hCom, &dcb); /* first get parameters to fill struct */

		/* Set parameters requried by AOR receiver (All data transfers are at
		   1200 baud, No parity, 8 bits, 1 stop bit (1200 N 8 1). There is no
		   hardware or software flow control other than that inherent in the
		   command structure) */
		dcb.BaudRate = 1200;
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.fParity=FALSE;
		dcb.StopBits = ONESTOPBIT;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fBinary=TRUE;
		dcb.fNull=FALSE;
		dcb.fAbortOnError=TRUE;
		dcb.fDtrControl=DTR_CONTROL_DISABLE;
		dcb.fRtsControl=RTS_CONTROL_DISABLE;

		BOOL bSuccess = SetCommState(hCom, &dcb); /* apply new parameters */

		if (bSuccess == TRUE)
		{
			_BYTE byCurByte;
			DWORD lpNumOfByWr;

			byCurByte = (_BYTE) (0x50); /* Select working mem (page 0) */
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			byCurByte = (_BYTE) (0x31); /* Frequency address = 01AH */
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			byCurByte = (_BYTE) (0x4A);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			/* Convert kHz to steps */
			int iAORFreq = (int) ((_REAL) iFreqkHz * 376.635223 + 0.5);

			/* Exact multiplicand is (2^24) / 44545 */
			byCurByte = (_BYTE) (0x30 + iAORFreq / 1048576);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			/* Write frequency as 6 hex digits */
			iAORFreq = iAORFreq % 1048576;
			byCurByte = (_BYTE) (0x60 + iAORFreq / 65536);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			iAORFreq = iAORFreq % 65536;
			byCurByte = (_BYTE) (0x30 + iAORFreq / 4096);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			iAORFreq = iAORFreq % 4096;
			byCurByte = (_BYTE) (0x60 + iAORFreq / 256);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			iAORFreq = iAORFreq % 256;
			byCurByte = (_BYTE) (0x30 + iAORFreq / 16);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			iAORFreq = iAORFreq % 16;
			byCurByte = (_BYTE) (0x60 + iAORFreq);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			byCurByte = (_BYTE) (0x21);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);

			byCurByte = (_BYTE) (0x2C);
			WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);
		}

		CloseHandle(hCom);
	}
#endif

	return bSucceeded;
}


/******************************************************************************\
* Elektor DRM receiver (3/04), serial interface								   *
\******************************************************************************/
/*
	The Elektor DRM Receiver 3/04 COM interface is based on the Visual Basic
	source code by Burkhard Kainka which can be downloaded from
	www.b-kainka.de
	Linux support is based on a code written by Markus Maerz:
	http://mitglied.lycos.de/markusmaerz/drm
*/
_BOOLEAN StationsDlg::SetFrequencyElektor304(const ECOMNumber eCOMNumber,
											 const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

	/* Open serial interface */
	FILE_HANDLE hCom;

#ifdef _WIN32
	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM2:
		hCom = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM3:
		hCom = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;
	}

	if (hCom != INVALID_HANDLE_VALUE)
	{
#else
	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM2:
		hCom = ::open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM3:
		hCom = ::open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;
	}

	if (hCom != -1)
	{
#endif
		/* Initialization */
		SetOutStateElektor304(hCom, OW_TXD, 0); // TXD 0
		SetOutStateElektor304(hCom, OW_RTS, 0); // RTS 0
		SetOutStateElektor304(hCom, OW_DTR, 0); // DTR 0

		/* Set up frequency */
		const CReal rOscFreq = 50000.0;
		const CReal rIFMixFreq = 454.3;

		const uint32_t iNewFreq = (uint32_t)
			(((CReal) iFreqkHz + rIFMixFreq) / rOscFreq * (CReal) 4294967296.0);

		const uint32_t iFreqLoL = iNewFreq & 0xFF;
		const uint32_t iFreqLoH = (iNewFreq & 0xFF00) >> 8;
		const uint32_t iFreqHiL = (iNewFreq & 0xFF0000) >> 16;
		const uint32_t iFreqHiH = (iNewFreq & 0xFF000000) >> 24;

		OutputElektor304(hCom, 0xF800); // Reset
		OutputElektor304(hCom, (0x3000 + iFreqLoL)); // 4 Bytes to FREQ0
		OutputElektor304(hCom, (0x2100 + iFreqLoH));
		OutputElektor304(hCom, (0x3200 + iFreqHiL));
		OutputElektor304(hCom, (0x2300 + iFreqHiH));
		OutputElektor304(hCom, 0x8000); // Sync
		OutputElektor304(hCom, 0xC000); // Reset end

#ifdef _WIN32
		CloseHandle(hCom);
	}
#else
		::close(hCom);
	}
#endif

	return bSucceeded;
}

void StationsDlg::OutputElektor304(FILE_HANDLE hCom, const uint32_t iData)
{
	SetOutStateElektor304(hCom, OW_TXD, 0); // TXD 0
	SetOutStateElektor304(hCom, OW_DTR, 1); // DTR 1 // CE

	uint32_t iMask = 0x8000;
	for (int n = 0; n < 16; n++)
	{
		if ((iData & iMask) > 0)
			SetOutStateElektor304(hCom, OW_RTS, 0); // RTS 0
		else
			SetOutStateElektor304(hCom, OW_RTS, 1); // RTS 1

		SetOutStateElektor304(hCom, OW_TXD, 1); // TXD 1 // clock
		SetOutStateElektor304(hCom, OW_TXD, 0); // TXD 0

		iMask >>= 1; /* Next bit for masking */
	}

	SetOutStateElektor304(hCom, OW_DTR, 0); // DTR 0
}

void StationsDlg::SetOutStateElektor304(FILE_HANDLE hCom, EOutWire eOutWire,
										_BINARY biState)
{
#ifdef _WIN32
	switch (eOutWire)
	{
	case OW_TXD:
		if (biState == 0)
			EscapeCommFunction(hCom, CLRBREAK); // TXD 0
		else
			EscapeCommFunction(hCom, SETBREAK); // TXD 1
		break;

	case OW_DTR:
		if (biState == 0)
			EscapeCommFunction(hCom, CLRDTR); // DTR 0
		else
			EscapeCommFunction(hCom, SETDTR); // DTR 1
		break;

	case OW_RTS:
		if (biState == 0)
			EscapeCommFunction(hCom, CLRRTS); // RTS 0
		else
			EscapeCommFunction(hCom, SETRTS); // RTS 1
		break;
	}
#else
	int status;

	switch (eOutWire)
	{
	case OW_TXD:
		if (biState == 0)
			ioctl(hCom, TIOCCBRK, 0); // TXD 0
		else
			ioctl(hCom, TIOCSBRK, 0); // TXD 1
		break;

	case OW_DTR:
		ioctl(hCom, TIOCMGET, &status);
		if (biState == 0)
			status &= ~TIOCM_DTR; // DTR 0
		else
			status |= TIOCM_DTR; // DTR 1

		ioctl(hCom, TIOCMSET, &status);
		break;

	case OW_RTS:
		ioctl(hCom, TIOCMGET, &status);
		if (biState == 0)
			status &= ~TIOCM_RTS; // RTS 0
		else
			status |= TIOCM_RTS; // RTS 1

		ioctl(hCom, TIOCMSET, &status);
		break;
	}
#endif

	/* Introduce delay after changing the bit state */
	/* This implementation may not work for very fast computers: TODO */
	int iDelay = 4000;

	while (iDelay > 0)
		iDelay--;
}


/******************************************************************************\
* JRC NRD-535D																   *
\******************************************************************************/
/*
	This code was written by Mark J. Fine (Virginia, USA)
	http://www.fineware-swl.com/drm.html
*/
_BOOLEAN StationsDlg::SetFrequencyNRD535(const ECOMNumber eCOMNumber,
										 const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

	QString sFreqHz;
	const int iFreqHz = (iFreqkHz + 3) * 1000;    // freq in Hz, add 3kHz offset

	sFreqHz = QString("%1").arg(iFreqHz).rightJustify(8,'0',TRUE).
		prepend('F').append('\x0d');

#ifdef _WIN32
  FILE_HANDLE hCom;

	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM2:
		hCom = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM3:
		hCom = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;
	}

	if (hCom != INVALID_HANDLE_VALUE)
	{
		DCB dcb;

		GetCommState(hCom, &dcb); /* first get parameters to fill struct */

		dcb.BaudRate = 4800;
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.fParity=FALSE;
		dcb.StopBits = ONESTOPBIT;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fBinary=TRUE;
		dcb.fNull=FALSE;
		dcb.fAbortOnError=TRUE;
		dcb.fDtrControl=DTR_CONTROL_DISABLE;
		dcb.fRtsControl=RTS_CONTROL_DISABLE;

		BOOL bSuccess = SetCommState(hCom, &dcb); /* apply new parameters */

		if (bSuccess == TRUE)
		{
			DWORD lpNumOfByWr;

			PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);        // flush buffers

			WriteFile(hCom, "H1\x0d", 3, &lpNumOfByWr, NULL);      // send control on
			//Sleep(7);

			WriteFile(hCom, "U2-5000\x0d", 8, &lpNumOfByWr, NULL); // set BFO to -5kHz
			//Sleep(17);

			WriteFile(hCom, "D1\x0d", 3, &lpNumOfByWr, NULL);      // set mode to CW
			//Sleep(7);

			WriteFile(hCom, "B3\x0d", 3, &lpNumOfByWr, NULL);      // set BW to AUX
			//Sleep(7);

			WriteFile(hCom, "P-2000\x0d", 7, &lpNumOfByWr, NULL);  // set PBS to -2kHz
			//Sleep(15);

			WriteFile(hCom, "G0\x0d", 3, &lpNumOfByWr, NULL);      // set AGC slow
			//Sleep(7);

			WriteFile(hCom, sFreqHz.ascii(), 10, &lpNumOfByWr, NULL);// send frequency cmd
			//Sleep(21);

			WriteFile(hCom, "H0\x0d", 3, &lpNumOfByWr, NULL);      // send control off
			//Sleep(7);
		}

		CloseHandle(hCom);
		bSucceeded = TRUE;
	}
#else
	FILE_HANDLE hCom = -1;

	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM2:
		hCom = ::open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM3:
		hCom = ::open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;
	}

	if (hCom >= 0)
	{
		struct termios tty;

		tcgetattr(hCom, &tty);                   //get current attributes

		cfsetospeed(&tty, (speed_t)B4800);        // set baud rate to 4800
		cfsetispeed(&tty, (speed_t)B4800);
		tty.c_lflag = 0;
		tty.c_oflag = 0;                          // set OutX off;
		tty.c_iflag = IGNBRK;                     // set no echo mode
		tty.c_iflag &= ~(IXON|IXOFF|IXANY);       // set InX off
		tty.c_cflag = (tty.c_cflag & ~CSIZE)|CS8; // set 8 bits
		tty.c_cflag |= CLOCAL | CREAD;            // set no wait
		tty.c_cflag &= ~(PARENB | PARODD);        // set no parity
		tty.c_cflag &= ~CSTOPB;                   // set 1 stop bit
		tty.c_cflag &= ~HUPCL;                    // set no hangup
		tty.c_cflag &= ~CRTSCTS;                  // set DCD Flow
		tty.c_cc[VMIN] = 1;                       // set timeout constants
		tty.c_cc[VTIME] = 5;

		int bSuccess = tcsetattr(hCom, TCSANOW, &tty); //set new parms

		if (bSuccess == 0)
		{
			tcflush(hCom, TCIOFLUSH);                // flush buffers

			write(hCom, "H1\x0d", 3);                // send control on
			usleep(6250);

			write(hCom, "U2-5000\x0d", 8);           // set BFO to -5kHz
			usleep(16667);

			write(hCom, "D1\x0d", 3);                // set mode to CW
			usleep(6250);

			write(hCom, "B3\x0d", 3);                // set BW to AUX
			usleep(6250);

			write(hCom, "P-2000\x0d", 7);            // set PBS to -2kHz
			usleep(14584);

			write(hCom, "G0\x0d", 3);                // set AGC slow
			usleep(6250);

			write(hCom, sFreqHz.ascii(), 10);        // send frequency cmd
			usleep(20834);

			write(hCom, "H0\x0d", 3);                // send control off
			usleep(6250);
		}

		::close(hCom);
		bSucceeded = TRUE;
	}
#endif

  return bSucceeded;
}


/******************************************************************************\
* TenTec RX320D																   *
\******************************************************************************/
/*
	This code was written by Mark J. Fine (Virginia, USA)
	http://www.fineware-swl.com/drm.html
*/
_BOOLEAN StationsDlg::SetFrequencyRX320D(const ECOMNumber eCOMNumber,
										 const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;
	_BYTE freqAry[8];
	int i;

	QString sFreqHz;
	double rFreq = (iFreqkHz / 1000.0) - 0.00125;
	int cVal = (int) (rFreq * 400.0);
	int fVal = (int) (((rFreq * 400.0) - cVal) * 13650.0);
	cVal += 18000;
	freqAry[0] = 0x4E;
	freqAry[1] = (uint)(cVal / 256);
	freqAry[2] = (uint)(cVal % 256);
	freqAry[3] = (uint)(fVal / 256);
	freqAry[4] = (uint)(fVal % 256);
	freqAry[5] = 0x77;
	freqAry[6] = 0x70;
	freqAry[7] = 0x0d;

#ifdef _WIN32
	FILE_HANDLE hCom;

	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = CreateFile("COM1", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM2:
		hCom = CreateFile("COM2", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;

	case CN_COM3:
		hCom = CreateFile("COM3", GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
		break;
	}

	if (hCom != INVALID_HANDLE_VALUE)
	{
		DCB dcb;

		GetCommState(hCom, &dcb); /* first get parameters to fill struct */

		dcb.BaudRate = 1200;
		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.fParity=FALSE;
		dcb.StopBits = ONESTOPBIT;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.fDsrSensitivity = FALSE;
		dcb.fBinary=TRUE;
		dcb.fNull=FALSE;
		dcb.fAbortOnError=TRUE;
		dcb.fDtrControl=DTR_CONTROL_DISABLE;
		dcb.fRtsControl=RTS_CONTROL_DISABLE;

		BOOL bSuccess = SetCommState(hCom, &dcb); /* apply new parameters */

		if (bSuccess == TRUE)
		{
			_BYTE byCurByte;
			DWORD lpNumOfByWr;

			PurgeComm(hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);        // flush buffers

			WriteFile(hCom, "A\x7f\x3f\x0d", 4, &lpNumOfByWr, NULL); // mute line out
			//Sleep(34);

			WriteFile(hCom, "V\x7f\x3f\x0d", 4, &lpNumOfByWr, NULL); // mute speaker
			//Sleep(34);

			WriteFile(hCom, "G1\x0d", 3, &lpNumOfByWr, NULL);        // set AGC slow
			//Sleep(25);

			WriteFile(hCom, "W\x00\x0d", 3, &lpNumOfByWr, NULL);     // set BW to 6kHz
			//Sleep(25);

			for (i = 0; i < 8; i++)
			{
				byCurByte = freqAry[i];
				WriteFile(hCom, &byCurByte, 1, &lpNumOfByWr, NULL);    // set freq byte
				//Sleep(9);
			}
		}

		CloseHandle(hCom);
		bSucceeded = TRUE;
	}
#else
	FILE_HANDLE hCom = -1;

	switch (eCOMNumber)
	{
	case CN_COM1:
		hCom = ::open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM2:
		hCom = ::open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
		break;

	case CN_COM3:
		hCom = ::open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);
		break;
	}

	if (hCom >= 0)
	{
		struct termios tty;

		tcgetattr(hCom, &tty);                   //get current attributes

		cfsetospeed(&tty, (speed_t)B1200);        // set baud rate to 1200
		cfsetispeed(&tty, (speed_t)B1200);
		tty.c_lflag = 0;
		tty.c_oflag = 0;                          // set OutX off;
		tty.c_iflag = IGNBRK;                     // set no echo mode
		tty.c_iflag &= ~(IXON|IXOFF|IXANY);       // set InX off
		tty.c_cflag = (tty.c_cflag & ~CSIZE)|CS8; // set 8 bits
		tty.c_cflag |= CLOCAL | CREAD;            // set no wait
		tty.c_cflag &= ~(PARENB | PARODD);        // set no parity
		tty.c_cflag &= ~CSTOPB;                   // set 1 stop bit
		tty.c_cflag &= ~HUPCL;                    // set no hangup
		tty.c_cflag &= ~CRTSCTS;                  // set DCD Flow
		tty.c_cc[VMIN] = 1;                       // set timeout constants
		tty.c_cc[VTIME] = 5;

		int bSuccess = tcsetattr(hCom, TCSANOW, &tty); //set new parms

		if (bSuccess == 0)
		{
			_BYTE byCurByte;
			tcflush(hCom, TCIOFLUSH);                // flush buffers

			write(hCom, "A\x7f\x3f\x0d", 4);         // mute line out
			usleep(33334);

			write(hCom, "V\x7f\x3f\x0d", 4);         // mute speaker
			usleep(33334);

			write(hCom, "G1\x0d", 3);                // set AGC slow
			usleep(25000);

			write(hCom, "W\x00\x0d", 3);             // set BW to 6kHz
			usleep(25000);

			for (i = 0; i < 8; i++)
			{
				byCurByte = freqAry[i];
				write(hCom, &byCurByte, 1);            // set freq byte
				usleep(8334);
			}
		}

		::close(hCom);
		bSucceeded = TRUE;
	}
#endif

  return bSucceeded;
}

void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	QWhatsThis::add(ListViewStations,
		"<b>Stations List:</b> In the stations list view all DRM stations "
		"which are stored in the DRMSchedule.ini file are shown. It is "
		"possible to show only active stations by changing a setting in "
		"the 'view' menu. The color of the cube on the left of a menu "
		"item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column.<br>By clicking on a menu item, a remote front-end can "
		"be automatically switched to the current frequency and the "
		"Dream software is reset to a new acquisition (to speed up the "
		"synchronization process). Also, the log-file frequency edit "
		"is automatically updated.");

	/* Frequency Counter */
	QWhatsThis::add(QwtCounterFrequency,
		"<b>Frequency Counter:</b> The current frequency value can be "
		"changed by using this counter. The tuning steps are 100 kHz "
		"for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically.");

	/* UTC time label */
	QWhatsThis::add(TextLabelUTCTime,
		"<b>UTC Time:</b> Shows the current Coordinated Universal Time (UTC) "
		"which is also known as Greenwich Mean Time (GMT).");
}
