Rem/******************************************************************************\
rem * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
rem * Copyright (c) 2001-2006
rem *
rem * Author(s):
rem *	Volker Fischer
rem *
rem * Description:
rem *	Script for compiling the QT resources under Windows (MOCing and uic3ing)
rem *
rem ******************************************************************************
rem *
rem * This program is free software; you can redistribute it and/or modify it under
rem * the terms of the GNU General Public License as published by the Free Software
rem * Foundation; either version 2 of the License, or (at your option) any later
rem * version.
rem *
rem * This program is distributed in the hope that it will be useful, but WITHOUT
rem * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
rem * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 1111
rem * details.
rem *
rem * You should have received a copy of the GNU General Public License along with
rem * this program; if not, write to the Free Software Foundation, Inc.,
rem * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
rem *
rem\******************************************************************************/

rem .ui -------------
%qtdir%\bin\uic.exe ..\common\GUI-QT\DRMMainWindow.ui -o moc\ui_DRMMainWindow.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\TransmitterMainWindow.ui -o moc\ui_TransmitterMainWindow.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\AnalogMainWindow.ui -o moc\ui_AnalogMainWindow.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\SystemEvalDlg.ui -o moc\ui_SystemEvalDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\AboutDlg.ui -o moc\ui_AboutDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\MultimediaDlg.ui -o moc\ui_MultimediaDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\StationsDlg.ui -o moc\ui_StationsDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\LiveScheduleDlg.ui -o moc\ui_LiveScheduleDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\AMSSDlg.ui -o moc\ui_AMSSDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\EPGDlg.ui -o moc\ui_EPGDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\MultSettingsDlg.ui -o moc\ui_MultSettingsDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\ReceiverSettingsDlg.ui -o moc\ui_ReceiverSettingsDlg.h
%qtdir%\bin\uic.exe ..\common\GUI-QT\LatLongEditDlg.ui -o moc\ui_LatLongEditDlg.h

rem .h --------------
%qtdir%\bin\moc.exe ..\common\GUI-QT\DRMMainWindow.h -o moc\moc_DRMMainWindow.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\TransmDlg.h -o moc\moc_TransmDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\AnalogDemDlg.h -o moc\moc_AnalogDemDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\DialogUtil.h -o moc\moc_DialogUtil.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\MultColorLED.h -o moc\moc_MultColorLED.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\SystemEvalDlg.h -o moc\moc_SystemEvalDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\DRMPlot.h -o moc\moc_DRMPlot.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\MultimediaDlg.h -o moc\moc_MultimediaDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\StationsDlg.h -o moc\moc_StationsDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\LiveScheduleDlg.h -o moc\moc_LiveScheduleDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\EPGDlg.h -o moc\moc_EPGDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\Loghelper.h -o moc\moc_Loghelper.cpp
%qtdir%\bin\moc.exe ..\common\MDI\PacketSourceFile.h -o moc\moc_PacketSourceFile.cpp
%qtdir%\bin\moc.exe ..\common\MDI\PacketSocketQT.h -o moc\moc_PacketSocketQT.cpp
%qtdir%\bin\moc.exe ..\common\GPSReceiver.h -o moc\moc_GPSReceiver.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\MultSettingsDlg.h -o moc\moc_MultSettingsDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\ReceiverSettingsDlg.h -o moc\moc_ReceiverSettingsDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\LatLongEditDlg.h -o moc\moc_LatLongEditDlg.cpp

pause
