#ifndef TCXEXPORTERCONFIG_H
#define TCXEXPORTERCONFIG_H

#include "iexporterconfig.h"

class TCXExporterConfig : public IExporterConfig
{
public:
    TCXExporterConfig(const QString & serial);

    // load the config from element.
    virtual bool loadConfig( QDomElement & element );

    // update the config into document as child elements of element.
    virtual void updateConfig( QDomDocument & document, QDomElement & element );

    // return true if this setting may be stored on the watch.
    virtual bool allowSaveOnWatch() const;
    // return true if this should save in the "online" section of the XML.
    virtual bool isOnline() const;
};

#endif // TCXEXPORTERCONFIG_H
