// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netitem.h"

#include "private/netitemprivate.h"

#include <QDebug>

namespace dde {
namespace network {

bool NetItem::compare(NetItem *item1, NetItem *item2)
{
    if (item1->itemType() != item2->itemType())
        return item1->itemType() < item2->itemType();
    switch (item1->itemType()) {
    case NetType::WirelessItem: {
        NetWirelessItem *lItem = qobject_cast<NetWirelessItem *>(item1);
        NetWirelessItem *rItem = qobject_cast<NetWirelessItem *>(item2);
        if ((lItem->status() | rItem->status()) & NetType::CS_Connected) { // 存在已连接的
            return lItem->status() & NetType::CS_Connected;
        }
        if (lItem->strengthLevel() != rItem->strengthLevel())
            return lItem->strengthLevel() > rItem->strengthLevel();
        return item1->name().toLower() < item2->name().toLower();
    }
    // case NetType::ConnectionItem:
    // case NetType::WiredItem: {
    //     NetConnectionItem *lItem = qobject_cast<NetConnectionItem *>(item1);
    //     NetConnectionItem *rItem = qobject_cast<NetConnectionItem *>(item2);
    //     if (lItem->status() != rItem->status())
    //         return lItem->status() > rItem->status();
    //     return item1->name().toLower() < item2->name().toLower();
    // }
    default:
        return item1->name().toLower() < item2->name().toLower();
    }
}

/**
 * 单个列表项的基类
 */
NetItem::NetItem(NetItemPrivate *p, const QString &id)
    : QObject()
    , dptr(p)
{
    setObjectName(id);
}

NetItem::~NetItem() { }

NetItem *NetItem::getChild(int childPos) const
{
    return dptr->getChild(childPos);
}

int NetItem::getChildIndex(const NetItem *child) const
{
    return dptr->getChildIndex(child);
}

#define GETFUN(RETTYPE, CLASS, FUNNAME)                        \
    RETTYPE CLASS::FUNNAME() const                             \
    {                                                          \
        return static_cast<CLASS##Private *>(dptr)->FUNNAME(); \
    }

GETFUN(int, NetItem, getChildrenNumber)
GETFUN(const QVector<NetItem *> &, NetItem, getChildren)
GETFUN(int, NetItem, getIndex)
GETFUN(NetItem *, NetItem, getParent)
GETFUN(NetType::NetItemType, NetItem, itemType)
GETFUN(QString, NetItem, name)
GETFUN(QString, NetItem, id)

// 总线控制器
GETFUN(bool, NetControlItem, isEnabled)
GETFUN(bool, NetControlItem, enabledable)

// 设备基类
// GETFUN(QString, NetDeviceItem, path)
GETFUN(NetType::NetDeviceStatus, NetDeviceItem, status)
GETFUN(QStringList, NetDeviceItem, ips)
GETFUN(int, NetDeviceItem, pathIndex)

GETFUN(const QString &, NetTipsItem, linkActivatedText)
GETFUN(bool, NetTipsItem, tipsLinkEnabled)
// 有线设备

// 无线设备
GETFUN(bool, NetWirelessDeviceItem, apMode)

GETFUN(NetType::NetConnectionStatus, NetConnectionItem, status)

// 无线禁用项

// 有线禁用项

// 有线连接
// 我的网络
// 其他网络
GETFUN(bool, NetWirelessOtherItem, isExpanded)
// 隐藏网络

// 无线网络
GETFUN(uint, NetWirelessItem, flags)
GETFUN(int, NetWirelessItem, strength)
GETFUN(int, NetWirelessItem, strengthLevel)
GETFUN(bool, NetWirelessItem, isSecure)
GETFUN(bool, NetWirelessItem, hasConnection)

QString NetWiredControlItem::name() const
{
    return tr("Wired Network");
}

QString NetWirelessControlItem::name() const
{
    return tr("Wireless Network");
}

QString NetWirelessMineItem::name() const
{
    return tr("My Networks");
}

QString NetWirelessOtherItem::name() const
{
    return tr("Other Networks");
}

QString NetWirelessHiddenItem::name() const
{
    return tr("Connect to hidden network");
}

// VPNTip
QString NetVPNTipsItem::name() const
{
    return tr("VPN configuration is not connected or failed to connect. Please <a style=\"text-decoration: none;\" href=\"go to the control center\">go to the control center</a> for inspection.");
}

// VPN
GETFUN(bool, NetVPNControlItem, isExpanded)

// 系统代理

QString NetSystemProxyControlItem::name() const
{
    return tr("System Proxy");
}

GETFUN(NetType::ProxyMethod, NetSystemProxyControlItem, lastMethod)
GETFUN(NetType::ProxyMethod, NetSystemProxyControlItem, method)
GETFUN(const QString &, NetSystemProxyControlItem, autoProxy)
GETFUN(const QVariantMap &, NetSystemProxyControlItem, manualProxy)
// 应用代理
GETFUN(const QVariantMap &, NetAppProxyControlItem, config)
// Hotspot
GETFUN(const QVariantMap &, NetHotspotControlItem, config)
GETFUN(const QStringList &, NetHotspotControlItem, optionalDevice)
GETFUN(const QStringList &, NetHotspotControlItem, optionalDevicePath)
GETFUN(const QStringList &, NetHotspotControlItem, shareDevice)
GETFUN(bool, NetHotspotControlItem, deviceEnabled)

// 飞行模式提示项
// DetailsItem
GETFUN(const QList<QStringList> &, NetDetailsInfoItem, details)
GETFUN(const int &, NetDetailsInfoItem, index)

QString NetAirplaneModeTipsItem::name() const
{
    return tr("Disable <a style=\"text-decoration: none;\" href=\"Airplane Mode\">Airplane Mode</a> first if you want to connect to a wireless network");
    // return tr("Disable Airplane Mode first if you want to connect to a wireless network");
}

} // namespace network
} // namespace dde
