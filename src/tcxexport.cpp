#include "tcxexport.h"
#include <QDebug>
#include <centeredexpmovavg.h>

// http://www.utilities-online.info/xsdvalidation/?save=dbdfe5b3-b776-4e28-8964-3f17c1b54a1c-xsdvalidation

TCXExport::TCXExport()
{
}

void TCXExport::save(QIODevice *dev, ActivityPtr activity)
{
    CenteredExpMovAvg cadence;

    QXmlStreamWriter stream(dev);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();

    stream.writeStartElement("TrainingCenterDatabase");
    stream.writeAttribute("xsi:schemaLocation", "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2 http://www.garmin.com/xmlschemas/TrainingCenterDatabasev2.xsd");
    stream.writeAttribute("xmlns", "http://www.garmin.com/xmlschemas/TrainingCenterDatabase/v2");
    stream.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    stream.writeAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
    stream.writeAttribute("xmlns:ns2", "http://www.garmin.com/xmlschemas/ActivityExtension/v2");

    stream.writeStartElement("Activities");

    stream.writeStartElement("Activity");
    stream.writeAttribute("Sport", activity->sportString());

    stream.writeTextElement("Id", activity->date().toUTC().toString(Qt::ISODate));


    // pre-process the cadence.
    foreach( LapPtr lap, activity->laps() )
    {
        if ( lap->points().count() == 0 )
        {
            continue;
        }
        foreach (TrackPointPtr tp, lap->points())
        {
            int c = qMin(4, tp->cadence());
            cadence.add( c );
        }
    }


    int pos = -1;
    foreach( LapPtr lap, activity->laps() )
    {
        if ( lap->points().count() == 0 )
        {
            continue;
        }


        TrackPointPtr p = lap->points().first();

        int maxBpm = 0;
        quint64 totalBpm = 0;
        int bpmCount = 0;
        double maxSpeed = 0;
        double totalSpeed = 0;
        int speedCount = 0;

        foreach (TrackPointPtr tp, lap->points())
        {
            if ( tp->heartRate() > 20 && tp->heartRate() < 240 )
            {
                if ( tp->heartRate() > maxBpm )
                {
                    maxBpm = tp->heartRate();
                }
                totalBpm += tp->heartRate();
                bpmCount++;
            }

            if ( tp->speed() > 0 )
            {
                totalSpeed += tp->speed();
                speedCount++;
            }
            if ( tp->speed() > maxSpeed )
            {
                maxSpeed = tp->speed();
            }
        }


        stream.writeStartElement("Lap");
        stream.writeAttribute("StartTime", p->time().toUTC().toString(Qt::ISODate));

        stream.writeTextElement("TotalTimeSeconds", QString::number(lap->totalSeconds()) );
        stream.writeTextElement("DistanceMeters", QString::number( lap->length() ) );
        stream.writeTextElement("MaximumSpeed", QString::number( maxSpeed,'f',5 ));
        stream.writeTextElement("Calories", QString::number( lap->calories() ));
        if ( bpmCount > 0 )
        {
            stream.writeStartElement("AverageHeartRateBpm");
            stream.writeTextElement("Value", QString::number( totalBpm / bpmCount ));
            stream.writeEndElement(); // avgbpm

            stream.writeStartElement("MaximumHeartRateBpm");
            stream.writeTextElement("Value", QString::number( maxBpm ));
            stream.writeEndElement(); // MaximumHeartRateBpm
        }

        stream.writeTextElement("Intensity", "Active");
        stream.writeTextElement("TriggerMethod", "Manual");




        stream.writeStartElement("Track");

        foreach (TrackPointPtr tp, lap->points())
        {
            pos++;
            stream.writeStartElement("Trackpoint");


            stream.writeTextElement("Time", tp->time().toUTC().toString(Qt::ISODate));
            if ( tp->latitude() != 0 || tp->longitude() != 0 )
            {

                stream.writeStartElement("Position");
                stream.writeTextElement("LatitudeDegrees", QString::number(tp->latitude(),'f',9) );
                stream.writeTextElement("LongitudeDegrees", QString::number(tp->longitude(),'f',9) );
                stream.writeEndElement(); // Position.
            }

            if ( tp->altitude() > 0.0 )
            {
                stream.writeTextElement("AltitudeMeters", QString::number(tp->altitude(),'f',1) );
            }
            stream.writeTextElement("DistanceMeters", QString::number(tp->cummulativeDistance(),'f',9));

            if ( tp->heartRate() > 20 && tp->heartRate() < 240 )
            {
                stream.writeStartElement("HeartRateBpm");
                stream.writeTextElement("Value", QString::number(tp->heartRate()));
                stream.writeEndElement();
            }




            if ( activity->sport() == Activity::RUNNING )
            {                
                stream.writeTextElement("Cadence", QString::number( round(cadence.cea(pos) * 30.0 )));
            }
            if ( activity->sport() == Activity::BIKING )
            {
                stream.writeTextElement("Cadence", QString::number( round(cadence.cea(pos) )));
            }

            stream.writeStartElement("Extensions");
            stream.writeStartElement("TPX");
            stream.writeAttribute("xmlns", "http://www.garmin.com/xmlschemas/ActivityExtension/v2");
            stream.writeTextElement("Speed", QString::number(tp->speed(),'f',5));
            stream.writeEndElement();
            stream.writeEndElement();


            stream.writeEndElement(); // TrackPoint;
        }

        stream.writeEndElement(); // track.

        if ( speedCount > 0 )
        {
            stream.writeStartElement("Extensions");
            stream.writeStartElement("LX");
            stream.writeAttribute("xmlns", "http://www.garmin.com/xmlschemas/ActivityExtension/v2");
            stream.writeTextElement("AvgSpeed", QString::number( totalSpeed / speedCount,'f',5 ));
            stream.writeEndElement();
            stream.writeEndElement();
        }


        stream.writeEndElement(); // lap.
    }

    stream.writeStartElement("Creator");
    stream.writeAttribute("xsi:type","Device_t");
    stream.writeTextElement("Name", "TomTom GPS Sport Watch (TTWatcher)");
    stream.writeTextElement("UnitId", "0");
    stream.writeTextElement("ProductID", "0");
    stream.writeStartElement("Version");
    stream.writeTextElement("VersionMajor", "1");
    stream.writeTextElement("VersionMinor", "0");
    stream.writeTextElement("BuildMajor", "0");
    stream.writeTextElement("BuildMinor", "0");
    stream.writeEndElement();// version
    stream.writeEndElement(); // creator

    stream.writeEndElement(); // activity
    stream.writeEndElement(); // activities


    stream.writeEndElement(); // TrainingCenterDatabase

    stream.writeEndDocument();
}
