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

#include <Q3ListView>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QValidator>
#include <QMessageBox>
#include <QCheckBox>
#include <QSlider>
#include <QRadioButton>
#include <Q3ButtonGroup>
#include <QTabWidget>
#include <QPixmap>
#include <QHideEvent>
#include <QShowEvent>
#include "ReceiverSettingsDlg.h"
#include "../GlobalDefinitions.h"
#include "../util/Hamlib.h"

/* Implementation *************************************************************/

ReceiverSettingsDlg::ReceiverSettingsDlg(
    ReceiverInterface& NRx,
    CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, Qt::WFlags f) :
	QDialog(parent, name, modal, f), Ui_ReceiverSettingsDlg(),
	Receiver(NRx), Settings(NSettings), loading(true),
	no_rig(NULL), no_port(NULL), last_port(NULL), TimerRig(), iWantedrigModel(0)
{
    setupUi(this);

	bool bEnableRig = true;
#ifdef HAVE_LIBHAMLIB
	/* Rig Selection */
	ListViewRig->setSelectionMode(Q3ListView::Single);
	ListViewRig->setRootIsDecorated(true);
	ListViewRig->setAllColumnsShowFocus(true);
	ListViewRig->setColumnText(0, "Rig");
	ListViewRig->addColumn("ID");
	ListViewRig->addColumn("Status");
	ListViewRig->clear();
	/* COM port selection --------------------------------------------------- */
	ListViewPort->setSelectionMode(Q3ListView::Single);
	ListViewPort->setAllColumnsShowFocus(true);
	ListViewPort->setColumnText(0, tr("Name"));
	ListViewPort->addColumn(tr("Port"));
	ListViewPort->clear();
#else
	/* Tabs */
	bEnableRig = false;
#endif

	if(Receiver.UpstreamDIInputEnabled())
		bEnableRig = false;

	if(bEnableRig == false)
	{
		TabWidget->removePage(Rig);
	}

	/* Connections */

	connect(buttonOk, SIGNAL(clicked()), SLOT(ButtonOkClicked()) );
	connect(LineEditLatDegrees, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLatDegChanged(const QString&)));
	connect(LineEditLatMinutes, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLatMinChanged(const QString&)));
	connect(ComboBoxNS, SIGNAL(highlighted(int)), SLOT(OnComboBoxNSHighlighted(int)) );
	connect(LineEditLngDegrees, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLngDegChanged(const QString&)));
	connect(LineEditLngMinutes, SIGNAL(textChanged(const QString&)), SLOT(OnLineEditLngMinChanged(const QString&)));
	connect(ComboBoxEW, SIGNAL(highlighted(int)), SLOT(OnComboBoxEWHighlighted(int)) );

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
	connect(CheckBoxDisplayGPS, SIGNAL(clicked()), SLOT(OnCheckBoxDisplayGPS()) );
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

	connect(ListViewRig, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnRigSelected(Q3ListViewItem*)));
	connect(ListViewPort, SIGNAL(selectionChanged(Q3ListViewItem*)),
		this, SLOT(OnComPortSelected(Q3ListViewItem*)));

	connect(&TimerRig, SIGNAL(timeout()), this, SLOT(OnTimerRig()));

	TimerRig.stop();

	/* Set help text for the controls */
	AddWhatsThisHelp();

	Q3ButtonGroup* bg = new Q3ButtonGroup(this);
	bg->hide();
	bg->insert(RadioButtonAll, 0);
	bg->insert(RadioButtonPerMode, 1);


	setDefaults();
}

ReceiverSettingsDlg::~ReceiverSettingsDlg()
{
	double latitude, longitude;
	Receiver.GetParameters()->GPSData.GetLatLongDegrees(latitude, longitude);
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);

	if(Receiver.GetIsWriteWaveFile())
		Receiver.StopWriteWaveFile();

	Settings.Put("Receiver", "rigpermode", RadioButtonPerMode->isChecked());
}

void ReceiverSettingsDlg::hideEvent(QHideEvent*)
{
	ListViewRig->clear();
	ListViewPort->clear();
	no_rig = NULL;
	no_port = NULL;
	last_port = NULL;
}

/* this sets default values into the dialog and ini file for
 * items not covered in other places. It is currently called
 * from the constructor and contains items which can only
 * be modified in this dialog or on the command line.
 * (lat/long is still looking for a good home)
 */
void ReceiverSettingsDlg::setDefaults()
{
	CParameter& Parameters = *(Receiver.GetParameters());

	/* these won't get into the ini file unless we use GPS or have this: */
	double latitude = Settings.Get("Logfile", "latitude", 100.0);
	double longitude = Settings.Get("Logfile", "longitude", 0.0);
	if(latitude<=90.0)
	{
		Parameters.GPSData.SetPositionAvailable(true);
		Parameters.GPSData.SetGPSSource(CGPSData::GPS_SOURCE_MANUAL_ENTRY);
		Parameters.GPSData.SetLatLongDegrees(latitude, longitude);
	}
	else
	{
		latitude = 0.0;
		Parameters.GPSData.SetPositionAvailable(false);
	}

	/* Start log file flag */
	CheckBoxWriteLog->setChecked(Settings.Get("Logfile", "enablelog", false));

    /* log file flag for storing signal strength in long log */
	CheckBoxLogSigStr->setChecked(Settings.Get("Logfile", "enablerxl", false));

	/* log file flag for storing lat/long in long log */
	CheckBoxLogLatLng->setChecked(Settings.Get("Logfile", "enablepositiondata", false));

	/* logging delay value */
	int iLogDelay = Settings.Get("Logfile", "delay", 0);
	SliderLogStartDelay->setValue(iLogDelay);

	/* GPS ------------------------------------------------------------------- */
	string host = Settings.Get("GPS", "host", string("localhost"));
    LineEditGPSHost->setText(host.c_str());

	int port = Settings.Get("GPS", "port", 2947);
    LineEditGPSPort->setText(QString("%1").arg(port));

	CheckBoxUseGPS->setChecked(Settings.Get("GPS", "usegpsd", false));
	CheckBoxDisplayGPS->setChecked(Settings.Get("GPS", "showgps", false));

	if(Settings.Get("Receiver", "rigpermode", false))
		RadioButtonPerMode->setChecked(true);
	else
		RadioButtonAll->setChecked(true);

	/* get the defaults into the ini file */
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);
	Settings.Put("Logfile", "enablelog", CheckBoxWriteLog->isChecked());
	Settings.Put("Logfile", "enablerxl", CheckBoxLogSigStr->isChecked());
	Settings.Put("Logfile", "enablepositiondata", CheckBoxLogLatLng->isChecked());
	Settings.Put("Logfile", "delay", iLogDelay);
	Settings.Put("GPS", "usegpsd", CheckBoxUseGPS->isChecked());
	Settings.Put("GPS", "showgps", CheckBoxDisplayGPS->isChecked());
	Settings.Put("GPS", "host", host);
	Settings.Put("GPS", "port", port);
}

void ReceiverSettingsDlg::showEvent(QShowEvent*)
{
	loading = true; // prevent executive actions during reading state

	/* Sync ----------------------------------------------------------------- */
	if (Receiver.GetTimeInt() == TWIENER)
		RadioButtonTiWiener->setChecked(true);
	else
		RadioButtonTiLinear->setChecked(true);

	switch(Receiver.GetFreqInt())
	{
	case FLINEAR:
		RadioButtonFreqLinear->setChecked(true);
		break;
	case FDFTFILTER:
		RadioButtonFreqDFT->setChecked(true);
		break;
	case FWIENER:
		RadioButtonFreqWiener->setChecked(true);
	}

	if (Receiver.GetTiSyncTracType() == TSFIRSTPEAK)
		RadioButtonTiSyncFirstPeak->setChecked(true);
	else
		RadioButtonTiSyncEnergy->setChecked(true);

	/* Misc ----------------------------------------------------------------- */
	CheckBoxRecFilter->setChecked(Receiver.GetRecFilter());
	CheckBoxModiMetric->setChecked(Receiver.GetIntCons());
	CheckBoxFlipSpec->setChecked(Receiver.GetFlippedSpectrum());
	SliderNoOfIterations->setValue(Receiver.GetInitNumIterations());

	/* Audio ---------------------------------------------------------------- */

	CheckBoxMuteAudio->setChecked(Receiver.GetMuteAudio());
	CheckBoxReverb->setChecked(Receiver.GetReverbEffect());
	CheckBoxSaveAudioWave->setChecked(Receiver.GetIsWriteWaveFile());

	/* GPS */
	ExtractReceiverCoordinates();

	/* Rig */
#ifdef HAVE_LIBHAMLIB
	QPixmap	BitmLittleGreenSquare;
	BitmLittleGreenSquare.resize(5, 5);
	BitmLittleGreenSquare.fill(QColor(0, 255, 0));

// TODO chose one rig for everything or a rig per band
	map<rig_model_t, CRigCaps> rigs;
	map<string,Q3ListViewItem*> manufacturers;
	rig_model_t current = Receiver.GetRigModel();

	CheckBoxEnableSMeter->setChecked(Receiver.GetEnableSMeter());

	Receiver.GetRigList(rigs);

	no_rig = new Q3ListViewItem(new Q3ListViewItem(ListViewRig, tr("[None]")), tr("None"), "0", "");
	Q3ListViewItem* selected_rig = no_rig;
	for (map<rig_model_t, CRigCaps>::const_iterator i=rigs.begin(); i!=rigs.end(); i++)
	{
		/* Store model ID */
		Q3ListViewItem* man, *model=NULL;
		rig_model_t iModelID = i->first;
		const CRigCaps& rig = i->second;

		string m;
		if(rig.hamlib_caps.mfg_name == NULL)
			continue;
		m = rig.hamlib_caps.mfg_name;
		map<string,Q3ListViewItem*>::const_iterator mfr = manufacturers.find(m);
		if(mfr==manufacturers.end())
		{
			manufacturers[m] = man = new Q3ListViewItem(ListViewRig, m.c_str());
		}
		else
		{
			man = mfr->second;
		}

		if(iModelID<0)
		{
			string model_name = string(rig.hamlib_caps.model_name)+" (DRM)";
			model = new Q3ListViewItem(
				man,
				model_name.c_str(),
				QString::number(iModelID),
				rig_strstatus(rig.hamlib_caps.status)
			);
			model->setPixmap(0, BitmLittleGreenSquare);
			man->setPixmap(0, BitmLittleGreenSquare);
		}
		else
		{
			model = new Q3ListViewItem(
				man, rig.hamlib_caps.model_name,
				QString::number(iModelID),
				rig_strstatus(rig.hamlib_caps.status)
			);
		}
		/* Check for selected Rig */
		if (current == iModelID)
		{
			selected_rig = model;
		}
	}

	/* COM port selection --------------------------------------------------- */
	no_port = new Q3ListViewItem(ListViewPort, tr("None"), "");
	map<string,string> ports;
	Receiver.GetComPortList(ports);
	for(map<string,string>::iterator p=ports.begin(); p!=ports.end(); p++)
	{
		last_port = new Q3ListViewItem(ListViewPort, p->first.c_str(), p->second.c_str());
	}

	checkRig(current);

	/* do this last so can update the com port, etc depending on rig type */
	ListViewRig->ensureItemVisible(selected_rig);
	ListViewRig->setSelected(selected_rig, true);
#endif
	loading = false; // loading completed
}

void ReceiverSettingsDlg::OnCheckBoxUseGPS()
{
	Settings.Put("GPS", "host", string(LineEditGPSHost->text().latin1()));
	Settings.Put("GPS", "port", LineEditGPSPort->text().toInt());
	Settings.Put("GPS", "usegpsd", CheckBoxUseGPS->isChecked());
	emit StartStopGPS(CheckBoxUseGPS->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxDisplayGPS()
{
	Settings.Put("GPS", "showgps", CheckBoxDisplayGPS->isChecked());
	emit ShowHideGPS(CheckBoxDisplayGPS->isChecked());
}

void ReceiverSettingsDlg::OnLineEditLatDegChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLatMinChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnComboBoxNSHighlighted(int)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLngDegChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnLineEditLngMinChanged(const QString&)
{
	SetLatLng();
}

void ReceiverSettingsDlg::OnComboBoxEWHighlighted(int)
{
	SetLatLng();
}

void ReceiverSettingsDlg::SetLatLng()
{
	CParameter& Parameters = *Receiver.GetParameters();
	double latitude, longitude;

	longitude = (LineEditLngDegrees->text().toDouble()
				+ LineEditLngMinutes->text().toDouble()/60.0
				)*((ComboBoxEW->currentText()=="E")?1:-1);

	latitude = (LineEditLatDegrees->text().toDouble()
				+ LineEditLatMinutes->text().toDouble()/60.0
				)*((ComboBoxNS->currentText()=="N")?1:-1);

	Parameters.Lock();
	Parameters.GPSData.SetPositionAvailable(true);
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
	CParameter& Parameters = *Receiver.GetParameters();

	double latitude, longitude;

	Parameters.Lock();
	Parameters.GPSData.GetLatLongDegrees(latitude, longitude);
	Parameters.Unlock();

	if(latitude<0.0)
	{
		latitude = 0.0 - latitude;
		ComboBoxNS->setCurrentItem(1);
	}
	else
	{
		ComboBoxNS->setCurrentItem(0);
	}
	LineEditLatDegrees->setText(QString::number(int(latitude)));
	LineEditLatMinutes->setText(QString::number(60.0*(latitude-int(latitude))));
	if(longitude<0.0)
	{
		longitude = 0.0 - longitude;
		ComboBoxEW->setCurrentItem(1);
	}
	else
	{
		ComboBoxEW->setCurrentItem(0);
	}
	LineEditLngDegrees->setText(QString::number(int(longitude)));
	LineEditLngMinutes->setText(QString::number(60.0*(longitude-int(longitude))));
}

void ReceiverSettingsDlg::OnRadioTimeLinear()
{
	if (Receiver.GetTimeInt() != TLINEAR)
		Receiver.SetTimeInt(TLINEAR);
}

void ReceiverSettingsDlg::OnRadioTimeWiener()
{
	if (Receiver.GetTimeInt() != TWIENER)
		Receiver.SetTimeInt(TWIENER);
}

void ReceiverSettingsDlg::OnRadioFrequencyLinear()
{
	if (Receiver.GetFreqInt() != FLINEAR)
		Receiver.SetFreqInt(FLINEAR);
}

void ReceiverSettingsDlg::OnRadioFrequencyDft()
{
	if (Receiver.GetFreqInt() != FDFTFILTER)
		Receiver.SetFreqInt(FDFTFILTER);
}

void ReceiverSettingsDlg::OnRadioFrequencyWiener()
{
	if (Receiver.GetFreqInt() != FWIENER)
		Receiver.SetFreqInt(FWIENER);
}

void ReceiverSettingsDlg::OnRadioTiSyncFirstPeak()
{
	if (Receiver.GetTiSyncTracType() !=
		TSFIRSTPEAK)
	{
		Receiver.SetTiSyncTracType(TSFIRSTPEAK);
	}
}

void ReceiverSettingsDlg::OnRadioTiSyncEnergy()
{
	if (Receiver.GetTiSyncTracType() != TSENERGY)
	{
		Receiver.SetTiSyncTracType(TSENERGY);
	}
}

void ReceiverSettingsDlg::OnSliderIterChange(int value)
{
	/* Set new value in working thread module */
	Receiver.SetNumIterations(value);

	/* Show the new value in the label control */
	TextNumOfIterations->setText(tr("MLC: Number of Iterations: ") +
		QString().setNum(value));
}

void ReceiverSettingsDlg::OnCheckFlipSpectrum()
{
	/* Set parameter in working thread module */
	Receiver.SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
}

void ReceiverSettingsDlg::OnCheckRecFilter()
{
	/* Set parameter in working thread module */
	Receiver.SetRecFilter(CheckBoxRecFilter->isChecked());

	/* If filter status is changed, a new aquisition is necessary */
    CParameter& Parameters = *Receiver.GetParameters();
    Parameters.Lock();
    Parameters.RxEvent = Reinitialise;
    Parameters.Unlock();
}

void ReceiverSettingsDlg::OnCheckModiMetric()
{
	/* Set parameter in working thread module */
	Receiver.SetIntCons(CheckBoxModiMetric->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxMuteAudio()
{
	/* Set parameter in working thread module */
	Receiver.MuteAudio(CheckBoxMuteAudio->isChecked());
}

void ReceiverSettingsDlg::OnCheckBoxReverb()
{
	/* Set parameter in working thread module */
	Receiver.SetReverbEffect(CheckBoxReverb->isChecked());
}

void ReceiverSettingsDlg::OnCheckSaveAudioWAV()
{
/*
	This code is copied in AnalogMainWindow.cpp. If you do changes here, you should
	apply the changes in the other file, too
*/
	if (CheckBoxSaveAudioWave->isChecked() == true)
	{
		/* Show "save file" dialog */
		QString strFileName =
			QFileDialog::getSaveFileName(tr("DreamOut.wav"), "*.wav", this);

		/* Check if user not hit the cancel button */
		if (!strFileName.isNull())
		{
			Receiver.StartWriteWaveFile(strFileName.latin1());
		}
		else
		{
			/* User hit the cancel button, uncheck the button */
			CheckBoxSaveAudioWave->setChecked(false);
		}
	}
	else
		Receiver.StopWriteWaveFile();
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
	Receiver.SetEnableSMeter(on);
}

void ReceiverSettingsDlg::checkRig(int iID)
{
#ifdef HAVE_LIBHAMLIB
	/* is s-meter enabled ? */
	CheckBoxEnableSMeter->setChecked(Receiver.GetEnableSMeter());

	if(iID == 0)
	{
		ListViewPort->setEnabled(true);
		ListViewPort->clearSelection();
		return;
	}

	CRigCaps caps;
	Receiver.GetRigCaps(iID, caps);
	if(caps.hamlib_caps.port_type == RIG_PORT_SERIAL)
	{
		ListViewPort->setEnabled(true);
		string strPort = Receiver.GetRigComPort();
		if(strPort!="")
		{
			last_port = no_port;
			Q3ListViewItemIterator it( ListViewPort );
			for ( ; it.current(); ++it )
			{
				if ( it.current()->text(1).latin1() == strPort.c_str() )
					last_port = it.current();
			}
			ListViewPort->ensureItemVisible(last_port);
			ListViewPort->setSelected(last_port, true);
		}
	}
	else
	{
		ListViewPort->setEnabled(false);
		ListViewPort->clearSelection();
	}
#endif
}

void ReceiverSettingsDlg::OnRigSelected(Q3ListViewItem* item)
{
	if(loading)
		return;

#ifdef HAVE_LIBHAMLIB
	iWantedrigModel = item->text(1).toInt();
	CRigCaps caps;
	Receiver.GetRigCaps(iWantedrigModel, caps);
	if(caps.hamlib_caps.port_type == RIG_PORT_SERIAL)
	{
		ListViewPort->setEnabled(true);
		if(last_port == NULL || last_port == no_port)
			return;
		/*
		string strPort = Receiver.GetRigComPort();
		if(last_port == NULL || last_port == no_port)
		{
			last_port = ListViewPort->firstChild();
		}
		if(strPort=="")
		{
			Receiver.SetRigComPort(last_port->text(1).latin1());
		}
		else
		{
			QListViewItemIterator it( ListViewPort );
			for ( ; it.current(); ++it )
			{
				if ( it.current()->text(1).latin1() == strPort.c_str() )
					last_port = it.current();
			}
		}
		*/
		Receiver.SetRigComPort(last_port->text(1).latin1());
		ListViewPort->ensureItemVisible(last_port);
		ListViewPort->setSelected(last_port, true);
	}

	if(RadioButtonAll->isChecked())
		Receiver.SetRigModelForAllModes(iWantedrigModel);
	else
		Receiver.SetRigModel(iWantedrigModel);

	TimerRig.start(500);
#endif
}

void ReceiverSettingsDlg::OnTimerRig()
{
#ifdef HAVE_LIBHAMLIB
	if(Receiver.GetRigChangeInProgress())
		return;

	TimerRig.stop();

	rig_model_t current = Receiver.GetRigModel();
	if(current == iWantedrigModel)
	{
		checkRig(current);
	}
	else
	{
		// could not select this rig
		QMessageBox::information( this, "Dream", "failed to select rig" );
		ListViewRig->ensureItemVisible(no_rig);
		ListViewRig->setSelected(no_rig, true);
	}
#endif
}

void ReceiverSettingsDlg::OnComPortSelected(Q3ListViewItem* item)
{
	if(loading)
		return;
	string s = item->text(1).latin1();
	last_port = item;
#ifdef HAVE_LIBHAMLIB
	Receiver.SetRigComPort(s);
#endif
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

    LineEditLatDegrees->setWhatsThis( strGPS);
    LineEditLatMinutes->setWhatsThis( strGPS);
    LineEditLngDegrees->setWhatsThis( strGPS);
    LineEditLngMinutes->setWhatsThis( strGPS);

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

	TextNumOfIterations->setWhatsThis( strNumOfIterations);
	SliderNoOfIterations->setWhatsThis( strNumOfIterations);

	/* Flip Input Spectrum */
	CheckBoxFlipSpec->setWhatsThis(
		tr("<b>Flip Input Spectrum:</b> Checking this box "
		"will flip or invert the input spectrum. This is necessary if the "
		"mixer in the front-end uses the lower side band."));

	/* Mute Audio */
	CheckBoxMuteAudio->setWhatsThis(
		tr("<b>Mute Audio:</b> The audio can be muted by "
		"checking this box. The reaction of checking or unchecking this box "
		"is delayed by approx. 1 second due to the audio buffers."));

	/* Reverberation Effect */
	CheckBoxReverb->setWhatsThis(
		tr("<b>Reverberation Effect:</b> If this check box is checked, a "
		"reverberation effect is applied each time an audio drop-out occurs. "
		"With this effect it is possible to mask short drop-outs."));

	/* Log File */
	CheckBoxWriteLog->setWhatsThis(
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

	RadioButtonFreqWiener->setWhatsThis( strWienerChanEst);
	RadioButtonTiWiener->setWhatsThis( strWienerChanEst);

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

	RadioButtonFreqLinear->setWhatsThis( strLinearChanEst);
	RadioButtonTiLinear->setWhatsThis( strLinearChanEst);

	/* DFT Zero Pad */
	RadioButtonFreqDFT->setWhatsThis(
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
	RadioButtonTiSyncEnergy->setWhatsThis(
		tr("<b>Guard Energy:</b> Time synchronization "
		"tracking algorithm utilizes the estimation of the impulse response. "
		"This method tries to maximize the energy in the guard-interval to set "
		"the correct timing."));

	/* First Peak */
	RadioButtonTiSyncFirstPeak->setWhatsThis(
		tr("<b>First Peak:</b> This algorithms searches for "
		"the first peak in the estimated impulse response and moves this peak "
		"to the beginning of the guard-interval (timing tracking algorithm)."));

	/* Save audio as wave */
	CheckBoxSaveAudioWave->setWhatsThis(
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

	CheckBoxRecFilter->setWhatsThis( strInterfRej);
	CheckBoxModiMetric->setWhatsThis( strInterfRej);
}
