/******************************************************************************\
* Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
* Copyright (c) 2001
*
* Author(s):
*	Alexander Kurpiers
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


#ifndef _SOUND_COMMON_H
#define _SOUND_COMMON_H

#ifdef USE_QT_GUI
#include <qmutex.h>
#endif
#include "../../common/util/Buffer.h"
#ifdef USE_ALSA
#include <alsa/asoundlib.h>
#endif

class CSoundBuf : public CCyclicBuffer<_SAMPLE> {

public:
	CSoundBuf() : keep_running(TRUE) {}
	bool keep_running;
#ifdef USE_QT_GUI
	void lock (){ data_accessed.lock(); }
	void unlock (){ data_accessed.unlock(); }
	
protected:
	QMutex	data_accessed;
#else
	void lock (){ }
	void unlock (){ }
#endif
};

#ifdef USE_QT_GUI
typedef QThread CThread;
#else
class CThread {
public:
	void run() {}
	void start() {}
	void wait(int) {}
	void msleep(int) {}
	bool running() { return true; }
};
#endif

#endif
