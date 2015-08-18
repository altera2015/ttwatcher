#ifndef TCXACTIVITYEXPORTER_H
#define TCXACTIVITYEXPORTER_H

#include "iactivityexporter.h"
#include "tcxexporterconfig.h"

class TCXActivityExporter : public IActivityExporter
{
    Q_OBJECT
public:
    explicit TCXActivityExporter(const QString & serial, QObject *parent = 0);

    virtual QString name() const;
    virtual QIcon icon() const;
    virtual bool hasSetup() const;
    virtual void setup( QWidget * parent );
    virtual IExporterConfig & config();
    IExporterConfig *createConfig();

public slots:
    virtual void exportActivity( ActivityPtr activity );
private:
    QIcon m_Icon;
    TCXExporterConfig m_Config;
};
typedef QSharedPointer<TCXActivityExporter>TCXActivityExporterPtr;

#endif // TCXACTIVITYEXPORTER_H
