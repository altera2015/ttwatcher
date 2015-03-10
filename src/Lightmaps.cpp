#include "Lightmaps.h"

LightMaps::LightMaps(QWidget *parent) : QWidget(parent), pressed(false), snapped(false), dragging(false)
{
    m_Map = new SlippyMap(this);
    connect(m_Map, SIGNAL(updated(QRect)), SLOT(updateMap(QRect)));
    connect(m_Map, SIGNAL(updated(QRect)), this, SIGNAL(updated()));
}

void LightMaps::setCenter(qreal lat, qreal lng)
{
    if ( m_Map->latitude == lat && m_Map->longitude == lng ) return;

    m_Map->latitude = lat;
    m_Map->longitude = lng;
    m_Map->locationSet = true;
    m_Map->cancelDownloads();
    m_Map->invalidate();

    emit latitudeChanged(lat);
    emit longitudeChanged(lng);
}

void LightMaps::setCenter(int zoom, qreal lat, qreal lng)
{
    if ( m_Map->latitude == lat && m_Map->longitude == lng && m_Map->zoom == zoom ) return;

    if ( zoom < 0 ) zoom = 0;
    if ( zoom > 18 ) zoom = 18;


    m_Map->latitude = lat;
    m_Map->longitude = lng;
    m_Map->zoom = zoom;
    m_Map->locationSet = true;
    m_Map->cancelDownloads();
    m_Map->invalidate();

    emit latitudeChanged(lat);
    emit longitudeChanged(lng);
    emit zoomChanged(zoom);
}

void LightMaps::setLatitude( qreal lat)
{
    if ( m_Map->latitude == lat ) return;

    m_Map->latitude = lat;
    m_Map->locationSet = true;
    m_Map->cancelDownloads();
    m_Map->invalidate();

    emit latitudeChanged(lat);
}

void LightMaps::setLongitude( qreal lng )
{
    if ( m_Map->longitude == lng ) return;

    m_Map->longitude = lng;
    m_Map->locationSet = true;
    m_Map->cancelDownloads();
    m_Map->invalidate();
    emit longitudeChanged(lng);
}

qreal LightMaps::latitude() const
{
    return m_Map->latitude;
}

qreal LightMaps::longitude() const
{
    return m_Map->longitude;
}

void LightMaps::updateMap(const QRect &r)
{
    update(r);
}
void LightMaps::resizeEvent(QResizeEvent *)
{
    m_Map->width = width();
    m_Map->height = height();
    m_Map->invalidate();
}

void LightMaps::paintEvent(QPaintEvent *event)
{
    QPainter p;    
    p.begin(this);
    p.setRenderHint(QPainter::Antialiasing);

    m_Map->render(&p, event->rect());

    QPen pen;
    pen.setColor(Qt::red);
    pen.setWidth(3);
    p.setPen(pen);

    QPolygon polygon;
    // Draw graphics over the top.
    foreach ( const QRectF& line, m_Lines )
    {
        double latitude1 = line.top();
        double longitude1 = line.left();
        double latitude2 = line.bottom();
        double longitude2 = line.right();

        QPoint p1,p2;

        bool p1Visible = geoToScreen(latitude1, longitude1, p1);
        bool p2Visible = geoToScreen(latitude2, longitude2, p2);

        if ( !p1Visible && !p2Visible )
        {
            // just skip it.
            continue;
        }

        p.drawLine(p1,p2);


    }

    QPen pen_blue;
    pen_blue.setColor(Qt::blue);
    pen_blue.setWidth(3);
    p.setPen(pen_blue);

    foreach ( const QPointF& pf, m_Circles )
    {
        QPoint p1;
        bool centerVisible = geoToScreen( pf.y(), pf.x(), p1);

        if ( !centerVisible )
        {
            continue;
        }



        p.drawEllipse(p1, 6,6);

    }


    p.setPen(Qt::darkGray);

    p.drawText(rect(),  Qt::AlignBottom | Qt::TextWordWrap,
               "Map data CCbySA 2009 OpenStreetMap.org contributors");
    p.end();

}

void LightMaps::timerEvent(QTimerEvent *)
{
    update();
}

void LightMaps::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton)
        return;
    pressed = snapped = true;
    pressPos = dragPos = event->pos();
}

void LightMaps::mouseMoveEvent(QMouseEvent *event)
{
    if (!event->buttons())
        return;

    if (!pressed || !snapped)
    {
        QPoint delta = event->pos() - pressPos;
        pressPos = event->pos();
        m_Map->pan(delta);
        emit dragBegin();
        dragging = true;
        return;
    }
    else
    {
        const int threshold = 10;
        QPoint delta = event->pos() - pressPos;
        if (snapped)
        {
            snapped &= delta.x() < threshold;
            snapped &= delta.y() < threshold;
            snapped &= delta.x() > -threshold;
            snapped &= delta.y() > -threshold;
        }
    }

}

void LightMaps::mouseReleaseEvent(QMouseEvent *)
{
    if ( dragging ) {
        emit dragEnd();
        dragging = false;
    }

    update();
}

void LightMaps::keyPressEvent(QKeyEvent *event)
{

    if (event->key() == Qt::Key_Left)
        m_Map->pan(QPoint(20, 0));
    if (event->key() == Qt::Key_Right)
        m_Map->pan(QPoint(-20, 0));
    if (event->key() == Qt::Key_Up)
        m_Map->pan(QPoint(0, 20));
    if (event->key() == Qt::Key_Down)
        m_Map->pan(QPoint(0, -20));

}

void LightMaps::wheelEvent(QWheelEvent *event)
{
    if ( event->delta() < 0 )
    {
        setZoom( zoom() - 1 );
    }
    else
    {
        setZoom( zoom() + 1 );
    }

}

void LightMaps::setZoom(int zoom)
{
    if ( zoom < 0 ) return;
    if ( zoom > 18 ) return;


    if ( m_Map->zoom == zoom ) return;
    m_Map->zoom = zoom;
    m_Map->cancelDownloads();
    m_Map->invalidate();
    emit zoomChanged(zoom);
}

int LightMaps::zoom() const
{
    return m_Map->zoom;
}

bool LightMaps::geoToScreen(qreal latitude, qreal longitude, QPoint &p) const
{
    return m_Map->geoToScreen(latitude, longitude, p);
}

QRectF LightMaps::geoBounds()
{
    return m_Map->geoBounds();
}

void LightMaps::clearLines()
{
    m_Lines.clear();
}

void LightMaps::addLine(qreal latitude1, qreal longitude1, qreal latitude2, qreal longitude2)
{
    QRectF line( QPointF(longitude1, latitude1),QPointF(longitude2, latitude2) );
    m_Lines.append(line);
}

void LightMaps::clearCircles()
{
    m_Circles.clear();
}

void LightMaps::addCircle(qreal latitude, qreal longitude)
{
    m_Circles.append( QPointF( longitude, latitude ));
}



