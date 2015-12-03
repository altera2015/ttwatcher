#ifndef ELEVATIONTILEDOWNLOADERDIALOG_H
#define ELEVATIONTILEDOWNLOADERDIALOG_H

#include <QDialog>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryFile>

#include "elevation.h"
#include "elevationdownloaderitem.h"
#include "elevationdownloaderitemmodel.h"

namespace Ui {
class ElevationTileDownloaderDialog;
}

class ElevationTileDownloaderDialog : public QDialog
{
    Q_OBJECT

    QString m_BaseDir;    
    ElevationDownloaderItemModel m_ListModel;
    QNetworkAccessManager m_Manager;
    QList<QNetworkReply *> m_Replies;

    bool unpack(ElevationTileDownloaderItem * item);
    void download(ElevationTileDownloaderItem * item );

    void checkDone();
    void closeEvent(QCloseEvent *e);

public:
    explicit ElevationTileDownloaderDialog(const QString & baseDir, QWidget *parent = 0);
    ~ElevationTileDownloaderDialog();
    void addSource(ElevationSource & source);
    void go();



private slots:

    void on_cancelButton_clicked();

private:
    Ui::ElevationTileDownloaderDialog *ui;
};

#endif // ELEVATIONTILEDOWNLOADERDIALOG_H
