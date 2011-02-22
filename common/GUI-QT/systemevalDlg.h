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
#if QT_VERSION < 0x040000
# include <qpopupmenu.h>
# include <qlistview.h>
typedef QListView Q3ListView;
typedef QListViewItem Q3ListViewItem;
# define Q3PopupMenu QPopupMenu
#include <qwt/qwt_thermo.h>
#else
# include <q3listview.h>
# include <q3buttongroup.h>
# include <q3popupmenu.h>
#include <qwt_thermo.h>
#endif
#include <qpixmap.h>

#include "systemevalDlgbase.h"
#include "DRMPlot.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "../ReceptLog.h"
#include "../util/Settings.h"

class CGPSReceiver;
class CRig;

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/
class systemevalDlg : public systemevalDlgBase
{
	Q_OBJECT

public:
	systemevalDlg(CDRMReceiver&, CRig&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);

	virtual ~systemevalDlg();

	void SetStatus(int MessID, int iMessPara);
	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	void UpdatePlotsStyle();
	void StopLogTimers();

protected:
	class CCharSelItem : public Q3ListViewItem
	{
	public:
		CCharSelItem(Q3ListView* parent, QString str1,
			CDRMPlot::ECharType eNewCharTy, _BOOLEAN bSelble = TRUE) :
			Q3ListViewItem(parent, str1), eCharTy(eNewCharTy)
			{setSelectable(bSelble);}
		CCharSelItem(Q3ListViewItem* parent, QString str1,
			CDRMPlot::ECharType eNewCharTy, _BOOLEAN bSelble = TRUE) :
			Q3ListViewItem(parent, str1), eCharTy(eNewCharTy)
			{setSelectable(bSelble);}

		CDRMPlot::ECharType GetCharType() {return eCharTy;}

	protected:
		CDRMPlot::ECharType eCharTy;
	};

	CDRMReceiver&		DRMReceiver;
	CSettings&		Settings;

	QTimer			Timer;
	QTimer			TimerInterDigit;

	/* logging */
	QTimer			TimerLogFileLong;
	QTimer			TimerLogFileShort;
	QTimer			TimerLogFileStart;

	CShortLog		shortLog;
	CLongLog		longLog;
	int				iLogDelay;
	CRig&			rig;

	virtual void		showEvent(QShowEvent* pEvent);
	virtual void		hideEvent(QHideEvent* pEvent);
	void			UpdateControls();
	void			AddWhatsThisHelp();
	CDRMPlot*		OpenChartWin(const CDRMPlot::ECharType eNewType);

	QString			GetRobModeStr();
	QString			GetSpecOccStr();

	Q3PopupMenu*		pListViewContextMenu;
	vector<CDRMPlot*>	vecpDRMPlots;

	CGPSReceiver*		pGPSReceiver;

public slots:
	void OnTimer();
	void OnTimerInterDigit();
	void OnTimerLogFileStart();
	void OnTimerLogFileShort();
	void OnTimerLogFileLong();
	void OnRadioTimeLinear();
	void OnRadioTimeWiener();
	void OnRadioFrequencyLinear();
	void OnRadioFrequencyDft();
	void OnRadioFrequencyWiener();
	void OnRadioTiSyncFirstPeak();
	void OnRadioTiSyncEnergy();
	void OnSliderIterChange(int value);
	void OnCheckFlipSpectrum();
	void OnCheckBoxMuteAudio();
	void OnCheckBoxReverb();
	void OnCheckWriteLog();
	void OnCheckSaveAudioWAV();
	void OnCheckRecFilter();
	void OnCheckModiMetric();
	void OnListViContMenu();
	void OnFrequencyEdited (const QString&);
#if QT_VERSION < 0x040000
	void OnListSelChanged(QListViewItem* NewSelIt);
	void OnListRightButClicked(QListViewItem* NewSelIt, const QPoint& iPnt, int iCol);
#else
	void OnListSelChanged(Q3ListViewItem* NewSelIt);
	void OnListRightButClicked(Q3ListViewItem* NewSelIt, const QPoint& iPnt, int iCol);
#endif
	void EnableGPS();
	void DisableGPS();
};
