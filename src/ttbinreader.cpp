#include "ttbinreader.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <QString>
#include <QObject>
#include <QDateTime>
#include <QDebug>

#include <QFile>
#include "order32.h"


#define TAG_FILE_HEADER         (0x20) // OK
#define TAG_STATUS              (0x21) // OK
#define TAG_GPS                 (0x22) // OK
#define TAG_HEART_RATE          (0x25) // OK
#define TAG_SUMMARY             (0x27) // OK
#define TAG_POOL_SIZE           (0x2a)
#define TAG_WHEEL_SIZE          (0x2b)
#define TAG_TRAINING_SETUP      (0x2d)
#define TAG_LAP                 (0x2f)
#define TAG_TREADMILL           (0x32)  // OK
#define TAG_SWIM                (0x34)  // OK
#define TAG_GOAL_PROGRESS       (0x35)
#define TAG_INTERVAL_SETUP      (0x39)
#define TAG_INTERVAL_START      (0x3a)
#define TAG_INTERVAL_FINISH     (0x3b)
#define TAG_RACE_SETUP          (0x3c)
#define TAG_RACE_RESULT         (0x3d)
#define TAG_ALTITUDE_UPDATE     (0x3e)  // OK
#define TAG_HEART_RATE_RECOVERY (0x3f)  // OK

// Activities:
#define TT_ACTIVITY_RUN         0
#define TT_ACTIVITY_CYCLE       1
#define TT_ACTIVITY_SWIM        2
#define TT_ACTIVITY_STOPWATCH   6
#define TT_ACTIVITY_TREADMILL   7
#define TT_ACTIVITY_FREESTYLE   8

quint16 TTBinReader::readquint16(const quint8 *data, int pos)
{
    quint16 d = (data[ pos + 0 ] ) |
            (data[ pos + 1 ] << 8 );
    return d;
}

qint16 TTBinReader::readqint16(const quint8 *data, int pos)
{
    qint16 v;
    quint8 * d = (quint8*)&v;

    if ( O32_HOST_ORDER == O32_LITTLE_ENDIAN )
    {
        d[0] = data[0 + pos ];
        d[1] = data[1 + pos ];
    }
    else
    {
        d[1] = data[0 + pos ];
        d[0] = data[1 + pos ];
    }
    return v;
}

quint32 TTBinReader::readquint32(const quint8 *data, int pos)
{
    quint32 d = (data[ pos + 0 ] ) |
            (data[ pos + 1 ] << 8 ) |
            (data[ pos + 2 ] << 16 ) |
            (data[ pos + 3 ] << 24 );
    return d;
}

qint32 TTBinReader::readqint32(const quint8 *data, int pos)
{
    qint32 v;
    quint8 * d = (quint8*)&v;

    if ( O32_HOST_ORDER == O32_LITTLE_ENDIAN )
    {
        d[0] = data[0 + pos ];
        d[1] = data[1 + pos ];
        d[2] = data[2 + pos ];
        d[3] = data[3 + pos ];
    }
    else
    {
        d[3] = data[0 + pos ];
        d[2] = data[1 + pos ];
        d[1] = data[2 + pos ];
        d[0] = data[3 + pos ];
    }
    return v;
}

QDateTime TTBinReader::readTime(const quint8 *data, int pos, bool inUTC)
{
    quint32 seconds = readquint32(data, pos);
    QDateTime dt = QDateTime::fromTime_t(seconds);
    if ( inUTC )
    {
        dt = dt.toUTC();
        dt.setTimeSpec( Qt::LocalTime );
    }
    return dt;
}

float TTBinReader::readFloat(const quint8 *data, int pos)
{
    float v;
    quint8 * d = (quint8*)&v;

    // float has 4 bytes.
    // little endian and big endian are different.
    //qDebug() << data[0 + pos ] << data[1 + pos ] << data[2 + pos ] << data[3 + pos ];
    if ( O32_HOST_ORDER == O32_LITTLE_ENDIAN )
    {        
        d[0] = data[0 + pos ];
        d[1] = data[1 + pos ];
        d[2] = data[2 + pos ];
        d[3] = data[3 + pos ];
    }
    else
    {             
        d[3] = data[0 + pos ];
        d[2] = data[1 + pos ];
        d[1] = data[2 + pos ];
        d[0] = data[3 + pos ];
    }

    return v;

}




bool TTBinReader::readHeader(QIODevice &ttbin, ActivityPtr activity )
{
    /*

    typedef struct __attribute__((packed))
    {
    uint8_t tag;
    uint16_t length;
    } RECORD_LENGTH;

     uint16_t file_version; OS = 0
    uint8_t firmware_version[3]; OS = 2
    uint16_t product_id; OS = 5
    uint32_t start_time; // local time OS = 7
    uint8_t software_version[16]; OS = 11
    uint8_t gps_firmware_version[80]; OS = 27
    uint32_t watch_time; // local time OS = 107
    int32_t local_time_offset; // seconds from UTC  OS = 111
    uint8_t _reserved; OS = 115
    uint8_t length_count; // number of RECORD_LENGTH objects to follow  OS = 116
    RECORD_LENGTH lengths[1];

    */

    quint8 data[0x75];
    if ( ttbin.read((char*)data, 0x75) != 0x75 )
    {
        qWarning() << "TTBinReader::readHeader / not enough bytes read.";
        return false;
    }

    if ( data[0] < 7 )
    {
        qWarning() << "TTBinReader::readHeader / unknown file format, proceed at own risk. " << data[0];
        return false;
    }

    activity->setDate( readTime(data, 7, true));
    // UTC offset at pos 15 int32
    m_UTCOffset = readqint32(data, 15);

    // read the record lengths.
    quint8 lengths = data[116];
    for (quint8 i=0;i<lengths;i++)
    {
        quint8 lenrec[3];
        ttbin.read((char*)lenrec, 3);
        quint8 tag = lenrec[0];
        quint16 len = ( lenrec[1] | lenrec[2] << 8) - 1; // length includes the tag, we exclude it.

        if ( tag < 0xff && len < 0x1000 )
        {
            m_RecordLengths[tag] = len;
        }
        // qDebug() << "TagLen " << QString::number(tag,16) << " len " << len;


    }


    LapList & ll = activity->laps();
    if ( ll.count() == 0 )
    {
        ll.append( LapPtr::create() );
    }
    return true;
}


bool TTBinReader::readData(QIODevice &ttbin, quint8 tag, int expectedSize, QByteArray &dest)
{
    int recordLen = m_RecordLengths.contains(tag) ? m_RecordLengths[tag] : expectedSize;
    if ( recordLen < expectedSize )
    {
        qCritical() << "TTBinReader::read / length from record length field is smaller than we expect." << QString::number(tag,16) << recordLen << expectedSize;
        return false;
    }

    dest = ttbin.read(recordLen);

    if ( dest.length() != recordLen )
    {
        qWarning() << "TTBinReader::read / not enough bytes read."  << QString::number(tag,16) << recordLen << expectedSize << dest.size();
        return false;
    }

    return true;
}

bool TTBinReader::readStatus(QIODevice &ttbin, ActivityPtr activity)
{

    /*
        uint8_t  status;        // 0 = ready, 1 = active, 2 = paused, 3 = stopped
        uint8_t  activity;      // 0 = running, 1 = cycling, 2 = swimming, 7 = treadmill, 8 = freestyle
        uint32_t timestamp;     // local time
    */


    QByteArray buffer;
    if ( !readData(ttbin, TAG_STATUS, 0x06, buffer))
    {
        return false;
    }

    return true;

}

bool TTBinReader::readLap(QIODevice &ttbin, ActivityPtr activity)
{
    /*
        uint32_t total_time;        // seconds since activity start
        float    total_distance;    // metres
        uint16_t total_calories;
    */

    QByteArray buffer;
    if ( !readData(ttbin, TAG_LAP, 0x0A, buffer))
    {
        return false;
    }

    LapList & ll = activity->laps();
    if ( ll.count() == 0 )
    {
        ll.append( LapPtr::create() );
    }

    LapPtr lap = ll.last();
    if ( lap->points().count() > 0 )
    {
        ll.append( LapPtr::create() );
        lap = ll.last();
    }

    return true;
}

bool TTBinReader::readHeartRate(QIODevice &ttbin, ActivityPtr activity)
{
    /*

    Tag 0x25 (len 0x6)

    typedef struct {
      quint8 heart_rate;
      quint8 u1;
      quint32 time;
    } HeartRate;*/

    QByteArray buffer;
    if ( !readData(ttbin, TAG_HEART_RATE, 0x06, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();

    LapPtr lap = activity->laps().last();

    if ( lap->points().count() == 0 )
    {
        return true; // heartreate without gps point.
    }

    TrackPointPtr tp = lap->points().last();

    tp->setHeartRate( data[0] );

    return true;
}

bool TTBinReader::readPosition(QIODevice &ttbin, ActivityPtr activity, bool forgiving)
{
    /*
    // Tag 0x22 ( len = 0x1b / 27 )
    typedef struct {
      qint32 latitude;  // in 1e-7 degrees
      qint32 longitude; // in 1e-7 degrees
      quint16 heading;  // degrees * 100, 0 = North, 9000 = East...
      quint16 speed;  // 100 * m/s
      quint32 time; // seconds since 1970
      quint16 calories;
      float instant_speed;
      float cum_distance;
      quint8 cycles; // Tomtom CSV calls it "cycles", maybe steps?
    } GPS;*/

    QByteArray buffer;
    if ( !readData(ttbin, TAG_GPS, 0x1b, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();

    LapPtr lap = activity->laps().last();
    TrackPointPtr tp = TrackPointPtr::create();

    tp->setLatitude( readqint32(data, 0) * 1.0e-7 );
    tp->setLongitude( readqint32(data, 4) * 1.0e-7 );



    // skip heading.
    tp->setSpeed( readquint16(data, 10) / 100.0 );

    if ( forgiving )
    {
        if ( tp->latitude() > 90 || tp->latitude() < -90 || tp->longitude() > 180 || tp->longitude() < -180 || tp->speed() > 50 )
        {
            qDebug() << "INVALID POSITION RECORD.";
            ttbin.seek( ttbin.pos() - buffer.length() );
            return true;
        }
    }

    tp->setCalories( readquint16(data,16) );
    tp->setTime( readTime(data,12, false));


    //tp->setIncrementalDistance( readFloat(data, 18 ));
    tp->setCummulativeDistance( readFloat(data, 22 ));
    tp->setCadence( data[26] );

    if ( tp->time().toTime_t() > 0 && tp->latitude() != 0 && tp->longitude() != 0 )
    {
        lap->points().append(tp);
    }

    return true;
}



bool TTBinReader::readSummary(QIODevice &ttbin, ActivityPtr activity)
{
    /* Tag 0x27 (len 0xb / 11)
    typedef struct {
      quint8 activity_type;  // 7 = treadmill?
      float distance;  // meters.
      quint32 duration;  // seconds
      quint16 calories;
    } Summary;*/


    QByteArray buffer;
    if ( !readData(ttbin, TAG_SUMMARY, 0x0b, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();

    switch ( data[0] )
    {
    case TT_ACTIVITY_RUN:
        activity->setSport(Activity::RUNNING);
        break;
    case TT_ACTIVITY_CYCLE:
        activity->setSport(Activity::BIKING);
        break;
    case TT_ACTIVITY_SWIM:
        activity->setSport(Activity::SWIMMING);
        break;
    case TT_ACTIVITY_TREADMILL:
        activity->setSport(Activity::TREADMILL);
        break;
    case TT_ACTIVITY_STOPWATCH:
        activity->setSport(Activity::STOPWATCH);
        break;
    case TT_ACTIVITY_FREESTYLE:
        activity->setSport(Activity::FREESTYLE);
        break;

    }

    // thanks, we'll calculate these.
    /*
    lap->setLength( readFloat(data, 1));
    lap->setTotalSeconds( readquint32(data, 5) );*/

    LapPtr lap = activity->laps().first();
    lap->setCalories( readquint16( data, 9 ));

    return true;
}

bool TTBinReader::readTreadmill(QIODevice &ttbin, ActivityPtr activity)
{
    /* Tag 0x32
    typedef struct {
        uint32_t timestamp;     // local time
        float    distance;      // metres
        uint16_t calories;
        uint32_t steps;
        uint16_t step_length;   // cm, not implemented yet
    } Treadmill; */


    QByteArray buffer;
    if ( !readData(ttbin, TAG_TREADMILL, 0x10, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();

    LapPtr lap = activity->laps().last();

    float distance = readFloat(data,4);

    TrackPointPtr lp = TrackPointPtr::create();
    lp->setTime( readTime( data, 0, false ) );
    lp->setCummulativeDistance( distance );

    if ( lap->points().count() > 0 )
    {
        TrackPointPtr lastPt = lap->points().last();
        lp->setCummulativeDistance( distance );

    }
    else
    {
        lp->setCummulativeDistance( distance);
    }
    lp->setCalories( readquint16( data, 8 ));
    lp->setCadence( readquint32(data, 10));

    lap->points().append(lp);

    return true;


}

bool TTBinReader::readSwim(QIODevice &ttbin, ActivityPtr activity)
{
    /*
    uint32_t timestamp;         // local time
    float    total_distance;    // metres
    uint8_t  frequency;
    uint8_t  stroke_type;
    uint32_t strokes;           // since the last report
    uint32_t completed_laps;
    uint16_t total_calories;

    */

    QByteArray buffer;
    if ( !readData(ttbin, TAG_SWIM, 0x1c, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();

    LapPtr lap = activity->laps().last();

    TrackPointPtr lp = TrackPointPtr::create();
    lp->setTime( readTime( data, 0,false ) );

    lp->setCalories( readquint32( data, 15 ));


    lap->points().append(lp);

    return true;
}

bool TTBinReader::readAltitude(QIODevice &ttbin, ActivityPtr activity)
{
    /*
     * {
    int16_t rel_altitude;   // altitude change from workout start
    float   total_climb;    // metres, descents are ignored
    uint8_t qualifier;      // not defined yet
    } FILE_ALTITUDE_RECORD;
    */
    QByteArray buffer;
    if ( !readData(ttbin, TAG_ALTITUDE_UPDATE, 0x07, buffer))
    {
        return false;
    }
    quint8 * data = (quint8*)buffer.data();
    qint16 rel = readqint16(data, 0);
    float climb = readFloat(data, 2);

    return true;


}

bool TTBinReader::readRecovery(QIODevice &ttbin, ActivityPtr activity)
{
    /*typedef struct __attribute__((packed))
    {
    uint32_t status;        // 3 = good, 4 = excellent
    uint32_t heart_rate;    // bpm
    } FILE_HEART_RATE_RECOVERY_RECORD;*/
    QByteArray buffer;
    if ( !readData(ttbin, TAG_HEART_RATE_RECOVERY, 0x08, buffer))
    {
        return false;
    }

    // does this export anywhere?

    return true;
}


bool TTBinReader::skipTag(QIODevice &ttbin, quint8 tag, int size)
{
    //int pos = ttbin.pos();
    QByteArray data = ttbin.read(size);
    if ( data.length() != size )
    {
        qWarning() << "TTBinReader::skipTag / not enough data." << QString::number(tag,16) << size << data.length();
        return false;
    }

    if ( tag != 0x23 )
    {
        qWarning() << "TTBinReader::skipTag / skipping tag " << QString::number(tag,16) << size << data.toHex();
    }

    return true;
}

TTBinReader::TTBinReader()
{
}



ActivityPtr TTBinReader::read(QIODevice &ttbin, bool forgiving)
{
    if ( !ttbin.isOpen() )
    {
        return ActivityPtr();
    }

    m_RecordLengths.clear();

    ActivityPtr ap = ActivityPtr::create();

    quint8 tag;

    while ( !ttbin.atEnd() )
    {
        if ( !ttbin.getChar((char*)&tag) )
        {
            qDebug() << "TTBinReader::read / could not read tag.";
            return ActivityPtr();
        }

        if ( tag != TAG_FILE_HEADER )
        {

            if ( !m_RecordLengths.contains(tag) )
            {
                continue; //skipping.
            }

            if ( forgiving && m_RecordLengths.contains(tag))
            {
                int recordLength = m_RecordLengths[tag];
                QByteArray data = ttbin.peek(recordLength+1);
                if ( data.length() == recordLength + 1 )
                {
                    quint8 nextTag = (quint8)data.at(recordLength);
                    if ( !m_RecordLengths.contains(nextTag))
                    {
                        qDebug() << "This does not appear to be a valid record, skipping one.";
                        continue;
                    }
                }
            }
        }

        bool result = false;

        switch ( tag )
        {
        case TAG_FILE_HEADER: // header;
            if ( ttbin.pos() == 1 )
            {
                result = readHeader(ttbin, ap);
            }
            else
            {
                if ( forgiving )
                {
                    qDebug() << "TTBinReader::read / got header not at start skipping.";
                    result = true;
                }
                else
                {
                    result = false;
                }

            }

            break;
        case TAG_STATUS: // lap.
            result = readStatus(ttbin, ap);
            break;
        case TAG_GPS: // GPS pos + cadence
            result = readPosition(ttbin, ap, forgiving);
            break;
        case TAG_HEART_RATE: // heart rate on Cardio Models
            result = readHeartRate(ttbin, ap);
            break;
        case TAG_SUMMARY: // summary at end.
            result = readSummary(ttbin, ap);
            break;
        case TAG_TREADMILL:
            result = readTreadmill(ttbin, ap);
            break;
        case TAG_SWIM:
            result = readSwim(ttbin, ap);
            break;
        case TAG_ALTITUDE_UPDATE:
            result = readAltitude(ttbin, ap);
            break;
        case TAG_HEART_RATE_RECOVERY:
            result = readRecovery(ttbin, ap);
            break;

        default:

            if ( m_RecordLengths.contains( tag ) )
            {
                result = skipTag(ttbin, tag, m_RecordLengths[tag]);
            }
            else
            {
                result = false;
                qWarning() << "TTBinReader::read / unknown tag, bailing out. " << QString::number(tag,16) << ttbin.pos();
            }
        }

        if ( !result && !forgiving )
        {
            qWarning() << "TTBinReader::read / failed on tag, bailing out. " << QString::number(tag,16) << ttbin.pos();
            return ActivityPtr();
        }
    }

    foreach ( LapPtr lap, ap->laps() )
    {
        lap->calcTotals();
    }

    return ap;
}

ActivityPtr TTBinReader::read(const QString &filename, bool forgiving)
{
    QFile f(filename);
    if ( !f.open(QIODevice::ReadOnly))
    {
        return ActivityPtr();
    }
    ActivityPtr a = read(f,forgiving);
    a->setFilename(filename);
    return a;
}
