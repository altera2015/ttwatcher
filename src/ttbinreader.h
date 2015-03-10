#ifndef TTBINREADER_H
#define TTBINREADER_H

#include <QString>
#include <QIODevice>
#include <QDateTime>
#include <QMap>
#include "activity.h"

class TTBinReader
{
    QMap<quint8, quint16> m_RecordLengths;
    qint32 m_UTCOffset;
    quint16 readquint16( quint8 * data, int pos );
    quint32 readquint32( quint8 * data, int pos );
    qint32 readqint32( quint8 * data, int pos );
    QDateTime readTime(quint8 * data, int pos , bool inUTC);
    float readFloat( quint8 * data, int pos );

    bool readData( QIODevice & ttbin, quint8 tag, int expectedSize, QByteArray & dest );
    bool readHeader( QIODevice & ttbin, ActivityPtr activity );
    bool readLap( QIODevice & ttbin, ActivityPtr activity );
    bool readHeartRate( QIODevice & ttbin, ActivityPtr activity );
    bool readPosition( QIODevice & ttbin, ActivityPtr activity, bool forgiving );
    bool readSummary( QIODevice & ttbin, ActivityPtr activity );
    bool readTreadmill( QIODevice & ttbin, ActivityPtr activity );
    bool readSwim( QIODevice & ttbin, ActivityPtr activity );

    bool skipTag( QIODevice & ttbin, quint8 tag, int size );

public:
    TTBinReader();

    ActivityPtr read( QIODevice & ttbin, bool forgiving = false );
};

int process(const QString &filename);

#endif // TTBINREADER_H
