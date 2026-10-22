// SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkthread.h"

#include "constants.h"
#include "networkdbus.h"
#include "nmaccess.h"

#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/VpnConnection>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/VpnSetting>

#include <QDBusMessage>
#include <QSet>
#include <QDBusPendingCallWatcher>
#include <QProcess>
#include <QThread>

using namespace NetworkManager;

namespace network {
namespace systemservice {
NetworkThread::NetworkThread(QDBusConnection &dbusConnection, QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
    , m_dbusConnection(dbusConnection)
    , m_networkConfig(new NetworkEnabledConfig())
    , m_dbusService(new QDBusServiceWatcher("org.freedesktop.NetworkManager", QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , m_count(0)
{
    // 本对象直接调用 NetworkManagerQt，必须与 NM Qt 单例同线程。
    // 该线程由 ensureNetworkManagerInitialized() 锚定，这里不再自建线程。
    moveToThread(networkManagerThread());

    connect(m_dbusService, &QDBusServiceWatcher::serviceRegistered, this, &NetworkThread::init);
    QDBusConnection::systemBus().connect("org.freedesktop.NetworkManager", "", "org.freedesktop.NetworkManager.VPN.Connection", "VpnStateChanged", this, SLOT(onVpnStateChanged(QDBusMessage)));
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner");
    message << "org.freedesktop.NetworkManager";
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(init()));
}

NetworkThread::~NetworkThread()
{
    delete m_networkConfig;
    m_networkConfig = nullptr;
}

bool NetworkThread::VpnEnabled() const
{
    return m_networkConfig->vpnEnabled();
}

void NetworkThread::setVpnEnabled(bool enabled)
{
    qCDebug(DSM()) << "set VpnEnabled" << enabled;
    if (!enabled) {
        disableVpn();
    }
    QString err = setPropVpnEnabled(enabled);
    m_networkConfig->setVpnEnabled(enabled);
    if (!err.isEmpty()) {
        qCWarning(DSM()) << "set VpnEnabled error:" << err;
    }
}

QDBusObjectPath NetworkThread::EnableDevice(const QString &pathOrIface, const bool enabled, const QDBusMessage &message)
{
    NMQT_ASSERT_THREAD();
    qCInfo(DSM()) << "call EnableDevice, ifc:" << pathOrIface << ", enabled: " << enabled;

    QString err;
    QString path = doEnableDevice(pathOrIface, enabled, err);
    if (err.isEmpty()) {
        dbusConnection().send(message.createReply(QVariant::fromValue(QDBusObjectPath(path))));
    } else {
        dbusConnection().send(message.createErrorReply(QDBusError::Failed, err));
    }
    return QDBusObjectPath();
}

bool NetworkThread::IsDeviceEnabled(const QString &pathOrIface, const QDBusMessage &message)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(pathOrIface);
    if (!snapshot.isValid()) {
        dbusConnection().send(message.createErrorReply(QDBusError::Failed, "not found device"));
        return true;
    }
    dbusConnection().send(message.createReply(m_networkConfig->deviceEnabled(snapshot.interfaceName)));
    return true;
}

void NetworkThread::Ping(const QString &host, const QDBusMessage &message)
{
    Q_UNUSED(host)
    dbusConnection().send(message.createReply());
}

bool NetworkThread::ToggleWirelessEnabled(const QDBusMessage &message)
{
    NMQT_ASSERT_THREAD();
    bool enabled = NetworkManager::isWirelessEnabled();
    enabled = !enabled;
    NetworkManagerProxy::instance()->setWirelessEnabled(enabled);

    const QList<DeviceSnapshot> list = NetworkManagerProxy::instance()->devices();
    for (const DeviceSnapshot &dev : list) {
        if (dev.type != NetworkManager::Device::Wifi) {
            continue;
        }
        QString err;
        doEnableDevice(dev.uni, enabled, err);
        if (!err.isEmpty()) {
            qCWarning(DSM()) << QString("failed to enable %1 device %2: %3").arg(enabled).arg(dev.uni).arg(err);
        }
    }

    dbusConnection().send(message.createReply(enabled));
    return true;
}

void NetworkThread::init()
{
    NMQT_ASSERT_THREAD();
    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
    if (!m_isInitialized) {
        m_isInitialized = true;
        // 库信号只在代理内部 connect；这里消费代理的自有信号
        connect(proxy, &NetworkManagerProxy::devicesChanged, this, &NetworkThread::onDevicesChanged);
        // 必须持续订阅：VPN 自动连接依赖“NM 变为 Connected”这一时刻，
        // 只在 init 里跑一次会让启动后才连上的网络永远不触发自动 VPN。
        connect(proxy, &NetworkManagerProxy::statusChanged, this, &NetworkThread::onStatusChanged);
        onStatusChanged(NetworkManager::status());
    }
    // NM 重启后设备集合可能整体变化，清空后按全新建处理
    m_devices.clear();
    m_deviceStates.clear();
    onDevicesChanged();
}

// 代理每次刷新缓存都会发 devicesChanged。这里做增量收敛：新增设备补齐启用状态、
// 接口名变化迁移配置、状态变化重放 onDeviceStateChanged、消失的设备清理。
void NetworkThread::onDevicesChanged()
{
    NMQT_ASSERT_THREAD();
    const QList<DeviceSnapshot> list = NetworkManagerProxy::instance()->devices();

    QSet<QString> alive;
    for (const DeviceSnapshot &dev : list) {
        alive.insert(dev.uni);
        auto ifaceIt = m_devices.find(dev.uni);

        if (dev.interfaceName.isEmpty()) {
            // 接口名失效：清掉该设备遗留的启用状态配置
            if (ifaceIt != m_devices.end()) {
                onInterfaceNameChanged(dev.uni, ifaceIt.value());
                m_devices.erase(ifaceIt);
                m_deviceStates.remove(dev.uni);
            }
            continue;
        }

        if (ifaceIt == m_devices.end()) {
            // 新设备：按配置补齐启用状态
            m_devices.insert(dev.uni, dev.interfaceName);
            m_deviceStates.insert(dev.uni, dev.state);
            QString err;
            doEnableDevice(dev.interfaceName, m_networkConfig->deviceEnabled(dev.interfaceName), err);
            continue;
        }

        auto stateIt = m_deviceStates.find(dev.uni);
        const bool stateChanged = (stateIt == m_deviceStates.end() || stateIt.value() != dev.state);
        stateIt = m_deviceStates.insert(dev.uni, dev.state);

        if (ifaceIt.value() != dev.interfaceName) {
            // 接口名变化：迁移配置并重新应用
            const QString oldIface = ifaceIt.value();
            ifaceIt.value() = dev.interfaceName;
            onInterfaceNameChanged(dev.uni, oldIface);
            continue;
        }

        if (stateChanged) {
            onDeviceStateChanged(dev.uni);
        }
    }

    // 移除已消失的设备
    for (auto it = m_devices.begin(); it != m_devices.end();) {
        if (alive.contains(it.key())) {
            ++it;
            continue;
        }
        onDeviceRemoved(it.key());
        it = m_devices.erase(it);
    }
}

void NetworkThread::onDeviceRemoved(const QString &uni)
{
    NMQT_ASSERT_THREAD();
    qCDebug(DSM()) << "device removed" << uni;
    m_deviceStates.remove(uni);
}

void NetworkThread::onDeviceStateChanged(const QString &uni)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(uni);
    if (!snapshot.isValid()) {
        return;
    }

    // 配置以接口名为 key（见 doEnableDevice），此处必须用同一 key
    const bool enabled = m_networkConfig->deviceEnabled(snapshot.interfaceName);
    if (enabled) {
        if (snapshot.state == Device::Activated && !snapshot.activeConnectionUni.isEmpty()) {
            qCDebug(DSM()) << "device connection success";
            const ActiveConnection::Ptr activeConnection = NetworkManager::findActiveConnection(snapshot.activeConnectionUni);
            if (!activeConnection.isNull() && !activeConnection->connection().isNull()) {
                m_networkConfig->setConnectionInfo(snapshot.interfaceName, activeConnection->connection()->uuid());
                m_networkConfig->saveConfig();
            }
        }
        return;
    }

    if (snapshot.state >= Device::Preparing && snapshot.state <= Device::Activated) {
        qCDebug(DSM()) << "disconnect device" << snapshot.uni;
        if (!snapshot.activeConnectionUni.isEmpty()) {
            NetworkManagerProxy::instance()->deactivateConnection(snapshot.activeConnectionUni);
        }
    }
}

void NetworkThread::onInterfaceNameChanged(const QString &uni, const QString &oldIface)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(uni);
    const QString newIface = snapshot.interfaceName;
    if (newIface.isEmpty() || newIface == "/") {
        qCDebug(DSM()) << "interface name gone, drop enabled config:" << oldIface;
        m_networkConfig->removeDeviceEnabled(oldIface);
        const QString err = m_networkConfig->saveConfig();
        if (!err.isEmpty()) {
            qCWarning(DSM()) << "save config failed, err:" << err;
        }
        return;
    }
    qCDebug(DSM()) << "interface renamed" << oldIface << "->" << newIface;
    QString err;
    doEnableDevice(newIface, m_networkConfig->deviceEnabled(newIface), err);
}

void NetworkThread::onVpnStateChanged(const QDBusMessage &msg)
{
    auto args = msg.arguments();
    uint state = args.at(0).toUInt();
    uint reason = args.at(1).toUInt();
    qCDebug(DSM()) << msg.path() << "vpn state changed" << (NetworkManager::VpnConnection::State)state << (NetworkManager::VpnConnection::StateChangeReason)reason;
    handleVpnStateChanged(state);
}

void NetworkThread::handleVpnStateChanged(uint state)
{
    NetworkManager::VpnConnection::State s = NetworkManager::VpnConnection::State(state);
    if (s >= VpnConnection::Prepare && s < VpnConnection::Activated) {
        setPropVpnEnabled(true);
    }
}

QString NetworkThread::doEnableDevice(const QString &pathOrIface, const bool enabled, QString &err)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(pathOrIface);
    if (!snapshot.isValid()) {
        err = "not found device";
        return "/";
    }
    m_networkConfig->setDeviceEnabled(snapshot.interfaceName, enabled);
    m_networkConfig->saveConfig();
    Q_EMIT DeviceEnabled(QDBusObjectPath(snapshot.uni), enabled);
    QString ret = "/";
    if (enabled) {
        ret = enableDevice(snapshot.uni);
    } else {
        err = disableDevice(snapshot.uni);
    }
    return ret;
}

void NetworkThread::disableVpn()
{
    NMQT_ASSERT_THREAD();
    auto activeConnectionList = NetworkManager::activeConnections();
    for (auto &&conn : activeConnectionList) {
        if (conn->vpn() && (conn->state() == NetworkManager::ActiveConnection::Activating || conn->state() == NetworkManager::ActiveConnection::Activated)) {
            NetworkManagerProxy::instance()->deactivateConnection(conn->path());
        }
    }
}

QString NetworkThread::setPropVpnEnabled(bool enabled)
{
    QString err;
    if (m_networkConfig->vpnEnabled() != enabled) {
        m_networkConfig->setVpnEnabled(enabled);
        err = m_networkConfig->saveConfig();

        QVariantMap properties;
        properties.insert("VpnEnabled", enabled);

        QList<QVariant> arguments;
        arguments.push_back(NETWORK_SYSTEM_DAEMON_INTERFACE);
        arguments.push_back(properties);
        arguments.push_back(QStringList());

        QDBusMessage msg = QDBusMessage::createSignal(NETWORK_SYSTEM_DAEMON_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        msg.setArguments(arguments);
        dbusConnection().send(msg);
    }
    return err;
}

QString NetworkThread::enableDevice(const QString &uniOrIface)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(uniOrIface);
    if (!snapshot.isValid()) {
        return "/";
    }
    const Device::Ptr device = NetworkManager::findNetworkInterface(snapshot.uni);
    if (device.isNull()) {
        return "/";
    }

    QString connPath0;
    const QString &device_uuid = m_networkConfig->connectionUuid(device->interfaceName());
    if (!device_uuid.isEmpty()) {
        NetworkManager::Connection::Ptr current_connection = NetworkManager::findConnectionByUuid(device_uuid);
        if (current_connection && current_connection->settings()->autoconnect()) {
            connPath0 = current_connection->path();
        }
    }
    if (connPath0.isEmpty()) {
        auto availableConnections = device->availableConnections();
        qCDebug(DSM()) << "available connections:" << availableConnections;
        QDateTime maxTs;
        for (auto &&connPath : availableConnections) {
            auto settings = connPath->settings();
            if (!settings->autoconnect()) {
                continue;
            }
            QDateTime ts = settings->timestamp();
            if (maxTs < ts || connPath0.isEmpty()) {
                maxTs = ts;
                connPath0 = connPath->path();
            }
        }
    }
    if (!connPath0.isEmpty()) {
        NetworkManagerProxy::instance()->activateConnection(connPath0, device->uni());
        qDebug() << "connected:" << connPath0;
    }

    bool enabled = NetworkManager::isNetworkingEnabled();
    if (!enabled) {
        NetworkManagerProxy::instance()->setNetworkingEnabled(true);
    }
    device->setAutoconnect(true);
    if (device->type() == NetworkManager::Device::Wifi) {
        if (!NetworkManager::isWirelessEnabled()) {
            requestAirplaneWifiEnabled(device->interfaceName());
        }
    }

    return connPath0.isEmpty() ? "/" : connPath0;
}

QString NetworkThread::disableDevice(const QString &uniOrIface)
{
    NMQT_ASSERT_THREAD();
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(uniOrIface);
    if (!snapshot.isValid()) {
        return "not found device";
    }
    const Device::Ptr device = NetworkManager::findNetworkInterface(snapshot.uni);
    if (device.isNull()) {
        return "not found device";
    }

    device->setAutoconnect(false);
    // TODO:
    // cause of nm'bug, sometimes accessapoints list is nil
    // so add a judge in system network, if get nil in GetAllAccessPoints func, set wirelessEnable down.
    // device->state 的值更新不及时，不判断
    auto oldWirelessEnabled = NetworkManager::isWirelessEnabled();
    if (device->type() == NetworkManager::Device::Wifi) {
        NetworkManager::WirelessDevice::Ptr wDevice = device.objectCast<NetworkManager::WirelessDevice>();
        if (wDevice->mode() != WirelessDevice::ApMode) {
            auto accessPointsList = wDevice->accessPoints();
            if (accessPointsList.size() > 0) {
                m_count = 0;
                qCDebug(DSM()) << "have aplist existed!!!";
            } else if (m_count++; m_count > 2) {
                m_count = 0;
                qCInfo(DSM()) << "try to set wireless-enabled, because ap list is empty";
                NetworkManagerProxy::instance()->setWirelessEnabled(false);
                if (oldWirelessEnabled) {
                    NetworkManagerProxy::instance()->setWirelessEnabled(true);
                }
                return QString();
            }
        }
    }
    QDBusPendingReply<> reply = device->disconnectInterface();
    if (reply.isError()) {
        qCWarning(DSM()) << "Failed to disconnect device:" << device->uni() << "Error:" << reply.error().message();
        return reply.error().message();
    }
    return QString();
}

void NetworkThread::onStatusChanged(NetworkManager::Status status)
{
    NMQT_ASSERT_THREAD();
    if (!m_networkConfig->vpnEnabled())
        return;

    if (status != NetworkManager::Status::Connected)
        return;

    QStringList activeConnectionPaths;
    NetworkManager::ActiveConnection::List allactiveConnections = NetworkManager::activeConnections();
    for (const NetworkManager::ActiveConnection::Ptr &activeConnection : allactiveConnections) {
        if (activeConnection->connection().isNull())
            continue;

        if (activeConnection->state() == NetworkManager::ActiveConnection::State::Activated
            || activeConnection->state() == NetworkManager::ActiveConnection::State::Activating)
            activeConnectionPaths << activeConnection->connection()->path();
    }
    QMap<QString, NetworkManager::Connection::Ptr> candidateVpns;
    // 检查到网络状态为连接成功后，检查本地是否存在自动连接的VPN，如果存在，就让它连接
    NetworkManager::Connection::List allConnections = NetworkManager::listConnections();
    for (const NetworkManager::Connection::Ptr &conn : allConnections) {
        // 过滤掉正在连接的或者已经连接成功的
        if (activeConnectionPaths.contains(conn->path()))
            continue;

        // 这里只查找设置了自动连接的
        if (!conn->settings()->autoconnect())
            continue;

        // 查找VPN的连接方式
        NetworkManager::ConnectionSettings::ConnectionType type = conn->settings()->connectionType();
        if (type == NetworkManager::ConnectionSettings::ConnectionType::Vpn) {
            NetworkManager::VpnSetting::Ptr setting = conn->settings()->setting(NetworkManager::Setting::SettingType::Vpn).dynamicCast<NetworkManager::VpnSetting>();
            if (setting.isNull())
                continue;

            QString serviceType = setting->serviceType();
            if (candidateVpns.contains(serviceType)) {
                // 优先使用最后一次连接成功的连接
                const NetworkManager::Connection::Ptr &lastSetting = candidateVpns[serviceType];
                if (lastSetting->settings()->timestamp() < conn->settings()->timestamp())
                    candidateVpns[serviceType] = conn;
            } else {
                candidateVpns[serviceType] = conn;
            }
        }
    }
    for (auto it = candidateVpns.begin(); it != candidateVpns.end(); it++) {
        const NetworkManager::Connection::Ptr &connection = it.value();
        NetworkManagerProxy::instance()->activateConnection(connection->path(), QString());
    }
}

void NetworkThread::requestAirplaneWifiEnabled(const QString &interfaceName)
{
    // 飞行模式查询是同步 D-Bus 调用，直接 waitForFinished() 会阻塞 NM 线程
    // （最长 25s），因此改为 watcher 异步回调。
    QDBusMessage msg = QDBusMessage::createMethodCall("org.deepin.dde.AirplaneMode1", "/org/deepin/dde/AirplaneMode1", "org.freedesktop.DBus.Properties", "Get");
    msg << "org.deepin.dde.AirplaneMode1" << "WifiEnabled";
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, interfaceName](QDBusPendingCallWatcher *self) {
        const QDBusPendingReply<QDBusVariant> reply = *self;
        self->deleteLater();
        if (reply.isError()) {
            qCWarning(DSM()) << "get WifiEnabled err:" << reply.error().message();
            return;
        }
        onAirplaneWifiEnabledResolved(reply.value().variant().toBool(), interfaceName);
    });
}

void NetworkThread::onAirplaneWifiEnabledResolved(bool enabled, const QString &interfaceName)
{
    NMQT_ASSERT_THREAD();
    Q_UNUSED(interfaceName)
    if (enabled) {
        qCDebug(DSM()) << "disable wifi because airplane mode is on";
        return;
    }
    NetworkManagerProxy::instance()->setWirelessEnabled(true);
}

} // namespace systemservice
} // namespace network
