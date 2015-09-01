#ifndef IEXPORTERCONFIG_H
#define IEXPORTERCONFIG_H

#include <QDomDocument>
#include <QDomElement>
#include <QMap>

class WatchExporters;

class IExporterConfig
{    
protected:
    void parseExportTag(QDomElement & element, const QString & idAttribute );
    void writeExportTag( QDomDocument & document, QDomElement &element, const QString & idAttribute);

    void writeTag(QDomDocument & document, QDomElement &parentElement, const QString &tag, const QString & value);
    QString readTag(QDomElement &parentElement, const QString & tag, const QString & def = QString(), bool * usedDef = 0);

    void writeEncodedTag(QDomDocument & document, QDomElement &parentElement, const QString &tag, const QByteArray & value);
    QByteArray readEncodedTag(QDomElement & parentElement, const QString & tag, const QByteArray & def = QByteArray());

    QByteArray scrambleToken(const QByteArray &sourceToken) const;
    QByteArray decodeToken(const QString &token) const;
    QString encodeToken(const QByteArray &token) const;
public:
    IExporterConfig(const QString & serial);
    virtual ~IExporterConfig();

    // return true if other is the same type AND it's config is exactly the same.
    virtual bool equals( const IExporterConfig * other );

    // copy the settings object from other to this.
    virtual bool apply( const IExporterConfig * other );

    // load the config from element.
    virtual bool loadConfig( QDomElement & element ) = 0;

    // update the config into document as child elements of element.
    virtual void updateConfig( QDomDocument & document, QDomElement & element ) = 0;

    // return true if this setting may be stored on the watch.
    virtual bool allowSaveOnWatch() const = 0;
    // return true if this should save in the "online" section of the XML.
    virtual bool isOnline() const = 0;

    // resets to defaults.
    virtual void reset();

    // true if any of the config params have changed since last save.
    virtual bool changed();
    virtual void setChanged( bool changed );

    // true if the exporter is enabled.
    virtual bool isValid() const;
    virtual void setValid(bool valid);

    virtual bool isAutoOpen() const;
    virtual void setAutoOpen( bool autoOpen );

    virtual QString serial() const;

private:
    bool m_Changed;
    bool m_IsValid;
    bool m_IsAutoOpen;
    QString m_Serial;
};

typedef QMap<QString, IExporterConfig *> IExporterConfigMap;


#endif // IEXPORTERCONFIG_H
