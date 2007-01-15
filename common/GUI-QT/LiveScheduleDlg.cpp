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
	string s = "0000000";
	for(int i=0; i<7; i++)
	{
		if((1<<(6-i)) & iVal)
			s[i]++;
	}
	return s;
}

QString LiveScheduleDlg::ExtractTime(const int iTimeStart, const int iDuration)
{
string sStartHours = "";
string sStartMinutes = "";
string sStopHours = "";
string sStopMinutes = "";
string sDays = "";
string sResult = "";

if ((iTimeStart == 0) && (iDuration == 0))
	return "";

	/* Start time */
	int iStartMinutes = iTimeStart % 60;
	int iStartHours = iTimeStart / 60;
		
	if (iStartMinutes < 10)
		sStartMinutes = "0";

	if (iStartHours < 10)
		sStartHours = "0";

	sStartHours += QString::number(iStartHours).latin1();
	sStartMinutes += QString::number(iStartMinutes).latin1();

	/* Stop time */	
	_BOOLEAN bAllWeek24Hours = FALSE;
    const int iTimeStop = iTimeStart + iDuration;

	int iStopMinutes = iTimeStop % 60;
	int iStopHours = iTimeStop / 60;
		
	if (iStopMinutes < 10)
		sStopMinutes = "0";

	if (iStopHours>24)
	{
		int iDays = iStopHours / 24;

		if (iDays == 7)
			/* All the week */
			bAllWeek24Hours = TRUE;
		else
		{		
			/* Add information about days duration */
			if (iDays > 1)
			{
				sDays += " (";
				sDays += QString::number(iDays).latin1(); 
				sDays += " days)";
			}		
			iStopHours = iStopHours % 24;
		}
	}

	if (iStopHours < 10)
		sStopHours = "0";

	sStopHours += QString::number(iStopHours).latin1();
	sStopMinutes += QString::number(iStopMinutes).latin1();

	if (bAllWeek24Hours == TRUE)
		sResult = "24 hours, 7 days a week";
	else	
	{
		sResult = sStartHours + ":" + sStartMinutes
			+ "-" + sStopHours + ":" + sStopMinutes + sDays;
	}

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

QString CDRMLiveSchedule::ExtractFirstDigits(const QString s, const int iDigits)
{
QString sVal;
QChar ch;
_BOOLEAN bStop;

	sVal = "";
	bStop = FALSE;

	/* scan the string for extract the first n digits */
	for (int i = 0; i <= iDigits - 1; i++)
	{
		if (bStop == FALSE)
		{
			ch = s.at(i);
			if (ch.isDigit() == TRUE)
				sVal = sVal + ch;
			else
				bStop = TRUE;
		}
	}
	return sVal;
}

void CDRMLiveSchedule::SetReceiverCoordinates(const string strLatitude, const string strLongitude)
{
int iLatitude;
int iLongitude;

QString sVal;

	/* parse the latitude and longitude string stored into Dream settings for
		extract local latitude degrees and longitude degrees */

	bCheckCoordinates = FALSE;

	const QString strCurLat = QString(strLatitude.c_str());
	const QString strCurLong = QString(strLongitude.c_str());

	if ((strCurLat != "") && (strCurLong != "")) 
	{
		/* latitude max 2 digits */
		sVal = ExtractFirstDigits(strCurLat, 2);

		if (sVal != "")
		{
			iLatitude = sVal.toInt();

			if (strCurLat.contains('S') > 0)
				iLatitude = -1 * iLatitude;

			if ((iLatitude <= 90) && (iLatitude >= -90))
			{
				/* longitude max 3 digits */

				sVal = ExtractFirstDigits(strCurLong, 3);

				if (sVal != "")
				{
					iLongitude = sVal.toInt();

					if (strCurLong.contains('W') > 0)
						iLongitude = -1 * iLongitude;
				
					if ((iLongitude >= -180) && (iLongitude <= 179))
					{
						iReceiverLatitude = iLatitude;
						iReceiverLongitude = iLongitude;

						bCheckCoordinates = TRUE;
					}
				}
			}
		}
	}
}

void CDRMLiveSchedule::DecodeTargets(const int iRegionID, const CVector<CParameter::CAltFreqRegion> vecAltFreqRegions
										, QString& strRegions, _BOOLEAN& bIntoTargetArea)
{
int iCIRAF;

	strRegions = "";
	
	if (iRegionID > 0)
	{
		int k = 0;
		while (k < vecAltFreqRegions.Size())
		{
			if (vecAltFreqRegions[k].iRegionID == iRegionID)
			{
				const int iLatitude = vecAltFreqRegions[k].iLatitude;
				const int iLongitude = vecAltFreqRegions[k].iLongitude;

				const int iLatitudeEx = vecAltFreqRegions[k].iLatitudeEx;
				const int iLongitudeEx = vecAltFreqRegions[k].iLongitudeEx;

				int iCIRAFSize = vecAltFreqRegions[k].veciCIRAFZones.Size();

				if (iCIRAFSize > 0)
				{
					/* Targets */					
					for (int kk = 0; kk < iCIRAFSize ; kk++)
					{
						iCIRAF = vecAltFreqRegions[k].veciCIRAFZones[kk];

						if (strRegions != "")
							strRegions += ", ";
	
						strRegions += QString(strTableCIRAFzones[iCIRAF].c_str());
					}
				}
				else
				{
					/* if ciraf zones aren't defined show the latitude and
						longitude of the centre of the target area */
					
					if (strRegions != "")
						strRegions += ", ";

					int iLatitudeMed = (iLatitude + (iLatitudeEx / 2));

					strRegions += "latitude "
							+ QString::number(abs(iLatitudeMed));

					if (iLatitudeMed < 0)
						strRegions += "° S";
					else
						strRegions += "° N";

					int iLongitudeMed = (iLongitude + (iLongitudeEx / 2));

					if (iLongitudeMed >= 180)
							iLongitudeMed = iLongitudeMed - 360;

					strRegions += "  longitude "
							+ QString::number(abs(iLongitudeMed));

					if (iLongitudeMed < 0)
						strRegions += "° W";
					else
						strRegions += "° E";
				}

				/* check if receiver coordinates are into target area */
				if (bCheckCoordinates == TRUE)
				{
					_BOOLEAN bLongitudeOK = ((iReceiverLongitude >= iLongitude)
						&& (iReceiverLongitude <= (iLongitude + iLongitudeEx)))
						|| (((iLongitude + iLongitudeEx) >= 180) &&
						(iReceiverLongitude <= (iLongitude + iLongitudeEx - 360)));

					_BOOLEAN bLatitudeOK = ((iReceiverLatitude >= iLatitude)
						&& (iReceiverLatitude <= (iLatitude + iLatitudeEx)));

					bIntoTargetArea = bIntoTargetArea || (bLongitudeOK && bLatitudeOK);
				}
			}
			k++;
		}
	}
}

string CDRMLiveSchedule::DecodeFrequency(const int iSystemID, const int iFreq)
{
	switch (iSystemID)
	{
		case 0:
		case 1:
		case 2:
			/* AM or DRM */
			return QString().setNum(iFreq).latin1();

			break;

		case 3:
		case 4:
		case 5:
			/* 'FM1 frequency' - 87.5 to 107.9 MHz (100 kHz steps) */
			return QString().setNum((float) (87.5 + 0.1 * iFreq), 'f', 1).latin1();

			break;

		case 6:
		case 7:
		case 8:
			/* 'FM2 frequency'- 76.0 to 90.0 MHz (100 kHz steps) */
			return QString().setNum((float) (76.0 + 0.1 * iFreq), 'f', 1).latin1();

			break;

		default:
			return "";
			break;
	}
}

void CDRMLiveSchedule::LoadAFSInformations(const CParameter::CAltFreqSign AltFreqSign
		, const CParameter::CAltFreqOtherServicesSign AltFreqOtherServicesSign)
{
int iSize;
int z;
int j;
int k;

_BOOLEAN bFound;
QString  strSystem;
QString  strRegions;
_BOOLEAN bIntoTargetArea;

/* Init table for stations */
StationsTable.Init(0);

/* Add AFS informations DRM service */

iSize = AltFreqSign.vecAltFreq.Size();

for (z = 0; z < iSize; z++)
{
	CParameter::CAltFreqSign::CAltFreq AltFreq = AltFreqSign.vecAltFreq[z];

	/* TODO multiplex and restrictions */
	//AltFreq.bIsSyncMultplx;
	
	//for ( k = 0; k < 4; k++)
	//	AltFreq.veciServRestrict[k];
	
	strRegions = "";
	bIntoTargetArea = FALSE;

	bFound = FALSE;

	if (AltFreq.bRegionSchedFlag == TRUE)
	{
		DecodeTargets(AltFreq.iRegionID, AltFreqSign.vecAltFreqRegions, strRegions, bIntoTargetArea);

		if (AltFreq.iScheduleID > 0)
		{
			k = 0;

			while (k < AltFreqSign.vecAltFreqSchedules.Size())
			{
				/* Schedules */
				if (AltFreqSign.vecAltFreqSchedules[k].iScheduleID == AltFreq.iScheduleID)
				{
					bFound = TRUE;
					
					string strDaysFlag = Binary2String(AltFreqSign.vecAltFreqSchedules[k].iDayCode);

					/* For all frequencies */
					for (j = 0; j < AltFreq.veciFrequencies.Size(); j++)
					{	
						CLiveScheduleItem LiveScheduleItem;
	
						/* Frequency */
						LiveScheduleItem.strFreq = DecodeFrequency(0, AltFreq.veciFrequencies[j]);

						/* Set start time and duration */
						LiveScheduleItem.iStartTime = AltFreqSign.vecAltFreqSchedules[k].iStartTime;
						LiveScheduleItem.iDuration = AltFreqSign.vecAltFreqSchedules[k].iDuration;

						/* Days flags */
						LiveScheduleItem.strDaysFlags = strDaysFlag;

						/* Add the target */
						LiveScheduleItem.strTarget = strRegions.latin1();

						/* Local receiver coordinates are into target area or not */
						LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

						/* Add the system (transmission mode) */
						LiveScheduleItem.strSystem = "DRM";

						/* Add new item in table */
						StationsTable.Add(LiveScheduleItem);
					}
				}
				k++;
			}
		}
	}

	if ((bFound == FALSE) || (AltFreq.bRegionSchedFlag == FALSE))
	{
		/* For all frequencies */
		for (j = 0; j < AltFreq.veciFrequencies.Size(); j++)
		{	
			CLiveScheduleItem LiveScheduleItem;
	
			/* Frequency */
			LiveScheduleItem.strFreq = DecodeFrequency(0, AltFreq.veciFrequencies[j]);

			/* Add the target */
			LiveScheduleItem.strTarget = strRegions.latin1();

			/* Local receiver coordinates are into target area or not */
			LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

			/* Add the system (transmission mode) */
			LiveScheduleItem.strSystem = "DRM";

			/* Add new item in table */
			StationsTable.Add(LiveScheduleItem);
		}
	}
}

/* Other Services */

iSize = AltFreqOtherServicesSign.vecAltFreqOtherServices.Size();

for (z = 0; z < iSize; z++)
{
	CParameter::CAltFreqOtherServicesSign::CAltFreqOtherServices AltFreqOther = AltFreqOtherServicesSign.vecAltFreqOtherServices[z];

	if (AltFreqOther.iSystemID < 9) /* Don't show DAB services */
	{
		/* TODO add DAB frequencies */

		/* TODO same service */
		//AltFreqOther.bSameService;

		switch (AltFreqOther.iSystemID)
		{
			case 0:
				strSystem = "DRM";
				break;

			case 1:
			case 2:
				strSystem = "AM";
				break;

			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				strSystem = "FM";
				break;

			default:
				strSystem = "";
				break;
		}

		strRegions = "";
		bIntoTargetArea = FALSE;

		bFound = FALSE;

		if (AltFreqOther.bRegionSchedFlag == TRUE)
		{
			DecodeTargets(AltFreqOther.iRegionID, AltFreqSign.vecAltFreqRegions, strRegions, bIntoTargetArea);

			if (AltFreqOther.iScheduleID > 0)
			{
				k = 0;
	
				while (k < AltFreqSign.vecAltFreqSchedules.Size())
				{
					/* Schedules */
					if (AltFreqSign.vecAltFreqSchedules[k].iScheduleID == AltFreqOther.iScheduleID)
					{
						bFound = TRUE;
			
						string strDaysFlag = Binary2String(AltFreqSign.vecAltFreqSchedules[k].iDayCode);

						/* For all frequencies */
						for (j = 0; j < AltFreqOther.veciFrequencies.Size(); j++)
						{	
							CLiveScheduleItem LiveScheduleItem;
	
							/* Frequency */
							LiveScheduleItem.strFreq = DecodeFrequency(AltFreqOther.iSystemID, AltFreqOther.veciFrequencies[j]);

							/* Set start time and duration */
							LiveScheduleItem.iStartTime = AltFreqSign.vecAltFreqSchedules[k].iStartTime;
							LiveScheduleItem.iDuration = AltFreqSign.vecAltFreqSchedules[k].iDuration;

							/* Days flags */
							LiveScheduleItem.strDaysFlags = strDaysFlag;

							/* Add the target */
							LiveScheduleItem.strTarget = strRegions.latin1();

							/* Local receiver coordinates are into target area or not */
							LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

							/* Add the system (transmission mode) */
							LiveScheduleItem.strSystem = strSystem.latin1();

							/* Add new item in table */
							StationsTable.Add(LiveScheduleItem);
						}
					}
					k++;
				}
			}
		}

		if ((bFound == FALSE) || (AltFreqOther.bRegionSchedFlag == FALSE))
		{
			/* For all frequencies */
			for (j = 0; j < AltFreqOther.veciFrequencies.Size(); j++)
			{	
				CLiveScheduleItem LiveScheduleItem;
	
				/* Frequency */
				LiveScheduleItem.strFreq = DecodeFrequency(AltFreqOther.iSystemID, AltFreqOther.veciFrequencies[j]);

				/* Add the target */
				LiveScheduleItem.strTarget = strRegions.latin1();

				/* Local receiver coordinates are into target area or not */
				LiveScheduleItem.bInsideTargetArea = bIntoTargetArea;

				/* Add the system (transmission mode) */
				LiveScheduleItem.strSystem = strSystem.latin1();

				/* Add new item in table */
				StationsTable.Add(LiveScheduleItem);
			}
		}
	}
}
}

LiveScheduleDlg::LiveScheduleDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f): CLiveScheduleDlgBase(parent, name, modal, f),
	pDRMRec(pNDRMR), vecpListItems(0)
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

	BitmCubeGreenLittle.resize(5, 5);
	BitmCubeGreenLittle.fill(QColor(0, 255, 0));


	BitmCubeYellow.resize(iXSize, iYSize);
	BitmCubeYellow.fill(QColor(255, 255, 0));
	BitmCubeRed.resize(iXSize, iYSize);
	BitmCubeRed.fill(QColor(255, 0, 0));
	BitmCubeOrange.resize(iXSize, iYSize);
	BitmCubeOrange.fill(QColor(255, 128, 0));
	BitmCubePink.resize(iXSize, iYSize);
	BitmCubePink.fill(QColor(255, 128, 128));

	/* Clear list box for file names and set up columns */
	ListViewStations->clear();

	/* We assume that one column is already there */
	ListViewStations->setColumnText(COL_FREQ, tr("Frequency [kHz/MHz]"));
	ListViewStations->addColumn(tr("System"));
	ListViewStations->addColumn(tr("Time [UTC]"));
	ListViewStations->addColumn(tr("Target"));
	ListViewStations->addColumn(tr("Start day"));

	/* Set right alignment for numeric columns */
	ListViewStations->setColumnAlignment(COL_FREQ, Qt::AlignRight);

	/* this for add spaces into the column and show all the header caption */
	vecpListItems.Init(1);
	vecpListItems[0] = new MyListLiveViewItem(ListViewStations,
		"00000000000000000");

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
	switch (pDRMRec->iSecondsPreviewLiveSched)
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

	/* Get current receiver latitude and longitude if defined */
	DRMSchedule.SetReceiverCoordinates(pDRMRec->GetParameters()->ReceptLog.GetLatitude()
			,pDRMRec->GetParameters()->ReceptLog.GetLongitude());

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

	/* Check boxes */
	connect(CheckBoxFreeze, SIGNAL(clicked()),
		this, SLOT(OnCheckFreeze()));
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

void LiveScheduleDlg::OnCheckFreeze()
{
	/* if CheckBoxFreeze is checked the schedule is freezed */
	if (CheckBoxFreeze->isChecked())
		TimerList.stop();
	else
	{
		OnTimerList();
		TimerList.start(GUI_TIMER_LIST_VIEW_UPDATE); /* Stations list */
	}
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

	DRMSchedule.LoadAFSInformations(pDRMRec->GetParameters()->AltFreqSign, pDRMRec->GetParameters()->AltFreqOtherServicesSign);

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

	QString strTitle = tr("Live Schedule");

	if (DRMSchedule.GetStationNumber() > 0)
	{
		/* Get current service */
		const int iCurSelAudioServ = pDRMRec->GetParameters()->GetCurSelAudioService();

		if (pDRMRec->GetParameters()->Service[iCurSelAudioServ].IsActive())
		{
			/* Do UTF-8 to string conversion with the label strings */
			QString strStationName = QString().fromUtf8(QCString(pDRMRec->GetParameters()
				->Service[iCurSelAudioServ].strLabel.c_str()));

			/* add station name on the title of the dialog */
			if (strStationName != "")
				strTitle += " [" + strStationName.stripWhiteSpace() + "]";
		}
	}

	SetDialogCaption(this, strTitle);
}

void LiveScheduleDlg::showEvent(QShowEvent*)
{
	/* Update window */
	OnTimerUTCLabel();
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);

	if (!CheckBoxFreeze->isChecked())
	{
		OnTimerList();

		/* Activate real-time timer when window is shown */
		TimerList.start(GUI_TIMER_LIST_VIEW_UPDATE); /* Stations list */
	}
}

void LiveScheduleDlg::hideEvent(QHideEvent*)
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
					QString(DRMSchedule.GetItem(i).strFreq.c_str()) /* freq. */,
					QString(DRMSchedule.GetItem(i).strSystem.c_str())   /* system */,
					ExtractTime(DRMSchedule.GetItem(i).iStartTime, DRMSchedule.GetItem(i).iDuration) /* time */,
					QString(DRMSchedule.GetItem(i).strTarget.c_str())   /* target */,
					ExtractDaysFlagString(DRMSchedule.GetItem(i).strDaysFlags) /* Show list of days */);

				/* Set flag for sorting the list */
				bListHastChanged = TRUE;
			}

			/* If receiver coordinates are into target area add a little cube green */
			if (DRMSchedule.GetItem(i).bInsideTargetArea == TRUE)
				vecpListItems[i]->setPixmap(COL_TARGET, BitmCubeGreenLittle);

			/* Check, if station is currently transmitting. If yes, set
			   special pixmap */
			switch (DRMSchedule.CheckState(i))
			{
				case CDRMLiveSchedule::IS_ACTIVE:
					vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeGreen);
					break;
				case CDRMLiveSchedule::IS_PREVIEW:
					vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeOrange);
					break;
				case CDRMLiveSchedule::IS_SOON_INACTIVE:
					vecpListItems[i]->setPixmap(COL_FREQ, BitmCubePink);
					break;
				case CDRMLiveSchedule::IS_INACTIVE:
					vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeRed);
					break;
				default:
					vecpListItems[i]->setPixmap(COL_FREQ, BitmCubeRed);
					break;
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
	if (strFileName.right(1).latin1() != QString("/"))
	{
		strCurrentSavePath = QFileInfo(strFileName).dirPath();

		if (strCurrentSavePath.right(1).latin1() != QString("/"))
			strCurrentSavePath += "/";
	}
	else
		strCurrentSavePath = strFileName;
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

	const int iCurSelAudioServ = pDRMRec->GetParameters()->GetCurSelAudioService();
	QString strStationName = pDRMRec->GetParameters()->Service[iCurSelAudioServ].strLabel.c_str();

	/* Lock mutex for use the vecpListItems */
	ListItemsMutex.Lock();

	/* Force the sort for all items */ 
 	ListViewStations->firstChild()
		->sortChildItems(iCurrentSortColumn,bCurrentSortAscending);

	/* Extract values from the list */
    QListViewItem * myItem = ListViewStations->firstChild();

    while( myItem )
	{
			strSchedule += "<tr>"
				"<td align=\"right\">" + myItem->text(COL_FREQ) + "</td>" /* freq */
				"<td>" + ColValue(myItem->text(1)) + "</td>" /* system */
				"<td>" + ColValue(myItem->text(2)) + "</td>" /* time */
				"<td>" + ColValue(myItem->text(3)) + "</td>" /* target */
				"<td>" + ColValue(myItem->text(4)) + "</td>" /* days */
				"</tr>\n";
        myItem = myItem->nextSibling();
    }

	ListItemsMutex.Unlock();

	if (strSchedule != "")
	{
		/* Save to file current schedule  */
		QString strTitle(tr("AFS Live Schedule"));
		QString strItems("");

		/* Prepare HTML page for storing the content */
		QString strText = "<html>\n<head>\n"
			"<meta http-equiv=\"content-Type\" "
			"content=\"text/html; charset=utf-8\">\n<title>"
			+ strStationName + " - " + strTitle +
			"</title>\n</head>\n\n<body>\n"
			"<h4>" + strTitle + "</h4>"
			"<h3>" + strStationName + "</h3>"
			"\n<table border=\"1\"><tr>\n"
			"<th>" + tr("Frequency [kHz/MHz]") + "</th>"
			"<th>" + tr("System") + "</th>"
			"<th>" + tr("Time [UTC]") + "</th>"
			"<th>" + tr("Target") + "</th>"
			"<th>" + tr("Start day") + "</th>\n"
			"</tr>\n"
			+ strSchedule +
			"</table>\n"
			/* Add current date and time */
			"<br><p align=right><font size=-2><i>" +
			QDateTime().currentDateTime().toString() + "</i></font></p>"
			"</body>\n</html>";

		/* Do UTF-8 to string conversion with the station name strings */
		strFileName = QFileDialog::getSaveFileName(strCurrentSavePath +
			QString().fromUtf8(QCString(strStationName)) + "_" +
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
		"it's possible to view AFS (Alternative Frequency Signalling) "
		"informations trasmitted with the current DRM or AMSS signal.</b>"
		"It is possible to show only active stations by changing a "
		"setting in the 'view' menu.<br>"
		"The color of the cube on the left of the "
		"frequency shows the current status of the transmission.<br>"
		"A green box shows that the transmission takes place right now "
		"a red cube it is shown that the transmission is offline, "
		"a pink cube shown that the transmission soon will be offline.<br>"
		"If the stations preview is active an orange box shows the stations "
		"that will be active.<br>"
		"A little green cube on the left of the target column show that the receiver"
		" coordinates (latitude and longitude) stored into Dream settings are into"
		" the target area of this transmission.<br>"
		"The list can be sorted by clicking on the headline of the "
		"column."));

	/* UTC time label */
	QWhatsThis::add(TextLabelUTCTime,
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is also known as Greenwich Mean Time "
		"(GMT)."));

	/* Check box freeze */
	QWhatsThis::add(CheckBoxFreeze,
		tr("<b>Freeze:</b> If this check box is selectd the live schedule is freezed."));
}


CDRMLiveSchedule::StationState CDRMLiveSchedule::CheckState(const int iPos)
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

	/* Check day
	   tm_wday: day of week (0 - 6; Sunday = 0) 
	   I must normalize so Monday = 0   */
	
	if (gmtCur->tm_wday == 0)
		iWeekDay = 6;
	else
		iWeekDay = gmtCur->tm_wday - 1;

	/* iTimeWeek minutes since last Monday 00:00 in UTC */
	/* the value is in the range 0 <= iTimeWeek < 60 * 24 * 7)   */
	
	const int iTimeWeek = (iWeekDay * 24 * 60) + (gmtCur->tm_hour * 60) + gmtCur->tm_min;
	
	/* DaysFlag structure 1111111 = 1234567 (1 = Monday, 7 = Sunday) */
	for (int i = 0; i < 7; i++)
	{
		/* Check if day is active */
		if (StationsTable[iPos].strDaysFlags[i] == CHR_ACTIVE_DAY_MARKER)
		{
			/* Tuesday -> 1 * 24 * 60 = 1440 */
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
