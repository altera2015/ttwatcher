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

LightMaps::LightMaps(QWidget *parent) :
    QWidget(parent),
    m_Pressed(false),
    m_Snapped(false),
    m_Dragging(false),
    m_Copyright("Map data CCbySA 2009 OpenStreetMap.org contributors")

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
        return;
    m_Pressed = m_Snapped = true;
    m_PressPos = m_DragPos = event->pos();
}

void LightMaps::mouseMoveEvent(QMouseEvent *event)
{
    if (!event->buttons())
        return;

    if (!m_Pressed || !m_Snapped)
    {
        QPoint delta = event->pos() - m_PressPos;
        m_PressPos = event->pos();
        m_Map->pan(delta);
        emit dragBegin();
        m_Dragging = true;
        return;
    }
    else
    {
        const int threshold = 10;
        QPoint delta = event->pos() - m_PressPos;
        if (m_Snapped)
        {
            m_Snapped &= delta.x() < threshold;
            m_Snapped &= delta.y() < threshold;
            m_Snapped &= delta.x() > -threshold;
            m_Snapped &= delta.y() > -threshold;
        }
    }

}

void LightMaps::mouseReleaseEvent(QMouseEvent *)
{
    if ( m_Dragging ) {
        emit dragEnd();
        m_Dragging = false;
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
}

void LightMaps::addCircle(qreal latitude, qreal longitude)
{
    m_Circles.append( QPointF( longitude, latitude ));
}

void LightMaps::setTilePath(const QString &tilePath, const QString &copyright)
{
    m_Map->setTilePath(tilePath);
    m_Copyright = copyright;
}



