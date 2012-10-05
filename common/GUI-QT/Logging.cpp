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

#include "Logging.h"
#include "../util/Settings.h"

/* Implementation *************************************************************/
CLogging::CLogging(CParameter& Parameters, CSettings& Settings) : QObject(),
    TimerLogFileLong(), TimerLogFileShort(), TimerLogFileStart(),
    shortLog(Parameters), longLog(Parameters),
    iLogDelay(0), running(false)
{
#if QT_VERSION >= 0x040000
	TimerLogFileStart.setSingleShot(true);
#endif
    connect(&TimerLogFileLong, SIGNAL(timeout()),
            this, SLOT(OnTimerLogFileLong()));
    connect(&TimerLogFileShort, SIGNAL(timeout()),
            this, SLOT(OnTimerLogFileShort()));
    connect(&TimerLogFileStart, SIGNAL(timeout()),
            this, SLOT(OnTimerLogFileStart()));
	
    /* Logfile -------------------------------------------------------------- */

    /* log file flag for storing signal strength in long log */
    _BOOLEAN logrxl = Settings.Get("Logfile", "enablerxl", FALSE);
    shortLog.SetRxlEnabled(logrxl);
    longLog.SetRxlEnabled(logrxl);

    /* log file flag for storing lat/long in long log */
    bool enablepositiondata = Settings.Get("Logfile", "enablepositiondata", false);
    shortLog.SetPositionEnabled(enablepositiondata);
    longLog.SetPositionEnabled(enablepositiondata);

    /* logging delay value */
    iLogDelay = Settings.Get("Logfile", "delay", 0);
	bool logEnabled = Settings.Get("Logfile", "enablelog", false);

    /* Activate log file start if necessary. */
    if (logEnabled)
    {
        /* One shot timer */
        TimerLogFileStart.start(iLogDelay * 1000 /* ms */);
    }

    /* GPS */
    /* Latitude string for log file */
	double latitude, longitude;
    latitude = Settings.Get("Logfile", "latitude", 1000.0);
    /* Longitude string for log file */
    longitude = Settings.Get("Logfile", "longitude", 1000.0);
    Parameters.Lock();
    Parameters.GPSData.SetLatLongDegrees(latitude, longitude);
	Parameters.Unlock();
}

void CLogging::SaveSettings(CSettings& Settings)
{
	double latitude=0.0, longitude=0.0;
	longLog.GetPosition(latitude, longitude);

    Settings.Put("Logfile", "delay", iLogDelay);
    Settings.Put("Logfile", "enablerxl", shortLog.GetRxlEnabled());
    Settings.Put("Logfile", "enablepositiondata", shortLog.GetPositionEnabled());
    Settings.Put("Logfile", "latitude", latitude);
    Settings.Put("Logfile", "longitude", longitude);
    Settings.Put("Logfile", "enablelog", running);
}

void CLogging::OnTimerLogFileStart()
{
    /* Start logging (if not already done) */
    if(!longLog.GetLoggingActivated() || !longLog.GetLoggingActivated())
    {
        /* Activate log file timer for long and short log file */
        TimerLogFileShort.start(60000); /* Every minute (i.e. 60000 ms) */
        TimerLogFileLong.start(1000); /* Every second */

        /* Open log file */
        shortLog.Start("DreamLog.txt");
        longLog.Start("DreamLogLong.csv");
    }
}

void CLogging::OnTimerLogFileShort()
{
    /* Write new parameters in log file (short version) */
    shortLog.Update();
}

void CLogging::OnTimerLogFileLong()
{
    /* Write new parameters in log file (long version) */
    longLog.Update();
}

void CLogging::start()
{
    TimerLogFileStart.start(iLogDelay * 1000 /* ms */);
    if(longLog.GetRxlEnabled())
    {
        emit subscribeRig();
    }
}

void CLogging::stop()
{
    TimerLogFileStart.stop();
    TimerLogFileShort.stop();
    TimerLogFileLong.stop();
    shortLog.Stop();
    longLog.Stop();
    if(longLog.GetRxlEnabled())
    {
        emit unsubscribeRig();
    }
}
