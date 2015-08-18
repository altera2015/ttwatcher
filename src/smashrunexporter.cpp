#include "smashrunexporter.h"
#include "watchexporters.h"
#include "tcxexport.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QByteArray>
#include <QBuffer>
#include <QNetworkProxy>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>

#include "httpserver.h"
#include "singleshot.h"

#include "smashrun_auth.h"

#define AUTH_REQUEST                1
#define ACTIVITY_SUBMIT             2
#define REFRESH_TOKEN_REQUEST       3

SmashrunExporter::SmashrunExporter(const QString &serial, QObject *parent) :
    IActivityExporter(parent),
    m_Icon( ":/icons/smashrunlogo.png"),
    m_Config(serial)
{
    connect(&m_Manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFinished(QNetworkReply*)));
#ifdef USE_DEBUG_PROXY
    QNetworkProxy p;
    p.setHostName("localhost");
    p.setPort(8888);
    p.setType(QNetworkProxy::HttpProxy);
    m_Manager.setProxy(p);
#endif
}

QString SmashrunExporter::name() const
{
    return "Smashrun";
}


QIcon SmashrunExporter::icon() const
{
    return m_Icon;
}

bool SmashrunExporter::hasSetup() const
{
    return true;
}

void SmashrunExporter::setup(QWidget *parent)
{
    HttpServer * server = HttpServer::singleton();

    QMessageBox b(QMessageBox::Information, tr("Opening Smashrun Authorization"), tr("This box will close when done, or cancelled.\nNote you might have to allow TTWatcher access in your firewall, a popup might appear for that."), QMessageBox::Cancel, parent );

    QUrl cburl = server->registerCallback( [this, server, &b](QHttpRequest *req, QHttpResponse *resp) {

        server->respond(":/html/smashrun.html", 200, req, resp);

        QUrlQuery codeQuery;
        codeQuery.setQuery( req->url().query() );
        QString code = codeQuery.queryItemValue("code");

        // now exchange for the auth token.
        QNetworkRequest r;
        QUrl url("https://secure.smashrun.com/oauth2/token");
        QUrlQuery q;
        q.addQueryItem("client_id", SMASHRUN_CLIENT_ID);
        q.addQueryItem("client_secret", SMASHRUN_CLIENT_SECRET);
        q.addQueryItem("code", code);
        q.addQueryItem("grant_type", "authorization_code");

        r.setUrl( url );

        r.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        QNetworkReply * reply = m_Manager.post( r, q.toString(QUrl::FullyEncoded).toUtf8() );
        reply->setProperty("REQUEST", AUTH_REQUEST);

        b.close(); // will continue execute below after b.exec, which then deletes the url.

    });

    QUrl smashrunAuthRequest ("https://secure.smashrun.com/oauth2/authenticate");
    QUrlQuery q;
    q.addQueryItem("client_id", SMASHRUN_CLIENT_ID);
    q.addQueryItem("response_type", "code");
    q.addQueryItem("redirect_uri",  QUrl::toPercentEncoding( cburl.toString(QUrl::FullyEncoded) ));
    q.addQueryItem("scope", "write_activity");

    smashrunAuthRequest.setQuery(q);

    QDesktopServices::openUrl(smashrunAuthRequest);

    b.exec();

    server->unregisterCallback(cburl.path());
}

void SmashrunExporter::authCodeAnswer(QJsonDocument &d, int httpCode)
{
    if ( httpCode != 200 )
    {
        m_Config.reset();
        emit setupFinished(this, false);
        return;
    }

    QJsonObject response = d.object();
    if ( response.contains("access_token"))
    {
        m_Config.setAuthToken(response["access_token"].toString().toUtf8());
        m_Config.setRefreshToken(response["refresh_token"].toString().toUtf8());
        m_Config.setLastRefresh(QDateTime::currentDateTime());
        emit setupFinished(this, true);
    }
    else
    {
        m_Config.reset();
        emit setupFinished(this, false);
    }
}



IExporterConfig &SmashrunExporter::config()
{
    return m_Config;
}

IExporterConfig * SmashrunExporter::createConfig()
{
    return new SmashrunExporterConfig( m_Config.serial() );
}

void SmashrunExporter::exportActivity(ActivityPtr activity)
{
    if ( !activity )
    {
        return;
    }

    QNetworkRequest r;
    r.setUrl(QUrl("https://api.smashrun.com/v1/my/activities/"));
    QByteArray bearer("Bearer ");
    bearer.append(m_Config.authToken());
    r.setRawHeader("Authorization", bearer);

    TCXExport tcxexport;
    QByteArray data;
    QBuffer dataBuffer(&data);
    dataBuffer.open(QIODevice::WriteOnly);
    tcxexport.save(&dataBuffer, activity);
    dataBuffer.close();
    m_ActivityAfterRefresh = activity;
    r.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");

    QNetworkReply * reply = m_Manager.post(r, data);    
    reply->setProperty("REQUEST", ACTIVITY_SUBMIT);
}

void SmashrunExporter::activitySubmitted(QJsonDocument &d, int httpCode)
{
    if ( httpCode != 200 )
    {
        refreshAuth();
        return;
    }
    m_ActivityAfterRefresh.clear();

    // {"activityIds":["2790031"]}
    QJsonObject o = d.object();
    if ( o.contains("activityIds") )
    {
        QUrl u;
        emit exportFinished(true, tr("Smashrun: Upload done"), u);
        /* QUrl u(QString("https://www.strava.com/activities/%1").arg( response["activity_id"].toInt() ));

        if ( m_AutoOpen )
        {
            QDesktopServices::openUrl(u);
        }*/
        qDebug() << o["activityIds"].toString();
    }
}

void SmashrunExporter::refreshCodeAnswer(QJsonDocument &d, int httpCode)
{
    if ( httpCode != 200 )
    {
        emit exportFinished(false, tr("Smashrun: Failed to authenticate."), QUrl());
        m_ActivityAfterRefresh.reset();
        m_Config.reset();
        emit settingsChanged(this);
        return;
    }

    QJsonObject response = d.object();
    if ( response.contains("access_token"))
    {
        m_Config.setAuthToken(response["access_token"].toString().toUtf8());
        m_Config.setRefreshToken(response["refresh_token"].toString().toUtf8());
        m_Config.setLastRefresh(QDateTime::currentDateTime());
        if ( m_ActivityAfterRefresh )
        {
            exportActivity(m_ActivityAfterRefresh);
            m_ActivityAfterRefresh.reset();
        }
        emit settingsChanged(this);
        return;
    }

    m_Config.reset();
    m_ActivityAfterRefresh.reset();
    emit settingsChanged(this);
}

void SmashrunExporter::refreshAuth()
{
    // now exchange for the auth token.
    QNetworkRequest r;
    QUrl url("https://secure.smashrun.com/oauth2/token");
    QUrlQuery q;
    q.addQueryItem("client_id", SMASHRUN_CLIENT_ID);
    q.addQueryItem("client_secret", SMASHRUN_CLIENT_SECRET);
    q.addQueryItem("code", QString::fromUtf8(m_Config.refreshToken()));
    q.addQueryItem("grant_type", "refresh_token");

    r.setUrl( url );

    r.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply * reply = m_Manager.post( r, q.toString(QUrl::FullyEncoded).toUtf8() );
    reply->setProperty("REQUEST", REFRESH_TOKEN_REQUEST);
}



void SmashrunExporter::requestFinished(QNetworkReply *reply)
{

    int requestType = reply->property("REQUEST").toInt();

    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();

    qDebug() << "SmashrunExporter " << data;

    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson(data, &pe);


    if ( pe.error != QJsonParseError::NoError )
    {
        qCritical() << "SmashrunExporter::requestFinished / JSON Parse Error. " << pe.errorString() << requestType << data;
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
    case REFRESH_TOKEN_REQUEST:
        refreshCodeAnswer(d, httpCode);
        break;

    default:
        qCritical() << "SmashrunExporter::requestFinished / unknown request type received " << requestType;
    }

    reply->deleteLater();
}


