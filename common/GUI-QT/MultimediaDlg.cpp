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
	/* Picture controls should be invisable. These controls are only used for
	   storing the resources */
	PixmapFhGIIS->hide();
	PixmapLogoJournaline->hide();

	/* Set pictures in source factory */
	QMimeSourceFactory::defaultFactory()->setImage("PixmapFhGIIS",
		PixmapFhGIIS->pixmap()->convertToImage());
	QMimeSourceFactory::defaultFactory()->setImage("PixmapLogoJournaline",
		PixmapLogoJournaline->pixmap()->convertToImage());

	/* Set FhG IIS text */
	strFhGIISText = "<table><tr><td><img source=\"PixmapFhGIIS\"></td>"
		"<td><font face=\"Courier\" size=\"-1\">Features NewsService "
		"Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
		"Germany. For more information visit http://www.iis.fhg.de/dab"
		"</font></td></tr></table>";

	/* Set Journaline headline text */
	strJournalineHeadText =
		"<table><tr><td><img source=\"PixmapLogoJournaline\"></td>"
		"<td><h2>NewsService Journaline" + QString(QChar(174)) /* (R) */ +
		"</h2></td></tr></table>";


	/* Set Menu ***************************************************************/
	/* File menu ------------------------------------------------------------ */
	pFileMenu = new QPopupMenu(this);
	CHECK_PTR(pFileMenu);
	pFileMenu->insertItem("C&lear all", this, SLOT(OnClearAll()),
		CTRL+Key_X, 0);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem("&Save...", this, SLOT(OnSave()), CTRL+Key_S, 1);
	pFileMenu->insertItem("Save &all...", this, SLOT(OnSaveAll()),
		CTRL+Key_A, 2);
	pFileMenu->insertSeparator();
	pFileMenu->insertItem("&Close", this, SLOT(close()), CTRL+Key_C, 3);


	/* Main menu bar -------------------------------------------------------- */
	pMenu = new QMenuBar(this);
	CHECK_PTR(pMenu);
	pMenu->insertItem("&File", pFileMenu);

	/* Now tell the layout about the menu */
	MultimediaDlgBaseLayout->setMenuBar(pMenu);

	
	/* Update time for color LED */
	LEDStatus->SetUpdateTime(1000);

	/* Init container and GUI */
	InitApplication(DRMReceiver.GetDataDecoder()->GetAppType());


	/* Connect controls */
	connect(PushButtonStepBack, SIGNAL(clicked()),
		this, SLOT(OnButtonStepBack()));
	connect(PushButtonStepForw, SIGNAL(clicked()),
		this, SLOT(OnButtonStepForw()));
	connect(PushButtonJumpBegin, SIGNAL(clicked()),
		this, SLOT(OnButtonJumpBegin()));
	connect(PushButtonJumpEnd, SIGNAL(clicked()),
		this, SLOT(OnButtonJumpEnd()));
	connect(TextBrowser, SIGNAL(textChanged()),
		this, SLOT(OnTextChanged()));

	connect(&Timer, SIGNAL(timeout()),
		this, SLOT(OnTimer()));
}

void MultimediaDlg::InitApplication(CDataDecoder::EAppType eNewAppType)
{
	/* Set internal parameter */
	eAppType = eNewAppType;

	/* Actual inits */
	switch (eAppType)
	{
	case CDataDecoder::AT_MOTSLISHOW:
		InitMOTSlideShow();
		break;

	case CDataDecoder::AT_JOURNALINE:
		InitJournaline();
		break;

	case CDataDecoder::AT_NOT_SUP:
	default:
		InitNotSupported();
		break;
	}
}

void MultimediaDlg::OnTextChanged()
{
	/* Check, if the current text is a link ID or regular text */
	if (TextBrowser->text().compare(TextBrowser->text().left(1), "<") != 0)
	{
		/* Save old ID */
		NewIDHistory.Add(iCurJourObjID);

		/* Set text to news ID text which was selected by the user */
		iCurJourObjID = TextBrowser->text().toInt();
		SetJournalineText();
	}
}

void MultimediaDlg::OnTimer()
{
	CMOTObject	NewPic;
	QPixmap		NewImage;
	_BOOLEAN	bPicLoadSuccess;
	FILE*		pFiBody;
	int			iCurNumPict;

	/* Check out which application is transmitted right now */
	CDataDecoder::EAppType eNewAppType =
		DRMReceiver.GetDataDecoder()->GetAppType();

	if (eNewAppType != eAppType)
		InitApplication(eNewAppType);

	switch (eAppType)
	{
	case CDataDecoder::AT_MOTSLISHOW:
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
				SetSlideShowPicture();
			}
			else
				UpdateAccButtonsSlideShow();
		}
		break;

	case CDataDecoder::AT_JOURNALINE:
		SetJournalineText();
		break;
	}
}

void MultimediaDlg::SetJournalineText()
{
	/* Get news from actual Journaline decoder */
	CNews News;
	DRMReceiver.GetDataDecoder()->GetNews(iCurJourObjID, News);

	/* Decode UTF-8 coding for title */
	QString strTitle = QString().fromUtf8(QCString(News.sTitle.c_str()));

	/* News items */
	QString strItems("");
	for (int i = 0; i < News.vecItem.Size(); i++)
	{
		/* Decode UTF-8 coding of this item text */
		QString strCurItem = QString().fromUtf8(
			QCString(News.vecItem[i].sText.c_str()));

		if (News.vecItem[i].iLink == JOURNALINE_IS_NO_LINK)
		{
			/* Only text, no link */
			strItems += strCurItem + QString("<br>");
		}
		else if (News.vecItem[i].iLink == JOURNALINE_LINK_NOT_ACTIVE)
		{
			/* Un-ordered list item without link */
			strItems += QString("<li>") + strCurItem + QString("</li>");
		}
		else
		{
			QString strLinkStr = QString().setNum(News.vecItem[i].iLink);

			/* Un-ordered list item with link */
			strItems += QString("<li><a href=\"") + strLinkStr +
				QString("\">") + strCurItem +
				QString("</a></li>");

			/* Store link location in factory (stores ID) */
			QMimeSourceFactory::defaultFactory()->
				setText(strLinkStr, strLinkStr);
		}
	}

	/* Set html text. Standard design. The first character must be a "<". This
	   is used to identify whether normal text is displayed or an ID was set */
	QString strAllText =
		"<table>"
		"<tr><th>" + strJournalineHeadText + "</th></tr>"
		"<tr><td><hr></td></tr>" /* horizontial line */
		"<tr><th>" + strTitle + "</th></tr>"
		"<tr><td><ul type=\"square\">" + strItems + "</ul></td></tr>"
		"<tr><td><hr></td></tr>" /* horizontial line */
		"<tr><td>" + strFhGIISText + "</td></tr>"
		"</table>";

	/* Only update text browser if text has changed */
	if (TextBrowser->text().compare(strAllText) != 0)
		TextBrowser->setText(strAllText);
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

void MultimediaDlg::OnButtonStepBack()
{
	switch (eAppType)
	{
	case CDataDecoder::AT_MOTSLISHOW:
		iCurImagePos--;
		SetSlideShowPicture();
		break;

	case CDataDecoder::AT_JOURNALINE:
		/* Step one level back, get ID from history */
		iCurJourObjID = NewIDHistory.Back();
		SetJournalineText();
		break;
	}
}

void MultimediaDlg::OnButtonStepForw()
{
	iCurImagePos++;
	SetSlideShowPicture();
}

void MultimediaDlg::OnButtonJumpBegin()
{
	/* Reset current picture number to zero (begin) */
	iCurImagePos = 0;
	SetSlideShowPicture();
}

void MultimediaDlg::OnButtonJumpEnd()
{
	/* Go to last received picture */
	iCurImagePos = GetIDLastPicture();
	SetSlideShowPicture();
}

void MultimediaDlg::SetSlideShowPicture()
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

	UpdateAccButtonsSlideShow();
}

void MultimediaDlg::UpdateAccButtonsSlideShow()
{
	/* Set enable menu entry for saving a picture */
	if (iCurImagePos < 0)
	{
		pFileMenu->setItemEnabled(0, FALSE);
		pFileMenu->setItemEnabled(1, FALSE);
		pFileMenu->setItemEnabled(2, FALSE);
	}
	else
	{
		pFileMenu->setItemEnabled(0, TRUE);
		pFileMenu->setItemEnabled(1, TRUE);
		pFileMenu->setItemEnabled(2, TRUE);
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

void MultimediaDlg::ClearAllSlideShow()
{
	/* Init vector which will store the received images with zero size */
	vecRawImages.Init(0);

	/* Init current image position */
	iCurImagePos = -1;

	/* Update GUI */
	UpdateAccButtonsSlideShow();

	/* Init text browser window */
	TextBrowser->setText("<center><h2>"
		"MOT Slide Show Viewer</h2></center>");
}

void MultimediaDlg::InitNotSupported()
{
	/* Hide all controls, disable menu items */
	pFileMenu->setItemEnabled(0, FALSE);
	pFileMenu->setItemEnabled(1, FALSE);
	pFileMenu->setItemEnabled(2, FALSE);
	PushButtonStepForw->hide();
	PushButtonJumpBegin->hide();
	PushButtonJumpEnd->hide();
	LabelCurPicNum->hide();
	PushButtonStepBack->hide();

	/* Show that application is not supported */
	TextBrowser->setText("<center><h2>No data service or data service not "
		"supported.</h2></center>");
}

void MultimediaDlg::InitMOTSlideShow()
{
	/* Make all browse buttons visible */
	PushButtonStepBack->show();
	PushButtonStepForw->show();
	PushButtonJumpBegin->show();
	PushButtonJumpEnd->show();
	LabelCurPicNum->show();

	/* Enable "clear all" menu item */
	pFileMenu->setItemEnabled(0, TRUE);

	/* Enable "save" menu items */
	pFileMenu->setItemEnabled(1, TRUE);
	pFileMenu->setItemEnabled(2, TRUE);

	ClearAllSlideShow();
}

void MultimediaDlg::InitJournaline()
{
	/* Disable "clear all" menu item */
	pFileMenu->setItemEnabled(0, FALSE);

	/* Disable "save" menu items */
	pFileMenu->setItemEnabled(1, FALSE);
	pFileMenu->setItemEnabled(2, FALSE);

	/* Only one back button is visible and enabled */
	PushButtonStepForw->hide();
	PushButtonJumpBegin->hide();
	PushButtonJumpEnd->hide();
	LabelCurPicNum->hide();
	PushButtonStepBack->show();
	PushButtonStepBack->setEnabled(TRUE);

	/* Init text browser window */
	iCurJourObjID = 0;
	SetJournalineText();

	NewIDHistory.Reset();
}
