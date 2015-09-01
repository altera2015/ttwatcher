#ifndef STRAVAEXPORTER_H
#define STRAVAEXPORTER_H

#include "iactivityexporter.h"
#include <QNetworkAccessManager>
#include "stravaexporterconfig.h"

class StravaExporter : public IActivityExporter
{
    Q_OBJECT

public:
    explicit StravaExporter(const QString & serial, QObject *parent = 0);

    virtual QString name() const;
    virtual QIcon icon() const;    
    virtual bool hasSetup() const;
    virtual void setup( QWidget * parent );
    virtual IExporterConfig & config();
    IExporterConfig *createConfig();

signals:

public slots:
    virtual void exportActivity( ActivityPtr activity );
private slots:
    void requestFinished( QNetworkReply * reply );

private:        
    QNetworkAccessManager m_Manager;    
    QIcon m_Icon;
    StravaExporterConfig m_Config;

    void getActivityStatus(int activityId , int retry);
    void activitySubmitted( QJsonDocument & d, int httpCode );
    void activityStatus(QJsonDocument &d, int httpCode , QNetworkReply *reply);
    void authCodeAnswer( QJsonDocument & d, int httpCode );
};
typedef QSharedPointer<StravaExporter>StravaExporterPtr;

#endif // STRAVAEXPORTER_H
