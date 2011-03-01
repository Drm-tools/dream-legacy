CONFIG		+= debug
QT			+= qt3support network xml
VPATH		+= common/GUI-QT
FORMS		+= TransmDlgbase.ui fdrmdialogbase.ui AnalogDemDlgbase.ui fmdialogbase.ui
FORMS		+= AMSSDlgbase.ui systemevalDlgbase.ui MultimediaDlgbase.ui
FORMS		+= LiveScheduleDlgbase.ui StationsDlgbase.ui EPGDlgbase.ui
FORMS		+= GeneralSettingsDlgbase.ui MultSettingsDlgbase.ui AboutDlgbase.ui

unix {
	INCLUDEPATH	+= /usr/include/qwt-qt4
	LIBS 		+= -lqwt-qt4
}

win32 {
	INCLUDEPATH	+= libs/qwt5
	LIBS 		+= qwt5.lib
}

include("dream-common.pro")

HEADERS		+= common/GUI-QT/DRMPlot-qwt5.h
SOURCES		+= common/GUI-QT/DRMPlot-qwt5.cpp
