// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef VPNCONTROLLER_H
#define VPNCONTROLLER_H

#include "networkconst.h"
#include "netutils.h"

#include <QObject>

namespace dde {

namespace network {
class NetworkDBusProxy;
class VPNItem;

class VPNController : public QObject
{
    Q_OBJECT

    friend class NetworkInterProcesser;
    friend class NetworkManagerProcesser;

public:
    void setEnabled(const bool enabled);                                                  // 开启或者关闭VPN
    inline bool enabled() const { return m_enabled; }                                            // VPN开启还是关闭
    void connectItem(VPNItem *item);                                                             // 连接VPN
    void connectItem(const QString &uuid);                                                       // 连接VPN(重载函数)
    void disconnectItem();                                                                       // 断开当前活动VPN连接
    inline QList<VPNItem *> items() { return m_vpnItems; }                                       // 获取所有的VPN列表

Q_SIGNALS:
    void enableChanged(const bool);                                                              // 开启关闭VPN发出的信号
    void itemAdded(const QList<VPNItem *> &);                                                    // 新增VPN发出的信号
    void itemRemoved(const QList<VPNItem *> &);                                                  // 移除VPN发出的信号
    void itemChanged(const QList<VPNItem *> &);                                                  // VPN项发生变化（ID）
    void activeConnectionChanged();                                                              // 活动连接发生变化的时候发出的信号

protected:
    explicit VPNController(NetworkDBusProxy *networkInter, QObject *parent = Q_NULLPTR);
    ~VPNController();

    void updateVPNItems(const QJsonArray &vpnArrays);
    void updateActiveConnection(const QJsonObject &activeConection);

private:
    VPNItem *findItem(const QString &path);
    VPNItem *findItemByUuid(const QString &uuid);

private Q_SLOTS:
    void onEnableChanged(const bool enabled);

private:
    NetworkDBusProxy *m_networkInter;
    bool m_enabled;
    QList<VPNItem *> m_vpnItems;
    QString m_activePath;
};

class VPNItem : public ControllItems
{
    friend class VPNController;

protected:
    VPNItem();
    ~VPNItem();
};

}

}

#endif // UVPNCONTROLLER_H
