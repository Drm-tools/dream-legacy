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

#include "StationsDlg.h"


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
	if ((StationsTable[iPos].iDays / ((6 - gmtCur->tm_wday) * 10)) % 2 == 1)
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

	/* Set stations in list view which are active right now */
	bShowAll = FALSE;
	RadioButtonOnlyActive->setChecked(TRUE);
	SetStationsView();

	/* Sort list by start time (second column) */
	ListViewStations->setSorting(1);
	ListViewStations->sort();


	/* Connections ---------------------------------------------------------- */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
	connect(ListViewStations, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(OnListItemClicked(QListViewItem*)));

	/* Button groups */
	connect(ButtonGroupShowStations, SIGNAL(clicked(int)),
		this, SLOT(OnRadioShowItems(int)));


	/* Set up timer */
	Timer.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnRadioShowItems(int iID)
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
}

void StationsDlg::OnTimer()
{
	/* Update list view */
	SetStationsView();
}

void StationsDlg::showEvent(QShowEvent* pEvent)
{
	/* If number of stations is zero, we assume that the ini file is missing */
	if (DRMSchedule.GetStationNumber() == 0)
	{
		QMessageBox::information(this, "Dream", "The file DRMSchedule.ini could "
			"not be found. No stations can be displayed.", QMessageBox::Ok);
	}
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
			/* Generate new list item with all necessary column entries */
			QListViewItem* NewListItem = new QListViewItem(ListViewStations,
				DRMSchedule.GetItem(i).strName.c_str()			/* name */,
				QString().sprintf("%04d-%04d",
				DRMSchedule.GetItem(i).GetStartTimeNum(),
				DRMSchedule.GetItem(i).GetStopTimeNum())		/* time */,
				QString().setNum(DRMSchedule.GetItem(i).iFreq)	/* frequency */,
				DRMSchedule.GetItem(i).strTarget.c_str()		/* target */,
				QString().setNum(DRMSchedule.GetItem(i).rPower)	/* power */,
				DRMSchedule.GetItem(i).strCountry.c_str()		/* country */,
				DRMSchedule.GetItem(i).strSite.c_str()			/* site */,
				DRMSchedule.GetItem(i).strLanguage.c_str()		/* language */);

			/* Check, if station is currently transmitting. If yes, set special
			   pixmap */
			if (DRMSchedule.IsActive(i) == TRUE)
				NewListItem->setPixmap(0, BitmCubeGreen);
			else
				NewListItem->setPixmap(0, BitmCubeRed);

			/* Insert this new item in list. The item object is destroyed by the
			   list view control when this is destroyed */
			ListViewStations->insertItem(NewListItem);
		}
	}

	/* Recover selection */
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

void StationsDlg::OnListItemClicked(QListViewItem* item)
{
	/* If no item is selected, return */
	if (item == 0)
		return;

#ifdef _WIN32
	/* Some type definitions needed for dll access */
	typedef BOOL (__stdcall *FNCCloseRadioDevice)(int hRadio);
	typedef BOOL (__stdcall *FNCG3SetFrequency)(int hRadio, DWORD dwFreq);
	typedef int (__stdcall *FNCOpenRadioDevice)(int iDeviceNum);
	typedef BOOL (__stdcall *FNCSetPower)(int hRadio, BOOL rPower);

	/* Try to load required dll */
	HMODULE dll = LoadLibrary("wrg3api.dll");

	if (dll)
	{
		/* Get process addresses from dll for function access */
		FNCCloseRadioDevice CloseRadioDevice =
			(FNCCloseRadioDevice) GetProcAddress(dll, "CloseRadioDevice");
		FNCG3SetFrequency G3SetFrequency =
			(FNCG3SetFrequency) GetProcAddress(dll, "SetFrequency");
		FNCOpenRadioDevice OpenRadioDevice =
			(FNCOpenRadioDevice) GetProcAddress(dll, "OpenRadioDevice");
		FNCSetPower SetPower = (FNCSetPower) GetProcAddress(dll, "SetPower");

		/* Open Winradio receiver handle */
		int hRadio = OpenRadioDevice(0);

		/* Make sure the receiver is switched on */
		SetPower(hRadio, 1);

		/* Set the frequency ("* 1000", because frequency is in kHz). Third text of
		   list view item is frequency -> text(2) */
		G3SetFrequency(hRadio, (DWORD) (QString(item->text(2)).toInt() * 1000));

		/* Close device afterwards and also clean up the dll access */
		CloseRadioDevice(hRadio);
		FreeLibrary(dll);

		/* Now tell the receiver that the frequency has changed */
		DRMReceiver.SetReceiverMode(CDRMReceiver::RM_DRM);
	}
#endif
}
