# Microsoft Developer Studio Project File - Name="FDRM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FDRM - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak" CFG="FDRM - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "FDRM - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "FDRM - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "FDRM - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /GX /O2 /I "$(QTDIR)\include" /I "../libs" /I "../common/GUI-QT" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ../libs/libfftw.lib ../libs/libfaad.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib ../libs/libqwt.lib /nologo /subsystem:windows /machine:I386 /out:"Release/Dream.exe"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "$(QTDIR)\include" /I "../libs" /I "../common/GUI-QT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib ../libs/libfftw.lib ../libs/libfaad.lib ../libs/libqwt.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/Dream.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "FDRM - Win32 Release"
# Name "FDRM - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat;for;f90"
# Begin Group "Source GUI, App"

# PROP Default_Filter ""
# Begin Group "Do not modify these files!"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\moc\AboutDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AboutDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_DRMPlot.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultColorLED.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\systemevalDlgbase.cpp
# End Source File
# End Group
# Begin Source File

SOURCE="..\common\GUI-QT\DRMPlot.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\fdrmdialog.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\main.cpp"
# End Source File
# Begin Source File

SOURCE=.\MocGUI.bat
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultColorLED.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\systemevalDlg.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Source MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\mlc\BitInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ConvEncoder.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\EnergyDispersal.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\Metric.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\MLC.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\QAMMapping.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ViterbiDecoder.cpp

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /G6

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Source FAC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\FAC\FAC.cpp
# End Source File
# End Group
# Begin Group "Source SDC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\SDC\SDCReceive.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SDC\SDCTransmit.cpp
# End Source File
# End Group
# Begin Group "Source sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\sync\FreqSyncAcq.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\RobModeDetection.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\SyncUsingPil.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSync.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSyncTrack.cpp
# End Source File
# End Group
# Begin Group "Source ChannelEstimation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\chanest\ChanEstTime.cpp
# End Source File
# Begin Source File

SOURCE=..\common\chanest\ChannelEstimation.cpp
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeDecDir.cpp
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeLinear.cpp
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeWiener.cpp
# End Source File
# End Group
# Begin Group "Source Matlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\matlib\MatlibSigProToolbox.cpp
# End Source File
# Begin Source File

SOURCE=..\common\matlib\MatlibStdToolbox.cpp
# End Source File
# End Group
# Begin Group "Source OFDMCellMapping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\ofdmcellmapping\CellMappingTable.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ofdmcellmapping\OFDMCellMapping.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\sourcedecoders\AudioSourceDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\BlockInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\common\drmchannel\ChannelSimulation.cpp
# End Source File
# Begin Source File

SOURCE=..\common\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Data.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DataDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DrmReceiver.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DRMSignalIO.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DrmSimulation.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DrmTransmitter.cpp
# End Source File
# Begin Source File

SOURCE=..\common\InputResample.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MSCMultiplexer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\OFDM.cpp
# End Source File
# Begin Source File

SOURCE=..\common\Parameter.cpp
# End Source File
# Begin Source File

SOURCE=..\common\resample\Resample.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SimulationParameters.cpp
# End Source File
# Begin Source File

SOURCE=.\Source\Sound.cpp
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\SymbolInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\common\TextMessage.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Group "Header GUI, App"

# PROP Default_Filter ""
# Begin Source File

SOURCE="..\common\GUI-QT\DRMPlot.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\fdrmdialog.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultColorLED.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\systemevalDlg.h"
# End Source File
# End Group
# Begin Group "Header MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\mlc\BitInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ConvEncoder.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\EnergyDispersal.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\Metric.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\MLC.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\QAMMapping.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ViterbiDecoder.h
# End Source File
# End Group
# Begin Group "Header FFT"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\fftw\dfftw.h
# End Source File
# Begin Source File

SOURCE=..\common\fftw\drfftw.h
# End Source File
# End Group
# Begin Group "Header Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\tables\TableCarMap.h
# End Source File
# Begin Source File

SOURCE=..\common\tables\TableDRMGlobal.h
# End Source File
# Begin Source File

SOURCE=..\common\tables\TableFAC.h
# End Source File
# Begin Source File

SOURCE=..\common\tables\TableMLC.h
# End Source File
# Begin Source File

SOURCE=..\common\tables\TableQAMMapping.h
# End Source File
# End Group
# Begin Group "Header FAC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\FAC\FAC.h
# End Source File
# End Group
# Begin Group "Header SDC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\SDC\SDC.h
# End Source File
# End Group
# Begin Group "Header sync"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\sync\FreqSyncAcq.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\RobModeDetection.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\SyncUsingPil.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSync.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSyncTrack.h
# End Source File
# End Group
# Begin Group "Header ChannelEstimation"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\chanest\ChanEstTime.h
# End Source File
# Begin Source File

SOURCE=..\common\chanest\ChannelEstimation.h
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeDecDir.h
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeLinear.h
# End Source File
# Begin Source File

SOURCE=..\common\chanest\TimeWiener.h
# End Source File
# End Group
# Begin Group "Header Matlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\matlib\Matlib.h
# End Source File
# Begin Source File

SOURCE=..\common\matlib\MatlibSigProToolbox.h
# End Source File
# Begin Source File

SOURCE=..\common\matlib\MatlibStdToolbox.h
# End Source File
# End Group
# Begin Group "Header OFDMCellMapping"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\ofdmcellmapping\CellMappingTable.h
# End Source File
# Begin Source File

SOURCE=..\common\ofdmcellmapping\OFDMCellMapping.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\sourcedecoders\AudioSourceDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\BlockInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\common\drmchannel\ChannelSimulation.h
# End Source File
# Begin Source File

SOURCE=..\common\CRC.h
# End Source File
# Begin Source File

SOURCE=..\common\Data.h
# End Source File
# Begin Source File

SOURCE=..\common\DataDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\DrmReceiver.h
# End Source File
# Begin Source File

SOURCE=..\common\DRMSignalIO.h
# End Source File
# Begin Source File

SOURCE=..\common\DrmSimulation.h
# End Source File
# Begin Source File

SOURCE=..\common\DrmTransmitter.h
# End Source File
# Begin Source File

SOURCE=..\common\GlobalDefinitions.h
# End Source File
# Begin Source File

SOURCE=..\common\InputResample.h
# End Source File
# Begin Source File

SOURCE=..\common\Modul.h
# End Source File
# Begin Source File

SOURCE=..\common\MSCMultiplexer.h
# End Source File
# Begin Source File

SOURCE=..\common\OFDM.h
# End Source File
# Begin Source File

SOURCE=..\common\Parameter.h
# End Source File
# Begin Source File

SOURCE=..\common\resample\Resample.h
# End Source File
# Begin Source File

SOURCE=..\common\resample\ResampleFilter.h
# End Source File
# Begin Source File

SOURCE=.\Source\Sound.h
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\SymbolInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\TextMessage.h
# End Source File
# Begin Source File

SOURCE=..\common\Vector.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE="..\common\GUI-QT\AboutDlgbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\fdrmdialogbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\README
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\systemevalDlgbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# End Target
# End Project
