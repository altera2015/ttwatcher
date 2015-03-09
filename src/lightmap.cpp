#include "lightmap.h"


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::                                                                         :*/
/*::  This routine calculates the distance between two points (given the     :*/
/*::  latitude/longitude of those points). It is being used to calculate     :*/
/*::  the distance between two locations using GeoDataSource(TM) products.   :*/
/*::                                                                         :*/
/*::  Definitions:                                                           :*/
/*::    South latitudes are negative, east longitudes are positive           :*/
/*::                                                                         :*/
/*::  Passed to function:                                                    :*/
/*::    lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)  :*/
/*::    lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)  :*/
/*::    unit = the unit you desire for results                               :*/
/*::           where: 'M' is statute miles                                   :*/
/*::                  'K' is kilometers (default)                            :*/
/*::                  'N' is nautical miles                                  :*/
/*::  Worldwide cities and other features databases with latitude longitude  :*/
/*::  are available at http://www.geodatasource.com                          :*/
/*::                                                                         :*/
/*::  For enquiries, please contact sales@geodatasource.com                  :*/
/*::                                                                         :*/
/*::  Official Web site: http://www.geodatasource.com                        :*/
/*::                                                                         :*/
/*::           GeoDataSource.com (C) All Rights Reserved 2012                :*/
/*::                                                                         :*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#include <math.h>

#define pi 3.14159265358979323846

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}

double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  theta = lon1 - lon2;
  dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  dist = dist * 60 * 1.1515;
  switch(unit) {
    case 'M':
      break;
    case 'K':
      dist = dist * 1.609344;
      break;
    case 'N':
      dist = dist * 0.8684;
      break;
  }
  return (dist);
}



Lightmap::Lightmap(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    proxy = new QGraphicsProxyWidget(this);
    m_Map = new LightMaps();
    proxy->setWidget(m_Map);
    connect(m_Map, SIGNAL(latitudeChanged(double)), this, SIGNAL(latitudeChanged(double)));
    connect(m_Map, SIGNAL(longitudeChanged(double)), this, SIGNAL(longitudeChanged(double)));
    connect(m_Map, SIGNAL(zoomChanged(int)), this, SIGNAL(zoomChanged(int)));
    connect(m_Map, SIGNAL(updated()), this, SIGNAL(updated()));
    connect(m_Map, SIGNAL(dragBegin()), this, SIGNAL(dragBegin()));
    connect(m_Map, SIGNAL(dragEnd()), this, SIGNAL(dragEnd()));

    calcInnerRadius();
}

void Lightmap::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    proxy->resize(newGeometry.width(), newGeometry.height());
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
    calcInnerRadius();
}

double Lightmap::latitude() const
{
    return m_Map->latitude();
}

double Lightmap::longitude() const
{
    return m_Map->longitude();
}

void Lightmap::setLatitude(double latitude)
{
    m_Map->setLatitude(latitude);
}

void Lightmap::setLongitude(double longitude)
{
    m_Map->setLongitude(longitude);
}

void Lightmap::setCenter(double latitude, double longitude)
{
    m_Map->setCenter(latitude, longitude);
}

int Lightmap::zoom() const
{
    return m_Map->zoom();
}

double Lightmap::innerRadius() const
{
    return m_InnerRadius;
}

void Lightmap::setZoom(int zoom)
{
    m_Map->setZoom(zoom);
    calcInnerRadius();
}


QString Lightmap::geoToScreen(qreal latitude, qreal longitude)
{
    QPoint p;

    if ( m_Map->geoToScreen(latitude, longitude, p) )
    {
        QString s;
        s.append(QString("%1,%2").arg(p.x()).arg(p.y()));
        return s;
    }
    return QString();

}

QVariant Lightmap::geoToScreen2(qreal latitude, qreal longitude)
{
    QPoint p;

    if ( m_Map->geoToScreen(latitude, longitude, p) )
    {
        return QVariant( p );
    }
    return 0;
}

bool Lightmap::geoToScreen3(qreal latitude, qreal longitude, QPoint &p)
{
    return m_Map->geoToScreen(latitude, longitude, p);
}

QPoint Lightmap::geoToScreen4(qreal latitude, qreal longitude)
{
    QPoint p;
    m_Map->geoToScreen(latitude, longitude, p);
    return p;
}

int Lightmap::geoToScreenX(qreal latitude, qreal longitude)
{
    QPoint p;
    m_Map->geoToScreen(latitude, longitude, p);
    qDebug() << p.x() << latitude << longitude;
    return p.x();
}

int Lightmap::geoToScreenY(qreal latitude, qreal longitude)
{
    QPoint p;
    m_Map->geoToScreen(latitude, longitude, p);
    qDebug() << p.y() << latitude << longitude;
    return p.y();
}

void Lightmap::setInnerRadius(double innerRadius)
{
    if ( innerRadius == m_InnerRadius ) return;
    m_InnerRadius = innerRadius;
    emit innerRadiusChanged();
}

void Lightmap::calcInnerRadius()
{
    QRectF bounds = m_Map->geoBounds();

    double da = distance( bounds.top(), bounds.left(), bounds.bottom(),  bounds.left() ,'K');
    double db = distance( bounds.top(), bounds.left(), bounds.top(),  bounds.right(), 'K');

    double radius = da > db ? da : db;
    setInnerRadius(radius * 1000);
}

