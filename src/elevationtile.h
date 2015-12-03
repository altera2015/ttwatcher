#ifndef ELEVATIONTILE_H
#define ELEVATIONTILE_H

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <memory>
#include <QUrl>
#include <QList>
// longitude = +east / -west = X
// latitude = +north / -south = Y

class ElevationSource {
public:
    ElevationSource() : valid(false) {}
    ElevationSource( QUrl _url, QString _baseDir, double _arcSeconds, QString _source, QPointF _p) :
        url(_url),
        baseDir(_baseDir),
        arcSeconds(_arcSeconds),
        source(_source),
        valid(true),
        southWest( _p.x(), _p.y(), 1.0, 1.0)
    {}

    QUrl url;
    QString baseDir;
    double arcSeconds;
    QString source;
    bool valid;
    QRectF southWest;
    double m_Degrees;
    bool contains( const QPointF & p ) {
        return southWest.contains(p);
    }
};
typedef QList<ElevationSource>ElevationSourceList;


class ElevationTile : public QObject
{
    Q_OBJECT
public:
    ElevationTile(QObject * parent = 0);
    virtual ~ElevationTile();

    virtual bool readHeader () = 0;

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    // box edges included
    virtual QPointF bottomLeft() const = 0;
    virtual QPointF topRight() const = 0;
    virtual bool contains( const QPointF & p ) const = 0;

    // accuracy
    virtual float arcSeconds() const = 0;

    // calculation.
    virtual bool elevation( const QPointF & p, float & elevation ) const = 0;

    // removes the tiles from HD.
    virtual bool removeFiles() = 0;
};

#endif // ELEVATIONTILE_H
