#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include "lap.h"


class Activity
{

public:
    enum Sport { RUNNING, TREADMILL, BIKING, SWIMMING, STOPWATCH, FREESTYLE, OTHER };

    Activity();

    QDateTime date() const;
    void setDate( const QDateTime & date );
    LapList & laps();
    QString notes() const;
    void setNotes( const QString & notes );
    Sport sport() const;
    void setSport(Sport sport);
    QString sportString() const;    
    static QString sportToString( Sport sport );

    void setFilename( const QString & filename);
    QString filename() const;

    QString toString() const;
    TrackPointPtr find( int secondsSinceStart );

    void setDistance( float distance );
    float distance() const;
    void setDuration( quint32 duration);
    quint32 duration() const;


private:
    LapList m_Laps;
    QDateTime m_Date;    
    QString m_Notes;
    Sport m_Sport;
    QString m_Filename;
    quint32 m_Duration;
    float m_Distance;

};

typedef QSharedPointer<Activity>ActivityPtr;
Q_DECLARE_METATYPE( ActivityPtr )

#endif // ACTIVITY_H
