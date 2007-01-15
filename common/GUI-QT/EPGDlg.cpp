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

EPGDlg::EPGDlg(CDRMReceiver* pNDRMR, QWidget* parent,
               const char* name, bool modal, WFlags f)
:CEPGDlgbase(parent, name, modal, f),epg(),pDRMRec(pNDRMR)
{

#ifdef _WIN32 /* This works only reliable under Windows :-( */
	/* Get window geometry data from DRMReceiver module and apply it */
	const QRect WinGeom(pDRMRec->GeomEPGDlg.iXPos,
		pDRMRec->GeomEPGDlg.iYPos,
		pDRMRec->GeomEPGDlg.iWSize,
		pDRMRec->GeomEPGDlg.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);
#else /* Under Linux only restore the size */
	resize(pDRMRec->GeomEPGDlg.iWSize,
		pDRMRec->GeomEPGDlg.iHSize);
#endif

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

	/* show a label is EPG decoding is disabled */
	if (pDRMRec->GetDataDecoder()->GetDecodeEPG() == TRUE)
		TextEPGDisabled->hide();
	else
		TextEPGDisabled->show();
}

EPGDlg::~EPGDlg()
{
	/* Set window geometry data in DRMReceiver module */
	QRect WinGeom = geometry();

	pDRMRec->GeomEPGDlg.iXPos = WinGeom.x();
	pDRMRec->GeomEPGDlg.iYPos = WinGeom.y();
	pDRMRec->GeomEPGDlg.iHSize = WinGeom.height();
	pDRMRec->GeomEPGDlg.iWSize = WinGeom.width();
}

void EPGDlg::OnTimer()
{	
    if ((basic->text() == tr("no basic profile data"))
		|| (advanced->text() == tr("no advanced profile data")))
		/* not all informations are loaded */
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
    // Use the currently receiving channel 
    CParameter* pP = pDRMRec->GetParameters();
    int sNo = pP->GetCurSelAudioService();
    CParameter::CService& s = pDRMRec->GetParameters()->Service[sNo];
    QString label = s.strLabel.c_str();
    if ((s.DataParam.iUserAppIdent == CDataDecoder::AT_MOTEPG)
		&& (label!=""))
    {
	epg.addChannel(label, s.iServiceID);
    }
    // use the current date
    date = QDate::currentDate();
    // update the channels combobox from the epg
    channel->clear();
    int n = -1;
    for (QMap < QString, uint32_t >::Iterator i = epg.sids.begin(); 
         i != epg.sids.end(); i++) {
    	channel->insertItem(i.key());
    	if (i.key() == label) {
    	    n = channel->currentItem();
        }
    }
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

void EPGDlg::select()
{
	QListViewItem* CurrActiveItem = NULL;

    if (!do_updates)
	    return;
    Data->clear();
    basic->setText(tr("no basic profile data"));
    advanced->setText(tr("no advanced profile data"));
    QString chan = channel->currentText();
    if(!epg.sids.contains(chan)) {
	    (void) new QListViewItem(Data, tr("no data"));
         return;
    }
    CDateAndTime d;
    d.year = date.year();
    d.month = date.month();
    d.day = date.day();
    epg.select(chan, d);
    if(epg.progs.count()==0) {
	    (void) new QListViewItem(Data, tr("no data"));
	    return;
    }
    Data->setSorting(COL_START);

    for (QMap < uint32_t, EPG::CProg >::Iterator i = epg.progs.begin();
	 i != epg.progs.end(); i++)
	{
	    const EPG::CProg & p = i.data();
	    QString name, genre;
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
	    if(p.name=="" && p.mainGenre.size()>0)
		name = "unknown " + p.mainGenre[0] + " programme";
		else
          name = p.name;
	    QListViewItem* CurrItem = new QListViewItem(Data, p.start, name, genre, p.description, p.duration);

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
			if (IsActive(p.start,p.duration,ltime))
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
