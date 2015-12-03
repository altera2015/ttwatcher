#ifndef ELEVATIONADJUSTER_H
#define ELEVATIONADJUSTER_H

class GeoPoint {
public:
    double m_Latitude;
    double m_Longitude;
    double m_Altitude;

    GeoPoint(double latitude = 0.0, double longitude = 0.0, double altitude = 0.0);
    GeoPoint(const GeoPoint & source );
};

class Bridge {
public:
    GeoPoint p1;
    GeoPoint p2;
    double elevation;
    double width;
    bool inside( const GeoPoint & ref );
};


class ElevationAdjuster
{
public:
    ElevationAdjuster();

    double elevation( double latitude, double longitude, double altitude );

};

#endif // ELEVATIONADJUSTER_H
