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
#include "../selectioninterface.h"
#include "../util/Utilities.h"
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
		const make *m = dynamic_cast<const make*>(r);
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
    if(i==NULL)
	return QVariant();
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
		    return rigs[index.row()].caps.model_name;
		break;
		case 1:
		    return rigs[index.row()].caps.rig_model;
		    break;
		case 2:
		    return rig_strstatus(rigs[index.row()].caps.status);
		    break;
	    }
	    break;
	case Qt::UserRole:
	    {
	    	QVariant var;
		var.setValue(rigs[index.row()].caps);
		return var;
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
RigModel::add(const rig_caps& caps)
{
    rig r;
    r.caps = caps;
    r.parent = -1;
    rigs.push_back(r);
    reset();
}

SoundChoice::SoundChoice():QAbstractItemModel(), items()
{
}

int SoundChoice::rowCount (const QModelIndex&) const
{
    return items.size();
}

int SoundChoice::columnCount (const QModelIndex&) const
{
    return 1;
}

QVariant
SoundChoice::data (const QModelIndex& index, int role) const
{
    switch(role)
    {
	case Qt::DecorationRole:
	    break;
	case Qt::DisplayRole:
	    return items[index.row()].c_str();
	    break;
	case Qt::UserRole:
	    return index.internalId();
	    break;
	case Qt::TextAlignmentRole:
	    return QVariant(Qt::AlignLeft|Qt::AlignVCenter);
    }
    return QVariant();
}

QModelIndex SoundChoice::index(int row, int column, const QModelIndex &parent) const
{
  return createIndex(row, column, row);
}

QModelIndex SoundChoice::parent(const QModelIndex&) const
{
    return QModelIndex();
}

void SoundChoice::set(const CSelectionInterface& s)
{
	s.Enumerate(items);
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
	bgTimeInterp(NULL), bgFreqInterp(NULL), bgTiSync(NULL),
	bgDRMriq(NULL), bgDRMlrm(NULL), bgDRMiq(NULL),
	bgAMriq(NULL), bgAMlrm(NULL), bgAMiq(NULL),
	bgHamriq(NULL), bgHamlrm(NULL), bgHamiq(NULL)
#ifdef HAVE_LIBHAMLIB
	,RigTypes(),Rigs(),soundcards()
#endif
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

    comboBoxDRMRig->setModel(treeViewRigs->model());
    comboBoxDRMSound->setModel(&soundcards);
    bgDRMriq = new QButtonGroup(this);
    bgDRMriq->addButton(radioButtonDRMReal, 0);
    bgDRMriq->addButton(radioButtonDRMIQ, 1);
    bgDRMlrm = new QButtonGroup(this);
    bgDRMlrm->addButton(radioButtonDRMLeft, CS_LEFT_CHAN);
    bgDRMlrm->addButton(radioButtonDRMRight, CS_RIGHT_CHAN);
    bgDRMlrm->addButton(radioButtonDRMMix, CS_MIX_CHAN);
    bgDRMiq = new QButtonGroup(this);
    bgDRMiq->addButton(radioButtonDRMIQPos, CS_IQ_POS);
    bgDRMiq->addButton(radioButtonDRMIQNeg, CS_IQ_NEG);

    comboBoxAMRig->setModel(treeViewRigs->model());
    comboBoxAMSound->setModel(&soundcards);
    bgAMriq = new QButtonGroup(this);
    bgAMriq->addButton(radioButtonAMReal, 0);
    bgAMriq->addButton(radioButtonAMIQ, 1);
    bgAMlrm = new QButtonGroup(this);
    bgAMlrm->addButton(radioButtonAMLeft, CS_LEFT_CHAN);
    bgAMlrm->addButton(radioButtonAMRight, CS_RIGHT_CHAN);
    bgAMlrm->addButton(radioButtonAMMix, CS_MIX_CHAN);
    bgAMiq = new QButtonGroup(this);
    bgAMiq->addButton(radioButtonAMIQPos, CS_IQ_POS);
    bgAMiq->addButton(radioButtonAMIQNeg, CS_IQ_NEG);

    comboBoxFMRig->setModel(treeViewRigs->model());
    comboBoxFMSound->setModel(&soundcards);

    comboBoxHamRig->setModel(treeViewRigs->model());
    comboBoxHamSound->setModel(&soundcards);
    bgHamriq = new QButtonGroup(this);
    bgHamriq->addButton(radioButtonHamReal, 0);
    bgHamriq->addButton(radioButtonHamIQ, 1);
    bgHamlrm = new QButtonGroup(this);
    bgHamlrm->addButton(radioButtonHamLeft, CS_LEFT_CHAN);
    bgHamlrm->addButton(radioButtonHamRight, CS_RIGHT_CHAN);
    bgHamlrm->addButton(radioButtonHamMix, CS_MIX_CHAN);
    bgHamiq = new QButtonGroup(this);
    bgHamiq->addButton(radioButtonHamIQPos, CS_IQ_POS);
    bgHamiq->addButton(radioButtonHamIQNeg, CS_IQ_NEG);

    connect(pushButtonDRMApply, SIGNAL(clicked()), SLOT(OnButtonDRMApply()));
    connect(pushButtonAMApply, SIGNAL(clicked()), SLOT(OnButtonAMApply()));
    connect(pushButtonFMApply, SIGNAL(clicked()), SLOT(OnButtonFMApply()));
    connect(pushButtonHamApply, SIGNAL(clicked()), SLOT(OnButtonHamApply()));

    connect(bgDRMriq, SIGNAL(buttonClicked(int)), SLOT(OnRadioDRMRealIQ(int)));
    connect(bgAMriq, SIGNAL(buttonClicked(int)), SLOT(OnRadioAMRealIQ(int)));
    connect(bgHamriq, SIGNAL(buttonClicked(int)), SLOT(OnRadioHamRealIQ(int)));

    /* Check boxes */
    connect(CheckBoxUseGPS, SIGNAL(clicked()), SLOT(OnCheckBoxUseGPS()) );
    connect(CheckBoxDisplayGPS, SIGNAL(clicked()), SLOT(OnCheckBoxDisplayGPS()) );
    connect(CheckBoxWriteLog, SIGNAL(clicked()), this, SLOT(OnCheckWriteLog()));
    connect(CheckBoxRecFilter, SIGNAL(clicked()), this, SLOT(OnCheckRecFilter()));
    connect(CheckBoxModiMetric, SIGNAL(clicked()), this, SLOT(OnCheckModiMetric()));
    connect(CheckBoxLogLatLng, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogLatLng()));
    connect(CheckBoxLogSigStr, SIGNAL(clicked()), this, SLOT(OnCheckBoxLogSigStr()));

#ifdef HAVE_LIBHAMLIB
    connect(pushButtonAddRig, SIGNAL(clicked()), this, SLOT(OnButtonAddRig()));
    connect(pushButtonRemoveRig, SIGNAL(clicked()), this, SLOT(OnButtonRemoveRig()));
    //connect(CheckBoxEnableSMeter, SIGNAL(toggled(bool)), this, SLOT(OnCheckEnableSMeterToggled(bool)));
    connect(treeViewRigs, SIGNAL(clicked (const QModelIndex&)),
	    this, SLOT(OnRigSelected(const QModelIndex&)));

    connect(&TimerRig, SIGNAL(timeout()), this, SLOT(OnTimerRig()));

    connect(pushButtonConnectRig, SIGNAL(clicked()), this, SLOT(OnButtonConnectRig()));
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
	QAbstractButton* button = NULL;
	button = bgTimeInterp->button(int(Receiver.GetTimeInt()));
	if(button) button->setChecked(true);
	button = bgFreqInterp->button(int(Receiver.GetFreqInt()));
	if(button) button->setChecked(true);
	button = bgTiSync->button(int(Receiver.GetTiSyncTracType()));
	if(button) button->setChecked(true);

	CheckBoxRecFilter->setChecked(Receiver.GetRecFilter());
	CheckBoxModiMetric->setChecked(Receiver.GetIntCons());
	SliderNoOfIterations->setValue(Receiver.GetInitNumIterations());

	/* Input ----------------------------------------------------------------- */
	soundcards.set(*Receiver.GetSoundInInterface());

	comboBoxDRMSound->setCurrentIndex(Settings.Get("Input-DRM", "soundcard", int(0)));
	int riq = Settings.Get("Input-DRM", "mode", int(0));
	stackedWidgetDRMip->setCurrentIndex(riq);
	button = bgDRMriq->button(riq);
	if(button) button->setChecked(true);
	button = bgDRMlrm->button(Settings.Get("Input-DRM", "channels", int(0)));
	if(button) button->setChecked(true);
	button = bgDRMiq->button(Settings.Get("Input-DRM", "sign", int(0)));
	if(button) button->setChecked(true);
	CheckBoxDRMFlipSpec->setChecked(Settings.Get("Input-DRM", "flipspectrum", int(0)));

	comboBoxAMSound->setCurrentIndex(Settings.Get("Input-AM", "soundcard", int(0)));
	riq = Settings.Get("Input-AM", "mode", int(0));
	stackedWidgetAMip->setCurrentIndex(riq);
	button = bgAMriq->button(riq);
	if(button) button->setChecked(true);
	button = bgAMlrm->button(Settings.Get("Input-AM", "channels", int(0)));
	if(button) button->setChecked(true);
	button = bgAMiq->button(Settings.Get("Input-AM", "sign", int(0)));
	if(button) button->setChecked(true);
	CheckBoxAMFlipSpec->setChecked(Settings.Get("Input-AM", "flipspectrum", int(0)));

	comboBoxFMSound->setCurrentIndex(Settings.Get("Input-FM", "soundcard", int(0)));

	comboBoxHamSound->setCurrentIndex(Settings.Get("Input-Ham", "soundcard", int(0)));
	riq = Settings.Get("Input-Ham", "mode", int(0));
	stackedWidgetHamip->setCurrentIndex(riq);
	button = bgHamriq->button(riq);
	if(button) button->setChecked(true);
	button = bgHamlrm->button(Settings.Get("Input-Ham", "channels", int(0)));
	if(button) button->setChecked(true);
	button = bgHamiq->button(Settings.Get("Input-Ham", "sign", int(0)));
	if(button) button->setChecked(true);
	CheckBoxHamFlipSpec->setChecked(Settings.Get("Input-Ham", "flipspectrum", int(0)));
#ifdef HAVE_LIBHAMLIB
	comboBoxDRMRig->setCurrentIndex(Settings.Get("Input-DRM", "rig", int(0)));
	comboBoxAMRig->setCurrentIndex(Settings.Get("Input-AM", "rig", int(0)));
	comboBoxFMRig->setCurrentIndex(Settings.Get("Input-FM", "rig", int(0)));
	comboBoxHamRig->setCurrentIndex(Settings.Get("Input-Ham", "rig", int(0)));
#endif

	/* GPS */
	ExtractReceiverCoordinates();

	/* Rig */
#ifdef HAVE_LIBHAMLIB
	rig_model_t current = 0;
	CRig* rig = Receiver.GetCurrentRig();

	if(rig)
		CheckBoxEnableSMeter->setChecked(rig->GetEnableSMeter());

#endif

	loading = false; // loading completed
}

void ReceiverSettingsDlg::hideEvent(QHideEvent*)
{
	// input
	Settings.Put("Input-DRM", "soundcard", comboBoxDRMSound->currentIndex());
	Settings.Put("Input-DRM", "mode", stackedWidgetDRMip->currentIndex());
	Settings.Put("Input-DRM", "channels", bgDRMlrm->checkedId());
	Settings.Put("Input-DRM", "sign", bgDRMiq->checkedId());
	Settings.Put("Input-DRM", "flipspectrum", CheckBoxDRMFlipSpec->isChecked());

	Settings.Put("Input-AM", "soundcard", comboBoxAMSound->currentIndex());
	Settings.Put("Input-AM", "mode", stackedWidgetAMip->currentIndex());
	Settings.Put("Input-AM", "channels", bgAMlrm->checkedId());
	Settings.Put("Input-AM", "sign", bgAMiq->checkedId());
	Settings.Put("Input-AM", "flipspectrum", CheckBoxAMFlipSpec->isChecked());

	Settings.Put("Input-FM", "soundcard", comboBoxFMSound->currentIndex());

	Settings.Put("Input-Ham", "soundcard", comboBoxHamSound->currentIndex());
	Settings.Put("Input-Ham", "mode", stackedWidgetHamip->currentIndex());
	Settings.Put("Input-Ham", "channels", bgHamlrm->checkedId());
	Settings.Put("Input-Ham", "sign", bgHamiq->checkedId());
	Settings.Put("Input-Ham", "flipspectrum", CheckBoxHamFlipSpec->isChecked());

#ifdef HAVE_LIBHAMLIB
	Settings.Put("Input-DRM", "rig", comboBoxDRMRig->currentIndex());
	Settings.Put("Input-AM", "rig", comboBoxAMRig->currentIndex());
	Settings.Put("Input-FM", "rig", comboBoxFMRig->currentIndex());
	Settings.Put("Input-Ham", "rig", comboBoxHamRig->currentIndex());
#endif
	// rig
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

void ReceiverSettingsDlg::OnSelFrequencyInterp(int iId)
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
void 	ReceiverSettingsDlg::OnRadioDRMRealIQ(int i)
{
	stackedWidgetDRMip->setCurrentIndex(i);
}

void 	ReceiverSettingsDlg::OnRadioAMRealIQ(int i)
{
	stackedWidgetAMip->setCurrentIndex(i);
}

void 	ReceiverSettingsDlg::OnRadioHamRealIQ(int i)
{
	stackedWidgetHamip->setCurrentIndex(i);
}

void ReceiverSettingsDlg::OnButtonDRMApply()
{
    Settings.Put("mode", "Input-DRM", bgDRMriq->checkedId());
    Settings.Put("channels", "Input-DRM", bgDRMlrm->checkedId());
    Settings.Put("sign", "Input-DRM", bgDRMriq->checkedId());
    Settings.Put("flipspectrum", "Input-DRM", CheckBoxDRMFlipSpec->isChecked());
    Settings.Put("rig", "Input-DRM", comboBoxDRMRig->currentText().toStdString());
    Settings.Put("rig", "Input-DRM", comboBoxDRMSound->currentText().toStdString());
}

void ReceiverSettingsDlg::OnButtonAMApply()
{
    Settings.Put("mode", "Input-AM", bgAMriq->checkedId());
    Settings.Put("channels", "Input-AM", bgAMlrm->checkedId());
    Settings.Put("sign", "Input-AM", bgAMiq->checkedId());
    Settings.Put("flipspectrum", "Input-AM", CheckBoxAMFlipSpec->isChecked());
    Settings.Put("rig", "Input-AM", comboBoxAMRig->currentText().toStdString());
    Settings.Put("rig", "Input-AM", comboBoxAMSound->currentText().toStdString());
}

void ReceiverSettingsDlg::OnButtonFMApply()
{
    Settings.Put("rig", "Input-FM", comboBoxFMRig->currentText().toStdString());
    Settings.Put("rig", "Input-FM", comboBoxFMSound->currentText().toStdString());
}

void ReceiverSettingsDlg::OnButtonHamApply()
{
    Settings.Put("mode", "Input-Ham", bgHamriq->checkedId());
    Settings.Put("channels", "Input-Ham", bgHamlrm->checkedId());
    Settings.Put("sign", "Input-Ham", bgHamriq->checkedId());
    Settings.Put("flipspectrum", "Input-Ham", CheckBoxHamFlipSpec->isChecked());
    Settings.Put("rig", "Input-Ham", comboBoxHamRig->currentText().toStdString());
    Settings.Put("rig", "Input-Ham", comboBoxHamSound->currentText().toStdString());
    //Receiver.SetChannelSelection(EInChanSel(iId));
    //Receiver.SetFlippedSpectrum(CheckBoxFlipSpec->isChecked());
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
	CRig* rig = Receiver.GetCurrentRig();
	if(rig)
		rig->SetEnableSMeter(on);
}

void
ReceiverSettingsDlg::OnRigTypeSelected(const QModelIndex&)
{
}

void
ReceiverSettingsDlg::OnRigSelected(const QModelIndex& index)
{
#ifdef HAVE_LIBHAMLIB
    QVariant var = index.data(Qt::UserRole);
    rig_caps caps = var.value<rig_caps>();
    if(caps.port_type == RIG_PORT_SERIAL)
    {
	comboBoxComPort->setEnabled(true);
	comboBoxComPort->clear();
	map<string,string> ports;
	GetComPortList(ports);
	for(map<string,string>::const_iterator i=ports.begin();
		i!=ports.end(); i++)
	{
	    comboBoxComPort->addItem(i->first.c_str(), i->second.c_str());
	}
    }
    else if(caps.port_type == RIG_PORT_USB)
    {
	comboBoxComPort->setEnabled(true);
	comboBoxComPort->clear();
    }
    else
    {
	comboBoxComPort->setEnabled(false);
    }
#endif
}

void
ReceiverSettingsDlg::OnButtonAddRig()
{
    rig_model_t model = treeViewRigTypes->currentIndex().data(Qt::UserRole).toInt();
    const rig_caps* caps = Receiver.GetRigCaps(model);
    if(caps)
	Rigs.add(*caps);
}

void
ReceiverSettingsDlg::OnButtonRemoveRig()
{
}

void
ReceiverSettingsDlg::OnButtonConnectRig()
{
    if(loading)
	    return;

#ifdef HAVE_LIBHAMLIB
	iWantedrigModel = treeViewRigs->currentIndex().data(Qt::UserRole).toInt();
	CRig* rig = Receiver.GetRig(iWantedrigModel);
	if(rig==NULL)
		return;
	if(rig->caps && rig->caps->port_type == RIG_PORT_SERIAL)
	{
	    string strPort = "";//comboBoxRigPort->currentText().toStdString();
	    if(strPort!="")
	    {
		rig->setConf("rig_pathname", strPort.c_str());
	    }
	}

	Receiver.SetRigModel(iWantedrigModel);
#endif
	TimerRig.start(500);
}

void ReceiverSettingsDlg::OnTimerRig()
{
#ifdef HAVE_LIBHAMLIB
    if(Receiver.GetRigChangeInProgress())
	    return;
	CRig* rig = Receiver.GetCurrentRig();
	if(rig)
	    labelRigInfo->setText(rig->getInfo());

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
	const QString s = tr("<b>Flip Input Spectrum:</b> Checking this box "
		"will flip or invert the input spectrum. This is necessary if the "
		"mixer in the front-end uses the lower side band.");
	CheckBoxDRMFlipSpec->setWhatsThis(s);
	CheckBoxAMFlipSpec->setWhatsThis(s);
	CheckBoxHamFlipSpec->setWhatsThis(s);

	/* Log File */
	CheckBoxWriteLog->setWhatsThis(
		tr("<b>Log File:</b> Checking this box brings the "
		"Dream software to write a log file about the current reception. "
		"Every minute the average SNR, number of correct decoded FAC and "
		"number of correct decoded MSC blocks are logged including some "
		"additional information, e.g. the station label and bit-rate. The "
		"log mechanism works only for audio services using AAC source coding. "
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
