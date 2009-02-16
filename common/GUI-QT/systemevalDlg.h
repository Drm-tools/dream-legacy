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

#include <qtimer.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <q3filedialog.h>
#include <q3whatsthis.h>
#include <q3listview.h>
#include <q3buttongroup.h>
#include <q3popupmenu.h>
#include <qpixmap.h>
#include <qwt_thermo.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>

#include "systemevalDlgbase.h"
#include "DRMPlot.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "../util/Settings.h"

class CGPSReceiver;

/* Definitions ****************************************************************/


/* Classes ********************************************************************/
class systemevalDlg : public systemevalDlgBase
{
	Q_OBJECT

public:
	systemevalDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);

	virtual ~systemevalDlg();

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	void UpdatePlotsStyle();
	void StopLogTimers();

protected:
	class CCharSelItem : public Q3ListViewItem
	{
	public:
		CCharSelItem(Q3ListView* parent, QString str1,
			CPlotManager::EPlotType eNewCharTy, bool bSelble = true) :
			Q3ListViewItem(parent, str1), eCharTy(eNewCharTy)
			{setSelectable(bSelble);}
		CCharSelItem(Q3ListViewItem* parent, QString str1,
			CPlotManager::EPlotType eNewCharTy, bool bSelble = true) :
			Q3ListViewItem(parent, str1), eCharTy(eNewCharTy)
			{setSelectable(bSelble);}

		CPlotManager::EPlotType GetCharType() {return eCharTy;}

	protected:
		CPlotManager::EPlotType eCharTy;
	};

	CDRMReceiver&		DRMReceiver;
	CSettings&			Settings;

	QTimer				Timer;
	QTimer				TimerLineEditFrequency;
	QTimer				TimerTuning;

    virtual void		showEvent(QShowEvent* pEvent);
	virtual void		hideEvent(QHideEvent* pEvent);
	void				UpdateControls();
	void				AddWhatsThisHelp();
	CDRMPlot*			OpenChartWin(const CPlotManager::EPlotType eNewType);

	QString				GetRobModeStr();
	QString				GetSpecOccStr();

	Q3PopupMenu*			pListViewContextMenu;
	vector<CDRMPlot*>	vecpDRMPlots;

	CGPSReceiver*		pGPSReceiver;

	bool			bTuningInProgress;

public slots:
	void OnTimer();
	void OnTimerLineEditFrequency();
	void OnTimerTuning();
	void OnListSelChanged(Q3ListViewItem* NewSelIt);
	void OnListViContMenu();
	void OnListRightButClicked(Q3ListViewItem* NewSelIt, const QPoint& iPnt,
		int iCol);
	void OnLineEditFrequencyChanged(const QString& str);
	void EnableGPS(bool);
	void ShowGPS(bool);
};
