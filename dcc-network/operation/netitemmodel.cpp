// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "netitemmodel.h"

namespace dde {
namespace network {
enum NetModelRole {
    NetItemRole = Qt::UserRole + 300,
    NetItemTypeRole,
};

class NetItemSourceModel : public QAbstractItemModel
{
public:
    explicit NetItemSourceModel(QObject *parent = nullptr);
    ~NetItemSourceModel() override;

    void setRoot(NetItem *root);

protected:
    // Basic functionality:
    QModelIndex index(int row, int column, const QModelIndex &parentIndex = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected Q_SLOTS:
    void updateSort();
    void onAddObject(const NetItem *child);

    void AboutToAddObject(const NetItem *, int pos) { beginInsertRows(QModelIndex(), pos, pos); }

    void addObject(const NetItem *child)
    {
        endInsertRows();
        onAddObject(child);
    }

    bool needRemoveNodeItem(const NetItem *childItem) const
    {
        if (childItem && (childItem->itemType() == NetType::NetItemType::SystemProxyControlItem
            || childItem->itemType() == NetType::NetItemType::AppProxyControlItem)) {
            // 对于系统代理和应用代理（应用代理在V25上被裁减了，但是这里还是加上去这个判断，防止以后加上），
            // 这里始终显示当前的节点
            return false;
        }
        return true;
    }

    void AboutToRemoveObject(const NetItem *parentItem, int pos)
    {
        if (!parentItem)
            return;

        if (needRemoveNodeItem(parentItem->getChild(pos)))
            beginRemoveRows(QModelIndex(), pos, pos);
    }

    void removeObject(const NetItem *child)
    {
        if (needRemoveNodeItem(child))
            endRemoveRows();
    }

protected:
    NetItem *m_root;
    friend class NetItemModel;
};

NetItemSourceModel::NetItemSourceModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_root(nullptr)
{
}

NetItemSourceModel::~NetItemSourceModel() { }

void NetItemSourceModel::setRoot(NetItem *root)
{
    if (m_root) {
        disconnect(m_root, nullptr, this, nullptr);
    }
    beginResetModel();
    m_root = root;
    endResetModel();
    if (!m_root)
        return;
    for (auto &&child : m_root->getChildren()) {
        onAddObject(child);
    }
    connect(m_root, &NetItem::childAboutToBeAdded, this, &NetItemSourceModel::AboutToAddObject);
    connect(m_root, &NetItem::childAdded, this, &NetItemSourceModel::addObject);
    connect(m_root, &NetItem::childAboutToBeRemoved, this, &NetItemSourceModel::AboutToRemoveObject);
    connect(m_root, &NetItem::childRemoved, this, &NetItemSourceModel::removeObject);
}

QModelIndex NetItemSourceModel::index(int row, int column, const QModelIndex &) const
{
    if (row < 0 || row >= rowCount()) {
        return QModelIndex();
    }
    return createIndex(row, column, m_root->getChild(row));
}

QModelIndex NetItemSourceModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

int NetItemSourceModel::rowCount(const QModelIndex &parent) const
{
    return (!parent.isValid() && m_root) ? m_root->getChildrenNumber() : 0;
}

int NetItemSourceModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant NetItemSourceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    int i = index.row();
    const NetItem *item = m_root->getChild(i);
    if (!item) {
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
        return item->name();
    case NetItemRole:
        return QVariant::fromValue(item);
    case NetItemTypeRole:
        return item->itemType();
    default:
        break;
    }
    return QVariant();
}

void NetItemSourceModel::updateSort()
{
    void *send = sender();
    if (!send)
        return;
    QList<QModelIndex> indexes;
    indexes.append(QModelIndex());
    while (!indexes.isEmpty()) {
        QModelIndex i = indexes.takeFirst();
        if (i.internalPointer() == send) {
            Q_EMIT dataChanged(i, i, { Qt::DisplayRole });
            break;
        }
        for (int j = 0; j < rowCount(i); j++) {
            indexes.append(index(j, 0, i));
        }
    }
}

void NetItemSourceModel::onAddObject(const NetItem *child)
{
    connect(child, &NetItem::nameChanged, this, &NetItemSourceModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    switch (child->itemType()) {
    case NetType::WirelessItem: {
        const NetWirelessItem *obj = NetItem::toItem<NetWirelessItem>(child);
        connect(obj, &NetWirelessItem::statusChanged, this, &NetItemSourceModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
        connect(obj, &NetWirelessItem::strengthLevelChanged, this, &NetItemSourceModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } break;
    case NetType::ConnectionItem:
    case NetType::WiredItem: {
        const NetConnectionItem *connItem = NetItem::toItem<NetConnectionItem>(child);
        connect(connItem, &NetConnectionItem::statusChanged, this, &NetItemSourceModel::updateSort, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    } break;
    default:
        break;
    }
    updateSort();
}

//////////////////////////////////////////////////////
NetItemModel::NetItemModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // setFilterRole(SearchTextRole);

    setSourceModel(new NetItemSourceModel(this));
    setSortRole(Qt::DisplayRole);
    sort(0);
}

NetItemModel::~NetItemModel() { }

NetItem *NetItemModel::root() const
{

    return static_cast<NetItemSourceModel *>(sourceModel())->m_root;
}

void NetItemModel::setRoot(NetItem *root)
{
    static_cast<NetItemSourceModel *>(sourceModel())->setRoot(root);
}

QHash<int, QByteArray> NetItemModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[NetItemRole] = "item";
    names[NetItemTypeRole] = "type";
    return names;
}

bool NetItemModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    QModelIndex childIndex = sourceModel()->index(source_row, 0, source_parent);
    NetItem *curItem = static_cast<NetItem *>(childIndex.internalPointer());
    if (!curItem) {
        return false;
    }
    // 有的节点出现了两次，所以这里让显示第一次出现的那个节点
    if (m_rowMap.contains(curItem->itemType())) {
        return (m_rowMap[curItem->itemType()] == source_row);
    }

    m_rowMap[curItem->itemType()] = source_row;
    return true;
}

bool NetItemModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    NetItem *leftItem = static_cast<NetItem *>(source_left.internalPointer());
    if (!leftItem)
        return false;

    NetItem *rightItem = static_cast<NetItem *>(source_right.internalPointer());
    if (!rightItem)
        return false;

    if (leftItem->itemType() != rightItem->itemType())
        return leftItem->itemType() < rightItem->itemType();
    switch (leftItem->itemType()) {
    case NetType::WirelessItem: {
        NetWirelessItem *lItem = qobject_cast<NetWirelessItem *>(leftItem);
        NetWirelessItem *rItem = qobject_cast<NetWirelessItem *>(rightItem);
        if ((lItem->status() | rItem->status()) & NetType::CS_Connected) { // 存在已连接的
            return lItem->status() & NetType::CS_Connected;
        }
        // if (lItem->status() != rItem->status())
        //     return lItem->status() > rItem->status();
        if (lItem->strengthLevel() != rItem->strengthLevel())
            return lItem->strengthLevel() > rItem->strengthLevel();
    } break;
    // case NetType::ConnectionItem:
    // case NetType::WiredItem: {
    //     NetConnectionItem *lItem = qobject_cast<NetConnectionItem *>(leftItem);
    //     NetConnectionItem *rItem = qobject_cast<NetConnectionItem *>(rightItem);
    //     if (lItem->status() != rItem->status())
    //         return lItem->status() > rItem->status();
    // } break;
    default:
        break;
    }
    return leftItem->name().toLower() < rightItem->name().toLower();
}

} // namespace network
} // namespace dde
