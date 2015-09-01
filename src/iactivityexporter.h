#ifndef IACTIVITYEXPORTER_H
#define IACTIVITYEXPORTER_H

#include <QObject>

#include "activity.h"
#include <QDomElement>
#include <QSharedPointer>
#include <QList>
#include <QUrl>
#include <QIcon>

#include "iexporterconfig.h"

class WatchExporters;

class IActivityExporter : public QObject
{
    Q_OBJECT
protected:
public:
    explicit IActivityExporter(QObject *parent = 0);

    // return name of exporter
    virtual QString name() const = 0;

    // return icon for exporter
    virtual QIcon icon() const = 0;

    // return true if setup actually has a dialog.
    virtual bool hasSetup() const = 0;

    // popup a dialog with setup for exporter.
    virtual void setup( QWidget * parent ) = 0;

    // return the current config object.
    virtual IExporterConfig & config() = 0;

    // create a config object for current activity type.
    virtual IExporterConfig * createConfig() = 0;

signals:
    // fires when this activity is done exporting it's data.
    void exportFinished( bool success, QString message, QUrl url );
    // fires when this activity is done setting up it's authentication data (if any)
    void setupFinished( IActivityExporter * exporter, bool success );
    // fires when the settings have changed and need re-saving
    void settingsChanged(IActivityExporter * exporter);

public slots:
    // call this when you need to export the activity.
    virtual void exportActivity( ActivityPtr activity ) = 0;

};
typedef QSharedPointer<IActivityExporter> IActivityExporterPtr;
typedef QList<IActivityExporterPtr> IActivityExporterList;

#endif // IACTIVITYEXPORTER_H
