#include "stravaexporter.h"
#include "watchpreferences.h"
#include "tcxexport.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QByteArray>
#include <QBuffer>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDomNamedNodeMap>

#include "httpserver.h"
#include "singleshot.h"

#include "strava_auth.h"

#define AUTH_REQUEST    1
#define ACTIVITY_SUBMIT 2
#define ACTIVITY_STATUS 3

StravaExporter::StravaExporter(QObject *parent) :
    IActivityExporter(parent),
    m_Enabled(false),
    m_AutoOpen(false),
    m_Icon( ":/icons/strava.png")
{

    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
    QNetworkProxy p;
    p.setHostName("localhost");
    p.setPort(8888);
    p.setType(QNetworkProxy::HttpProxy);
    m_Manager.setProxy(p);

}

QString StravaExporter::name() const
{
    return "Strava";
}

bool StravaExporter::loadConfig(const WatchPreferences & preferences, QDomElement element)
{
    parseExportTag(element, "Strava", m_Enabled, m_AutoOpen);

    QDomElement token = element.firstChildElement("StravaAuthToken");
    if ( !token.isNull() )
    {
        m_AuthToken = preferences.decodeToken( token.text() );
    }

    return true;
}

bool StravaExporter::isEnabled() const
{
    return m_Enabled;
}

void StravaExporter::setEnabled(bool enabled)
{
    m_Enabled = enabled;
}

bool StravaExporter::isOnline() const
{
    return true;
}

bool StravaExporter::autoOpen() const
{
    return m_AutoOpen;
}

void StravaExporter::setAutoOpen(bool autoOpen)
{
    m_AutoOpen = autoOpen;
}

QIcon StravaExporter::icon() const
{
    return m_Icon;
}

void StravaExporter::reset()
{
    m_Enabled = false;
    m_AutoOpen = false;
    m_AuthToken = "";
}

bool StravaExporter::hasSetup() const
{
    return true;
}

void StravaExporter::setup(QWidget *parent)
{
    HttpServer * server = HttpServer::singleton();

    QMessageBox b(QMessageBox::Information, tr("Opening Strava Authorization"), tr("This box will close when done, or cancelled.\nNote you might have to allow TTWatcher access in your firewall, a popup might appear for that."), QMessageBox::Cancel, parent );

    QUrl cburl = server->registerCallback( [this, server, &b](QHttpRequest *req, QHttpResponse *resp) {

        server->respond(":/html/strava.html", 200, req, resp);

        QUrlQuery codeQuery;
        codeQuery.setQuery( req->url().query() );
        QString code = codeQuery.queryItemValue("code");

        // now exchange for the auth token.
        QNetworkRequest r;
        QUrl url("https://www.strava.com/oauth/token");
        QUrlQuery q;
        q.addQueryItem("client_id", STRAVA_CLIENT_ID);
        q.addQueryItem("client_secret", STRAVA_CLIENT_SECRET);
        q.addQueryItem("code", code);

        r.setUrl( url );

        r.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply * reply = m_Manager.post( r, q.toString(QUrl::FullyEncoded).toUtf8() );
        reply->setProperty("REQUEST", AUTH_REQUEST);

        b.close(); // will continue execute below after b.exec, which then deletes the url.

    });

    QUrl stravaAuthRequest ("https://www.strava.com/oauth/authorize");
    QUrlQuery q;
    q.addQueryItem("client_id", STRAVA_CLIENT_ID);
    q.addQueryItem("response_type", "code");
    q.addQueryItem("redirect_uri",  QUrl::toPercentEncoding( cburl.toString(QUrl::FullyEncoded) ));
    q.addQueryItem("scope", "write,view_private");
    q.addQueryItem("response_type", "code");
    stravaAuthRequest.setQuery(q);

    QDesktopServices::openUrl(stravaAuthRequest);

    b.exec();

    server->unregisterCallback(cburl.path());
}


void StravaExporter::authCodeAnswer(QJsonDocument &d, int httpCode)
{
    if ( httpCode != 200 )
    {
        emit setupFinished(this, false);
        return;
    }

    QJsonObject response = d.object();
    if ( response.contains("access_token"))
    {
        m_AuthToken = response["access_token"].toString().toUtf8();
        emit setupFinished(this, true);
    }
    else
    {
        emit setupFinished(this, false);
    }
}


void StravaExporter::saveConfig(const WatchPreferences & preferences, QDomDocument &document, QDomElement &element)
{

    writeExportTag(document, element, "Strava", m_Enabled, m_AutoOpen);

    QDomElement oldAuthToken = element.firstChildElement("StravaAuthToken");
    if ( !oldAuthToken.isNull() )
    {
        element.removeChild(oldAuthToken);
    }


    if ( m_AuthToken.length() > 0 )
    {
        QDomElement e = document.createElement("StravaAuthToken");
        QDomText text = document.createTextNode(preferences.encodeToken( m_AuthToken ));
        element.appendChild(e);
        e.appendChild(text);
    }
}

void StravaExporter::exportActivity(ActivityPtr activity)
{
    if ( !activity )
    {
        return;
    }

    QNetworkRequest r;
    r.setUrl(QUrl("https://www.strava.com/api/v3/uploads"));
    QByteArray bearer("Bearer ");
    bearer.append(m_AuthToken);
    r.setRawHeader("Authorization", bearer);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_type\""));
    switch (activity->sport()) {
    case Activity::RUNNING:
        activityTypePart.setBody("run");
        break;
    case Activity::BIKING:
        activityTypePart.setBody("ride");
        break;
    case Activity::SWIMMING:
        activityTypePart.setBody("swim");
        break;
    case Activity::TREADMILL:
        activityTypePart.setBody("run");
        break;
    default:
        activityTypePart.setBody("run");
        break;
    }

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("tcx");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"TTWatcher.tcx\""));

    TCXExport tcxexport;
    QByteArray data;
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::WriteOnly);
    tcxexport.save(&dataBuffer, activity);
    dataBuffer.close();

    filePart.setBody(data);

    multiPart->append(activityTypePart);
    multiPart->append(dataTypePart);
    multiPart->append(filePart);


    QNetworkReply * reply = m_Manager.post(r, multiPart);
    multiPart->setParent(reply);
    reply->setProperty("REQUEST", ACTIVITY_SUBMIT);
}


void StravaExporter::activitySubmitted(QJsonDocument &d, int httpCode)
{
    if ( httpCode == 401 )
    {
        emit exportFinished(false, tr("Your account has been de-authorized."), QUrl());
        return;
    }

    QJsonObject o = d.object();

    if ( o.contains("error") && !o["error"].isNull() )
    {
        QString error = o["error"].toString();
        if ( error.contains("duplicate of activity") )
        {
            error = error.trimmed();
            int pos = error.lastIndexOf(" ");
            error = error.mid(pos+1);
            QUrl u(QString("https://www.strava.com/activities/%1").arg(error));
            emit exportFinished(true, tr("Duplicate"), u);
            return;
        }
        else
        {
            emit exportFinished(false, error, QUrl());
        }

    }

    if ( o.contains("activity_id") && !o["activity_id"].isNull() )
    {
        QUrl u(QString("https://www.strava.com/activities/%1").arg(o["activity_id"].toInt()));
        emit exportFinished(true, tr("Export Successful"), u);
        return;
    }

    if ( o.contains("id"))
    {
        int id = o["id"].toInt();
        SingleShot::go([this, id]() {
            getActivityStatus(id, 0);
        }, 4000, true, this);
    }
    else
    {
        emit exportFinished(false, tr("Response Error"), QUrl());
    }
}

void StravaExporter::getActivityStatus(int uploadId, int retry)
{
    QNetworkRequest r;
    r.setUrl(QUrl( QString("https://www.strava.com/api/v3/uploads/%1").arg(uploadId)));
    QByteArray bearer("Bearer ");
    bearer.append(m_AuthToken);
    r.setRawHeader("Authorization", bearer);

    QNetworkReply * reply = m_Manager.get( r );
    reply->setProperty("REQUEST", ACTIVITY_STATUS);
    reply->setProperty("ACTIVITY_STATUS_RETRY", retry);
    reply->setProperty("ACTIVITY_UPLOAD_ID", uploadId);

}

void StravaExporter::activityStatus(QJsonDocument &d, int httpCode, QNetworkReply * reply)
{
    if ( httpCode == 404 )
    {
        emit exportFinished(false, tr("Not Found"), QUrl());
        return;
    }

    int uploadId = reply->property("ACTIVITY_UPLOAD_ID").toInt();
    int retry = reply->property("ACTIVITY_STATUS_RETRY").toInt();
    QJsonObject response = d.object();
    if ( response.contains("activity_id") && !response["activity_id"].isNull() )
    {
        QUrl u(QString("https://www.strava.com/activities/%1").arg( response["activity_id"].toInt() ));
        emit exportFinished(true, tr("Upload done"), u);
    }
    else
    {
        retry++;
        if ( retry < 4 )
        {
            SingleShot::go([this, uploadId,retry]() {
                getActivityStatus(uploadId, retry);
            }, 4000, true, this);
        }
        else
        {
            emit exportFinished(false, tr("Processing taking too long"), QUrl());
        }

    }
}

void StravaExporter::requestFinished(QNetworkReply *reply)
{
    int requestType = reply->property("REQUEST").toInt();
    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();


    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(data, &pe);


    if ( pe.error != QJsonParseError::NoError )
    {
        qCritical() << "StravaExporter::requestFinished / JSON Parse Error. " << pe.errorString() << requestType << data;
        reply->deleteLater();
        return;
    }

    switch ( requestType )
    {
    case ACTIVITY_SUBMIT:
        activitySubmitted(d, httpCode);
        break;
    case AUTH_REQUEST:
        authCodeAnswer(d, httpCode);
        break;
    case ACTIVITY_STATUS:
        activityStatus(d, httpCode, reply);
        break;
    default:
        qCritical() << "StravaExporter::requestFinished / unknown request type received " << requestType;
    }

    reply->deleteLater();
}

