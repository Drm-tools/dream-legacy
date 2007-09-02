/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Julian Cable, Andrea Russo
 *
 * Description:
 * Settings for the receiver
 * Perhaps this should be Receiver Controls rather than Settings
 * since selections take effect immediately and there is no apply/cancel
 * feature. This makes sense, since one wants enable/disable GPS, Rig, Smeter
 * to be instant and mute/savetofile etc.
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

#include <qlistview.h>
#include <qfiledialog.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qvalidator.h>
#include <qmessagebox.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qradiobutton.h>
#include "../GlobalDefinitions.h"
#include "ReceiverSettingsDlg.h"
#include "LatLongEditDlg.h"
#include "../DrmReceiver.h"

#if !defined(HAVE_RIG_PARSE_MODE) && defined(HAVE_LIBHAMLIB)
extern "C"
{
	extern rmode_t parse_mode(const char *);
	extern vfo_t parse_vfo(const char *);
	extern setting_t parse_func(const char *);
	extern setting_t parse_level(const char *);
	extern setting_t parse_parm(const char *);
	extern const char* strstatus(enum rig_status_e);
}
# define rig_parse_mode parse_mode
# define rig_parse_vfo parse_vfo
# define rig_parse_func parse_func
# define rig_parse_level parse_level
# define rig_parse_parm parse_parm
# define rig_strstatus strstatus
#endif


/* Implementation *************************************************************/

ReceiverSettingsDlg::ReceiverSettingsDlg(CDRMReceiver& NRx, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f) :
	ReceiverSettingsDlgBase(parent, name, modal, f), DRMReceiver(NRx),Settings(NSettings),
	loading(true)
{

	/* Connections */

	connect(buttonOk, SIGNAL(clicked()), SLOT(ButtonOkClicked()) );
	connect(PushButtonEditLatitude, SIGNAL(clicked()), SLOT(OnEditLatitude()) );
	connect(PushButtonEditLongitude, SIGNAL(clicked()), SLOT(OnEditLongitude()) );
	connect(LineEditLatitude, SIGNAL(textChanged(const QString&)), SLOT(OnChangedLatLong(const QString&)) );
	connect(LineEditLongitude, SIGNAL(textChanged(const QString&)), SLOT(OnChangedLatLong(const QString&)) );

	connect(SliderLogStartDelay, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderLogStartDelayChange(int)));

	connect(SliderNoOfIterations, SIGNAL(valueChanged(int)),
		this, SLOT(OnSliderIterChange(int)));

	/* Radio buttons */
	connect(RadioButtonTiLinear, SIGNAL(clicked()),
		this, SLOT(OnRadioTimeLinear()));
	connect(RadioButtonTiWiener, SIGNAL(clicked()),
		this, SLOT(OnRadioTimeWiener()));
	connect(RadioButtonFreqLinear, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyLinear()));
	connect(RadioButtonFreqDFT, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyDft()));
	connect(RadioButtonFreqWiener, SIGNAL(clicked()),
		this, SLOT(OnRadioFrequencyWiener()));
	connect(RadioButtonTiSyncEnergy, SIGNAL(clicked()),
		this, SLOT(OnRadioTiSyncEnergy()));
	connect(RadioButtonTiSyncFirstPeak, SIGNAL(clicked()),
		this, SLOT(OnRadioTiSyncFirstPeak()));

	/* Check boxes */
	connect(CheckBoxUseGPS, SIGNAL(clicked()), SLOT(OnCheckBoxUseGPS()) );
	connect(CheckBoxFlipSpec, SIGNAL(clicked()), this, SLOT(OnCheckFlipSpectrum()));
	connect(CheckBoxMuteAudio, SIGNAL(clicked()), this, SLOT(OnCheckBoxMuteAudio()));
	connect(CheckBoxWriteLog, SIGNAL(clicked()), this, SLOT(OnCheckWriteLog()));
	connect(CheckBoxRecFilter, SIGNAL(clicked()), this, SLOT(OnCheckRecFilter()));
	connect(CheckBoxModiMetric, SIGNAL(clicked()), this, SLOT(OnCheckModiMetric()));
	connect(CheckBoxReverb, SIGNAL(clicked()), this, SLOT(OnCheckBoxReverb()));
	connect(CheckBoxLogLatLng, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogLatLng()));
	connect(CheckBoxLogSigStr, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogSigStr()));
	connect(CheckBoxSaveAudioWave, SIGNAL(clicked()), this, SLOT(OnCheckSaveAudioWAV()));

	connect(CheckBoxEnableSMeter, SIGNAL(toggled(bool)), this, SLOT(OnCheckEnableSMeterToggled(bool)));

	connect(ListViewRig, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnRigSelected(QListViewItem*)));
	connect(ListViewPort, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(OnComPortSelected(QListViewItem*)));

	/* Set help text for the controls */
	AddWhatsThisHelp();

	setDefaults();
}

ReceiverSettingsDlg::~ReceiverSettingsDlg()
{
	double latitude, longitude;
	DRMReceiver.GetParameters()->GPSData.GetLatLongDegrees(latitude, longitude);
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);

	if(DRMReceiver.GetIsWriteWaveFile())
		DRMReceiver.StopWriteWaveFile();
}

void ReceiverSettingsDlg::hideEvent(QHideEvent*)
{
}

/* this sets default values into the dialog and ini file for
 * items not covered in other places. It is currently called
 * from the constructor and contains items which can only
 * be modified in this dialog or on the command line.
 * (lat/long is still looking for a good home)
 */
void ReceiverSettingsDlg::setDefaults()
{
	CParameter& Parameters = *(DRMReceiver.GetParameters());

	/* these won't get into the ini file unless we use GPS or have this: */
	double latitude = Settings.Get("Logfile", "latitude", 100.0);
	double longitude = Settings.Get("Logfile", "longitude", 0.0);
	if(latitude<=90.0)
	{
		Parameters.GPSData.SetPositionAvailable(TRUE);
		Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
		Parameters.GPSData.SetLatLongDegrees(latitude, longitude);
	}
	else
	{
		latitude = 0.0;
		Parameters.GPSData.SetPositionAvailable(FALSE);
	}

	/* Start log file flag */
	CheckBoxWriteLog->setChecked(Settings.Get("Logfile", "enablelog", FALSE));

    /* log file flag for storing signal strength in long log */
	CheckBoxLogSigStr->setChecked(Settings.Get("Logfile", "enablerxl", FALSE));

	/* log file flag for storing lat/long in long log */
	CheckBoxLogLatLng->setChecked(Settings.Get("Logfile", "enablepositiondata", FALSE));

	/* logging delay value */
	int iLogDelay = Settings.Get("Logfile", "delay", 0);
	SliderLogStartDelay->setValue(iLogDelay);

	/* GPS ------------------------------------------------------------------- */
	string host = Settings.Get("GPS", "host", string("localhost"));
    LineEditGPSHost->setText(host.c_str());

	int port = Settings.Get("GPS", "port", 2947);
    LineEditGPSPort->setText(QString("%1").arg(port));

	CheckBoxUseGPS->setChecked(Settings.Get("GPS", "usegpsd", FALSE));

	/* get the defaults into the ini file */
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);
	Settings.Put("Logfile", "enablelog", CheckBoxWriteLog->isChecked());
	Settings.Put("Logfile", "enablerxl", CheckBoxLogSigStr->isChecked());
	Settings.Put("Logfile", "enablepositiondata", CheckBoxLogLatLng->isChecked());
	Settings.Put("Logfile", "delay", iLogDelay);
	Settings.Put("GPS", "usegpsd", CheckBoxUseGPS->isChecked());
	Settings.Put("GPS", "host", host);
	Settings.Put("GPS", "port", port);
}

void ReceiverSettingsDlg::showEvent(QShowEvent*)
{
	loading = true; // prevent exective actions during reading state
	
	/* Sync ----------------------------------------------------------------- */
	if (DRMReceiver.GetTimeInt() == CChannelEstimation::TWIENER)
		RadioButtonTiWiener->setChecked(TRUE);
	else
		RadioButtonTiLinear->setChecked(TRUE);

	switch(DRMReceiver.GetFreqInt())
	{
	case CChannelEstimation::FLINEAR:
		RadioButtonFreqLinear->setChecked(TRUE);
		break;
	case CChannelEstimation::FDFTFILTER:
		RadioButtonFreqDFT->setChecked(TRUE);
		break;
	case CChannelEstimation::FWIENER:
		RadioButtonFreqWiener->setChecked(TRUE);
	}

	if (DRMReceiver.GetTiSyncTracType() == CTimeSyncTrack::TSFIRSTPEAK)
		RadioButtonTiSyncFirstPeak->setChecked(TRUE);
	else
		RadioButtonTiSyncEnergy->setChecked(TRUE);

	/* Misc ----------------------------------------------------------------- */
	CheckBoxRecFilter->setChecked(DRMReceiver.GetRecFilter());
	CheckBoxModiMetric->setChecked(DRMReceiver.GetIntCons());
	CheckBoxFlipSpec->setChecked(DRMReceiver.GetFlippedSpectrum());
	SliderNoOfIterations->setValue(DRMReceiver.GetInitNumIterations());

	/* Audio ---------------------------------------------------------------- */

	CheckBoxMuteAudio->setChecked(DRMReceiver.GetMuteAudio());
	CheckBoxReverb->setChecked(DRMReceiver.GetReverbEffect());
	CheckBoxSaveAudioWave->setChecked(DRMReceiver.GetIsWriteWaveFile());

	/* GPS */
	ExtractReceiverCoordinates();

#ifdef HAVE_LIBHAMLIB
	/* Rig */
	ListViewRig->setRootIsDecorated(true);
	ListViewRig->setAllColumnsShowFocus(true);
	ListViewRig->setColumnText(0, "Rig");
	ListViewRig->addColumn("ID");
	ListViewRig->addColumn("Status");
	ListViewRig->clear();
	QPixmap	BitmLittleGreenSquare;
	BitmLittleGreenSquare.resize(5, 5);
	BitmLittleGreenSquare.fill(QColor(0, 255, 0));

	map<rig_model_t, CRigCaps> rigs;
	map<string,QListViewItem*> manufacturers;
	rig_model_t current = DRMReceiver.GetRigModel();

	DRMReceiver.GetRigList(rigs);

	new QListViewItem(new QListViewItem(ListViewRig, "-None"), "None", "0", "");
	for (map<rig_model_t, CRigCaps>::iterator i=rigs.begin(); i!=rigs.end(); i++)
	{
		/* Store model ID */
		QListViewItem* man, *model=NULL, *mod_model=NULL;
		rig_model_t iModelID = i->first;
		CRigCaps& rig = i->second;
		if(rig.strManufacturer=="")
			continue;
		map<string,QListViewItem*>::iterator mfr = manufacturers.find(rig.strManufacturer);
		if(mfr==manufacturers.end())
			manufacturers[rig.strManufacturer] = 
				man = new QListViewItem(ListViewRig, rig.strManufacturer.c_str());
		else
			man = mfr->second;
		model = new QListViewItem(
			man,
			rig.strModelName.c_str(),
			QString::number(iModelID),
			rig_strstatus(rig.eRigStatus)
		);

		if (!rig.settings[DRM_MODIFIED].levels.empty())
		{
			mod_model = new QListViewItem(
				man,
				(rig.strModelName+" (DRM)").c_str(),
				QString::number(iModelID),
				rig_strstatus(rig.eRigStatus)
			);
			mod_model->setPixmap(0, BitmLittleGreenSquare);
			man->setPixmap(0, BitmLittleGreenSquare);
		}

		/* Check for selected Rig */
		if (current == iModelID)
		{
			if(DRMReceiver.GetEnableModRigSettings())
			{
				mod_model->setSelected(TRUE);
				ListViewRig->ensureItemVisible(mod_model);
			}
			else
			{
				model->setSelected(TRUE);
				ListViewRig->ensureItemVisible(model);
			}
		}
	}

	/* COM port selection --------------------------------------------------- */
	ListViewPort->setAllColumnsShowFocus(true);
	ListViewPort->setColumnText(0, "Name");
	ListViewPort->addColumn("Port");
	ListViewPort->clear();
	map<string,string> ports;
	DRMReceiver.GetComPortList(ports);
	string strPort = DRMReceiver.GetRigComPort();
	for(map<string,string>::iterator p=ports.begin(); p!=ports.end(); p++)
	{
		QListViewItem* item = new QListViewItem(ListViewPort, p->first.c_str(), p->second.c_str());
		if(strPort == p->second)
			item->setSelected(TRUE);
	}

	/* Enable s-meter */
	CheckBoxEnableSMeter->setChecked(DRMReceiver.GetEnableSMeter());

	/* Enable special settings for rigs */
	//CheckBoxWithDRMMod->setChecked(DRMReceiver.GetEnableModRigSettings());
#endif
	loading = false; // loading completed
}

void ReceiverSettingsDlg::OnCheckBoxUseGPS()
{
#if defined(_MSC_VER) && (_MSC_VER < 1400)
	QMessageBox::information( this, "Dream", "Don't enable GPS unless you have gpsd running." );
#endif
	Settings.Put("GPS", "host", string(LineEditGPSHost->text().latin1()));
	Settings.Put("GPS", "port", LineEditGPSPort->text().toInt());
	Settings.Put("GPS", "usegpsd", CheckBoxUseGPS->isChecked());
	emit StartStopGPS(CheckBoxUseGPS->isChecked());
}

void ReceiverSettingsDlg::OnEditLatitude()
{
	LatLongEditDlg* edt = new LatLongEditDlg(LineEditLatitude, this);
	edt->show();
}

void ReceiverSettingsDlg::OnEditLongitude()
{
	LatLongEditDlg* edt = new LatLongEditDlg(LineEditLongitude, this);
	edt->show();
}

void ReceiverSettingsDlg::OnChangedLatLong(const QString&)
{
	CParameter& Parameters = *DRMReceiver.GetParameters();
	double latitude, longitude;

	QStringList a = QStringList::split("'", LineEditLongitude->text());
	QStringList b = QStringList::split(ring, a[0]);
	longitude = (b[0].toDouble() + b[1].toDouble()/60.0)*((a[1]=="E")?1:-1);
	a = QStringList::split("'", LineEditLatitude->text());
	b = QStringList::split(ring, a[0]);
	latitude = (b[0].toDouble() + b[1].toDouble()/60.0)*((a[1]=="N")?1:-1);

	Parameters.Lock(); 
	Parameters.GPSData.SetPositionAvailable(TRUE);
	Parameters.GPSData.SetLatLongDegrees(latitude, longitude);
	Parameters.Unlock(); 
}

/* when the dialog closes save the contents of any controls which don't have
 * their own slot handlers
 */

void ReceiverSettingsDlg::ButtonOkClicked()
{
	/* save current settings */
	Settings.Put("GPS", "host", string(LineEditGPSHost->text().latin1()));
	Settings.Put("GPS", "port", LineEditGPSPort->text().toInt());

	accept(); /* close the dialog */
}

void ReceiverSettingsDlg::ExtractReceiverCoordinates()
{
	QString sVal, sDir;
	CParameter& Parameters = *DRMReceiver.GetParameters();

	/* parse the latitude and longitude string stored into Dream settings to
		extract local latitude and longitude coordinates */

	string latitude, longitude;

	Parameters.Lock(); 
	Parameters.GPSData.asDM(latitude, longitude);
	Parameters.Unlock(); 

	QString s;
	s.setLatin1(longitude.c_str());
	LineEditLongitude->setText(s);
	s.setLatin1(latitude.c_str());
	LineEditLatitude->setText(s);
}

void ReceiverSettingsDlg::OnRadioTimeLinear() 
{
	if (DRMReceiver.GetTimeInt() != CChannelEstimation::TLINEAR)
		DRMReceiver.SetTimeInt(CChannelEstimation::TLINEAR);
}

void ReceiverSettingsDlg::OnRadioTimeWiener() 
{
	if (DRMReceiver.GetTimeInt() != CChannelEstimation::TWIENER)
		DRMReceiver.SetTimeInt(CChannelEstimation::TWIENER);
}

void ReceiverSettingsDlg::OnRadioFrequencyLinear() 
{
	if (DRMReceiver.GetFreqInt() != CChannelEstimation::FLINEAR)
		DRMReceiver.SetFreqInt(CChannelEstimation::FLINEAR);
}

void ReceiverSettingsDlg::OnRadioFrequencyDft() 
{
	if (DRMReceiver.GetFreqInt() != CChannelEstimation::FDFTFILTER)
		DRMReceiver.SetFreqInt(CChannelEstimation::FDFTFILTER);
}

void ReceiverSettingsDlg::OnRadioFrequencyWiener() 
{
	if (DRMReceiver.GetFreqInt() != CChannelEstimation::FWIENER)
		DRMReceiver.SetFreqInt(CChannelEstimation::FWIENER);
}

void ReceiverSettingsDlg::OnRadioTiSyncFirstPeak() 
{
	if (DRMReceiver.GetTiSyncTracType() != 
		CTimeSyncTrack::TSFIRSTPEAK)
	{
		DRMReceiver.SetTiSyncTracType(CTimeSyncTrack::TSFIRSTPEAK);
	}
}

void ReceiverSettingsDlg::OnRadioTiSyncEnergy() 
{
	if (DRMReceiver.GetTiSyncTracType() != 
		CTimeSyncTrack::TSENERGY)
	{
		DRMReceiver.SetTiSyncTracType(CTimeSyncTrack::TSENERGY);
	}
}

void ReceiverSettingsDlg::OnSliderIterChange(int value)
{
	/* Set new value in working thread module */
	DRMReceiver.SetNumIterations(value);

	/* Show the new value in the label control */
	TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
		QString().setNum(value));
}

void ReceiverSettingsDlg::OnCheckFlipSpectrum()
{
	/* Set parameter in working thread module */
	DRMReceiver.SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
}

void ReceiverSettingsDlg::OnCheckRecFilter()
{
	/* Set parameter in working thread module */
	DRMReceiver.SetRecFilter(CheckBoxRecFilter->isChecked());

	/* If filter status is changed, a new aquisition is necessary */
	DRMReceiver.RequestNewAcquisition();
}

void ReceiverSettingsDlg::OnCheckModiMetric()
{
	/* Set parameter in working thread module */
	DRMReceiver.SetIntCons(CheckBoxModiMetric->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	DRMReceiver.MuteAudio(CheckBoxMuteAudio->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxReverb()
{
	/* Set parameter in working thread module */
	DRMReceiver.SetReverbEffect(CheckBoxReverb->isChecked());
}

void ReceiverSettingsDlg::OnCheckSaveAudioWAV()
{
/*
	This code is copied in AnalogDemDlg.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (CheckBoxSaveAudioWave->isChecked() == TRUE)
	{
		/* Show "save file" dialog */
		QString strFileName =
			QFileDialog::getSaveFileName(tr("DreamOut.wav"), "*.wav", this);

		/* Check if user not hit the cancel button */
		if (!strFileName.isNull())
		{
			DRMReceiver.StartWriteWaveFile(strFileName.latin1());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(FALSE);
		}
	}
	else
		DRMReceiver.StopWriteWaveFile();
}

void ReceiverSettingsDlg::OnCheckWriteLog()
{
	emit StartStopLog(CheckBoxWriteLog->isChecked());
	Settings.Put("Logfile", "enablelog", CheckBoxWriteLog->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxLogLatLng()
{
	emit LogPosition(CheckBoxLogLatLng->isChecked());
	Settings.Put("Logfile", "enablepositiondata", CheckBoxLogLatLng->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxLogSigStr()
{
	emit LogSigStr(CheckBoxLogSigStr->isChecked());
	Settings.Put("Logfile", "enablerxl", CheckBoxLogSigStr->isChecked());
}

void ReceiverSettingsDlg::OnSliderLogStartDelayChange(int value)
{
	emit SetLogStartDelay(value);
	Settings.Put("Logfile", "delay", value);
}

void ReceiverSettingsDlg::OnCheckEnableSMeterToggled(bool on)
{
	if(loading)
		return;
	DRMReceiver.SetEnableSMeter(on);
}

void ReceiverSettingsDlg::OnRigSelected(QListViewItem* item)
{
	if(loading)
		return;
	int iID = item->text(1).toInt();
	if(iID==0)
	{
		DRMReceiver.SetRigModel(0);
	}
	else
	{
		DRMReceiver.SetRigFreqOffset(LineEditRigFreqOff->text().toInt());
		DRMReceiver.SetEnableModRigSettings(item->pixmap(0)!=NULL);
		QListViewItem* cp = ListViewPort->selectedItem();
		if(cp)
			DRMReceiver.SetRigComPort(cp->text(1).latin1());
		DRMReceiver.SetEnableSMeter(CheckBoxEnableSMeter->isChecked());
		DRMReceiver.SetRigModel(iID);
	}
}

void ReceiverSettingsDlg::OnComPortSelected(QListViewItem* item)
{
	if(loading)
		return;
	string key = item->text(1).latin1();
	map<string,string> ports;
	DRMReceiver.GetComPortList(ports);
	for(map<string,string>::iterator p=ports.begin(); p!=ports.end(); p++)
	{
		if(p->first == key)
			DRMReceiver.SetRigComPort(p->second);
	}
}

void ReceiverSettingsDlg::OnConfigChanged(int row, int col)
{
	/*
		QListViewItemIterator it( ListViewRigConf );
		for ( ; it.current(); ++it )
		{
			DRMReceiver.GetHamlib()->config[it.current()->text(0).latin1()] = it.current()->text(1).latin1();
		}
		*/
	(void)row;
	(void)col; /* TODO */
}

void ReceiverSettingsDlg::OnRigOffsetChanged(QString text)
{
	DRMReceiver.SetRigFreqOffset(text.toInt());
}

void ReceiverSettingsDlg::OnRigSettingsChanged(QString text)
{
}

void ReceiverSettingsDlg::AddWhatsThisHelp()
{
	/* GPS */
	const QString strGPS =
		tr("<b>Receiver coordinates:</b> Are used on "
		"Live Schedule Dialog to show a little green cube on the left"
		" of the target column if the receiver coordinates (latitude and longitude)"
		" are inside the target area of this transmission.<br>"
		"Receiver coordinates are also saved into the Log file.");

    QWhatsThis::add(LineEditLatitude, strGPS);
    QWhatsThis::add(LineEditLongitude, strGPS);

	/* MLC, Number of Iterations */
	const QString strNumOfIterations =
		tr("<b>MLC, Number of Iterations:</b> In DRM, a "
		"multilevel channel coder is used. With this code it is possible to "
		"iterate the decoding process in the decoder to improve the decoding "
		"result. The more iterations are used the better the result will be. "
		"But switching to more iterations will increase the CPU load. "
		"Simulations showed that the first iteration (number of "
		"iterations = 1) gives the most improvement (approx. 1.5 dB at a "
		"BER of 10-4 on a Gaussian channel, Mode A, 10 kHz bandwidth). The "
		"improvement of the second iteration will be as small as 0.3 dB."
		"<br>The recommended number of iterations given in the DRM "
		"standard is one iteration (number of iterations = 1).");

	QWhatsThis::add(TextNumOfIterations, strNumOfIterations);
	QWhatsThis::add(SliderNoOfIterations, strNumOfIterations);

	/* Flip Input Spectrum */
	QWhatsThis::add(CheckBoxFlipSpec,
		tr("<b>Flip Input Spectrum:</b> Checking this box "
		"will flip or invert the input spectrum. This is necessary if the "
		"mixer in the front-end uses the lower side band."));

	/* Mute Audio */
	QWhatsThis::add(CheckBoxMuteAudio,
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Reverberation Effect */
	QWhatsThis::add(CheckBoxReverb,
		tr("<b>Reverberation Effect:</b> If this check box is checked, a "
		"reverberation effect is applied each time an audio drop-out occurs. "
		"With this effect it is possible to mask short drop-outs."));

	/* Log File */
	QWhatsThis::add(CheckBoxWriteLog,
		tr("<b>Log File:</b> Checking this box brings the "
		"Dream software to write a log file about the current reception. "
		"Every minute the average SNR, number of correct decoded FAC and "
		"number of correct decoded MSC blocks are logged including some "
		"additional information, e.g. the station label and bit-rate. The "
		"log mechanism works only for audio services using AAC source coding. "
#ifdef _WIN32
		"During the logging no Dream windows "
		"should be moved or re-sized. This can lead to incorrect log files "
		"(problem with QT timer implementation under Windows). This problem "
		"does not exist in the Linux version of Dream."
#endif
		"<br>The log file will be "
		"written in the directory were the Dream application was started and "
		"the name of this file is always DreamLog.txt"));

	/* Wiener */
	const QString strWienerChanEst =
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>Wiener:</b> Wiener interpolation method "
		"uses estimation of the statistics of the channel to design an optimal "
		"filter for noise reduction.");

	QWhatsThis::add(RadioButtonFreqWiener, strWienerChanEst);
	QWhatsThis::add(RadioButtonTiWiener, strWienerChanEst);

	/* Linear */
	const QString strLinearChanEst =
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>Linear:</b> Simple linear interpolation "
		"method to get the channel estimate. The real and imaginary parts "
		"of the estimated channel at the pilot positions are linearly "
		"interpolated. This algorithm causes the lowest CPU load but "
		"performs much worse than the Wiener interpolation at low SNRs.");

	QWhatsThis::add(RadioButtonFreqLinear, strLinearChanEst);
	QWhatsThis::add(RadioButtonTiLinear, strLinearChanEst);

	/* DFT Zero Pad */
	QWhatsThis::add(RadioButtonFreqDFT,
		tr("<b>Channel Estimation Settings:</b> With these "
		"settings, the channel estimation method in time and frequency "
		"direction can be selected. The default values use the most powerful "
		"algorithms. For more detailed information about the estimation "
		"algorithms there are a lot of papers and books available.<br>"
		"<b>DFT Zero Pad:</b> Channel estimation method "
		"for the frequency direction using Discrete Fourier Transformation "
		"(DFT) to transform the channel estimation at the pilot positions to "
		"the time domain. There, a zero padding is applied to get a higher "
		"resolution in the frequency domain -> estimates at the data cells. "
		"This algorithm is very speed efficient but has problems at the edges "
		"of the OFDM spectrum due to the leakage effect."));

	/* Guard Energy */
	QWhatsThis::add(RadioButtonTiSyncEnergy,
		tr("<b>Guard Energy:</b> Time synchronization "
		"tracking algorithm utilizes the estimation of the impulse response. "
		"This method tries to maximize the energy in the guard-interval to set "
		"the correct timing."));

	/* First Peak */
	QWhatsThis::add(RadioButtonTiSyncFirstPeak,
		tr("<b>First Peak:</b> This algorithms searches for "
		"the first peak in the estimated impulse response and moves this peak "
		"to the beginning of the guard-interval (timing tracking algorithm)."));

	/* Save audio as wave */
	QWhatsThis::add(CheckBoxSaveAudioWave,
		tr("<b>Save Audio as WAV:</b> Save the audio signal "
		"as stereo, 16-bit, 48 kHz sample rate PCM wave file. Checking this "
		"box will let the user choose a file name for the recording."));

	/* Interferer Rejection */
	const QString strInterfRej =
		tr("<b>Interferer Rejection:</b> There are two "
		"algorithms available to reject interferers:<ul>"
		"<li><b>Bandpass Filter (BP-Filter):</b>"
		" The bandpass filter is designed to have the same bandwidth as "
		"the DRM signal. If, e.g., a strong signal is close to the border "
		"of the actual DRM signal, under some conditions this signal will "
		"produce interference in the useful bandwidth of the DRM signal "
		"although it is not on the same frequency as the DRM signal. "
		"The reason for that behaviour lies in the way the OFDM "
		"demodulation is done. Since OFDM demodulation is a block-wise "
		"operation, a windowing has to be applied (which is rectangular "
		"in case of OFDM). As a result, the spectrum of a signal is "
		"convoluted with a Sinc function in the frequency domain. If a "
		"sinusoidal signal close to the border of the DRM signal is "
		"considered, its spectrum will not be a distinct peak but a "
		"shifted Sinc function. So its spectrum is broadened caused by "
		"the windowing. Thus, it will spread in the DRM spectrum and "
		"act as an in-band interferer.<br>"
		"There is a special case if the sinusoidal signal is in a "
		"distance of a multiple of the carrier spacing of the DRM signal. "
		"Since the Sinc function has zeros at certain positions it happens "
		"that in this case the zeros are exactly at the sub-carrier "
		"frequencies of the DRM signal. In this case, no interference takes "
		"place. If the sinusoidal signal is in a distance of a multiple of "
		"the carrier spacing plus half of the carrier spacing away from the "
		"DRM signal, the interference reaches its maximum.<br>"
		"As a result, if only one DRM signal is present in the 20 kHz "
		"bandwidth, bandpass filtering has no effect. Also,  if the "
		"interferer is far away from the DRM signal, filtering will not "
		"give much improvement since the squared magnitude of the spectrum "
		"of the Sinc function is approx -15 dB down at 1 1/2 carrier "
		"spacing (approx 70 Hz with DRM mode B) and goes down to approx "
		"-30 dB at 10 times the carrier spacing plus 1 / 2 of the carrier "
		"spacing (approx 525 Hz with DRM mode B). The bandpass filter must "
		"have very sharp edges otherwise the gain in performance will be "
		"very small.</li>"
		"<li><b>Modified Metrics:</b> Based on the "
		"information from the SNR versus sub-carrier estimation, the metrics "
		"for the Viterbi decoder can be modified so that sub-carriers with "
		"high noise are attenuated and do not contribute too much to the "
		"decoding result. That can improve reception under bad conditions but "
		"may worsen the reception in situations where a lot of fading happens "
		"and no interferer are present since the SNR estimation may be "
		"not correct.</li></ul>");

	QWhatsThis::add(CheckBoxRecFilter, strInterfRej);
	QWhatsThis::add(CheckBoxModiMetric, strInterfRej);
}
