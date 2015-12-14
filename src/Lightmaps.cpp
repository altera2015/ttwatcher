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

#include "Lightmaps.h"
#include <QApplication>

LightMaps::LightMaps(QWidget *parent) :
    QWidget(parent),

    m_Snapped(false),
    m_Dragging(false),
    m_Copyright("Map data CCbySA 2009 OpenStreetMap.org contributors"),
    m_SelectedCircle(-1),
    m_AllowCircleDragging(false),
    m_InCircleDragging(false)
{
    m_Map = new SlippyMap(this);
    connect(m_Map, SIGNAL(updated(QRect)), SLOT(updateMap(QRect)));
    connect(m_Map, SIGNAL(updated(QRect)), this, SIGNAL(updated()));
}

void LightMaps::allowCircleDragging(bool allow)
{
    m_AllowCircleDragging = allow;
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

void LightMaps::setCenter(const QPointF &center)
{
    setCenter( center.y(), center.x() );
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

void LightMaps::setCenter(int zoom, const QPointF &center)
{
    setCenter(zoom, center.y(), center.x());
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

QPointF LightMaps::center() const
{
    return QPointF(m_Map->longitude, m_Map->latitude);
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

    QPen red_pen;
    red_pen.setColor(Qt::red);
    red_pen.setWidth(3);
    p.setPen(red_pen);

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

    QPen blue_pen;
    blue_pen.setColor(Qt::blue);
    blue_pen.setWidth(3);
    p.setPen(blue_pen);

    for (int i=0;i<m_Circles.count();i++)
    {
        const QPointF &pf = m_Circles[i];
        QPoint p1;
        bool centerVisible = geoToScreen( pf.y(), pf.x(), p1);

        if ( !centerVisible )
        {
            continue;
        }


        if ( i == m_SelectedCircle )
        {
            p.setPen(red_pen);
        }
        else
        {
            p.setPen(blue_pen);
        }

        p.drawEllipse(p1, 6,6);

    }


    QRect r = rect();

    r.translate(10, -10);

    p.setPen(Qt::white);
    p.drawText(r,  Qt::AlignBottom | Qt::TextWordWrap,
               m_Copyright);


    p.setPen(Qt::black);

    r.translate(1,1);
    p.drawText(r,  Qt::AlignBottom | Qt::TextWordWrap,
               m_Copyright);

    p.end();

}

void LightMaps::timerEvent(QTimerEvent *)
{
    update();
}

void LightMaps::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() != Qt::LeftButton)
    {
        return;
    }

    m_Snapped = true;
    m_PressPos = m_DragPos = event->pos();

    if ( m_AllowCircleDragging )
    {
        int newIndex = circleHitTest(event->pos());
        if ( newIndex >= 0 )
        {
            m_SelectedCircle = newIndex;
            emit circleSelected(m_SelectedCircle, m_DragPos);
        }
        else
        {
            if (m_SelectedCircle >= 0 )
            {
                emit circleUnselected(m_SelectedCircle);
                m_SelectedCircle = -1;
            }
        }
        update();
        return;
    }
}

void LightMaps::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::NoButton )
    {
        QPointF geo;
        screenToGeo(event->pos(), geo);
        emit mouseMove(geo);
        return;
    }


    if (m_Snapped)
    {        
        QPoint delta = event->pos() - m_PressPos;
        m_Snapped = (delta.x() * delta.x() + delta.y() * delta.y()) < 25;

        if ( m_Snapped )
        {
            return;
        }

        if ( m_AllowCircleDragging && m_SelectedCircle >= 0 )
        {
            // not dragging, we're in a circle!
            m_InCircleDragging = true;
            return;
        }

        m_Dragging = true;
        emit dragBegin();

    }
    else
    {
        if ( m_AllowCircleDragging && m_InCircleDragging )
        {
            screenToGeo(event->pos(), m_Circles[m_SelectedCircle] );
            update();
            return;
        }

        // if we are not dragging a circle we are panning the map.
        QPoint delta = event->pos() - m_PressPos;
        m_PressPos = event->pos();
        m_Map->pan(delta);                     
    }
}

void LightMaps::mouseReleaseEvent(QMouseEvent *event)
{
    if ( m_Dragging )
    {
        emit dragEnd();
        m_Dragging = false;
        m_Snapped = false;
        update();
        return;
    }

    if ( m_InCircleDragging )
    {
        emit circleMoved(m_SelectedCircle, m_Circles[m_SelectedCircle]);
        m_InCircleDragging = false;
        update();
        return;
    }

    qreal latitude, longitude;
    screenToGeo( event->pos(), latitude, longitude );
    mouseUp( latitude, longitude );
}

void LightMaps::keyPressEvent(QKeyEvent *event)
{
    switch ( event->key() )
    {
    case Qt::Key_Left:
        m_Map->pan(QPoint(20,0));
        break;
    case Qt::Key_Right:
        m_Map->pan(QPoint(-20,0));
        break;
    case Qt::Key_Up:
        m_Map->pan(QPoint(0,20));
        break;
    case Qt::Key_Down:
        m_Map->pan(QPoint(-20,0));
        break;
    }
}

void LightMaps::wheelEvent(QWheelEvent *event)
{
    QPoint pos = event->pos();
    QPointF geo;
    screenToGeo(pos, geo); // coordinates under cursor currently.
    // update zoom level
    int zoom = this->zoom();
    if ( event->delta() < 0 )
    {
        zoom--;
    }
    else
    {
        zoom++;
    }
    if ( zoom < 0 )
    {
        zoom = 0;
    }
    if ( zoom > 18 )
    {
        zoom = 18;
    }
    QPoint viewPort ( this->width(), this->height());
    QPoint newCenter = viewPort - pos;
    double latitude, longitude;
    m_Map->screenToGeo( SlippyMap::tileForCoordinate(geo,zoom), zoom, newCenter, latitude, longitude);
    setCenter(zoom, latitude, longitude);
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

bool LightMaps::geoToScreen(const QPointF &circle, QPoint &p) const
{
    return m_Map->geoToScreen(circle.y(), circle.x(), p);
}

bool LightMaps::geoToScreen(qreal latitude, qreal longitude, QPoint &p) const
{
    return m_Map->geoToScreen(latitude, longitude, p);
}

void LightMaps::screenToGeo(const QPoint &p, qreal &latitude, qreal &longitude) const
{
    m_Map->screenToGeo(p, latitude, longitude);
}

void LightMaps::screenToGeo(const QPoint &p, QPointF &geo) const
{
    qreal lat, lng;
    m_Map->screenToGeo(p, lat, lng);
    geo.setX(lng);
    geo.setY(lat);
}

int LightMaps::boundsToZoom(const QRectF &bounds)
{
    return m_Map->boundsToZoom(bounds);
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
    m_SelectedCircle = -1;
}

void LightMaps::addCircle(qreal latitude, qreal longitude)
{
    m_Circles.append( QPointF( longitude, latitude ));
}

void LightMaps::addCircle(const QPointF &geo)
{
    m_Circles.append( geo );
}

const QList<QPointF> LightMaps::circles() const
{
    return m_Circles;
}

void LightMaps::removeCircle(int index)
{
    if ( index < 0 || index >= m_Circles.count() )
    {
        return;
    }

    if ( m_SelectedCircle >= 0 )
    {
        emit circleUnselected(m_SelectedCircle);
    }

    m_SelectedCircle = -1;
    m_Circles.removeAt(index);
    update();
}

void LightMaps::selectCircle(int index)
{
    m_SelectedCircle = index;
    update();
}

int LightMaps::circleHitTest(const QPoint &p, int pixelRadius)
{
    for (int i=0;i<m_Circles.count();i++)
    {
        QPoint cp;
        if (geoToScreen(m_Circles[i], cp))
        {
            QPoint delta = p - cp;
            if ( delta.x() * delta.x() + delta.y() * delta.y() <= pixelRadius * pixelRadius )
            {
                return i;
            }
        }
    }
    return -1;
}

void LightMaps::setTilePath(const QString &tilePath, const QString &copyright)
{
    m_Map->setTilePath(tilePath);
    m_Copyright = copyright;
}



