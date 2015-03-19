QT       += core gui widgets network printsupport xml

TARGET = ttwatcher
TEMPLATE = app

win32:LIBS+=-lUser32 -lPsapi -lhid -lsetupapi
win32:RC_FILE = ttwatcher.rc
DEFINES+=I_UNDERSTAND_THIS_IS_DANGEROUS
DEFINES+=Q_HTTP_STATIC_BUILD

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
    elevationloader.cpp \
    watchpreferences.cpp \
    iactivityexporter.cpp \
    stravaexporter.cpp \
    httpserver.cpp \
    singleshot.cpp \
    flatfileiconprovider.cpp

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
    elevationloader.h \
    version.h \
    watchpreferences.h \
    iactivityexporter.h \
    stravaexporter.h \
    httpserver.h \
    strava_auth.h \
    singleshot.h \
    flatfileiconprovider.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qhttpserver/src/release/ -lqhttpserver
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qhttpserver/src/debug/ -lqhttpserver
else:unix: LIBS += -L$$OUT_PWD/../qhttpserver/src/ -lqhttpserver

INCLUDEPATH += $$PWD/../qhttpserver/src
DEPENDPATH += $$PWD/../qhttpserver/src

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/release/libqhttpserver.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/debug/libqhttpserver.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/release/qhttpserver.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/debug/qhttpserver.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/libqhttpserver.a
