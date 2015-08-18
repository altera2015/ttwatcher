#ifndef SMASHRUNEXPORTER_H
#define SMASHRUNEXPORTER_H

#include <QNetworkAccessManager>
#include "iactivityexporter.h"
#include "smashrunexporterconfig.h"

class SmashrunExporter : public IActivityExporter
{
    Q_OBJECT
public:
    explicit SmashrunExporter(const QString & serial, QObject *parent = 0);

    virtual QString name() const ;
    virtual QIcon icon() const;
    virtual bool hasSetup() const;
    virtual void setup( QWidget * parent );
    virtual IExporterConfig & config();
    IExporterConfig * createConfig();

public slots:
    virtual void exportActivity( ActivityPtr activity );
private slots:
    void requestFinished( QNetworkReply * reply );

private:
    QNetworkAccessManager m_Manager;
    QIcon m_Icon;
    SmashrunExporterConfig m_Config;
    ActivityPtr m_ActivityAfterRefresh;

    void refreshAuth();
    void activitySubmitted( QJsonDocument & d, int httpCode );    
    void authCodeAnswer( QJsonDocument & d, int httpCode );
    void refreshCodeAnswer( QJsonDocument & d, int httpCode );
};

typedef QSharedPointer<SmashrunExporter>SmashrunExporterPtr;

#endif // SMASHRUNEXPORTER_H
