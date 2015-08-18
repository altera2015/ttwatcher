#ifndef EXPORTWORKINGDIALOG_H
#define EXPORTWORKINGDIALOG_H

#include <QDialog>
#include "watchexporters.h"
#include "activity.h"

namespace Ui {
class ExportWorkingDialog;
}

class ExportWorkingDialog : public QDialog
{
    Q_OBJECT
    ActivityPtr m_Activity;
    WatchExportersPtr m_Exporters;
    QString m_ExporterName;
    bool m_HadError;

    void showEvent(QShowEvent *e);

public:
    explicit ExportWorkingDialog(ActivityPtr activity, WatchExportersPtr exporters, const QString &exporterName, QWidget *parent = 0);
    ~ExportWorkingDialog();

private slots:
    void process();
    void workInfo(const QString &message, bool done);
    void onExportingFinished();

    void onExportFinished( bool success, QString message, QUrl url );

    void on_cancelButton_clicked();

private:
    Ui::ExportWorkingDialog *ui;
};

#endif // EXPORTWORKINGDIALOG_H
