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
#include <QRegExp>
#include <ctime>

EPGDlg::EPGDlg(ReceiverInterface& NDRMR, CSettings& NSettings, QWidget* parent,
               const char* name, bool modal, Qt::WFlags f)
:QDialog(parent, name, modal, f),Ui_EPGDlg(),BitmCubeGreen(),
date(QDate::currentDate()),do_updates(false),
epg(*NDRMR.GetParameters()),DRMReceiver(NDRMR),
Settings(NSettings),Timer(),currentSID(0)
{
    setupUi(this);

	/* Create 8x8 bitmap */
	BitmCubeGreen.resize(8, 8);
	BitmCubeGreen.fill(QColor(0, 255, 0));

	/* Connections ---------------------------------------------------------- */
	connect(dateEdit, SIGNAL(dateChanged(const QDate&)), this, SLOT(setDate(const QDate&)));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	/* Deactivate real-time timer */
	Timer.stop();


	/* TODO show a label if EPG decoding is disabled */
	//if (DRMReceiver.GetDataDecoder()->GetDecodeEPG() == true)
	if(true)
		TextEPGDisabled->hide();
	else
		TextEPGDisabled->show();
    dateEdit->setCalendarPopup(true);
}

EPGDlg::~EPGDlg()
{
}

void EPGDlg::OnTimer()
{
    /* Get current UTC time */
    time_t ltime;
    time(&ltime);
    tm gmtCur = *gmtime(&ltime);

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

		if (date == todayUTC) /* if today */
		{
			/* Extract values from the list */
			Q3ListViewItem * myItem = Data->firstChild();

			while( myItem )
			{
				/* Check, if the programme is now on line. If yes, set
				special pixmap */
				if (IsActive(myItem->text(COL_START), myItem->text(COL_DURATION), gmtCur))
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

	/* recover window size and position */
	CWinGeom s;
	Settings.Get("EPG Dialog", s);
	const QRect WinGeom(s.iXPos, s.iYPos, s.iWSize, s.iHSize);
	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	/* auto resize of the programme name column */
	Data->setColumnWidthMode(COL_NAME, Q3ListView::Maximum);

    /* restore selected service */
    bool ok=false;
    currentSID = QString(Settings.Get("EPG Dialog", "serviceid", string("0")).c_str()).toULong(&ok, 16);
    if(!ok)
        currentSID=0;
    // update the channels combobox from the epg
    channel->clear();
    int n = -1, m = -1;
	Parameters.Lock();
    for (map < uint32_t, CServiceInformation >::const_iterator i = Parameters.ServiceInformation.begin();
         i != Parameters.ServiceInformation.end(); i++) {
		QString channel_label = QString().fromUtf8(i->second.label.begin()->c_str());
		uint32_t channel_id = i->second.id;
    	channel->insertItem(++n, channel_label, channel_id);
    	if(channel_id == currentSID)
    	{
    	    m = n;
    	}
    }
	Parameters.Unlock();
	if(m>=0)
        channel->setCurrentIndex(m);

    do_updates = true;

    date = QDate::currentDate();
    dateEdit->setDate(date);

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
    Settings.Put("EPG Dialog", "serviceid", QString("%1").arg(currentSID, 16).toStdString());
}

void EPGDlg::setDate(const QDate& d)
{
    date = d;
    select();
}

void EPGDlg::selectChannel(const QString &)
{
    select();
}

void EPGDlg::select()
{
	Q3ListViewItem* CurrActiveItem = NULL;

    if (!do_updates)
	    return;
    Data->clear();
    basic->setText(tr("no basic profile data"));
    advanced->setText(tr("no advanced profile data"));
    CDateAndTime d;
    d.year = date.year();
    d.month = date.month();
    d.day = date.day();

    currentSID = channel->itemData(channel->currentIndex()).toUInt();
    epg.select(currentSID, d);

    if(epg.progs.count()==0) {
	    (void) new Q3ListViewItem(Data, tr("no data"));
	    return;
    }
    Data->setSorting(COL_START);

    for (QMap < QDateTime, EPG::CProg >::Iterator i = epg.progs.begin();
	 i != epg.progs.end(); i++)
	{
	    const EPG::CProg & p = i.data();
	    QString name, description, genre;
	    if(p.name=="" && p.mainGenre.size()>0)
		name = "unknown " + p.mainGenre[0] + " programme";
		else
          name = p.name;
        description = p.description;
        // collapse white space in description
        description.replace(QRegExp("[\t\r\n ]+"), " ");
        // TODO - let user choose time or actualTime if available, or show as tooltip
        QDateTime start;
        int duration;
        if(p.actualTime.isValid())
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
        {
            char s[40];
            sprintf(s, "%02d:%02d", start.time().hour(), start.time().minute());
            s_start = s;
            sprintf(s, "%02d:%02d", int(duration/60), duration%60);
            s_duration = s;
        }
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
	    Q3ListViewItem* CurrItem = new Q3ListViewItem(Data, s_start, name, genre, description, s_duration);

		/* Get current UTC time */
		time_t ltime;
		time(&ltime);
        tm gmtCur = *gmtime(&ltime);

		/* today in UTC */
		QDate todayUTC = QDate(gmtCur.tm_year + 1900, gmtCur.tm_mon + 1, gmtCur.tm_mday);

		if (date == todayUTC) /* if today */
		{
			/* Check, if the programme is now on line. If yes, set
			special pixmap */
			if (IsActive(s_start, s_duration, gmtCur))
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

bool EPGDlg::IsActive(const QString& start, const QString& duration, const tm& now)
{
    QStringList sl = QStringList::split(":", start);
    QStringList dl = QStringList::split(":", duration);
    int s = 60*sl[0].toInt()+sl[1].toInt();
    int e = s + 60*dl[0].toInt()+dl[1].toInt();
    int n = 60*now.tm_hour+now.tm_min;
	if ((s <= n) && (n < e))
		return true;
	else
		return false;
}
