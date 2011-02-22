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
#include <qevent.h>
#include <qlayout.h>
#include <qpalette.h>
#include <qcolordialog.h>
#if QT_VERSION < 0x040000
# define Q3PopupMenu QPopupMenu
# define Q3ButtonGroup QButtonGroup
# include <qwt/qwt_thermo.h>
# include <qpopupmenu.h>
#else
# include <qwt_thermo.h>
# include <q3popupmenu.h>
# include <QShowEvent>
# include <QHideEvent>
# include <QCustomEvent>
# include <QCloseEvent>
#endif

#include "fdrmdialogbase.h"
#include "DialogUtil.h"
#include "systemevalDlg.h"
#include "MultimediaDlg.h"
#include "StationsDlg.h"
#include "LiveScheduleDlg.h"
#include "EPGDlg.h"
#include "fmdialog.h"
#include "AnalogDemDlg.h"
#include "MultSettingsDlg.h"
#include "GeneralSettingsDlg.h"
#include "MultColorLED.h"
#include "../DrmReceiver.h"
#include "../util/Vector.h"
#include "../datadecoding/DataDecoder.h"



/* Classes ********************************************************************/
class FDRMDialog : public FDRMDialogBase
{
	Q_OBJECT

public:
	FDRMDialog(CDRMReceiver&, CSettings&, CRig&, QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE,	Qt::WFlags f = 0);

	virtual ~FDRMDialog();

protected:
	CDRMReceiver&		DRMReceiver;
	CSettings&			Settings;
	QTimer				Timer;
	vector<QLabel*>		serviceLabels;

	systemevalDlg*		pSysEvalDlg;
	MultimediaDlg*		pMultiMediaDlg;
	MultSettingsDlg*	pMultSettingsDlg;
	StationsDlg*		pStationsDlg;
	LiveScheduleDlg*	pLiveScheduleDlg;
	EPGDlg*				pEPGDlg;
	AnalogDemDlg*		pAnalogDemDlg;
	FMDialog*			pFMDlg;
	GeneralSettingsDlg* pGeneralSettingsDlg;
	QMenuBar*			pMenu;
	Q3PopupMenu*			pReceiverModeMenu;
	Q3PopupMenu*			pSettingsMenu;
	Q3PopupMenu*			pPlotStyleMenu;
	Q3ButtonGroup*		pButtonGroup;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	virtual void	customEvent(QCustomEvent* Event);
	virtual void	closeEvent(QCloseEvent* ce);
	virtual void	showEvent(QShowEvent* pEvent);
	void			hideEvent(QHideEvent* pEvent);
	void			AddWhatsThisHelp();
	void			UpdateDisplay();
	void			ClearDisplay();


	void			SetDisplayColor(const QColor newColor);

	void			ChangeGUIModeToDRM();
	void			ChangeGUIModeToAM();
	void			ChangeGUIModeToFM();

	QString	GetCodecString(const CService&);
	QString	GetTypeString(const CService&);
	QString serviceSelector(CParameter&, int);
	void showTextMessage(const QString&);
	void showServiceInfo(const CService&);

public slots:
	void OnTimer();
	void OnSelectAudioService(int);
	void OnSelectDataService(int);
	void OnViewStationsDlg();
	void OnMenuSetDisplayColor();
	void OnMenuPlotStyle(int value);
	void OnNewAcquisition();
	void OnSwitchMode(int);
	void OnSwitchToFM();
	void OnSwitchToAM();
};
