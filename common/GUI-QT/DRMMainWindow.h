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


#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
# include "Loghelper.h"
#include "ui_DRMMainWindow.h"
#include "DialogUtil.h"
#include "MultColorLED.h"
#include "../util/Vector.h"
#include <QMainWindow>

/* Classes ********************************************************************/

class DRMMainWindow : public QMainWindow, public Ui_DRMMainWindow
{
	Q_OBJECT

public:
	DRMMainWindow(CDRMReceiver&, CSettings&, QWidget* parent = 0,
	const char* name = 0,	Qt::WFlags f = 0);
	virtual ~DRMMainWindow();
	/* dummy assignment operator to help MSVC8 */
	DRMMainWindow& operator=(const DRMMainWindow&)
	{ throw "should not happen"; return *this;}

protected:

	CDRMReceiver&	Receiver;
	CSettings&		Settings;

	QMainWindow*    jlViewer;
	QMainWindow*    bwsViewer;
	QMainWindow*    slideShowViewer;
	QDialog*        sysEvalDlg;
	QDialog*        stationsDlg;
	QDialog*        liveScheduleDlg;
	QDialog*        epgDlg;
	QDialog*        receiverSettingsDlg;
	QDialog*        multSettingsDlg;

    Loghelper		loghelper;
	int				iCurSelServiceGUI;
	int				iOldNoServicesGUI;
	QTimer			Timer;

	EDemodulationType	eReceiverMode;
	bool            quitWanted;

	void SetStatus(CMultColorLED* LED, ETypeRxStatus state);
	void			SetService(int iNewServiceID);
	void			AddWhatsThisHelp();
	void			UpdateDisplay();
	void			ClearDisplay();

	QString			GetCodecString(const int iServiceID);
	QString			GetTypeString(const int iServiceID);

	void			SetDisplayColor(const QColor newColor);
    void	        showEvent(QShowEvent* pEvent);
    void	        hideEvent(QHideEvent* pEvent);
    void            closeEvent(QCloseEvent*);

public slots:
	void OnTimer();
	void OnButtonService1();
	void OnButtonService2();
	void OnButtonService3();
	void OnButtonService4();
	void OnNewDRMAcquisition();
	void OnMenuSetDisplayColor();
    void OnSwitchToAnalog();
};
