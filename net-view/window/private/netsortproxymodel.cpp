// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "netsortproxymodel.h"

#include "netitem.h"

namespace dde {
namespace network {

NetSortProxyModel::NetSortProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void NetSortProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &NetSortProxyModel::onRowsInserted);
}

bool NetSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    NetItem *item1 = static_cast<NetItem *>(source_left.internalPointer());
    NetItem *item2 = static_cast<NetItem *>(source_right.internalPointer());
    return NetItem::compare(item1, item2);
}

void NetSortProxyModel::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    auto model = sourceModel();
    QList<QModelIndex> indexes;
    for (int row = first; row <= last; ++row) {
        indexes.append(model->index(row, 0, parent));
    }
    while (!indexes.isEmpty()) {
        QModelIndex index = indexes.takeFirst();
        if (index.isValid()) {
            NetItem *item = static_cast<NetItem *>(index.internalPointer());
            connect(item, &NetItem::nameChanged, this, &NetSortProxyModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
            switch (item->itemType()) {
            case NetType::WirelessItem: {
                const NetWirelessItem *obj = qobject_cast<NetWirelessItem *>(item);
                connect(obj, &NetWirelessItem::statusChanged, this, &NetSortProxyModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
                connect(obj, &NetWirelessItem::strengthLevelChanged, this, &NetSortProxyModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
            } break;
            case NetType::WiredItem: {
                const NetWiredItem *connItem = qobject_cast<NetWiredItem *>(item);
                connect(connItem, &NetWiredItem::statusChanged, this, &NetSortProxyModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
            } break;
            default:
                break;
            }
        }
        for (int j = 0; j < model->rowCount(index); j++) {
            indexes.append(model->index(j, 0, index));
        }
    }
    updateSort();
}

void NetSortProxyModel::updateSort()
{
    void *send = sender();
    if (!send)
        return;
    auto model = sourceModel();
    QList<QModelIndex> indexes;
    indexes.append(QModelIndex());
    while (!indexes.isEmpty()) {
        QModelIndex i = indexes.takeFirst();
        if (i.internalPointer() == send) {
            Q_EMIT model->dataChanged(i, i, { sortRole() });
            break;
        }
        for (int j = 0; j < model->rowCount(i); j++) {
            indexes.append(model->index(j, 0, i));
        }
    }
}

} // namespace network
} // namespace dde
