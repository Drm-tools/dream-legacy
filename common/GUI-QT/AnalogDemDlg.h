/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
 *
 * Author(s):
 *	Volker Fischer, Andrew Murphy
 *
 * Description:
 *	See AnalogDemDlg.cpp
 *
 * 11/21/2005 Andrew Murphy, BBC Research & Development, 2005
 *	- Additional widgets for displaying AMSS information
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
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qlayout.h>
#include <qcombobox.h>
/* This include is for setting the progress bar style */
#include <qmotifstyle.h>

#if QT_VERSION < 0x040000
# include <AnalogDemDlgbase.h>
# include "AMSSDlgbase.h"
#else
# include <ui_AnalogDemDlgbase.h>
# include "ui_AMSSDlgbase.h"
# include <QDialog>
#endif
#include "DialogUtil.h"
#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../util/Settings.h"
#include "../tables/TableAMSS.h"

/* Definitions ****************************************************************/
/* Update time of PLL phase dial control */
#define PLL_PHASE_DIAL_UPDATE_TIME				100


/* Classes ********************************************************************/

class CDRMPlot;

/* AMSS dialog -------------------------------------------------------------- */
class CAMSSDlg :
#if QT_VERSION < 0x040000
 public CAMSSDlgBase
#else
 public QDialog, public Ui_CAMSSDlgBase
#endif
{
	Q_OBJECT

public:
	CAMSSDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0, const char* name = 0,
		bool modal = FALSE, Qt::WFlags f = 0);

protected:
	CDRMReceiver&	DRMReceiver;
	CSettings&		Settings;

	QTimer			Timer;
	QTimer			TimerPLLPhaseDial;
	void			AddWhatsThisHelp();
	virtual void	hideEvent(QHideEvent* pEvent);
    virtual void	showEvent(QShowEvent* pEvent);

public slots:
	void OnTimer();
	void OnTimerPLLPhaseDial();
};


/* Analog demodulation dialog ----------------------------------------------- */
class AnalogDemDlg :
#if QT_VERSION < 0x040000
 public AnalogDemDlgBase
#else
 public QDialog, public Ui_AnalogDemDlgBase
#endif
{
	Q_OBJECT

public:
	AnalogDemDlg(CDRMReceiver&, CSettings&, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);

	void 			UpdatePlotsStyle();

protected:
	CDRMReceiver&	DRMReceiver;
	CSettings&		Settings;

	CDRMPlot*		MainPlot;

	QTimer			Timer;
	QTimer			TimerPLLPhaseDial;
	CAMSSDlg		AMSSDlg;

	void			UpdateControls();
	void			AddWhatsThisHelp();
    virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);
	virtual void	closeEvent(QCloseEvent* pEvent);

public slots:
	void OnTimer();
	void OnTimerPLLPhaseDial();
	void OnRadioDemodulation(int iID);
	void OnRadioAGC(int iID);
	void OnCheckBoxMuteAudio();
	void OnCheckSaveAudioWAV();
	void OnCheckAutoFreqAcq();
	void OnCheckPLL();
	void OnChartxAxisValSet(double dVal);
	void OnSliderBWChange(int value);
	void OnRadioNoiRed(int iID);
	void OnButtonWaterfall();
	void OnButtonAMSS();
	void OnSwitchToDRM();
	void OnSwitchToFM();

signals:
	void SwitchMode(int);
	void NewAMAcquisition();
	void ViewStationsDlg();
	void ViewLiveScheduleDlg();
	void Closed();
};
