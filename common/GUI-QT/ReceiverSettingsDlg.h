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

#include "ReceiverSettingsDlgbase.h"

/* Definitions ****************************************************************/

/* Classes ********************************************************************/
class LatLongEditDlg;
class CDRMReceiver;

class ReceiverSettingsDlg : public ReceiverSettingsDlgBase
{
	Q_OBJECT

public:

	ReceiverSettingsDlg(CDRMReceiver& NRx, CSettings& NSettings, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, WFlags f = 0);
	virtual ~ReceiverSettingsDlg();

protected:
	virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);

	_BOOLEAN 		ValidInput(const QLineEdit* le);
	void			ExtractReceiverCoordinates();
	void			setDefaults();

	void			AddWhatsThisHelp();

	CDRMReceiver&	DRMReceiver;
	CSettings&		Settings;
	LatLongEditDlg* latlongeditdlg;
	bool			loading;

signals:
	void StartStopLog(bool);
	void LogPosition(bool);
	void LogSigStr(bool);
	void SetLogStartDelay(long);
	void StartStopGPS(bool);

public slots:
	void OnChangedLatLong(const QString&);
	void OnEditLatitude();
	void OnEditLongitude();
	void OnCheckBoxUseGPS();
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
	void OnCheckEnableRigToggled(bool);
	void OnCheckEnableSMeterToggled(bool);
	void OnCheckWithDRMModToggled(bool);
	void OnRigSelected(QListViewItem* item);
	void OnComPortSelected(QListViewItem* item);
	void OnConfigChanged(int row, int col);
	void OnRigOffsetChanged(QString text);
	void OnRigSettingsChanged(QString text);
};
