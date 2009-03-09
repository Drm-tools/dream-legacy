/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: Journaline Specialisation of TextBrowser
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

#include "jlbrowser.h"
#include "../datadecoding/DataDecoder.h"
#include "../datadecoding/Journaline.h"

JLBrowser::JLBrowser(QWidget * parent)
: QTextBrowser(parent),datadecoder(NULL)
{

	/* Set FhG IIS text */
	strFhGIISText =
		"<table><tr><td><img src=\":/icons/fhgiis.bmp\"></td>"
		"<td><font face=\"Courier\" size=\"-1\">Features NewsService "
		"Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
		"Germany. For more information visit http://www.iis.fhg.de/dab"
		"</font></td></tr></table>";

	/* Set Journaline headline text */
	strJournalineHeadText =
		"<table><tr><td><img src=\":/icons/LogoJournaline.png\"></td>"
		"<td valign=\"middle\"><h2>NewsService Journaline" + QString(QChar(174)) /* (R) */ +
		"</h2></td></tr></table>";
}

QVariant JLBrowser::loadResource( int type, const QUrl & name )
{
	/* Get news from actual Journaline decoder */
	CNews News;
	if(datadecoder==NULL)
        return QVariant::Invalid;

    int JourID = name.toString().toInt();

cerr << "JLB " << JourID << " name " << name.toString().toStdString() << endl;
	datadecoder->GetNews(JourID, News);

	/* Decode UTF-8 coding for title */
	QString strTitle = QString().fromUtf8(News.sTitle.c_str());

	QString strItems = "";
	for (int i = 0; i < News.vecItem.Size(); i++)
	{
		QString strCurItem = QString().fromUtf8(News.vecItem[i].sText.c_str());

		/* Replace \n by html command <br> */
		strCurItem = strCurItem.replace(QRegExp("\n"), "<br>");

		switch(News.vecItem[i].iLink)
		{
        case JOURNALINE_IS_NO_LINK: /* Only text, no link */
			strItems += strCurItem + QString("<br>");
			break;
        case JOURNALINE_LINK_NOT_ACTIVE:
			/* Un-ordered list item without link */
			strItems += QString("<li>") + strCurItem + QString("</li>");
			break;
        default:
            QString strLinkStr = QString("%1").arg(News.vecItem[i].iLink);
            /* Un-ordered list item with link */
            strItems += QString("<li><a href=\"") + strLinkStr +
                QString("\">") + strCurItem + QString("</a></li>");
		}
	}

	/* Set html text. Standard design. The first character must be a "<". This
	   is used to identify whether normal text is displayed or an ID was set */
	QString strAllText =
		strJournalineHeadText +
		"<table>"
		"<tr><td><hr></td></tr>" /* horizontial line */
		"<tr><td><stylebody><b><center>" + strTitle + "</center></b></stylebody></td></tr>"
		"<tr><td><stylebody><ul type=\"square\">" + strItems +
		"</ul></stylebody></td></tr>"
		"<tr><td><hr></td></tr>" /* horizontial line */
		"</table>";
		//+ strFhGIISText;

    return strAllText;
}

