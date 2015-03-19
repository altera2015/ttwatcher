#ifndef IACTIVITYEXPORTER_H
#define IACTIVITYEXPORTER_H

#include <QObject>

#include "activity.h"
#include <QDomElement>
#include <QSharedPointer>
#include <QList>
#include <QUrl>
#include <QIcon>

class WatchPreferences;

class IActivityExporter : public QObject
{
    Q_OBJECT
protected:
    void parseExportTag( QDomElement element, const QString & idAttribute, bool & enabled, bool & autoOpen );
    void writeExportTag( QDomDocument & document, QDomElement &element, const QString & idAttribute, bool enabled, bool autoOpen);
public:
    explicit IActivityExporter(QObject *parent = 0);

    virtual QString name() const = 0;
    virtual bool loadConfig( const WatchPreferences & preferences, QDomElement element ) = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual bool isOnline() const = 0;
    virtual bool autoOpen() const = 0;
    virtual void setAutoOpen( bool autoOpen ) = 0;
    virtual QIcon icon() const = 0;
    virtual void reset() = 0;
    virtual bool hasSetup() const = 0;
    virtual void setup( QWidget * parent ) = 0;
    virtual void saveConfig( const WatchPreferences & preferences, QDomDocument & document, QDomElement & element ) = 0;


signals:
    void exportFinished( bool success, QString message, QUrl url );
    void setupFinished( bool success );

public slots:
    virtual void exportActivity( ActivityPtr activity ) = 0;
};
typedef QSharedPointer<IActivityExporter> IActivityExporterPtr;
typedef QList<IActivityExporterPtr> IActivityExporterList;

#endif // IACTIVITYEXPORTER_H
