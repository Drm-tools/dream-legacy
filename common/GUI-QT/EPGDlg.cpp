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
#include <qsocketdevice.h>

static _BOOLEAN IsActive(const QString& start, const QString& duration, const tm& now);

EPGDlg::EPGDlg(CDRMReceiver& NDRMR, CSettings& NSettings, QWidget* parent,
               const char* name, bool modal, WFlags f)
        :CEPGDlgbase(parent, name, modal, f),BitmCubeGreen(),date(QDate::currentDate()),
        do_updates(false),epg(*NDRMR.GetParameters()),DRMReceiver(NDRMR),
        Settings(NSettings),Timer(),sids(),next(NULL)
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
    connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));
    connect(this, SIGNAL(NowNext(QString)), this, SLOT(sendNowNext(QString)));
    connect(Prev, SIGNAL(clicked()), this, SLOT(previousDay()));
    connect(Next, SIGNAL(clicked()), this, SLOT(nextDay()));
    connect(channel, SIGNAL(activated(const QString&)), this
            , SLOT(selectChannel(const QString&)));
    connect(day, SIGNAL(valueChanged(int)), this, SLOT(setDay(int)));
    connect(month, SIGNAL(valueChanged(int)), this, SLOT(setMonth(int)));
    connect(year, SIGNAL(valueChanged(int)), this, SLOT(setYear(int)));

    /* Deactivate real-time timer */
    Timer.stop();

    day->setMinValue(1);
    day->setMaxValue(31);
    month->setMinValue(1);
    month->setMaxValue(12);
    year->setMinValue(0000);
    year->setMaxValue(3000);

	TextEPGDisabled->hide();
}

EPGDlg::~EPGDlg()
{
}

void EPGDlg::setActive(QListViewItem* myItem)
{
	MyListViewItem* item = dynamic_cast<MyListViewItem*>(myItem);
	if(item->IsActive())
	{
		item->setPixmap(COL_START, BitmCubeGreen);
		Data->ensureItemVisible(myItem);
		emit NowNext(item->text(COL_NAME));
		next = item->itemBelow();
	}
	else
	{
		item->setPixmap(COL_START,QPixmap()); /* no pixmap */
	}
}

void EPGDlg::sendNowNext(QString s)
{
	int port = -1; // disable the facility - edit Dream.ini to enable
	string addr = Settings.Get("NowNext", "address", string("127.0.0.1"));
	port = Settings.Get("NowNext", "port", port);
	if(port==-1)
		return;
	Settings.Put("NowNext", "address", addr);
	Settings.Put("NowNext", "port", port);
	QSocketDevice sock(QSocketDevice::Datagram);
	sock.writeBlock(s.utf8(), s.length(), QHostAddress(addr.c_str()), port);
}

void EPGDlg::OnTimer()
{
    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    tm gmtCur = *gmtime(&ltime);

    if(gmtCur.tm_sec==30) // 1/2 minute boundary
    {
	if(next)
	{
		emit NowNext(QString("next: ")+next->text(COL_NAME));
	}
    }
    if(gmtCur.tm_sec==0) // minute boundary
    {
        /* today in UTC */
        QDate todayUTC = QDate(gmtCur.tm_year + 1900, gmtCur.tm_mon + 1, gmtCur.tm_mday);

        if ((basic->text() == tr("no basic profile data"))
                || (advanced->text() == tr("no advanced profile data")))
        {
            /* not all information is loaded */
            select();
        }
	next = NULL;

        if (date == todayUTC) /* if today */
        {
            /* Extract values from the list */
            QListViewItem * myItem = Data->firstChild();

            while( myItem )
            {
                /* Check, if the programme is now on line. */
                if (myItem)
			setActive(myItem);

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
    epg.progs.clear ();
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
    epg.progs.clear ();
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

void EPGDlg::select()
{
    QListViewItem* CurrActiveItem = NULL;

    if (!do_updates)
        return;
    Data->clear();
    basic->setText(tr("no basic profile data"));
    advanced->setText(tr("no advanced profile data"));
    QString chan = channel->currentText();
    // get schedule for date +/- 1 - will allow user timezones sometime
    QDomDocument *doc;
    QDate o = date.addDays(-1);
    doc = epg.getFile (o, sids[chan], false);
    if(doc)
		epg.parseDoc(*doc);
    doc = epg.getFile (o, sids[chan], true);
    if(doc)
		epg.parseDoc(*doc);
    o = date.addDays(1);
    doc = epg.getFile (o, sids[chan], false);
    if(doc)
		epg.parseDoc(*doc);
    doc = epg.getFile (o, sids[chan], true);
    if(doc)
		epg.parseDoc(*doc);

    QString xml;
    doc = epg.getFile (date, sids[chan], false);
    if(doc)
    {
		epg.parseDoc(*doc);
		xml = doc->toString();
		if (xml.length() > 0)
			basic->setText(xml);
    }

    doc = epg.getFile (date, sids[chan], true);
    if(doc)
    {
		epg.parseDoc(*doc);
		xml = doc->toString();
		if (xml.length() > 0)
			advanced->setText(xml);
    }

    if (epg.progs.count()==0) {
        (void) new QListViewItem(Data, tr("no data"));
        return;
    }
    Data->setSorting(COL_START);

    for (QMap < time_t, EPG::CProg >::Iterator i = epg.progs.begin();
            i != epg.progs.end(); i++)
    {
        const EPG::CProg & p = i.data();
        // TODO - let user choose time or actualTime if available, or show as tooltip
        time_t start;
        int duration;
        if (p.actualTime!=0)
        {
            start = p.actualTime;
            duration = p.actualDuration;
        }
        else
        {
            start = p.time;
            duration = p.duration;
        }
        QString s_start, s_duration;
        tm bdt = *gmtime(&start);

	// skip entries not on the wanted day
        if((bdt.tm_year+1900) != date.year())
        {
        	continue;
        }
        if((bdt.tm_mon+1) != date.month())
        {
        	continue;
        }
        if(bdt.tm_mday != date.day())
        {
        	continue;
        }

        char s[40];
        sprintf(s, "%02d:%02d", bdt.tm_hour, bdt.tm_min);
        s_start = s;
        int min = duration / 60;
        sprintf(s, "%02d:%02d", int(min/60), min%60);
        s_duration = s;
        QString name, description, genre;
        if (p.name=="" && p.mainGenre.size()>0)
            name = "unknown " + p.mainGenre[0] + " programme";
        else
            name = p.name;
        description = p.description;
        // collapse white space in description
        description.replace(QRegExp("[\t\r\n ]+"), " ");
        if (p.mainGenre.size()==0)
            genre = "";
        else
        {
            QString sep="";
            for (size_t i=0; i<p.mainGenre.size(); i++) {
                if (p.mainGenre[i] != "Audio only") {
                    genre = genre+sep+p.mainGenre[i];
                    sep = ", ";
                }
            }
        }
        MyListViewItem* CurrItem = new MyListViewItem(Data, s_start, name, genre, description, s_duration,
        start, duration);
    /* Check, if the programme is now on line. If yes, set
    special pixmap */
		if (CurrItem->IsActive())
		{
			CurrActiveItem = CurrItem;
		}
    }
    if (CurrActiveItem) /* programme is now on line */
        setActive(CurrActiveItem);
}

_BOOLEAN EPGDlg::MyListViewItem::IsActive()
{
	time_t now = time(NULL);
	if(now<start)
		return false;
	if(now>=(start+duration))
		return false;
	return true;
}

static _BOOLEAN IsActive(const QString& start, const QString& duration, const tm& now)
{
    QStringList sl = QStringList::split(":", start);
    QStringList dl = QStringList::split(":", duration);
    int s = 60*sl[0].toInt()+sl[1].toInt();
    int e = s + 60*dl[0].toInt()+dl[1].toInt();
    int n = 60*now.tm_hour+now.tm_min;
    if ((s <= n) && (n < e))
        return TRUE;
    else
        return FALSE;
}
