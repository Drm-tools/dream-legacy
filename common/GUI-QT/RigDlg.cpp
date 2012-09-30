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
#include <QTreeWidgetItem>

/* Implementation *************************************************************/

RigDlg::RigDlg(
    CSettings& NSettings,
    CRig& nrig,
    QWidget* parent, Qt::WFlags f) :
    QDialog(parent, f), Ui_RigDlg(),
    Settings(NSettings),
    TimerRig(),rig(nrig),rigmap()
{

    map<rig_model_t,CHamlib::SDrRigCaps> r;

    setupUi(this);
    rig.GetRigList(r);
    modified->setEnabled(false);
    //rigTypes->setColumnCount(2);
    rigTypes->setSortingEnabled(false);
    for(map<rig_model_t,CHamlib::SDrRigCaps>::const_iterator i=r.begin(); i!=r.end(); i++)
    {
	rig_model_t model_num = i->first;
        CHamlib::SDrRigCaps rc =  i->second;
        QTreeWidgetItem* mfr, *model;
        if(rc.strManufacturer=="" || rc.strModelName=="")
        {
            continue;
        }
        QList<QTreeWidgetItem *> l = rigTypes->findItems(rc.strManufacturer.c_str(), Qt::MatchFixedString);
        if(l.size()==0)
        {
            mfr = new QTreeWidgetItem(rigTypes);
            mfr->setText(0,rc.strManufacturer.c_str());
        }
        else
        {
            mfr = l.first();
        }
        model = new QTreeWidgetItem(mfr);
        model->setText(0,rc.strModelName.c_str());
	model->setData(0, Qt::UserRole, model_num);
	rigmap[model_num] = rc.strModelName;

    }
    rigTypes->setSortingEnabled(false);
    rigTypes->sortItems(9, Qt::AscendingOrder);

}

RigDlg::~RigDlg()
{
}

void RigDlg::showEvent(QShowEvent*)
{
    map<string,string> ports;
    rig.GetPortList(ports);
    comboBoxPort->clear();
    for(map<string,string>::const_iterator i=ports.begin(); i!=ports.end(); i++)
    {
	comboBoxPort->addItem(i->first.c_str(), i->second.c_str());	
    }

    prev_rig_model = rig.GetHamlibModelID();
    map<rig_model_t,string>::const_iterator m = rigmap.find(prev_rig_model);
    if(m!=rigmap.end())
    {
	QList<QTreeWidgetItem *> l = rigTypes->findItems(m->second.c_str(), Qt::MatchExactly);
	if(l.size()>0)
		rigTypes->setCurrentItem(l.front());
    }

    prev_port = rig.GetComPort();
    comboBoxPort->setCurrentIndex(comboBoxPort->findText(prev_port.c_str()));
}

void RigDlg::hideEvent(QHideEvent*)
{
}

void
RigDlg::on_rigTypes_itemSelectionChanged()
{
}

void
RigDlg::on_modified_stateChanged(int state)
{
    (void)state;
}

void
RigDlg::on_enableSMeter_stateChanged(int state)
{
    (void)state;
}

void
RigDlg::on_testRig_clicked()
{
	rig.SetComPort(comboBoxPort->itemData(comboBoxPort->currentIndex()).toString().toStdString());
	rig.SetHamlibModelID(rigTypes->currentItem()->data(0, Qt::UserRole).toInt());
	rig.subscribe();
}

void
RigDlg::on_buttonBox_accepted()
{
	rig.SetComPort(comboBoxPort->itemData(comboBoxPort->currentIndex()).toString().toStdString());
	rig.SetHamlibModelID(rigTypes->currentItem()->data(0, Qt::UserRole).toInt());
	rig.unsubscribe();
	close();
}

void
RigDlg::on_buttonBox_rejected()
{
	rig.SetComPort(prev_port);
	rig.SetHamlibModelID(prev_rig_model);
	rig.unsubscribe();
	close();
}

void
RigDlg::on_comboBoxPort_currentIndexChanged(int index)
{
    (void)index;
}

void RigDlg::OnTimerRig()
{
}

void
RigDlg::on_rig_sigstr(double r)
{
	sMeter->setValue(int(100*r));
}
