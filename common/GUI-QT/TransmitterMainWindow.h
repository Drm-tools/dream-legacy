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
#include "ui_TransmitterMainWindow.h"
#include "../util/Utilities.h"
#include <vector>
#include <QDialog>
#include <QTimer>

/* Classes ********************************************************************/
class CDRMTransmitterInterface;
class CSettings;
class QMenuBar;
class Q3PopupMenu;

class TransmitterMainWindow : public QMainWindow, public Ui_TransmitterMainWindow
{
	Q_OBJECT

public:
	TransmitterMainWindow(
		CDRMTransmitterInterface& tx,
		CSettings&,
		QWidget* parent=0, const char* name=0, Qt::WFlags f=0);
	virtual ~TransmitterMainWindow();
	/* dummy assignment operator to help MSVC8 */
	TransmitterMainWindow& operator=(const TransmitterMainWindow&)
	{ throw "should not happen"; return *this;}

protected:

	void				DisableAllControlsForSet();
	void				EnableAllControlsForSet();

	void				UpdateMSCProtLevCombo();
	void				EnableTextMessage(const bool bFlag);
	void				EnableAudio(const bool bFlag);
	void				EnableData(const bool bFlag);
	void				AddWhatsThisHelp();
	void				choseComboBoxItem(QComboBox* box, const QString& text);
	void				GetFromTransmitter();
	void				SetTransmitter();
	void				GetChannel();
	void				SetChannel();
	void				GetStreams();
	void				SetStreams();
	void				GetAudio(int);
	void				SetAudio(int);
	void				GetData(int, int);
	void				SetData(int, int);
	void				GetServices();
	void				SetServices();
	void				GetCOFDM();
	void				SetCOFDM();
	void				GetMDIIn();
	void				SetMDIIn();
	void				SetMDIOut();
	void				GetMDIOut();
	void				UpdateCapacities();
    void                AddSlide(const QString& path);

	virtual void		closeEvent(QCloseEvent* ce);

	QMenuBar*			pMenu;
	Q3PopupMenu*		pSettingsMenu;
	QTimer				Timer;

	CDRMTransmitterInterface&	DRMTransmitter;
	CSettings&			Settings;
	bool			bIsStarted;
	vector<CIpIf>vecIpIf;


public slots:
	void OnButtonStartStop();
	void OnButtonClose();
	void OnTimer();
	void OnHelpWhatsThis();

	/* Channel */
	void OnComboBoxSDCConstellationActivated(int iID);
	void OnComboBoxMSCInterleaverActivated(int iID);
	void OnComboBoxMSCConstellationActivated(int iID);
	void OnComboBoxMSCProtLevActivated(int iID);
	void OnRadioBandwidth(int iID);
	void OnRadioRobustnessMode(int iID);
	void OnRadioMode(int iID);

	/* Audio */
	void OnComboBoxAudioSourceActivated(int iID);
	void OnToggleCheckBoxEnableTextMessage(bool bState);
	void OnPushButtonAddText();
	void OnPushButtonDeleteText();
	void OnButtonClearAllText();
	void OnButtonAudioSourceFileBrowse();
	void OnLineEditAudioSourceFileChanged(const QString& str);
	void OnToggleCheckBoxAudioSourceIsFile(bool bState);

	/* Data */
	void OnPushButtonAddFileName();
	void OnButtonClearAllFileNames();

	/* streams */
	void OnButtonAddStream();
	void OnButtonDeleteStream();
	void OnComboBoxStreamTypeActivated(int item);
	void OnComboBoxPacketsPerFrameActivated(const QString& str);
	void OnLineEditPacketLenChanged(const QString& str);
	void OnStreamsListItemClicked(Q3ListViewItem* item);

	/* services */
	void OnTextChangedServiceLabel(const QString& strLabel);
	void OnTextChangedServiceID(const QString& strID);
	void OnButtonAddService();
	void OnButtonDeleteService();
	void OnServicesListItemClicked(Q3ListViewItem* item);

    /* MDI Input */
	void OnLineEditMDIinPortChanged(const QString& str);
	void OnToggleCheckBoxMDIinMcast(bool bState);
	void OnLineEditMDIinGroupChanged(const QString& str);
	void OnComboBoxMDIinInterfaceActivated(int iID);
	void OnLineEditMDIInputFileChanged(const QString& str);
	void OnButtonMDIInBrowse();
	void OnToggleCheckBoxReadMDIFile(bool bState);

    /* MDI Output */
	void OnButtonAddMDIDest();
	void OnButtonAddMDIFileDest();
	void OnButtonDeleteMDIOutput();
	void OnButtonMDIOutBrowse();
	void OnComboBoxMDIoutInterfaceActivated(int iID);
	void OnLineEditMDIOutAddrChanged(const QString& str);
	void OnLineEditMDIOutputFileChanged(const QString& str);
	void OnLineEditMDIoutPortChanged(const QString& str);
	void OnMDIOutListItemClicked(Q3ListViewItem* item);

    /* COFDM */
	void OnComboBoxCOFDMDestActivated(int iID);
	void OnTextChangedSndCrdIF(const QString& strIF);
	void OnButtonCOFDMAddAudio();
	void OnComboBoxCOFDMdestActivated(int iID);
	void OnLineEditCOFDMOutputFileChanged(const QString& str);
	void OnButtonCOFDMAddFile();
	void OnButtonCOFDMDeleteSelected();
	void OnButtonCOFDMBrowse();
	void OnCOFDMOutListItemClicked(Q3ListViewItem* item);
};
