#include "tcxexporterconfig.h"

TCXExporterConfig::TCXExporterConfig(const QString &serial) : IExporterConfig(serial)
{
}

bool TCXExporterConfig::loadConfig( QDomElement & element)
{
    parseExportTag(element, "TCX");
    setChanged(false);
    return true;
}

void TCXExporterConfig::updateConfig( QDomDocument &document, QDomElement &element)
{
    writeExportTag(document, element, "TCX");
    setChanged(false);
}

bool TCXExporterConfig::allowSaveOnWatch() const
{
    return true;
}

bool TCXExporterConfig::isOnline() const
{
    return false;
}


