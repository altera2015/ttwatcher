#ifndef LIGHTMAP_H
#define LIGHTMAP_H

#include <QDeclarativeItem>
#include "Lightmaps.h"

class Lightmap : public QDeclarativeItem
{
    Q_OBJECT

    LightMaps * m_Map;
    QGraphicsProxyWidget *proxy;
    double m_InnerRadius;

    void calcInnerRadius();
    void setInnerRadius( double innerRadius );
public:
    explicit Lightmap(QDeclarativeItem *parent = 0);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

    Q_PROPERTY( double latitude READ latitude WRITE setLatitude NOTIFY latitudeChanged )
    Q_PROPERTY( double longitude READ longitude WRITE setLongitude NOTIFY longitudeChanged )
    Q_PROPERTY( int zoom READ zoom WRITE setZoom NOTIFY zoomChanged )
    Q_PROPERTY( double innerRadius READ innerRadius NOTIFY innerRadiusChanged ) // inner radius in meters.
    double latitude() const;
    double longitude() const;
    int zoom() const;
    double innerRadius() const;
    void setLatitude( double latitude );
    void setLongitude( double longitude );
    void setZoom( int zoom );

signals:
    void latitudeChanged( double lat );
    void longitudeChanged( double lng );
    void zoomChanged ( int zoom );
    void updated();
    void dragBegin();
    void dragEnd();
    void innerRadiusChanged();

public slots:

    void setCenter( double latitude, double longitude );
    QString geoToScreen( qreal latitude, qreal longitude );
    QVariant geoToScreen2(qreal latitude, qreal longitude );
    bool geoToScreen3( qreal latitude, qreal longitude, QPoint& p );
    QPoint geoToScreen4( qreal latitude, qreal longitude );

    int geoToScreenX(qreal latitude, qreal longitude);
    int geoToScreenY(qreal latitude, qreal longitude);


};

#endif // LIGHTMAP_H
