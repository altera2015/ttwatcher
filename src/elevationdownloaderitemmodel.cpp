#include "elevationdownloaderitemmodel.h"

ElevationDownloaderItemModel::ElevationDownloaderItemModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

int ElevationDownloaderItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

int ElevationDownloaderItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_List.count();
}

QModelIndex ElevationDownloaderItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if ( parent.isValid() )
    {
        return QModelIndex();
    }
    return createIndex(row,column);
}

QModelIndex ElevationDownloaderItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

bool ElevationDownloaderItemModel::hasChildren(const QModelIndex &parent) const
{
    if ( parent.isValid() )
        return false;
    else
        return true;
}

QVariant ElevationDownloaderItemModel::data(const QModelIndex &index, int role) const
{
    return m_List.at(index.row())->data(index.column(), role);
}

QVariant ElevationDownloaderItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }
    switch ( section )
    {
    case 0:
        break;
    case 1:
        return "Name";
    case 2:
        return "Status";
    }
    return QVariant();
}

const ElevationDownloaderItemList &ElevationDownloaderItemModel::list() const
{
    return m_List;
}

void ElevationDownloaderItemModel::addSource(ElevationSource &source)
{
    beginResetModel();

    int idx = m_List.count();
    ElevationTileDownloaderItem * item = new ElevationTileDownloaderItem(idx,this);
    item->name = source.source;
    item->destDir = source.baseDir;
    item->urls.append( source.url.toString() );
    connect(item, SIGNAL(cellChanged(int)), this, SLOT(cellChanged(int)));
    m_List.append(item);

    endResetModel();
}

void ElevationDownloaderItemModel::cellChanged(int id)
{    
    for (int i=0;i<m_List.count();i++)
    {
        if ( m_List.at(i)->id()==id)
        {
            QModelIndex startIndex = index(i, 0, QModelIndex());
            QModelIndex stopIndex = index(i, 2, QModelIndex());
            QVector<int> roles;
            roles <<Qt::DisplayRole << Qt::DecorationRole;
            emit dataChanged(startIndex, stopIndex, roles);
        }
    }
}
