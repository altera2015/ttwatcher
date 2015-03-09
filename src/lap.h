#ifndef LAP_H
#define LAP_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include "trackpoint.h"

class Lap
{
    quint32 m_TotalSeconds;
    double m_Length;
    int m_Calories;
    int m_HeartBeats; // -1 if not present
    int m_MaxHeartBeats; // -1 if not present
    int m_Cadence; // -1 if not present.
    TrackPointList m_Points;




public:
    Lap();

    quint32 totalSeconds() const;
    void setTotalSeconds( quint32 totalSeconds );

    double length() const;
    void setLength( double length );

    int calories( ) const;
    void setCalories( int calories );

    int heartBeats() const;
    void setHeartBeats( int heartBeats );

    int maximumHeartBeats( ) const;
    void setMaximumHeartBeats( int maximumHeartBeats );

    int cadence() const;
    void setCadence( int cadence );

    void calcTotals();

    TrackPointList & points();

    QString toString() const;

};
typedef QSharedPointer<Lap>LapPtr;
typedef QList<LapPtr>LapList;

#endif // LAP_H
