/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2010
 *
 * Author(s):
 *	Julian Cable
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

#include "fmdialogbase.h"
#include "DialogUtil.h"
#include "MultColorLED.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"

/* Classes ********************************************************************/
class FMDialog : public FMDialogBase
{
	Q_OBJECT

public:
	FMDialog(CDRMReceiver&, CSettings&, QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE,	WFlags f = 0);

	virtual ~FMDialog();

protected:
	CDRMReceiver&		DRMReceiver;
	CSettings&			Settings;

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
	_BOOLEAN		bStationsDlgWasVis;
	_BOOLEAN		bEPGDlgWasVis;
	ERecMode		eReceiverMode;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	virtual void	customEvent(QCustomEvent* Event);
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

public slots:
	void OnTune();
	void OnTimer();
	void OnMenuSetDisplayColor();
	void OnSwitchToDRM();
	void OnSwitchToAM();

signals:
	void SwitchMode(int);
	void ViewStationsDlg();
	void ViewLiveScheduleDlg();
	void Closed();
};
