#ifndef RUNKEEPEREXPORTER_H
#define RUNKEEPEREXPORTER_H

#include "iactivityexporter.h"
#include <QNetworkAccessManager>
#include "runkeeperexporterconfig.h"

class RunKeeperExporter : public IActivityExporter
{
    Q_OBJECT
public:
    explicit RunKeeperExporter(const QString & serial, QObject *parent = 0);


    virtual QString name() const;
    virtual QIcon icon() const;
    virtual bool hasSetup() const;
    virtual void setup( QWidget * parent );
    virtual IExporterConfig & config();
    virtual IExporterConfig *createConfig();
public slots:
    virtual void exportActivity( ActivityPtr activity );

private:
    QNetworkAccessManager m_Manager;
    QIcon m_Icon;
    RunKeeperExporterConfig m_Config;
    static QString toRKDate( const QDateTime & date );

    QByteArray m_AuthRedirectUrl;
private slots:
    void onRequestFinished( QNetworkReply * reply );
private:
    void onActivityPosted( QNetworkReply * reply );
    void onGetUserId( QNetworkReply * reply );
    void onAuthRequest( QNetworkReply * reply );
};
typedef QSharedPointer<RunKeeperExporter>RunKeeperExporterPtr;

#endif // RUNKEEPEREXPORTER_H
