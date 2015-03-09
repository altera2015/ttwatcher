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


// Mercator projection calculations.
static QPointF tileForCoordinate(qreal lat, qreal lng, int zoom)
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


// this little gem takes a spiral walk over an imaginary array and stores the
// indices in the list in reverse order. this is useful for tile loading
// by loading the inner tiles first then the outer tiles.
static void reverseSpiralList ( int start, int end, int startY, int h, QList<QPoint> &list)
{
    /*int x,w;

    if ( h < 0 ) return;

    for ( int i=start; i<end;i++ )
    {
        list.insert(0, QPoint(i,y));
    }

    if ( (start == end && h == 0) || h <= 0 ) return;


    x = end - 1;
    w = end - start - 1;
    start = y + 1;
    end = y + h;

    for (int j=start; j<end; j++)
    {
        list.insert(0, QPoint(x,j));
    }
    if ( (start == end && w == 0) || w <= 0 ) return;


    y = end-1;
    h = end-start-1;
    start = x - 1;
    end = x - w;

    for (int i=start; i>=end; i-- )
    {
        list.insert(0, QPoint(i,y));
    }

    if ( h <= 0 ) return;

    x = end;
    w = start -end +1 ;
    start = y - 1;
    end = y - h;

    for (int j=start; j>=end; j--)
    {
        list.insert(0, QPoint(x,j));
    }

    if ( w <= 0 ) return;

    reverseSpiralList( x+1, x+w, end, start-end+1,list);*/


    for (int y=startY;y < (startY + h); y++)
    {
        for (int x=start;x<=end;x++)
        {
             list.insert(0, QPoint(x,y));
        }
    }
}



SlippyMap::SlippyMap(QObject *parent) : QObject(parent), width(400), height(300), zoom(15),
    latitude(0), longitude(0), locationSet(false)
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

    if ( !locationSet) return;

    if (width <= 0 || height <= 0)
        return;

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



    /*
    if (m_url.isEmpty())
    {
        download();
    }*/

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
    qDebug() << "Pan";
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

void SlippyMap::handleNetworkData(QNetworkReply *reply)
{   
    m_url = 0;

    QPoint tp = reply->request().attribute(QNetworkRequest::User).toPoint();
    QUrl url = reply->url();    

    m_ActiveRequests.removeOne(reply);

    if (!reply->error())
    {
        QByteArray data = reply->readAll();
        processTile(tp, data);
    }
    else
    {
        qDebug() << "SlippyMap::handleData / " << reply->errorString() << (int)reply->error();
        m_Cache->remove( url );
        // download(tp);
    }

    reply->deleteLater();        

    download();

}


void SlippyMap::download()
{
    // qDebug() << "SlippyMap::download / called";

    if (! m_url.isEmpty() )
    {
        // qDebug() << "downloading...";
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
        qDebug() << "SlippyMap::download /x and y less than zero!";
        return true;
    }

    //QString path = "http://tile.openstreetmap.org/%1/%2/%3.png";   
    QString path = "http://otile1.mqcdn.com/tiles/1.0.0/map/%1/%2/%3.png";
    // QString path = "http://otile1.mqcdn.com/tiles/1.0.0/sat/%1/%2/%3.png";
    m_url = QUrl(path.arg(zoom).arg(grab.x()).arg(grab.y()));


    QNetworkCacheMetaData md = m_Cache->metaData(m_url);
    QDateTime now = QDateTime::currentDateTime();

    if ( md.isValid() && now.secsTo(md.expirationDate()) > -3600 * 24 )
    {
        QIODevice * d = m_Cache->data(m_url);
        QByteArray data = d->readAll();
        d->close();
        d->deleteLater();

        processTile(grab, data);

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
    // multiply by tdim to get screen coordiantes.
    QPointF screen = offset * tdim;

    p.setX( static_cast<int>( width / 2.0 - screen.x()   ));
    p.setY( static_cast<int>( height / 2.0 - screen.y()  ));

    return p.x() >= 0 && p.x() <= width && p.y() >= 0 && p.y() <= height;
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
