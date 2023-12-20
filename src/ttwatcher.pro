QT       += core gui widgets network printsupport xml

TARGET = ttwatcher
TEMPLATE = app

win32:INCLUDEPATH += C:\Qt\5.4.1\src\qtbase\src\3rdparty\zlib
win32:CONFIG += c++11
win32:LIBS +=-lUser32 -lPsapi -lhid -lsetupapi
win32:RC_FILE = ttwatcher.rc
linux:CONFIG += c++11
linux:LIBS +=-lusb-1.0
unix:macx:CONFIG += app_bundle
unix:macx:LIBS+=-framework IOKit -framework CoreFoundation
unix:macx:QMAKE_CXXFLAGS+= -stdlib=libc++
unix:macx:CONFIG+=c++11
unix:macx:ICON=runningman2.icns
unix:macx:QMAKE_INFO_PLIST=Info.plist
# rebuild on OSX actually delete the build dir if you get odd errors about incorrect Qt versions.
# deploy on osx ~/Qt/5.4/clang_64/bin/macdeployqt ttwatcher.app -dmg


#DEFINES+=USE_DEBUG_PROXY
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
    iactivityexporter.cpp \
    stravaexporter.cpp \
    httpserver.cpp \
    singleshot.cpp \
    flatfileiconprovider.cpp \
    datasmoothing.cpp \
    tcxactivityexporter.cpp \
    settingsdialog.cpp \
    settings.cpp \
    aboutdialog.cpp \
    downloaddialog.cpp \
    runkeeperexporter.cpp \
    smashrunexporter.cpp \
    iexporterconfig.cpp \
    stravaexporterconfig.cpp \
    tcxexporterconfig.cpp \
    runkeeperexporterconfig.cpp \
    smashrunexporterconfig.cpp \
    watchexporters.cpp \
    exportworkingdialog.cpp \
    centeredexpmovavg.cpp \
    workouttreemodel.cpp \
    qtsingleapplication.cpp \
    qtlocalpeer.cpp \
    qtlockedfile.cpp \
    elevation.cpp \
    srtmelevationtile.cpp \
    gridfloatelevationtile.cpp \
    elevationtile.cpp \
    elevationtiledownloaderdialog.cpp \
    elevationdownloaderitem.cpp \
    elevationdownloaderitemmodel.cpp \
    bridge.cpp \
    bridgeeditordialog.cpp \
    bridgepointitemmodel.cpp

win32:SOURCES+=hid.c qtlockedfile_win.cpp
unix:linux:SOURCES+=hidlinux.c qtlockedfile_unix.cpp
unix:macx:SOURCES+=hidmac.c qtlockedfile_unix.cpp

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
    iactivityexporter.h \
    stravaexporter.h \
    httpserver.h \
    strava_auth.h \
    singleshot.h \
    flatfileiconprovider.h \
    datasmoothing.h \
    tcxactivityexporter.h \
    settingsdialog.h \
    settings.h \
    aboutdialog.h \
    downloaddialog.h \
    runkeeperexporter.h \
    smashrunexporter.h \
    iexporterconfig.h \
    stravaexporterconfig.h \
    tcxexporterconfig.h \
    runkeeperexporterconfig.h \
    smashrunexporterconfig.h \
    watchexporters.h \
    exportworkingdialog.h \
    centeredexpmovavg.h \
    workouttreemodel.h \
    qtsingleapplication.h \
    qtlocalpeer.h \
    qtlockedfile.h \
    elevation.h \
    elevationtile.h \
    gridfloatelevationtile.h \
    srtmelevationtile.h \
    elevationtiledownloaderdialog.h \
    elevationdownloaderitem.h \
    elevationdownloaderitemmodel.h \
    bridge.h \
    bridgeeditordialog.h \
    bridgepointitemmodel.h

FORMS    += mainwindow.ui \
    settingsdialog.ui \
    aboutdialog.ui \
    downloaddialog.ui \
    exportworkingdialog.ui \
    elevationtiledownloaderdialog.ui \
    bridgeeditordialog.ui

RESOURCES += \
    resources.qrc \
    datasetdownloadericons.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qhttpserver/src/release/ -lqhttpserver
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qhttpserver/src/debug/ -lqhttpserver
else:unix:LIBS += -L$$OUT_PWD/../qhttpserver/src/ -lqhttpserver

INCLUDEPATH += $$PWD/../qhttpserver/src
DEPENDPATH += $$PWD/../qhttpserver/src

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/release/libqhttpserver.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/debug/libqhttpserver.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/release/qhttpserver.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/debug/qhttpserver.lib
else:unix:PRE_TARGETDEPS += $$OUT_PWD/../qhttpserver/src/libqhttpserver.a

OTHER_FILES += \
    smashrun.html

DISTFILES += \
    runningman2.icns


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../quazip/quazip/release/ -lquazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../quazip/quazip/debug/ -lquazip
else:unix: LIBS += -L$$OUT_PWD/../quazip/quazip/ -lquazip

INCLUDEPATH += $$PWD/../quazip/quazip
DEPENDPATH += $$PWD/../quazip/quazip


CONFIG(debug, debug|release) {
    DEFINES += "TT_DEBUG"
}
else {
    DEFINES += "TT_RELEASE"
}
