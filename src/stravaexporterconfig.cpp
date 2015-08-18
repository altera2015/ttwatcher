#include "stravaexporterconfig.h"
#include "watchexporters.h"

StravaExporterConfig::StravaExporterConfig(const QString &serial) :IExporterConfig(serial)
{
}

bool StravaExporterConfig::equals(const IExporterConfig *other)
{
    if ( !IExporterConfig::equals(other) )
    {
        return false;
    }

    const StravaExporterConfig * o = dynamic_cast<const StravaExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    return o->m_AuthToken == m_AuthToken;
}

bool StravaExporterConfig::apply(const IExporterConfig *other)
{
    const StravaExporterConfig * o = dynamic_cast<const StravaExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    IExporterConfig::apply(other);
    m_AuthToken = o->m_AuthToken;
    return true;
}

void StravaExporterConfig::reset()
{
    IExporterConfig::reset();
    m_AuthToken.clear();
}

void StravaExporterConfig::setAuthToken(const QByteArray &authToken)
{
    if ( m_AuthToken != authToken )
    {
        m_AuthToken = authToken;
        setChanged(true);
    }
}

QByteArray StravaExporterConfig::authToken() const
{
    return m_AuthToken;
}

bool StravaExporterConfig::isOnline() const
{
    return true;
}

bool StravaExporterConfig::allowSaveOnWatch() const
{
    return true;
}

void StravaExporterConfig::updateConfig(QDomDocument &document, QDomElement &element)
{
    writeExportTag(document, element, "Strava");
    writeEncodedTag(document, element, "StravaAuthToken", m_AuthToken);
    setChanged(false);
}

bool StravaExporterConfig::loadConfig(QDomElement & element)
{
    parseExportTag(element, "Strava");
    m_AuthToken = readEncodedTag(element, "StravaAuthToken");
    setChanged(false);
    return true;
}
