#ifndef ELEVATIONDOWNLOADERITEMMODEL_H
#define ELEVATIONDOWNLOADERITEMMODEL_H

#include <QAbstractItemModel>
#include "elevationdownloaderitem.h"
#include "elevationtile.h"

class ElevationDownloaderItemModel : public QAbstractItemModel
{
    Q_OBJECT
    ElevationDownloaderItemList m_List;
public:
    explicit ElevationDownloaderItemModel(QObject *parent = 0);
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    const ElevationDownloaderItemList &list() const;
signals:
public slots:
    void addSource(ElevationSource & source);
private slots:
    void cellChanged(int id);
};

#endif // ELEVATIONDOWNLOADERITEMMODEL_H
