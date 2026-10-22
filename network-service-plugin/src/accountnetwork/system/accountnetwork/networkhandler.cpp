// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "networkhandler.h"
#include "networkconfig.h"
#include "accountmanager.h"
#include "constants.h"

#include <QTimer>

#include <iterator>

#include <NetworkManagerQt/Device>

using namespace accountnetwork::systemservice;
using network::systemservice::DeviceSnapshot;
using network::systemservice::NetworkManagerProxy;

NetworkHandler::NetworkHandler(AccountManager *accountManager, NetworkConfig *conf, QObject *parent)
    : QObject(parent)
    , m_accountManager(accountManager)
    , m_networkConfig(conf)
{
    // 代理在 NM 线程，本对象在插件线程 —— AutoConnection 自动变成队列投递
    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
    m_devicesConnection = connect(proxy, &NetworkManagerProxy::devicesChanged,
                                  this, &NetworkHandler::onDevicesChanged);
    m_idConnection = connect(proxy, &NetworkManagerProxy::activeConnectionIdReady,
                             this, &NetworkHandler::onActiveConnectionIdReady);
    onDevicesChanged();
}

NetworkHandler::~NetworkHandler()
{
    disconnect(m_devicesConnection);
    disconnect(m_idConnection);
}

void NetworkHandler::disconnectNetwork()
{
    // D-Bus 调用的入口，运行在插件线程；写操作经代理排队到 NM 线程
    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
    const QList<DeviceSnapshot> devices = proxy->devices();
    for (const DeviceSnapshot &device : devices) {
        if (device.type != NetworkManager::Device::Type::Ethernet
                && device.type != NetworkManager::Device::Type::Wifi)
            continue;

        if (device.activeConnectionUni.isEmpty())
            continue;

        proxy->deactivateConnection(device.activeConnectionUni);
    }
}

// 代理每次刷新缓存都会发 devicesChanged（含设备状态变化）。
// 原实现通过 ActiveConnection::stateChanged 在 Activated 时记录配置，
// 这里等价地按“设备已激活且活跃连接变化”触发，避免重复写配置文件。
void NetworkHandler::onDevicesChanged()
{
    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
    const QList<DeviceSnapshot> devices = proxy->devices();

    QSet<QString> aliveDevices;
    QSet<QString> aliveConnections;
    aliveDevices.reserve(devices.size());
    aliveConnections.reserve(devices.size());

    for (const DeviceSnapshot &device : devices) {
        if (device.type != NetworkManager::Device::Type::Wifi
                && device.type != NetworkManager::Device::Type::Ethernet) {
            continue;
        }
        aliveDevices.insert(device.uni);
        if (!device.activeConnectionUni.isEmpty()) {
            aliveConnections.insert(device.activeConnectionUni);
        }

        // 只在设备真正激活后才记录；activeConnectionUni 在 Activating 阶段就已写入
        if (device.state != NetworkManager::Device::Activated || device.activeConnectionUni.isEmpty()) {
            continue;
        }
        // 同一条活跃连接只记录一次
        if (m_savedConnections.value(device.uni) == device.activeConnectionUni) {
            continue;
        }
        // 已经问过代理、结果还没回来
        if (m_pendingQueries.contains(device.activeConnectionUni)) {
            continue;
        }
        m_pendingQueries.insert(device.activeConnectionUni);
        proxy->requestActiveConnectionId(device.activeConnectionUni,
                                         device.type == NetworkManager::Device::Type::Wifi);
    }

    // 清理已消失的设备和查询
    for (auto it = m_savedConnections.begin(); it != m_savedConnections.end();) {
        it = aliveDevices.contains(it.key()) ? std::next(it) : m_savedConnections.erase(it);
    }
    for (auto it = m_pendingQueries.begin(); it != m_pendingQueries.end();) {
        it = aliveConnections.contains(*it) ? std::next(it) : m_pendingQueries.erase(it);
    }
}

void NetworkHandler::onActiveConnectionIdReady(const QString &activeConnectionUni, const QString &saveId)
{
    m_pendingQueries.remove(activeConnectionUni);
    if (saveId.isEmpty() || m_accountManager->account().isEmpty()) {
        // 设备稳定在 Activated 时不会再发 devicesChanged，只等就永远不会重试；
        // 做有限次退避重试，避免账户网络映射静默丢失。
        scheduleRetry();
        return;
    }

    const QString account = m_accountManager->account();

    // 用最新快照而非缓存的 m_devices：查询往返期间设备状态可能已经变了
    const QList<DeviceSnapshot> devices = NetworkManagerProxy::instance()->devices();
    for (const DeviceSnapshot &device : devices) {
        if (device.activeConnectionUni != activeConnectionUni
                || device.state != NetworkManager::Device::Activated) {
            continue;
        }
        qCDebug(DSM) << "activated:" << saveId << "device:" << device.interfaceName
                     << "account name:" << account;
        m_networkConfig->saveNetwork(account, saveId, device.interfaceName);
        m_savedConnections.insert(device.uni, activeConnectionUni);
        m_retryCount = 0;
        return;
    }
}

void NetworkHandler::scheduleRetry()
{
    if (m_retryScheduled || m_retryCount >= kMaxRetries) {
        return;
    }
    m_retryScheduled = true;
    QTimer::singleShot(kRetryIntervalMs, this, [this] {
        m_retryScheduled = false;
        ++m_retryCount;
        onDevicesChanged();
    });
}
