/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
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

#include "RxApp.h"
#include "DRMMainWindow.h"
#include "AnalogDemDlg.h"
#include <iostream>
using namespace std;

void RxApp::doNewMainWindow()
{
        if(qApp->closingDown())
            return;

        QMainWindow* MainDlg;

        switch(Receiver.GetReceiverMode())
        {
        case NONE:
            // wait for receiver to get into a mode
            QTimer::singleShot(1000, this, SLOT(doNewMainWindow()));
            return;
            break;
        case DRM:
            MainDlg = new DRMMainWindow(Receiver, Settings, 0, 0, Qt::WStyle_MinMax);
            break;
        default:
            MainDlg = new AnalogDemDlg(Receiver, Settings, 0, 0, Qt::WStyle_MinMax);
        }

        MainDlg->setAttribute(Qt::WA_QuitOnClose, false);
        MainDlg->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(MainDlg, SIGNAL(destroyed()), this, SLOT(doNewMainWindow()));
        MainDlg->show();
}
