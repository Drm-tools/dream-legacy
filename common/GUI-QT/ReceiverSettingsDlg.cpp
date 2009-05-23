/******************************************************************************\
 * British Broadcasting Corporation * Copyright (c) 2007
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

#include "ReceiverSettingsDlg.h"
#include "../GlobalDefinitions.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QButtonGroup>
#include <algorithm>
#include <iostream>

/* Implementation *************************************************************/


#ifdef HAVE_LIBHAMLIB

int
RigTypesModel::rowCount ( const QModelIndex & parent ) const
{
    if(parent.isValid())
    {
    	const model_index* r = (const model_index*)parent.internalPointer();
    	if(r->parent==-1) // its a make - what we expect!
    	{
		const make *m = reinterpret_cast<const make*>(r);
		return m->model.size();
    	}
    	else // its a model - stop descending
    	{
		return 0;
    	}
    }
    else
    {
	return rigs.size();
    }
}

int
RigTypesModel::columnCount ( const QModelIndex & parent) const
{
    return 1;
}

QVariant
RigTypesModel::data ( const QModelIndex & index, int role) const
{
    const model_index* i = (const model_index*)index.internalPointer();
    if(i->parent==-1)
    {
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    if(index.column()==0)
	    {
		if(int(rigs.size())>index.row())
		    return rigs[index.row()].name.c_str();
	    }
	    break;
	case Qt::UserRole:
	    break;
	case Qt::TextAlignmentRole:
	    break;
	}
    }
    else
    {
	const rig *r = reinterpret_cast<const rig*>(i);
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    switch(index.column())
	    {
		case 0:
		    return r->name.c_str();
		break;
	    }
	    break;
	case Qt::UserRole:
	    {
		return r->model;
	    }
	    break;
	case Qt::TextAlignmentRole:
	    switch(index.column())
	    {
		case 1:
			return QVariant(Qt::AlignRight|Qt::AlignVCenter);
			break;
		default:
			return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
	    }
	}
    }
    return QVariant();
}

QVariant
RigTypesModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
	    return QVariant();
    if(orientation != Qt::Horizontal)
	    return QVariant();
    switch(section)
    {
	case 0: return tr("Rig Make/Model"); break;
    }
    return "";
}

QModelIndex
RigTypesModel::index(int row, int column, const QModelIndex &parent) const
{
    if(parent.isValid())
    {
    	const model_index* r = (const model_index*)parent.internalPointer();
    	if(r->parent==-1) // its a make - what we expect!
    	{
		const make *m = reinterpret_cast<const make*>(r);
		if(int(m->model.size())>row)
		{
		    return createIndex(row, column, (void*)&m->model[row]);
		}
    	} // else its wrong, drop through
    }
    else
    {
    	if(int(rigs.size())>row)
    	{
	    return createIndex(row, column, (void*)&rigs[row]);
    	}
    }
    return QModelIndex();
}

QModelIndex
RigTypesModel::parent(const QModelIndex &child) const
{
    if(child.isValid())
    {
    	const model_index* r = (const model_index*)child.internalPointer();
    	if(r->parent!=-1) // its a model - what we expect!
    	{
	    size_t p = r->parent;
	    return createIndex(p, 0, (void*)&rigs[p]);
    	} // else its wrong, drop through
    }
    return QModelIndex();
}

void
RigTypesModel::load(ReceiverInterface& Receiver)
{
    CRigMap r;
    Receiver.GetRigList(r);
    for(map<string,map<string,rig_model_t> >::const_iterator
	i=r.rigs.begin(); i!=r.rigs.end(); i++)
    {
	make m;
	m.name = i->first;
	m.parent = -1;
	size_t n = rigs.size();
	for(map<string,rig_model_t>::const_iterator
	    j = i->second.begin(); j!=i->second.end(); j++)
	{
	    rig r;
	    r.name = j->first;
	    r.model = j->second;
	    r.parent = n;
	    m.model.push_back(r);
	}
	rigs.push_back(m);
    }
    reset();
}

RigModel::RigModel() : QAbstractItemModel(),BitmLittleGreenSquare(5,5),rigs()
{
    BitmLittleGreenSquare.fill(QColor(0, 255, 0));
}

QModelIndex RigModel::index(int row, int column,
		  const QModelIndex &parent) const
{
    if(parent.isValid())
    {
    }
    else
    {
	return createIndex(row, column, (void*)&rigs[row]);
    }
    return QModelIndex();
}

QModelIndex RigModel::parent(const QModelIndex &child) const
{
    if(child.isValid())
    {
    	const model_index* r = (const model_index*)child.internalPointer();
    	if(r->parent!=-1) // its a model - what we expect!
    	{
	    size_t p = r->parent;
	    return createIndex(p, 0, (void*)&rigs[p]);
    	} // else its wrong, drop through
    }
    return QModelIndex();
}

int RigModel::rowCount (const QModelIndex& parent) const
{
    if(parent.isValid())
    {
	return 0;
    }
    else
    {
	return rigs.size();
    }
}

int RigModel::columnCount (const QModelIndex& parent) const
{
	return 3;
}

QVariant RigModel::data (const QModelIndex& index, int role) const
{
    const model_index* i = (const model_index*)index.internalPointer();
    if(i->parent==-1)
    {
	switch(role)
	{
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    switch(index.column())
	    {
		case 0:
		    return rigs[index.row()].caps.hamlib_caps.model_name;
		break;
		case 1:
		    return rigs[index.row()].caps.hamlib_caps.rig_model;
		    break;
		case 2:
		    return rig_strstatus(rigs[index.row()].caps.hamlib_caps.status);
		    break;
	    }
	    break;
	case Qt::UserRole:
	    return rigs[index.row()].caps.hamlib_caps.rig_model;
	    break;
	case Qt::TextAlignmentRole:
	    switch(index.column())
	    {
		case 1:
			return QVariant(Qt::AlignRight|Qt::AlignVCenter);
			break;
		default:
			return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
	    }
	    break;
	}
    }
    else
    {
#if 0
	const rig *r = reinterpret_cast<const rig*>(i);
	switch(role)
	{
	case Qt::DecorationRole:
	    if(index.column()==0)
	    {
		QIcon icon;
		//icon.addPixmap(BitmCubeGreen);
		return icon;
	    }
	    break;
	case Qt::DisplayRole:
	    break;
	case Qt::UserRole:
	    break;
	case Qt::TextAlignmentRole:
	}
#endif
    }
    return QVariant();
}

QVariant RigModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
    if(role != Qt::DisplayRole)
	    return QVariant();
    if(orientation != Qt::Horizontal)
	    return QVariant();
    switch(section)
    {
	case 0: return tr("Rig"); break;
	case 1: return tr("ID"); break;
	case 2: return tr("Status"); break;
    }
    return "";
}

void
RigModel::add(const CRigCaps& caps)
{
    rig r;
    //memcpy(&r.caps, &caps, sizeof(CRigCaps));
    r.caps = caps;
    r.parent = -1;
    rigs.push_back(r);
    reset();
}
#endif

ReceiverSettingsDlg::ReceiverSettingsDlg(
    ReceiverInterface& NRx,
    CSettings& NSettings,
	QWidget* parent, Qt::WFlags f) :
	QDialog(parent, f), Ui_ReceiverSettingsDlg(),
	Receiver(NRx), Settings(NSettings), loading(true),
	TimerRig(),iWantedrigModel(0),
	bgTimeInterp(NULL), bgFreqInterp(NULL), bgTiSync(NULL), bgChanSel(NULL)
{
    setupUi(this);

    bool bEnableRig = false;
#ifdef HAVE_LIBHAMLIB
    bEnableRig = true;
    treeViewRigTypes->setModel(&RigTypes);
    RigTypes.load(Receiver);
    treeViewRigs->setModel(&Rigs);
#endif

    if(Receiver.UpstreamDIInputEnabled())
	bEnableRig = false;

    if(bEnableRig == false)
    {
	    TabWidget->removeTab(TabWidget->indexOf(Rig));
    }

    /* Connections */

    connect(buttonClose, SIGNAL(clicked()), SLOT(OnButtonClose()) );
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
    bgTimeInterp = new QButtonGroup(this);
    bgTimeInterp->addButton(RadioButtonTiWiener, 0);
    bgTimeInterp->addButton(RadioButtonTiLinear, 0);
    bgFreqInterp = new QButtonGroup(this);
    bgFreqInterp->addButton(RadioButtonFreqWiener, 0);
    bgFreqInterp->addButton(RadioButtonFreqLinear, 0);
    bgFreqInterp->addButton(RadioButtonFreqDFT, 0);
    bgTiSync = new QButtonGroup(this);
    bgTiSync->addButton(RadioButtonTiSyncEnergy, 0);
    bgTiSync->addButton(RadioButtonTiSyncFirstPeak, 0);
    bgChanSel = new QButtonGroup(this);
    bgChanSel->addButton(radioButtonLeft, CS_LEFT_CHAN);
    bgChanSel->addButton(radioButtonRight, CS_RIGHT_CHAN);
    bgChanSel->addButton(radioButtonMix, CS_MIX_CHAN);
    bgChanSel->addButton(radioButtonIQPos, CS_IQ_POS);
    bgChanSel->addButton(radioButtonIQNeg, CS_IQ_NEG);
    bgChanSel->addButton(radioButtonIQPosZero, CS_IQ_POS_ZERO);
    bgChanSel->addButton(radioButtonIQNegZero, CS_IQ_NEG_ZERO);
    connect(bgTimeInterp, SIGNAL(buttonClicked(int)),
	    this, SLOT(OnSelTimeInterp(int)));
    connect(bgFreqInterp, SIGNAL(buttonClicked(int)),
	    this, SLOT(OnSelFrequencyInterp(int)));
    connect(bgTiSync, SIGNAL(buttonClicked(int)),
	    this, SLOT(OnSelTiSync(int)));
    connect(bgChanSel, SIGNAL(buttonClicked(int)),
	    this, SLOT(OnSelInputChan(int)));

    /* Check boxes */
    connect(CheckBoxUseGPS, SIGNAL(clicked()), SLOT(OnCheckBoxUseGPS()) );
    connect(CheckBoxDisplayGPS, SIGNAL(clicked()), SLOT(OnCheckBoxDisplayGPS()) );
    connect(CheckBoxFlipSpec, SIGNAL(clicked()), this, SLOT(OnCheckFlipSpectrum()));
    connect(CheckBoxWriteLog, SIGNAL(clicked()), this, SLOT(OnCheckWriteLog()));
    connect(CheckBoxRecFilter, SIGNAL(clicked()), this, SLOT(OnCheckRecFilter()));
    connect(CheckBoxModiMetric, SIGNAL(clicked()), this, SLOT(OnCheckModiMetric()));
    connect(CheckBoxLogLatLng, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogLatLng()));
    connect(CheckBoxLogSigStr, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogSigStr()));

#ifdef HAVE_LIBHAMLIB
    connect(pushButtonAddRig, SIGNAL(clicked()), this, SLOT(OnButtonAddRig()));
    connect(pushButtonRemoveRig, SIGNAL(clicked()), this, SLOT(OnButtonRemoveRig()));
    //connect(CheckBoxEnableSMeter, SIGNAL(toggled(bool)), this, SLOT(OnCheckEnableSMeterToggled(bool)));
    connect(treeViewRigTypes, SIGNAL(clicked (const QModelIndex&)),
	    this, SLOT(OnRigSelected(const QModelIndex&)));

    connect(&TimerRig, SIGNAL(timeout()), this, SLOT(OnTimerRig()));

    connect(pushButtonApplyRig, SIGNAL(clicked()), this, SLOT(OnButtonApplyRigSettings()));
    TimerRig.stop();
    //TimerRig.setSingleShot(true);
#endif

    /* Set help text for the controls */
    AddWhatsThisHelp();

    setDefaults();
}

ReceiverSettingsDlg::~ReceiverSettingsDlg()
{
	double latitude, longitude;
	Receiver.GetParameters()->GPSData.GetLatLongDegrees(latitude, longitude);
	Settings.Put("Logfile", "latitude", latitude);
	Settings.Put("Logfile", "longitude", longitude);
}

void ReceiverSettingsDlg::showEvent(QShowEvent*)
{
	loading = true; // prevent executive actions during reading state

	/* DRM ----------------------------------------------------------------- */
	bgTimeInterp->button(int(Receiver.GetTimeInt()))->setChecked(true);
	bgFreqInterp->button(int(Receiver.GetFreqInt()))->setChecked(true);
	bgTiSync->button(int(Receiver.GetTiSyncTracType()))->setChecked(true);

	CheckBoxRecFilter->setChecked(Receiver.GetRecFilter());
	CheckBoxModiMetric->setChecked(Receiver.GetIntCons());
	SliderNoOfIterations->setValue(Receiver.GetInitNumIterations());

	/* Input ----------------------------------------------------------------- */
	bgChanSel->button(int(Receiver.GetChannelSelection()))->setChecked(true);
	CheckBoxFlipSpec->setChecked(Receiver.GetFlippedSpectrum());

	/* GPS */
	ExtractReceiverCoordinates();

	/* Rig */
#ifdef HAVE_LIBHAMLIB

	// TODO chose one rig for everything or a rig per mode / ? band ?
	//map<rig_model_t, CRigCaps> rigs;
	//map<string,Q3ListViewItem*> manufacturers;

	rig_model_t current = 0;


	CheckBoxEnableSMeter->setChecked(Receiver.GetEnableSMeter());

	map<string,string> ports;
	Receiver.GetComPortList(ports);
	///comboBoxRigPort->addItem("None");
	for(map<string,string>::const_iterator
	    it = ports.begin(); it!=ports.end(); it++)
	{
	    //comboBoxRigPort->addItem(it->first.c_str(), it->second.c_str());
	}

	CRigCaps caps;
	Receiver.GetRigCaps(current, caps);
	if(caps.hamlib_caps.port_type == RIG_PORT_SERIAL)
	{
		//comboBoxRigPort->setEnabled(true);
		string strPort = Receiver.GetRigComPort();
		if(strPort!="")
		{
		    //comboBoxRigPort->setCurrentIndex(
			//comboBoxRigPort->findText(strPort.c_str()));
		}
	}
	else
	{
		//comboBoxRigPort->setEnabled(false);
		//comboBoxRigPort->setCurrentIndex(0);
	}
#endif
	loading = false; // loading completed
}

void ReceiverSettingsDlg::hideEvent(QHideEvent*)
{
#ifdef HAVE_LIBHAMLIB
#endif
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
/* when the dialog closes save the contents of any controls which don't have
 * their own slot handlers
 */

void ReceiverSettingsDlg::OnButtonClose()
{
	/* save current settings */
	Settings.Put("GPS", "host", LineEditGPSHost->text().toStdString());
	Settings.Put("GPS", "port", LineEditGPSPort->text().toInt());

	accept(); /* close the dialog */
}

// = GPS Tab ==============================================================

void ReceiverSettingsDlg::OnCheckBoxUseGPS()
{
	Settings.Put("GPS", "host", LineEditGPSHost->text().toStdString());
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
		ComboBoxNS->setCurrentIndex(1);
	}
	else
	{
		ComboBoxNS->setCurrentIndex(0);
	}
	LineEditLatDegrees->setText(QString::number(int(latitude)));
	LineEditLatMinutes->setText(QString::number(60.0*(latitude-int(latitude))));
	if(longitude<0.0)
	{
		longitude = 0.0 - longitude;
		ComboBoxEW->setCurrentIndex(1);
	}
	else
	{
		ComboBoxEW->setCurrentIndex(0);
	}
	LineEditLngDegrees->setText(QString::number(int(longitude)));
	LineEditLngMinutes->setText(QString::number(60.0*(longitude-int(longitude))));
}

// = DRM Tab ==============================================================

void ReceiverSettingsDlg::OnSelTimeInterp(int iId)
{
    Receiver.SetTimeInt(ETypeIntTime(iId));
}

void ReceiverSettingsDlg::OnSelFrequencyIterp(int iId)
{
    Receiver.SetFreqInt(ETypeIntFreq(iId));
}


void ReceiverSettingsDlg::OnSelTiSync(int iId)
{

    Receiver.SetTiSyncTracType(ETypeTiSyncTrac(iId));

}

void ReceiverSettingsDlg::OnSliderIterChange(int value)
{
	Receiver.SetNumIterations(value);
}

// input TAB
void ReceiverSettingsDlg::OnSelInputChan(int iId)
{
    Receiver.SetChannelSelection(EInChanSel(iId));
}

void ReceiverSettingsDlg::OnCheckFlipSpectrum()
{
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

void
ReceiverSettingsDlg::OnRigTypeSelected(const QModelIndex&)
{
}

void
ReceiverSettingsDlg::OnRigSelected(const QModelIndex&)
{
}

void
ReceiverSettingsDlg::OnButtonAddRig()
{
    CRigCaps caps;
    rig_model_t model = treeViewRigTypes->currentIndex().data(Qt::UserRole).toInt();
    Receiver.GetRigCaps(model, caps);
    Rigs.add(caps);
}

void
ReceiverSettingsDlg::OnButtonRemoveRig()
{
}

void
ReceiverSettingsDlg::OnButtonApplyRigSettings()
{
    if(loading)
	    return;

#ifdef HAVE_LIBHAMLIB
	iWantedrigModel = treeViewRigs->currentIndex().data(Qt::UserRole).toInt();
	CRigCaps caps;
	Receiver.GetRigCaps(iWantedrigModel, caps);
	if(caps.hamlib_caps.port_type == RIG_PORT_SERIAL)
	{
	    string strPort = "";//comboBoxRigPort->currentText().toStdString();
	    if(strPort!="")
	    {
		    Receiver.SetRigComPort(strPort);
	    }
	}

	Receiver.SetRigModel(iWantedrigModel);
#endif
	TimerRig.start(500);
}

#if 0
void ReceiverSettingsDlg::OnRigSelected(const QModelIndex& index)
{
#ifdef HAVE_LIBHAMLIB
    int iWantedRigModel = index.data(Qt::UserRole).toInt();
    CRigCaps caps;
    Receiver.GetRigCaps(iWantedRigModel, caps);
    comboBoxRigPort->setEnabled(caps.hamlib_caps.port_type == RIG_PORT_SERIAL);
#endif
}
#endif

void ReceiverSettingsDlg::OnTimerRig()
{
#ifdef HAVE_LIBHAMLIB
    if(Receiver.GetRigChangeInProgress())
	    return;
    labelRigInfo->setText(Receiver.GetRigInfo().c_str());

    TimerRig.stop();
#endif
}

#if 0
void ReceiverSettingsDlg::OnComboBoxRigPort(int)
{
    if(loading)
	    return;
#ifdef HAVE_LIBHAMLIB
    Receiver.SetRigComPort(comboBoxRigPort->currentText().toStdString());
#endif
}
#endif

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

	SliderNoOfIterations->setWhatsThis( strNumOfIterations);

	/* Flip Input Spectrum */
	CheckBoxFlipSpec->setWhatsThis(
		tr("<b>Flip Input Spectrum:</b> Checking this box "
		"will flip or invert the input spectrum. This is necessary if the "
		"mixer in the front-end uses the lower side band."));

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
