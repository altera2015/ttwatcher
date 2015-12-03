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

    enum Status { IDLE, DOWNLOADING, SUCCESS, FAILED };
    enum Source { USE_TT, USE_SRTMANDNED };



    Status load( ActivityPtr activity, Source source = USE_SRTMANDNED, bool synchronous = false, bool fullReload = false );
    Status status() const;
private slots:
    void finished( QNetworkReply * reply );
signals:
    void loaded( bool success, ActivityPtr activity );
private:
    Status m_Status;
    Status loadTT(ActivityPtr activity,bool synchronous );
    Status loadSRTM(ActivityPtr activity);
    bool process(ActivityPtr activity, QByteArray data );
};

#endif // ELEVATIONLOADER_H
