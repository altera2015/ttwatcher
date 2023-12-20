#ifndef TTBINREADER_H
#define TTBINREADER_H

#include <QString>
#include <QIODevice>
#include <QDateTime>
#include <QMap>
#include <functional>
#include "activity.h"

class TTBinReader
{
    QMap<quint8, quint16> m_RecordLengths;
    qint32 m_UTCOffset;


    bool readData( QIODevice & ttbin, quint8 tag, int expectedSize, QByteArray & dest );
    bool readHeader( QIODevice & ttbin, ActivityPtr activity, QByteArray * cpy = 0 );
    bool readStatus( QIODevice & ttbin, ActivityPtr activity );
    bool readLap(QIODevice & ttbin, ActivityPtr activity );
    bool readHeartRate( QIODevice & ttbin, ActivityPtr activity );
    bool readPosition( QIODevice & ttbin, ActivityPtr activity, bool forgiving );
    bool readSummary( QIODevice & ttbin, ActivityPtr activity );
    bool readTreadmill( QIODevice & ttbin, ActivityPtr activity );
    bool readSwim( QIODevice & ttbin, ActivityPtr activity );
    bool readAltitude( QIODevice & ttbin, ActivityPtr activity );
    bool readRecovery(QIODevice &ttbin, ActivityPtr activity);
    bool readTraining(QIODevice & ttbin, ActivityPtr activity, int maxSize );
    bool readIndoor_Biking( QIODevice & ttbin, ActivityPtr activity );
    bool skipTag(QIODevice & ttbin, quint8 tag, int size , QByteArray *cpy = 0);

public:
    TTBinReader();




    ActivityPtr read( QIODevice & ttbin, bool forgiving = false, bool headerAndSummaryOnly = false );
    ActivityPtr read( const QString &filename, bool forgiving = false, bool headerAndSummaryOnly = false );


    static quint16 readquint16(const quint8 * data, int pos );
    static quint32 readquint32(const quint8 * data, int pos );
    static qint16 readqint16(const quint8 *data, int pos);
    static qint32 readqint32(const quint8 * data, int pos );
    static QDateTime readTime(const quint8 * data, int pos , bool inUTC);
    static float readFloat(const quint8 * data, int pos );


    bool updateActivityType(QIODevice &ttbin, bool forgiving, QIODevice &output, Activity::Sport newSport);
private:


};

int process(const QString &filename);

#endif // TTBINREADER_H
