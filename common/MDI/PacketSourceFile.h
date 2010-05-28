/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007
 *
 * Author(s):
 *	Juian Cable
 *
 * Description:
 *	Implementation of a CPacketSource that reads from a file
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

#ifndef _PACKETSOURCE_FILE_H
#define _PACKETSOURCE_FILE_H

#include "../GlobalDefinitions.h"
#include "../util/Vector.h"
#include "../util/Buffer.h"
#include "PacketInOut.h"
#include <qobject.h>
#include <qdatetime.h>

class CPacketSourceFile : public QObject, public CPacketSource
{
	Q_OBJECT
public:
	CPacketSourceFile();
	virtual ~CPacketSourceFile();
	// Set the sink which will receive the packets
	virtual void SetPacketSink(CPacketSink *pSink);
	// Stop sending packets to the sink
	virtual void ResetPacketSink(void);
	virtual _BOOLEAN SetOrigin(const string& str);

private:

    void readRawOrFF(vector<_BYTE>& vecbydata, int& interval);
    void readPcap(vector<_BYTE>& vecbydata, int& interval);

	CPacketSink		*pPacketSink;
	QTime			timeKeeper;
	uint64_t		last_packet_time;
    void*			pf;
	_BOOLEAN		bRaw;
	int 			wanted_dest_port;
public slots:
	void OnDataReceived();
};

#endif
