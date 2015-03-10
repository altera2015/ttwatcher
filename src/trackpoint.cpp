#include "trackpoint.h"


int TrackPoint::calories() const
{
    return m_Calories;
}

void TrackPoint::setCalories(int Calories)
{
    m_Calories = Calories;
}

float TrackPoint::cummulativeDistance() const
{
    return m_CummulativeDistance;
}

void TrackPoint::setCummulativeDistance(float CummulativeDistance)
{
    m_CummulativeDistance = CummulativeDistance;
}

void TrackPoint::setSpeed(double speed)
{
    m_Speed = speed;
}

double TrackPoint::speed() const
{
    return m_Speed;
}

bool TrackPoint::hasGPS()
{
    return m_Latitude!=0 && m_Longitude!=0;
}

QString TrackPoint::toString() const
{
    return QString("Time %1, Latitude %2, Longitude %3, Altitude %4, HeartRate %5, Cadence %6, Calories %7, Cummulative Distance %8")\
            .arg(time().toString())\
            .arg(latitude())\
            .arg(longitude())\
            .arg(altitude())\
            .arg(heartRate())\
            .arg(cadence())\
            .arg(calories())\
            .arg(cummulativeDistance());
}

TrackPoint::TrackPoint() :
    m_Latitude(0.0),
    m_Longitude(0.0),
    m_Altitude(0.0),
    m_HeartRate(-1),
    m_Cadence(-1),
    m_Calories(-1),    
    m_CummulativeDistance(0.0),
    m_Speed(0.0)
{
}

QDateTime TrackPoint::time() const
{
    return m_Time;
}

void TrackPoint::setTime(const QDateTime &Time)
{
    m_Time = Time;
}

double TrackPoint::latitude() const
{
    return m_Latitude;
}

void TrackPoint::setLatitude(double Latitude)
{
    m_Latitude = Latitude;
}

double TrackPoint::longitude() const
{
    return m_Longitude;
}

void TrackPoint::setLongitude(double Longitude)
{
    m_Longitude = Longitude;
}

double TrackPoint::altitude() const
{
    return m_Altitude;
}

void TrackPoint::setAltitude(double Altitude)
{
    m_Altitude = Altitude;
}

int TrackPoint::heartRate() const
{
    return m_HeartRate;
}

void TrackPoint::setHeartRate(int HeartRate)
{
    m_HeartRate = HeartRate;
}

int TrackPoint::cadence() const
{
    return m_Cadence;
}

void TrackPoint::setCadence(int Cadence)
{
    m_Cadence = Cadence;
}

