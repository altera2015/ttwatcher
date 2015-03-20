#include "tcxactivityexporter.h"
#include "tcxexport.h"
#include <QFile>
#include "watchpreferences.h"

TCXActivityExporter::TCXActivityExporter(QObject *parent) :
    IActivityExporter(parent),
    m_Enabled(false),
    m_AutoOpen(false),
    m_Icon(":/icons/tcx.png")
{
}

QString TCXActivityExporter::name() const
{
    return "TCX";
}

bool TCXActivityExporter::loadConfig(const WatchPreferences &preferences, QDomElement element)
{
    Q_UNUSED(preferences);
    parseExportTag(element, "TCX", m_Enabled, m_AutoOpen);
    setChanged(false);
    return true;
}

bool TCXActivityExporter::isEnabled() const
{
    return m_Enabled;
}

void TCXActivityExporter::setEnabled(bool enabled)
{
    if ( m_Enabled != enabled )
    {
        m_Enabled = enabled;
        setChanged(true);
    }
}

bool TCXActivityExporter::isOnline() const
{
    return false;
}

bool TCXActivityExporter::autoOpen() const
{
    return false;
}

void TCXActivityExporter::setAutoOpen(bool autoOpen)
{
    Q_UNUSED(autoOpen);
}

QIcon TCXActivityExporter::icon() const
{
    return m_Icon;
}

void TCXActivityExporter::reset()
{

}

bool TCXActivityExporter::hasSetup() const
{
    return false;
}

void TCXActivityExporter::setup(QWidget *parent)
{
    Q_UNUSED(parent);
}

void TCXActivityExporter::saveConfig(const WatchPreferences &preferences, QDomDocument &document, QDomElement &element)
{
    Q_UNUSED(preferences);
    writeExportTag(document, element, "TCX", m_Enabled, m_AutoOpen);
    setChanged(false);
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
        emit exportFinished(false, tr("Could not write tcx file %1.").arg(f.fileName()), QUrl());
        return;
    }

    e.save(&f, activity);
    f.close();
    emit exportFinished(true, tr("TCX Export success."), QUrl::fromLocalFile( f.fileName() ));
}
