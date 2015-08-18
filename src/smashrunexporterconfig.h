#ifndef SMASHRUNEXPORTERCONFIG_H
#define SMASHRUNEXPORTERCONFIG_H

#include <QDateTime>
#include "iexporterconfig.h"

class SmashrunExporterConfig : public IExporterConfig
{
public:
    SmashrunExporterConfig(const QString & serial);

    // return true if other is the same type AND it's config is exactly the same.
    virtual bool equals( const IExporterConfig * other );

    // copy the settings object from other to this.
    virtual bool apply( const IExporterConfig * other );

    // load the config from element.
    virtual bool loadConfig(QDomElement &element );

    // update the config into document as child elements of element.
    virtual void updateConfig(QDomDocument & document, QDomElement & element );

    // return true if this setting may be stored on the watch.
    virtual bool allowSaveOnWatch() const;
    // return true if this should save in the "online" section of the XML.
    virtual bool isOnline() const;

    // resets to defaults.
    virtual void reset();

    void setAuthToken( const QByteArray & authToken );
    QByteArray authToken( ) const;
    void setRefreshToken(const QByteArray & refreshToken );
    QByteArray refreshToken( ) const;
    void setLastRefresh( const QDateTime & lastRefresh );
    QDateTime lastRefresh() const;

private:
    QByteArray m_AuthToken;
    QByteArray m_RefreshToken;
    QDateTime m_LastRefresh;
};

#endif // SMASHRUNEXPORTERCONFIG_H
