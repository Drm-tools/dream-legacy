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
	/* Init transport ID for current picture */
	iCurTransportID = 0;

	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Init vector which will store the received images with zero size */
	vecImages.resize(0);

	/* Init current image position */
	iCurImagePos = -1;

	/* Update GUI */
	UpdateAccButtons();

	/* Init text browser window */
	TextBrowser->setText("<center><b>MOT Slide Show Viewer</center>");

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

		/* Load picture in QT format */
		bPicLoadSuccess =
			NewImage.loadFromData(&NewPic.vecbRawData[0],
			iPicSize, QString(NewPic.strFormat.c_str()));

		if (bPicLoadSuccess == TRUE)
		{
			/* Add successfully imported picture to vector */
			iCurNumPict = vecImages.Size();
			vecImages.Enlarge(1);
			vecImages[iCurNumPict] = NewImage;

			UpdateAccButtons();

			/* If the last received picture was selected, automatically show
			   new picture */
			if (iCurImagePos == iCurNumPict - 1)
			{
				iCurImagePos = iCurNumPict;
				SetPicture();
			}
		}
		else
		{
			TextBrowser->setText("<center><b>Image could not be loaded, format "
				"not supported!"
#ifdef _WIN32
				"<br><br>Try to load image in external viewer."
#endif
				"</b></center>");

			/* Store image in file instead */
			char cFileName[100];
			strcpy(cFileName, "DreamReceivedDataFileTMP.");
			strcat(cFileName, NewPic.strFormat.c_str());

			pFiBody = fopen(cFileName, "wb");

			for (int i = 0; i < iPicSize; i++)
				fwrite((void*) &NewPic.vecbRawData[i], size_t(1), size_t(1),
					pFiBody);

			/* Close the file afterwards */
			fclose(pFiBody);

			/* Call application to show the image */
#ifdef _WIN32
			ShellExecute(NULL, "open", cFileName, NULL,NULL,
				SW_SHOWNORMAL);
#endif
		}
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
	/* Set new picture in source factory and set it in text control */
	QMimeSourceFactory::defaultFactory()->setImage("MOTSlideShowimage",
		vecImages[iCurImagePos].convertToImage());

	TextBrowser->setText("<center><img source=\"MOTSlideShowimage\">"
		"</center>");

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
	iCurImagePos = vecImages.Size() - 1;
	SetPicture();
}

void MultimediaDlg::UpdateAccButtons()
{
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

	if (iCurImagePos == vecImages.Size() - 1)
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
		QString().setNum(vecImages.Size()));
}