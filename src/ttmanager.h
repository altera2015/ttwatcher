#ifndef TTMANAGER_H
#define TTMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "ttwatch.h"
#include "watchpreferences.h"

typedef QList<quint16> DeviceIdList;
typedef QList<TTWatch*> TTWatchList;
typedef QMap<QString, WatchPreferencesPtr> PreferencesMap;

class TTManager : public QObject
{
    Q_OBJECT

    TTWatchList m_TTWatchList;
    PreferencesMap m_Preferences;
    void checkvds(quint16 vid, const DeviceIdList & deviceIds );

    TTWatch * find( const QString & path );
    bool remove( const QString & path );
public:

    explicit TTManager(QObject *parent = 0);
    virtual ~TTManager();
    void startSearch();
    const TTWatchList & watches();
    TTWatch * watch( const QString & serial );

    PreferencesMap &preferences();
    WatchPreferencesPtr preferences( const QString & serial );
    WatchPreferencesPtr preferencesForName ( const QString & name );
    WatchPreferencesPtr defaultPreferences();

    void savePreferences(WatchPreferencesPtr preferences );
    void savePreferences( );
    void loadPreferences(const QString &filename);
    void loadPreferences( );

    QString preferenceDir() const;

signals:

    void ttArrived();
    void ttRemoved();
    void allExportingFinished(); // this gets fired for each WatchPreferences object, so while one watch might be done, not all of them could.
    void exportError( QString error );

public slots:

    void checkForTTs();

private:
    void setupDefaultPreferences();


};

#endif // TTMANAGER_H
