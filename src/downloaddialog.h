#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>

#include "ttmanager.h"
#include "settings.h"

namespace Ui {
class DownloadDialog;
}

class DownloadDialog : public QDialog
{
    Q_OBJECT
    Settings * m_Settings;
    TTManager * m_TTManager;
    QNetworkAccessManager m_Manager;
    QStringList m_Files;

protected:
    void showEvent(QShowEvent *e);

public:
    explicit DownloadDialog(Settings * settings, TTManager * ttManager, QWidget *parent = 0);
    ~DownloadDialog();
    int processWatches();
    QStringList filesDownloaded() const;
private slots:
    void process();
    void workInfo( const QString & message, bool done );
    void onFinished(QNetworkReply * reply );
    void onExportingFinished();
    void onExportError( QString message );
private:
    Ui::DownloadDialog *ui;
};

#endif // DOWNLOADDIALOG_H
