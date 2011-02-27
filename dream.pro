FORMS		+= TransmDlgbase.ui fdrmdialogbase.ui AnalogDemDlgbase.ui fmdialogbase.ui
FORMS		+= AMSSDlgbase.ui systemevalDlgbase.ui MultimediaDlgbase.ui
FORMS		+= LiveScheduleDlgbase.ui StationsDlgbase.ui EPGDlgbase.ui
FORMS		+= GeneralSettingsDlgbase.ui MultSettingsDlgbase.ui AboutDlgbase.ui

unix {
	INCLUDEPATH	+= /usr/include/qwt
	LIBS 		+= -lqwt
}

win32 {
	INCLUDEPATH	+= libs/qwt
	LIBS 		+= qwt.lib
}

include("dream-common.pro")

HEADERS		+= common/GUI-QT/DRMPlot.h
SOURCES		+= common/GUI-QT/DRMPlot.cpp
