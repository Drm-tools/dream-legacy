# Microsoft Developer Studio Project File - Name="FDRM" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=FDRM - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FDRM.mak" CFG="FDRM - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "FDRM - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "FDRM - Win32 Debug" (based on "Win32 (x86) Application")
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
# ADD CPP /nologo /MD /GX /O2 /I "$(QTDIR)\include" /I "../libs" /I "../common/GUI-QT" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "FREEIMAGE_LIB" /YX /FD /c
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
# ADD LINK32 libfaac.lib libhamlib.lib FreeImage.lib libfftw.lib libfaad.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib libqwt.lib libfhgjournaline.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib:"MSVCRTD" /out:"Release/Dream.exe" /libpath:"../libs"
# SUBTRACT LINK32 /profile /debug /nodefaultlib

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
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "$(QTDIR)\include" /I "../libs" /I "../common/GUI-QT" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "QT_DLL" /D "QT_THREAD_SUPPORT" /D "FREEIMAGE_LIB" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libfaac.lib libhamlib.lib FreeImage.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib wsock32.lib winspool.lib winmm.lib $(QTDIR)\lib\qt-mt230nc.lib $(QTDIR)\lib\qtmain.lib libfftw.lib libfaad.lib libqwt.lib libfhgjournaline.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/Dream.exe" /pdbtype:sept /libpath:"../libs"

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

SOURCE=.\moc\AMSSDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\AnalogDemDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\EPGDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\LiveScheduleDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\MultSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\GeneralSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AboutDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AMSSDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AnalogDemDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_AnalogDemDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_DialogUtil.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_DRMPlot.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_EPGDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_EPGDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_fdrmdialogbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_LiveScheduleDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_LiveScheduleDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultSettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_GeneralSettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_GeneralSettingsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultColorLED.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultimediaDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_MultimediaDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_PacketSocketQT.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_StationsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_StationsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_systemevalDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_TransmDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_TransmDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\MultimediaDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\StationsDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\systemevalDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\TransmDlgbase.cpp
# End Source File
# Begin Source File

SOURCE=.\moc\moc_GPSReceiver.cpp
# End Source File
# End Group
# Begin Source File

SOURCE="..\common\GUI-QT\AnalogDemDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\DialogUtil.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\DRMPlot.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\EPGDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\fdrmdialog.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\LiveScheduleDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultSettingsDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\GeneralSettingsDlg.cpp"
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

SOURCE="..\common\GUI-QT\MultimediaDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\StationsDlg.cpp"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\systemevalDlg.cpp"

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /MD

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\TransmDlg.cpp"
# End Source File
# End Group
# Begin Group "Source MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\mlc\BitInterleaver.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ChannelCode.cpp
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

SOURCE=..\common\mlc\TrellisUpdateMMX.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\TrellisUpdateSSE2.cpp
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ViterbiDecoder.cpp

!IF  "$(CFG)" == "FDRM - Win32 Release"

# ADD CPP /G6

!ELSEIF  "$(CFG)" == "FDRM - Win32 Debug"

!ENDIF 

# End Source File
# End Group
# Begin Group "Source Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\tables\TableFAC.cpp
# End Source File
# Begin Source File

SOURCE=..\common\tables\TableCarMap.cpp
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

SOURCE=..\common\sync\SyncUsingPil.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSync.cpp
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSyncFilter.cpp
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

SOURCE=..\common\chanest\IdealChannelEstimation.cpp
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
# Begin Group "Source datadecoding"

# PROP Default_Filter ".c .cpp"
# Begin Group "epg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\datadecoding\epg\EPG.cpp
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\epg\epgdec.cpp
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\epg\epgutil.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\datadecoding\DABMOT.cpp
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\DataDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\Journaline.cpp
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\MOTSlideShow.cpp
# End Source File
# End Group
# Begin Group "Source Utilities"

# PROP Default_Filter ".c .cpp"
# Begin Source File

SOURCE=..\common\util\CRC.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util\LogPrint.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util\Settings.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util\Utilities.cpp
# End Source File
# Begin Source File

SOURCE=..\common\util\Reassemble.cpp
# End Source File
# End Group
# Begin Group "Source MDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\MDI\MDIDecode.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDIRSCI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDIInBuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDITagItemDecoders.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDITagItems.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RCITagItems.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoderMDI.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSocketNull.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSocketQT.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSinkFile.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RSISubscriber.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RSCITagItemDecoders.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoder.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoderRSCIControl.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketGenerator.cpp
# End Source File
# Begin Source File

SOURCE=..\common\MDI\Pft.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\AMDemodulation.cpp
# End Source File
# Begin Source File

SOURCE=..\common\AMSSDemodulation.cpp
# End Source File
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

SOURCE=..\common\Version.cpp
# End Source File
# Begin Source File

SOURCE=..\common\DataIO.cpp
# End Source File
# Begin Source File

SOURCE=..\common\GPSReceiver.cpp
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

SOURCE=..\common\resample\ResampleFilter.cpp
# End Source File
# Begin Source File

SOURCE=..\common\SimulationParameters.cpp
# End Source File
# Begin Source File

SOURCE=..\common\IQInputFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\Source\Sound.cpp
# End Source File
# Begin Source File

SOURCE=.\Source\Pacer.cpp
# End Source File
# Begin Source File

SOURCE=..\common\audiofilein.cpp
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

SOURCE="..\common\GUI-QT\AnalogDemDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\DialogUtil.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\DRMPlot.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\EPGDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\fdrmdialog.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\LiveScheduleDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultSettingsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\GeneralSettingsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultColorLED.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\MultimediaDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\StationsDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\systemevalDlg.h"
# End Source File
# Begin Source File

SOURCE="..\common\GUI-QT\TransmDlg.h"
# End Source File
# End Group
# Begin Group "Header MLC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\mlc\BitInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\mlc\ChannelCode.h
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
# Begin Group "Header Tables"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\tables\TableAMSS.h
# End Source File
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

SOURCE=..\common\sync\SyncUsingPil.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSync.h
# End Source File
# Begin Source File

SOURCE=..\common\sync\TimeSyncFilter.h
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

SOURCE=..\common\chanest\IdealChannelEstimation.h
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
# Begin Group "Header datadecoding"

# PROP Default_Filter ".h"
# Begin Group "Header epg"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\datadecoding\epg\EPG.h
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\epg\epgdec.h
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\epg\epgutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\datadecoding\DABMOT.h
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\DataDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\Journaline.h
# End Source File
# Begin Source File

SOURCE=..\common\datadecoding\MOTSlideShow.h
# End Source File
# End Group
# Begin Group "Header Utilities"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=..\common\util\AudioFile.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Buffer.h
# End Source File
# Begin Source File

SOURCE=..\common\util\CRC.h
# End Source File
# Begin Source File

SOURCE=..\common\util\LogPrint.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Modul.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Settings.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Utilities.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Reassemble.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Vector.h
# End Source File
# End Group
# Begin Group "Header MDI"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\MDI\MDIRSCI.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDIDecode.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDIDefinitions.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDIInBuffer.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDITagItemDecoders.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\MDITagItems.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RCITagItems.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoderMDI.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketInOut.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSocketNull.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSocketQT.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\PacketSinkFile.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RSISubscriber.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\RSCITagItemDecoders.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagItemDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketDecoderRSCIControl.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\TagPacketGenerator.h
# End Source File
# Begin Source File

SOURCE=..\common\MDI\Pft.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\common\AMDemodulation.h
# End Source File
# Begin Source File

SOURCE=..\common\AMSSDemodulation.h
# End Source File
# Begin Source File

SOURCE=..\common\sourcedecoders\AudioSourceDecoder.h
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\BlockInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\drmchannel\ChannelSimulation.h
# End Source File
# Begin Source File

SOURCE=..\common\DataIO.h
# End Source File
# Begin Source File

SOURCE=..\common\GPSReceiver.h
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

SOURCE=..\common\Version.h
# End Source File
# Begin Source File

SOURCE=..\common\GlobalDefinitions.h
# End Source File
# Begin Source File

SOURCE=..\common\InputResample.h
# End Source File
# Begin Source File

SOURCE=..\common\IQInputFilter.h
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

SOURCE=..\common\selectioninterface.h
# End Source File
# Begin Source File

SOURCE=..\common\util\Pacer.h
# End Source File
# Begin Source File

SOURCE=..\common\soundinterface.h
# End Source File
# Begin Source File

SOURCE=..\common\soundnull.h
# End Source File
# Begin Source File

SOURCE=.\Source\Sound.h
# End Source File
# Begin Source File

SOURCE=..\common\audiofilein.h
# End Source File
# Begin Source File

SOURCE=..\common\interleaver\SymbolInterleaver.h
# End Source File
# Begin Source File

SOURCE=..\common\TextMessage.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE="..\common\GUI-QT\res\MainIcon.ico"
# End Source File
# End Group
# Begin Source File

SOURCE="..\common\GUI-QT\AboutDlgbase.ui"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\FDRM.rc
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
