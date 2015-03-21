#include "watchpreferences.h"
#include "stravaexporter.h"
#include "runkeeperexporter.h"
#include "tcxactivityexporter.h"
#include "ttbinreader.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QFile>

WatchPreferences::WatchPreferences(const QString &serial, QObject *parent) :
    QObject(parent),
    m_Serial(serial),
    m_ExportFinishedCounter(0)
{
    m_Exporters.append( StravaExporterPtr::create() );
    m_Exporters.append( TCXActivityExporterPtr::create() );
    m_Exporters.append( RunKeeperExporterPtr::create() );

    foreach ( IActivityExporterPtr exptr, m_Exporters)
    {        
        connect(exptr.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SIGNAL(exportFinished(bool,QString,QUrl)));
        connect(exptr.data(), SIGNAL(exportFinished(bool,QString,QUrl)), this, SLOT(onExportFinished(bool,QString,QUrl)));
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

QString WatchPreferences::serial() const
{
    return m_Serial;
}

bool WatchPreferences::parsePreferences(const QByteArray &data)
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
    QDomElement offlineExporters = exporters.firstChildElement("offline");

    foreach ( IActivityExporterPtr exp, m_Exporters)
    {
        if (!exp->loadConfig( *this, exp->isOnline() ? onlineExporters : offlineExporters ))
        {
            return false;
        }
    }

    return true;
}

QByteArray WatchPreferences::updatePreferences( const QByteArray &data)
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
    QDomElement offlineExporters = exporters.firstChildElement("offline");
    if ( offlineExporters.isNull() )
    {
        offlineExporters = dd.documentElement();
        offlineExporters.setTagName("offline");
        exporters.appendChild(offlineExporters);
    }



    foreach ( IActivityExporterPtr exp, m_Exporters)
    {
        exp->saveConfig(*this, dd, exp->isOnline() ? onlineExporters : offlineExporters );
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

QString WatchPreferences::encodeToken(const QByteArray &token) const
{
    QByteArray dest = scrambleToken( token );

    QString result;
    foreach ( char c, dest )
    {
        result.append(QChar(c));
    }

    return result;
}


QByteArray WatchPreferences::decodeToken(const QString &token) const
{
    QByteArray source;

    foreach ( QChar c, token )
    {
        source.append( c.toLatin1() );
    }

    return scrambleToken(source);
}



QByteArray WatchPreferences::scrambleToken(const QByteArray &sourceToken) const
{
    QByteArray key;
    key.append( m_Serial.toLatin1() );
    while ( key.length() < sourceToken.length() )
    {
        key.append( char( 0x56 ) );
        key.append( char( 0x33 ) );
        key.append( char( 0x49 ) );
        key.append( char( 0x37 ) );
        key.append( char( 0x4b ) );
        key.append( char( 0x30 ) );
        key.append( char( 0x49 ) );
        key.append( char( 0x39 ) );
        key.append( char( 0x4e ) );
        key.append( char( 0x34 ) );
        key.append( char( 0x47 ) );
        key.append( char( 0x30 ) );
    }

    QByteArray scrambledToken;

    for ( int i=0;i<sourceToken.length();i++)
    {
        quint8 sc, kc;
        sc = (quint8)sourceToken.at(i);
        kc = (quint8)key.at(i);
        scrambledToken.append((char) ( sc ^ kc ));
    }

    return scrambledToken;
}

bool WatchPreferences::exportActivity(ActivityPtr activity)
{
    foreach ( IActivityExporterPtr ae, m_Exporters )
    {
        if ( ae->isEnabled() )
        {
            m_ExportFinishedCounter++;
            ae->exportActivity(activity);
        }
    }

    return true;
}

bool WatchPreferences::exportFile(const QString &filename)
{
    QFile f(filename);
    if ( !f.open(QIODevice::ReadOnly))
    {
        qDebug() << "WatchPreferences::exportFile / could not open file " << filename;
        return false;
    }
    TTBinReader br;
    ActivityPtr a = br.read(f, true);
    if ( !a )
    {
        qDebug() << "WatchPreferences::exportFile / could not parse file " << filename;
        return false;
    }

    return exportActivity(a);
}

bool WatchPreferences::isExporting()
{
    return m_ExportFinishedCounter > 0;
}

void WatchPreferences::onExportFinished(bool success,QString message,QUrl url)
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
