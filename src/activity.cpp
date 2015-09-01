#include "activity.h"


Activity::Activity() :
    m_Duration(0),
    m_Distance(0)
{
}

QDateTime Activity::date() const
{
    return m_Date;
}

void Activity::setDate(const QDateTime &date)
{
    m_Date = date;
}

LapList &Activity::laps()
{
    return m_Laps;
}

QString Activity::notes() const
{
    return m_Notes;
}

void Activity::setNotes(const QString &notes)
{
    m_Notes = notes;
}

Activity::Sport Activity::sport() const
{
    return m_Sport;
}

void Activity::setSport(Activity::Sport sport)
{
    m_Sport = sport;
}

QString Activity::sportString() const
{
    return sportToString(m_Sport);
}

QString Activity::sportToString(Activity::Sport sport)
{
    switch (sport)
    {
    case RUNNING:
        return "Running";
    case TREADMILL:
        return "Treadmill";
    case BIKING:
        return "Biking";
    case SWIMMING:
        return "Swimming";
    case FREESTYLE:
        return "Freestyle";
    case STOPWATCH:
        return "Stopwatch";
    default:
        return "Other";
    }
}

void Activity::setFilename(const QString &filename)
{
    m_Filename = filename;
}

QString Activity::filename() const
{
    return m_Filename;
}

QString Activity::toString() const
{
    QString str = QString("Time %1, Notes %2, Sport %3\n").arg(m_Date.toString()).arg(m_Notes).arg(m_Sport);
    foreach ( LapPtr lap, m_Laps)
    {
        str += "\t" + lap->toString();
    }
    return str;
}

TrackPointPtr Activity::find(int secondsSinceStart)
{
    int startT;
    if ( m_Laps.count() == 0 )
    {
        return TrackPointPtr();
    }
    LapPtr lap = m_Laps.first();
    if ( lap->points().count() == 0 )
    {
        return TrackPointPtr();
    }

    TrackPointPtr tp = lap->points().first();


    startT = tp->time().toTime_t();

    foreach ( LapPtr lap, m_Laps)
    {
        foreach ( TrackPointPtr tp, lap->points())
        {
            int dt = tp->time().toTime_t() - startT;
            if ( dt >= secondsSinceStart )
            {
                return tp;
            }
        }
    }
    return TrackPointPtr();
}

void Activity::setDistance(float distance)
{
    m_Distance = distance;
}

float Activity::distance() const
{
    return m_Distance;
}

void Activity::setDuration(quint32 duration)
{
    m_Duration = duration;
}

quint32 Activity::duration() const
{
    return m_Duration;
}
