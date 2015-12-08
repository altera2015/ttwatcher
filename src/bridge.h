#ifndef BRIDGE_H
#define BRIDGE_H

#include <QPointF>
#include <QList>
#include <QRectF>
#include "trackpoint.h"

class BridgePoint{
    QPointF m_Coordinate;
    double m_Elevation;
public:
    BridgePoint( const QPointF & coordinate, double elevation ) : m_Coordinate(coordinate), m_Elevation(elevation) {
    }
    BridgePoint( double latitude, double longitude, double elevation ) : m_Coordinate(longitude, latitude), m_Elevation(elevation) {
    }
    BridgePoint ( ) : m_Elevation(0.0) {
    }

    const QPointF & coordinate() const {
        return m_Coordinate;
    }
    double elevation() const {
        return m_Elevation;
    }
};

class Bridge;
typedef QList<Bridge>BridgeList;

class Bridge {
    QString m_Name;
    QList<BridgePoint> m_Bridge;
    QRectF m_Bounds;
    double m_Length;

    // meters that one has to be from the bridge path to be concidered on the bridge.
    double m_CaptureWidth;

    bool isOnBridge(const QPointF&p, double &elevation, int &index) const;
    void initialize();
public:
    Bridge( const QString & name, const QList<BridgePoint> & bridge, double captureWidth );
    Bridge();
    bool isNearBridge( TrackPointList track ) const;
    bool fixBridge ( TrackPointList track ) const;
    static bool loadBridgeFile(const QString &filename, BridgeList & bridgeList);
    static bool loadBridgeFiles(const QString & dir, BridgeList & bridgeList);
    static bool loadBridgeFiles(BridgeList &bridgeList);
};



#endif // BRIDGE_H
