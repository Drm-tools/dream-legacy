bin_PROGRAMS = drm
drm_LDADD   = $(ARTSLIBS)

# these need to be generated before the rest can be compiled

BUILT_SOURCES=moc/moc_MultColorLED.cpp \
	moc/moc_DRMPlot.cpp \
	moc/moc_fdrmdialog.cpp moc/moc_fdrmdialogbase.cpp moc/fdrmdialogbase.h moc/fdrmdialogbase.cpp \
	moc/moc_systemevalDlg.cpp  moc/moc_systemevalDlgbase.cpp moc/systemevalDlgbase.h moc/systemevalDlgbase.cpp \
	moc/moc_AboutDlgbase.cpp moc/AboutDlgbase.h moc/AboutDlgbase.cpp

# and should be cleaned by make clean

CLEANFILES=$(BUILT_SOURCES)

drm_SOURCES = ../common/SimulationParameters.cpp \
	../common/CRC.cpp \
	../common/DRMSignalIO.cpp \
	../common/Data.cpp \
	../common/DataDecoder.cpp \
	../common/DrmReceiver.cpp \
	../common/DrmSimulation.cpp \
	../common/DrmTransmitter.cpp \
	../common/InputResample.cpp \
	../common/MSCMultiplexer.cpp \
	../common/OFDM.cpp \
	../common/Parameter.cpp \
	../common/TextMessage.cpp \
	../common/FAC/FAC.cpp \
	../common/matlib/MatlibSigProToolbox.cpp \
	../common/matlib/MatlibStdToolbox.cpp \
	../common/SDC/SDCReceive.cpp \
	../common/SDC/SDCTransmit.cpp \
	../common/chanest/ChannelEstimation.cpp \
	../common/chanest/TimeLinear.cpp \
	../common/chanest/TimeWiener.cpp \
	../common/chanest/ChanEstTime.cpp \
	../common/chanest/TimeDecDir.cpp \
	../common/drmchannel/ChannelSimulation.cpp \
	../common/interleaver/BlockInterleaver.cpp \
	../common/interleaver/SymbolInterleaver.cpp \
	../common/mlc/BitInterleaver.cpp \
	../common/mlc/ConvEncoder.cpp \
	../common/mlc/EnergyDispersal.cpp \
	../common/mlc/MLC.cpp \
	../common/mlc/Metric.cpp \
	../common/mlc/ViterbiDecoder.cpp \
	../common/mlc/QAMMapping.cpp \
	../common/ofdmcellmapping/CellMappingTable.cpp \
	../common/ofdmcellmapping/OFDMCellMapping.cpp \
	../common/resample/Resample.cpp \
	../common/sourcedecoders/AudioSourceDecoder.cpp \
	../common/sync/FreqSyncAcq.cpp \
	../common/sync/TimeSync.cpp \
	../common/sync/RobModeDetection.cpp \
	../common/sync/SyncUsingPil.cpp \
	../common/sync/TimeSyncTrack.cpp \
	source/sound.cpp \
	../common/Buffer.h \
	../common/CRC.h \
	../common/DRMSignalIO.h \
	../common/Data.h \
	../common/DataDecoder.h \
	../common/DrmReceiver.h \
	../common/DrmSimulation.h \
	../common/DrmTransmitter.h \
	../common/GlobalDefinitions.h \
	../common/InputResample.h \
	../common/MSCMultiplexer.h \
	../common/Modul.h \
	../common/OFDM.h \
	../common/Parameter.h \
	../common/TextMessage.h \
	../common/Vector.h \
	../common/FAC/FAC.h \
	../common/matlib/Matlib.h \
	../common/matlib/MatlibSigProToolbox.h \
	../common/matlib/MatlibStdToolbox.h \
	../common/SDC/SDC.h \
	../common/chanest/ChanEstTime.h \
	../common/chanest/ChannelEstimation.h \
	../common/chanest/TimeDecDir.h \
	../common/chanest/TimeLinear.h \
	../common/chanest/TimeWiener.h \
	../common/drmchannel/ChannelSimulation.h \
	../common/interleaver/BlockInterleaver.h \
	../common/interleaver/SymbolInterleaver.h \
	../common/mlc/BitInterleaver.h \
	../common/mlc/ConvEncoder.h \
	../common/mlc/EnergyDispersal.h \
	../common/mlc/MLC.h \
	../common/mlc/Metric.h \
	../common/mlc/QAMMapping.h \
	../common/mlc/ViterbiDecoder.h \
	../common/ofdmcellmapping/CellMappingTable.h \
	../common/ofdmcellmapping/OFDMCellMapping.h \
	../common/resample/Resample.h \
	../common/resample/ResampleFilter.h \
	../common/sourcedecoders/AudioSourceDecoder.h \
	../common/sourcedecoders/faad.h \
	../common/sync/FreqSyncAcq.h \
	../common/sync/RobModeDetection.h \
	../common/sync/SyncUsingPil.h \
	../common/sync/TimeSync.h \
	../common/tables/TableCarMap.h \
	../common/tables/TableDRMGlobal.h \
	../common/tables/TableFAC.h \
	../common/tables/TableMLC.h \
	../common/tables/TableQAMMapping.h \
	../common/GUI-QT/DRMPlot.cpp \
	../common/GUI-QT/fdrmdialog.cpp \
	../common/GUI-QT/main.cpp \
	../common/GUI-QT/MultColorLED.cpp \
	../common/GUI-QT/systemevalDlg.cpp \
	../common/GUI-QT/fdrmdialogbase.ui \
	../common/GUI-QT/systemevalDlgbase.ui \
	../common/GUI-QT/AboutDlgbase.ui \
	../common/GUI-QT/fdrmdialog.h \
	../common/GUI-QT/MultColorLED.h \
	../common/GUI-QT/systemevalDlg.h \
	source/sound.h
	
nodist_drm_SOURCES=$(BUILT_SOURCES)

drm_CXXFLAGS=$(ARTSINCL) $(QWTINCL) -I../common/GUI-QT -I$(QTDIR)/include -DQT_THREAD_SUPPORT -D_REENTRANT -pg

moc/moc_MultColorLED.cpp: ../common/GUI-QT/MultColorLED.h
	$(MOC) ../common/GUI-QT/MultColorLED.h -o moc/moc_MultColorLED.cpp
	
moc/moc_DRMPlot.cpp: ../common/GUI-QT/DRMPlot.h
	$(MOC) ../common/GUI-QT/DRMPlot.h -o moc/moc_DRMPlot.cpp
	

moc/moc_AboutDlgbase.cpp: moc/AboutDlgbase.h
	$(MOC) moc/AboutDlgbase.h -o moc/moc_AboutDlgbase.cpp
	
moc/AboutDlgbase.h: ../common/GUI-QT/AboutDlgbase.ui
	$(UIC) ../common/GUI-QT/AboutDlgbase.ui -o moc/AboutDlgbase.h
	
moc/AboutDlgbase.cpp: ../common/GUI-QT/AboutDlgbase.ui moc/AboutDlgbase.h
	$(UIC) ../common/GUI-QT/AboutDlgbase.ui -i moc/AboutDlgbase.h -o moc/AboutDlgbase.cpp	
	
	
moc/moc_fdrmdialog.cpp: ../common/GUI-QT/fdrmdialog.h
	$(MOC) ../common/GUI-QT/fdrmdialog.h -o moc/moc_fdrmdialog.cpp
	
moc/moc_fdrmdialogbase.cpp: moc/fdrmdialogbase.h
	$(MOC) moc/fdrmdialogbase.h -o moc/moc_fdrmdialogbase.cpp

moc/fdrmdialogbase.h: ../common/GUI-QT/fdrmdialogbase.ui
	$(UIC) ../common/GUI-QT/fdrmdialogbase.ui -o moc/fdrmdialogbase.h

moc/fdrmdialogbase.cpp: ../common/GUI-QT/fdrmdialogbase.ui moc/fdrmdialogbase.h
	$(UIC) ../common/GUI-QT/fdrmdialogbase.ui -i moc/fdrmdialogbase.h -o moc/fdrmdialogbase.cpp
	

moc/moc_systemevalDlg.cpp: ../common/GUI-QT/systemevalDlg.h
	$(MOC) ../common/GUI-QT/systemevalDlg.h -o moc/moc_systemevalDlg.cpp
		
moc/moc_systemevalDlgbase.cpp: moc/systemevalDlgbase.h
	$(MOC) moc/systemevalDlgbase.h -o moc/moc_systemevalDlgbase.cpp
	
moc/systemevalDlgbase.h: ../common/GUI-QT/systemevalDlgbase.ui
	$(UIC) ../common/GUI-QT/systemevalDlgbase.ui -o moc/systemevalDlgbase.h

moc/systemevalDlgbase.cpp: ../common/GUI-QT/systemevalDlgbase.ui moc/systemevalDlgbase.h
	$(UIC) ../common/GUI-QT/systemevalDlgbase.ui -i moc/systemevalDlgbase.h -o moc/systemevalDlgbase.cpp
	

	
