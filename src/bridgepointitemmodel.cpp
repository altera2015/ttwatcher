#include "bridgepointitemmodel.h"
#include <QColor>

BridgePointItemModel::BridgePointItemModel(QObject *parent) :
    QAbstractItemModel(parent),
    m_Bridge(nullptr),
    m_SelectedPoint(-1)
{
}

void BridgePointItemModel::setBridge(Bridge *bridge)
{
    beginResetModel();
    m_Bridge = bridge;
    endResetModel();
}

Bridge *BridgePointItemModel::bridge() const
{
    return m_Bridge;
}

int BridgePointItemModel::selectedPoint() const
{
    return m_SelectedPoint;
}

int BridgePointItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

int BridgePointItemModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if ( m_Bridge )
    {
        return m_Bridge->points().count();
    }
    else
    {
        return 0;
    }
}

QModelIndex BridgePointItemModel::index(int row, int column, const QModelIndex &parent) const
{
    if ( parent.isValid() )
    {
        return QModelIndex();
    }
    return createIndex(row,column);
}

QModelIndex BridgePointItemModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

bool BridgePointItemModel::hasChildren(const QModelIndex &parent) const
{
    if ( parent.isValid() )
        return false;
    else
        return true;
}

QVariant BridgePointItemModel::data(const QModelIndex &index, int role) const
{
    if ( m_Bridge == nullptr || !index.isValid() || index.row() >= m_Bridge->points().count() )
    {
        return QVariant();
    }

    if ( role == Qt::DisplayRole || role == Qt::EditRole )
    {
        switch ( index.column())
        {
        case 0:
            return m_Bridge->points().at(index.row()).coordinate().x();
        case 1:
            return m_Bridge->points().at(index.row()).coordinate().y();
        case 2:
            return m_Bridge->points().at(index.row()).elevation();
        }
    }

    if ( role == Qt::BackgroundColorRole )
    {
        if ( index.row() == m_SelectedPoint )
        {
            return QVariant( QColor( 255, 0, 0,100) );
        }
    }

    return QVariant();
}

bool BridgePointItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_Bridge->points().count())
    {
        return false;
    }

    BridgePoint & p = m_Bridge->points()[index.row()];
    switch ( index.column() )
    {
    case 0:
        p.setLongitude(value.toDouble() );
        break;
    case 1:
        p.setLatitude(value.toDouble() );
        break;
    case 2:
        p.setElevation( value.toDouble() );
        break;
    }

    return true;
}

QVariant BridgePointItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch ( section )
    {
    case 0:
        return "Longitude";
    case 1:
        return "Latitude";
    case 2:
        return "Elevation";
    }
    return QVariant();
}

Qt::ItemFlags BridgePointItemModel::flags(const QModelIndex &index) const
{
    if ( !index.isValid() )
    {
        return 0;
    }

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if ( index.column() == 2 )
    {
        f |= Qt::ItemIsEditable;
    }

    return f;

}

Qt::DropActions BridgePointItemModel::supportedDropActions() const
{
    return Qt::DropAction();
}

void BridgePointItemModel::selectPoint(int selectIndex)
{
    if ( m_Bridge == nullptr || m_SelectedPoint >= m_Bridge->points().count() )
    {
        return;
    }

    m_SelectedPoint = selectIndex;
    emit dataChanged( index(selectIndex,0, QModelIndex()), index(selectIndex, columnCount(QModelIndex()), QModelIndex()));
}

void BridgePointItemModel::bridgePointUpdated(int pointIndex)
{
    emit dataChanged( index(pointIndex,0, QModelIndex()), index(pointIndex, columnCount(QModelIndex()), QModelIndex()));
}

void BridgePointItemModel::removePoint(int index)
{
    if ( m_Bridge == nullptr || index < 0 || index >= m_Bridge->points().count() )
    {
        return;
    }

    beginRemoveRows( QModelIndex(), index, index);
    m_Bridge->points().removeAt(index);
    endRemoveRows();
}

void BridgePointItemModel::addPoint(const QPointF &pos, double elevation)
{
    if ( m_Bridge == nullptr )
    {
        return;
    }

    beginInsertRows(QModelIndex(), rowCount(QModelIndex()), rowCount(QModelIndex()));
    m_Bridge->points().append(BridgePoint(pos, elevation));
    endInsertRows();
}

void BridgePointItemModel::replacePoints(BridgePointList &bpl)
{
    beginResetModel();
    m_Bridge->points() = bpl;
    endResetModel();
}
