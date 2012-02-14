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

#include "RigDlg.h"
#include "Rig.h"
#include "../GlobalDefinitions.h"
#include "../util/Utilities.h"
#include <QMessageBox>
#include <QButtonGroup>
#include <QFileDialog>
#include <algorithm>

/* Implementation *************************************************************/

RigTypesModel::RigTypesModel():QAbstractItemModel(),rigs()
{
}

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
            if( (index.column()==0) && (int(rigs.size())>index.row()) )
                return rigs[index.row()].name.c_str();
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
    case 0:
        return tr("Rig Make/Model");
        break;
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

int RigTypesModel::rig_enumerator(const rig_caps *caps, void *data)
{
    //CRigMap& map = *(CRigMap *)data;
    //map.rigs[caps->mfg_name][caps->model_name] = caps->rig_model;
    return 1;	/* !=0, we want them all! */
}

void
RigTypesModel::load()
{
}


RigModel::RigModel(CSettings& ns) : QAbstractItemModel(), settings(ns),
    rigs(),unmodified(),modified()
{
    unmodified[RIG_MODEL_G303].levels["ATT"]=0;
    unmodified[RIG_MODEL_G303].levels["AGC"]=3;

    modified[RIG_MODEL_G303].levels["ATT"]=0;
    modified[RIG_MODEL_G303].levels["AGC"]=3;

    unmodified[RIG_MODEL_G313].levels["ATT"]=0;
    unmodified[RIG_MODEL_G313].levels["AGC"]=3;

    modified[RIG_MODEL_G313].levels["ATT"]=0;
    modified[RIG_MODEL_G313].levels["AGC"]=3;

    unmodified[RIG_MODEL_AR7030].mode_for_drm = RIG_MODE_AM;
    unmodified[RIG_MODEL_AR7030].width_for_drm = 3000;
    unmodified[RIG_MODEL_AR7030].levels["AGC"]=5;

    modified[RIG_MODEL_AR7030].mode_for_drm = RIG_MODE_AM;
    modified[RIG_MODEL_AR7030].width_for_drm = 6000;
    modified[RIG_MODEL_AR7030].levels["AGC"]=5;

    unmodified[RIG_MODEL_NRD535].mode_for_drm = RIG_MODE_CW;
    unmodified[RIG_MODEL_NRD535].width_for_drm = 12000;
    unmodified[RIG_MODEL_NRD535].levels["CWPITCH"]=-5000;
    unmodified[RIG_MODEL_NRD535].levels["IF"]=-2000;
    unmodified[RIG_MODEL_NRD535].levels["AGC"]=3;
    unmodified[RIG_MODEL_NRD535].offset=3;

    modified[RIG_MODEL_NRD535].levels["AGC"]=3;
    modified[RIG_MODEL_NRD535].offset=3;

    unmodified[RIG_MODEL_RX320].mode_for_drm = RIG_MODE_AM;
    unmodified[RIG_MODEL_RX320].width_for_drm = 6000;
    unmodified[RIG_MODEL_RX320].levels["AF"]=1;
    unmodified[RIG_MODEL_RX320].levels["AGC"]=3;

    modified[RIG_MODEL_RX320].levels["AGC"]=3;

    unmodified[RIG_MODEL_RX340].mode_for_drm = RIG_MODE_USB;
    unmodified[RIG_MODEL_RX340].width_for_drm = 16000;
    unmodified[RIG_MODEL_RX340].levels["AF"]=1;
    unmodified[RIG_MODEL_RX340].levels["IF"]=2000;
    unmodified[RIG_MODEL_RX340].levels["AGC"]=3;
    unmodified[RIG_MODEL_RX340].offset=-12;

    modified[RIG_MODEL_RX340].levels["AGC"]=3;
    modified[RIG_MODEL_RX340].offset=-12;

    string s = settings.Get("Receiver", "rigs", string(""));
    if(s!="")
    {
        stringstream ss(s);
        while(ss)
        {
            int i;
            ss >> i;
            rigs.insert(i);
        }
    }
}

QModelIndex RigModel::index(int row, int column,
                            const QModelIndex &parent) const
{
    if(parent.isValid())
    {
    }
    else
    {
        int n = 0;
        for(set<int>::const_iterator i=rigs.begin(); i!=rigs.end(); i++)
        {
            if(n==row)
                return createIndex(row, column, *i);
            n++;
        }
    }
    return QModelIndex();
}

QModelIndex RigModel::parent(const QModelIndex &child) const
{
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
    int id = int(index.internalId());
    if(rigs.count(id)==0)
        return QVariant();

    if(true) // no structure visible at the moment
    {
        stringstream sec;
        sec << "Rig-" << id;
        string name;
        switch(role)
        {
        case Qt::DecorationRole:
            break;
        case Qt::DisplayRole:
            switch(index.column())
            {
            case 0:
                name = settings.Get(sec.str(), "model_name", string(""));
                if(name != "")
                    return name.c_str();
                break;
            case 1:
            {
                int model = settings.Get(sec.str(), "model", -1);
                if(model != -1)
                    return model;
            }
            break;

            case 2:
                name = settings.Get(sec.str(), "status", string(""));
                if(name != "")
                    return name.c_str();
                break;
            }
            break;
        case Qt::UserRole:
        {
            QVariant var;
            var.setValue(id);
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
    case 0:
        return tr("Rig");
        break;
    case 1:
        return tr("ID");
        break;
    case 2:
        return tr("Status");
        break;
    }
    return "";
}

void
RigModel::add(rig_model_t m, bool modified_for_drm)
{
}

void
RigModel::remove(int id)
{
    for(set<int>::iterator i=rigs.begin(); i!=rigs.end(); i++)
    {
        if((*i) == id)
        {
            rigs.erase(i); // rig is still stored in receiver TODO
            break;
        }
    }
    reset();
}

RigDlg::RigDlg(
    CSettings& NSettings,
    QWidget* parent, Qt::WFlags f) :
    QDialog(parent, f), Ui_RigDlg(),
    Settings(NSettings), loading(true),
    TimerRig(),iWantedrigModel(0),
    RigTypes(),Rigs(NSettings)
{
    setupUi(this);

    treeViewRigTypes->setModel(&RigTypes);
    RigTypes.load();
    treeViewRigs->setModel(&Rigs);

    /* Connections */

    connect(pushButtonAddRig, SIGNAL(clicked()), this, SLOT(OnButtonAddRig()));
    connect(pushButtonConfigureRig, SIGNAL(clicked()), this, SLOT(OnButtonConfigureRig()));

    connect(pushButtonRemoveRig, SIGNAL(clicked()), this, SLOT(OnButtonRemoveRig()));

    connect(treeViewRigTypes, SIGNAL(clicked (const QModelIndex&)),
            this, SLOT(OnRigTypeSelected(const QModelIndex&)));
    connect(treeViewRigs, SIGNAL(clicked (const QModelIndex&)),
            this, SLOT(OnRigSelected(const QModelIndex&)));

    checkBoxModified->setEnabled(false);

}

RigDlg::~RigDlg()
{
}

void RigDlg::showEvent(QShowEvent*)
{
    loading = true; // prevent executive actions during reading state

    pushButtonConfigureRig->setEnabled(false);

}

void RigDlg::hideEvent(QHideEvent*)
{
}

void
RigDlg::OnRigTypeSelected(const QModelIndex& m)
{
    QVariant var = m.data(Qt::UserRole);
    if(var.isValid()==false)
        return;
    rig_model_t model = var.toInt();
}

void
RigDlg::OnRigSelected(const QModelIndex& index)
{
    QVariant var = index.data(Qt::UserRole);
    //int r = var.value();
    if(false)
    {
        QMessageBox::information(this, tr("Hamlib"), tr("Rig not created error"), QMessageBox::Ok);
        return;
    }
    pushButtonConfigureRig->setEnabled(true);
}

void
RigDlg::OnButtonAddRig()
{
    rig_model_t model = treeViewRigTypes->currentIndex().data(Qt::UserRole).toInt();

    bool parms=false;
    if(parms)
    {
        //const RigTypesModel::parms& p = pp->second;
        /*
        for(map<string,int>::const_iterator i=p.levels.begin(); i!=p.levels.end(); i++)
        {
            r->setLevel(rig_parse_level(i->first.c_str()), i->second);
        }
        if(p.mode_for_drm!=RIG_MODE_NONE)
        {
            r->SetModeForDRM(p.mode_for_drm, p.width_for_drm);
        }
        r->SetFrequencyOffset(p.offset);
        */
        // TODO set these in settings
    }
    //else
    //r->SetFrequencyOffset(0);
    Rigs.add(model);
    //treeViewRigs->setCurrentIndex(Rigs.rowCount()-1);
}

void
RigDlg::OnButtonRemoveRig()
{
    Rigs.remove(treeViewRigs->currentIndex().internalId());
}

void RigDlg::OnButtonTestRig()
{
}

void RigDlg::OnTimerRig()
{
}

void RigDlg::OnCheckEnableSMeterToggled(bool)
{
}
