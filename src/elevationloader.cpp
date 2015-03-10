#include "elevationloader.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>

#include <QNetworkProxy>
ElevationLoader::ElevationLoader(QObject * parent) : QObject(parent)
{
    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
}


void ElevationLoader::load(ActivityPtr activity)
{
    // the tt watches do not log elevation, even though some
    // models have barometric height measurement, hey TT please
    // log that data in the .ttbin files!!

    // for now we have to load the elevation data from their website.

    QFile f(activity->filename() + ".elevation" );
    if ( f.open(QIODevice::ReadOnly))
    {
        QByteArray data = f.readAll();
        f.close();
        if ( data.length() > 4 )
        {
            if ( process(activity, data) )
            {
                emit loaded(true, activity);
                return;
            }
        }
    }


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
    QNetworkReply * reply = m_Manager.post( r, requestData.toJson() );
    reply->setProperty("ActivityPtr", QVariant::fromValue( activity ));
    Q_UNUSED(reply);

}

void ElevationLoader::finished(QNetworkReply *reply)
{
    ActivityPtr activity = reply->property("ActivityPtr").value< ActivityPtr >();

    if ( reply->error() != QNetworkReply::NoError )
    {
        qDebug() << "ElevationLoader::finished / could not load elevation " << reply->errorString();
        emit loaded(false, activity);
        reply->deleteLater();
        return;
    }


    QByteArray data = reply->readAll();
    reply->deleteLater();
    reply = 0;

    qDebug() << data;

    bool success = process(activity, data);

    // if ( success )
    {
        QFile f( activity->filename() + ".elevation");
        if ( f.open(QIODevice::ReadWrite) )
        {
            f.write(data);
            f.close();
        }
    }

    emit loaded(success, activity);
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


