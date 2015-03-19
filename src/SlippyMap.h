#ifndef SLIPPYMAP_H
#define SLIPPYMAP_H

#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QNetworkAccessManager>

class SlippyMap: public QObject
{
    Q_OBJECT    
    QNetworkDiskCache * m_Cache;
    void processTile(QPoint & tp, QByteArray & data );
public:
    int width;
    int height;
    int zoom;
    qreal latitude;
    qreal longitude;
    bool locationSet;
    SlippyMap(QObject *parent = 0);

    void invalidate();

    void render(QPainter *p, const QRect &rect);       

    void pan(const QPoint &delta);
    bool geoToScreen( qreal latitude, qreal longitude, QPoint & p ) const;
    int boundsToZoom ( const QRectF & bounds );
    QRectF geoBounds();

    void cancelDownloads();

private slots:

    void handleNetworkData(QNetworkReply *reply);

    void download();
    bool download(QPoint grab);

signals:
    void updated(const QRect &rect);

protected:
    QRect tileRect(const QPoint &tp);

private:
    QPoint m_offset;
    QRect m_tilesRect;
    QPointF m_CenterPoint;
    QPixmap m_emptyTile;
    QHash<QPoint, QPixmap> m_tilePixmaps;
    QList<QPoint> m_TileList;
    QNetworkAccessManager m_manager;
    QList<QNetworkReply*> m_ActiveRequests;
    QUrl m_url;
};

#endif // SLIPPYMAP_H
