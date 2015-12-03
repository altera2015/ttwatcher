#include "elevationdownloaderitem.h"
#include <QIcon>

ElevationTileDownloaderItem::ElevationTileDownloaderItem(int id, QObject *parent) :
    QObject(parent),
    m_Id(id),
    m_Retries(0),
    m_Progress(0),
    m_Status(WAITING),
    f(0),
    reply(0)
{
}

ElevationTileDownloaderItem::~ElevationTileDownloaderItem()
{
    if ( f )
    {
        f->deleteLater();
    }
    if ( reply )
    {
        reply->deleteLater();
    }
}

QVariant ElevationTileDownloaderItem::data(int column, int role) const
{

    switch ( column )
    {
    case 0:

        if ( role != Qt::DecorationRole )
        {
            break;
        }

        switch ( m_Status )
        {
        case WAITING:
            return QIcon(":/icons/time28.png");
        case DOWNLOADING:
            return QIcon(":/icons/internet43.png");
        case UNPACKING:
            return QIcon(":/icons/boxes37.png");
        case SUCCESS:
            return QIcon(":/icons/ok.png");
        case FAILED:
            return QIcon(":/icons/test20.png");
        }

        break;
    case 1:

        if ( role != Qt::DisplayRole )
        {
            break;
        }

        return name;
    case 2:

        if ( role != Qt::DisplayRole )
        {
            break;
        }

        return QString("Progress: %1%, Retries: %2").arg(m_Progress, 0, 'f', 2).arg(m_Retries);
    }

    return QVariant();
}

void ElevationTileDownloaderItem::setRetries(int retries)
{
    if ( m_Retries != retries )
    {
        m_Retries = retries;
        emit cellChanged(m_Id);
    }
}

int ElevationTileDownloaderItem::retries() const
{
    return m_Retries;
}

void ElevationTileDownloaderItem::setProgress(float progress)
{
    if ( progress < 0 )
    {
        progress = 0;
    }
    if ( progress > 100 )
    {
        progress = 100;
    }

    if ( m_Progress != progress )
    {
        m_Progress = progress;
        emit cellChanged(m_Id);
    }
}

void ElevationTileDownloaderItem::setStatus(ElevationTileDownloaderItem::Status status)
{
    if ( m_Status != status )
    {
        m_Status = status;
        emit cellChanged(m_Id);
    }
}

ElevationTileDownloaderItem::Status ElevationTileDownloaderItem::status() const
{
    return m_Status;
}

int ElevationTileDownloaderItem::id() const
{
    return m_Id;
}
