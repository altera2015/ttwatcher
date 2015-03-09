#ifndef TRACKPOINT_H
#define TRACKPOINT_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include <QDateTime>

class TrackPoint
{
    QDateTime m_Time;
    double m_Latitude;
    double m_Longitude;
    double m_Altitude;
    int m_HeartRate;
    int m_Cadence;
    int m_Calories;
    float m_IncrementalDistance;
    float m_CummulativeDistance;
    double m_Speed;

public:
    TrackPoint();


    QDateTime time() const;
    void setTime(const QDateTime &time);
    double latitude() const;
    void setLatitude(double latitude);
    double longitude() const;
    void setLongitude(double longitude);
    double altitude() const;
    void setAltitude(double altitude);
    int heartRate() const;
    void setHeartRate(int heartRate);
    int cadence() const;
    void setCadence(int cadence);
    int calories() const;
    void setCalories(int calories);
    float incrementalDistance() const;
    void setIncrementalDistance(float incrementalDistance);
    float cummulativeDistance() const;
    void setCummulativeDistance(float cummulativeDistance);

    void setSpeed( double speed );
    double speed() const;

    bool hasGPS();

    QString toString() const;

};
typedef QSharedPointer<TrackPoint>TrackPointPtr;
typedef QList<TrackPointPtr>TrackPointList;
#endif // TRACKPOINT_H
