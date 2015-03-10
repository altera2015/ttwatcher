QT       += core gui widgets network printsupport

TARGET = ttwatcher
TEMPLATE = app

win32:LIBS+=-lUser32 -lPsapi -lhid -lsetupapi


SOURCES += main.cpp\
        mainwindow.cpp \    
    ttmanager.cpp \
    ttwatch.cpp \
    Lightmaps.cpp \
    SlippyMap.cpp \
    ttbinreader.cpp \
    activity.cpp \
    lap.cpp \
    trackpoint.cpp \
    geodistance.cpp \
    tcxexport.cpp \
    qcustomplot.cpp \
    elevationloader.cpp

win32:SOURCES+=hid.c
unix:linux:SOURCES+=hidlinux.c
unix:macx:SOURCES+=hidmac.c

HEADERS  += mainwindow.h \
    hidapi.h \
    ttmanager.h \
    ttwatch.h \
    Lightmaps.h \
    SlippyMap.h \
    ttbinreader.h \
    activity.h \
    lap.h \
    trackpoint.h \
    order32.h \
    geodistance.h \
    tcxexport.h \
    qcustomplot.h \
    elevationloader.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
