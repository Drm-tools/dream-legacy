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
#include <qwt_thermo.h>
#include <qdatetime.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <qfiledialog.h>

#ifdef _WIN32
# include "../../Windows/moc/systemevalDlgbase.h"
#else
# include "moc/systemevalDlgbase.h"
#endif
#include "DRMPlot.h"
#include "MultColorLED.h"
#include "../GlobalDefinitions.h"
#include "../Vector.h"
#include "../DrmReceiver.h"

extern CDRMReceiver	DRMReceiver;


/* Definitions ****************************************************************/
/* Define this macro if you prefer the QT-type of displaying date and time */
#define GUI_QT_DATE_TIME_TYPE

/* Define the time out when the delayed log file should begin */
#define DELAYED_LOG_FILE_TIME_OUT		60000 /* ms */


/* Classes ********************************************************************/
class systemevalDlg : public systemevalDlgBase
{
	Q_OBJECT

public:
	enum ECharType {AVERAGED_IR, TRANSFERFUNCTION, 
					FAC_CONSTELLATION, SDC_CONSTELLATION, MSC_CONSTELLATION, 
					POWER_SPEC_DENSITY, INPUTSPECTRUM_NO_AV};

	systemevalDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		WFlags f = 0);

	void SetStatus(int MessID, int iMessPara);

protected:
	QTimer			Timer;
	QTimer			TimerChart;
	QTimer			TimerLogFileLong;
	QTimer			TimerLogFileShort;
	QTimer			TimerLogFileStart;
	ECharType		CharType;
	int				iCurFrequency;
	void			OnlyThisButDown(QPushButton* pButton);
    virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);

	QString			GetRobModeStr();
	QString			GetSpecOccStr();

public slots:
	void OnTimer();
	void OnTimerChart();
	void OnTimerLogFileLong();
	void OnTimerLogFileShort();
	void OnTimerLogFileStart();
	void OnRadioTimeLinear();
	void OnRadioTimeWiener();
	void OnRadioFrequencyLinear();
	void OnRadioFrequencyDft();
	void OnRadioFrequencyWiener();
	void OnRadioTiSyncFirstPeak();
	void OnRadioTiSyncEnergy();
	void OnSliderIterChange(int value);
	void OnButtonAvIR();
	void OnButtonTransFct();
	void OnButtonFACConst();
	void OnButtonSDCConst();
	void OnButtonMSCConst();
	void OnButtonPSD();
	void OnButtonInpSpec();
	void OnCheckFlipSpectrum();
	void OnCheckBoxMuteAudio();
	void OnCheckWriteLog();
	void OnCheckSaveAudioWAV();
};
