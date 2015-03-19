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
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("TTWatcher");
    a.setApplicationDisplayName("TTWatcher");
    a.setApplicationVersion(VER_FILEVERSION_STR);
    MainWindow w;
    w.show();
    return a.exec();
}
