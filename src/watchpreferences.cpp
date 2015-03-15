#include "watchpreferences.h"
#include "stravaexporter.h"
#include <QDomDocument>
#include <QDomElement>
#include <QDebug>

WatchPreferences::WatchPreferences(QObject *parent) :
    QObject(parent)
{
    m_Exporters.append( StravaExporterPtr::create() );

    foreach ( IActivityExporterPtr exptr, m_Exporters)
    {
        connect(exptr.data(), SIGNAL(setupFinished(bool)), this, SIGNAL(setupFinished(bool)));
        connect(exptr.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SIGNAL(exportFinished(bool,QString,QUrl)));
    }
}

QString WatchPreferences::name() const
{
    return m_Name;
}

void WatchPreferences::setName(const QString &name)
{
    m_Name = name;
}

bool WatchPreferences::parsePreferences(TTWatch * watch, const QByteArray &data)
{
    foreach ( IActivityExporterPtr exp, m_Exporters)
    {
        exp->reset();
    }
    setName("");

    QDomDocument dd;

    if ( !dd.setContent(data) )
    {
        qWarning() << "WatchPreferences::parsePreferences / could not read preferences.";
        return false;
    }

    QDomElement preferences = dd.firstChildElement("preferences");
    if ( preferences.isNull() )
    {
        qWarning() << "WatchPreferences::parsePreferences / no preferences element.";
        return false;
    }

    QDomElement watchNameElement = preferences.firstChildElement("watchName");
    if ( watchNameElement.isNull() )
    {
        return false;
    }

    setName( watchNameElement.text() );

    QDomElement exporters = preferences.firstChildElement("exporters");
    if ( exporters.isNull() )
    {
        return false;
    }

    QDomElement onlineExporters = exporters.firstChildElement("online");
    if ( !onlineExporters.isNull() )
    {

        foreach ( IActivityExporterPtr exp, m_Exporters)
        {
            if (!exp->loadConfig(watch, onlineExporters))
            {
                return false;
            }
        }

    }

    return true;
}

QByteArray WatchPreferences::updatePreferences(TTWatch *watch, const QByteArray &data)
{
    QDomDocument dd;

    if ( !dd.setContent(data) )
    {
        qWarning() << "WatchPreferences::updatePreferences / could not read preferences.";
        return data;
    }

    QDomElement preferences = dd.firstChildElement("preferences");
    if ( preferences.isNull() )
    {
        qWarning() << "WatchPreferences::updatePreferences / no preferences element.";
        return data;
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


    foreach ( IActivityExporterPtr exp, m_Exporters)
    {
        exp->saveConfig(watch, dd, onlineExporters);
    }


    return dd.toByteArray(3);
}

IActivityExporterList WatchPreferences::exporters()
{
    return m_Exporters;
}

IActivityExporterPtr WatchPreferences::exporter(const QString &service)
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
