#ifndef TTMANAGER_H
#define TTMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "ttwatch.h"
#include "watchexporters.h"

typedef QList<quint16> DeviceIdList;
typedef QList<TTWatch*> TTWatchList;
typedef QMap<QString, WatchExportersPtr> WatchExportersMap;

class TTManager : public QObject
{
    Q_OBJECT

    TTWatchList m_TTWatchList;
    WatchExportersMap m_WatchExporters;
    void checkvds(quint16 vid, const DeviceIdList & deviceIds );
    void prepareWatch ( TTWatch * watch );

    TTWatch * find( const QString & path );
    bool remove( const QString & path );
public:

    explicit TTManager(QObject *parent = 0);
    virtual ~TTManager();
    void startSearch();
    const TTWatchList & watches();
    TTWatch * watch( const QString & serial );

    WatchExportersMap &exporters();
    WatchExportersPtr exporters( const QString & serial );
    WatchExportersPtr exportersForName ( const QString & name );
    WatchExportersPtr defaultExporters();


    void saveConfig(QIODevice * dest, WatchExportersPtr watchExporters );
    bool loadConfig(QIODevice * source, const IExporterConfigMap & configMap, QString & name);

    void saveConfig(const QString & filename, WatchExportersPtr watchExporters );
    bool loadConfig(const QString & filename, const IExporterConfigMap & configMap, QString & name);

    void saveConfig(QByteArray & dest, WatchExportersPtr watchExporters );
    bool loadConfig(const QByteArray & source, const IExporterConfigMap & configMap, QString & name);

    void saveConfig(WatchExportersPtr watchExporters);

    // merge config for storing back on watch
    // this only saves configs that allow to be stored on phone.
    QByteArray mergeConfig( const QByteArray & source, const IExporterConfigMap & configMap );

    void saveAllConfig( bool saveToWatch, const QString serialOnly = QString() );
    void loadAllConfig( );

    QString configDir() const;

signals:

    void ttArrived(QString serial);
    void ttRemoved(QString serial);
    void allExportingFinished(); // this gets fired for each WatchPreferences object, so while one watch might be done, not all of them could.
    void exportError( QString error );

public slots:

    void checkForTTs();

private:

    void setupDefaultExporter();
private slots:
    void configChanged( QString serial );

};

#endif // TTMANAGER_H
