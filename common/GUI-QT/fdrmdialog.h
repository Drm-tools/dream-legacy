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

#include <qlabel.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qstring.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qwt/qwt_thermo.h>
#include <qevent.h>
#include <qcstring.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qpalette.h>
#include <qcolordialog.h>

#ifdef _WIN32
# include "../../Windows/moc/fdrmdialogbase.h"
#else
# include "moc/fdrmdialogbase.h"
#endif
#include "DialogUtil.h"
#include "systemevalDlg.h"
#include "MultimediaDlg.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "EPGDlg.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "GeneralSettingsDlg.h"
#include "MultColorLED.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"


/* Define for application types */
#define AT_MOTSLISHOW 2
#define AT_MOTBROADCASTWEBSITE 3
#define AT_JOURNALINE 0x44A
#define AT_MOTEPG 	7

/* Classes ********************************************************************/
class FDRMDialog : public FDRMDialogBase
{
	Q_OBJECT

public:
	FDRMDialog(CDRMReceiver* pNDRMR, QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE,	WFlags f = 0);

	virtual ~FDRMDialog();

protected:
	CDRMReceiver*		pDRMRec;

	systemevalDlg*		pSysEvalDlg;
	MultimediaDlg*		pMultiMediaDlg;
	StationsDlg*		pStationsDlg;
	LiveScheduleDlg*	pLiveScheduleDlg;
	EPGDlg*				pEPGDlg;
	AnalogDemDlg*		pAnalogDemDlg;
	QMenuBar*			pMenu;
	QPopupMenu*			pReceiverModeMenu;
	QPopupMenu*			pSettingsMenu;
	QPopupMenu*			pPlotStyleMenu;
	int					iCurSelServiceGUI;
	int					iOldNoServicesGUI;
	QTimer				Timer;

	_BOOLEAN		bSysEvalDlgWasVis;
	_BOOLEAN		bMultMedDlgWasVis;
	_BOOLEAN		bLiveSchedDlgWasVis;
	ERecMode		eReceiverMode;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	virtual void	customEvent(QCustomEvent* Event);
	virtual void	closeEvent(QCloseEvent* ce);
	virtual void	showEvent(QShowEvent* pEvent);
	void			hideEvent(QHideEvent* pEvent);
	void			SetService(int iNewServiceID);
	void			AddWhatsThisHelp();
	void			SetReceiverMode(const ERecMode eNewReMo);

	QString			GetCodecString(const int iServiceID);
	QString			GetTypeString(const int iServiceID);

	void			SetDisplayColor(const QColor newColor);

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
	void OnViewGeneralSettingsDlg();
	void OnSwitchToDRM();
	void OnSwitchToAM();
	void OnMenuSetDisplayColor();
	void OnMenuPlotStyle(int value);
};
