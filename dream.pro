TEMPLATE = app
TARGET = dream
QT += network \
    xml
CONFIG += qt \
    warn_on \
    thread \
    hamlib \
    portaudio
INCLUDEPATH += libs
INCLUDEPATH += common/GUI-QT
VPATH += common/GUI-QT
LIBS += -Llibs
DEFINES += HAVE_LIBFAAC \
    HAVE_LIBFAAD
FORMS += DRMMainWindow.ui \
    FMMainWindow.ui \
    AnalogMainWindow.ui \
    TransmitterMainWindow.ui \
    common/GUI-QT/receiverinputwidget.ui \
    common/GUI-QT/DIoutputSelector.ui
FORMS += AMSSDlg.ui \
    SystemEvalDlg.ui \
    JLViewer.ui \
    BWSViewer.ui \
    SlideShowViewer.ui
FORMS += LiveScheduleDlg.ui \
    StationsDlg.ui \
    EPGDlg.ui \
    AboutDlg.ui
FORMS += ReceiverSettingsDlg.ui \
    LatLongEditDlg.ui
RCC_DIR = common/GUI-QT/res
RESOURCES += common/GUI-QT/res/icons.qrc
TRANSLATIONS = common/GUI-QT/languages/drm.fr.ts \
    common/GUI-QT/languages/dreamtr.es.ts
UI_DIR = moc
MOC_DIR = moc
debug:OBJECTS_DIR = Debug
else:OBJECTS_DIR = Release
macx {
    RC_FILE = common/GUI-QT/res/macicons.icns
    INCLUDEPATH += /Developer/dream/include
    INCLUDEPATH += /Developer/qwt-5.1.1/include
    LIBS += -L/Developer/dream/lib
    LIBS += -L/Developer/qwt-5.1.1/lib \
	-lqwt
    LIBS += -framework \
	CoreFoundation \
	-framework \
	CoreServices
    LIBS += -framework \
	CoreAudio \
	-framework \
	AudioToolbox \
	-framework \
	AudioUnit
}
unix {
    LIBS += -lsndfile \
	-lpcap \
	-lz \
	-lfaac_drm \
	-lfaad_drm \
	-lrfftw \
	-lfftw
    DEFINES += HAVE_RFFTW_H \
	HAVE_LIBPCAP
    DEFINES += HAVE_DLFCN_H \
	HAVE_MEMORY_H \
	HAVE_STDINT_H \
	HAVE_STDLIB_H
    DEFINES += HAVE_STRINGS_H \
	HAVE_STRING_H \
	STDC_HEADERS
    DEFINES += HAVE_INTTYPES_H \
	HAVE_STDINT_H \
	HAVE_SYS_STAT_H \
	HAVE_SYS_TYPES_H \
	HAVE_UNISTD_H
    SOURCES += linux/source/Pacer.cpp
    !macx {
	INCLUDEPATH += /usr/include/qwt-qt4
	LIBS += -lqwt-qt4
	INCLUDEPATH += linux
	LIBS += -lrt
	SOURCES += linux/source/shmsoundin.cpp \
	    linux/source/pa_shm_ringbuffer.c
	HEADERS += linux/source/shmsoundin.h \
	    linux/source/pa_shm_ringbuffer.h
    }
}
win32 {
    DEFINES -= UNICODE
    SOURCES += windows/Source/Pacer.cpp
    INCLUDEPATH += libs/qwt
    win32-g++ {
	DEFINES += HAVE_STDINT_H \
	    HAVE_STDLIB_H \
	    HAVE_LIBPCAP __INTERLOCKED_DECLARED
	LIBS += -lsndfile \
	    -lz \
	    -lfaac \
	    -lfaad \
	    -lrfftw \
	    -lfftw \
	    -lqwt5
	LIBS += -lwpcap \
	    -lstdc++
	LIBS += -lsetupapi \
	    -lws2_32
    }
    else {
	DEFINES += NOMINMAX
	TEMPLATE = vcapp
	LIBS += libsndfile-1.lib \
	    zdll.lib \
	    qwt5.lib
	LIBS += libfaac.lib \
	    libfaad.lib
	LIBS += libfftw.lib \
	    setupapi.lib \
	    ws2_32.lib
	QMAKE_LFLAGS_RELEASE += /NODEFAULTLIB:"MSVCRTD, LIBCMT"
	QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrtd.lib
    }
}
hamlib {
    DEFINES += HAVE_LIBHAMLIB \
	HAVE_RIG_PARSE_MODE
    HEADERS += common/util/Hamlib.h
    SOURCES += common/util/Hamlib.cpp common/util/rigclass.cc
    unix:LIBS += -lhamlib
    macx:LIBS += -framework \
	IOKit
    win32 {
	win32-g++:LIBS += -lhamlib
	else:LIBS += libhamlib.lib
    }
}
portaudio {
    DEFINES += USE_PORTAUDIO
    HEADERS += common/sound/pa_ringbuffer.h \
	common/sound/drm_portaudio.h
    SOURCES += common/sound/drm_portaudio.cpp \
	common/sound/pa_ringbuffer.c
    win32-g++:LIBS += ./libs/PortAudio.dll
    else:LIBS += -lportaudio
}
winmm {
    CONFIG -= portaudio
    HEADERS += windows/Source/Sound.h \
	windows/Source/SoundWin.h
    SOURCES += windows/Source/Sound.cpp
    LIBS += -lwinmm
}
alsa {
    CONFIG -= portaudio
    DEFINES += USE_ALSA
    HEADERS += linux/source/soundcommon.h \
	linux/source/soundin.h \
	linux/source/soundout.h
    SOURCES += linux/source/alsa.cpp \
	linux/source/soundcommon.cpp
    LIBS += -lasound
}
jack {
    CONFIG -= portaudio
    DEFINES += USE_JACK
    HEADERS += linux/source/soundcommon.h \
	linux/source/soundin.h \
	linux/source/soundout.h
    SOURCES += linux/source/jack.cpp \
	linux/source/soundcommon.cpp
    LIBS += -ljack
}
HEADERS += common/Measurements.h \
    common/AMDemodulation.h \
    common/AMSSDemodulation.h \
    common/chanest/ChanEstTime.h \
    common/chanest/ChannelEstimation.h \
    common/chanest/IdealChannelEstimation.h \
    common/chanest/TimeLinear.h \
    common/chanest/TimeWiener.h \
    common/datadecoding/DABMOT.h \
    common/datadecoding/DataDecoder.h \
    common/datadecoding/DataApplication.h \
    common/datadecoding/DataEncoder.h \
    common/datadecoding/EPGDecoder.h \
    common/datadecoding/epg/EPG.h \
    common/datadecoding/epg/epgdec.h \
    common/datadecoding/epg/epgutil.h \
    common/datadecoding/journaline/NML.h \
    common/datadecoding/journaline/Splitter.h \
    common/datadecoding/journaline/cpplog.h \
    common/datadecoding/journaline/crc_8_16.h \
    common/datadecoding/journaline/dabdatagroupdecoder.h \
    common/datadecoding/journaline/dabdgdec_impl.h \
    common/datadecoding/journaline/log.h \
    common/datadecoding/journaline/newsobject.h \
    common/datadecoding/journaline/newssvcdec.h \
    common/datadecoding/journaline/newssvcdec_impl.h \
    common/datadecoding/Journaline.h \
    common/datadecoding/MOTSlideShow.h \
    common/DataIO.h \
    common/drmchannel/ChannelSimulation.h \
    common/ReceptLog.h \
    common/ServiceInformation.h \
    common/DrmReceiver.h \
    common/DRMSignalIO.h \
    common/DrmSimulation.h \
    common/DrmTransmitter.h \
    common/DrmEncoder.h \
    common/DrmModulator.h \
    common/FAC/FAC.h \
    common/SigProc.h \
    common/GlobalDefinitions.h \
    common/GPSData.h \
    common/GPSReceiver.h \
    common/GUI-QT/RxApp.h \
    common/GUI-QT/DialogUtil.h \
    common/GUI-QT/DRMPlot.h \
    common/GUI-QT/EPGDlg.h \
    common/GUI-QT/AnalogMainWindow.h \
    common/GUI-QT/DRMMainWindow.h \
    common/GUI-QT/FMMainWindow.h \
    common/GUI-QT/TransmitterMainWindow.h \
    common/GUI-QT/LiveScheduleDlg.h \
    common/GUI-QT/LatLongEditDlg.h \
    common/GUI-QT/MultColorLED.h \
    common/GUI-QT/JLViewer.h \
    common/GUI-QT/SlideShowViewer.h \
    common/GUI-QT/BWSViewer.h \
    common/GUI-QT/ReceiverSettingsDlg.h \
    common/GUI-QT/Loghelper.h \
    common/GUI-QT/StationsDlg.h \
    common/GUI-QT/ScheduleModel.h \
    common/GUI-QT/SystemEvalDlg.h \
    common/GUI-QT/jlbrowser.h \
    common/GUI-QT/bwsbrowser.h \
    common/InputResample.h \
    common/interleaver/BlockInterleaver.h \
    common/interleaver/SymbolInterleaver.h \
    common/IQInputFilter.h \
    common/matlib/Matlib.h \
    common/matlib/MatlibSigProToolbox.h \
    common/matlib/MatlibStdToolbox.h \
    common/MDI/AFPacketGenerator.h \
    common/MDI/MDIDecode.h \
    common/MDI/MDIDefinitions.h \
    common/MDI/MDIInBuffer.h \
    common/MDI/MDIRSCI.h \
    common/MDI/MDITagItemDecoders.h \
    common/MDI/MDITagItems.h \
    common/MDI/PacketInOut.h \
    common/MDI/PacketSinkFile.h \
    common/MDI/PacketSourceFile.h \
    common/MDI/PacketSocketNull.h \
    common/MDI/PacketSocketQT.h \
    common/MDI/Pft.h \
    common/MDI/RCITagItems.h \
    common/MDI/RSCITagItemDecoders.h \
    common/MDI/RSISubscriber.h \
    common/MDI/TagItemDecoder.h \
    common/MDI/TagPacketDecoder.h \
    common/MDI/TagPacketDecoderMDI.h \
    common/MDI/TagPacketDecoderRSCIControl.h \
    common/MDI/TagPacketGenerator.h \
    common/mlc/BitInterleaver.h \
    common/mlc/ChannelCode.h \
    common/mlc/ConvEncoder.h \
    common/mlc/EnergyDispersal.h \
    common/mlc/Metric.h \
    common/mlc/MLC.h \
    common/mlc/QAMMapping.h \
    common/mlc/ViterbiDecoder.h \
    common/MSCMultiplexer.h \
    common/OFDM.h \
    common/ofdmcellmapping/CellMappingTable.h \
    common/ofdmcellmapping/OFDMCellMapping.h \
    common/Parameter.h \
    common/resample/Resample.h \
    common/resample/ResampleFilter.h \
    common/SDC/SDC.h \
    common/ReceiverInterface.h \
    common/selectioninterface.h \
    common/soundinterface.h \
    common/sound.h \
    common/sound/soundnull.h \
    common/sound/soundfile.h \
    common/sourcedecoders/AudioSourceDecoder.h \
    common/sourcedecoders/AudioSourceEncoder.h \
    common/sync/FreqSyncAcq.h \
    common/sync/SyncUsingPil.h \
    common/sync/TimeSync.h \
    common/sync/TimeSyncFilter.h \
    common/sync/TimeSyncTrack.h \
    common/tables/TableAMSS.h \
    common/tables/TableCarMap.h \
    common/tables/TableDRMGlobal.h \
    common/tables/TableFAC.h \
    common/tables/TableMLC.h \
    common/tables/TableQAMMapping.h \
    common/tables/TableStations.h \
    common/TextMessage.h \
    common/util/AudioFile.h \
    common/util/Buffer.h \
    common/util/CRC.h \
    common/util/LogPrint.h \
    common/util/Modul.h \
    common/util/ReceiverModul.h \
    common/util/ReceiverModul_impl.h \
    common/util/SimulationModul.h \
    common/util/SimulationModul_impl.h \
    common/util/TransmitterModul.h \
    common/util/TransmitterModul_impl.h \
    common/util/Pacer.h \
    common/util/Reassemble.h \
    common/util/Settings.h \
    common/util/Utilities.h \
    common/util/Vector.h \
    common/Version.h \
    common/GUI-QT/receiverinputwidget.h \
    common/GUI-QT/DIoutputSelector.h
SOURCES += common/Measurements.cpp \
    common/AMDemodulation.cpp \
    common/AMSSDemodulation.cpp \
    common/chanest/ChanEstTime.cpp \
    common/chanest/ChannelEstimation.cpp \
    common/chanest/IdealChannelEstimation.cpp \
    common/chanest/TimeLinear.cpp \
    common/chanest/TimeWiener.cpp \
    common/datadecoding/DABMOT.cpp \
    common/datadecoding/DataDecoder.cpp \
    common/datadecoding/DataEncoder.cpp \
    common/datadecoding/EPGDecoder.cpp \
    common/datadecoding/epg/EPG.cpp \
    common/datadecoding/epg/epgdec.cpp \
    common/datadecoding/epg/epgutil.cpp \
    common/datadecoding/journaline/NML.cpp \
    common/datadecoding/journaline/dabdgdec_impl.c \
    common/datadecoding/journaline/Splitter.cpp \
    common/datadecoding/journaline/newsobject.cpp \
    common/datadecoding/journaline/newssvcdec_impl.cpp \
    common/datadecoding/journaline/crc_8_16.c \
    common/datadecoding/journaline/log.c \
    common/datadecoding/Journaline.cpp \
    common/datadecoding/MOTSlideShow.cpp \
    common/DataIO.cpp \
    common/drmchannel/ChannelSimulation.cpp \
    common/ReceptLog.cpp \
    common/ServiceInformation.cpp \
    common/DrmReceiver.cpp \
    common/DRMSignalIO.cpp \
    common/DrmSimulation.cpp \
    common/DrmTransmitter.cpp \
    common/DrmEncoder.cpp \
    common/DrmModulator.cpp \
    common/FAC/FAC.cpp \
    common/GPSData.cpp \
    common/GPSReceiver.cpp \
    common/GUI-QT/RxApp.cpp \
    common/GUI-QT/DialogUtil.cpp \
    common/GUI-QT/DRMPlot.cpp \
    common/GUI-QT/EPGDlg.cpp \
    common/GUI-QT/AnalogMainWindow.cpp \
    common/GUI-QT/DRMMainWindow.cpp \
    common/GUI-QT/FMMainWindow.cpp \
    common/GUI-QT/TransmitterMainWindow.cpp \
    common/GUI-QT/LiveScheduleDlg.cpp \
    common/GUI-QT/main.cpp \
    common/GUI-QT/LatLongEditDlg.cpp \
    common/GUI-QT/MultColorLED.cpp \
    common/GUI-QT/JLViewer.cpp \
    common/GUI-QT/SlideShowViewer.cpp \
    common/GUI-QT/BWSViewer.cpp \
    common/GUI-QT/ReceiverSettingsDlg.cpp \
    common/GUI-QT/Loghelper.cpp \
    common/GUI-QT/ScheduleModel.cpp \
    common/GUI-QT/StationsDlg.cpp \
    common/GUI-QT/SystemEvalDlg.cpp \
    common/GUI-QT/jlbrowser.cpp \
    common/GUI-QT/bwsbrowser.cpp \
    common/InputResample.cpp \
    common/interleaver/BlockInterleaver.cpp \
    common/interleaver/SymbolInterleaver.cpp \
    common/IQInputFilter.cpp \
    common/matlib/MatlibSigProToolbox.cpp \
    common/matlib/MatlibStdToolbox.cpp \
    common/MDI/AFPacketGenerator.cpp \
    common/MDI/MDIDecode.cpp \
    common/MDI/MDIInBuffer.cpp \
    common/MDI/MDIRSCI.cpp \
    common/MDI/MDITagItemDecoders.cpp \
    common/MDI/MDITagItems.cpp \
    common/MDI/PacketSinkFile.cpp \
    common/MDI/PacketSourceFile.cpp \
    common/MDI/PacketSocketNull.cpp \
    common/MDI/PacketSocketQT.cpp \
    common/MDI/Pft.cpp \
    common/MDI/RCITagItems.cpp \
    common/MDI/RSCITagItemDecoders.cpp \
    common/MDI/RSISubscriber.cpp \
    common/MDI/TagPacketDecoder.cpp \
    common/MDI/TagPacketDecoderMDI.cpp \
    common/MDI/TagPacketDecoderRSCIControl.cpp \
    common/MDI/TagPacketGenerator.cpp \
    common/mlc/BitInterleaver.cpp \
    common/mlc/ChannelCode.cpp \
    common/mlc/ConvEncoder.cpp \
    common/mlc/EnergyDispersal.cpp \
    common/mlc/Metric.cpp \
    common/mlc/MLC.cpp \
    common/mlc/QAMMapping.cpp \
    common/mlc/TrellisUpdateMMX.cpp \
    common/mlc/TrellisUpdateSSE2.cpp \
    common/mlc/ViterbiDecoder.cpp \
    common/MSCMultiplexer.cpp \
    common/OFDM.cpp \
    common/ofdmcellmapping/CellMappingTable.cpp \
    common/ofdmcellmapping/OFDMCellMapping.cpp \
    common/Parameter.cpp \
    common/resample/Resample.cpp \
    common/resample/ResampleFilter.cpp \
    common/SDC/SDCReceive.cpp \
    common/SDC/SDCTransmit.cpp \
    common/SimulationParameters.cpp \
    common/sound/soundfile.cpp \
    common/sourcedecoders/AudioSourceDecoder.cpp \
    common/sourcedecoders/AudioSourceEncoder.cpp \
    common/sync/FreqSyncAcq.cpp \
    common/sync/SyncUsingPil.cpp \
    common/sync/TimeSync.cpp \
    common/sync/TimeSyncFilter.cpp \
    common/sync/TimeSyncTrack.cpp \
    common/tables/TableCarMap.cpp \
    common/tables/TableFAC.cpp \
    common/tables/TableStations.cpp \
    common/TextMessage.cpp \
    common/util/CRC.cpp \
    common/util/LogPrint.cpp \
    common/util/Reassemble.cpp \
    common/util/Settings.cpp \
    common/util/Utilities.cpp \
    common/Version.cpp \
    common/GUI-QT/receiverinputwidget.cpp \
    common/GUI-QT/DIoutputSelector.cpp
