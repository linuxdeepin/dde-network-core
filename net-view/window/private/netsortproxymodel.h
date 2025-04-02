// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETSORTPROXYMODEL_H
#define NETSORTPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace dde {
namespace network {

class NetSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit NetSortProxyModel(QObject *parent = nullptr);
    void setSourceModel(QAbstractItemModel *sourceModel) override;

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

protected Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void updateSort();
};
} // namespace network
} // namespace dde

#endif // NETSORTPROXYMODEL_H
