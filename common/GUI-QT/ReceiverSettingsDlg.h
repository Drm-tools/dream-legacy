/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
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

#include "../util/Settings.h"
#include "qtimer.h"
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>

#include "ui_ReceiverSettingsDlg.h"

/* Definitions ****************************************************************/

/* Classes ********************************************************************/
class CDRMReceiver;

class ReceiverSettingsDlg : public QDialog, public Ui_ReceiverSettingsDlg
{
	Q_OBJECT

public:

	ReceiverSettingsDlg(CDRMReceiver& NRx, CSettings& NSettings, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, Qt::WFlags f = 0);
	virtual ~ReceiverSettingsDlg();

protected:
	virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);

	bool 	    	ValidInput(const QLineEdit* le);
	void			ExtractReceiverCoordinates();
	void			setDefaults();
	void 			checkRig(int);

	void			AddWhatsThisHelp();

	CDRMReceiver&	Receiver;
	CSettings&		Settings;
	bool			loading;
	Q3ListViewItem*  no_rig;
	Q3ListViewItem*	no_port;
	Q3ListViewItem*	last_port;
	QTimer			TimerRig;
	int				iWantedrigModel;

signals:
	void StartStopLog(bool);
	void LogPosition(bool);
	void LogSigStr(bool);
	void SetLogStartDelay(long);
	void StartStopGPS(bool);
	void ShowHideGPS(bool);

public slots:
	void OnTimerRig();
	void OnLineEditLatDegChanged(const QString& str);
	void OnLineEditLatMinChanged(const QString& str);
	void OnComboBoxNSHighlighted(int iID);
	void OnLineEditLngDegChanged(const QString& str);
	void OnLineEditLngMinChanged(const QString& str);
	void OnComboBoxEWHighlighted(int iID);
	void SetLatLng();
	void OnCheckBoxUseGPS();
	void OnCheckBoxDisplayGPS();
	void OnRadioTimeLinear();
	void OnRadioTimeWiener();
	void OnRadioFrequencyLinear();
	void OnRadioFrequencyDft();
	void OnRadioFrequencyWiener();
	void OnRadioTiSyncFirstPeak();
	void OnRadioTiSyncEnergy();
	void OnSliderIterChange(int value);
	void OnSliderLogStartDelayChange(int value);
	void ButtonOkClicked();
	void OnCheckFlipSpectrum();
	void OnCheckBoxMuteAudio();
	void OnCheckBoxReverb();
	void OnCheckWriteLog();
    void OnCheckBoxLogLatLng();
    void OnCheckBoxLogSigStr();
	void OnCheckSaveAudioWAV();
	void OnCheckRecFilter();
	void OnCheckModiMetric();
	void OnCheckEnableSMeterToggled(bool);
	void OnRigSelected(Q3ListViewItem* item);
	void OnComPortSelected(Q3ListViewItem* item);
};
