#include "ttmanager.h"
#include <QTimer>
#include <QDebug>
#include "hidapi.h"


void TTManager::checkvds(quint16 vid, const DeviceIdList &deviceIds)
{
    // prepare list of currently attached TT's
    QString currentPaths;
    foreach ( const TTWatch * watch, m_TTWatchList )
    {
        currentPaths.append( watch->path() );
    }


    // get all devices for vendor id.
    hid_device_info * devices = hid_enumerate(vid, 0);
    if ( devices != 0 )
    {
        while ( devices != 0 )
        {
            // find matching devices
            if ( deviceIds.contains(devices->product_id))
            {
                //QString ms = QString::fromWCharArray( devices->manufacturer_string );
                //QString ds = QString::fromWCharArray( devices->product_string );
                QString serial = QString::fromWCharArray( devices->serial_number );
                QString path = devices->path;

                TTWatch * w = find(path);
                if ( w )
                {
                    currentPaths.remove(path);
                }
                else
                {
                    m_TTWatchList.append( new TTWatch( path, serial, this ) );
                    emit ttArrived();
                }

            }
            devices = devices->next;
        }

        hid_free_enumeration(devices);
    }


    TTWatchList wl;
    foreach ( const QString & path, currentPaths )
    {
        TTWatch * w = find(path);
        if ( w )
        {
            wl.append(w);
            m_TTWatchList.removeOne(w);
        }
    }

    if ( currentPaths.length() > 0 )
    {
        emit ttRemoved();
    }

    foreach ( TTWatch * w, wl )
    {
        w->deleteLater();
    }

}

TTManager::TTManager(QObject *parent) :
    QObject(parent)
{

    if ( hid_init() < 0 )
    {
        qCritical() << "TTManager::TTManager / could not initialize HID library.";
        return;
    }


}

TTManager::~TTManager()
{
    hid_exit();
}

void TTManager::startSearch()
{
    QTimer * t = new QTimer(this);
    t->setInterval(10000);
    connect(t, SIGNAL(timeout()), this, SLOT(checkForTTs()));
    t->start();
    checkForTTs();
}

const TTWatchList &TTManager::watches()
{
    return m_TTWatchList;
}

TTWatch *TTManager::watch(const QString &serial)
{
    foreach ( TTWatch * watch, m_TTWatchList)
    {
        if ( watch->serial() == serial )
        {
            return watch;
        }
    }
    return 0;
}

TTWatch *TTManager::find(const QString &path)
{
    TTWatchList::iterator i = m_TTWatchList.begin();
    for ( ;i != m_TTWatchList.end(); i++)
    {
        TTWatch * w = (*i);

        if ( w->path() == path )
        {
            return w;
        }
    }

    return 0;
}

bool TTManager::remove(const QString &path)
{
    TTWatchList::iterator i = m_TTWatchList.begin();
    for ( ;i != m_TTWatchList.end(); i++)
    {
        TTWatch * w = (*i);
        if ( w->path() == path )
        {
            m_TTWatchList.erase(i);
            w->deleteLater();
            return true;
        }
    }
    return false;
}

void TTManager::checkForTTs()
{
    DeviceIdList deviceIdList;
    deviceIdList.append(0x7474);
    checkvds(0x1390, deviceIdList);
}
