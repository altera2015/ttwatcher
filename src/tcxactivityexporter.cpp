#include "tcxactivityexporter.h"
#include "tcxexport.h"
#include <QFile>
#include "watchexporters.h"

TCXActivityExporter::TCXActivityExporter(const QString &serial, QObject *parent) :
    IActivityExporter(parent),
    m_Icon(":/icons/tcx.png"),
    m_Config(serial)
{
}

QString TCXActivityExporter::name() const
{
    return "TCX";
}

QIcon TCXActivityExporter::icon() const
{
    return m_Icon;
}

bool TCXActivityExporter::hasSetup() const
{
    return false;
}

void TCXActivityExporter::setup(QWidget *parent)
{
    Q_UNUSED(parent);
}

IExporterConfig &TCXActivityExporter::config()
{
    return m_Config;
}

IExporterConfig * TCXActivityExporter::createConfig()
{
    return new TCXExporterConfig( m_Config.serial() );
}


void TCXActivityExporter::exportActivity(ActivityPtr activity)
{
    if ( !activity )
    {
        return;
    }

    TCXExport e;

    QFile f( activity->filename() + ".tcx");
    if ( !f.open(QIODevice::WriteOnly) )
    {
        emit exportFinished(false, tr("TCX: Could not write file %1.").arg(f.fileName()), QUrl());
        return;
    }

    e.save(&f, activity);
    f.close();
    emit exportFinished(true, tr("TCX: Export success."), QUrl::fromLocalFile( f.fileName() ));
}
