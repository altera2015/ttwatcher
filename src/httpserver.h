#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QObject>
#include <qhttpserver.h>
#include <qhttprequest.h>
#include <qhttpresponse.h>
#include <functional>
#include <QMap>

class HttpServer : public QObject
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = 0);

    static HttpServer * singleton(bool destroy=false);
    quint16 port() const;

    typedef std::function< void ( QHttpRequest *req, QHttpResponse *resp ) > Callback;
    QUrl registerCallback( Callback cb );
    void unregisterCallback ( const QString & path );

    void respond(const QString & filename, int code, QHttpRequest *req, QHttpResponse *resp);
signals:

private slots:
    void handle(QHttpRequest *req, QHttpResponse *resp);

private:
    quint16 m_Port;
    QHttpServer m_Server;
    QMap< QString, Callback > m_Callbacks;
};

#endif // HTTPSERVER_H
