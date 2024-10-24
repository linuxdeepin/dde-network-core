// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "hotspotcontroller.h"
#include "wirelessdevice.h"
#include "networkdbusproxy.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

#include <networkmanagerqt/manager.h>

using namespace dde::network;
using namespace NetworkManager;

HotspotController::HotspotController(NetworkDBusProxy *networkInter, QObject *parent)
    : QObject(parent)
    , m_networkInter(networkInter)
{
    Q_ASSERT(m_networkInter);
}

HotspotController::~HotspotController()
{
}

void HotspotController::setEnabled(WirelessDevice *device, const bool enable)
{
    QList<HotspotItem *> deviceHotsItem = items(device);
    if (enable) {
        auto getTimeStamp = [](const QString &path) {
            NetworkManager::Connection::Ptr connection(new NetworkManager::Connection(path));
            return connection->settings()->timestamp();
        };
       // 在打开热点的时候,默认打开上次打开的热点
       auto lastNode = std::max_element(deviceHotsItem.begin(), deviceHotsItem.end(), [ getTimeStamp ](HotspotItem *item1, HotspotItem *item2) {
               return getTimeStamp(item1->connection()->path()) < getTimeStamp(item2->connection()->path());
       });
       if (lastNode != deviceHotsItem.end())
           m_networkInter->ActivateConnection((*lastNode)->connection()->uuid(), QDBusObjectPath(device->path()));
    } else {
        // 在关闭热点的时候,找到当前已经连接的热点,并断开它的连接
        disconnectItem(device);
    }
}

bool HotspotController::enabled(WirelessDevice *device)
{
    return device->hotspotEnabled();
}

bool HotspotController::supportHotspot()
{
    return m_devices.size() > 0;
}

void HotspotController::connectItem(HotspotItem *item)
{
    // 获取当前连接的UUID和设备的path
    m_networkInter->ActivateConnection(item->connection()->uuid(), QDBusObjectPath(item->devicePath()));
}

void HotspotController::connectItem(WirelessDevice *device, const QString &uuid)
{
    for (HotspotItem *item : m_hotspotItems) {
        if (item->device() == device && item->connection()->uuid() == uuid) {
            connectItem(item);
            break;
        }
    }
}

void HotspotController::disconnectItem(WirelessDevice *device)
{
    QList<HotspotItem *> deviceHotspotsItem = items(device);
    for (HotspotItem *item : deviceHotspotsItem) {
        if (item->status() == ConnectionStatus::Activated &&
                !item->activeConnection().isEmpty()) {
            deactivateConnection(item->activeConnection());
        }
    }
}

QList<HotspotItem *> HotspotController::items(WirelessDevice *device)
{
    // 获取当前设备对应的所有的热点
    QList<HotspotItem *> items;
    for (HotspotItem *item : m_hotspotItems) {
        if (item->device() == device)
            items << item;
    }

    return items;
}

QList<WirelessDevice *> HotspotController::devices()
{
    return m_devices;
}

HotspotItem *HotspotController::findItem(WirelessDevice *device, const QJsonObject &json)
{
    // 从列表中查找指定UUID的连接
    for (HotspotItem *item : m_hotspotItems) {
        if (item->device() == device
                && item->connection()->uuid() == json.value("Uuid").toString())
            return item;
    }

    return Q_NULLPTR;
}

bool HotspotController::isHotspotConnection(const QString &uuid)
{
    for (HotspotItem *item : m_hotspotItems) {
        if (item->connection()->uuid() == uuid)
            return true;
    }

    return false;
}

void HotspotController::updateActiveConnection(const QJsonObject &activeConnections)
{
    QList<WirelessDevice *> activeDevices;
    // 先保存所有的连接状态
    QMap<QString, ConnectionStatus> allConnectionStatus;
    for (HotspotItem *item : m_hotspotItems) {
        allConnectionStatus[item->connection()->uuid()] = item->status();
        item->setConnectionStatus(ConnectionStatus::Deactivated);
        item->setActiveConnection("");
    }

    // 在这里记录一个标记，用来表示是否发送活动连接发生变化的信号，
    // 因为这个连接更新后，紧接着会调用获取活动连接信息的信号，响应这个信号会调用下面的updateActiveConnectionInfo函数
    bool activeConnChanged = false;

    QStringList keys = activeConnections.keys();
    for (int i = 0; i < keys.size(); i++) {
        QString path = keys[i];
        QJsonObject activeConnection = activeConnections.value(path).toObject();

        QString uuid = activeConnection.value("Uuid").toString();

        if (!isHotspotConnection(uuid))
            continue;

        ConnectionStatus state = convertConnectionStatus(activeConnection.value("State").toInt());

        QJsonArray devicePaths = activeConnection.value("Devices").toArray();
        for (const QJsonValue jsonValue : devicePaths) {
            QString devicePath = jsonValue.toString();
            WirelessDevice *device = findDevice(devicePath);
            HotspotItem *hotspotItem = findItem(device, activeConnection);
            if (!hotspotItem)
                continue;

            hotspotItem->setConnectionStatus(state);
            hotspotItem->setActiveConnection(path);
            if (!allConnectionStatus.contains(uuid))
                continue;

            ConnectionStatus oldConnectionStatus = allConnectionStatus[uuid];
            // 上一次热点的连接状态为正在激活，当前连接状态为激活
            // 或者上一次热点的连接状态为正在取消激活，当前连接状态为取消激活，将其判断为最近一次有效的连接
            // 此时去更新热点开关的使能状态
            if ((oldConnectionStatus == ConnectionStatus::Activating && state == ConnectionStatus::Activated)
                || (oldConnectionStatus == ConnectionStatus::Deactivating && state == ConnectionStatus::Deactivated)) {
                Q_EMIT enableHotspotSwitch(true);
            }

            if (oldConnectionStatus != hotspotItem->status()) {
                activeConnChanged = true;
                if (!activeDevices.contains(device))
                    activeDevices << device;
            }
        }
    }
    if (activeConnChanged)
        Q_EMIT activeConnectionChanged(activeDevices);
}

WirelessDevice *HotspotController::findDevice(const QString &path)
{
    for (WirelessDevice *device : m_devices) {
        if (device->path() == path)
            return device;
    }

    return Q_NULLPTR;
}

void HotspotController::updateDevices(const QList<NetworkDeviceBase *> &devices)
{
    QList<WirelessDevice *> tmpDevices = m_devices;
    m_devices.clear();
    for (NetworkDeviceBase *device : devices) {
        if (device->deviceType() != DeviceType::Wireless)
            continue;

        if (!device->supportHotspot())
            continue;

        if (!device->isEnabled())
            continue;

        m_devices << static_cast<WirelessDevice *>(device);
    }

    // 移除不在列表中的热点的数据，防止数据更新不及时访问了野指针
    for (auto it = m_hotspotItems.begin(); it != m_hotspotItems.end();) {
        if (!m_devices.contains((*it)->device())) {
            delete (*it);
            it = m_hotspotItems.erase(it);
        } else {
            ++it;
        }
    }

    bool hotspotEnabled = (m_devices.size() > 0);
    if ((tmpDevices.size() > 0) != hotspotEnabled)
        Q_EMIT enabledChanged(hotspotEnabled);

    // 查找移除的设备
    QList<WirelessDevice *> rmDevices;
    for (WirelessDevice *device : tmpDevices) {
        if (!m_devices.contains(device))
            rmDevices << device;
    }

    // 查找新增的设备
    QList<WirelessDevice *> newDevices;
    for (WirelessDevice *device : m_devices) {
        if (!tmpDevices.contains(device))
            newDevices << device;
    }

    // 告诉外面有新增的热点设备
    if (newDevices.size() > 0)
        Q_EMIT deviceAdded(newDevices);

    // 告诉外面有移除的热点设备
    if (rmDevices.size() > 0)
        Q_EMIT deviceRemove(rmDevices);
}

void HotspotController::updateConnections(const QJsonArray &jsons)
{
    // 筛选出通用的(HwAddress为空)热点和指定HwAddress的热点
    QList<QJsonObject> commonConnections;
    QMap<QString, QList<QJsonObject>> deviceConnections;
    for (QJsonValue jsonValue : jsons) {
        QJsonObject json = jsonValue.toObject();
        QString hwAddress = json.value("HwAddress").toString();
        if (hwAddress.isEmpty())
            commonConnections << json;
        else
            deviceConnections[hwAddress] << json;
    }

    // 将所有热点的UUID缓存，用来对比不存在的热点，删除不存在的热点
    QMap<WirelessDevice *, QList<HotspotItem *>> newItems;
    QMap<WirelessDevice *, QList<HotspotItem *>> infoChangeItems;
    QStringList allHotsItem;
    // HwAddress为空的热点适用于所有的设备，HwAddress不为空的热点只适用于指定的设备
    for (WirelessDevice *device : m_devices) {
        QList<QJsonObject> hotspotJsons = commonConnections;
        if (deviceConnections.contains(device->realHwAdr()))
            hotspotJsons << deviceConnections[device->realHwAdr()];

        for (QJsonValue value : hotspotJsons) {
            QJsonObject json = value.toObject();
            HotspotItem *item = findItem(device, json);
            if (!item) {
                item = new HotspotItem(device);
                m_hotspotItems << item;
                newItems[device] << item;
            } else {
                if (item->connection()->ssid() != json.value("Ssid").toString())
                    infoChangeItems[device] << item;
            }

            item->setConnection(json);
            QString pathName = QString("%1-%2").arg(device->path()).arg(json.value("Path").toString());
            allHotsItem << pathName;
        }
    }

    // 若现有连接发生变化，则抛出变化的信号
    if (infoChangeItems.size() > 0)
        Q_EMIT itemChanged(infoChangeItems);

    // 如果有新增的连接，则发送新增连接的信号
    if (newItems.size() > 0)
        Q_EMIT itemAdded(newItems);

    QMap<WirelessDevice *, QList<HotspotItem *>> rmItemsMap;
    QList<HotspotItem *> rmItems;
    // 删除列表中不存在的设备
    for (HotspotItem *item : m_hotspotItems) {
        QString pathKey = QString("%1-%2").arg(item->device()->path()).arg(item->connection()->path());
        if (!allHotsItem.contains(pathKey)) {
            rmItemsMap[item->device()] << item;
            rmItems << item;
        }
    }

    // 从原来的列表中移除已经删除的对象
    for (HotspotItem *item : rmItems)
        m_hotspotItems.removeOne(item);

    // 如果有删除的连接，向外发送删除的信号
    if (rmItemsMap.size() > 0)
        Q_EMIT itemRemoved(rmItemsMap);

    // 清空已经删除的对象
    for (HotspotItem *item : rmItems)
        delete item;
}

/**
 * @brief UHotspotItem::UHotspotItem
 */

HotspotItem::HotspotItem(WirelessDevice *device)
    : ControllItems()
    , m_device(device)
    , m_devicePath(m_device->path())
{
}

HotspotItem::~HotspotItem()
{
}

QString HotspotItem::name() const
{
    return connection()->id();
}

WirelessDevice *HotspotItem::device() const
{
    return m_device;
}

QString HotspotItem::devicePath() const
{
    return m_devicePath;
}
