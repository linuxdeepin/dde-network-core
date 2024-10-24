// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DSLCONTROLLER_H
#define DSLCONTROLLER_H

#include "networkconst.h"
#include "netutils.h"

#include <QMap>
#include <QObject>

namespace dde {

namespace network {

class DSLItem;
class NetworkDeviceBase;
class NetworkDBusProxy;

class DSLController : public QObject
{
    Q_OBJECT

    friend class NetworkInterProcesser;
    friend class NetworkManagerProcesser;

public:
    void connectItem(DSLItem *item);                                      // 连接到当前的DSL
    void connectItem(const QString &uuid);                                // 根据UUID连接当前DSL
    void disconnectItem();                                                // 断开连接
    inline QList<DSLItem *> items() const { return m_items; }             // 返回所有的DSL列表

Q_SIGNALS:
    void itemAdded(const QList<DSLItem *> &);                             // 新增DSL项目
    void itemRemoved(const QList<DSLItem *> &);                           // 移除DSL项目
    void itemChanged(const QList<DSLItem *> &);                           // 项目发生变化（一般是ID发生了变化）
    void activeConnectionChanged();                                       // 连接状态发生变化

protected:
    explicit DSLController(NetworkDBusProxy *networkInter, QObject *parent = Q_NULLPTR);
    ~DSLController();

    void updateDevice(const QList<NetworkDeviceBase *> &devices);
    void updateDSLItems(const QJsonArray &dsljson);
    void updateActiveConnections(const QJsonObject &connectionJson);       // 更新DSL的活动连接信息

private:
    DSLItem *findItem(const QString &path);
    DSLItem *findDSLItemByUuid(const QString &uuid);

private:
    QList<DSLItem *> m_items;
    NetworkDBusProxy *m_networkInter;
    QMap<QString, QString> m_deviceInfo;
    QString m_activePath;
};

class DSLItem : public ControllItems
{
    friend class DSLController;

protected:
    DSLItem();
    ~DSLItem();
};

}

}

#endif // UDSLCONTROLLER_H
