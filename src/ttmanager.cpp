#include "ttmanager.h"
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QStandardPaths>
#include "hidapi.h"
#include "watchpreferences.h"

void TTManager::checkvds(quint16 vid, const DeviceIdList &deviceIds)
{
    // prepare list of currently attached TT's
    QStringList currentPaths;
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
                    currentPaths.removeOne(path);
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
    loadPreferences();

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
    t->setInterval(60000);
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

PreferencesMap & TTManager::preferences()
{
    return m_Preferences;
}

WatchPreferencesPtr TTManager::preferences(const QString &serial)
{
    if ( m_Preferences.contains(serial))
    {
        return m_Preferences[serial];
    }

    WatchPreferencesPtr wp = WatchPreferencesPtr::create(serial);
    connect(wp.data(), SIGNAL(allExportsFinished()), this, SIGNAL(allExportingFinished()));
    connect(wp.data(), SIGNAL(exportError(QString)), this, SIGNAL(exportError(QString)));
    m_Preferences[ serial ] = wp;
    return wp;
}

WatchPreferencesPtr TTManager::preferencesForName(const QString &name)
{
    PreferencesMap::iterator i = m_Preferences.begin();

    for(;i!=m_Preferences.end();i++)
    {
        if ( i.value()->name() == name )
        {
            return i.value();
        }
    }

    return WatchPreferencesPtr();
}

WatchPreferencesPtr TTManager::defaultPreferences()
{
    return preferences("DEFAULT");
}

void TTManager::savePreferences(WatchPreferencesPtr preferences)
{
    QDomDocument d;
    QDomElement preferencesElement = d.createElement("preferences");
    d.appendChild(preferencesElement);

    QDomElement nameElement = d.createElement("watchName");
    nameElement.appendChild(  d.createTextNode( preferences->name() ) );
    preferencesElement.appendChild(nameElement);

    QDomElement serialElement = d.createElement("serial");
    serialElement.appendChild(  d.createTextNode( preferences->serial() ) );
    preferencesElement.appendChild(serialElement);

    QDomElement exportersElement = d.createElement("exporters");
    preferencesElement.appendChild(exportersElement);

    QDomElement onlineElement = d.createElement("online");
    exportersElement.appendChild(onlineElement);

    QDomElement offlineElement = d.createElement("offline");
    exportersElement.appendChild(offlineElement);

    foreach ( IActivityExporterPtr exp, preferences->exporters())
    {
        exp->saveConfig( *preferences.data(), d, exp->isOnline() ? onlineElement : offlineElement );
    }

    QString path = preferenceDir();
    QDir mkpathd;
    mkpathd.mkpath(path);

    QString filename = path + QDir::separator() + "watchPrefs_" + preferences->serial() + ".xml";

    QFile f( filename );
    if ( !f.open(QIODevice::WriteOnly))
    {
        qCritical() << "TTManager::savePreferences / could not write preferences file " << filename;
        return;
    }
    f.write( d.toByteArray() );
    f.close();
}

void TTManager::savePreferences()
{
    PreferencesMap::iterator i = m_Preferences.begin();

    for(;i!=m_Preferences.end();i++)
    {

        WatchPreferencesPtr preferences = i.value();

        if ( preferences->name().length() == 0 )
        {
            continue;
        }

        bool changed = false;
        foreach ( IActivityExporterPtr exp, preferences->exporters())
        {
            changed |= exp->changed();
        }

        if ( !changed )
        {
            continue;
        }

        TTWatch * w = watch( preferences->serial() );

        if ( w )
        {
            savePreferences(preferences);

            QByteArray prefData;
            if ( !w->downloadPreferences(prefData))
            {
                qCritical() << "TTManager::savePreferences / could not load preferences.";
                continue;
            }

            prefData = preferences->updatePreferences(prefData);

            if ( !w->uploadPreferences(prefData) )
            {
                qCritical() << "TTManager::savePreferences / could not save preferences to watch.";
            }
        }
        else
        {
            // watch not plugged in, could still be the default device...
            if ( preferences->serial() == "DEFAULT" )
            {
                savePreferences(preferences);
            }
            else
            {
                // nope, really not here, revert to previous
                qWarning() << "TTManager::savePreferences / watch gone, reverting changes.";
                QString filename = preferenceDir() + QDir::separator() + "watchPrefs_" + preferences->serial() + ".xml";
                loadPreferences(filename);
            }
        }
    }
}

void TTManager::loadPreferences(const QString & filename)
{
    QFile f( filename );
    if ( !f.open(QIODevice::ReadOnly))
    {
        qDebug() << "TTManager::loadPreferences / could not load preferences.";
        return;
    }

    QByteArray xml = f.readAll();
    f.close();

    QDomDocument d;
    d.setContent(xml);

    QDomElement preferencesElement = d.firstChildElement("preferences");
    if ( preferencesElement.isNull() )
    {
        qDebug() << "TTManager::loadPreferences / not a preferences XML file. " << filename;
        return;
    }

    QDomElement nameElement = preferencesElement.firstChildElement("watchName");
    QDomElement serialElement = preferencesElement.firstChildElement("serial");
    if ( nameElement.isNull() || serialElement.isNull() )
    {
        qDebug() << "TTManager::loadPreferences / missing element name or serial. " << filename;
        return;
    }

    QString name = nameElement.text();
    QString serial = serialElement.text();

    WatchPreferencesPtr p = preferences( serial );
    p->setName(name);

    QDomElement exportersElement = preferencesElement.firstChildElement("exporters");

    QDomElement onlineElement = exportersElement.firstChildElement("online");
    QDomElement offlineElement = exportersElement.firstChildElement("offline");

    foreach ( IActivityExporterPtr exp, p->exporters())
    {
        exp->loadConfig( *p.data(), exp->isOnline() ? onlineElement : offlineElement );
    }
}

void TTManager::loadPreferences()
{
    m_Preferences.clear();

    QString path = preferenceDir();
    QStringList sl;
    sl.append("watchPrefs_*.xml");
    QDirIterator i(path, sl, QDir::Files);
    while ( i.hasNext() )
    {
        QString filename = i.next();


        loadPreferences(filename);
    }

    if ( !defaultPreferences() )
    {
        setupDefaultPreferences();
    }
}

QString TTManager::preferenceDir() const
{
    return QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation );
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

void TTManager::setupDefaultPreferences()
{
    WatchPreferencesPtr p = preferences("DEFAULT");
    p->setName("Default");

    IActivityExporterPtr ae = p->exporter("TCX");
    if ( ae )
    {
        ae->setEnabled(true);
    }
}
