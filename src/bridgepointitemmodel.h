#ifndef BRIDGEPOINTITEMMODEL_H
#define BRIDGEPOINTITEMMODEL_H

#include <QAbstractItemModel>
#include "bridge.h"

class BridgePointItemModel : public QAbstractItemModel
{
    Q_OBJECT
    Bridge * m_Bridge;
    int m_SelectedPoint;
public:
    explicit BridgePointItemModel(QObject *parent = 0);
    void setBridge( Bridge * bridge );
    Bridge * bridge() const;
    int selectedPoint() const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Qt::DropActions supportedDropActions() const;

signals:

public slots:
    void selectPoint( int index );
    void bridgePointUpdated(int index);

    void removePoint(int index);
    void addPoint( const QPointF & pos, double elevation );
    void replacePoints( BridgePointList & bpl );

};

#endif // BRIDGEPOINTITEMMODEL_H
