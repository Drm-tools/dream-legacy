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

#include "MultSettingsDlg.h"

/* Implementation *************************************************************/

MultSettingsDlg::MultSettingsDlg(CDRMReceiver* pNDRMR, QWidget* parent,
	const char* name, bool modal, WFlags f) :
	CMultSettingsDlgBase(parent, name, modal, f), pDRMRec(pNDRMR)
{
	/* Set help text for the controls */
	AddWhatsThisHelp();

	/* Connect buttons */
	connect(buttonClearCacheMOT, SIGNAL(clicked()),
		this, SLOT(OnbuttonClearCacheMOT()));

	connect(buttonClearCacheEPG, SIGNAL(clicked()),
		this, SLOT(OnbuttonClearCacheEPG()));

	EdtSecRefresh->setValidator(new QIntValidator(MIN_MOT_BWS_REFRESH_TIME, MAX_MOT_BWS_REFRESH_TIME, EdtSecRefresh));
}

MultSettingsDlg::~MultSettingsDlg()
{
}

void MultSettingsDlg::hideEvent(QHideEvent*)
{
	/* save current settings */
	if (CheckBoxAddRefresh->isChecked())
		pDRMRec->bAddRefreshHeader = TRUE;
	else
		pDRMRec->bAddRefreshHeader = FALSE;

	QString strRefresh = EdtSecRefresh->text();
	int iMOTRefresh = strRefresh.toUInt();

	if (iMOTRefresh < MIN_MOT_BWS_REFRESH_TIME)
		iMOTRefresh = MIN_MOT_BWS_REFRESH_TIME;

	if (iMOTRefresh > MAX_MOT_BWS_REFRESH_TIME)
		iMOTRefresh = MAX_MOT_BWS_REFRESH_TIME;

	pDRMRec->iMOTBWSRefreshTime = iMOTRefresh;
}

void MultSettingsDlg::showEvent(QShowEvent*)
{
	if (pDRMRec->bAddRefreshHeader == TRUE)
		CheckBoxAddRefresh->setChecked(TRUE);

	EdtSecRefresh->setText(QString().setNum(pDRMRec->iMOTBWSRefreshTime));
}

void MultSettingsDlg::ClearCache(QString sPath, QString sFilter = "", _BOOLEAN bDeleteDirs = FALSE)
{
	/* Delete files into sPath directory with scan recursive */

	QDir dir(sPath);

	/* Check if the directory exists */
	if (dir.exists())
	{
		dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoSymLinks);

		/* Eventually apply the filter */
		if (sFilter != "")
			dir.setNameFilter(sFilter);

		dir.setSorting( QDir::DirsFirst );

		const QFileInfoList *list = dir.entryInfoList();
		QFileInfoListIterator it( *list ); /* create list iterator */

		for(QFileInfo *fi; (fi=it.current()); ++it )
		{
			/* for each file/dir */
			/* if directory...=> scan recursive */
			if (fi->isDir())
			{
				if(fi->fileName()!="." && fi->fileName()!="..")
				{
					ClearCache(fi->filePath(), sFilter, bDeleteDirs);
				
					/* Eventually delete the directory */
					if (bDeleteDirs == TRUE)
						dir.rmdir(fi->fileName());
				}
			}
			else
			{
				/* Is a file so remove it */
				dir.remove(fi->fileName());
			}	
		}
	}
}

void MultSettingsDlg::OnbuttonClearCacheMOT()
{
	/* delete all files and directories into the MOTBWS directory */
	ClearCache(MOT_BROADCAST_WEBSITE_PATH,"", TRUE);
}

void MultSettingsDlg::OnbuttonClearCacheEPG()
{
	/* Delete all EPG files */
	ClearCache(EPG_SAVE_PATH, "*.EHA;*.EHB");
}

void MultSettingsDlg::AddWhatsThisHelp()
{
	//TODO
}
