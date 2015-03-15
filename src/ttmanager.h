#ifndef TTMANAGER_H
#define TTMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "ttwatch.h"

typedef QList<quint16> DeviceIdList;
typedef QList<TTWatch*> TTWatchList;

class TTManager : public QObject
{
    Q_OBJECT

    TTWatchList m_TTWatchList;
    void checkvds(quint16 vid, const DeviceIdList & deviceIds );

    TTWatch * find( const QString & path );
    bool remove( const QString & path );
public:

    explicit TTManager(QObject *parent = 0);
    virtual ~TTManager();
    void startSearch();
    const TTWatchList & watches();
    TTWatch * watch( const QString & serial );




signals:

    void ttArrived();
    void ttRemoved();

public slots:

    void checkForTTs();

private slots:


};

#endif // TTMANAGER_H
