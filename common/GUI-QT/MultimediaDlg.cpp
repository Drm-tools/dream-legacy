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

#include "MultimediaDlg.h"


MultimediaDlg::MultimediaDlg(QWidget* parent, const char* name, bool modal,
	WFlags f) : MultimediaDlgBase(parent, name, modal, f)
{
	/* Set Menu ***************************************************************/
	/* File menu ------------------------------------------------------------ */
	pFileMenu = new QPopupMenu(this);
	CHECK_PTR(pFileMenu);
	pFileMenu->insertItem("C&lear all", this, SLOT(OnClearAll()), CTRL+Key_X);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem("&Save...", this, SLOT(OnSave()), CTRL+Key_S);
	pFileMenu->insertItem("Save &all...", this, SLOT(OnSaveAll()), CTRL+Key_A);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem("&Close", this, SLOT(close()), CTRL+Key_C);


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&File", pFileMenu);

	/* Now tell the layout about the menu */
	MultimediaDlgBaseLayout->setMenuBar(pMenu);

	
	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Init container and GUI */
	ClearAll();

	/* Connect controls */
	connect(PushButtonStepBack, SIGNAL(clicked()),
		this, SLOT(OnButtonStepBack()));
	connect(PushButtonStepForw, SIGNAL(clicked()),
		this, SLOT(OnButtonStepForw()));
	connect(PushButtonJumpBegin, SIGNAL(clicked()),
		this, SLOT(OnButtonJumpBegin()));
	connect(PushButtonJumpEnd, SIGNAL(clicked()),
		this, SLOT(OnButtonJumpEnd()));

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
}

void MultimediaDlg::OnTimer()
{
	CMOTObject	NewPic;
	QPixmap		NewImage;
	_BOOLEAN	bPicLoadSuccess;
	FILE*		pFiBody;
	int			iCurNumPict;

	/* Poll the data decoder module for new picture */
	if (DRMReceiver.GetDataDecoder()->GetSlideShowPicture(NewPic) == TRUE)
	{
		/* Store received picture */
		iCurNumPict = vecRawImages.Size();
		vecRawImages.Enlarge(1);

		/* Copy new picture in storage container */
		vecRawImages[iCurNumPict] = NewPic;

		/* If the last received picture was selected, automatically show
		   new picture */
		if (iCurImagePos == iCurNumPict - 1)
		{
			iCurImagePos = iCurNumPict;
			SetPicture();
		}
		else
			UpdateAccButtons();
	}
}

void MultimediaDlg::showEvent(QShowEvent* pEvent)
{
	/* Activte real-time timer when window is shown */
	Timer.start(GUI_CONTROL_UPDATE_TIME);

	/* Update window */
	OnTimer();
}

void MultimediaDlg::hideEvent(QHideEvent* pEvent)
{
	/* Deactivate real-time timer so that it does not get new pictures */
	Timer.stop();
}

void MultimediaDlg::SetStatus(int MessID, int iMessPara)
{
	switch(MessID)
	{
	case MS_MOT_OBJ_STAT:
		LEDStatus->SetLight(iMessPara);
		break;
	}
}

void MultimediaDlg::SetPicture()
{
	_BOOLEAN	bPicLoadSuccess;
	QPixmap		NewImage;
	int			iPicSize;

	/* Get picture size */
	iPicSize = vecRawImages[iCurImagePos].vecbRawData.Size();

	/* Load picture in QT format */
	bPicLoadSuccess =
		NewImage.loadFromData(&vecRawImages[iCurImagePos].vecbRawData[0],
		iPicSize, QString(vecRawImages[iCurImagePos].strFormat.c_str()));

	if (bPicLoadSuccess == TRUE)
	{
		/* Set new picture in source factory and set it in text control */
		QMimeSourceFactory::defaultFactory()->setImage("MOTSlideShowimage",
			NewImage.convertToImage());

		TextBrowser->setText("<center><img source=\"MOTSlideShowimage\">"
			"</center>");
	}
	else
	{
		/* Show text that tells the user of load failure */
		TextBrowser->setText("<br><br><center><b>Image could not be "
			"loaded, " +
			 QString(vecRawImages[iCurImagePos].strFormat.c_str()) +
			 "-format not supported by this version of QT!"
			"</b><br><br><br>If you want to view the image, "
			"save it to file and use an external viewer</center>");
	}

	UpdateAccButtons();
}

void MultimediaDlg::OnButtonStepBack()
{
	iCurImagePos--;
	SetPicture();
}

void MultimediaDlg::OnButtonStepForw()
{
	iCurImagePos++;
	SetPicture();
}

void MultimediaDlg::OnButtonJumpBegin()
{
	/* Reset current picture number to zero (begin) */
	iCurImagePos = 0;
	SetPicture();
}

void MultimediaDlg::OnButtonJumpEnd()
{
	/* Go to last received picture */
	iCurImagePos = GetIDLastPicture();
	SetPicture();
}

void MultimediaDlg::UpdateAccButtons()
{
	/* Set enable menu entry for saving a picture */
	if (iCurImagePos < 0)
	{
		pFileMenu->setItemEnabled(0, FALSE);
		pFileMenu->setItemEnabled(1, FALSE);
	}
	else
	{
		pFileMenu->setItemEnabled(0, TRUE);
		pFileMenu->setItemEnabled(1, TRUE);
	}

	if (iCurImagePos <= 0)
	{
		/* We are already at the beginning */
		PushButtonStepBack->setEnabled(FALSE);
		PushButtonJumpBegin->setEnabled(FALSE);
	}
	else
	{
		PushButtonStepBack->setEnabled(TRUE);
		PushButtonJumpBegin->setEnabled(TRUE);
	}

	if (iCurImagePos == GetIDLastPicture())
	{
		/* We are already at the end */
		PushButtonStepForw->setEnabled(FALSE);
		PushButtonJumpEnd->setEnabled(FALSE);
	}
	else
	{
		PushButtonStepForw->setEnabled(TRUE);
		PushButtonJumpEnd->setEnabled(TRUE);
	}

	LabelCurPicNum->setText(QString().setNum(iCurImagePos + 1) + "/" +
		QString().setNum(GetIDLastPicture() + 1));
}

void MultimediaDlg::OnSave()
{
	/* Show "save file" dialog */
	QString strFileName =
		QFileDialog::getSaveFileName("RecPic." +
		QString(vecRawImages[iCurImagePos].strFormat.c_str()),
		"*." + QString(vecRawImages[iCurImagePos].strFormat.c_str()), this);

	/* Check if user not hit the cancel button */
    if (!strFileName.isNull())
		SavePicture(iCurImagePos, strFileName);
}

void MultimediaDlg::OnSaveAll()
{
	/* Let the user choose a directory */
	QString strDirName =
		QFileDialog::getExistingDirectory(NULL, this);

    if (!strDirName.isNull())
	{
		/* Loop over all pictures received yet */
		for (int j = 0; j < GetIDLastPicture() + 1; j++)
		{
			/* Construct file name from date and picture number */
			QString strFileName = strDirName + "Dream_" + 
				QDate().currentDate().toString() + "_#" +
				QString().setNum(j) + "." +
				QString(vecRawImages[j].strFormat.c_str());

			SavePicture(j, strFileName);
		}
	}
}

void MultimediaDlg::SavePicture(const int iPicID, const QString& strFileName)
{
	/* Get picture size */
	const int iPicSize = vecRawImages[iPicID].vecbRawData.Size();

	/* Open file */
	FILE* pFiBody = fopen(strFileName.latin1(), "wb");

	if (pFiBody != NULL)
	{
		for (int i = 0; i < iPicSize; i++)
			fwrite((void*) &vecRawImages[iPicID].vecbRawData[i],
				size_t(1), size_t(1), pFiBody);

		/* Close the file afterwards */
		fclose(pFiBody);
	}
}

void MultimediaDlg::ClearAll()
{
	/* Init vector which will store the received images with zero size */
	vecRawImages.Init(0);

	/* Init current image position */
	iCurImagePos = -1;

	/* Update GUI */
	UpdateAccButtons();

	/* Init text browser window */
	TextBrowser->setText("<center><b><font size=7>"
		"MOT Slide Show Viewer</font></b></center>");
}
