#ifndef WATCHPREFERENCES_H
#define WATCHPREFERENCES_H

#include <QObject>
#include "iactivityexporter.h"

class TTWatch;

class WatchPreferences : public QObject
{
    Q_OBJECT
public:
    explicit WatchPreferences(QObject *parent = 0);

    QString name() const;
    void setName( const QString & name );

    bool parsePreferences(TTWatch *watch, const QByteArray & data );
    QByteArray updatePreferences( TTWatch * watch, const QByteArray &data );

    IActivityExporterList exporters();
    IActivityExporterPtr exporter( const QString & service );

signals:
    void exportFinished( bool success, QString message, QUrl url );
    void setupFinished( bool success );

public slots:
private slots:

private:

    QString m_Name;
    IActivityExporterList m_Exporters;

};

#endif // WATCHPREFERENCES_H
