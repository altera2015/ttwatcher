#ifndef IACTIVITYEXPORTER_H
#define IACTIVITYEXPORTER_H

#include <QObject>

#include "activity.h"
#include <QDomElement>
#include <QSharedPointer>
#include <QList>
#include <QUrl>

class TTWatch;

class IActivityExporter : public QObject
{
    Q_OBJECT
public:
    explicit IActivityExporter(QObject *parent = 0);

    virtual QString name() const = 0;
    virtual bool loadConfig( TTWatch * watch, QDomElement element ) = 0;
    virtual bool isEnabled( ) const = 0;
    virtual void reset() = 0;
    virtual bool hasSetup() const = 0;
    virtual void setup( QWidget * parent ) = 0;
    virtual void saveConfig( TTWatch * watch, QDomDocument & document, QDomElement & element ) = 0;

signals:
    void exportFinished( bool success, QString message, QUrl url );
    void setupFinished( bool success );

public slots:
    virtual void exportActivity( ActivityPtr activity ) = 0;
};
typedef QSharedPointer<IActivityExporter> IActivityExporterPtr;
typedef QList<IActivityExporterPtr> IActivityExporterList;

#endif // IACTIVITYEXPORTER_H
