CONFIG		+= uic3
QT			+= qt3support network xml
FORMS3		+= TransmDlgbase.ui fdrmdialogbase.ui AnalogDemDlgbase.ui fmdialogbase.ui
FORMS3		+= AMSSDlgbase.ui systemevalDlgbase.ui MultimediaDlgbase.ui
FORMS3		+= LiveScheduleDlgbase.ui StationsDlgbase.ui EPGDlgbase.ui
FORMS3		+= GeneralSettingsDlgbase.ui MultSettingsDlgbase.ui AboutDlgbase.ui

unix {
	INCLUDEPATH	+= /usr/include/qwt-qt4
	LIBS 		+= -lqwt-qt4
}

win32 {
	INCLUDEPATH	+= libs/qwt5
	LIBS 		+= qwt5.lib
}

include("dream-common.pro")