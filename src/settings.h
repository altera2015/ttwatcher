#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QMap>
#include <QDateTime>

class Settings : public QObject
{
    Q_OBJECT
    explicit Settings(QObject *parent = 0);
    static Settings * gSettings;
public:

    static Settings * get();
    static void free();

    QString tileUrl() const;
    void setTileUrl( const QString & tileUrl );

    double lastLatitude() const;
    void setLastLatitude(double lastLatitude);
    double lastLongitude() const;
    void setLastLongitude(double lastLongitude);
    int lastZoom() const;
    void setLastZoom(int zoom);

    bool autoDownload() const;
    void setAutoDownload( bool download );

    void setQuickFixDate( const QString & serial, QDateTime t );
    QDateTime lastQuickFix( const QString & serial );

    bool useMetric() const;
    void setUseMetric( bool useMetric );


    void save();
    void load();


    static QString ttdir();
signals:
    void tileUrlChanged( QString url );
    void lastLatitudeChanged( double latitude );
    void lastLongitudeChanged( double longitude );
    void lastZoomChanged( int zoom );
    void autoDownloadChanged( bool autoDownload );
    void useMetricChanged(bool useMetric);

private:
    QString m_TileUrl;
    double m_LastLatitude;
    double m_LastLongitude;
    int m_LastZoom;
    bool m_AutoDownload;
    bool m_UseMetric;
    QMap<QString, QDateTime> m_LastQuickFix;
    void saveQuickFix();
    void loadQuickFix();
    static QString preferenceDir();
    static QString settingsFilename();
    static QString quickFixFilename();
};

#endif // SETTINGS_H
