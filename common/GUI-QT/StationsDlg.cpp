/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Mark J. Fine, Markus Maerz
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
#include <sys/termios.h>
#endif


/* Implementation *************************************************************/
CDRMSchedule::CDRMSchedule()
{
	ReadStatTabFromFile("DRMSchedule.ini");
}

void CDRMSchedule::WriteStatTabToFile(const string strFileName)
{
	/* Load table from file */
	FILE* pFile = fopen(strFileName.c_str(), "w");

	/* Check if opening of file was successful */
	if (pFile == 0)
		return;

	fprintf(pFile, "[DRMSchedule]\n");
	for (int i = 0; i < StationsTable.Size(); i++)
	{
		/* Start stop time */
		fprintf(pFile, "StartStopTimeUTC=%04d-%04d\n",
			StationsTable[i].GetStartTimeNum(),
			StationsTable[i].GetStopTimeNum());

		/* Days */
		fprintf(pFile, "Days[SMTWTFS]=%d\n", StationsTable[i].iDays);

		/* Frequency */
		fprintf(pFile, "Frequency=%d\n", StationsTable[i].iFreq);

		/* Target */
		fprintf(pFile, "Target=%s\n", StationsTable[i].strTarget.c_str());

		/* Power */
		if (StationsTable[i].rPower == 0)
			fprintf(pFile, "Power=?\n");
		else
			fprintf(pFile, "Power=%.3f\n", StationsTable[i].rPower);

		/* Name of the station */
		fprintf(pFile, "Programme=%s\n", StationsTable[i].strName.c_str());

		/* Language */
		fprintf(pFile, "Language=%s\n", StationsTable[i].strLanguage.c_str());

		/* Site */
		fprintf(pFile, "Site=%s\n", StationsTable[i].strSite.c_str());

		/* Country */
		fprintf(pFile, "Country=%s\n\n", StationsTable[i].strCountry.c_str());
	}
	fclose(pFile);
}

void CDRMSchedule::ReadStatTabFromFile(const string strFileName)
{
	const int	iMaxLenName = 255;
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
		/* Get start time */
		struct tm* gmtStart = gmtime(&ltime);
		gmtStart->tm_hour = StationsTable[iPos].iStartHour;
		gmtStart->tm_min = StationsTable[iPos].iStartMinute;
		time_t lStartTime = mktime(gmtStart);

		/* Get stop time */
		struct tm* gmtStop = gmtime(&ltime);
		gmtStop->tm_hour = StationsTable[iPos].iStopHour;
		gmtStop->tm_min = StationsTable[iPos].iStopMinute;

		/* Check, if stop time is on next day */
		if (StationsTable[iPos].iStartHour > StationsTable[iPos].iStopHour)
			gmtStop->tm_mday++;

		const time_t lStopTime = mktime(gmtStop);

		/* Check interval */
		if ((lCurTime >= lStartTime) && (lCurTime < lStopTime))
			return TRUE;
	}

	return FALSE;
}

StationsDlg::StationsDlg(QWidget* parent, const char* name, bool modal,
	WFlags f) :	CStationsDlgBase(parent, name, modal, f)
{
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

	/* Set up frequency selector control (QWTCounter control) */
	QwtCounterFrequency->setRange(0.0, 30000.0, 1.0);
	QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
	QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

	/* Init with current setting in log file */
	QwtCounterFrequency->
		setValue(DRMReceiver.GetParameters()->ReceptLog.GetFrequency());


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

	/* Sort list by start time (second column) */
	ListViewStations->setSorting(1);
	ListViewStations->sort();


	/* Remote menu  --------------------------------------------------------- */
	pRemoteMenu = new QPopupMenu(this);
	CHECK_PTR(pRemoteMenu);
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

	/* Separator */
	pRemoteMenu->insertSeparator();

	/* Which COM port. Start menu IDs from ID = 100 */
	pRemoteMenu->insertItem("COM1",
		this, SLOT(OnComPortMenu(int)), 0, 100);
	pRemoteMenu->insertItem("COM2",
		this, SLOT(OnComPortMenu(int)), 0, 101);
#ifdef _WIN32
	pRemoteMenu->insertItem("COM3",
		this, SLOT(OnComPortMenu(int)), 0, 102);
#else
	pRemoteMenu->insertItem("USB",
		this, SLOT(OnComPortMenu(int)), 0, 102);
#endif

	/* Default com number */
	eComNumber = CN_COM1;
	pRemoteMenu->setItemChecked(100, TRUE);

#ifdef _WIN32
	/* Set WINRADIO to default, because I own such a device :-) */
	eWhichRemoteControl = RC_WINRADIO;
	pRemoteMenu->setItemChecked(1, TRUE);

	/* No com number needed for Winrado receiver */
	pRemoteMenu->setItemEnabled(100, FALSE);
	pRemoteMenu->setItemEnabled(101, FALSE);
	pRemoteMenu->setItemEnabled(102, FALSE);
#else
	eWhichRemoteControl = RC_NOREMCNTR;
	pRemoteMenu->setItemChecked(0, TRUE);
#endif


	/* Update menu ---------------------------------------------------------- */
	QPopupMenu* pUpdateMenu = new QPopupMenu(this);
	CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem("&Get Update...", this, SLOT(OnGetUpdate()));


	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&View", pViewMenu);
	pMenu->insertItem("&Remote", pRemoteMenu);
	pMenu->insertItem("&Update", pUpdateMenu);
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	CStationsDlgBaseLayout->setMenuBar(pMenu);


	/* Connections ---------------------------------------------------------- */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(ListViewStations, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));
	connect(&UrlUpdateSchedule, SIGNAL(finished(QNetworkOperation*)),
		this, SLOT(OnUrlFinished(QNetworkOperation*)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));


	/* Set up timer */
	Timer.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnShowStationsMenu(int iID)
{
	if (iID == 0)
	{
		/* Show only active stations */
		bShowAll = FALSE;
	}
	else
	{
		/* Show all stations */
		bShowAll = TRUE;
	}

	/* Update list view */
	SetStationsView();

	/* Sort list by start time (second column) */
	ListViewStations->setSorting(1);
	ListViewStations->sort();

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
		/* First, the network protokol (ftp) must be registered */
		QNetworkProtocol::registerNetworkProtocol("ftp", new
			QNetworkProtocolFactory<QFtp>);

		/* Try to download the current schedule. Copy the file to the
		   current working directory (which is "QDir().absFilePath(NULL)") */
		UrlUpdateSchedule.copy(QString(DRM_SCHEDULE_UPDATE_FILE),
			QString(QDir().absFilePath(NULL)));
	}
}

void StationsDlg::OnUrlFinished(QNetworkOperation* pNetwOp)
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
			QMessageBox::information(this, "Dream", "Update successful.",
				QMessageBox::Ok);

			/* Read updated ini-file */
			DRMSchedule.ReadStatTabFromFile("DRMSchedule.ini");

			/* Update list view */
			SetStationsView();
		}
	}
}

void StationsDlg::showEvent(QShowEvent* pEvent)
{
	/* If number of stations is zero, we assume that the ini file is missing */
	if (DRMSchedule.GetStationNumber() == 0)
	{
		QMessageBox::information(this, "Dream", "The file DRMSchedule.ini could "
			"not be found.\nNo stations can be displayed.\n"
			"Try to download this file by using the 'Update' menu.",
			QMessageBox::Ok);
	}
}

void StationsDlg::OnTimer()
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
	QString strSelTabItemName = "";
	QString strSelTabItemTime = "";
	QString strSelTabItemFreq = "";

	/* Make sure that the selected item is still selected. Identify selected
	   item by frequency, time and name */
	if (ListViewStations->selectedItem() != 0)
	{
		strSelTabItemName = ListViewStations->selectedItem()->text(0);
		strSelTabItemTime = ListViewStations->selectedItem()->text(1);
		strSelTabItemFreq = ListViewStations->selectedItem()->text(2);
	}

	/* First, clear list view */
	ListViewStations->clear();

	/* Get active stations from schedule */
	const int iNumStations = DRMSchedule.GetStationNumber();

	/* Add new item for each station in list view */
	for (int i = 0; i < iNumStations; i++)
	{
		if (!((bShowAll == FALSE) && (DRMSchedule.IsActive(i) == FALSE)))
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
			QListViewItem* NewListItem = new MyListViewItem(ListViewStations,
				DRMSchedule.GetItem(i).strName.c_str()			/* name */,
				QString().sprintf("%04d-%04d",
				DRMSchedule.GetItem(i).GetStartTimeNum(),
				DRMSchedule.GetItem(i).GetStopTimeNum())		/* time */,
				QString().setNum(DRMSchedule.GetItem(i).iFreq)	/* frequency */,
				DRMSchedule.GetItem(i).strTarget.c_str()		/* target */,
				strPower										/* power */,
				DRMSchedule.GetItem(i).strCountry.c_str()		/* country */,
				DRMSchedule.GetItem(i).strSite.c_str()			/* site */,
				DRMSchedule.GetItem(i).strLanguage.c_str()		/* language */);

			/* Check, if station is currently transmitting. If yes, set special
			   pixmap */
			if (DRMSchedule.IsActive(i) == TRUE)
			{
				/* Check for "special case" transmissions */
				if (DRMSchedule.GetItem(i).iDays == 0)
					NewListItem->setPixmap(0, BitmCubeYellow);
				else
					NewListItem->setPixmap(0, BitmCubeGreen);
			}
			else
				NewListItem->setPixmap(0, BitmCubeRed);

			/* Insert this new item in list. The item object is destroyed by the
			   list view control when this is destroyed */
			ListViewStations->insertItem(NewListItem);
		}
	}


	/* Recover selection ---------------------------------------------------- */
	QListViewItem* pCurItem = ListViewStations->firstChild();

	/* Check all items */
	while (pCurItem != 0)
	{
		if ((pCurItem->text(0) == strSelTabItemName) &&
			(pCurItem->text(1) == strSelTabItemTime) &&
			(pCurItem->text(2) == strSelTabItemFreq))
		{
			/* Selecte item. selectionChanged() is emitted which would case the
			   front-end to be switched to the frequncy all the time but since
			   we use the clicked() signal, that is no problem in our case */
			ListViewStations->setSelected(pCurItem, TRUE);
		}

		/* Get next item in list view */
		pCurItem = pCurItem->nextSibling();
	}
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
	/* If no item is selected, return */
	if (item == 0)
		return;

	/* Third text of list view item is frequency -> text(2) */
	const int iCurFreqkHz = QString(item->text(2)).toInt();

	SetFrequency(iCurFreqkHz);

	/* Now tell the receiver that the frequency has changed */
	DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);

	/* Set selected frequency in log file class */
	DRMReceiver.GetParameters()->ReceptLog.SetFrequency(iCurFreqkHz);

	/* Set value in frequency counter control QWT */
	QwtCounterFrequency->setValue(iCurFreqkHz);
}

void StationsDlg::OnRemoteMenu(int iID)
{
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
		pRemoteMenu->setItemEnabled(100, FALSE);
		pRemoteMenu->setItemEnabled(101, FALSE);
		pRemoteMenu->setItemEnabled(102, FALSE);
		break;

	default:
		pRemoteMenu->setItemEnabled(100, TRUE);
		pRemoteMenu->setItemEnabled(101, TRUE);
		pRemoteMenu->setItemEnabled(102, TRUE);
		break;
	}
}

void StationsDlg::OnComPortMenu(int iID)
{
	switch (iID)
	{
	case 100:
		eComNumber = CN_COM1;
		break;

	case 101:
		eComNumber = CN_COM2;
		break;

	case 102:
		eComNumber = CN_COM3;
		break;
	}

	/* Taking care of checks in the menu */
	pRemoteMenu->setItemChecked(100, 100 == iID);
	pRemoteMenu->setItemChecked(101, 101 == iID);
	pRemoteMenu->setItemChecked(102, 102 == iID);
}

void StationsDlg::SetFrequency(const int iFreqkHz)
{	
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
}


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

		const _UINT32BIT iNewFreq = (_UINT32BIT)
			(((CReal) iFreqkHz + rIFMixFreq) / rOscFreq * (CReal) 4294967296.0);

		const _UINT32BIT iFreqLoL = iNewFreq & 0xFF;
		const _UINT32BIT iFreqLoH = (iNewFreq & 0xFF00) >> 8;
		const _UINT32BIT iFreqHiL = (iNewFreq & 0xFF0000) >> 16;
		const _UINT32BIT iFreqHiH = (iNewFreq & 0xFF000000) >> 24;

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

void StationsDlg::OutputElektor304(FILE_HANDLE hCom, const _UINT32BIT iData)
{
	SetOutStateElektor304(hCom, OW_TXD, 0); // TXD 0
	SetOutStateElektor304(hCom, OW_DTR, 1); // DTR 1 // CE

	_UINT32BIT iMask = 0x8000;
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
