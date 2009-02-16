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

#ifndef _MULTIMEDIADLG_H
#define _MULTIMEDIADLG_H

#include <q3textbrowser.h>
#include <qmime.h>
#include <qimage.h>
#include <qtimer.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qmenubar.h>
#include <q3popupmenu.h>
#include <qlayout.h>
#include <q3filedialog.h>
#include <qdatetime.h>
#include <qregexp.h>
#include <qtooltip.h>
#include <q3textstream.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qmessagebox.h>
#include <qfontdialog.h>
#include <qfont.h>
#include <q3stylesheet.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QHideEvent>

#include "MultimediaDlgbase.h"
#include "MultColorLED.h"
#include "DialogUtil.h"
#include "../GlobalDefinitions.h"
#include "../DrmReceiver.h"
#include "../datadecoding/DABMOT.h"
#include "../util/Settings.h"

/* Definitions ****************************************************************/
/* Maximum number of levels. A maximum of 20 hierarchy levels is set
   (including the Main Menu and the final Message Object) */
#define MAX_NUM_LEV_JOURNALINE			20

/* Classes ********************************************************************/
class CNewIDHistory
{
public:
	CNewIDHistory() : veciNewsID(MAX_NUM_LEV_JOURNALINE), iNumHist(0) {}
	virtual ~CNewIDHistory() {}

	void Add(const int iNewID)
	{
		veciNewsID[iNumHist] = iNewID;
		iNumHist++;
	}

	int Back()
	{
		if (iNumHist > 0)
		{
			iNumHist--;
			return veciNewsID[iNumHist];
		}
		else
			return 0; /* Root ID */
	}

	void Reset() {iNumHist = 0;}

protected:
	CVector<int>	veciNewsID;
	int				iNumHist;
};

class MultimediaDlg : public MultimediaDlgBase
{
	Q_OBJECT

public:
	MultimediaDlg(CDRMReceiver&, QWidget* parent = 0,
		const char* name = 0, bool modal = false, Qt::WFlags f = 0);
	virtual ~MultimediaDlg();
	/* dummy assignment operator to help MSVC8 */
	MultimediaDlg& operator=(const MultimediaDlg&)
	{ throw "should not happen"; return *this;}

	void LoadSettings(const CSettings&);
	void SaveSettings(CSettings&);

protected:

	CParameter&				Parameters;
	CDataDecoder&			DataDecoder;

	QTimer					Timer;
	QMenuBar*				pMenu;
	Q3PopupMenu*				pFileMenu;
	virtual void			showEvent(QShowEvent* pEvent);
	virtual void			hideEvent(QHideEvent* pEvent);
	CVector<CMOTObject>		vecRawImages;
	int						iCurImagePos;
	QString					strFhGIISText;
	QString					strJournalineHeadText;
	int						iCurJourObjID;
	CDataDecoder::EAppType	eAppType;
	CNewIDHistory			NewIDHistory;
	QString					strCurrentSavePath;
	QString					strDirMOTCache;
	QString					strBWSHomePage;
	QFont					fontTextBrowser;
	QFont					fontDefault;
	bool				bAddRefresh;
	int						iRefresh;

	void SetSlideShowPicture();
	void SetJournalineText();
	void UpdateAccButtonsSlideShow();
	int GetIDLastPicture() {return vecRawImages.Size() - 1;}
	void SaveMOTObject(const CVector<_BYTE>& vecbRawData, const QString& strFileName);
	void CreateDirectories(const QString& filename);
	void ClearAllSlideShow();

	void InitApplication(CDataDecoder::EAppType eNewAppType);

	void InitNotSupported();
	void InitMOTSlideShow();
	void InitJournaline();
	void InitBroadcastWebSite();

	void JpgToPng(CMOTObject& NewPic);

	void ExtractJournalineBody(const int iCurJourID, const bool bHTMLExport,
		QString &strTitle, QString &strItems);

	void SetCurrentSavePath(const QString strFileName);
	void AddRefreshHeader(const QString& strFileName);

	bool openBrowser(QWidget *widget, const QString &filename);

public slots:
	void OnTimer();
	void OnButtonStepBack();
	void OnButtonStepForw();
	void OnButtonJumpBegin();
	void OnButtonJumpEnd();
	void OnSave();
	void OnSaveAll();
	void OnClearAll() {ClearAllSlideShow();}
	void OnTextChanged();
	void OnSetFont();
};

#endif
