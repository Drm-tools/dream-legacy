/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: MOT Broadcast Website Viewer
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

#include "BWSViewer.h"
#include "../util/Settings.h"
#include "../DrmReceiver.h"

BWSViewer::BWSViewer(CDRMReceiver& rec, CSettings& s,
        QWidget* parent,
		const char* name, Qt::WFlags f):
		QMainWindow(parent, name, f), Ui_BWSViewer(), Timer(), receiver(rec), settings(s)
{
    setupUi(this);

    connect(buttonOk, SIGNAL(clicked()), this, SLOT(close()));

	connect(actionClear_All, SIGNAL(triggered()), SLOT(OnClearAll()));
	connect(actionSave, SIGNAL(triggered()), SLOT(OnSave()));
	connect(actionSave_All, SIGNAL(triggered()), SLOT(OnSaveAll()));
	connect(actionClose, SIGNAL(triggered()), SLOT(close()));

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Connect controls */
	connect(ButtonStepBack, SIGNAL(clicked()), this, SLOT(OnButtonStepBack()));

	connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

	/* Add the service description into the dialog caption */
	QString strTitle = tr("MOT Slide Show");

    CParameter& Parameters = *receiver.GetParameters();
    Parameters.Lock();
    const int iCurSelAudioServ = Parameters.GetCurSelAudioService();
    const uint32_t iAudioServiceID = Parameters.Service[iCurSelAudioServ].iServiceID;

    /* Get current data service */
    const int iCurSelDataServ = Parameters.GetCurSelDataService();
    CService service = Parameters.Service[iCurSelDataServ];
    Parameters.Unlock();

    if (service.IsActive())
    {
        /* Do UTF-8 to QString (UNICODE) conversion with the label strings */
        QString strLabel = QString().fromUtf8(service.strLabel.c_str()).stripWhiteSpace();


        /* Service ID (plot number in hexadecimal format) */
        QString strServiceID = "";

        /* show the ID only if differ from the audio service */
        if ((service.iServiceID != 0) && (service.iServiceID != iAudioServiceID))
        {
            if (strLabel != "")
                strLabel += " ";

            strServiceID = "- ID:" +
                QString().setNum(long(service.iServiceID), 16).upper();
        }

        /* add the description on the title of the dialog */
        if (strLabel != "" || strServiceID != "")
            strTitle += " [" + strLabel + strServiceID + "]";
    }
	setCaption(strTitle);

	/* Get window geometry data and apply it */
	CWinGeom g;
	settings.Get("BWS", g);
	const QRect WinGeom(g.iXPos, g.iYPos, g.iWSize, g.iHSize);

	if (WinGeom.isValid() && !WinGeom.isEmpty() && !WinGeom.isNull())
		setGeometry(WinGeom);

	strCurrentSavePath = settings.Get("BWS", "storagepath", strCurrentSavePath);

	Timer.stop();
}

BWSViewer::~BWSViewer()
{
}

void BWSViewer::OnTimer()
{
    CParameter& Parameters = *receiver.GetParameters();
	Parameters.Lock();
	ETypeRxStatus status = Parameters.ReceiveStatus.MOT.GetStatus();
	Parameters.Unlock();
	switch(status)
	{
	case NOT_PRESENT:
		LEDStatus->Reset(); /* GREY */
		break;

	case CRC_ERROR:
		LEDStatus->SetLight(2); /* RED */
		break;

	case DATA_ERROR:
		LEDStatus->SetLight(1); /* YELLOW */
		break;

	case RX_OK:
		LEDStatus->SetLight(0); /* GREEN */
		break;
	}
}

void BWSViewer::OnButtonStepBack()
{
}
void BWSViewer::OnButtonStepForward()
{
}
void BWSViewer::OnButtonJumpBegin()
{
}
void BWSViewer::OnButtonJumpEnd()
{
}

void BWSViewer::OnSave()
{
}

void BWSViewer::OnSaveAll()
{
}

void BWSViewer::OnClearAll()
{
}

void BWSViewer::showEvent(QShowEvent*)
{
	/* Update window */
	OnTimer();

	/* Activate real-time timer when window is shown */
	Timer.start(GUI_CONTROL_UPDATE_TIME);
}

void BWSViewer::hideEvent(QHideEvent*)
{
	/* Deactivate real-time timer so that it does not get new pictures */
	Timer.stop();

	/* Save window geometry data */
	QRect WinGeom = geometry();

	CWinGeom c;
	c.iXPos = WinGeom.x();
	c.iYPos = WinGeom.y();
	c.iHSize = WinGeom.height();
	c.iWSize = WinGeom.width();
	settings.Put("BWS", c);

	/* Store save path */
	settings.Put("BWS ","storagepath", strCurrentSavePath);
}


