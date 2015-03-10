#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <QString>
#include <QList>
#include <QSharedPointer>
#include "lap.h"


class Activity
{

public:
    enum Sport { RUNNING, TREADMILL, BIKING, SWIMMING, OTHER };

    Activity();

    QDateTime date() const;
    void setDate( const QDateTime & date );
    LapList & laps();
    QString notes() const;
    void setNotes( const QString & notes );
    Sport sport() const;
    void setSport(Sport sport);
    QString sportString() const;    
    void setFilename( const QString & filename);
    QString filename() const;

    QString toString() const;
    TrackPointPtr find( int secondsSinceStart );

private:
    LapList m_Laps;
    QDateTime m_Date;
    QString m_Notes;
    Sport m_Sport;
    QString m_Filename;

};

typedef QSharedPointer<Activity>ActivityPtr;
Q_DECLARE_METATYPE( ActivityPtr )

#endif // ACTIVITY_H
