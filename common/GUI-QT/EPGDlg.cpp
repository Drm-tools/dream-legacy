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

#include "EPGDlg.h"
#include <qregexp.h>

EPGDlg::EPGDlg(CDRMReceiver& NDRMR, CSettings& NSettings, QWidget* parent,
               const char* name, bool modal, WFlags f)
:CEPGDlgbase(parent, name, modal, f),BitmCubeGreen(),date(QDate::currentDate()),
do_updates(false),epg(*NDRMR.GetParameters()),DRMReceiver(NDRMR),
Settings(NSettings),Timer(),sids()
{
	/* recover window size and position */
	CWinGeom s;
	Settings.Get("EPG Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	/* auto resize of the programme name column */
	Data->setColumnWidthMode(COL_NAME, QListView::Maximum);

	/* Define size of the bitmaps */
	const int iXSize = 8;
	const int iYSize = 8;

	/* Create bitmaps */
	BitmCubeGreen.resize(iXSize, iYSize);
	BitmCubeGreen.fill(QColor(0, 255, 0));

	/* Connections ---------------------------------------------------------- */
	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));

	/* Deactivate real-time timer */
	Timer.stop();

	connect(Prev, SIGNAL(clicked()), this, SLOT(previousDay()));
	connect(Next, SIGNAL(clicked()), this, SLOT(nextDay()));
	connect(channel, SIGNAL(activated(const QString&)), this
		, SLOT(selectChannel(const QString&)));
	connect(day, SIGNAL(valueChanged(int)), this, SLOT(setDay(int)));
	connect(month, SIGNAL(valueChanged(int)), this, SLOT(setMonth(int)));
	connect(year, SIGNAL(valueChanged(int)), this, SLOT(setYear(int)));

    day->setMinValue(1);
    day->setMaxValue(31);
    month->setMinValue(1);
    month->setMaxValue(12);
    year->setMinValue(0000);
    year->setMaxValue(3000);

	/* show a label if EPG decoding is disabled */
	if (DRMReceiver.GetDataDecoder()->GetDecodeEPG() == TRUE)
		TextEPGDisabled->hide();
	else
		TextEPGDisabled->show();
}

EPGDlg::~EPGDlg()
{
}

void EPGDlg::OnTimer()
{
    if ((basic->text() == tr("no basic profile data"))
		|| (advanced->text() == tr("no advanced profile data")))
		/* not all information is loaded */
 		select();
	else
	{
		/* Get current UTC time */
		time_t ltime;
		time(&ltime);
		struct tm* gmtCur = gmtime(&ltime);

		/* today in UTC */
		QDate todayUTC = QDate(gmtCur->tm_year + 1900, gmtCur->tm_mon + 1, gmtCur->tm_mday);

		if (date == todayUTC) /* if today */
		{
			/* Extract values from the list */
			QListViewItem * myItem = Data->firstChild();

			while( myItem )
			{
				/* Check, if the programme is now on line. If yes, set
				special pixmap */
				if (IsActive(myItem->text(COL_START),myItem->text(COL_DURATION),ltime))
				{
					myItem->setPixmap(COL_START, BitmCubeGreen);
					Data->ensureItemVisible(myItem);
				}
				else
					myItem->setPixmap(COL_START,QPixmap()); /* no pixmap */
				myItem = myItem->nextSibling();
			}
		}
	}
}

void EPGDlg::showEvent(QShowEvent *)
{
    CParameter& Parameters = *DRMReceiver.GetParameters();
	Parameters.Lock();
    int sNo = Parameters.GetCurSelAudioService();
    uint32_t sid = Parameters.Service[sNo].iServiceID;

    // use the current date
    date = QDate::currentDate();
    // update the channels combobox from the epg
    channel->clear();
    int n = -1;
	sids.clear();
    for (map < uint32_t, CServiceInformation >::const_iterator i = Parameters.ServiceInformation.begin();
         i != Parameters.ServiceInformation.end(); i++) {
		QString channel_label = QString().fromUtf8(i->second.label.begin()->c_str());
		uint32_t channel_id = i->second.id;
		sids[channel_label] = channel_id;
    	channel->insertItem(channel_label);
    	if (channel_id == sid) {
    	    n = channel->currentItem();
        }
    }
	Parameters.Unlock();
    // update the current selection
    if (n >= 0) {
	    channel->setCurrentItem(n);
    }
    do_updates = true;
    setDate();
    select();

	/* Activate real-time timer when window is shown */
	Timer.start(GUI_TIMER_EPG_UPDATE);
}

void EPGDlg::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer */
	Timer.stop();

	CWinGeom s;
	QRect WinGeom = geometry();
	s.iXPos = WinGeom.x();
	s.iYPos = WinGeom.y();
	s.iHSize = WinGeom.height();
	s.iWSize = WinGeom.width();
	Settings.Put("EPG Dialog", s);
}

void EPGDlg::previousDay()
{
    date = date.addDays(-1);
    setDate();
}

void EPGDlg::nextDay()
{
    date = date.addDays(1);
    setDate();
}

void EPGDlg::setDate()
{
    day->setValue(date.day());
    month->setValue(date.month());
    year->setValue(date.year());
}

void EPGDlg::selectChannel(const QString &)
{
    select();
}

void EPGDlg::setDay(int n)
{
    date.setYMD(date.year(), date.month(), n);
    setDate();
    select();
}

void EPGDlg::setMonth(int n)
{
    date.setYMD(date.year(), n, date.day());
    setDate();
    select();
}

void EPGDlg::setYear(int n)
{
    date.setYMD(n, date.month(), date.day());
    setDate();
    select();
}

static QString parseStart (const QString & start)
{
	QStringList
		d = QStringList::split ('+', start, true);
	QStringList
		t = QStringList::split ('T', d[0], true);
	QStringList
		hms = QStringList::split (':', t[1], true);
	if (hms[2] == "00")
		return hms[0] + ":" + hms[1];
	else
		return t[1];
}

static QString parseDuration (const QString & duration)
{
	QRegExp r ("[PTHMS]");
	QStringList dur = QStringList::split (r, duration);
	return QCString ("").sprintf ("%02u:%02u", dur[0].toInt (), dur[1].toInt ());
}

void EPGDlg::select()
{
	QListViewItem* CurrActiveItem = NULL;

    if (!do_updates)
	    return;
    Data->clear();
    basic->setText(tr("no basic profile data"));
    advanced->setText(tr("no advanced profile data"));
    QString chan = channel->currentText();
    CDateAndTime d;
    d.year = date.year();
    d.month = date.month();
    d.day = date.day();
    epg.select(sids[chan], d);
    if(epg.progs.count()==0) {
	    (void) new QListViewItem(Data, tr("no data"));
	    return;
    }
    Data->setSorting(COL_START);

    for (QMap < QDateTime, EPG::CProg >::Iterator i = epg.progs.begin();
	 i != epg.progs.end(); i++)
	{
	    const EPG::CProg & p = i.data();
	    QString name, description, start, duration, genre;
	    if(p.name=="" && p.mainGenre.size()>0)
		name = "unknown " + p.mainGenre[0] + " programme";
		else
          name = p.name;
        description = p.description;
        // collapse white space in description
        description.replace(QRegExp("[\t\r\n ]+"), " ");
        // TODO - let user choose time or actualTime if available, or show as tooltip
        start = parseStart(p.time);
        duration = parseDuration(p.duration);
		if(p.mainGenre.size()==0)
			genre = "";
		else
		{
			QString sep="";
			for(size_t i=0; i<p.mainGenre.size(); i++) {
				if(p.mainGenre[i] != "Audio only") {
					genre = genre+sep+p.mainGenre[i];
					sep = ", ";
				}
			}
		}
	    QListViewItem* CurrItem = new QListViewItem(Data, start, name, genre, description, duration);

		/* Get current UTC time */
		time_t ltime;
		time(&ltime);
		struct tm* gmtCur = gmtime(&ltime);

		/* today in UTC */
		QDate todayUTC = QDate(gmtCur->tm_year + 1900, gmtCur->tm_mon + 1, gmtCur->tm_mday);

		if (date == todayUTC) /* if today */
		{
			/* Check, if the programme is now on line. If yes, set
			special pixmap */
			if (IsActive(p.time,p.duration,ltime))
			{
				CurrItem->setPixmap(COL_START, BitmCubeGreen);
				CurrActiveItem = CurrItem;
			}
		}
	}

    QString xml;
    xml = epg.basic.doc.toString();
    if(xml.length() > 0)
        basic->setText(xml);

    xml = epg.advanced.doc.toString();
    if(xml.length() > 0)
        advanced->setText(xml);

	if (CurrActiveItem) /* programme is now on line */
		Data->ensureItemVisible(CurrActiveItem);
}

_BOOLEAN EPGDlg::IsActive(const QString start, const QString duration, const time_t ltime)
{
	/* extract hours and minutes from the strings hh:mm */
	int iStartHour = start.left(2).toInt();
	int iStartMinute = start.right(2).toInt();

	int iDurationHour = duration.left(2).toInt();
	int iDurationMinute = duration.right(2).toInt();

	/* Calculate time in UTC */
	struct tm* gmtCur = gmtime(&ltime);
	const time_t lCurTime = mktime(gmtCur);

	/* Get start time */
	struct tm* gmtStart = gmtime(&ltime);
	gmtStart->tm_hour = iStartHour;
	gmtStart->tm_min = iStartMinute;
	const time_t lStartTime = mktime(gmtStart);

	/* Get stop time */
	struct tm* gmtStop = gmtime(&ltime);
	gmtStop->tm_hour = iStartHour + iDurationHour;
	gmtStop->tm_min = iStartMinute + iDurationMinute;
	const time_t lStopTime = mktime(gmtStop);

	if ((lCurTime >= lStartTime) && (lCurTime < lStopTime))
		return TRUE;
	else
		return FALSE;
}
