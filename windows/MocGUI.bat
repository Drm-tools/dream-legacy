rem/******************************************************************************\
rem * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
rem * Copyright (c) 2001
rem *
rem * Author(s):
rem *	Volker Fischer
rem *
rem * Description:
rem *	Script for compiling the QT resources under Windows (MOCing and UICing)
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


rem .h --------------
%qtdir%\bin\moc.exe ..\common\GUI-QT\MultColorLED.h -o moc\moc_MultColorLED.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\systemevalDlg.h -o moc\moc_systemevalDlg.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\fdrmdialog.h -o moc\moc_fdrmdialog.cpp
%qtdir%\bin\moc.exe ..\common\GUI-QT\DRMPlot.h -o moc\moc_DRMPlot.cpp


rem .ui -------------
%qtdir%\bin\uic.exe ..\common\GUI-QT\fdrmdialogbase.ui -o moc\fdrmdialogbase.h  
%qtdir%\bin\uic.exe ..\common\GUI-QT\fdrmdialogbase.ui -i fdrmdialogbase.h -o moc\fdrmdialogbase.cpp  
%qtdir%\bin\moc.exe moc\fdrmdialogbase.h -o moc\moc_fdrmdialogbase.cpp

%qtdir%\bin\uic.exe ..\common\GUI-QT\systemevalDlgbase.ui -o moc\systemevalDlgbase.h  
%qtdir%\bin\uic.exe ..\common\GUI-QT\systemevalDlgbase.ui -i systemevalDlgbase.h -o moc\systemevalDlgbase.cpp  
%qtdir%\bin\moc.exe moc\systemevalDlgbase.h -o moc\moc_systemevalDlgbase.cpp

%qtdir%\bin\uic.exe ..\common\GUI-QT\AboutDlgbase.ui -o moc\AboutDlgbase.h  
%qtdir%\bin\uic.exe ..\common\GUI-QT\AboutDlgbase.ui -i AboutDlgbase.h -o moc\AboutDlgbase.cpp  
%qtdir%\bin\moc.exe moc\AboutDlgbase.h -o moc\moc_AboutDlgbase.cpp

