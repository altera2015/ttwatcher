#include "httpserver.h"
#include <QUuid>
#include <QFile>

HttpServer::HttpServer(QObject *parent) :
    QObject(parent),
    m_Port(9999)
{    
    while ( !m_Server.listen(m_Port) )
    {
        m_Port++;
    }

    qDebug() << "HttpServer::HttpServer / started on port " << m_Port;

    connect(&m_Server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(handle(QHttpRequest*, QHttpResponse*)));
}

HttpServer *HttpServer::singleton(bool destroy)
{
    static HttpServer * server = 0;

    if ( destroy )
    {
        if ( server != 0 )
        {
            server->deleteLater();
        }
        return 0;
    }
    else
    {
        if ( server == 0 )
        {
            server = new HttpServer();
        }

        return server;
    }
}

quint16 HttpServer::port() const
{
    return m_Port;
}

QUrl HttpServer::registerCallback(HttpServer::Callback cb)
{
    while ( true )
    {
        QUuid d =QUuid::createUuid();
        QString uuid = d.toString();
        if ( uuid.startsWith("{"))
        {
            uuid = uuid.mid(1);
        }

        if ( uuid.endsWith("}"))
        {
            uuid.chop(1);
        }

        QUrl url( QString("http://localhost:%1/cb/%2/index.html").arg(m_Port).arg(uuid));
        QString path = url.path();

        if (!m_Callbacks.contains( path ))
        {
            m_Callbacks[ path ] = cb;
        }
        return url;
    }
}

void HttpServer::unregisterCallback(const QString &path)
{
    if ( m_Callbacks.contains(path))
    {
        m_Callbacks.remove(path);
    }
}

void HttpServer::respond(const QString &filename, int code, QHttpRequest *req, QHttpResponse *resp)
{
    QFile f(filename);
    if ( f.open(QIODevice::ReadOnly) )
    {
        QByteArray data = f.readAll();
        f.close();
        resp->setHeader("Content-Length", QString::number(data.length()));
        resp->writeHead(code);

        if ( filename.endsWith(".html"))
        {
            resp->setHeader("Content-Type", "text/html");
        }
        else if ( filename.endsWith(".png"))
        {
            resp->setHeader("Content-Type", "image/png");
        }
        else if ( filename.endsWith(".jpg"))
        {
            resp->setHeader("Content-Type", "image/jpg");
        }
        else
        {
            resp->setHeader("Content-Type", "text/html");
        }
        resp->write(data);
        resp->end();
    }
    else
    {
        if ( filename != ":/html/404.html")
        {
            respond(":/html/404.html", 404, req, resp);
        }
        else
        {
            QString serverError = tr("<html><body><h1>Server Error</h1></body></html>");
            QByteArray data = serverError.toUtf8();
            resp->setHeader("Content-Length", QString::number(data.length()));
            resp->writeHead(505);
            resp->setHeader("Content-Type", "text/html");
            resp->write(data);
            resp->end();
        }
    }
}

void HttpServer::handle(QHttpRequest *req, QHttpResponse *resp)
{
    QString path = req->url().path();

    if ( m_Callbacks.contains(path))
    {
        m_Callbacks[path](req,resp);
    }
    else
    {
        if ( !path.startsWith("/"))
        {
            path = "/" + path;
        }
        respond(":/html/data" + path, 200, req, resp);
    }
}
