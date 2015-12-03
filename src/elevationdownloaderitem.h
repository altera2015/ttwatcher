#ifndef ELEVATIONDOWNLOADERITEM_H
#define ELEVATIONDOWNLOADERITEM_H

#include <QObject>
#include <QTemporaryFile>
#include <QNetworkReply>

class ElevationTileDownloaderItem : public QObject
{
    Q_OBJECT

public:
    explicit ElevationTileDownloaderItem(int id, QObject *parent = 0);
    ~ElevationTileDownloaderItem();
    QVariant data(int column, int role) const;

    QString name;
    QStringList urls;
    QString destDir;


    QTemporaryFile * f;
    QNetworkReply * reply;

    void setRetries(int retries);
    int retries() const;
    void setProgress(float progress);
    enum Status { WAITING, DOWNLOADING, UNPACKING, SUCCESS, FAILED };
    void setStatus( Status status );
    Status status() const;
    int id() const;
signals:
    void cellChanged(int id);

public slots:

private:
    int m_Id;
    int m_Retries;
    float m_Progress;

    Status m_Status;

};

typedef QList<ElevationTileDownloaderItem*> ElevationDownloaderItemList;

#endif // ELEVATIONDOWNLOADERITEM_H
