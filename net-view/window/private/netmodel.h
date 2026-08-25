// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef NETMODEL_H
#define NETMODEL_H

#include <QAbstractItemModel>
#define CUSTOMROLE (Qt::UserRole + 100)

namespace dde {
namespace network {

class NetItem;

class NetModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit NetModel(QObject *parent = nullptr);
    ~NetModel() override;

    enum NetModelRole {
        NetItemRole = Qt::UserRole + 300,
        NetItemIdRole,
        NetItemTypeRole,
    };

    void setRoot(NetItem *object);
    NetItem *toObject(const QModelIndex &index);

    // inherited from QAbstractItemModel
    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parentIndex = QModelIndex()) const override;
    QModelIndex index(const NetItem *object);
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

private:
    void connectObject(const NetItem *obj);
    void disconnectObject(const NetItem *obj);

protected Q_SLOTS:
    void updateObject();
    void aboutToAddObject(const NetItem *parent, int pos);
    void addObject(const NetItem *child);
    void aboutToRemoveObject(const NetItem *parent, int pos);
    void removeObject(const NetItem *child);
    void aboutToBeMoveObject(const NetItem *parent, int pos, const NetItem *newParent, int newPos);
    void moveObject(const NetItem *child);

private:
    NetItem *m_treeRoot;
    bool m_moving;
};

} // namespace network
} // namespace dde
#endif // NETMODEL_H
