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

    void setTilePath( const QString & tilePath, const QString & copyright );

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
    bool m_Pressed;
    bool m_Snapped;
    bool m_Dragging;
    QPoint m_PressPos;
    QPoint m_DragPos;
    QString m_Copyright;
};

#endif // LIGHTMAPS_H
