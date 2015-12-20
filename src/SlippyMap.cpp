/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "SlippyMap.h"

#include <QApplication>
#include <math.h>


// tile size in pixels
static const int tdim = 256;

// hash for caching the tiles
static unsigned int qHash(const QPoint& p)
{
    return p.x() * 17 ^ p.y();
}

QPointF SlippyMap::tileForCoordinate(const QPointF & geo, int zoom)
{
    return tileForCoordinate(geo.y(), geo.x(), zoom);
}


// Mercator projection calculations.
QPointF SlippyMap::tileForCoordinate(qreal lat, qreal lng, int zoom)
{
    qreal zn = static_cast<qreal>(1 << zoom);
    qreal tx = (lng + 180.0) / 360.0;
    qreal ty = (1.0 - log(tan(lat * M_PI / 180.0) +
                          1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0;
    return QPointF(tx * zn, ty * zn);
}

static qreal longitudeFromTile(qreal tx, int zoom)
{
    qreal zn = static_cast<qreal>(1 << zoom);
    qreal lat = tx / zn * 360.0 - 180.0;
    return lat;
}

static qreal latitudeFromTile(qreal ty, int zoom)
{
    qreal zn = static_cast<qreal>(1 << zoom);
    qreal n = M_PI - 2 * M_PI * ty / zn;
    qreal lng = 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
    return lng;
}

// not quite a spirallist. TODO make it one.
static void reverseSpiralList ( int start, int end, int startY, int h, QList<QPoint> &list)
{
    for (int y=startY;y < (startY + h); y++)
    {
        for (int x=start;x<=end;x++)
        {
             list.insert(0, QPoint(x,y));
        }
    }
}


//QString path = "http://tile.openstreetmap.org/%1/%2/%3.png";
// QString path = "http://otile1.mqcdn.com/tiles/1.0.0/map/%1/%2/%3.png";
// QString path = "http://otile1.mqcdn.com/tiles/1.0.0/sat/%1/%2/%3.png";


SlippyMap::SlippyMap(QObject *parent) : QObject(parent), width(400), height(300), zoom(15),
    latitude(0),
    longitude(0),
    locationSet(false),
    m_TilePath("http://otile1.mqcdn.com/tiles/1.0.0/map/%1/%2/%3.png")

    // latitude(59.9138204), longitude(10.7387413), locationSet(false)
{    
    m_emptyTile = QPixmap(tdim, tdim);
    m_emptyTile.fill(Qt::lightGray);

    m_Cache = new QNetworkDiskCache;

    // QString c = gSettings->installPath() + "\\cache";

    QString c = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    c += "/" + qApp->applicationName() + "/tiles";


    QDir d(c);
    if (!d.exists())
    {
        d.mkpath(d.path());
    }


    m_Cache->setCacheDirectory(c);


    m_manager.setCache(m_Cache);
    connect(&m_manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(handleNetworkData(QNetworkReply*)));
}

void SlippyMap::invalidate()
{

    if ( !locationSet)
    {
        return;
    }

    if (width <= 0 || height <= 0)
    {
        return;
    }

    m_CenterPoint = tileForCoordinate(latitude, longitude, zoom);

    qreal tx = m_CenterPoint.x();
    qreal ty = m_CenterPoint.y();

    // top-left corner of the center tile
    int xp = width / 2 - (tx - floor(tx)) * tdim;
    int yp = height / 2 - (ty - floor(ty)) * tdim;

    // first tile vertical and horizontal
    int xa = (xp + tdim - 1) / tdim;
    int ya = (yp + tdim - 1) / tdim;
    int xs = static_cast<int>(tx) - xa;
    int ys = static_cast<int>(ty) - ya;


    // offset for top-left tile
    m_offset = QPoint(xp - xa * tdim, yp - ya * tdim);

    // last tile vertical and horizontal
    int xe = static_cast<int>(tx) + (width - xp - 1) / tdim;
    int ye = static_cast<int>(ty) + (height - yp - 1) / tdim;

    // build a rect
    m_tilesRect = QRect(xs, ys, xe - xs + 1, ye - ys + 1);
    m_TileList.clear();
    reverseSpiralList(0, m_tilesRect.width(), 0, m_tilesRect.height(), m_TileList);

    for ( int i=0; i <  m_TileList.length(); i++ )
    {
        m_TileList[i] = m_tilesRect.topLeft() + m_TileList[i];
    }

    // now move the center tile to the beginning just to be sure.
    QPoint center( static_cast<int>(tx),static_cast<int>(ty) );

    for ( int i=0; i <  m_TileList.length(); i++ )
    {
        if ( m_TileList[i] == center )
        {
            m_TileList.move(i,0);
            break;
        }
    }

    download();

    emit updated(QRect(0, 0, width, height));
}

void SlippyMap::render(QPainter *p, const QRect &rect)
{
    for (int x = 0; x <= m_tilesRect.width(); ++x)
    {
        for (int y = 0; y <= m_tilesRect.height(); ++y)
        {
            QPoint tp(x + m_tilesRect.left(), y + m_tilesRect.top());
            QRect box = tileRect(tp);
            if (rect.intersects(box))
            {
                if (m_tilePixmaps.contains(tp))
                {
                    p->drawPixmap(box, m_tilePixmaps.value(tp));
                }
                else
                {
                    p->drawPixmap(box, m_emptyTile);
                }
            }
        }
    }
}

void SlippyMap::pan(const QPoint &delta)
{    
    QPointF dx = QPointF(delta) / qreal(tdim);
    QPointF center = tileForCoordinate(latitude, longitude, zoom) - dx;
    latitude = latitudeFromTile(center.y(), zoom);
    longitude = longitudeFromTile(center.x(), zoom);    
    invalidate();

}


void SlippyMap::processTile(QPoint &tp, QByteArray &data)
{
    QImage img;
    if (!img.loadFromData(data)) //(!img.load(reply, 0))
    {
        // download(tp); infinite recursion, handle errors a bit better.
        qDebug() << "processTile could not load data ";
    }
    else
    {
        m_tilePixmaps[tp] = QPixmap::fromImage(img);
        if (img.isNull())
        {
            m_tilePixmaps[tp] = m_emptyTile;
        }

        emit updated(tileRect(tp));


        // purge unused spaces
        QRect bound = m_tilesRect.adjusted(-5, -5, 5, 5);
        foreach(QPoint tp, m_tilePixmaps.keys())
        {
            if (!bound.contains(tp))
            {
                m_tilePixmaps.remove(tp);
            }
        }
    }
}

QByteArray & SlippyMap::loadEmptyTile()
{
    if ( m_EmptyTileData.length() == 0 )
    {
        QFile f(":/empty_tile.png");
        if ( !f.open(QIODevice::ReadOnly))
        {
           qDebug() << "SlippyMap::handleData / could not load empty tile from resources";
        }
        else
        {
            m_EmptyTileData = f.readAll();
        }
        f.close();
    }
    return m_EmptyTileData;
}

void SlippyMap::handleNetworkData(QNetworkReply *reply)
{   
    m_url = 0;

    QPoint tp = reply->request().attribute(QNetworkRequest::User).toPoint();
    QUrl url = reply->url();    

    m_ActiveRequests.removeOne(reply);

    QNetworkReply::NetworkError e = reply->error();
    QByteArray data;
    switch ( e )
    {        
    case QNetworkReply::NoError:
        data = reply->readAll();
        processTile(tp, data);
        break;
    case QNetworkReply::ContentNotFoundError:
        processTile(tp, loadEmptyTile());
        break;
    default:
        qDebug() << "SlippyMap::handleData / " << reply->errorString() << (int)reply->error();
        m_Cache->remove( url );
        //download(tp);
    }

    reply->deleteLater();        

    download();

}


void SlippyMap::download()
{
    if (! m_url.isEmpty() )
    {
        // qDebug() << "already downloading abort...";
        return;
    }

    QPoint grab(0, 0);
    for ( int i=0;i<m_TileList.length();i++)
    {
        if (!m_tilePixmaps.contains( m_TileList[i] ))
        {
            grab = m_TileList[i];
            if ( !download(grab) )
            {
                return;
            }
        }
    }    
}

// returns true if download should get the next
// returns false if we are actually downloading.
bool SlippyMap::download(QPoint grab)
{

    if ( grab.x()<0 || grab.y()<0)
    {        
        processTile(grab, loadEmptyTile());
        return true;
    }

    m_url = QUrl(m_TilePath.arg(zoom).arg(grab.x()).arg(grab.y()));

    QNetworkCacheMetaData md = m_Cache->metaData(m_url);
    QDateTime now = QDateTime::currentDateTime();

    if ( md.isValid() && now.secsTo(md.expirationDate()) > -3600 * 24 )
    {
        QIODevice * d = m_Cache->data(m_url);
        QByteArray data = d->readAll();
        d->close();
        d->deleteLater();

        processTile(grab, data);
        m_url = 0;        

        return true;
    }
    else
    {
        QNetworkRequest request;
        request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 6.3; WOW64; rv:29.0) Gecko/20100101 Firefox/29.0");
        request.setUrl(m_url);
        request.setAttribute(QNetworkRequest::User, QVariant(grab));
        // request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
        m_ActiveRequests.append ( m_manager.get(request) );

        return false;
    }
}

void SlippyMap::setTilePath(const QString &tilePath)
{
    m_TilePath = tilePath;
    m_tilePixmaps.clear();
    invalidate();
}


QRect SlippyMap::tileRect(const QPoint &tp)
{
    QPoint t = tp - m_tilesRect.topLeft();
    int x = t.x() * tdim + m_offset.x();
    int y = t.y() * tdim + m_offset.y();
    return QRect(x, y, tdim, tdim);
}

bool SlippyMap::geoToScreen( qreal latitude, qreal longitude, QPoint & p ) const
{
    // slippy map is a mercator projection. Each lat/long combination points to a tile.
    // the globe is split in 2 ^ zoom tiles.
    // tileForCoordinate gives you the tile number including the fractional part of the tile
    // you also know how many pixels each tile is (tdim). Combining this will allow us
    // to precisely pinpoint coordinates on the map.

    QPointF req = tileForCoordinate(latitude, longitude, zoom);

    // QPointF is in tile coordinates. We want the offset from the center
    QPointF offset = m_CenterPoint - req;

    // offset now holds the distance (and angle) from the center in tile coordiantes.
    // multiply by tdim to get screen coordinates.
    QPointF screen = offset * tdim;

    p.setX( static_cast<int>( width / 2.0 - screen.x()   ));
    p.setY( static_cast<int>( height / 2.0 - screen.y()  ));

    return p.x() >= 0 && p.x() <= width && p.y() >= 0 && p.y() <= height;
}

void SlippyMap::screenToGeo(const QPointF &centerPoint, int zoom, const QPoint &p, qreal &latitude, qreal &longitude) const
{
    QPointF offset ( (width / 2.0 - p.x()),
                     (height / 2.0 - p.y()));

    offset /= tdim;
    offset = centerPoint - offset;
    latitude = latitudeFromTile( offset.y(), zoom);
    longitude = longitudeFromTile( offset.x(), zoom);

    while ( latitude > 90 )
    {
        latitude -= 90;
    }
    while ( latitude < -90 )
    {
        latitude += -90;
    }
    while ( longitude > 180 )
    {
        longitude -= 180;
    }
    while ( longitude < -180 )
    {
        longitude += 180;
    }
}

void SlippyMap::screenToGeo(const QPoint &p, qreal &latitude, qreal &longitude) const
{
    screenToGeo(m_CenterPoint, zoom, p, latitude, longitude);
}

// bounds calculation from:
// http://stackoverflow.com/questions/6048975/google-maps-v3-how-to-calculate-the-zoom-level-for-a-given-bounds
static double latRad( double lat )
{
    double s = sin( lat * M_PI / 180.0 );
    double radX2 = log(( 1 + s ) / ( 1 - s ) ) / 2;
    return qMax( qMin( radX2, M_PI ), -M_PI) / 2;
}

static int calc_zoom( double mapPx, double worldPx, double fraction )
{
    if ( qFuzzyCompare( fraction, 0.0 ) )
    {
        return 18;
    }

    return floor( log(mapPx / worldPx / fraction) / 0.6931471805599453); // 0.6931471805599453 = nat log 2.
}


int SlippyMap::boundsToZoom(const QRectF &bounds)
{
    QPoint worldDim (256,256);
    int zoomMax = 18;

    QPointF ne = bounds.topRight();
    QPointF sw = bounds.bottomLeft();

    double latFraction = fabs( latRad(ne.y()) - latRad( sw.y())) / M_PI;
    double lngDiff = fabs(ne.x() - sw.x());
    double lngFraction = ((lngDiff < 0 ) ? ( lngDiff + 360 ) : lngDiff ) / 360;
    int latZoom = calc_zoom( height, worldDim.y(), latFraction);
    int lngZoom = calc_zoom( width, worldDim.x(), lngFraction);

    return qMax(0, qMin( qMin( latZoom, lngZoom), zoomMax));
}


QRectF SlippyMap::geoBounds()
{
	qreal w = (qreal)width / (qreal)tdim;
	qreal h = (qreal)height / (qreal)tdim;


    QRectF bounds;
    bounds.setTop( latitudeFromTile( m_CenterPoint.y() - h / 2, zoom) );
    bounds.setBottom( latitudeFromTile( m_CenterPoint.y() + h / 2, zoom) );
    bounds.setLeft( longitudeFromTile( m_CenterPoint.x() - w / 2, zoom) );
    bounds.setRight( longitudeFromTile( m_CenterPoint.x() + w / 2, zoom) );

    return bounds;
}

void SlippyMap::cancelDownloads()
{  
    foreach(QNetworkReply* reply, m_ActiveRequests)
    {        
        reply->abort();
        reply->deleteLater();
    }

    m_ActiveRequests.clear();
    m_url.clear();
}
