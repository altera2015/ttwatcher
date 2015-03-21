#ifndef RUNKEEPEREXPORTER_H
#define RUNKEEPEREXPORTER_H

#include "iactivityexporter.h"
#include <QNetworkAccessManager>

class RunKeeperExporter : public IActivityExporter
{
    Q_OBJECT
public:
    explicit RunKeeperExporter(QObject *parent = 0);


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

signals:

public slots:
    virtual void exportActivity( ActivityPtr activity );

private:
    bool m_Enabled;
    QByteArray m_AuthToken;
    QNetworkAccessManager m_Manager;
    bool m_AutoOpen;
    QIcon m_Icon;
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
