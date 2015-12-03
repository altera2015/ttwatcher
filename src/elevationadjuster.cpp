#include "elevationadjuster.h"

ElevationAdjuster::ElevationAdjuster()
{
}


GeoPoint::GeoPoint(double latitude, double longitude, double altitude) :
    m_Latitude(latitude),
    m_Longitude(longitude),
    m_Altitude(altitude)
{

}

GeoPoint::GeoPoint(const GeoPoint &source) :
    m_Latitude(source.m_Latitude),
    m_Longitude(source.m_Longitude),
    m_Altitude(source.m_Altitude)
{

}


