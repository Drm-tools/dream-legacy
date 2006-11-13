bin_PROGRAMS = drm


drmwithoutqt_SOURCES = \
	../common/Version.cpp \
	../common/SimulationParameters.cpp \
	../common/DRMSignalIO.cpp \
	../common/DataIO.cpp \
	../common/DrmReceiver.cpp \
	../common/DrmSimulation.cpp \
	../common/DrmTransmitter.cpp \
	../common/InputResample.cpp \
	../common/MSCMultiplexer.cpp \
	../common/OFDM.cpp \
	../common/Parameter.cpp \
	../common/TextMessage.cpp \
	../common/AMDemodulation.cpp \
	../common/AMSSDemodulation.cpp \
	../common/datadecoding/DataDecoder.cpp \
	../common/datadecoding/DABMOT.cpp \
	../common/datadecoding/MOTSlideShow.cpp \
	../common/datadecoding/Journaline.cpp \
	../common/datadecoding/epg/epgutil.cpp \
	../common/FAC/FAC.cpp \
	../common/matlib/MatlibSigProToolbox.cpp \
	../common/matlib/MatlibStdToolbox.cpp \
	../common/util/CRC.cpp \
	../common/util/Settings.cpp \
	../common/util/Utilities.cpp \
	../common/util/LogPrint.cpp \
	../common/util/Reassemble.cpp \
	../common/SDC/SDCReceive.cpp \
	../common/SDC/SDCTransmit.cpp \
	../common/chanest/ChannelEstimation.cpp \
	../common/chanest/TimeLinear.cpp \
	../common/chanest/TimeWiener.cpp \
	../common/chanest/ChanEstTime.cpp \
	../common/chanest/IdealChannelEstimation.cpp \
	../common/drmchannel/ChannelSimulation.cpp \
	../common/interleaver/BlockInterleaver.cpp \
	../common/interleaver/SymbolInterleaver.cpp \
	../common/mlc/BitInterleaver.cpp \
	../common/mlc/ConvEncoder.cpp \
	../common/mlc/EnergyDispersal.cpp \
	../common/mlc/MLC.cpp \
	../common/mlc/Metric.cpp \
	../common/mlc/ViterbiDecoder.cpp \
	../common/mlc/TrellisUpdateMMX.cpp \
	../common/mlc/TrellisUpdateSSE2.cpp \
	../common/mlc/QAMMapping.cpp \
	../common/mlc/ChannelCode.cpp \
	../common/MDI/MDIRSCI.cpp \
	../common/MDI/MDIDecode.cpp \
	../common/MDI/MDIInBuffer.cpp \
	../common/MDI/MDITagItemDecoders.cpp \
	../common/MDI/MDITagItems.cpp \
	../common/MDI/PacketSocketNull.cpp \
	../common/MDI/RSCITagItemDecoders.cpp \
	../common/MDI/TagPacketDecoder.cpp \
	../common/MDI/TagPacketDecoderMDI.cpp \
	../common/MDI/TagPacketDecoderRSCIControl.cpp \
	../common/MDI/TagPacketGenerator.cpp \
	../common/MDI/Pft.cpp \
	../common/ofdmcellmapping/CellMappingTable.cpp \
	../common/ofdmcellmapping/OFDMCellMapping.cpp \
	../common/resample/Resample.cpp \
	../common/sourcedecoders/AudioSourceDecoder.cpp \
	../common/sync/FreqSyncAcq.cpp \
	../common/sync/TimeSync.cpp \
	../common/sync/SyncUsingPil.cpp \
	../common/sync/TimeSyncTrack.cpp \
	../common/GUI-QT/main.cpp \
	source/soundcommon.cpp \
	source/soundin.cpp \
	source/soundout.cpp \
	../common/Version.h \
	../common/DRMSignalIO.h \
	../common/DataIO.h \
	../common/DrmReceiver.h \
	../common/DrmSimulation.h \
	../common/DrmTransmitter.h \
	../common/GlobalDefinitions.h \
	../common/InputResample.h \
	../common/MSCMultiplexer.h \
	../common/OFDM.h \
	../common/Parameter.h \
	../common/TextMessage.h \
	../common/AMDemodulation.h \
	../common/AMSSDemodulation.h \
	../common/datadecoding/DataDecoder.h \
	../common/datadecoding/DABMOT.h \
	../common/datadecoding/MOTSlideShow.h \
	../common/datadecoding/Journaline.h \
	../common/datadecoding/epg/epgutil.h \
	../common/FAC/FAC.h \
	../common/matlib/Matlib.h \
	../common/matlib/MatlibSigProToolbox.h \
	../common/matlib/MatlibStdToolbox.h \
	../common/util/Modul.h \
	../common/util/Vector.h \
	../common/util/Buffer.h \
	../common/util/CRC.h \
	../common/util/AudioFile.h \
	../common/util/Settings.h \
	../common/util/Utilities.h \
	../common/util/LogPrint.h \
	../common/util/Reassemble.h \
	../common/SDC/SDC.h \
	../common/IQInputFilter.h \
	../common/chanest/ChanEstTime.h \
	../common/chanest/ChannelEstimation.h \
	../common/chanest/TimeLinear.h \
	../common/chanest/TimeWiener.h \
	../common/chanest/IdealChannelEstimation.h \
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
	../common/mlc/ChannelCode.h \
	../common/MDI/MDIRSCI.h \
	../common/MDI/MDIDefinitions.h \
	../common/MDI/MDIDecode.h \
	../common/MDI/MDIInBuffer.h \
	../common/MDI/MDITagItemDecoders.h \
	../common/MDI/MDITagItems.h \
	../common/MDI/TagPacketDecoderMDI.h \
	../common/MDI/PacketInOut.h \
	../common/MDI/PacketSocketNull.h \
	../common/MDI/RSCITagItemDecoders.h \
	../common/MDI/TagItemDecoder.h \
	../common/MDI/TagPacketDecoder.h \
	../common/MDI/TagPacketDecoderRSCIControl.h \
	../common/MDI/TagPacketGenerator.h \
	../common/MDI/Pft.h \
	../common/ofdmcellmapping/CellMappingTable.h \
	../common/ofdmcellmapping/OFDMCellMapping.h \
	../common/resample/Resample.h \
	../common/resample/ResampleFilter.h \
	../common/sourcedecoders/AudioSourceDecoder.h \
	../common/sync/FreqSyncAcq.h \
	../common/sync/SyncUsingPil.h \
	../common/sync/TimeSync.h \
	../common/sync/TimeSyncFilter.h \
	../common/sync/TimeSyncTrack.h \
	../common/tables/TableAMSS.h \
	../common/tables/TableCarMap.h \
	../common/tables/TableDRMGlobal.h \
	../common/tables/TableFAC.h \
	../common/tables/TableMLC.h \
	../common/tables/TableQAMMapping.h \
	source/sound.h \
	source/soundcommon.h \
	source/soundin.h \
	source/soundout.h

if USE_QT

drmqtonly_SOURCES = \
	../common/GUI-QT/DialogUtil.cpp \
	../common/GUI-QT/DRMPlot.cpp \
	../common/GUI-QT/fdrmdialog.cpp \
	../common/GUI-QT/TransmDlg.cpp \
	../common/GUI-QT/MultColorLED.cpp \
	../common/GUI-QT/systemevalDlg.cpp \
	../common/GUI-QT/MultimediaDlg.cpp \
	../common/GUI-QT/StationsDlg.cpp \
	../common/GUI-QT/LiveScheduleDlg.cpp \
	../common/GUI-QT/MultSettingsDlg.cpp \
	../common/GUI-QT/GeneralSettingsDlg.cpp \
	../common/GUI-QT/EPGDlg.cpp \
	../common/GUI-QT/AnalogDemDlg.cpp \
	../common/MDI/PacketSocketQT.cpp \
	../common/GUI-QT/fdrmdialogbase.ui \
	../common/GUI-QT/TransmDlgbase.ui \
	../common/GUI-QT/AboutDlgbase.ui \
	../common/GUI-QT/systemevalDlgbase.ui \
	../common/GUI-QT/MultimediaDlgbase.ui \
	../common/GUI-QT/StationsDlgbase.ui \
	../common/GUI-QT/LiveScheduleDlgbase.ui \
	../common/GUI-QT/MultSettingsDlgbase.ui \
	../common/GUI-QT/GeneralSettingsDlgbase.ui \
	../common/GUI-QT/EPGDlgbase.ui \
	../common/GUI-QT/AnalogDemDlgbase.ui \
	../common/GUI-QT/AMSSDlgbase.ui \
	../common/GUI-QT/MultimediaDlg.h \
	../common/GUI-QT/fdrmdialog.h \
	../common/GUI-QT/TransmDlg.h \
	../common/GUI-QT/MultColorLED.h \
	../common/GUI-QT/systemevalDlg.h \
	../common/GUI-QT/DialogUtil.h \
	../common/GUI-QT/DRMPlot.h \
	../common/GUI-QT/StationsDlg.h \
	../common/GUI-QT/LiveScheduleDlg.h \
	../common/GUI-QT/MultSettingsDlg.h \
	../common/GUI-QT/GeneralSettingsDlg.h \
	../common/GUI-QT/EPGDlg.h \
	../common/GUI-QT/AnalogDemDlg.h \
	../common/MDI/PacketSocketQT.h \
	../common/datadecoding/epg/epgdec.h \
	../common/datadecoding/epg/epgdec.cpp \
	../common/datadecoding/epg/EPG.cpp \
	../common/datadecoding/epg/EPG.h

drm_SOURCES = $(drmwithoutqt_SOURCES) $(drmqtonly_SOURCES)


# these need to be generated before the rest can be compiled

BUILT_SOURCES=moc/moc_MultColorLED.cpp \
	moc/moc_DRMPlot.cpp \
	moc/moc_DialogUtil.cpp \
	moc/moc_fdrmdialog.cpp moc/moc_fdrmdialogbase.cpp moc/fdrmdialogbase.h moc/fdrmdialogbase.cpp \
	moc/moc_TransmDlg.cpp moc/moc_TransmDlgbase.cpp moc/TransmDlgbase.h moc/TransmDlgbase.cpp \
	moc/moc_systemevalDlg.cpp  moc/moc_systemevalDlgbase.cpp moc/systemevalDlgbase.h \
	moc/systemevalDlgbase.cpp \
	moc/moc_MultimediaDlg.cpp moc/moc_MultimediaDlgbase.cpp moc/MultimediaDlgbase.h moc/MultimediaDlgbase.cpp \
	moc/moc_StationsDlg.cpp moc/moc_StationsDlgbase.cpp moc/StationsDlgbase.h moc/StationsDlgbase.cpp \
	moc/moc_LiveScheduleDlg.cpp moc/moc_LiveScheduleDlgbase.cpp moc/LiveScheduleDlgbase.h moc/LiveScheduleDlgbase.cpp \
	moc/moc_MultSettingsDlg.cpp moc/moc_MultSettingsDlgbase.cpp moc/MultSettingsDlgbase.h moc/MultSettingsDlgbase.cpp \
	moc/moc_GeneralSettingsDlg.cpp moc/moc_GeneralSettingsDlgbase.cpp moc/GeneralSettingsDlgbase.h moc/GeneralSettingsDlgbase.cpp \
	moc/moc_EPGDlg.cpp moc/moc_EPGDlgbase.cpp moc/EPGDlgbase.h moc/EPGDlgbase.cpp \
	moc/moc_AnalogDemDlg.cpp moc/moc_AnalogDemDlgbase.cpp moc/AnalogDemDlgbase.h moc/AnalogDemDlgbase.cpp \
	moc/moc_AboutDlgbase.cpp moc/AboutDlgbase.h moc/AboutDlgbase.cpp \
	moc/moc_AMSSDlgbase.cpp moc/AMSSDlgbase.h moc/AMSSDlgbase.cpp \
	moc/moc_PacketSocketQT.cpp

# and should be cleaned by make clean

CLEANFILES=$(BUILT_SOURCES)
nodist_drm_SOURCES=$(BUILT_SOURCES)

dist-hook:
	mkdir $(distdir)/moc

moc/moc_MultColorLED.cpp: ../common/GUI-QT/MultColorLED.h
	$(MOC) ../common/GUI-QT/MultColorLED.h -o moc/moc_MultColorLED.cpp

moc/moc_DRMPlot.cpp: ../common/GUI-QT/DRMPlot.h
	$(MOC) ../common/GUI-QT/DRMPlot.h -o moc/moc_DRMPlot.cpp

moc/moc_DialogUtil.cpp: ../common/GUI-QT/DialogUtil.h
	$(MOC) ../common/GUI-QT/DialogUtil.h -o moc/moc_DialogUtil.cpp

moc/moc_PacketSocketQT.cpp: ../common/MDI/PacketSocketQT.h
	$(MOC) ../common/MDI/PacketSocketQT.h -o moc/moc_PacketSocketQT.cpp


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


moc/moc_TransmDlg.cpp: ../common/GUI-QT/TransmDlg.h
	$(MOC) ../common/GUI-QT/TransmDlg.h -o moc/moc_TransmDlg.cpp
	
moc/moc_TransmDlgbase.cpp: moc/TransmDlgbase.h
	$(MOC) moc/TransmDlgbase.h -o moc/moc_TransmDlgbase.cpp

moc/TransmDlgbase.h: ../common/GUI-QT/TransmDlgbase.ui
	$(UIC) ../common/GUI-QT/TransmDlgbase.ui -o moc/TransmDlgbase.h

moc/TransmDlgbase.cpp: ../common/GUI-QT/TransmDlgbase.ui moc/TransmDlgbase.h
	$(UIC) ../common/GUI-QT/TransmDlgbase.ui -i moc/TransmDlgbase.h -o moc/TransmDlgbase.cpp


moc/moc_systemevalDlg.cpp: ../common/GUI-QT/systemevalDlg.h
	$(MOC) ../common/GUI-QT/systemevalDlg.h -o moc/moc_systemevalDlg.cpp
		
moc/moc_systemevalDlgbase.cpp: moc/systemevalDlgbase.h
	$(MOC) moc/systemevalDlgbase.h -o moc/moc_systemevalDlgbase.cpp
	
moc/systemevalDlgbase.h: ../common/GUI-QT/systemevalDlgbase.ui
	$(UIC) ../common/GUI-QT/systemevalDlgbase.ui -o moc/systemevalDlgbase.h

moc/systemevalDlgbase.cpp: ../common/GUI-QT/systemevalDlgbase.ui moc/systemevalDlgbase.h
	$(UIC) ../common/GUI-QT/systemevalDlgbase.ui -i moc/systemevalDlgbase.h -o moc/systemevalDlgbase.cpp


moc/moc_MultimediaDlg.cpp: ../common/GUI-QT/MultimediaDlg.h
	$(MOC) ../common/GUI-QT/MultimediaDlg.h -o moc/moc_MultimediaDlg.cpp
		
moc/moc_MultimediaDlgbase.cpp: moc/MultimediaDlgbase.h
	$(MOC) moc/MultimediaDlgbase.h -o moc/moc_MultimediaDlgbase.cpp
	
moc/MultimediaDlgbase.h: ../common/GUI-QT/MultimediaDlgbase.ui
	$(UIC) ../common/GUI-QT/MultimediaDlgbase.ui -o moc/MultimediaDlgbase.h

moc/MultimediaDlgbase.cpp: ../common/GUI-QT/MultimediaDlgbase.ui moc/MultimediaDlgbase.h
	$(UIC) ../common/GUI-QT/MultimediaDlgbase.ui -i moc/MultimediaDlgbase.h -o moc/MultimediaDlgbase.cpp


moc/moc_StationsDlg.cpp: ../common/GUI-QT/StationsDlg.h
	$(MOC) ../common/GUI-QT/StationsDlg.h -o moc/moc_StationsDlg.cpp
		
moc/moc_StationsDlgbase.cpp: moc/StationsDlgbase.h
	$(MOC) moc/StationsDlgbase.h -o moc/moc_StationsDlgbase.cpp
	
moc/StationsDlgbase.h: ../common/GUI-QT/StationsDlgbase.ui
	$(UIC) ../common/GUI-QT/StationsDlgbase.ui -o moc/StationsDlgbase.h

moc/StationsDlgbase.cpp: ../common/GUI-QT/StationsDlgbase.ui moc/StationsDlgbase.h
	$(UIC) ../common/GUI-QT/StationsDlgbase.ui -i moc/StationsDlgbase.h -o moc/StationsDlgbase.cpp


moc/moc_LiveScheduleDlg.cpp: ../common/GUI-QT/LiveScheduleDlg.h
	$(MOC) ../common/GUI-QT/LiveScheduleDlg.h -o moc/moc_LiveScheduleDlg.cpp
		
moc/moc_LiveScheduleDlgbase.cpp: moc/LiveScheduleDlgbase.h
	$(MOC) moc/LiveScheduleDlgbase.h -o moc/moc_LiveScheduleDlgbase.cpp
	
moc/LiveScheduleDlgbase.h: ../common/GUI-QT/LiveScheduleDlgbase.ui
	$(UIC) ../common/GUI-QT/LiveScheduleDlgbase.ui -o moc/LiveScheduleDlgbase.h

moc/LiveScheduleDlgbase.cpp: ../common/GUI-QT/LiveScheduleDlgbase.ui moc/LiveScheduleDlgbase.h
	$(UIC) ../common/GUI-QT/LiveScheduleDlgbase.ui -i moc/LiveScheduleDlgbase.h -o moc/LiveScheduleDlgbase.cpp


moc/moc_MultSettingsDlg.cpp: ../common/GUI-QT/MultSettingsDlg.h
	$(MOC) ../common/GUI-QT/MultSettingsDlg.h -o moc/moc_MultSettingsDlg.cpp
		
moc/moc_MultSettingsDlgbase.cpp: moc/MultSettingsDlgbase.h
	$(MOC) moc/MultSettingsDlgbase.h -o moc/moc_MultSettingsDlgbase.cpp
	
moc/MultSettingsDlgbase.h: ../common/GUI-QT/MultSettingsDlgbase.ui
	$(UIC) ../common/GUI-QT/MultSettingsDlgbase.ui -o moc/MultSettingsDlgbase.h

moc/MultSettingsDlgbase.cpp: ../common/GUI-QT/MultSettingsDlgbase.ui moc/MultSettingsDlgbase.h
	$(UIC) ../common/GUI-QT/MultSettingsDlgbase.ui -i moc/MultSettingsDlgbase.h -o moc/MultSettingsDlgbase.cpp


moc/moc_GeneralSettingsDlg.cpp: ../common/GUI-QT/GeneralSettingsDlg.h
	$(MOC) ../common/GUI-QT/GeneralSettingsDlg.h -o moc/moc_GeneralSettingsDlg.cpp
		
moc/moc_GeneralSettingsDlgbase.cpp: moc/GeneralSettingsDlgbase.h
	$(MOC) moc/GeneralSettingsDlgbase.h -o moc/moc_GeneralSettingsDlgbase.cpp
	
moc/GeneralSettingsDlgbase.h: ../common/GUI-QT/GeneralSettingsDlgbase.ui
	$(UIC) ../common/GUI-QT/GeneralSettingsDlgbase.ui -o moc/GeneralSettingsDlgbase.h

moc/GeneralSettingsDlgbase.cpp: ../common/GUI-QT/GeneralSettingsDlgbase.ui moc/GeneralSettingsDlgbase.h
	$(UIC) ../common/GUI-QT/GeneralSettingsDlgbase.ui -i moc/GeneralSettingsDlgbase.h -o moc/GeneralSettingsDlgbase.cpp


moc/moc_EPGDlg.cpp: ../common/GUI-QT/EPGDlg.h
	$(MOC) ../common/GUI-QT/EPGDlg.h -o moc/moc_EPGDlg.cpp
		
moc/moc_EPGDlgbase.cpp: moc/EPGDlgbase.h
	$(MOC) moc/EPGDlgbase.h -o moc/moc_EPGDlgbase.cpp
	
moc/EPGDlgbase.h: ../common/GUI-QT/EPGDlgbase.ui
	$(UIC) ../common/GUI-QT/EPGDlgbase.ui -o moc/EPGDlgbase.h

moc/EPGDlgbase.cpp: ../common/GUI-QT/EPGDlgbase.ui moc/EPGDlgbase.h
	$(UIC) ../common/GUI-QT/EPGDlgbase.ui -i moc/EPGDlgbase.h -o moc/EPGDlgbase.cpp


moc/moc_AnalogDemDlg.cpp: ../common/GUI-QT/AnalogDemDlg.h
	$(MOC) ../common/GUI-QT/AnalogDemDlg.h -o moc/moc_AnalogDemDlg.cpp
		
moc/moc_AnalogDemDlgbase.cpp: moc/AnalogDemDlgbase.h
	$(MOC) moc/AnalogDemDlgbase.h -o moc/moc_AnalogDemDlgbase.cpp
	
moc/AnalogDemDlgbase.h: ../common/GUI-QT/AnalogDemDlgbase.ui
	$(UIC) ../common/GUI-QT/AnalogDemDlgbase.ui -o moc/AnalogDemDlgbase.h

moc/AnalogDemDlgbase.cpp: ../common/GUI-QT/AnalogDemDlgbase.ui moc/AnalogDemDlgbase.h
	$(UIC) ../common/GUI-QT/AnalogDemDlgbase.ui -i moc/AnalogDemDlgbase.h -o moc/AnalogDemDlgbase.cpp
	
	
moc/moc_AMSSDlgbase.cpp: moc/AMSSDlgbase.h
	$(MOC) moc/AMSSDlgbase.h -o moc/moc_AMSSDlgbase.cpp
	
moc/AMSSDlgbase.h: ../common/GUI-QT/AMSSDlgbase.ui
	$(UIC) ../common/GUI-QT/AMSSDlgbase.ui -o moc/AMSSDlgbase.h
	
moc/AMSSDlgbase.cpp: ../common/GUI-QT/AMSSDlgbase.ui moc/AMSSDlgbase.h
	$(UIC) ../common/GUI-QT/AMSSDlgbase.ui -i moc/AMSSDlgbase.h -o moc/AMSSDlgbase.cpp		


# QT translation file (added by Stephane Fillod 11/10/2004)

LUPDATE=lupdate
LRELEASE=lrelease

dreamtr.ts: $(drmqtonly_SOURCES)
	$(LUPDATE) $(drmqtonly_SOURCES) -ts $@

languages/%.ts: $(drmqtonly_SOURCES)
	mkdir -p languages
	test -f $@ || cp -p ../common/GUI-QT/$@ languages/. || exit 0
	$(LUPDATE) $(drmqtonly_SOURCES) -ts $@

languages/%.qm: languages/%.ts
	$(LRELEASE) $<

%.qm: languages/%.qm
	cp $< dreamtr.qm

else
drm_SOURCES = $(drmwithoutqt_SOURCES)
endif


drm_CXXFLAGS=$(QWTINCL) -I../common/GUI-QT
