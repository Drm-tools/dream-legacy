/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
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

#include <qtextbrowser.h>
#include <qmime.h>
#include <qimage.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qfiledialog.h>


#ifdef _WIN32
# include "../../Windows/moc/MultimediaDlgbase.h"
#else
# include "moc/MultimediaDlgbase.h"
#endif

#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../datadecoding/DABData.h"
#include "MultColorLED.h"

extern CDRMReceiver	DRMReceiver;


/* Classes ********************************************************************/
class MultimediaDlg : public MultimediaDlgBase
{
	Q_OBJECT

public:
	MultimediaDlg(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		WFlags f = 0);

	void SetStatus(int MessID, int iMessPara);

protected:
	QTimer					Timer;
	QMenuBar*				pMenu;
	QPopupMenu*				pFileMenu;
	int						iCurTransportID;
    virtual void			showEvent(QShowEvent* pEvent);
	virtual void			hideEvent(QHideEvent* pEvent);
	CVector<CMOTPicture*>	vecpRawImages;
	int						iCurImagePos;

	void SetPicture();
	void UpdateAccButtons();
	int GetIDLastPicture() {return vecpRawImages.Size() - 1;}

public slots:
	void OnTimer();
	void OnButtonStepBack();
	void OnButtonStepForw();
	void OnButtonJumpBegin();
	void OnButtonJumpEnd();
	void OnSave();
};

