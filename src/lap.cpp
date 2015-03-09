#include "lap.h"
#include "geodistance.h"

Lap::Lap() :
    m_TotalSeconds(0),
    m_Length(0.0),
    m_Calories(0),
    m_HeartBeats(-1),
    m_MaxHeartBeats(-1),
    m_Cadence(-1)
{
}

quint32 Lap::totalSeconds() const
{
    return m_TotalSeconds;
}

void Lap::setTotalSeconds(quint32 totalSeconds)
{
    m_TotalSeconds = totalSeconds;
}

double Lap::length() const
{
    return m_Length;
}

void Lap::setLength(double length)
{
    m_Length = length;
}

int Lap::calories() const
{
    return m_Calories;
}

void Lap::setCalories(int calories)
{
    m_Calories = calories;
}

int Lap::heartBeats() const
{
    return m_HeartBeats;
}

void Lap::setHeartBeats(int heartBeats)
{
    m_HeartBeats = heartBeats;
}

int Lap::maximumHeartBeats() const
{
    return m_MaxHeartBeats;
}

void Lap::setMaximumHeartBeats(int maximumHeartBeats)
{
    m_MaxHeartBeats = maximumHeartBeats;
}

int Lap::cadence() const
{
    return m_Cadence;
}

void Lap::setCadence(int cadence)
{
    m_Cadence = cadence;
}

void Lap::calcTotals()
{
    if ( m_Points.count() == 0 )
    {
        m_MaxHeartBeats = -1;
        m_Cadence = -1;
        m_Calories = -1;
        return;
    }

    quint64 duration = m_Points.first()->time().secsTo( m_Points.last()->time());
    quint64 heartBeats = 0;
    int heartBeatCount = 0;
    quint64 cadence = 0;
    int cadenceCount = 0;
    float distance = 0;

    for(int i=0;i<m_Points.count();i++)
    {
        TrackPointPtr p = m_Points.at(i);

        if ( i > 0 )
        {
            TrackPointPtr lastp = m_Points.at(i-1);
            if ( p->hasGPS() && lastp->hasGPS() )
            {
                double incrementalDistance = geodistance( p->latitude(), p->longitude(), lastp->latitude(), lastp->longitude(), 'K' ) * 1000;
                distance += incrementalDistance;
                if ( p->incrementalDistance()== 0 )
                {
                    p->setIncrementalDistance(incrementalDistance);
                    p->setCummulativeDistance(distance);
                }
            }
            else
            {
                if ( p->incrementalDistance() == 0 )
                {
                    p->setIncrementalDistance( lastp->cummulativeDistance() - distance );
                    distance = p->cummulativeDistance();
                }
                else
                {
                    if ( p->cummulativeDistance()==0)
                    {
                        distance += p->incrementalDistance();
                        p->setCummulativeDistance(distance);
                    }
                }
            }



        }

        if ( p->heartRate() > 0 )
        {
            heartBeatCount++;
            heartBeats += p->heartRate();
            if ( p->heartRate() > m_MaxHeartBeats )
            {
                m_MaxHeartBeats = p->heartRate();
            }
        }
        if ( cadence > 0 )
        {
            cadence += p->cadence();
            cadenceCount++;
        }
    }

    setTotalSeconds( duration );
    if ( heartBeatCount>0)
    {
        setHeartBeats( heartBeats / heartBeatCount );
    }
    if ( duration > 0 )
    {
        setCadence( cadence * 60 / duration );
    }
    setLength( distance );
}

TrackPointList &Lap::points()
{
    return m_Points;
}

QString Lap::toString() const
{
    QString str = QString("Total Time %1, Distance %2, Calories %3, Heartbeat %4, Max HeartBeat %5, Cadence %6\n")
            .arg(totalSeconds())
            .arg(length())
            .arg(calories())
            .arg(heartBeats())
            .arg(maximumHeartBeats())
            .arg(cadence());

    foreach ( TrackPointPtr tp, m_Points )
    {
        str += "\t" + tp->toString() + "\n";
    }
    return str;

}
