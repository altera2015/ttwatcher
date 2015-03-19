#ifndef LIGHTMAPS_H
#define LIGHTMAPS_H


#include <QtCore>
#include <QtGui>
#include <QtNetwork>
#include <QDebug>
#include <QWidget>
#include <QList>

#include "SlippyMap.h"

class LightMaps: public QWidget
{
    Q_OBJECT

    QList<QRectF> m_Lines;
    QList<QPointF> m_Circles;

public:
    LightMaps(QWidget *parent = 0);

    void setCenter(qreal lat, qreal lng);
    void setCenter(int zoom, qreal lat, qreal lng);

    void setLatitude( qreal lat);
    void setLongitude( qreal lng );
    qreal latitude() const;
    qreal longitude() const;
    void setZoom ( int zoom );
    int zoom( ) const;
    bool geoToScreen( qreal latitude, qreal longitude, QPoint & p ) const;
    int boundsToZoom ( const QRectF & bounds );
    QRectF geoBounds();

    void clearLines();
    void addLine( qreal latitude1, qreal longitude1, qreal latitude2, qreal longitude2);

    void clearCircles();
    void addCircle( qreal latitude, qreal longitude );    

private slots:
    void updateMap(const QRect &r);


protected:

    void resizeEvent(QResizeEvent *);
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *);
    void mousePressEvent(QMouseEvent *event) ;
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent * event);

signals:
    void latitudeChanged( double lat );
    void longitudeChanged( double lng );
    void zoomChanged ( int zoom );
    void updated();
    void dragEnd();
    void dragBegin();

private:
    SlippyMap *m_Map;
    bool pressed;
    bool snapped;
    bool dragging;
    QPoint pressPos;
    QPoint dragPos;
};

#endif // LIGHTMAPS_H
