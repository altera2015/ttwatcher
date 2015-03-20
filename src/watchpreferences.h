#ifndef WATCHPREFERENCES_H
#define WATCHPREFERENCES_H

#include <QObject>
#include <QSharedPointer>
#include "iactivityexporter.h"

class TTWatch;

class WatchPreferences : public QObject
{
    Q_OBJECT
public:
    explicit WatchPreferences(const QString & serial, QObject *parent = 0);

    QString name() const;
    void setName( const QString & name );
    QString serial() const;

    bool parsePreferences(const QByteArray & data );
    QByteArray updatePreferences(const QByteArray &data );

    IActivityExporterList exporters();
    IActivityExporterPtr exporter( const QString & service );

    QString encodeToken(const QByteArray &token) const;
    QByteArray decodeToken(const QString &token) const;

    bool exportActivity(ActivityPtr activity);
    bool exportFile(const QString &filename);
    bool isExporting();

signals:
    void exportFinished( bool success, QString message, QUrl url );
    void allExportsFinished();
    void exportError( QString error );
public slots:
private slots:
    void onExportFinished(bool success, QString message, QUrl url);

private:

    QString m_Name;
    QString m_Serial;
    int m_ExportFinishedCounter;
    IActivityExporterList m_Exporters;

    QByteArray scrambleToken(const QByteArray &sourceToken) const;
};
typedef QSharedPointer<WatchPreferences>WatchPreferencesPtr;

#endif // WATCHPREFERENCES_H
