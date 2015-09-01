#include "mainwindow.h"
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QString>
#include "qtsingleapplication.h"

#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QByteArray>
#include <QNetworkRequest>
#include <QCommandLineParser>

#include "version.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    SharedTools::QtSingleApplication a("ttwatcher",argc, argv);
    if ( a.isRunning( ))
    {
        a.sendMessage("focus_app");
        return 0;
    }

    a.setApplicationName("TTWatcher");    
    a.setApplicationVersion(VER_FILEVERSION_STR);    

    a.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("ttwatcher is a 3rd party application for managing your T.T. watch.");

    QCommandLineOption startHidden(QStringList() << "h" << "hidden", QCoreApplication::translate("main", "Startup in traybar with main window hidden."));
    parser.addOption(startHidden);
    parser.process(a);

    Settings::get(); // create settings object.

    int result = 0;

    {
        // scope this so that everything is gone and we can destroy
        // settings at the end.
        MainWindow w;
        QObject::connect(&a, SIGNAL(messageReceived(QString,QObject*)), &w, SLOT(onMessage(QString)));
        QObject::connect(&a, SIGNAL(commitDataRequest(QSessionManager&)), &w, SLOT(onCommitDataRequest(QSessionManager&)));

        if ( !parser.isSet(startHidden) )
        {
           w.show();
        }
        result = a.exec();
    }

    Settings::free();
    return result;
}
