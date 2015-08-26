#include "mainwindow.h"
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QString>

#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QByteArray>
#include <QNetworkRequest>
#include "version.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("TTWatcher");
    a.setApplicationVersion(VER_FILEVERSION_STR);    
    Settings::get(); // create settings object.

    int result = 0;

    {
        // scope this so that everything is gone and we can destroy
        // settings at the end.
        MainWindow w;
        w.show();
        result = a.exec();
    }

    Settings::free();
    return result;
}
