#include "exportworkingdialog.h"
#include "ui_exportworkingdialog.h"
#include "singleshot.h"
#include <QDebug>


ExportWorkingDialog::ExportWorkingDialog(ActivityPtr activity, WatchExportersPtr exporters, const QString &exporterName, QWidget *parent) :
    QDialog(parent),
    m_Activity(activity),
    m_Exporters(exporters),
    m_ExporterName(exporterName),
    m_HadError(false),
    ui(new Ui::ExportWorkingDialog)
{
    ui->setupUi(this);

    connect(m_Exporters.data(), SIGNAL(allExportsFinished()), this, SLOT(onExportingFinished()));
    connect(m_Exporters.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SLOT(onExportFinished(bool,QString,QUrl)));
    connect(&m_Timer, SIGNAL(timeout()), this, SLOT(process()), Qt::QueuedConnection);
    m_Timer.setSingleShot(true);
    m_Timer.start(100);

}

ExportWorkingDialog::~ExportWorkingDialog()
{
    delete ui;
}

void ExportWorkingDialog::process()
{
    QStringList names;
    if ( m_Exporters->exportActivity(m_Activity, m_ExporterName, &names) )
    {
        foreach ( const QString & name, names)
        {
            workInfo(tr("Exporting to %1").arg(name), false);
        }
    }
    else
    {
        workInfo(tr("Could not export"), true);
        ui->cancelButton->setEnabled(true);
    }
}

void ExportWorkingDialog::workInfo(const QString &message, bool done)
{
    qDebug() << "ExportWorkingDialog::workInfo" << message<< done;
    ui->logWidget->addItem(message);
    qApp->processEvents(QEventLoop::AllEvents);
    if ( done )
    {
        if ( !m_HadError )
        {
            // on OSX we can't call accept , dont know why.
            QMetaObject::invokeMethod(this, "accept",Qt::QueuedConnection);
        }
        else
        {
            ui->cancelButton->setEnabled(true);
        }
    }
}

void ExportWorkingDialog::onExportingFinished()
{
    if (!m_Exporters->isExporting() )
    {
        workInfo(tr("Done"), true);
    }
}

void ExportWorkingDialog::onExportFinished(bool success, QString message, QUrl url)
{
    if ( !success )
    {
        m_HadError = true;
    }

    workInfo(message, false);
    onExportingFinished();
}

void ExportWorkingDialog::on_cancelButton_clicked()
{
    reject();
}
