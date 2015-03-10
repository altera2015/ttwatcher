#ifndef ELEVATIONLOADER_H
#define ELEVATIONLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include "activity.h"

class ElevationLoader : public QObject
{
    Q_OBJECT

    QNetworkAccessManager m_Manager;

public:
    ElevationLoader( QObject * parent = 0);
    void load( ActivityPtr activity );

private slots:
    void finished( QNetworkReply * reply );
signals:
    void loaded( bool success, ActivityPtr activity );
private:
    bool process(ActivityPtr activity, QByteArray data );
};

#endif // ELEVATIONLOADER_H
