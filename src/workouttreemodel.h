#ifndef WORKOUTTREEMODEL_H
#define WORKOUTTREEMODEL_H

#include <QFileSystemWatcher>
#include <QAbstractItemModel>
#include <QObject>
#include <QList>
#include <QDateTime>
#include <QTime>
#include "activity.h"

class TTItem : public QObject {
    Q_OBJECT

public:
    TTItem(QObject * parent = 0) : QObject(parent) {}
    virtual ~TTItem() {}

    virtual QVariant data(int column, int role) const = 0;
    enum Type { WatchItem, WorkoutItem };
    virtual Type type() const = 0;
};

class TTWatchItem : public TTItem {

    Q_OBJECT
    QString m_Name;
public:
    TTWatchItem( const QString & name, QObject * parent = 0 );
    QVariant data(int column, int role) const;
    Type type() const { return WatchItem; }
    bool match ( const QString & name );
};

class TTWorkoutItem : public TTItem {
    Q_OBJECT
    QString m_Name;
    QString m_Filename;
    QDateTime m_StartTime;
    QTime m_Duration;
    int m_Distance;
    Activity::Sport m_Sport;
    bool m_Tag;
    QString formatDistance() const;
    QString formatPace() const;
public:
    TTWorkoutItem( const QString & name, const QString & filename, TTWatchItem * parent );
    QVariant data(int column, int role) const;
    Type type() const { return WorkoutItem; }
    bool match ( const QString & filename );
    void set( QDateTime startTime, QTime duration, int distance, Activity::Sport sport );
    bool saveCache();
    bool loadCache();
    QString filename() const;
    bool tag() const;
    void setTag( bool tag);

};


class WorkoutTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    QString m_TTDir;
    typedef QList<TTWatchItem*> WatchItems;
    WatchItems m_WatchItems;
    QFileSystemWatcher m_FileSystemWatcher;

    TTWatchItem * getWatchItem(const QString & name , bool *newlyCreated = 0);
    TTWorkoutItem *getWorkoutItem( const QString & filename ) const;

    void clearTags();
    void deleteUntagged();
    void process(const QString & filename , bool full);

public:
    explicit WorkoutTreeModel(const QString & ttDir, QObject *parent = 0);
    virtual ~WorkoutTreeModel();
    void rescan(bool full);
    void clear();
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex findWatchItem ( TTWatchItem * item ) const;
    QModelIndex findWorkoutItem( const QString & filename ) const;
    static TTItem * indexToItem( QModelIndex & index );
    static TTWorkoutItem * indexToWorkoutItem( QModelIndex & index );


signals:

public slots:
private slots:
    void fileSystemChanged();

};

#endif // WORKOUTTREEMODEL_H
