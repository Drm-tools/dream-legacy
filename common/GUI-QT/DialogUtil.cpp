/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2005
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

#include "DialogUtil.h"
#include "../Version.h"
#ifdef USE_ALSA
# include <alsa/version.h>
#endif
#ifdef USE_OSS
# include <sys/soundcard.h>
#endif
#ifdef USE_PORTAUDIO
# include <portaudio.h>
#endif
#ifdef HAVE_LIBSNDFILE
# include <sndfile.h>
#endif

/* Implementation *************************************************************/
/* About dialog ------------------------------------------------------------- */
CAboutDlg::CAboutDlg(QWidget* parent, const char* name, bool modal, WFlags f)
	: CAboutDlgBase(parent, name, modal, f)
{
#ifdef HAVE_LIBSNDFILE
	char  sfversion [128] ;
	sf_command (NULL, SFC_GET_LIB_VERSION, sfversion, sizeof (sfversion)) ;
#endif
	/* Set the text for the about dialog html text control */
	TextViewCredits->setText(
		"<p>" /* General description of Dream software */
		"<big><b>Dream</b> " + tr("is a software implementation of a Digital "
		"Radio Mondiale (DRM) receiver. With Dream, DRM broadcasts can be received "
		"with a modified analog receiver (SW, MW, LW) and a PC with a sound card.")
		 + "</big></p><br>"
		"<p><font face=\"" + FONT_COURIER + "\">" /* GPL header text */
		"This program is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version.<br>This program is distributed in "
		"the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
		"even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
		"PARTICULAR PURPOSE. See the GNU General Public License for more "
		"details.<br>You should have received a copy of the GNU General Public "
		"License along with his program; if not, write to the Free Software "
		"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
		"USA"
		"</font></p><br>" /* Our warning text */
		"<p><font color=\"#ff0000\" face=\"" + FONT_COURIER + "\">" +
		tr("Although this software is going to be "
		"distributed as free software under the terms of the GPL this does not "
		"mean that its use is free of rights of others. The use may infringe "
		"third party IP and thus may not be legal in some countries.") +
		"</font></p><br>"
		"<p>" /* Libraries used by this compilation of Dream */
		"<b>" + tr("This compilation of Dream uses the following libraries:") +
		"</b></p>"
		"<ul>"
		"<li><b>FFTW</b> <i>http://www.fftw.org</i></li>"
#ifdef USE_FAAD2_LIBRARY
		"<li><b>FAAD2</b> (" + QString(FAAD2_VERSION) + ") <i>AAC/HE-AAC/HE-AACv2/DRM decoder "
		"(c) Ahead Software, www.nero.com (http://faac.sf.net)</i></li>"
#endif
#ifdef USE_FAAC_LIBRARY
		"<li><b>FAAC</b> <i>http://faac.sourceforge.net</i></li>"
#endif
#ifdef USE_QT_GUI /* QWT */
		"<li><b>Qt</b> (" + QString(QT_VERSION_STR) + ") <i>http://www.trolltech.com</i></li>"
		"<li><b>QWT</b> (" + QString(QWT_VERSION_STR) + ") <i>Dream is based in part on the work of the Qwt "
		"project (http://qwt.sf.net).</i></li>"
#endif
#ifdef HAVE_LIBHAMLIB
		"<li><b>Hamlib</b> (" + QString(hamlib_version) + ") <i>http://hamlib.sourceforge.net</i></li>"
#endif
#ifdef HAVE_JOURNALINE
		"<li><b>FhG IIS Journaline Decoder</b> <i>Features NewsService "
		"Journaline(R) decoder technology by Fraunhofer IIS, Erlangen, "
		"Germany. For more information visit http://www.iis.fhg.de/dab</i></li>"
#endif
#ifdef HAVE_LIBFREEIMAGE
		"<li><b>FreeImage</b> (" + QString(FreeImage_GetVersion()) + ") <i>This software uses the FreeImage open source "
		"image library. See http://freeimage.sourceforge.net for details. "
		"FreeImage is used under the GNU GPL.</i></li>"
#endif
#ifdef HAVE_LIBPCAP
		"<li><b>LIBPCAP</b> (" + QString(pcap_lib_version()) + ") <i>http://www.tcpdump.org/ "
		"This product includes software developed by the Computer Systems "
		"Engineering Group at Lawrence Berkeley Laboratory.</i></li>"
#endif
#ifdef HAVE_LIBSNDFILE
		"<li><b>LIBSNDFILE</b> (" + QString(sfversion) + ") <i>http://www.mega-nerd.com/libsndfile</i></li>"
#endif
#ifdef USE_OSS
		"<li><b>OSS</b> (" + QString("Open Sound System version %1").arg(SOUND_VERSION, 0, 16) + ")</li>"
#endif
#ifdef USE_ALSA
		"<li><b>ALSA</b> (" + QString(SND_LIB_VERSION_STR) + ") <i>http://www.alsa-project.org</i></li>"
#endif
#ifdef USE_PORTAUDIO
		"<li><b>portaudio</b> ("+QString(Pa_GetVersionText())+") <i>http://www.jackaudio.org</i></li>"
#endif
#ifdef USE_JACK
		"<li><b>libjack</b> (The Jack Audio Connection Kit) <i>http://www.jackaudio.org</i></li>"
#endif
		"</ul><br><br><hr/><br><br>"
		"<center><b>HISTORY</b></center><br>"
        "The Dream software development was started at <i>Darmstadt University "
        "of Technology</i> at the Institute of Communication Technology by <i>Volker "
        "Fischer</i> and <i>Alexander Kurpiers</i> in 2001-2005. "
        "The core digital signal processing and most of the GUI were the "
        "result of this development.<br>In 2005, <i>Andrew Murphy</i> of the <i>British "
        "Broadcasting Corporation</i> added code for an "
        "AMSS demodulator. <i>Oliver Haffenden</i> and <i>Julian Cable</i> (also <i>BBC</i>) rewrote "
        "the MDI interface and added RSCI support. The EPG was implemented by "
        "<i>Julian Cable</i> and the Broadcast Website and AFS features as well as many "
        "other GUI improvements were implemented by <i>Andrea Russo</i>.<br>Right now "
        "the code is mainly maintained by <i>Julian Cable</i>."
        "<br><br><br>"
		"<center><b>CREDITS</b></center><br>"
		"We want to thank all the contributors to the Dream software (in "
		"alphabetical order):<br><br>"
		"<b>Developers</b>"
		"<center>"
		"<p>Bakker, Menno</p>"
		"<p>Cesco</p>"
		"<p>Fillod, Stephane</p>"
		"<p>Fine, Mark J.</p>"
		"<p>Manninen, Tomi</p>"
		"<p>Moore, Josh</p>" 
		"<p>Pascutto, Gian C.</p>"
		"<p>Peca, Marek</p>"
		"<p>Richard, Doyle</p>"
		"</center>"
		"<br><b>Parts of Dream are based on code by</b>"
		"<center>"
		"<p>Karn, Phil (www.ka9q.net)</p>"
		"<p>Ptolemy Project (http://ptolemy.eecs.berkeley.edu)</p>"
		"<p>Tavernini, Lucio (http://tavernini.com/home.html)</p>"
		"<p>The Math Forum (http://mathforum.org)</p>"
		"<p>The Synthesis ToolKit in C++ (STK) "
		"(http://ccrma.stanford.edu/software/stk)</p>"
		"</center>"
		"<br><b>Supporters</b>"
		"<center>"
		"<p>Amorim, Roberto José de</p>"
		"<p>Kainka, Burkhard</p>"
		"<p>Keil, Jens</p>"
		"<p>Kilian, Gerd</p>"
		"<p>Kn&uuml;tter, Carsten</p>"
		"<p>Ramisch, Roland</p>"
		"<p>Schall, Norbert</p>"
		"<p>Schill, Dietmar</p>"
		"<p>Schneider, Klaus</p>"
		"<p>St&ouml;ppler, Simone</p>"
		"<p>Varlamov, Oleg</p>"
		"<p>Wade, Graham</p>"
		"</center><br>");

	/* Set version number in about dialog */
	QString strVersionText;
	strVersionText = "<center><b>" + tr("Dream, Version ");
	strVersionText += dream_version;
	strVersionText += "</b><br> " + tr("Open-Source Software Implementation of "
		"a DRM-Receiver") + "<br>";
	strVersionText += tr("Under the GNU General Public License (GPL)") +
		"</center>";
	TextLabelVersion->setText(strVersionText);
}


/* Help menu ---------------------------------------------------------------- */
CDreamHelpMenu::CDreamHelpMenu(QWidget* parent) : QPopupMenu(parent)
{
	/* Standard help menu consists of about and what's this help */
	insertItem(tr("What's &This"), this ,
		SLOT(OnHelpWhatsThis()), SHIFT+Key_F1);
	insertSeparator();
	insertItem(tr("&About..."), this, SLOT(OnHelpAbout()));
}


/* Sound card selection menu ------------------------------------------------ */
CSoundCardSelMenu::CSoundCardSelMenu(
	CSelectionInterface* pNSIn, CSelectionInterface* pNSOut, QWidget* parent) :
	QPopupMenu(parent), pSoundInIF(pNSIn), pSoundOutIF(pNSOut)
{
	pSoundInMenu = new QPopupMenu(parent);
	CHECK_PTR(pSoundInMenu);
	pSoundOutMenu = new QPopupMenu(parent);
	CHECK_PTR(pSoundOutMenu);
	int i;

	/* Get sound device names */
	pSoundInIF->Enumerate(vecSoundInNames);
	iNumSoundInDev = vecSoundInNames.size();
	for (i = 0; i < iNumSoundInDev; i++)
	{
		pSoundInMenu->insertItem(QString(vecSoundInNames[i].c_str()), this,
			SLOT(OnSoundInDevice(int)), 0, i);
	}

	pSoundOutIF->Enumerate(vecSoundOutNames);
	iNumSoundOutDev = vecSoundOutNames.size();
	for (i = 0; i < iNumSoundOutDev; i++)
	{
		pSoundOutMenu->insertItem(QString(vecSoundOutNames[i].c_str()), this,
			SLOT(OnSoundOutDevice(int)), 0, i);
	}

	/* Set default device. If no valid device was selected, select
	   "Wave mapper" */
	int iDefaultInDev = pSoundInIF->GetDev();
	if ((iDefaultInDev > iNumSoundInDev) || (iDefaultInDev < 0))
		iDefaultInDev = iNumSoundInDev;

	int iDefaultOutDev = pSoundOutIF->GetDev();
	if ((iDefaultOutDev > iNumSoundOutDev) || (iDefaultOutDev < 0))
		iDefaultOutDev = iNumSoundOutDev;

	pSoundInMenu->setItemChecked(iDefaultInDev, TRUE);
	pSoundOutMenu->setItemChecked(iDefaultOutDev, TRUE);

	insertItem(tr("Sound &In"), pSoundInMenu);
	insertItem(tr("Sound &Out"), pSoundOutMenu);
}

void CSoundCardSelMenu::OnSoundInDevice(int id)
{
	pSoundInIF->SetDev(id);

	/* Taking care of checks in the menu. "+ 1" because of wave mapper entry */
	for (int i = 0; i < iNumSoundInDev + 1; i++)
		pSoundInMenu->setItemChecked(i, i == id);
}

void CSoundCardSelMenu::OnSoundOutDevice(int id)
{
	pSoundOutIF->SetDev(id);

	/* Taking care of checks in the menu. "+ 1" because of wave mapper entry */
	for (int i = 0; i < iNumSoundOutDev + 1; i++)
		pSoundOutMenu->setItemChecked(i, i == id);
}
