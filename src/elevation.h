#ifndef ELEVATION_H
#define ELEVATION_H

#include <QObject>
#include <QString>
#include <QList>

#include "elevationtile.h"

class Elevation : public QObject
{
    Q_OBJECT

    QString m_BasePath;
    QList<ElevationTile *> m_Tiles;
    ElevationTile * m_LastTile;
public:    
    Elevation( const QString & basePath = QString() );
    ~Elevation();
    void clear();
    void prepare();
    QString basePath () const;

    enum ElevationResult { ER_NO_DATA, ER_NO_TILE, ER_SUCCESS };
    bool hasElevation( const QPointF &p );
    ElevationResult elevation( const QPointF & p, float & elevation );
    ElevationSource dataSources( const QPointF & p );

};

#endif // ELEVATION_H
