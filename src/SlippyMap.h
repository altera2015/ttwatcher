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
    void setTilePath( const QString & tilePath );

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
    QString m_TilePath;
};

#endif // SLIPPYMAP_H
