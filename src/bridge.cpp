#include "bridge.h"
#include <QDebug>
#include <QLineF>
#include <QMarginsF>
#include <QMap>
#include <QPolygonF>
#include "geodistance.h"

#include <QFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDirIterator>
#include <QApplication>

#ifndef M_PI
#define M_PI 3.141592654
#endif

double pointDistance( const QPointF &a, const QPointF b)
{
    return fabs(geodistance(a.y(), a.x(), b.y(), b.x(), 'K')) * 1000;
}

double crossProduct2D(const QPointF &a, const QPointF &b)
{
    return (a.x() * b.y() ) - ( a.y() * b.x()) ;
}

double dotProduct2D(const QPointF &a, const QPointF &b)
{
    return (a.x() * b.x()) + (a.y() * b.y());
}

QPointF calcOffset( const QPointF &orig, double dlng, double dlat )
{
    double R = 6378137.0;

    dlat = dlat/R;
    double div = (R*cos(M_PI*orig.y()/180.0));
    if ( div == 0 )
    {
        return orig;
    }
    dlng = dlng/div;
    double latO = dlat * 180.0/M_PI;
    double lonO = dlng * 180.0/M_PI;
    return QPointF(lonO, latO);
}

bool Bridge::isOnBridge(const QPointF &p, double & elevation, int & index ) const
{
    double closests = 10000000;
    bool closestsWasInBetweenPoints = false;

    for (int i=0;i<m_Bridge.count()-1;i++)
    {
        const BridgePoint &bpA = m_Bridge.at(i);
        const BridgePoint &bpB = m_Bridge.at(i+1);

        QLineF bridgeSegment(bpA.coordinate(), bpB.coordinate());
        QLineF normal = bridgeSegment.normalVector();

        normal.translate( p - normal.p1() ); // place at point.

        QPointF intersectPoint;
        normal.intersect(bridgeSegment, &intersectPoint);

        QPointF p1 = bpB.coordinate() - bpA.coordinate() ;
        QPointF p2 = intersectPoint - bpA.coordinate();


        double dp = dotProduct2D( p1, p2 );
        double ls = dotProduct2D( p1, p1);

        if ( dp >= 0.0 && dp <= ls )
        {
            // in  between the points!
            double distance = pointDistance(intersectPoint, p);
            if ( distance < closests )
            {
                index = i;
                closests = distance;
                QLineF partial(bpA.coordinate(), intersectPoint);
                elevation = bpA.elevation() + (( bpB.elevation() - bpA.elevation() ) * partial.length())  / bridgeSegment.length();
                closestsWasInBetweenPoints = true;
            }
        }
        else
        {
            // outside of the points.
            double distanceToA = (pointDistance(bpA.coordinate(), p));
            double distanceToB = (pointDistance(bpB.coordinate(), p));            
            if ( distanceToA < closests )
            {
                index = i;
                closests = distanceToA;
                elevation = bpA.elevation();
                closestsWasInBetweenPoints = false;
            }
            if ( distanceToB < closests )
            {
                index = i+1;
                closests = distanceToB;
                elevation = bpB.elevation();
                closestsWasInBetweenPoints = false;
            }
        }


    }

    // if the closets points were the first or last point
    // of a bridge we don't extend the bridge by the capture width.
    if ( closestsWasInBetweenPoints )
    {
        return ( closests < m_CaptureWidth );
    }
    else
    {
        if ( index == 0 || ( index + 1 ) == m_Bridge.count())
        {
            return false;
        }
        else
        {
            return ( closests < m_CaptureWidth );
        }
    }
}

void Bridge::initialize()
{
    QPolygonF poly;

    // estimate the bounds of this bridge.
    for (int i=0;i<m_Bridge.count();i++)
    {
        poly.append( m_Bridge.at(i).coordinate() );
    }

    m_Bounds = poly.boundingRect();
    //qDebug() << "BOUNDS "<<m_Bounds << m_Bounds.topLeft() << m_Bounds.bottomRight();

    QPointF offset = calcOffset(m_Bounds.center(), m_CaptureWidth*2, m_CaptureWidth * 2 );
    //qDebug() << "offset "<<offset;
    QMarginsF margins(offset.x(), offset.y(), offset.x(), offset.y());
    //qDebug() << "MARGINS " << margins;
    m_Bounds += margins;
    //qDebug() << "NEW BOUNDS "<<m_Bounds<< m_Bounds.topLeft() << m_Bounds.bottomRight();



    m_Length = 0.0;
    for (int i=0;i<m_Bridge.count()-1;i++)
    {
        const BridgePoint &bpA = m_Bridge.at(i);
        const BridgePoint &bpB = m_Bridge.at(i+1);
        m_Length+=pointDistance(bpA.coordinate(), bpB.coordinate());        
    }
}

Bridge::Bridge(const QString &name, const BridgePointList &bridge, double captureWidth) :
    m_Name(name),
    m_Bridge(bridge),
    m_CaptureWidth(captureWidth)
{    
    initialize();
}

Bridge::Bridge(const QString &name, double captureWidth) : m_Name(name), m_CaptureWidth(captureWidth)
{

}


bool Bridge::isNearBridge(TrackPointList track) const
{
    for(int i=0;i<track.count();i++)
    {
        if ( m_Bounds.contains( track.at(i)->coordinate()) )
        {
            return true;
        }
    }
    return false;
}

bool Bridge::fixBridge(TrackPointList track) const
{
    bool result = false;
    QList<int> indexes;
    int missedPoints = 0;
    QMap<int, double> elevationMap;
    TrackPointPtr lastPoint;
    double lengthOnBridge = 0.0;

    for(int i=0;i<track.count();i++)
    {
        int bridgeIndex;
        double elevation;

        if (isOnBridge(track.at(i)->coordinate(), elevation, bridgeIndex))
        {
            if ( lastPoint )
            {
                lengthOnBridge += pointDistance(lastPoint->coordinate(), track.at(i)->coordinate());
            }
            else
            {
                lengthOnBridge = 0.0;
            }

            lastPoint = track.at(i);
            elevationMap[i] = elevation;
            missedPoints = 0;
            if ( !indexes.contains(bridgeIndex))
            {
                indexes.append(bridgeIndex);
            }
        }
        else
        {
            if ( indexes.count() > 0 )
            {
                missedPoints++;
                if ( missedPoints > 10 )
                {                    
                    if ( lengthOnBridge > m_Length * 0.9 )
                    {
                        // we ran the bridge.
                        QMap<int, double>::iterator it = elevationMap.begin();
                        for (; it!=elevationMap.end(); it++ )
                        {
                            track[it.key()]->setAltitude(it.value());
                        }
                        result = true;
                    }
                    else
                    {
                        // nah something weird happened.
                    }
                    elevationMap.clear();
                    missedPoints = 0;
                    indexes.clear();
                    lengthOnBridge = 0.0;
                }
            }
        }
    }
    return result;
}

const QString Bridge::name() const
{
    return m_Name;
}

void Bridge::setName(const QString &name)
{
    m_Name = name;
}

const double Bridge::captureWidth() const
{
    return m_CaptureWidth;
}

void Bridge::setCaptureWidth(double captureWidth)
{
    m_CaptureWidth = captureWidth;
}

BridgePointList &Bridge::points()
{
    return m_Bridge;
}

const BridgePointList &Bridge::constPoints() const
{
    return m_Bridge;
}

bool Bridge::loadBridgeFile(const QString &filename, BridgeList &bridgeList)
{
    qDebug() << "Bridge::loadBridgeFile / " << filename;
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument d;
    QJsonParseError pe;
    d = QJsonDocument::fromJson(data, &pe);
    if ( pe.error != QJsonParseError::NoError )
    {
        qDebug() << "BRIDGE LIST ERROR: " << pe.errorString() << " for file " << filename;
        return false;
    }


    foreach (const QJsonValue & v, d.array())
    {
        /*
        [
            {
                "name": "Jensen Beach Causeway",
                "captureWidth": 20,
                "points": [
                    [-80.2220, 27.2535, 1.0],
                    [-80.2193, 27.2531, 20.0],
                    [-80.2146, 27.2544, 1.0]
                ]
            }
        ]
        */
        QJsonObject jBridge = v.toObject();
        if ( !jBridge.contains("name"))
        {
            qDebug() << "Bridge loading failed: name MISSING for file " << filename;
            return false;
        }
        if ( !jBridge.contains("captureWidth"))
        {
            qDebug() << "Bridge loading failed: captureWidth MISSING for file " << filename;
            return false;
        }
        if ( !jBridge.contains("points"))
        {
            qDebug() << "Bridge loading failed: points MISSING for file " << filename;
            return false;
        }
        QList<BridgePoint> bps;
        foreach ( const QJsonValue & jbp, jBridge["points"].toArray() )
        {
            QJsonArray jbpa = jbp.toArray();
            if ( jbpa.count() != 3 )
            {
                qDebug() << "BRIDGE " << jBridge["name"].toString() << " INVALID COORDINATES MUST BE LNG, LAT, ELE for file " << filename;
                return false;
            }
            bps.append( BridgePoint( QPointF(jbpa[0].toDouble(), jbpa[1].toDouble()), jbpa[2].toDouble()));
        }

        Bridge b(jBridge["name"].toString(), bps, jBridge["captureWidth"].toDouble());
        bridgeList.append(b);
    }

    return true;
}

bool Bridge::loadBridgeFiles(const QString &dir, BridgeList &bridgeList)
{
    qDebug() << "Bridge::loadBridgeFiles / " << dir;
    bool allOk = true;
    QDirIterator it(dir, QStringList() << "*.bridges", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QString fn = it.next();
        allOk &= loadBridgeFile(fn, bridgeList);
    }
    return allOk;
}

bool Bridge::loadBridgeFiles(BridgeList &bridgeList)
{
    bool allOk = true;    
    QString dir;

#ifdef TT_DEBUG
    dir = QApplication::applicationDirPath() + "/../../../src";
#else
    dir = QApplication::applicationDirPath();
#endif
    allOk &= loadBridgeFiles(dir, bridgeList);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() ;
#else
    dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator();
#endif

    allOk &= loadBridgeFiles(dir, bridgeList);
    return allOk;
}

bool Bridge::saveBridgeFile(const QString &filename, BridgeList &bridgeList)
{
    QFile f(filename);
    if ( !f.open(QIODevice::WriteOnly| QIODevice::Truncate))
    {
        return false;
    }

    QJsonArray bridges;

    foreach ( const Bridge & bridge, bridgeList)
    {
        QJsonObject jBridge;

        jBridge["name"] = bridge.name();
        jBridge["captureWidth"] = bridge.captureWidth();

        QJsonArray jPoints;

        foreach ( const BridgePoint & bp, bridge.constPoints() )
        {
            QJsonArray jPoint;
            jPoint.append( bp.coordinate().x() );
            jPoint.append( bp.coordinate().y() );
            jPoint.append( bp.elevation() );
            jPoints.append(jPoint);
        }
        jBridge["points"] = jPoints;

        bridges.append(jBridge);
    }

    QJsonDocument d;
    d.setArray(bridges);
    f.write( d.toJson() );
    f.close();
    return true;
}


