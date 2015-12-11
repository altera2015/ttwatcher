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
    void setCoordinate( const QPointF & p ) {
        m_Coordinate = p;
    }
    void setLatitude(double latitude ) {
        m_Coordinate.setY(latitude);
    }
    void setLongitude(double longitude) {
        m_Coordinate.setX(longitude);
    }
    double elevation() const {
        return m_Elevation;
    }
    void setElevation( double elevation ) {
        m_Elevation = elevation;
    }
};

class Bridge;
typedef QList<Bridge>BridgeList;
typedef QList<BridgePoint> BridgePointList;

class Bridge {
    QString m_Name;
    BridgePointList m_Bridge;
    QRectF m_Bounds;
    double m_Length;

    // meters that one has to be from the bridge path to be concidered on the bridge.
    double m_CaptureWidth;

    bool isOnBridge(const QPointF&p, double &elevation, int &index) const;
    void initialize();
public:
    Bridge( const QString & name, const BridgePointList & bridge, double captureWidth );
    Bridge( const QString & name = "", double captureWidth = 20.0 );
    bool isNearBridge( TrackPointList track ) const;
    bool fixBridge ( TrackPointList track ) const;

    const QString name() const;
    void setName(const QString &name);
    const double captureWidth() const;
    void setCaptureWidth( double captureWidth );
    BridgePointList & points();
    const BridgePointList & constPoints() const;

    static bool loadBridgeFile(const QString &filename, BridgeList & bridgeList);
    static bool loadBridgeFiles(const QString & dir, BridgeList & bridgeList);
    static bool loadBridgeFiles(BridgeList &bridgeList);

    static bool saveBridgeFile(const QString &filename, BridgeList & bridgeList);
};



#endif // BRIDGE_H
