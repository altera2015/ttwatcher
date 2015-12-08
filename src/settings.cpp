#include "settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

Settings * Settings::gSettings = 0;

Settings::Settings(QObject *parent) :
    QObject(parent),
    m_TileUrl("http://otile1.mqcdn.com/tiles/1.0.0/map/%1/%2/%3.png"),
    m_LastLatitude(26.929148599999998),
    m_LastLongitude(-80.174721549999987),
    m_LastZoom(13),
    m_AutoDownload(false),
    m_UseMetric(true)
{
    qDebug()<<Settings::settingsFilename() ;
}

Settings *Settings::get()
{
    if ( gSettings != 0 )
    {
        return gSettings;
    }
    else
    {
        gSettings = new Settings();
        gSettings->load();
        return gSettings;
    }

}

void Settings::free()
{
    if ( gSettings != 0 )
    {
        delete gSettings;
        gSettings = 0;
    }
}

QString Settings::tileUrl() const
{
    return m_TileUrl;
}

void Settings::setTileUrl(const QString &tileUrl)
{
    if ( m_TileUrl != tileUrl )
    {
        m_TileUrl = tileUrl;
        emit tileUrlChanged( m_TileUrl );
    }
}

double Settings::lastLatitude() const
{
    return m_LastLatitude;
}

void Settings::setLastLatitude(double lastLatitude)
{
    if ( m_LastLatitude != lastLatitude )
    {
        m_LastLatitude = lastLatitude;
        emit lastLatitudeChanged(lastLatitude);
    }
}

double Settings::lastLongitude() const
{
    return m_LastLongitude;
}

void Settings::setLastLongitude(double lastLongitude)
{
    if ( m_LastLongitude != lastLongitude )
    {
        m_LastLongitude = lastLongitude;
        emit lastLongitudeChanged(lastLongitude);
    }
}

int Settings::lastZoom() const
{
    return m_LastZoom;
}

void Settings::setLastZoom(int zoom)
{
    if ( m_LastZoom != zoom )
    {
        m_LastZoom = zoom;
        emit lastZoomChanged(zoom);
    }
}

bool Settings::autoDownload() const
{
    return m_AutoDownload;
}

void Settings::setAutoDownload(bool download)
{
    if ( m_AutoDownload != download )
    {
        m_AutoDownload = download;
        emit autoDownloadChanged(download);
    }
}

void Settings::setQuickFixDate(const QString &serial, QDateTime t)
{
    m_LastQuickFix[serial] = t;
    saveQuickFix();
}

QDateTime Settings::lastQuickFix(const QString &serial)
{
    if ( m_LastQuickFix.contains(serial))
    {
        return m_LastQuickFix[serial];
    }
    else
    {
        return QDateTime::fromTime_t(0);
    }
}

bool Settings::useMetric() const
{
    return m_UseMetric;
}

void Settings::setUseMetric(bool useMetric)
{
    if ( m_UseMetric != useMetric )
    {
        m_UseMetric = useMetric;
        emit useMetricChanged(useMetric);
    }
}

void Settings::save()
{
    QJsonObject o;
    o["titleUrl"] = tileUrl();
    o["lastLongitude"] = lastLongitude();
    o["lastLatitude"] = lastLatitude();
    o["lastZoom"] = lastZoom();
    o["autoDownload"] = autoDownload();
    o["useMetric"] = useMetric();

    QJsonDocument d;
    d.setObject(o);

    QFile f( Settings::settingsFilename() );
    if ( !f.open(QIODevice::WriteOnly|QIODevice::Truncate ))
    {
        qDebug() << "Settings::save / could not save settings.";
        return;
    }

    f.write(d.toJson());
    f.close();

}

void Settings::load()
{
    loadQuickFix();
    QFile f( Settings::settingsFilename() );
    if ( !f.open(QIODevice::ReadOnly))
    {
        return;
    }

    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson( f.readAll(), &pe );
    f.close();

    if ( pe.error != QJsonParseError::NoError )
    {
        qCritical() << "Settings::load / " << pe.errorString();
        return;
    }

    QJsonObject settings = d.object();

    if ( settings.contains("titleUrl"))
    {
        setTileUrl( settings["titleUrl"].toString() );
    }
    if ( settings.contains("lastLongitude"))
    {
        setLastLongitude( settings["lastLongitude"].toDouble());
    }
    if ( settings.contains("lastLatitude"))
    {
        setLastLatitude(settings["lastLatitude"].toDouble());
    }
    if ( settings.contains("lastZoom"))
    {
        setLastZoom(settings["lastZoom"].toInt());
    }
    if ( settings.contains("autoDownload"))
    {
        setAutoDownload( settings["autoDownload"].toBool());
    }
    if ( settings.contains("useMetric"))
    {
        setUseMetric( settings["useMetric"].toBool());
    }
}

QString Settings::ttdir()
{
    return QDir::homePath() + QDir::separator() + "TomTom MySports";
}

void Settings::saveQuickFix()
{

    QJsonObject o;

    QMap<QString, QDateTime>::iterator i=m_LastQuickFix.begin();
    for(;i!=m_LastQuickFix.end();i++)
    {
        o[ i.key() ] = i.value().toString();
    }

    QJsonDocument d;
    d.setObject(o);

    QFile f( Settings::quickFixFilename() );
    if ( !f.open(QIODevice::WriteOnly|QIODevice::Truncate ))
    {
        qDebug() << "Settings::saveQuickFix / could not save settings.";
        return;
    }

    f.write(d.toJson());
    f.close();
}

void Settings::loadQuickFix()
{
    m_LastQuickFix.clear();
    QFile f( Settings::quickFixFilename() );
    if ( !f.open(QIODevice::ReadOnly))
    {
        return;
    }

    QJsonParseError pe;
    QJsonDocument d = QJsonDocument::fromJson( f.readAll(), &pe );
    f.close();

    if ( pe.error != QJsonParseError::NoError )
    {
        qCritical() << "Settings::load / " << pe.errorString();
        return;
    }
    QJsonObject o = d.object();

    QJsonObject::iterator i = o.begin();
    for(;i!=o.end();i++)
    {
        m_LastQuickFix[ i.key() ] = QDateTime::fromString(i.value().toString());
    }
}

QString Settings::preferenceDir()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
    return QStandardPaths::writableLocation( QStandardPaths::ConfigLocation );
#else
    return QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation );
#endif
}

QString Settings::settingsFilename()
{
    return preferenceDir() + QDir::separator() + "settings.json";
}

QString Settings::quickFixFilename()
{
    return preferenceDir() + QDir::separator() + "quickfix.json";
}

