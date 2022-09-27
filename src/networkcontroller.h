// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "networkconst.h"

#include <QObject>

namespace dde {

namespace network {

class ProxyController;
class VPNController;
class DSLController;
class HotspotController;
class NetworkDetails;
class NetworkDeviceBase;
class NetworkProcesser;

enum InterfaceFlags {
    NM_DEVICE_INTERFACE_FLAG_NONE     = 0,          // an alias for numeric zero, no flags set.
    NM_DEVICE_INTERFACE_FLAG_UP       = 0x1,        // the interface is enabled from the administrative point of view. Corresponds to kernel IFF_UP.
    NM_DEVICE_INTERFACE_FLAG_LOWER_UP = 0x2,        // the physical link is up. Corresponds to kernel IFF_LOWER_UP.
    NM_DEVICE_INTERFACE_FLAG_CARRIER  = 0x10000,    // the interface has carrier. In most cases this is equal to the value of @NM_DEVICE_INTERFACE_FLAG_LOWER_UP
};

class NetworkController : public QObject
{
    Q_OBJECT

public:
    static NetworkController *instance();                                        // 对外返回的单例
    static void setActiveSync(const bool sync = false);                          // 设置QDbus服务通讯的方式 ,true表示同步获取，false表示异步获取
    static void setServiceType(const ServiceLoadType serviceType);               // 是否系统级（用于在登录界面，此时尚未进入用户，无法访问用户接口）
    static void setIPConflictCheck(const bool &checkIp);                         // 设置是否主动检测IP冲突
    void updateSync(const bool sync);                                            // 用于中途更新同步或异步方式

    ProxyController *proxyController();                                          // 返回代理控制管理器
    VPNController *vpnController();                                              // 返回VPN控制器
    DSLController *dslController();                                              // DSL控制器
    HotspotController *hotspotController();                                      // 个人热点控制器
    QList<NetworkDetails *> networkDetails();                                    // 网络详情
    QList<NetworkDeviceBase *> devices() const;                                  // 当前网络设备列表
    Connectivity connectivity();                                                 // 获取网络连接状态
    void retranslate();                                                          // 更新翻译

Q_SIGNALS:
    void deviceAdded(QList<NetworkDeviceBase *>);                                 // 新增设备发出的信号
    void deviceRemoved(QList<NetworkDeviceBase *>);                               // 移除设备发出的信号
    void connectivityChanged(const Connectivity &) const;                         // 网络连接状态发生变化的时候发出的信号
    void connectionChanged();                                                     // 连接信息发生变化的时候触发的信号
    void activeConnectionChange();                                                // 网络数据发生变化

protected:
    explicit NetworkController();
    ~NetworkController();

private:
    NetworkProcesser *m_processer;
    ProxyController *m_proxyController;
    VPNController *m_vpnController;
    DSLController *m_dslController;
    HotspotController *m_hotspotController;
    QList<NetworkDetails *> m_networkDetails;
    static bool m_sync;
    static ServiceLoadType m_serviceLoadType;
    static bool m_checkIpConflicted;
};

}

}

#endif // UNETWORKCONTROLLER_H
