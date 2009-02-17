/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer, Andrea Russo
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


#include <QDialog>
#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "ui_DRMDialog.h"
#include "DialogUtil.h"
#include "systemevalDlg.h"
#include "MultimediaDlg.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "Loghelper.h"
#include "EPGDlg.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "MultColorLED.h"
#include "../util/Vector.h"
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>
#include <Q3PopupMenu>
#include <QCloseEvent>


/* Define for application types */
#define AT_MOTSLISHOW 2
#define AT_MOTBROADCASTWEBSITE 3
#define AT_JOURNALINE 0x44A
#define AT_MOTEPG 	7

/* Classes ********************************************************************/
class ReceiverSettingsDlg;

class DRMDialog : public QDialog, public Ui_DRMDialog
{
	Q_OBJECT

public:
	DRMDialog(CDRMReceiver&, CSettings&, QWidget* parent = 0, const char* name = 0,
		bool modal = false,	Qt::WFlags f = 0);
	virtual ~DRMDialog();
	/* dummy assignment operator to help MSVC8 */
	DRMDialog& operator=(const DRMDialog&)
	{ throw "should not happen"; return *this;}

protected:
	CDRMReceiver&		DRMReceiver;
	CSettings&			Settings;

	systemevalDlg*		pSysEvalDlg;
	MultimediaDlg*		pMultiMediaDlg;
	StationsDlg*		pStationsDlg;
	LiveScheduleDlg*	pLiveScheduleDlg;
	EPGDlg*				pEPGDlg;
	AnalogDemDlg*		pAnalogDemDlg;
	ReceiverSettingsDlg* pReceiverSettingsDlg;
	Loghelper			loghelper;
	QMenuBar*			pMenu;
	Q3PopupMenu*			pReceiverModeMenu;
	Q3PopupMenu*			pSettingsMenu;
	Q3PopupMenu*			pPlotStyleMenu;
	int					iCurSelServiceGUI;
	int					iOldNoServicesGUI;
	QTimer				Timer;

	bool		bSysEvalDlgWasVis;
	bool		bMultMedDlgWasVis;
	bool		bLiveSchedDlgWasVis;
	bool		bStationsDlgWasVis;
	bool		bEPGDlgWasVis;
	EDemodulationType	eReceiverMode;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	virtual void	closeEvent(QCloseEvent* ce);
	virtual void	showEvent(QShowEvent* pEvent);
	void			hideEvent(QHideEvent* pEvent);
	void			SetService(int iNewServiceID);
	void			AddWhatsThisHelp();
	void			UpdateDisplay();
	void			ClearDisplay();

	QString			GetCodecString(const int iServiceID);
	QString			GetTypeString(const int iServiceID);

	void			SetDisplayColor(const QColor newColor);

	void			ChangeGUIModeToDRM();
	void			ChangeGUIModeToAM();

public slots:
	void OnTimer();
	void OnButtonService1();
	void OnButtonService2();
	void OnButtonService3();
	void OnButtonService4();
	void OnViewEvalDlg();
	void OnViewMultiMediaDlg();
	void OnViewStationsDlg();
	void OnViewLiveScheduleDlg();
	void OnViewEPGDlg();
	void OnViewMultSettingsDlg();
	void OnViewReceiverSettingsDlg();
	void OnNewDRMAcquisition();
	void OnSwitchToDRM();
	void OnSwitchToAM();
	void OnMenuSetDisplayColor();
	void OnMenuPlotStyle(int value);
};
