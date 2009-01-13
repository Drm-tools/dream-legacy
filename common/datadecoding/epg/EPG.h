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
#include <qdatetime.h>
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
	/* assignment operator to help MSVC8 */
	EPG& operator=(const EPG&);

    void select (const uint32_t chan, const CDateAndTime & d);
    void loadChannels (const QString & fileName);
    void saveChannels (const QString & fileName);
    void addChannel (const string& label, uint32_t sid);
    void parseDoc (const QDomNode &);
    void getFile (CEPGDecoder & epg, const QString & fileName);

    class CProg
    {
      public:

	  CProg(): time(), actualTime(), duration(0), actualDuration(0),
                name(""), description(""),
			  crid(""), shortId(0), mainGenre(), secondaryGenre(), otherGenre()
		{}
        void augment(const CProg&);

		QDateTime time, actualTime;
		int duration, actualDuration;
		QString name, description;
		QString crid;
		uint32_t shortId;
		vector<QString> mainGenre, secondaryGenre, otherGenre;
    };

    QMap < QDateTime, CProg > progs;
    QMap < QString, QString > genres;
    QString dir, servicesFilename;
    CEPGDecoder basic, advanced;
	CParameter& Parameters;
private:
    static const struct gl { const char *genre; const char* desc; } genre_list[];
    QDateTime parseTime(const QString & time);
    int parseDuration (const QString & duration);
};
