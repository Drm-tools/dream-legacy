/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2006
 *
 * Author(s):
 *	Andrea Russo
 *
 * Description:
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

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qvalidator.h>
#include <qmessagebox.h>

#include "../DrmReceiver.h"
#include "../util/Settings.h"

#ifdef _WIN32
# include "../../Windows/moc/GeneralSettingsDlgbase.h"
#else
# include "moc/GeneralSettingsDlgbase.h"
#endif

/* Definitions ****************************************************************/

/* Classes ********************************************************************/
class GeneralSettingsDlg : public CGeneralSettingsDlgBase
{
	Q_OBJECT

public:

	GeneralSettingsDlg(CDRMReceiver* pNDRMR, QWidget* parent = 0,
		const char* name = 0, bool modal = FALSE, WFlags f = 0);
	virtual ~GeneralSettingsDlg();

protected:
	virtual void	showEvent(QShowEvent* pEvent);
	virtual void	hideEvent(QHideEvent* pEvent);

	_BOOLEAN 	ValidInput(const QLineEdit* le);
	QString 	ExtractDigits(const QString strS, const int iStart, const int iDigits);
	void		ExtractReceiverCoordinates();

	void			AddWhatsThisHelp();

	CDRMReceiver*	pDRMRec;

public slots:
	void CheckSN(const QString& NewText);
	void CheckEW(const QString& NewText);
	void ButtonOkClicked();
};
