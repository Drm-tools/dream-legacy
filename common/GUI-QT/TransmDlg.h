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

#include "../GlobalDefinitions.h"
#include "TransmDlgbase.h"
#include <vector>
#include <qtimer.h>

/* Classes ********************************************************************/
class CDRMTransmitter;
class CSettings;
class QMenuBar;
class QPopupMenu;

class TransmDialog : public TransmDlgBase
{
	Q_OBJECT

public:
	TransmDialog(
		CDRMTransmitter& tx,
		CSettings&,
		QWidget* parent=0, const char* name=0, bool modal=FALSE, WFlags f=0);
	virtual ~TransmDialog();
	/* dummy assignment operator to help MSVC8 */
	TransmDialog& operator=(const TransmDialog&)
	{ throw "should not happen"; return *this;}

protected:
	struct ipIf			{string name; uint32_t addr;};

	void				DisableAllControlsForSet();
	void				EnableAllControlsForSet();

	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const _BOOLEAN bFlag);
	void				EnableAudio(const _BOOLEAN bFlag);
	void				EnableData(const _BOOLEAN bFlag);
	void				AddWhatsThisHelp();
	void				GetNetworkInterfaces();
	void				choseComboBoxItem(QComboBox* box, const QString& text);
	void				GetFromTransmitter();
	void				SetTransmitter();
	void				GetChannel();
	void				SetChannel();
	void				GetStreams();
	void				SetStreams();
	void				GetAudio();
	void				SetAudio();
	void				GetData();
	void				SetData();
	void				GetServices();
	void				SetServices();
	void				GetCOFDM();
	void				SetCOFDM();
	void				GetMDIIn();
	void				SetMDIIn();
	void				SetMDIOut();
	void				GetMDIOut();

	virtual void		closeEvent(QCloseEvent* ce);

	QMenuBar*			pMenu;
	QPopupMenu*			pSettingsMenu;
	QTimer				Timer;

	CDRMTransmitter&	DRMTransmitter;
	CSettings&			Settings;
	_BOOLEAN			bIsStarted;
	vector<string>		vecstrTextMessage;
	size_t				iIDCurrentText;
	vector<ipIf>vecIpIf;


public slots:
	void OnButtonStartStop();
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnPushButtonAddText();
	void OnButtonClearAllText();
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();
	void OnComboBoxTextMessageHighlighted(int iID);
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnComboBoxCOFDMDestHighlighted(int iID);
	void OnComboBoxAudioSourceHighlighted(int iID);
	void OnRadioMode(int iID);

	/* streams */
	void OnButtonAddStream();
	void OnButtonDeleteStream();
	void OnComboBoxStreamTypeHighlighted(int item);
	void OnComboBoxPacketsPerFrameHighlighted(const QString& str);
	void OnLineEditPacketLenChanged(const QString& str);
	void OnStreamsListItemClicked(QListViewItem* item);

	/* services */
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnButtonAddAudioService();
	void OnButtonAddDataService();
	void OnButtonDeleteService();
	void OnServicesListItemClicked(QListViewItem* item);

	void OnTimer();
	void OnHelpWhatsThis();
};
