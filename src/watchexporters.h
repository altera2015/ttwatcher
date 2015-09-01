#ifndef WATCHEXPORTERS_H
#define WATCHEXPORTERS_H

#include <QObject>
#include <QSharedPointer>
#include "iactivityexporter.h"

class TTWatch;

class WatchExporters : public QObject
{
    Q_OBJECT
public:
    explicit WatchExporters(const QString & serial, QObject *parent = 0);

    QString name() const;
    void setName( const QString & name );
    QString serial() const;

    IActivityExporterList exporters();
    IActivityExporterPtr exporter( const QString & service );

    // do NOT free the returned objects. they are owned by this object.
    const IExporterConfigMap configMap();

    // you must call freeImportMap on this puppy.
    IExporterConfigMap configImportMap();
    void freeImportMap(IExporterConfigMap & map);

    bool exportActivity(ActivityPtr activity, const QString &exporterName = "", QStringList *sl = 0);
    bool exportFile(const QString &filename);
    bool isExporting();

signals:
    void exportFinished( bool success, QString message, QUrl url );
    void allExportsFinished();
    void exportError( QString error );
    void settingsChanged( QString serial );
public slots:
private slots:
    void onSettingsChanged();
    void onExportFinished(bool success, QString message, QUrl url);

private:

    QString m_Name;
    QString m_Serial;
    int m_ExportFinishedCounter;
    IActivityExporterList m_Exporters;
};
typedef QSharedPointer<WatchExporters>WatchExportersPtr;

#endif // WATCHEXPORTERS_H
