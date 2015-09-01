#ifndef STRAVAEXPORTERCONFIG_H
#define STRAVAEXPORTERCONFIG_H

#include <QString>
#include "iexporterconfig.h"

class StravaExporterConfig : public IExporterConfig
{
public:
    StravaExporterConfig(const QString & serial);

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

    // reset to default values.
    virtual void reset();

    void setAuthToken( const QByteArray & authToken );
    QByteArray authToken( ) const;

private:
    QByteArray m_AuthToken;

};

#endif // STRAVAEXPORTERCONFIG_H
