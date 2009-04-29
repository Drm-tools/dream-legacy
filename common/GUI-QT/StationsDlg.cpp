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
#include <QMenuBar>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDateTime>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHeaderView>
#include <iostream>

StationsDlg::StationsDlg(ReceiverInterface& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	QDialog(parent, name, modal, f), Ui_StationsDlg(),
	Receiver(NDRMR), Settings(NSettings), Schedule(),
	TimerList(), TimerUTCLabel(), TimerSMeter(), TimerEditFrequency(), TimerMonitorFrequency(),
	TimerTuning(),
	iCurrentSortColumn(0), bCurrentSortAscending(true), bShowAll(false),
	bTuningInProgress(false), bReInitOnFrequencyChange(false), eModulation(DRM),
	networkManager(NULL),
	pViewMenu(NULL), pPreviewMenu(NULL), pUpdateMenu(NULL),
	ListItemsMutex()
{
    setupUi(this);
	/* Set help text for the controls */
	AddWhatsThisHelp();

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(&Schedule);
	proxyModel->setFilterKeyColumn(0); // actually we don't care
	proxyModel->setFilterRole(Qt::UserRole);

	stationsView->setModel(proxyModel);
	stationsView->setSortingEnabled(true);
	stationsView->horizontalHeader()->setVisible(true);

	/* Set up frequency selector control (QWTCounter control) */
	QwtCounterFrequency->setRange(0.0, 30000.0, 1.0);
	QwtCounterFrequency->setNumButtons(3); /* Three buttons on each side */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button1, 1); /* Increment */
	QwtCounterFrequency->setIncSteps(QwtCounter::Button2, 10);
	QwtCounterFrequency->setIncSteps(QwtCounter::Button3, 100);

	/* Init UTC time shown with a label control */
	SetUTCTimeLabel();

	/* Set Menu ***************************************************************/
	/* View menu ------------------------------------------------------------ */
	pViewMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pViewMenu);
	pViewMenu->insertItem(tr("Show &only active stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 0);
	pViewMenu->insertItem(tr("Show &all stations"), this,
		SLOT(OnShowStationsMenu(int)), 0, 1);

	/* Set stations in list view which are active right now */
	bShowAll = false;
	pViewMenu->setItemChecked(0, true);

	/* Stations Preview menu ------------------------------------------------ */
	pPreviewMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pPreviewMenu);
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
		pPreviewMenu->setItemChecked(1, true);
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
		break;

	case NUM_SECONDS_PREV_15MIN:
		pPreviewMenu->setItemChecked(2, true);
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case NUM_SECONDS_PREV_30MIN:
		pPreviewMenu->setItemChecked(3, true);
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0, also takes care of out of value parameters */
		pPreviewMenu->setItemChecked(0, true);
		Schedule.SetSecondsPreview(0);
		break;
	}

	pViewMenu->insertSeparator();
	pViewMenu->insertItem(tr("Stations &preview"), pPreviewMenu);

	//SetStationsView();

	/* Update menu ---------------------------------------------------------- */
	pUpdateMenu = new Q3PopupMenu(this);
	Q_CHECK_PTR(pUpdateMenu);
	pUpdateMenu->insertItem(tr("&Get Update..."), this, SLOT(OnGetUpdate()), 0, 0);

	/* Main menu bar -------------------------------------------------------- */
	QMenuBar* pMenu = new QMenuBar(this);
	Q_CHECK_PTR(pMenu);
	pMenu->insertItem(tr("&View"), pViewMenu);
	pMenu->insertItem(tr("&Update"), pUpdateMenu); /* String "Update" used below */
	pMenu->setSeparator(QMenuBar::InWindowsStyle);

	/* Now tell the layout about the menu */
	gridLayout->setMenuBar(pMenu);

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(OnUrlFinished(QNetworkReply*)));

	/* Connections ---------------------------------------------------------- */
	connect(&TimerList, SIGNAL(timeout()), this, SLOT(OnTimerList()));
	connect(&TimerUTCLabel, SIGNAL(timeout()), this, SLOT(OnTimerUTCLabel()));
	connect(&TimerSMeter, SIGNAL(timeout()), this, SLOT(OnTimerSMeter()));
	connect(&TimerEditFrequency, SIGNAL(timeout()), this, SLOT(OnTimerEditFrequency()));
	connect(&TimerMonitorFrequency, SIGNAL(timeout()), this, SLOT(OnTimerMonitorFrequency()));
	connect(&TimerTuning, SIGNAL(timeout()), this, SLOT(OnTimerTuning()));
	connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

	TimerList.stop();
	TimerUTCLabel.stop();
	TimerSMeter.stop();
	TimerEditFrequency.stop();
	TimerMonitorFrequency.stop();
	TimerTuning.stop();

	connect(stationsView, SIGNAL(clicked(const QModelIndex&)),
		this, SLOT(OnItemClicked(const QModelIndex&)));

	connect(QwtCounterFrequency, SIGNAL(valueChanged(double)),
		this, SLOT(OnFreqCntNewValue(double)));

	connect(ComboBoxFilterTarget, SIGNAL(activated(const QString&)),
		this, SLOT(OnFilterByTarget(const QString&)));
	connect(ComboBoxFilterCountry, SIGNAL(activated(const QString&)),
		this, SLOT(OnFilterByCountry(const QString&)));
	connect(ComboBoxFilterLanguage, SIGNAL(activated(const QString&)),
		this, SLOT(OnFilterByLanguage(const QString&)));

	ProgrSigStrength->setRange(S_METER_THERMO_MIN, S_METER_THERMO_MAX);
	ProgrSigStrength->setOrientation(Qt::Horizontal, QwtThermo::TopScale);
	ProgrSigStrength->setAlarmLevel(S_METER_THERMO_ALARM);
	ProgrSigStrength->setAlarmColor(QColor(255, 0, 0));
	ProgrSigStrength->setScale(S_METER_THERMO_MIN, S_METER_THERMO_MAX, 10.0);
}

StationsDlg::~StationsDlg()
{
}

void StationsDlg::OnFilterByTarget(const QString&)
{
	QString target = ComboBoxFilterTarget->currentText();
	if(target=="") target = "[^#]*";
	QString country = ComboBoxFilterCountry->currentText();
	if(country=="") country = "[^#]*";
	QString language = ComboBoxFilterLanguage->currentText();
	if(language=="") language = "[^#]*";
	QString r = QString("%1#%2#%3#%4").arg(target).arg(country).arg(language).arg(bShowAll?".":"1");
	//cerr << "filter " << r.toStdString() << endl;
	proxyModel->setFilterRegExp(QRegExp(r));
}

void StationsDlg::OnFilterByCountry(const QString&)
{
	OnFilterByTarget("");
}

void StationsDlg::OnFilterByLanguage(const QString&)
{
	OnFilterByTarget("");
}

bool StationsDlg::CheckFilter(const int iPos)
{
    return false;
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
		bShowAll = false;
		/* clear all and reload. If the list is long this increases performance */
		Schedule.clear();
	}
	else
		bShowAll = true;

	/* Taking care of checks in the menu */
	pViewMenu->setItemChecked(0, 0 == iID);
	pViewMenu->setItemChecked(1, 1 == iID);
}

void StationsDlg::OnShowPreviewMenu(int iID)
{
	switch (iID)
	{
	case 1:
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_5MIN);
		break;

	case 2:
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_15MIN);
		break;

	case 3:
		Schedule.SetSecondsPreview(NUM_SECONDS_PREV_30MIN);
		break;

	default: /* case 0: */
		Schedule.SetSecondsPreview(0);
		break;
	}

	/* Taking care of checks in the menu */
	pPreviewMenu->setItemChecked(0, 0 == iID);
	pPreviewMenu->setItemChecked(1, 1 == iID);
	pPreviewMenu->setItemChecked(2, 2 == iID);
	pPreviewMenu->setItemChecked(3, 3 == iID);
}

void StationsDlg::OnGetUpdate()
{
    QString path;
    QString fname = (eModulation==DRM)?DRMSCHEDULE_INI_FILE_NAME:AMSCHEDULE_INI_FILE_NAME;
    if (QMessageBox::question(this, tr("Dream Schedule Update"),
        QString(tr("Dream tries to download the newest schedule\nfrom the internet.\n\n"
            "The current file %1 will be overwritten.\n"
            "Do you want to continue?")).arg(fname),
            QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok)
    {
        return;
    }
    if(eModulation==DRM)
    {
        path = DRM_SCHEDULE_UPDATE_URL;
    }
    else
    {
        QDate d = QDate::currentDate();
        int wk = d.weekNumber();
        int yr = d.year();
        QString y,w;
        if(wk <= 13)
        {
            w = "b";
            y = QString::number(yr-1);
        }
        else if(wk <= 43)
        {
            w = "a";
            y = QString::number(yr);
        }
        else
        {
            w = "b";
            y = QString::number(yr);
        }
        path = QString(AM_SCHEDULE_UPDATE_URL).arg(w, y.right(2));
    }
    /* Try to download the current schedule. */
    QNetworkReply * reply = networkManager->get(QNetworkRequest(QUrl(path)));
    if(reply == NULL)
    {
        //cerr << "bad request " << path.toStdString() << endl;
        return;
    }
}

void StationsDlg::OnUrlFinished(QNetworkReply* reply)
{
    QString fname = (eModulation==DRM)?DRMSCHEDULE_INI_FILE_NAME:AMSCHEDULE_INI_FILE_NAME;

	/* Check that pointer points to valid object */
	if (reply)
	{
		if (reply->error() != QNetworkReply::NoError)
		{
            QMessageBox::information(this, "Dream", QString(tr(
                "Update failed. The following things may caused the failure:\n"
                "\t- the internet connection was not set up properly\n"
                "\t- the server is currently not available\n"
                "\t- the file %1 could not be written")).arg(fname));
			return;
		}

        QMessageBox::information(this, "Dream", tr("Update successful."));
        QFile f(fname);
        f.open(QIODevice::WriteOnly);
        f.write(reply->readAll());
        f.close();
        /* Read updated ini-file */
        Schedule.load(fname.toStdString());

		/* Add last update information on menu item */

		pUpdateMenu->setItemEnabled(0, true);

		/* init with empty string in case there is no schedule file */
		QString s = "";

		/* get time and date information */
		QFileInfo fi = QFileInfo(fname);
		if (fi.exists()) /* make sure the schedule file exists */
		{
			/* use QT-type of data string for displaying */
			s = tr(" (last update: ")
				+ fi.lastModified().date().toString() + ")";
		}

		pUpdateMenu->changeItem(0, tr("&Get Update") + s + "...");
	}
}

void StationsDlg::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timers */
	TimerList.stop();
	TimerUTCLabel.stop();
	TimerSMeter.stop();
	EnableSMeter(false);

	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	CWinGeom c;
	c.iXPos = WinGeom.x();
	c.iYPos = WinGeom.y();
	c.iHSize = WinGeom.height();
	c.iWSize = WinGeom.width();
	Settings.Put("Stations Dialog", c);

	/* Store preview settings */
	Settings.Put("Stations Dialog", "preview", Schedule.GetSecondsPreview());

	/* Store sort settings */
	switch (eModulation)
	{
	case DRM:
		Settings.Put("Stations Dialog", "sortcolumndrm", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendingdrm", bCurrentSortAscending);
		break;

	case NONE: // not really likely
        break;

	default:
		Settings.Put("Stations Dialog", "sortcolumnanalog", iCurrentSortColumn);
		Settings.Put("Stations Dialog", "sortascendinganalog", bCurrentSortAscending);
		break;
	}
}

void StationsDlg::showEvent(QShowEvent*)
{
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("Stations Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

    QString fname = (eModulation==DRM)?DRMSCHEDULE_INI_FILE_NAME:AMSCHEDULE_INI_FILE_NAME;
	QwtCounterFrequency->setValue(Receiver.GetFrequency());

	/* Load the schedule if necessary */
	if (Schedule.rowCount() == 0)
		Schedule.load(fname.toStdString());

	ComboBoxFilterTarget->insertStringList(Schedule.ListTargets);
	ComboBoxFilterCountry->insertStringList(Schedule.ListCountries);
	ComboBoxFilterLanguage->insertStringList(Schedule.ListLanguages);

	/* If number of stations is zero, we assume that the ini file is missing */
	if (Schedule.rowCount() == 0)
	{
        QMessageBox::information(this, "Dream", QString(tr(
            "The file %1 could not be found or contains no data.\n"
            "No stations can be displayed.\n"
            "Try to download this file by using the 'Update' menu.")).arg(fname));
	}

	/* Update window */
	OnTimerUTCLabel();
	OnTimerList();

	/* Activate timers when window is shown */
	TimerList.start(GUI_TIMER_LIST_VIEW_STAT); /* Stations list */
	TimerUTCLabel.start(GUI_TIMER_UTC_TIME_LABEL);
	TimerSMeter.start(GUI_TIMER_S_METER);
	TimerMonitorFrequency.start(GUI_TIMER_UPDATE_FREQUENCY);
}

void StationsDlg::OnTimerList()
{
	Schedule.update();
}

void StationsDlg::SetStationsView()
{
	size_t i;
	/* Stop the timer and disable the list */
	TimerList.stop();

	const bool bListFocus = stationsView->hasFocus();

	stationsView->setUpdatesEnabled(false);
	stationsView->setEnabled(false);

	/* Set lock because of list view items. These items could be changed
	   by another thread */
	ListItemsMutex.lock();
	Schedule.update();
	ListItemsMutex.unlock();

	/* Start the timer and enable the list */
	stationsView->setUpdatesEnabled(true);
	stationsView->setEnabled(true);

	if (bListFocus == true)
		stationsView->setFocus();

	TimerList.start(GUI_TIMER_LIST_VIEW_STAT);
}

void StationsDlg::OnFreqCntNewValue(double)
{
	// wait an inter-digit timeout
	TimerEditFrequency.start(GUI_TIMER_INTER_DIGIT, true);
	bTuningInProgress = true;
}

void StationsDlg::OnTimerEditFrequency()
{
	// commit the frequency if different
	int iTunedFrequency = Receiver.GetFrequency();
	int iDisplayedFreq = (int)QwtCounterFrequency->value();
	if(iTunedFrequency != iDisplayedFreq)
	{
		Receiver.SetFrequency(iDisplayedFreq);
		bTuningInProgress = true;
		TimerTuning.start(GUI_TIME_TO_TUNE, true);
	}

#if 0
	Q3ListViewItem* item = stationsView->selectedItem();
	if(item)
	{
		if(QString(item->text(2)).toInt() != iDisplayedFreq)
			stationsView->clearSelection();
	}
#endif
}

void StationsDlg::OnTimerTuning()
{
	bTuningInProgress = false;
}

void StationsDlg::OnTimerMonitorFrequency()
{
	/* Update frequency edit control
	 * frequency could be changed by evaluation dialog
	 * or RSCI
	 */
	int iTunedFrequency = Receiver.GetFrequency();
	int iDisplayedFreq = (int)QwtCounterFrequency->value();
	if(iTunedFrequency == iDisplayedFreq)
	{
		bTuningInProgress = false;
	}
	else
	{
		if(bTuningInProgress == false)
			QwtCounterFrequency->setValue(iTunedFrequency);
	}
	CParameter& Parameters = *Receiver.GetParameters();
	Parameters.Lock();
	EModulationType eNewMode = Parameters.eModulation;
	Parameters.Unlock();
	if (eModulation != eNewMode)
	{
		hide();
	}
}

void StationsDlg::OnItemClicked(const QModelIndex& item)
{
	/* Third column (column 2) of stationsView is frequency.
	   Set value in frequency counter control QWT. Setting this parameter
	   will emit a "value changed" signal which sets the new frequency.
	   Therefore, here no call to "SetFrequency()" is needed.*/
	QwtCounterFrequency->setValue(item.sibling(item.row(),2).data().toInt());
}

void StationsDlg::OnTimerSMeter()
{
	bool bSMeter = Receiver.GetEnableSMeter();
	EnableSMeter(bSMeter);
}

void StationsDlg::EnableSMeter(const bool bStatus)
{
	/* Need both, GUI "enabled" and signal strength valid before s-meter is used */
	_REAL rSigStr;
	Receiver.GetParameters()->Lock();
	bool bValid = Receiver.GetParameters()->Measurements.SigStrstat.getCurrent(rSigStr);
	Receiver.GetParameters()->Unlock();

	if((bStatus == true) && (bValid == true))
	{
		/* Init progress bar for input s-meter */
		ProgrSigStrength->setAlarmEnabled(true);
		ProgrSigStrength->setValue(rSigStr);
		ProgrSigStrength->setFillColor(QColor(0, 190, 0));

		ProgrSigStrength->setEnabled(true);
		TextLabelSMeter->setEnabled(true);
		ProgrSigStrength->show();
		TextLabelSMeter->show();
	}
	else
	{
		/* Set s-meter control in "disabled" status */
		ProgrSigStrength->setAlarmEnabled(false);
		ProgrSigStrength->setValue(S_METER_THERMO_MAX);
		ProgrSigStrength->setFillColor(palette().disabled().light());

		ProgrSigStrength->setEnabled(false);
		TextLabelSMeter->setEnabled(false);
		ProgrSigStrength->hide();
		TextLabelSMeter->hide();
	}
}

void StationsDlg::AddWhatsThisHelp()
{
	/* Stations List */
	stationsView->setWhatsThis(
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
	QwtCounterFrequency->setWhatsThis(
		tr("<b>Frequency Counter:</b> The current frequency "
		"value can be changed by using this counter. The tuning steps are "
		"100 kHz for the  buttons with three arrows, 10 kHz for the "
		"buttons with two arrows and 1 kHz for the buttons having only "
		"one arrow. By keeping the button pressed, the values are "
		"increased / decreased automatically."));

	/* UTC time label */
	TextLabelUTCTime->setWhatsThis(
		tr("<b>UTC Time:</b> Shows the current Coordinated "
		"Universal Time (UTC) which is nearly the same as Greenwich Mean Time "
		"(GMT)."));

#ifdef HAVE_LIBHAMLIB
	/* S-meter */
	const QString strSMeter =
		tr("<b>Signal-Meter:</b> Shows the signal strength "
		"level in dB relative to S9.<br>Note that not all "
		"front-ends controlled by hamlib support this feature. If the s-meter "
		"is not available, the controls are disabled.");

	TextLabelSMeter->setWhatsThis(strSMeter);
	ProgrSigStrength->setWhatsThis(strSMeter);
#endif
}
