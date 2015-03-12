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

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
