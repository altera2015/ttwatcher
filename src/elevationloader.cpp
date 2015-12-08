#include "elevationloader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QEventLoop>
#include <QNetworkProxy>
#include "elevation.h"
#include "elevationtiledownloaderdialog.h"
#include "bridge.h"

ElevationLoader::ElevationLoader(QObject * parent) :
    QObject(parent),
    m_Status(IDLE)
{    
}

ElevationLoader::Status ElevationLoader::load(ActivityPtr activity, Source source, bool synchronous, bool fullReload)
{    
    // the tt watches do not log elevation, even though some
    // models have barometric height measurement, hey TT please
    // log that data in the .ttbin files!!

    // for now we have to load the elevation data from their website.

    if ( !fullReload )
    {
        QFile f(activity->filename() + ".elevation" );
        if ( f.open(QIODevice::ReadOnly))
        {
            QByteArray data = f.readAll();
            f.close();
            if ( data.length() > 1 )
            {
                if ( process(activity, data) )
                {
                    m_Status = SUCCESS;
                    emit loaded(true, activity);
                    return SUCCESS;
                }
            }
        }
    }


    switch ( source )
    {
    case USE_TT:
        // The TT elevation data is really really bad (at least for US). Let's use the SRTM or NED1 tiles instead.
        return loadTT(activity, synchronous);
    case USE_SRTMANDNED:
        return loadSRTM(activity);
    default:
        return FAILED;
    }
}


ElevationLoader::Status ElevationLoader::loadTT(ActivityPtr activity, bool synchronous)
{

    QJsonDocument requestData;

    QJsonArray coordinates;

    foreach ( LapPtr lap, activity->laps() )
    {
        foreach ( TrackPointPtr tp, lap->points() )
        {

            if ( tp->latitude() != 0 && tp->longitude() != 0 )
            {
                QJsonArray coordinate;
                coordinate.append( tp->latitude() );
                coordinate.append( tp->longitude() );
                coordinates.append(coordinate);
            }
        }
    }
    requestData.setArray(coordinates);

    QNetworkRequest r( QUrl("https://mysports.tomtom.com/tyne/dem/fixmodel"));
    r.setHeader( QNetworkRequest::ContentTypeHeader, "text/plain");
    r.setHeader( QNetworkRequest::UserAgentHeader, "Mozilla/5.0");
    QByteArray jr = requestData.toJson();
    qDebug() << jr;
    QNetworkReply * reply = m_Manager.post( r, jr );
    reply->setProperty("ActivityPtr", QVariant::fromValue( activity ));
    m_Status = DOWNLOADING;

    if ( synchronous )
    {
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();
        finished(reply); // sets m_Status to final value.
        return m_Status;
    }
    else
    {
        connect(reply, &QNetworkReply::finished, this, [this, reply](){
            finished(reply);
        });
        return DOWNLOADING;
    }
}



void ElevationLoader::finished(QNetworkReply *reply)
{
    ActivityPtr activity = reply->property("ActivityPtr").value< ActivityPtr >();

    if ( reply->error() != QNetworkReply::NoError )
    {
        qDebug() << "ElevationLoader::finished / could not load elevation " << reply->errorString();
        m_Status = FAILED;
        emit loaded(false, activity);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    reply = 0;

    bool success = process(activity, data);

    if ( success )
    {
        QFile f( activity->filename() + ".elevation");
        if ( f.open(QIODevice::ReadWrite) )
        {
            f.write(data);
            f.close();
        }

        m_Status = SUCCESS;
    }
    else
    {
        m_Status = FAILED;
    }

    emit loaded(success, activity);
}

ElevationLoader::Status ElevationLoader::loadSRTM(ActivityPtr activity)
{
    Elevation e;
    e.prepare();


    float lastElevation = 0.0;

    foreach ( LapPtr lap, activity->laps() )
    {
        foreach ( TrackPointPtr tp, lap->points() )
        {
            if ( tp->latitude() != 0 && tp->longitude() != 0 )
            {
                bool repeat = true;
                while (repeat)
                {
                    repeat = false;
                    QPointF p( tp->longitude(), tp->latitude() );
                    float elevation;
                    switch ( e.elevation(p, elevation) )
                    {
                        case Elevation::NO_TILE:
                        {
                            ElevationSource source = e.dataSources(p);
                            if ( !source.valid )
                            {
                                tp->setAltitude(lastElevation);
                                break;
                            }

                            ElevationTileDownloaderDialog d(e.basePath());
                            d.addSource(source);
                            d.show();
                            d.go();
                            if ( d.exec() == QDialog::Rejected )
                            {
                                emit loaded(false, activity);
                                return ElevationLoader::FAILED;
                            }
                            e.prepare();
                            repeat = true;
                            break;
                        }
                        case Elevation::NO_DATA:
                            tp->setAltitude(lastElevation);
                            break;
                        case Elevation::SUCCESS:
                            lastElevation = elevation;
                            tp->setAltitude(elevation);
                            break;
                    }
                }
            }
        }
    }

    // Bridge Fix
    BridgeList bl;
    Bridge::loadBridgeFiles(bl);

    // bl.append( Bridge()); // defaults to jensen beach bridge.

    foreach ( const Bridge & b, bl )
    {
        foreach ( LapPtr lap, activity->laps() )
        {
            if ( b.isNearBridge(lap->points()))
            {
                TrackPointList tpl;
                foreach ( LapPtr lap, activity->laps() )
                {
                    tpl.append( lap->points());
                }
                b.fixBridge(tpl);
                break;
            }
        }
    }

    QJsonArray elevations;
    foreach ( LapPtr lap, activity->laps() )
    {
        foreach ( TrackPointPtr tp, lap->points() )
        {
            if ( tp->latitude() != 0 && tp->longitude() != 0 )
            {
                elevations.append( tp->altitude());
            }
        }
    }


    QJsonDocument d;
    d.setArray(elevations);

    QFile f( activity->filename() + ".elevation");
    if ( f.open(QFile::WriteOnly|QFile::Truncate) )
    {
        f.write( d.toJson());
        f.close();
    }
    emit loaded(true, activity);
    return ElevationLoader::SUCCESS;
}

bool ElevationLoader::process(ActivityPtr activity, QByteArray data)
{
    QJsonParseError error;
    QJsonDocument d = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        qDebug() << data;
        qDebug() << "ElevationLoader::finished / could not parse response.";
        return false;
    }

    QJsonArray elevationData = d.array();

    int index = 0;

    foreach ( LapPtr lap, activity->laps() )
    {
        foreach ( TrackPointPtr tp, lap->points() )
        {
            if ( tp->latitude() != 0 && tp->longitude() != 0 )
            {
                if ( index < elevationData.count() )
                {
                    tp->setAltitude(elevationData.at(index).toDouble());
                    index++;
                }
                else
                {
                    qDebug() << "ElevationLoader::finished / did not get enough elevation data.";
                    return false;
                }
            }
        }
    }

    return true;
}


