#include "watchexporters.h"
#include "stravaexporter.h"
#include "runkeeperexporter.h"
#include "tcxactivityexporter.h"
#include "ttbinreader.h"
#include "smashrunexporter.h"
#include "elevationloader.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QFile>

WatchExporters::WatchExporters(const QString &serial, QObject *parent) :
    QObject(parent),
    m_Serial(serial),
    m_ExportFinishedCounter(0)
{
    m_Exporters.append( StravaExporterPtr::create(serial) );
    m_Exporters.append( TCXActivityExporterPtr::create(serial) );
    m_Exporters.append( RunKeeperExporterPtr::create(serial) );
    m_Exporters.append( SmashrunExporterPtr::create(serial) );

    foreach ( IActivityExporterPtr exptr, m_Exporters)
    {        
        connect(exptr.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SIGNAL(exportFinished(bool,QString,QUrl)));
        connect(exptr.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SLOT(onExportFinished(bool,QString,QUrl)));
        connect(exptr.data(), SIGNAL(settingsChanged(IActivityExporter*)), this, SLOT(onSettingsChanged()));
    }
}

QString WatchExporters::name() const
{
    return m_Name;
}

void WatchExporters::setName(const QString &name)
{
    m_Name = name;
}

QString WatchExporters::serial() const
{
    return m_Serial;
}

IActivityExporterList WatchExporters::exporters()
{
    return m_Exporters;
}

IActivityExporterPtr WatchExporters::exporter(const QString &service)
{
    foreach (IActivityExporterPtr exp, m_Exporters)
    {
        if ( exp->name() == service )
        {
            return exp;
        }
    }

    return IActivityExporterPtr();
}

const IExporterConfigMap WatchExporters::configMap()
{
    IExporterConfigMap map;

    foreach (IActivityExporterPtr exp, m_Exporters)
    {
        map[exp->name()] = &exp->config();
    }
    return map;
}

IExporterConfigMap WatchExporters::configImportMap()
{
    IExporterConfigMap map;

    foreach (IActivityExporterPtr exp, m_Exporters)
    {
        map[exp->name()] = exp->createConfig();
    }
    return map;
}

void WatchExporters::freeImportMap(IExporterConfigMap &map)
{
    foreach(IExporterConfig * config, map)
    {
        delete config;
    }
}



bool WatchExporters::exportActivity(ActivityPtr activity, const QString & exporterName, QStringList * sl)
{
    if ( m_ExportFinishedCounter != 0 )
    {
        return false;
    }

    foreach ( IActivityExporterPtr ae, m_Exporters )
    {
        if ( ( exporterName == "" || ae->name() == exporterName) )
        {
            if ( ae->config().isValid() )
            {
                m_ExportFinishedCounter++;
                ae->exportActivity(activity);
                if ( sl != 0 )
                {
                    sl->append( ae->name() );
                }
            }
            else
            {
                continue;
            }
        }
    }

    return true;
}

bool WatchExporters::exportFile(const QString &filename)
{
    TTBinReader br;
    ActivityPtr a = br.read(filename, true);
    if ( !a )
    {
        qDebug() << "WatchExporters::exportFile / could not parse file " << filename;
        return false;
    }

    ElevationLoader el;
    if ( el.load(a, true) != ElevationLoader::SUCCESS )
    {
        return false;
    }

    return exportActivity(a);
}

bool WatchExporters::isExporting()
{
    return m_ExportFinishedCounter > 0;
}

void WatchExporters::onSettingsChanged()
{
    emit settingsChanged(m_Serial);
}

void WatchExporters::onExportFinished(bool success,QString message,QUrl url)
{
    if ( !success )
    {
        emit exportError(message);
    }

    m_ExportFinishedCounter--;
    if ( m_ExportFinishedCounter < 0 )
    {
        m_ExportFinishedCounter = 0;
    }

    if ( m_ExportFinishedCounter == 0)
    {
        emit allExportsFinished();
    }
}
