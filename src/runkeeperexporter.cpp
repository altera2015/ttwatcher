#include "runkeeperexporter.h"
#include "watchexporters.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QDesktopServices>


#include "httpserver.h"
#include "runkeeper_auth.h"

#define POST_ACTIVITY   1
#define GET_USER_URI    2
#define AUTH_REQUEST    3


RunKeeperExporter::RunKeeperExporter(const QString &serial, QObject *parent) :
    IActivityExporter(parent),
    m_Icon( QIcon(":/icons/runkeeper.png") ),
    m_Config(serial)
{
    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRequestFinished(QNetworkReply*)));
#ifdef USE_DEBUG_PROXY
    QNetworkProxy p;
    p.setHostName("localhost");
    p.setPort(8888);
    p.setType(QNetworkProxy::HttpProxy);
    m_Manager.setProxy(p);
#endif
}

QString RunKeeperExporter::name() const
{
    return "RunKeeper";
}

QIcon RunKeeperExporter::icon() const
{
    return m_Icon;
}

bool RunKeeperExporter::hasSetup() const
{
    return true;
}

void RunKeeperExporter::setup(QWidget *parent)
{
    HttpServer * server = HttpServer::singleton();

    QMessageBox b(QMessageBox::Information, tr("Opening RunKeeper Authorization"), tr("This box will close when done, or cancelled.\nNote you might have to allow TTWatcher access in your firewall, a popup might appear for that."), QMessageBox::Cancel, parent );

    QUrl cburl = server->registerCallback( [this, server, &b](QHttpRequest *req, QHttpResponse *resp) {

        server->respond(":/html/runkeeper.html", 200, req, resp);

        QUrlQuery codeQuery;
        codeQuery.setQuery( req->url().query() );
        QString code = codeQuery.queryItemValue("code");

        // now exchange for the auth token.
        QNetworkRequest r;
        QUrl url("https://runkeeper.com/apps/token");
        QUrlQuery q;
        q.addQueryItem("client_id", RUNKEEPER_CLIENT_ID);
        q.addQueryItem("client_secret", RUNKEEPER_CLIENT_SECRET);
        q.addQueryItem("code", code);
        q.addQueryItem("grant_type", "authorization_code");
        q.addQueryItem("redirect_uri", m_AuthRedirectUrl);

        r.setUrl( url );

        r.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply * reply = m_Manager.post( r, q.toString(QUrl::FullyEncoded).toUtf8() );
        reply->setProperty("REQUEST", AUTH_REQUEST);

        b.close(); // will continue execute below after b.exec, which then deletes the url.

    });

    m_AuthRedirectUrl = QUrl::toPercentEncoding( cburl.toString(QUrl::FullyEncoded) );
    QUrl runkeeperAuthRequest ("https://runkeeper.com/apps/authorize");
    QUrlQuery q;
    q.addQueryItem("client_id", RUNKEEPER_CLIENT_ID);
    q.addQueryItem("response_type", "code");
    q.addQueryItem("redirect_uri", m_AuthRedirectUrl );


    runkeeperAuthRequest.setQuery(q);

    QDesktopServices::openUrl(runkeeperAuthRequest);

    b.exec();

    server->unregisterCallback(cburl.path());
}

IExporterConfig &RunKeeperExporter::config()
{
    return m_Config;
}

IExporterConfig * RunKeeperExporter::createConfig()
{
    return new RunKeeperExporterConfig( m_Config.serial() );
}

void RunKeeperExporter::exportActivity(ActivityPtr activity)
{
    QJsonDocument d;

    QJsonObject af;

    af["post_to_facebook"] = false;
    af["post_to_twitter"] = false;
    af["start_time"] = toRKDate( activity->date() );

    int tcalories = 0;
    int tdistance = 0;
    foreach ( LapPtr lap, activity->laps() )
    {
        tcalories += lap->calories();
        tdistance += lap->length();
    }

    af["total_calories"] = tcalories;
    af["total_distance"] = tdistance;

    switch (activity->sport())
    {
    case Activity::RUNNING:
    case Activity::TREADMILL:
        af["type"] = QStringLiteral("Running");
        break;
    case Activity::BIKING:
        af["type"] = QStringLiteral("Cycling");
        break;
    case Activity::SWIMMING:
        af["type"] = QStringLiteral("Swimming");
        break;
    default:
        af["type"] = QStringLiteral("Other");
    }

    QJsonArray heartRate;
    QJsonArray path;
    QJsonArray calories;
    QJsonArray distance;

    int firstTime = -1;
    foreach ( LapPtr lap, activity->laps() )
    {
        foreach ( TrackPointPtr tp, lap->points())
        {
            int time = tp->time().toTime_t();
            if ( firstTime < 0 )
            {
                firstTime = time;
            }
            time = time - firstTime;
            if ( tp->hasGPS() )
            {
                QJsonObject pathEntry;
                pathEntry["timestamp"] = time;
                pathEntry["longitude"] = tp->longitude();
                pathEntry["latitude"] = tp->latitude();
                pathEntry["altitude"] = tp->altitude();
                pathEntry["type"] = QStringLiteral("gps");
                path.append( pathEntry );
            }
            if ( tp->heartRate() > 0 )
            {
                QJsonObject heartRateEntry;
                heartRateEntry["timestamp"] = time;
                heartRateEntry["heart_rate"] = tp->heartRate();
                heartRate.append(heartRateEntry);
            }

            if ( tp->calories() > 0 )
            {
                QJsonObject caloriesEntry;
                caloriesEntry["timestamp"] = time;
                caloriesEntry["calories"] = tp->calories();
                calories.append(caloriesEntry);
            }

            {
                QJsonObject distanceEntry;
                distanceEntry["distance"] = tp->cummulativeDistance();
                distanceEntry["timestamp"] = time;
                distance.append(distanceEntry);
            }
        }
    }

    af["heart_rate"] = heartRate;
    af["calories"] = calories;
    af["path"] = path;
    af["distance"] = distance;

    d.setObject(af);

    QByteArray data = d.toJson();


    QNetworkRequest r( QUrl("http://api.runkeeper.com/fitnessActivities"));
    r.setRawHeader("Authorization", "Bearer " + m_Config.authToken());
    r.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.com.runkeeper.NewFitnessActivity+json");

    QNetworkReply * reply = m_Manager.post(r, data);
    reply->setProperty("REQUEST", POST_ACTIVITY);
}


void RunKeeperExporter::onRequestFinished(QNetworkReply *reply)
{
    int type = reply->property("REQUEST").toInt();

    switch ( type )
    {
    case POST_ACTIVITY:
        onActivityPosted(reply);
        break;
    case GET_USER_URI:
        onGetUserId(reply);
        break;
    case AUTH_REQUEST:
        onAuthRequest(reply);
        break;
    default:
        qCritical() << "RunKeeperExporter::onRequestFinished / unknown reply";
    }

    reply->deleteLater();
}

void RunKeeperExporter::onActivityPosted(QNetworkReply *reply)
{

    if ( reply->error() != QNetworkReply::NoError )
    {
        emit exportFinished(false, tr("Runkeeper: Failed to export"), QUrl());
        return;
    }

    QString location = reply->rawHeader("Location");

    int slashPos = location.lastIndexOf("/");
    location = location.mid(slashPos+1);

    QNetworkRequest r( QUrl("http://api.runkeeper.com/profile"));
    r.setHeader(QNetworkRequest::ContentTypeHeader, "application/vnd.com.runkeeper.Profile+json");
    r.setRawHeader("Authorization", "Bearer " + m_Config.authToken());
    QNetworkReply * reply2 = m_Manager.get(r);
    reply2->setProperty("REQUEST", GET_USER_URI);
    reply2->setProperty("LOCATION", location);
}

void RunKeeperExporter::onGetUserId(QNetworkReply *reply)
{
    if ( reply->error() != QNetworkReply::NoError)
    {
        emit exportFinished(false, tr("Runkeeper: Failed to export (2)"), QUrl());
        return;
    }

    QString location = reply->property("LOCATION").toString();

    QJsonDocument d = QJsonDocument::fromJson( reply->readAll() );
    QJsonObject o = d.object();
    if ( o.contains("profile"))
    {
        QString profile = o["profile"].toString();
        emit exportFinished(true, tr("Runkeeper: Exported Done."), QUrl( profile + location ));
        if ( m_Config.isAutoOpen() )
        {
            QDesktopServices::openUrl(QUrl( profile + "/activity/" + location ));
        }
    }
}

void RunKeeperExporter::onAuthRequest(QNetworkReply *reply)
{
    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(data, &pe);

    if ( httpCode != 200 )
    {
        m_Config.setAuthToken("");
        emit setupFinished(this, false);
        return;
    }

    QJsonObject response = d.object();
    if ( response.contains("access_token"))
    {
        m_Config.setAuthToken( response["access_token"].toString().toUtf8() );
        emit setupFinished(this, true);
    }
    else
    {
        m_Config.setAuthToken("");
        emit setupFinished(this, false);
    }
}


QString RunKeeperExporter::toRKDate(const QDateTime &sourceDate)
{
    QString date;

    switch ( sourceDate.date().dayOfWeek())
    {
    case 1:
        date +="Mon";
        break;
    case 2:
        date +="Tue";
        break;
    case 3:
        date +="Wed";
        break;
    case 4:
        date +="Thu";
        break;
    case 5:
        date +="Fri";
        break;
    case 6:
        date +="Sat";
        break;
    case 7:
        date +="Sun";
        break;
    }

    date += ", " + QString::number(sourceDate.date().day()) + " ";
    switch ( sourceDate.date().month() )
    {
    case 1:
        date+="Jan";
        break;
    case 2:
        date+="Feb";
        break;
    case 3:
        date+="Mar";
        break;
    case 4:
        date+="Apr";
        break;
    case 5:
        date+="May";
        break;
    case 6:
        date+="Jun";
        break;
    case 7:
        date+="Jul";
        break;
    case 8:
        date+="Aug";
        break;
    case 9:
        date+="Sep";
        break;
    case 10:
        date+="Oct";
        break;
    case 11:
        date+="Nov";
        break;
    case 12:
        date+="Dec";
        break;
    }
    return date + sourceDate.toString(" yyyy hh:mm:ss");
}
