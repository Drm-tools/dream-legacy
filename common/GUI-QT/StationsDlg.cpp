/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Stephane Fillod, Tomi Manninen
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

#ifdef HAVE_LIBHAMLIB
	/* Init progress bar for input s-meter */
	ProgrSigStrength->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
	ProgrSigStrength->setOrientation(QwtThermo::Horizontal, QwtThermo::Top);
	ProgrSigStrength->setAlarmLevel(S_METER_THERMO_ALARM);
	ProgrSigStrength->setAlarmColor(QColor(255, 0, 0));
	ProgrSigStrength->setScale(S_METER_THERMO_MIN, S_METER_THERMO_MAX, 10.0);
	EnableSMeter(FALSE); /* disable for initialization */
#else
	/* s-meter only implemented for hamlib */
	ProgrSigStrength->hide();
	TextLabelSMeter->hide();
#endif

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(0, tr("Station Name"));
	ListViewStations->addColumn(tr("Time [UTC]"));
	ListViewStations->addColumn(tr("Frequency [kHz]"));
	ListViewStations->addColumn(tr("Target"));
	ListViewStations->addColumn(tr("Power [KW]"));
	ListViewStations->addColumn(tr("Country"));
	ListViewStations->addColumn(tr("Site"));
	ListViewStations->addColumn(tr("Language"));

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
	pViewMenu->insertItem(tr("Show &only active stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem(tr("Show &all stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Set stations in list view which are active right now */
	bShowAll = FALSE;
	pViewMenu->setItemChecked(0, TRUE);
	SetStationsView();

	/* Sort list by transmit power (5th column), most powerful on top */
	ListViewStations->setSorting(4, FALSE);
	ListViewStations->sort();


#ifdef HAVE_LIBHAMLIB
	/* If config string is empty, set default COM port 1 */
	if (DRMReceiver.GetHamlibConf().empty())
		DRMReceiver.SetHamlibConf(HAMLIB_CONF_COM1);


	/* Remote menu  --------------------------------------------------------- */
	pRemoteMenu = new QPopupMenu(this);
	CHECK_PTR(pRemoteMenu);

	pRemoteMenuOther = new QPopupMenu(this);
	CHECK_PTR(pRemoteMenuOther);

	/* Special DRM front-end list */
	vecSpecDRMRigs.Init(0);

#ifdef RIG_MODEL_G303
	/* Winradio G3 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_G303,
		"l_ATT=0,l_AGC=3", 0,
		"l_ATT=0,l_AGC=3"));
#endif

#ifdef RIG_MODEL_AR7030
	/* AOR 7030 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_AR7030,
		"m_CW=9500,l_IF=-4200,l_AGC=3", 5 /* kHz frequency offset */,
		"l_AGC=3"));
#endif

#ifdef RIG_MODEL_ELEKTOR304
	/* Elektor 3/04 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_ELEKTOR304, "", 0, ""));
#endif

#ifdef RIG_MODEL_NRD535
	/* JRC NRD 535 */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_NRD535,
		"l_CWPITCH=-5000,m_CW=12000,l_IF=-2000,l_AGC=3" /* AGC=slow */,
		3 /* kHz frequency offset */,
		"l_AGC=3"));
#endif

#ifdef RIG_MODEL_RX320
	/* TenTec RX320D */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_RX320,
		"l_AF=1,l_AGC=3,m_AM=6000", 0,
		"l_AGC=3"));
#endif

#ifdef RIG_MODEL_RX340
	/* TenTec RX340D */
	vecSpecDRMRigs.Add(CSpecDRMRig(RIG_MODEL_RX340,
		"l_AF=1,m_USB=16000,l_AGC=3,l_IF=2000",
		-12 /* kHz frequency offset */,
		"l_AGC=3"));
#endif


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
	pRemoteMenu->insertItem(tr("None"), this, SLOT(OnRemoteMenu(int)), 0, 0);
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
		pRemoteMenu->insertItem(tr("Other"), pRemoteMenuOther);

	if (bCheckWasSet == FALSE)
	{
		/* No remote as default */
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

	/* Try to get the COM port number from the hamlib configure string */
	if (DRMReceiver.GetHamlibConf() == HAMLIB_CONF_COM1)
		pacMenuCOM1->setOn(TRUE);

	if (DRMReceiver.GetHamlibConf() == HAMLIB_CONF_COM2)
		pacMenuCOM2->setOn(TRUE);

	if (DRMReceiver.GetHamlibConf() == HAMLIB_CONF_COM3)
		pacMenuCOM3->setOn(TRUE);


	/* Other settings ------------------------------------------------------- */
	/* Enable s-meter */
	/* Separator */
	pRemoteMenu->insertSeparator();

	const int iSMeterMenuID = pRemoteMenu->insertItem(tr("Enable S-Meter"),
		this, SLOT(OnSMeterMenu(int)), 0);

	/* Set check */
	if (DRMReceiver.GetEnableSMeter() == TRUE)
		pRemoteMenu->setItemChecked(iSMeterMenuID, 1);

	/* Enable special settings for rigs */
	/* Separator */
	pRemoteMenu->insertSeparator();

	const int iModRigMenuID = pRemoteMenu->insertItem(tr("With DRM "
		"Modification"), this, SLOT(OnModRigMenu(int)), 0);

	/* Set check */
	if (DRMReceiver.GetEnableModRigSettings() == TRUE)
		pRemoteMenu->setItemChecked(iModRigMenuID, 1);
#endif


	/* Update menu ---------------------------------------------------------- */
	QPopupMenu* pUpdateMenu = new QPopupMenu(this);
	CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem(tr("&Get Update..."), this, SLOT(OnGetUpdate()));


	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), pViewMenu);
#ifdef HAVE_LIBHAMLIB
	pMenu->insertItem(tr("&Remote"), pRemoteMenu);
#endif
	pMenu->insertItem(tr("&Update"), pUpdateMenu); /* String "Udate" used below */
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CStationsDlgBaseLayout->setMenuBar(pMenu);


	/* Register the network protokol (ftp). This is needed for the DRMSchedule
	   download */
	QNetworkProtocol::registerNetworkProtocol("ftp",
		new QNetworkProtocolFactory<QFtp>);


	/* Connections ---------------------------------------------------------- */
#ifdef HAVE_LIBHAMLIB
	/* Action group */
	connect(agCOMPortSel, SIGNAL(selected(QAction*)),
		this, SLOT(OnComPortMenu(QAction*)));
#endif

	connect(&TimerList, SIGNAL(timeout()),
		this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()),
		this, SLOT(OnTimerUTCLabel()));
	connect(&TimerSMeter, SIGNAL(timeout()),
		this, SLOT(OnTimerSMeter()));

	connect(ListViewStations, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));
	connect(&UrlUpdateSchedule, SIGNAL(finished(QNetworkOperation*)),
		this, SLOT(OnUrlFinished(QNetworkOperation*)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));


	/* Set up timers */
	TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);
	TimerSMeter.start(GUI_TIMER_S_METER);
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
				tr("Update failed. The following things may caused the "
				"failure:\n"
				"\t- the internet connection was not set up properly\n"
				"\t- the server www.drm-dx.de is currently not available\n"
				"\t- the file 'DRMSchedule.ini' could not be written"),
				QMessageBox::Ok);
		}

		/* We are interested in the state of the final put function */
		if (pNetwOp->operation() == QNetworkProtocol::OpPut)
		{
			if (pNetwOp->state() == QNetworkProtocol::StDone)
			{
				/* Notify the user that update was successful */
#ifdef _WIN32
				QMessageBox::warning(this, "Dream", tr("Update successful.\n"
					"Due to network problems with the Windows version of QT, "
					"the Dream software must be restarted after a DRMSchedule "
					"update.\nPlease exit Dream now."),
					"Ok");
#else
				QMessageBox::information(this, "Dream",
					tr("Update successful."), QMessageBox::Ok);
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
		QMessageBox::information(this, "Dream", tr("The file DRMSchedule.ini "
			"could not be found or contains no data.\nNo stations can be "
			"displayed.\nTry to download this file by using the 'Update' "
			"menu."), QMessageBox::Ok);
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
#ifdef HAVE_LIBHAMLIB
	/* Set frequency to front-end */
	SetFrequency((int) dVal);
#endif

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

void StationsDlg::OnSMeterMenu(int iID)
{
#ifdef HAVE_LIBHAMLIB
	if (pRemoteMenu->isItemChecked(iID))
	{
		pRemoteMenu->setItemChecked(iID, FALSE);
		DRMReceiver.SetEnableSMeter(FALSE);
	}
	else
	{
		pRemoteMenu->setItemChecked(iID, TRUE);
		DRMReceiver.SetEnableSMeter(TRUE);
	}

	/* Init hamlib, use current selected model ID */
	InitHamlib(iCurSelModelID);
#endif
}

void StationsDlg::OnModRigMenu(int iID)
{
#ifdef HAVE_LIBHAMLIB
	if (pRemoteMenu->isItemChecked(iID))
	{
		pRemoteMenu->setItemChecked(iID, FALSE);
		DRMReceiver.SetEnableModRigSettings(FALSE);
	}
	else
	{
		pRemoteMenu->setItemChecked(iID, TRUE);
		DRMReceiver.SetEnableModRigSettings(TRUE);
	}

	/* Init hamlib, use current selected model ID */
	InitHamlib(iCurSelModelID);
#endif
}

void StationsDlg::OnRemoteMenu(int iID)
{
#ifdef HAVE_LIBHAMLIB
	/* Set ID if valid */
	const int iNewModelID = veciModelID[iID];

	InitHamlib(iNewModelID);

	/* Take care of check */
	for (int i = 0; i < veciModelID.Size(); i++)
	{
		/* We don't care here that not all IDs are in each menu. If there is a
		   non-valid ID for the menu item, there is simply nothing done */
		pRemoteMenu->setItemChecked(i, i == iID);
		pRemoteMenuOther->setItemChecked(i, i == iID);
	}
#endif
}

void StationsDlg::OnComPortMenu(QAction* action)
{
#ifdef HAVE_LIBHAMLIB
	/* We cannot use the switch command for the non constant expressions here */
	if (action == pacMenuCOM1)
		DRMReceiver.SetHamlibConf(HAMLIB_CONF_COM1);

	if (action == pacMenuCOM2)
		DRMReceiver.SetHamlibConf(HAMLIB_CONF_COM2);

	if (action == pacMenuCOM3)
		DRMReceiver.SetHamlibConf(HAMLIB_CONF_COM3);

	/* Init hamlib, use current selected model ID */
	InitHamlib(iCurSelModelID);
#endif
}

void StationsDlg::OnTimerSMeter()
{
#ifdef HAVE_LIBHAMLIB
	if ((bSMeterEnabled == TRUE) && (pRig != 0))
	{
		value_t tValue;

		const int iRetVal =
			rig_get_level(pRig, RIG_VFO_CURR, RIG_LEVEL_STRENGTH, &tValue);

		if (!iRetVal)
			ProgrSigStrength->setValue((_REAL) tValue.i);

		/* If a time-out happened, do not update s-meter anymore (disable it) */
		if (iRetVal == -RIG_ETIMEOUT)
			EnableSMeter(FALSE);
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
void StationsDlg::EnableSMeter(const _BOOLEAN bStatus)
{
	/* Both, GUI "enabled" and hamlib "enabled" must be fullfilled before
	   s-meter is used */
	if ((bStatus == TRUE) && (DRMReceiver.GetEnableSMeter() == TRUE))
	{
		/* Init progress bar for input s-meter */
		ProgrSigStrength->setAlarmEnabled(TRUE);
		ProgrSigStrength->setValue(S_METER_THERMO_MIN);
		ProgrSigStrength->setFillColor(QColor(0, 190, 0));

		ProgrSigStrength->setEnabled(TRUE);
		TextLabelSMeter->setEnabled(TRUE);

		bSMeterEnabled = TRUE;
	}
	else
	{
		/* Set s-meter control in "disabled" status */
		ProgrSigStrength->setAlarmEnabled(FALSE);
		ProgrSigStrength->setValue(S_METER_THERMO_MAX);
		ProgrSigStrength->setFillColor(palette().disabled().light());

		ProgrSigStrength->setEnabled(FALSE);
		TextLabelSMeter->setEnabled(FALSE);

		bSMeterEnabled = FALSE;
	}
}

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

_BOOLEAN StationsDlg::SetFrequency(const int iFreqkHz)
{
	_BOOLEAN bSucceeded = FALSE;

	/* Prepare actual frequency value for hamlib */
	int iActHamFreq = iFreqkHz;

	/* Check if we have a modified or not modified receiver */
	if (DRMReceiver.GetEnableModRigSettings() == FALSE)
	{
		/* Check for special rig if there is a frequency offset */
		int iIndex;
		if (CheckForSpecDRMFE(iCurSelModelID, iIndex) == TRUE)
			iActHamFreq += vecSpecDRMRigs[iIndex].iFreqOffs;
	}

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

try
{
	/* Set value for current selected model ID */
	iCurSelModelID = newModID;

	/* Set new model ID in receiver object which is needed for init-file */
	DRMReceiver.SetHamlibModel(newModID);

	/* If rig was already open, close it first */
	if (pRig != NULL)
	{
		/* Close everything */
		rig_close(pRig);
		rig_cleanup(pRig);
		pRig = NULL;
	}

	if (newModID == 0)
		throw CGenErr(tr("No rig model ID selected.").latin1());

	/* Init rig */
	pRig = rig_init(newModID);
	if (pRig == NULL)
		throw CGenErr(tr("Initialization of hamlib failed.").latin1());

	/* Set config for hamlib */
	string strHamlibConfig = DRMReceiver.GetHamlibConf();

	/* Config setup */
	char *p_dup, *p, *q, *n;
	for (p = p_dup = strdup(strHamlibConfig.c_str()); p && *p != '\0'; p = n)
	{
		if ((q = strchr(p, '=')) == NULL)
		{
			rig_cleanup(pRig);
			pRig = NULL;

			throw CGenErr(tr("Malformatted config string.").latin1());
		}
		*q++ = '\0';

		if ((n = strchr(q, ',')) != NULL)
			*n++ = '\0';

		ret = rig_set_conf(pRig, rig_token_lookup(pRig, p), q);
		if (ret != RIG_OK)
		{
			rig_cleanup(pRig);
			pRig = NULL;

			throw CGenErr(tr("Rig set conf failed.").latin1());
		}
	}
	if (p_dup)
		free(p_dup);

	/* Open rig */
	if (ret = rig_open(pRig) != RIG_OK)
	{
		/* Fail! */
		rig_cleanup(pRig);
		pRig = NULL;

		throw CGenErr(tr("Rig open failed.").latin1());
	}

	/* Ignore result, some rigs don't have support for this */
	rig_set_powerstat(pRig, RIG_POWER_ON);

	/* Check for special DRM front-end selection */
	int iIndex;
	if (CheckForSpecDRMFE(newModID, iIndex) == TRUE)
	{
		/* Get correct parameter string */
		string strSet;
		if (DRMReceiver.GetEnableModRigSettings() == TRUE)
			strSet = vecSpecDRMRigs[iIndex].strDRMSetMod;
		else
			strSet = vecSpecDRMRigs[iIndex].strDRMSetNoMod;

		/* Parse special settings */
		char *p_dup, *p, *q, *n;
		for (p = p_dup = strdup(strSet.c_str()); p && *p != '\0'; p = n)
		{
			if ((q = strchr(p, '=')) == NULL)
			{
				/* Malformatted config string */
				rig_cleanup(pRig);
				pRig = NULL;

				throw CGenErr(tr("Malformatted config string.").latin1());
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
				{
					cerr << tr("Rig set mode failed: ") << rigerror(ret) <<
					endl;
				}
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
				{
					cerr << tr("Rig set level failed: ") << rigerror(ret) <<
					endl;
				}
			}
			else if (p[0] == 'f' && (setting = rig_parse_func(p + 2)) !=
				RIG_FUNC_NONE)
			{
				ret = rig_set_func(pRig, RIG_VFO_CURR, setting, atoi(q));
				if (ret != RIG_OK)
				{
					cerr << tr("Rig set func failed: ") << rigerror(ret) <<
					endl;
				}
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
				{
					cerr << tr("Rig set parm failed: ") << rigerror(ret) <<
					endl;
				}
			}
			else
				cerr << tr("Rig unknown setting: ") << p << "=" << q << endl;
		}
		if (p_dup)
			free(p_dup);
	}

	/* Check if s-meter capabilities are available */
	if (pRig != NULL)
	{
		/* Check if s-meter can be used. Disable GUI control if not */
		if (rig_has_get_level(pRig, RIG_LEVEL_STRENGTH))
			EnableSMeter(TRUE);
		else
			EnableSMeter(FALSE);
	}
}

catch (CGenErr GenErr)
{
	/* Print error message */
	cerr << GenErr.strError << endl;

	/* Disable s-meter controls */
	EnableSMeter(FALSE);
}
}
#endif


void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	QWhatsThis::add(ListViewStations,
		"<b>" + tr("Stations List:") + "</b> " + tr("In the stations list "
		"view all DRM stations which are stored in the DRMSchedule.ini file "
		"are shown. It is possible to show only active stations by changing a "
		"setting in the 'view' menu. The color of the cube on the left of a "
		"menu item shows the current status of the DRM transmission. A green "
		"box shows that the transmission takes place right now, a "
		"yellow cube shows that this is a test transmission and with a "
		"red cube it is shown that the transmission is offline.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column.<br>By clicking on a menu item, a remote front-end can "
		"be automatically switched to the current frequency and the "
		"Dream software is reset to a new acquisition (to speed up the "
		"synchronization process). Also, the log-file frequency edit "
		"is automatically updated."));

	/* Frequency Counter */
	QWhatsThis::add(QwtCounterFrequency,
		"<b>" + tr("Frequency Counter:") + "</b> " + tr("The current frequency "
		"value can be changed by using this counter. The tuning steps are "
		"100 kHz for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically."));

	/* UTC time label */
	QWhatsThis::add(TextLabelUTCTime,
		"<b>" + tr("UTC Time:") + "</b> " + tr("Shows the current Coordinated "
		"Universal Time (UTC) which is also known as Greenwich Mean Time "
		"(GMT)."));

#ifdef HAVE_LIBHAMLIB
	/* S-meter */
	const QString strSMeter =
		"<b>" + tr("Signal-Meter:") + "</b> " + tr("Shows the signal strength "
		"level in dB relative to S9.") + "<br>" + tr("Note that not all "
		"front-ends controlled by hamlib support this feature. If the s-meter "
		"is not available, the controls are disabled.");

	QWhatsThis::add(TextLabelSMeter, strSMeter);
	QWhatsThis::add(ProgrSigStrength, strSMeter);
#endif
}
