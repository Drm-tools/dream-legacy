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


MultimediaDlg::MultimediaDlg(QWidget* parent, const char* name, bool modal, WFlags f )
	: MultimediaDlgBase( parent, name, modal, f)
{
	/* Set Menu ***************************************************************/
	/* File menu ------------------------------------------------------------ */
	pFileMenu = new QPopupMenu(this);
	CHECK_PTR(pFileMenu);
	pFileMenu->insertItem("&Save...", this, SLOT(OnSave()), CTRL+Key_S, 0);


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&File", pFileMenu);

	/* Now tell the layout about the menu */
	MultimediaDlgBaseLayout->setMenuBar(pMenu);

	
	/* Init transport ID for current picture */
	iCurTransportID = 0;

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Init vector which will store the received images with zero size */
	vecpRawImages.Init(0);

	/* Init current image position */
	iCurImagePos = -1;

	/* Update GUI */
	UpdateAccButtons();

	/* Init text browser window */
	TextBrowser->setText("<center><b><font size=7>"
		"MOT Slide Show Viewer</font></b></center>");


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
	CMOTPicture NewPic;
	QPixmap		NewImage;
	_BOOLEAN	bPicLoadSuccess;
	FILE*		pFiBody;
	int			iPicSize;
	int			iCurNumPict;

	/* Poll the data decoder module for new picture */
	DRMReceiver.GetDataDecoder()->GetSlideShowPicture(NewPic);

	if ((iCurTransportID != NewPic.iTransportID) && (NewPic.iTransportID != -1))
	{
		iCurTransportID = NewPic.iTransportID;

		/* Get picture size */
		iPicSize = NewPic.vecbRawData.Size();

		/* Store received picture */
		iCurNumPict = vecpRawImages.Size();
		vecpRawImages.Enlarge(1);

		vecpRawImages[iCurNumPict] = new CMOTPicture;
		vecpRawImages[iCurNumPict]->vecbRawData.Init(iPicSize);

		/* Actual data */
		for (int i = 0; i < iPicSize; i++)
			vecpRawImages[iCurNumPict]->vecbRawData[i] = NewPic.vecbRawData[i];

		/* Format string */
		vecpRawImages[iCurNumPict]->strFormat = NewPic.strFormat;

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
	iPicSize = vecpRawImages[iCurImagePos]->vecbRawData.Size();

	/* Load picture in QT format */
	bPicLoadSuccess =
		NewImage.loadFromData(&vecpRawImages[iCurImagePos]->vecbRawData[0],
		iPicSize, QString(vecpRawImages[iCurImagePos]->strFormat.c_str()));

	if (bPicLoadSuccess == TRUE)
	{
		/* Set new picture in source factory and set it in text control */
		QMimeSourceFactory::defaultFactory()->setImage("MOTSlideShowimage",
			NewImage.convertToImage());

		TextBrowser->setText("<center><img source=\"MOTSlideShowimage\">"
			"</center>");
		}
	else
		TextBrowser->setText("<br><br><center><b>Image could not be "
			"loaded, format not supported by this version of QT!"
			"</b><br><br><br>If you want to view the image, "
			"save it to file and use an external viewer</center>");

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
		pFileMenu->setItemEnabled(0, FALSE);
	else
		pFileMenu->setItemEnabled(0, TRUE);

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
		QString(vecpRawImages[iCurImagePos]->strFormat.c_str()),
		"*." + QString(vecpRawImages[iCurImagePos]->strFormat.c_str()), this);

    if (!strFileName.isNull())
	{
		/* Get picture size */
		int iPicSize = vecpRawImages[iCurImagePos]->vecbRawData.Size();

		/* Got a file name */
		char cFileName[100];
		strcpy(cFileName, strFileName);

		FILE* pFiBody = fopen(cFileName, "wb");

		for (int i = 0; i < iPicSize; i++)
			fwrite((void*) &vecpRawImages[iCurImagePos]->vecbRawData[i],
				size_t(1), size_t(1), pFiBody);

		/* Close the file afterwards */
		fclose(pFiBody);
	}
}
