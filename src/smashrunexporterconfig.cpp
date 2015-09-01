#include "smashrunexporterconfig.h"
#include "watchexporters.h"

SmashrunExporterConfig::SmashrunExporterConfig(const QString &serial) : IExporterConfig(serial)
{
}

bool SmashrunExporterConfig::equals(const IExporterConfig *other)
{
    if ( !IExporterConfig::equals(other) )
    {
        return false;
    }

    const SmashrunExporterConfig * o = dynamic_cast<const SmashrunExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    return o->m_AuthToken == m_AuthToken && o->m_RefreshToken == m_RefreshToken;
}

bool SmashrunExporterConfig::apply(const IExporterConfig *other)
{
    const SmashrunExporterConfig * o = dynamic_cast<const SmashrunExporterConfig*>(other);
    if ( !o )
    {
        return false;
    }

    IExporterConfig::apply(other);
    m_AuthToken = o->m_AuthToken;
    m_RefreshToken = o->m_RefreshToken;
    m_LastRefresh = o->m_LastRefresh;
    return true;
}

bool SmashrunExporterConfig::allowSaveOnWatch() const
{
    return false; // TomTom doesn't support Smashrun yet... Silly TomTom.
}

bool SmashrunExporterConfig::isOnline() const
{
    return true;
}

void SmashrunExporterConfig::reset()
{
    IExporterConfig::reset();
    m_AuthToken.clear();
    m_RefreshToken.clear();
    m_LastRefresh = QDateTime();
}

void SmashrunExporterConfig::setAuthToken(const QByteArray &authToken)
{
    if ( m_AuthToken != authToken )
    {
        m_AuthToken = authToken;
        setChanged(true);
    }
}

QByteArray SmashrunExporterConfig::authToken() const
{
    return m_AuthToken;
}

void SmashrunExporterConfig::setRefreshToken(const QByteArray &refreshToken)
{
    if ( m_RefreshToken != refreshToken )
    {
        m_RefreshToken = refreshToken;
        setChanged(true);
    }
}

QByteArray SmashrunExporterConfig::refreshToken() const
{
    return m_RefreshToken;
}

void SmashrunExporterConfig::setLastRefresh(const QDateTime &lastRefresh)
{
    if ( m_LastRefresh != lastRefresh )
    {
        m_LastRefresh = lastRefresh;
        setChanged(true);
    }
}

QDateTime SmashrunExporterConfig::lastRefresh() const
{
    return m_LastRefresh;
}

void SmashrunExporterConfig::updateConfig(QDomDocument &document, QDomElement &element)
{
    writeExportTag(document, element, "Smashrun");
    writeEncodedTag(document, element, "SmashrunAuthToken", m_AuthToken);
    writeEncodedTag(document, element, "SmashrunRefreshToken", m_RefreshToken );
    writeTag(document, element, "SmashrunRefresh",QString::number(m_LastRefresh.toTime_t()) );
    setChanged(false);
}

bool SmashrunExporterConfig::loadConfig(QDomElement & element)
{
    parseExportTag(element, "Smashrun");
    m_AuthToken = readEncodedTag(element, "SmashrunAuthToken");
    m_RefreshToken = readEncodedTag(element, "SmashrunRefreshToken");
    QString val = readTag(element, "SmashrunRefresh");
    if ( val.length() > 0)
    {
        m_LastRefresh = QDateTime::fromTime_t( val.toLongLong() );
    }
    else
    {
        m_LastRefresh = QDateTime();
    }
    setChanged(false);
    return true;
}
