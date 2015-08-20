#include "ttmanager.h"
#include <QTimer>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QStandardPaths>
#include <QBuffer>
#include <QRegExp>
#include "hidapi.h"
#include "watchexporters.h"

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
                QString serial = QString::fromWCharArray( devices->serial_number );
                QString path = devices->path;

                TTWatch * w = find(path);
                if ( w )
                {                    
                    currentPaths.removeOne(path);
                }
                else
                {
                    TTWatch * newWatch = new TTWatch( path, serial, this );
                    m_TTWatchList.append( newWatch );
                    prepareWatch( newWatch );
                    emit ttArrived(serial);
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
            emit ttRemoved(w->serial());
        }
    }

    foreach ( TTWatch * w, wl )
    {        
        m_TTWatchList.removeOne(w);
        w->deleteLater();
    }

}

void TTManager::prepareWatch(TTWatch *watch)
{
    // Settings rules.
    // Store settings on PC (this can include settings that do not appear on the actual watch.
    // Load these settings
    // Check that the settings from the watch have not changed
    // If changed ask user if they want to apply changes locally.

    if ( m_WatchExporters.contains( watch->serial() ))
    {
        WatchExportersPtr exp = exporters(watch->serial());
        QByteArray data;
        bool mustSave = false;

        if ( watch->downloadPreferences(data))
        {
            IExporterConfigMap configImportMap = exp->configImportMap();
            QString name;
            if (!loadConfig(data, configImportMap, name))
            {
                return;
            }
            if ( exp->name() != name )
            {
                exp->setName(name);
                mustSave = true;
            }

            // now compare with what we have.
            IExporterConfigMap configMap = exp->configMap();
            for(IExporterConfigMap::iterator i = configMap.begin(); i!=configMap.end();i++)
            {
                QString name = i.key();
                IExporterConfig * config = i.value();
                IExporterConfig * configImport = configImportMap[name];

                if ( config->allowSaveOnWatch() && !config->equals( configImport ))
                {
                    config->apply( configImport );
                    mustSave = true;
                }
            }


            if ( mustSave )
            {
                saveConfig(exp);
            }

            exp->freeImportMap(configImportMap);
        }
    }
    else
    {
        WatchExportersPtr exp = exporters(watch->serial()); // this will create the exporter if it didn't exist.
        QByteArray data;
        if ( watch->downloadPreferences(data))
        {
            QString name;
            if ( loadConfig(data, exp->configMap(),name))
            {
                exp->setName(name);
                saveConfig(exp);
            }
        }
    }
}

TTManager::TTManager(QObject *parent) :
    QObject(parent)
{
    loadAllConfig();

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

WatchExportersMap & TTManager::exporters()
{
    return m_WatchExporters;
}

WatchExportersPtr TTManager::exporters(const QString &serial)
{
    if ( m_WatchExporters.contains(serial))
    {
        return m_WatchExporters[serial];
    }

    WatchExportersPtr wp = WatchExportersPtr::create(serial);
    connect(wp.data(), SIGNAL(allExportsFinished()), this, SIGNAL(allExportingFinished()));
    connect(wp.data(), SIGNAL(exportError(QString)), this, SIGNAL(exportError(QString)));
    connect(wp.data(), SIGNAL(settingsChanged(QString)), this, SLOT(configChanged(QString)));
    m_WatchExporters[ serial ] = wp;

    if ( serial == "DEFAULT")
    {
        wp->setName(tr("Default"));
        IActivityExporterPtr ae = wp->exporter("TCX");
        if ( ae )
        {
            ae->config().setValid(true);
        }
    }
    return wp;
}

WatchExportersPtr TTManager::exportersForName(const QString &name)
{
    WatchExportersMap::iterator i = m_WatchExporters.begin();

    for(;i!=m_WatchExporters.end();i++)
    {
        if ( i.value()->name() == name )
        {
            return i.value();
        }
    }

    return WatchExportersPtr();
}

WatchExportersPtr TTManager::defaultExporters()
{
    return exporters("DEFAULT");
}

void TTManager::saveConfig(QIODevice *dest, WatchExportersPtr watchExporters)
{
    QDomDocument d;
    QDomElement preferencesElement = d.createElement("preferences");
    d.appendChild(preferencesElement);

    QDomElement nameElement = d.createElement("watchName");
    nameElement.appendChild(  d.createTextNode( watchExporters->name() ) );
    preferencesElement.appendChild(nameElement);

    QDomElement exportersElement = d.createElement("exporters");
    preferencesElement.appendChild(exportersElement);

    QDomElement onlineElement = d.createElement("online");
    exportersElement.appendChild(onlineElement);

    QDomElement offlineElement = d.createElement("offline");
    exportersElement.appendChild(offlineElement);

    IExporterConfigMap configMap = watchExporters->configMap();
    foreach ( IExporterConfig * config, configMap )
    {         
        config->updateConfig( d, config->isOnline() ? onlineElement : offlineElement );
    }

    dest->write( d.toByteArray( ));
}

void TTManager::saveAllConfig(bool saveToWatch, const QString serialOnly)
{
    WatchExportersMap::iterator i = m_WatchExporters.begin();

    for(;i!=m_WatchExporters.end();i++)
    {

        WatchExportersPtr watchExporters = i.value();

        if ( serialOnly.length() > 0 && serialOnly != watchExporters->serial() )
        {
            continue;
        }

        if ( watchExporters->name().length() == 0 )
        {
            continue;
        }

        bool changed = false;

        IExporterConfigMap configMap = watchExporters->configMap();

        foreach ( IExporterConfig * config, configMap)
        {
            changed |= config->changed();
        }

        if ( !changed )
        {
            continue;
        }

        saveConfig(watchExporters);

        if ( saveToWatch )
        {
            TTWatch * w = watch( watchExporters->serial() );
            if ( w )
            {
                QByteArray prefData;
                if ( !w->downloadPreferences(prefData))
                {
                    qCritical() << "TTManager::saveAllConfig / could not load preferences.";
                    continue;
                }

                prefData = mergeConfig(prefData, configMap);

                if ( !w->uploadPreferences(prefData) )
                {
                    qCritical() << "TTManager::saveAllConfig / could not save preferences to watch.";
                }
            }
        }
    }
}

bool TTManager::loadConfig(QIODevice *source, const IExporterConfigMap & configMap, QString &name)
{
    QByteArray xml = source->readAll();

    QDomDocument d;
    d.setContent(xml);

    QDomElement preferencesElement = d.firstChildElement("preferences");
    if ( preferencesElement.isNull() )
    {
        qDebug() << "TTManager::loadConfig / not a preferences XML file. " << source;
        return false;
    }

    QDomElement nameElement = preferencesElement.firstChildElement("watchName");

    if ( nameElement.isNull())
    {
        qDebug() << "TTManager::loadConfig / missing element name or serial. " << source;
        return false;
    }

    name = nameElement.text();

    QDomElement exportersElement = preferencesElement.firstChildElement("exporters");

    QDomElement onlineElement = exportersElement.firstChildElement("online");
    QDomElement offlineElement = exportersElement.firstChildElement("offline");

    foreach ( IExporterConfig * config, configMap)
    {
        config->loadConfig( config->isOnline() ? onlineElement : offlineElement );
    }

    return true;
}


QByteArray TTManager::mergeConfig(const QByteArray &source, const IExporterConfigMap &configMap)
{
    QDomDocument dd;

    if ( !dd.setContent(source) )
    {
        qWarning() << "TTManager::mergeConfig / could not read preferences.";
        return source;
    }

    QDomElement preferences = dd.firstChildElement("preferences");
    if ( preferences.isNull() )
    {
        qWarning() << "TTManager::mergeConfig / no preferences element.";
        return source;
    }


    QDomElement exporters = preferences.firstChildElement("exporters");
    if ( exporters.isNull() )
    {
        exporters = dd.documentElement();
        exporters.setTagName("exporters");
        preferences.appendChild(exporters);
    }

    QDomElement onlineExporters = exporters.firstChildElement("online");
    if ( onlineExporters.isNull() )
    {
        onlineExporters = dd.documentElement();
        onlineExporters.setTagName("online");
        exporters.appendChild(onlineExporters);
    }
    QDomElement offlineExporters = exporters.firstChildElement("offline");
    if ( offlineExporters.isNull() )
    {
        offlineExporters = dd.documentElement();
        offlineExporters.setTagName("offline");
        exporters.appendChild(offlineExporters);
    }



    foreach ( IExporterConfig * config, configMap)
    {
        if ( config->allowSaveOnWatch() )
        {
            config->updateConfig(dd, config->isOnline() ? onlineExporters : offlineExporters );
        }
    }

    return dd.toByteArray(3);
}


void TTManager::saveConfig(const QString &filename, WatchExportersPtr watchExporters)
{
    QFile f( filename );
    if ( !f.open(QIODevice::WriteOnly))
    {
        qCritical() << "TTManager::saveConfig / could not write preferences file " << filename;
        return;
    }
    saveConfig(&f, watchExporters);
}

bool TTManager::loadConfig(const QString &filename, const IExporterConfigMap &configMap, QString &name)
{
    QFile f( filename );
    if ( !f.open(QIODevice::ReadOnly))
    {
        qDebug() << "TTManager::loadConfig / could not load preferences.";
        return "";
    }
    return loadConfig(&f, configMap, name);
}

void TTManager::saveConfig(QByteArray &dest, WatchExportersPtr watchExporters)
{
    QBuffer b(&dest);
    b.open(QIODevice::WriteOnly);
    saveConfig(&b, watchExporters);
    b.close();
}

bool TTManager::loadConfig(const QByteArray &source, const IExporterConfigMap &configMap, QString &name)
{
    QBuffer b;
    b.setData(source);
    b.open(QIODevice::ReadOnly);
    bool result = loadConfig(&b, configMap, name);
    b.close();
    return result;
}

void TTManager::saveConfig(WatchExportersPtr watchExporters)
{
    QString filename = configDir() + QDir::separator() + "watchPrefs_" + watchExporters->serial() + ".xml";
    saveConfig(filename, watchExporters);
}

void TTManager::loadAllConfig()
{
    QRegExp reg("watchPrefs_(.*)\\.xml");
    QString path = configDir();
    QStringList sl;
    sl.append("watchPrefs_*.xml");
    QDirIterator i(path, sl, QDir::Files);
    while ( i.hasNext() )
    {
        QString filename = i.next();

        QString serial = "unknown";

        if ( reg.indexIn(filename) >= 0 )
        {
            serial = reg.cap(1);
        }


        WatchExportersPtr exp = exporters(serial);
        QString name;
        if ( loadConfig(filename, exp->configMap(), name) )
        {
            exp->setName(name);
        }
    }

    defaultExporters();
}

QString TTManager::configDir() const
{
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0) 
    QString path = QStandardPaths::writableLocation( QStandardPaths::ConfigLocation );
#else
    QString path = QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation );
#endif    
    QDir mkpathd;
    mkpathd.mkpath(path);
    return path;
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

void TTManager::configChanged(QString serial)
{
    saveAllConfig(true, serial);
}

