#include "exportworkingdialog.h"
#include "ui_exportworkingdialog.h"
#include "singleshot.h"
#include <QDebug>

void ExportWorkingDialog::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    ui->logWidget->clear();

    // we have to run process a bit after showEvent is done, otherwise
    // we might call accepted while still in showEvent,
    // which apparently fails.
    SingleShot::go([this]{
        process();
    }, 250, true, this);
}

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
    }
}

void ExportWorkingDialog::workInfo(const QString &message, bool done)
{
    qDebug() << "ExportWorkingDialog::workInfo" << message<< done;
    ui->logWidget->addItem(message);
    qApp->processEvents();
    if ( done )
    {
        if ( !m_HadError )
        {
            accept();
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
