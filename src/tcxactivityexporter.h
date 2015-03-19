#ifndef TCXACTIVITYEXPORTER_H
#define TCXACTIVITYEXPORTER_H

#include "iactivityexporter.h"

class TCXActivityExporter : public IActivityExporter
{
    Q_OBJECT
public:
    explicit TCXActivityExporter(QObject *parent = 0);

    virtual QString name() const;
    virtual bool loadConfig( const WatchPreferences & preferences, QDomElement element );
    virtual bool isEnabled() const;
    virtual void setEnabled(bool enabled);
    virtual bool isOnline() const;
    virtual bool autoOpen() const;
    virtual void setAutoOpen( bool autoOpen );
    virtual QIcon icon() const;
    virtual void reset();
    virtual bool hasSetup() const;
    virtual void setup( QWidget * parent );
    virtual void saveConfig( const WatchPreferences & preferences, QDomDocument & document, QDomElement & element );

public slots:
    virtual void exportActivity( ActivityPtr activity );
private:
    bool m_Enabled;
    bool m_AutoOpen;
    QIcon m_Icon;
};
typedef QSharedPointer<TCXActivityExporter>TCXActivityExporterPtr;

#endif // TCXACTIVITYEXPORTER_H
