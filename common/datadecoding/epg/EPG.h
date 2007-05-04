/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide utilities
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

#include <qdom.h>
#include <qmap.h>
#include <qstring.h>
#include "../../GlobalDefinitions.h"
#include "../../Parameter.h"
#include "../DABMOT.h"
#include "epgdec.h"

class EPG
{
  public:
    EPG (CParameter&);
    virtual ~ EPG ()
    {
	saveChannels (servicesFilename);
    }

    void select (const uint32_t chan, const CDateAndTime & d);
    void loadChannels (const QString & fileName);
    void saveChannels (const QString & fileName);
    void addChannel (const string& label, uint32_t sid);
    void parseDoc (const QDomNode &);
    QString parseDuration (const QString & duration);
    QString parseStart (const QString & start);
    void getFile (CEPGDecoder & epg, const QString & fileName);

    class CProg
    {
      public:

	  CProg(): start(""), duration(""), name(""), description("")
			  , id(""), mainGenre(), secondaryGenre(), otherGenre()
		{}
		  
		QString start, duration;
		QString name, description;
		QString id;
		vector<QString> mainGenre, secondaryGenre, otherGenre;
    };

    QMap < uint32_t, CProg > progs;
    QMap < QString, QString > genres;
    QString dir, servicesFilename;
    CEPGDecoder basic, advanced;
	CParameter& Parameters;
private:
    static const struct gl { char *genre; char* desc; } genre_list[];
};
