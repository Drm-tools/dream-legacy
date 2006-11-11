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


#include <qpushbutton.h>
#include <qstring.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qtabwidget.h>
#include <qcombobox.h>
#include <qstring.h>
#include <qbuttongroup.h>
#include <qmultilineedit.h>
#include <qlistview.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qstringlist.h>
#include <qmenubar.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qthread.h>
#include <qtimer.h>
#include <qwt_thermo.h>
#include <qwhatsthis.h>
#include <qprogressbar.h>

#ifdef _WIN32
# include "../../Windows/moc/TransmDlgbase.h"
#else
# include "moc/TransmDlgbase.h"
#endif
#include "DialogUtil.h"
#include "../DrmTransmitter.h"
#include "../Parameter.h"


/* Classes ********************************************************************/

class TransmDialog : public TransmDlgBase
{
	Q_OBJECT

public:
	TransmDialog(QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
		WFlags f = 0);
	virtual ~TransmDialog();
 	CDRMTransmitter* GetTx() { return &DRMTransmitter; }

protected:
	void DisableAllControlsForSet();
	void EnableAllControlsForSet();

	QMenuBar*			pMenu;
	QPopupMenu*			pSettingsMenu;
	QTimer				Timer;

	CDRMTransmitter		DRMTransmitter;
	_BOOLEAN			bIsStarted;
	CVector<string>		vecstrTextMessage;
	int					iIDCurrentText;
	_BOOLEAN			GetMessageText(const int iID);
	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const _BOOLEAN bFlag);
	void				EnableAudio(const _BOOLEAN bFlag);
	void				EnableData(const _BOOLEAN bFlag);
	void				AddWhatsThisHelp();


public slots:
	void OnButtonStartStop();
	void OnPushButtonAddText();
	void OnButtonClearAllText();
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
	void OnPushButtonChooseOutputFileName();
	void OnToggleCheckBoxEnableCOFDM(bool bState);
	void OnToggleCheckBoxEnableData(bool bState);
	void OnToggleCheckBoxEnableAudio(bool bState);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnComboBoxAudioSourceHighlighted(int iID);
	void OnComboBoxCOFDMDestHighlighted(int iID);
	void OnComboBoxMSCInterleaverHighlighted(int iID);
	void OnComboBoxMSCConstellationHighlighted(int iID);
	void OnComboBoxSDCConstellationHighlighted(int iID);
	void OnComboBoxLanguageHighlighted(int iID);
	void OnComboBoxProgramTypeHighlighted(int iID);
	void OnComboBoxTextMessageHighlighted(int iID);
	void OnComboBoxMSCProtLevHighlighted(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioOutput(int iID);
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnTextChangedOutputFileName(const QString& strFile);

	void OnToggleCheckBoxMDIoutEnable(bool bState);
	void OnTextChangedMDIoutPort(const QString& strLabel);
	void OnTextChangedMDIoutDest(const QString& strLabel);
	void OnComboBoxMDIoutInterfaceHighlighted(int iID);
	void OnToggleCheckBoxMDIinEnable(bool bState);
	void OnTextChangedMDIinPort(const QString& strLabel);
	void OnToggleCheckBoxMDIinMcast(bool bState);
	void OnComboBoxMDIinInterfaceHighlighted(int iID);

	void OnTimer();
	void OnHelpWhatsThis() {QWhatsThis::enterWhatsThisMode();}
};
