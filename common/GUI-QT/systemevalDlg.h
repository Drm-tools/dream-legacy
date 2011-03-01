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
#include <qwt_thermo.h>
#include <qpixmap.h>

#if QT_VERSION < 0x040000
# include <qpopupmenu.h>
# include <qlistview.h>
# define Q3ListView QListView
# define Q3ListViewItem QListViewItem
# define Q3PopupMenu QPopupMenu
# include "systemevalDlgbase.h"
#else
# include <Q3ListView>
# include <Q3ButtonGroup>
# include <Q3PopupMenu>
# include <QDialog>
# include "ui_systemevalDlgbase.h"
#endif

#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../DrmReceiver.h"
#include "../ReceptLog.h"
#include "../util/Settings.h"

class CGPSReceiver;
class CRig;
class CDRMPlot;

/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE


/* Classes ********************************************************************/
class systemevalDlg :
#if QT_VERSION < 0x040000
	public systemevalDlgBase
#else
	public QDialog, public Ui_systemevalDlgBase
#endif
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
	CDRMReceiver&		DRMReceiver;
	CSettings&		Settings;

	CDRMPlot*		MainPlot;

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
	CDRMPlot*		OpenChartWin(int);

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
	void OnListSelChanged(Q3ListViewItem* NewSelIt);
	void OnListRightButClicked(Q3ListViewItem* NewSelIt, const QPoint& iPnt, int iCol);
	void EnableGPS();
	void DisableGPS();
};
