#include "workouttreemodel.h"
#include <QDirIterator>
#include <QDebug>
#include <QFile>
#include <QDataStream>

#include "settings.h"
#include "ttbinreader.h"

TTWatchItem::TTWatchItem(const QString &name, QObject *parent) :
    TTItem(parent),
    m_Name(name)
{
}

QVariant TTWatchItem::data(int column, int role) const
{
    if ( column == 0 && ( role == Qt::DisplayRole || role == Qt::UserRole ))
    {
        return m_Name;
    }
    return QVariant();
}

bool TTWatchItem::match(const QString &name)
{
    return m_Name == name;
}


QString TTWorkoutItem::formatDistance() const
{
    Settings * settings = Settings::get();

    if ( settings->useMetric() )
    {
        if ( m_Distance > 1500 )
        {
            return QString("%1 K").arg(QString::number(m_Distance / 1000.0, 'f', 2 ));
        }
        else
        {
            return QString("%1 m").arg(QString::number( m_Distance ));
        }
    }
    else
    {
        // 1760 yards to a mile
        // 1609.34 meters to a mile.
        double miles = m_Distance / 1609.34;
        if ( miles < 1 )
        {
            return QString("%1 yd").arg( QString::number(miles * 1760.0, 'f', 0));
        }
        else
        {
            return QString("%1 M").arg( QString::number(miles, 'f', 2));
        }
    }
}

QString TTWorkoutItem::formatPace() const
{
    if ( m_Distance == 0 )
    {
        return "0";
    }

    double pace = ( m_Duration.hour() * 3600.0 + m_Duration.minute() * 60.0 + m_Duration.second() ) / 60.0 / m_Distance * 1000; // ( minutes per K )

    Settings * settings = Settings::get();

    if ( m_Sport == Activity::BIKING )
    {
        // returns speed instead of pace
        if ( settings->useMetric() )
        {
            return QString("%1 Km/hr").arg( QString::number(60.0/pace, 'f',2) );
        }
        else
        {
            return QString("%1 Mph").arg( QString::number( (60.0/pace)/1.60934, 'f',2) );
        }
    }
    else
    {
        if ( settings->useMetric() )
        {
            return QString("%1 min/Km").arg( QString::number(pace, 'f',2) );
        }
        else
        {
            return QString("%1 min/M").arg( QString::number(pace*1.60934, 'f',2) );
        }
    }
}

TTWorkoutItem::TTWorkoutItem(const QString &name, const QString &filename, TTWatchItem *parent) :
    TTItem(parent),
    m_Name(name),
    m_Filename(filename),
    m_Distance(0),
    m_Tag(false)
{
}

QVariant TTWorkoutItem::data(int column, int role) const
{
    if ( role == Qt::DisplayRole )
    {
        switch ( column )
        {
        case 0:
            return m_StartTime;
        case 1:
            if ( m_Duration.hour() > 0 )
            {
                return m_Duration.toString("h:mm:ss");
            }
            else
            {
                return m_Duration.toString("mm:ss");
            }
        case 2:
            return formatDistance();
        case 3:
            return formatPace();
        case 4:
            return Activity::sportToString( m_Sport );
        case 5:
            return m_Name;
        }
    }

    if ( role == Qt::UserRole) // used for sorting.
    {
        switch ( column )
        {
        case 0:
            return m_StartTime;
        case 1:
            return m_Duration;
        case 2:
            return m_Distance;
        case 3:
            return m_Distance == 0 ? 0 : ( m_Duration.hour() * 3600 + m_Duration.minute() * 60 + m_Duration.second() ) / 60.0 / m_Distance * 1000.0; // ( minutes per K )
        case 4:
            return (int)m_Sport;
        case 5:
            return m_Name;
        }
    }
    return QVariant();
}

bool TTWorkoutItem::match(const QString &filename)
{
    return m_Filename == filename;
}

void TTWorkoutItem::set(QDateTime startTime, QTime duration, int distance, Activity::Sport sport)
{
    m_StartTime = startTime;
    m_Duration = duration;
    m_Distance = distance;
    m_Sport = sport;
}

bool TTWorkoutItem::saveCache()
{
    QFile f( m_Filename + ".cache");
    if ( !f.open(QIODevice::WriteOnly))
    {
        return false;
    }
    QDataStream ds(&f);
    ds << quint8(1) << m_StartTime << m_Duration << m_Distance << (quint8)m_Sport;
    return true;
}

bool TTWorkoutItem::loadCache()
{
    quint8 version, sport;

    QFile f( m_Filename + ".cache");
    if ( !f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QDataStream ds(&f);

    ds >> version;

    switch ( version )
    {
    case 1:
        ds >> m_StartTime >> m_Duration >> m_Distance >> sport;
        break;
    }

    m_Sport = (Activity::Sport)sport;

    return true;
}

QString TTWorkoutItem::filename() const
{
    return m_Filename;
}

bool TTWorkoutItem::tag() const
{
    return m_Tag;
}

void TTWorkoutItem::setTag(bool tag)
{
    m_Tag = tag;
}


TTWatchItem *WorkoutTreeModel::getWatchItem(const QString &name, bool * newlyCreated )
{
    if ( newlyCreated )
    {
        *newlyCreated = false;
    }
    foreach ( TTWatchItem * item, m_WatchItems)
    {
        if ( item->match(name) )
        {
            return item;
        }
    }

    if ( newlyCreated )
    {
        *newlyCreated = true;
    }
    TTWatchItem * item = new TTWatchItem(name, 0);
    m_WatchItems.append(item);
    return item;
}

TTWorkoutItem * WorkoutTreeModel::getWorkoutItem(const QString &filename) const
{
    foreach ( TTWatchItem * watchItem, m_WatchItems )
    {
        QList<TTWorkoutItem*> items = watchItem->findChildren<TTWorkoutItem*>();
        foreach ( TTWorkoutItem * item, items )
        {
            if ( item->match( filename ) )
            {
                return item;
            }
        }
    }
    return 0;
}

void WorkoutTreeModel::clearTags()
{
    foreach( TTWatchItem * watchItem, m_WatchItems)
    {
        foreach (TTWorkoutItem * item, watchItem->findChildren<TTWorkoutItem*>())
        {
            item->setTag(false);
        }
    }
}

void WorkoutTreeModel::deleteUntagged()
{
    foreach( TTWatchItem * watchItem, m_WatchItems)
    {
        foreach (TTWorkoutItem * item, watchItem->findChildren<TTWorkoutItem*>())
        {
            if (!item->tag())
            {
                TTWatchItem * watchItem = (TTWatchItem*)item->parent();
                int row = watchItem->children().indexOf( item );
                beginRemoveRows(findWatchItem( watchItem), row, row );
                item->deleteLater();
                endRemoveRows();
            }
        }
    }
}


void WorkoutTreeModel::process(const QString &filename, bool full)
{
    if ( filename.length() < m_TTDir.length() )
    {
        qDebug() << "WorkoutTreeModel::process / filename not long enough " << filename;
        return;
    }

    QString n = filename.mid( m_TTDir.length());
    QStringList nparts = n.split("/", QString::SkipEmptyParts);
    if ( nparts.count() < 1 )
    {
        qDebug() << "WorkoutTreeModel::process / filename split didnt return enough parts " << filename;
        return;
    }

    QString watchName = nparts[0];
    bool newlyCreated;
    TTWatchItem * watchItem = getWatchItem(watchName, &newlyCreated);
    if ( watchItem == 0 )
    {
        qDebug() << "WorkoutTreeModel::process / getWatchItem return 0 " << filename;
        return;
    }

    if ( newlyCreated )
    {
        m_FileSystemWatcher.addPath( m_TTDir + QDir::separator() + watchName );
    }

    if ( !full )
    {
        TTWorkoutItem * wi = getWorkoutItem(filename);
        if ( wi )
        {
            // already existing, bail out!
            wi->setTag(true);
            return;
        }
    }

    TTWorkoutItem * item = new TTWorkoutItem( nparts.last(), filename, 0 );
    item->setTag(true);
    if ( !item->loadCache() )
    {
        TTBinReader br;
        ActivityPtr a = br.read(filename, true, true);
        if ( a )
        {

            QTime t(0,0,0);
            t = t.addSecs(a->duration());
            item->set(a->date(), t, a->distance(),a->sport());
            item->saveCache();
        }
        else
        {
            item->deleteLater();
            item = 0;
        }
    }

    if ( item == 0 )
    {
        return;
    }

    if ( !full )
    {
        if ( newlyCreated )
        {
            beginInsertRows(QModelIndex(),m_WatchItems.count()+1,m_WatchItems.count()+1);
        }
        else
        {
            QModelIndex parent = findWatchItem(watchItem);
            int count = watchItem->children().count();
            beginInsertRows(parent, count, count);
        }
    }


    item->setParent(watchItem);


    if ( !full )
    {
        endInsertRows();
    }
}

void WorkoutTreeModel::rescan(bool full)
{

    if ( full )
    {
        beginResetModel();
        clear();
    }
    else
    {
        clearTags();
    }

    QDirIterator it(m_TTDir, QStringList() << "*.ttbin", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        process(it.next(), full);
    }

    if ( full )
    {
        endResetModel();
    }
    else
    {
        deleteUntagged();
    }
}

WorkoutTreeModel::WorkoutTreeModel(const QString & ttDir, QObject *parent) :
    QAbstractItemModel(parent),
    m_TTDir(ttDir)
{
    connect(&m_FileSystemWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(fileSystemChanged()));
    rescan(true);
}

WorkoutTreeModel::~WorkoutTreeModel()
{
    clear();
}

void WorkoutTreeModel::clear()
{
    foreach ( TTWatchItem * item, m_WatchItems)
    {
        item->deleteLater();
    }
    m_WatchItems.clear();
}


QModelIndex WorkoutTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if ( parent.isValid() )
    {
        TTItem * item = (TTItem *)parent.internalPointer();
        QObjectList children = item->children();
        if ( row >= children.count( ))
        {
            return QModelIndex();
        }
        return createIndex(row, column, children.at(row));
    }
    else
    {
        if ( row >= m_WatchItems.count() )
        {
            return QModelIndex();
        }
        return createIndex(row, column, m_WatchItems.at(row));
    }
}

QModelIndex WorkoutTreeModel::parent(const QModelIndex &child) const
{
    if ( !child.isValid() )
    {
        return QModelIndex();
    }

    TTItem * childItem = (TTItem *)child.internalPointer();
    if ( !childItem )
    {
        return QModelIndex();
    }

    TTItem * parentItem = (TTItem*)childItem->parent();
    if ( !parentItem )
    {
        return QModelIndex();
    }

    if ( (QObject*)parentItem == this )
    {
        return QModelIndex();
    }

    int row = 0;

    if ( parentItem->parent() == this )
    {
        row = m_WatchItems.indexOf( (TTWatchItem*)parentItem );
    }
    else
    {
        row = parentItem->children().indexOf(parentItem);
    }

    return createIndex(row, 0, parentItem);

}

int WorkoutTreeModel::rowCount(const QModelIndex &parent) const
{
    if ( !parent.isValid() )
    {
        return m_WatchItems.count();
    }
    else
    {
        TTItem * item = (TTItem *)parent.internalPointer();
        if ( item )
        {
            return item->children().count();
        }
    }
    return 0;
}

int WorkoutTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 5; // leave out filename for now.
}

QVariant WorkoutTreeModel::data(const QModelIndex &index, int role) const
{
    if ( index.isValid() )
    {
        TTItem * item = (TTItem *)index.internalPointer();
        if ( item )
        {
            return item->data(index.column(), role);
        }
    }

    return QVariant();
}

QVariant WorkoutTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch ( section )
        {
        case 0:
            return "Date";
        case 1:
            return "Duration";
        case 2:
            return "Distance";
        case 3:
            return "Pace";
        case 4:
            return "Sport";
        case 5:
            return "Filename";
        }
    }
    return QVariant();
}

QModelIndex WorkoutTreeModel::findWatchItem(TTWatchItem *item) const
{
    int index = m_WatchItems.indexOf(item);
    return createIndex(index, 0, item);
}

QModelIndex WorkoutTreeModel::findWorkoutItem(const QString &filename) const
{
    TTWorkoutItem * item = getWorkoutItem(filename);
    if ( !item )
    {
        return QModelIndex();
    }

    TTWatchItem * watch = (TTWatchItem*)item->parent();
    int row = watch->children().indexOf( item );
    return createIndex(row, 0, item);
}

TTItem *WorkoutTreeModel::indexToItem(QModelIndex &index)
{
    if ( !index.isValid() )
    {
        return 0;
    }

    return (TTItem*)index.internalPointer();
}

TTWorkoutItem *WorkoutTreeModel::indexToWorkoutItem(QModelIndex &index)
{
    if ( !index.isValid() )
    {
        return 0;
    }

    TTWorkoutItem * item = (TTWorkoutItem *)index.internalPointer();
    if ( item->type() != TTItem::WorkoutItem )
    {
        return 0;
    }
    return item;
}

void WorkoutTreeModel::fileSystemChanged()
{
    rescan(false);
}






