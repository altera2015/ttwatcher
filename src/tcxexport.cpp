#include "tcxexport.h"
#include <QDebug>

TCXExport::TCXExport()
{
}

void TCXExport::save(QIODevice *dev, ActivityPtr activity)
{
    QVector<int> cadence;

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
    foreach( LapPtr lap, activity->laps() )
    {
        if ( lap->points().count() == 0 )
        {
            continue;
        }


        TrackPointPtr p = lap->points().first();

        stream.writeStartElement("Lap");
        stream.writeAttribute("StartTime", p->time().toUTC().toString(Qt::ISODate));

        stream.writeTextElement("TotalTimeSeconds", QString::number(lap->totalSeconds()) );
        stream.writeTextElement("DistanceMeters", QString::number( lap->length() ) );
        stream.writeTextElement("Calories", QString::number( lap->calories() ));
        stream.writeTextElement("Intensity", "Active");
        stream.writeTextElement("TriggerMethod", "Manual");

        stream.writeStartElement("Track");

        foreach (TrackPointPtr tp, lap->points())
        {

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
                stream.writeTextElement("AltitudeMeters", QString::number(tp->altitude(),'f',9) );
            }
            stream.writeTextElement("DistanceMeters", QString::number(tp->cummulativeDistance(),'f',9));

            if ( tp->heartRate() >= 0 )
            {
                stream.writeStartElement("HeartRateBpm");
                stream.writeTextElement("Value", QString::number(tp->heartRate()));
                stream.writeEndElement();
            }

            if ( activity->sport() == Activity::RUNNING )
            {

                // the first trackpoint has counted steps before starting.
                // that means the first data point will have potentially a lot of steps
                // so skip that guy.

                if ( tp->cadence() > 0 && tp != lap->points().first() )
                {
                    cadence.push_front( tp->cadence() );
                }

                if ( cadence.count() > 0 )
                {
                    double dc = 0;
                    foreach ( int c, cadence )
                    {
                        dc+=c;
                    }
                    if ( cadence.count() > 10 )
                    {
                        stream.writeTextElement("Cadence", QString::number( (int)(0.5 * 60.0 * dc / cadence.count()) ));
                    }
                }

                while ( cadence.count() > 30 )
                {
                    cadence.pop_back();
                }
            }
            if ( activity->sport() == Activity::BIKING )
            {
                stream.writeTextElement("Cadence", QString::number(tp->cadence()));
            }


            stream.writeEndElement(); // TrackPoint;
        }

        stream.writeEndElement(); // track.

        stream.writeEndElement(); // lap.
    }

    stream.writeStartElement("Creator");
    stream.writeAttribute("xsi","type","Device_t");
    stream.writeTextElement("Name", "TomTom GPS Sport Watch");
    stream.writeTextElement("UnitId", "0");
    stream.writeTextElement("ProductID", "0");
    stream.writeEndElement();




    stream.writeEndElement(); // activity
    stream.writeEndElement(); // activities
    stream.writeEndElement(); // TrainingCenterDatabase

    stream.writeEndDocument();
}
