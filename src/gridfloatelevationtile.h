#ifndef GRIDFLOATELEVATIONTILE_H
#define GRIDFLOATELEVATIONTILE_H

#include "elevationtile.h"

#include <QFile>
#include <QString>

class GridFloatElevationTile : public ElevationTile
{
    Q_OBJECT

    QString m_Filename;
    QPointF m_BottomLeft;
    QPointF m_TopRight;
    QFile m_File;
    // float m_ArcSeconds;
    int m_Rows;
    int m_Cols;
    quint64 m_Filesize;
    float m_CellSize; // in degrees.
    float * m_Data;
    float m_NoData;
    bool m_LSBFirst;


    static bool lookup( const QString &list, const QString &fn );

public:
    GridFloatElevationTile(const QString & filename, QObject * parent = 0);
    virtual ~GridFloatElevationTile();

    virtual bool readHeader();
    virtual bool open();
    virtual void close();
    virtual bool isOpen() const;
    virtual QPointF bottomLeft() const;
    virtual QPointF topRight() const;
    virtual bool contains( const QPointF & p ) const;
    virtual float arcSeconds() const;
    virtual bool elevation( const QPointF & p, float & elevation ) const;
    virtual bool removeFiles();

    static ElevationSourceList dataSetURLs( const QPointF &p );

};

#endif // GRIDFLOATELEVATIONTILE_H
