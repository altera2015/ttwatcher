#include "runkeeperexporterconfig.h"
#include "watchexporters.h"

RunKeeperExporterConfig::RunKeeperExporterConfig(const QString &serial) :IExporterConfig(serial)
{
}

bool RunKeeperExporterConfig::equals(const IExporterConfig *other)
{
    if ( !IExporterConfig::equals(other) )
    {
        return false;
    }

    const RunKeeperExporterConfig * o = dynamic_cast<const RunKeeperExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    return o->m_AuthToken == m_AuthToken;
}

bool RunKeeperExporterConfig::apply(const IExporterConfig *other)
{
    const RunKeeperExporterConfig * o = dynamic_cast<const RunKeeperExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    IExporterConfig::apply(other);
    m_AuthToken = o->m_AuthToken;
    return true;
}

bool RunKeeperExporterConfig::loadConfig(QDomElement & element)
{
    parseExportTag(element, "RunKeeper");
    m_AuthToken = readEncodedTag(element, "AuthToken");
    setChanged(false);
    return true;
}

void RunKeeperExporterConfig::updateConfig(QDomDocument &document, QDomElement &element)
{
    writeExportTag(document, element, "RunKeeper");
    writeEncodedTag(document, element, "AuthToken", m_AuthToken);
    setChanged(false);
}

bool RunKeeperExporterConfig::allowSaveOnWatch() const
{
    return true;
}

bool RunKeeperExporterConfig::isOnline() const
{
    return true;
}

void RunKeeperExporterConfig::reset()
{
    IExporterConfig::reset();
    m_AuthToken.clear();
}

void RunKeeperExporterConfig::setAuthToken(const QByteArray &authToken)
{
    if ( m_AuthToken != authToken )
    {
        m_AuthToken = authToken;
        setChanged(true);
    }
}

QByteArray RunKeeperExporterConfig::authToken() const
{
    return m_AuthToken;
}
