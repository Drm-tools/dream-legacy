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

#include "GeneralSettingsDlg.h"
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <qvalidator.h>
#include <qmessagebox.h>
#include <qcheckbox.h>


/* Implementation *************************************************************/

GeneralSettingsDlg::GeneralSettingsDlg(CDRMReceiver& NDRMR, CSettings& NSettings,
	QWidget* parent, const char* name, bool modal, WFlags f) :
	CGeneralSettingsDlgBase(parent, name, modal, f), DRMRec(NDRMR),Settings(NSettings),
	host("localhost"),port(2947),bUseGPS(FALSE)
{

	host = Settings.Get("GPS", "host", host);
	port = Settings.Get("GPS", "port", port);
	bUseGPS = Settings.Get("GPS", "usegps", bUseGPS);

	/* Set the validators fro the receiver coordinate */
	EdtLatitudeDegrees->setValidator(new QIntValidator(0, 90, EdtLatitudeDegrees));
	EdtLongitudeDegrees->setValidator(new QIntValidator(0, 180, EdtLongitudeDegrees));

	EdtLatitudeMinutes->setValidator(new QIntValidator(0, 59, EdtLatitudeMinutes));
	EdtLongitudeMinutes->setValidator(new QIntValidator(0, 59, EdtLongitudeMinutes));

	/* Connections */

	connect(buttonOk, SIGNAL(clicked()), SLOT(ButtonOkClicked()) );

	connect(EdtLatitudeNS, SIGNAL(textChanged( const QString &)), this
		, SLOT(CheckSN(const QString&)));

	connect(EdtLongitudeEW, SIGNAL(textChanged( const QString &)), this
		, SLOT(CheckEW(const QString&)));

	connect(CheckBoxUseGPS, SIGNAL(clicked()), SLOT(OnCheckBoxUseGPS()) );

	/* Set help text for the controls */
	AddWhatsThisHelp();
}

GeneralSettingsDlg::~GeneralSettingsDlg()
{
	Settings.Put("GPS", "host", host);
	Settings.Put("GPS", "port", port);
	Settings.Put("GPS", "usegpsd", bUseGPS);
}

void GeneralSettingsDlg::hideEvent(QHideEvent*)
{
}

void GeneralSettingsDlg::showEvent(QShowEvent*)
{
	/* Clear all fields */
	EdtLongitudeDegrees->setText(""); 
	EdtLongitudeMinutes->setText("");
	EdtLongitudeEW->setText("");
	EdtLatitudeDegrees->setText(""); 
	EdtLatitudeMinutes->setText("");
	EdtLatitudeNS->setText("");

    LineEditGPSHost->setText(host.c_str());
    LineEditGPSPort->setText(QString("%1").arg(port));

	/* Extract the receiver coordinates setted */
	ExtractReceiverCoordinates();
}

void GeneralSettingsDlg::CheckSN(const QString& NewText)
{
	/* Only S or N char are accepted */

	const QString sVal = NewText.upper();

	if (sVal != "S" && sVal != "N" && sVal != "")
		EdtLatitudeNS->setText("");
	else
		if (sVal != NewText) /* if lowercase change to uppercase */
			EdtLatitudeNS->setText(sVal);

}

void GeneralSettingsDlg::CheckEW(const QString& NewText)
{
	/* Only E or W char are accepted */

	const QString sVal = NewText.upper();

	if (sVal != "E" && sVal != "W" && sVal != "")
		EdtLongitudeEW->setText("");
	else
		if (sVal != NewText) /* if lowercase change to uppercase */
			EdtLongitudeEW->setText(sVal);

}

void GeneralSettingsDlg::OnCheckBoxUseGPS()
{
	bUseGPS = CheckBoxUseGPS->isChecked();
}

void GeneralSettingsDlg::ButtonOkClicked()
{
	_BOOLEAN bOK = TRUE;
	_BOOLEAN bAllEmpty = TRUE;
	_BOOLEAN bAllCompiled = FALSE;

	/* Check the values and close the dialog */

	if (ValidInput(EdtLatitudeDegrees) == FALSE)
	{
		bOK = FALSE;
		QMessageBox::information(this, "Dream",
			tr("Latitude value must be in the range 0 to 90")
			,QMessageBox::Ok);
	}
	else if (ValidInput(EdtLongitudeDegrees) == FALSE)
	{
		bOK = FALSE;

		QMessageBox::information(this, "Dream",
			tr("Longitude value must be in the range 0 to 180")
			,QMessageBox::Ok);
	}
	else if (ValidInput(EdtLongitudeMinutes) == FALSE
		|| ValidInput(EdtLatitudeMinutes) == FALSE)
	{
		bOK = FALSE;

		QMessageBox::information(this, "Dream",
			tr("Minutes value must be in the range 0 to 59")
			,QMessageBox::Ok);
	}
	
	if (bOK == TRUE)
	{
		/* Check if all coordinates are empty */

		bAllEmpty = (EdtLongitudeDegrees->text() 
			+ EdtLongitudeMinutes->text()
			+ EdtLongitudeEW->text()
			+ EdtLatitudeDegrees->text() 
			+ EdtLatitudeMinutes->text()
			+ EdtLatitudeNS->text()
			) == "";

		/* Check if all coordinates are compiled */

		bAllCompiled = (EdtLongitudeDegrees->text() != "")
			&& (EdtLongitudeMinutes->text() != "")
			&& (EdtLongitudeEW->text() != "")
			&& (EdtLatitudeDegrees->text() != "") 
			&& (EdtLatitudeMinutes->text() != "")
			&& (EdtLatitudeNS->text() != "");

		if (!bAllEmpty && !bAllCompiled)
		{
			bOK = FALSE;

			QMessageBox::information(this, "Dream",
				tr("Compile all fields on receiver coordinates")
				,QMessageBox::Ok);
		}
	}

	if (bOK == TRUE)
	{
		/* save current settings */

		CParameter& Parameters = *DRMRec.GetParameters();
		Parameters.Lock(); 

		if (!bAllEmpty)
		{
			double latitude, longitude;

			latitude = EdtLatitudeDegrees->text().toDouble() + EdtLatitudeMinutes->text().toDouble()/60.0;
			if(EdtLatitudeNS->text().upper().latin1()[0]=='S')
				latitude = - latitude;

			longitude = EdtLongitudeDegrees->text().toDouble() + EdtLongitudeMinutes->text().toDouble()/60.0;
			if(EdtLongitudeEW->text().upper().latin1()[0]=='W')
				longitude = - longitude;

			Parameters.GPSData.SetPositionAvailable(TRUE);
			Parameters.GPSData.SetLatLongDegrees(latitude, longitude);

		}
		else
		{
			Parameters.GPSData.SetPositionAvailable(FALSE);
		}
		Parameters.Unlock(); 

    	host = LineEditGPSHost->text().latin1();
    	port = LineEditGPSPort->text().toInt();

		accept(); /* If the values are valid close the dialog */
	}
}

_BOOLEAN GeneralSettingsDlg::ValidInput(const QLineEdit* le)
{
QString sText;

	/* Use the validator for check if the value is valid */

	sText = le->text();

	if (sText == "")
		return TRUE;
	else
	{
		int iPosCursor = 0;
		return le->validator()->validate(sText,iPosCursor) == QValidator::Acceptable;
	}
}

QString GeneralSettingsDlg::ExtractDigits(const QString strStr, const int iStart
	, const int iDigits)
{
QString sVal;
QChar ch;
_BOOLEAN bStop;

	/* Extract the first iDigits from position iStart */

	sVal = "";
	bStop = FALSE;

	for (int i = iStart - 1 ; i <= iStart + iDigits - 1; i++)
	{
		if (bStop == FALSE)
		{
			ch = strStr.at(i);
			if (ch.isDigit() == TRUE)
				sVal = sVal + ch;
			else
				bStop = TRUE;
		}
	}
	return sVal;
}

void GeneralSettingsDlg::ExtractReceiverCoordinates()
{
	QString sVal, sDir;

	/* parse the latitude and longitude string stored into Dream settings to
		extract local latitude and longitude coordinates */

	double latitude, longitude;

	CParameter& Parameters = *DRMRec.GetParameters();
	Parameters.Lock(); 
	Parameters.GPSData.GetLatLongDegrees(latitude, longitude);

	/* Extract latitude values */

	if(latitude<0.0)
	{
		latitude = -latitude;
		EdtLatitudeNS->setText("S");
	}
	else
	{
		EdtLatitudeNS->setText("N");
	}

	unsigned int Minutes;

	/* Extract degrees */

	/* Latitude degrees max 2 digits */
	sVal = QString("%1").arg(int(latitude));

	EdtLatitudeDegrees->setText(sVal);

	/* Extract minutes */
	Minutes = Parameters.GPSData.ExtractMinutes(latitude);
	sVal = QString("%1").arg(Minutes);

	EdtLatitudeMinutes->setText(sVal);

	/* Extract longitude values */

	if(longitude<0.0)
	{
		longitude = -longitude;
		EdtLongitudeEW->setText("W");
	}
	else if(longitude>180.0)
	{
		longitude = 360.0-longitude;
		EdtLongitudeEW->setText("E");
	}
	else
	{
		EdtLongitudeEW->setText("E");
	}

	/* Extract degrees */

	Minutes = Parameters.GPSData.ExtractMinutes(longitude);

	/* Longitude degrees max 3 digits */
	sVal = QString("%1").arg(int(longitude));

	EdtLongitudeDegrees->setText(sVal);

	/* Extract minutes */
	sVal = QString("%1").arg(Minutes);

	EdtLongitudeMinutes->setText(sVal);

	Parameters.Unlock(); 

}

void GeneralSettingsDlg::AddWhatsThisHelp()
{
	QWhatsThis::add(this,
		tr("<b>Receiver coordinates:</b> Are used on "
		"Live Schedule Dialog to show a little green cube on the left"
		" of the target column if the receiver coordinates (latitude and longitude)"
		" are inside the target area of this transmission.<br>"
		"Receiver coordinates are also saved into the Log file."));
}

