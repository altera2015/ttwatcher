#ifndef SRTMELEVATIONTILE_H
#define SRTMELEVATIONTILE_H

#include "elevationtile.h"
#include <QStringList>
#include <QFile>

class SRTMElevationTile : public ElevationTile
{
    Q_OBJECT
    QString m_Filename;
    QPointF m_BottomLeft;
    QPointF m_TopRight;
    QFile m_File;
    float m_ArcSeconds;
    double m_CellSize;
    int m_Rows;
    int m_Cols;
    quint64 m_Filesize;
    qint16 * m_Data;
    enum SRTMSet { UNKNOWN, SRTM1, SRTM3 };
    SRTMSet m_Set;

public:
    explicit SRTMElevationTile(const QString & filename, QObject *parent = 0);
    ~SRTMElevationTile();

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

signals:

public slots:

};

#endif // SRTMELEVATIONTILE_H
