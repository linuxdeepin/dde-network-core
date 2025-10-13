// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkthread.h"

#include "constants.h"
#include "networkdbus.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/VpnConnection>
#include <NetworkManagerQt/WirelessDevice>

#include <QDBusMessage>
#include <QProcess>
#include <QThread>

using namespace NetworkManager;

namespace network {
namespace systemservice {
NetworkThread::NetworkThread(QDBusConnection &dbusConnection, QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
    , m_thread(new QThread(this))
    , m_dbusConnection(dbusConnection)
    , m_networkConfig(new NetworkEnabledConfig())
    , m_dbusService(new QDBusServiceWatcher("org.freedesktop.NetworkManager", QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , m_count(0)
{
    connect(m_dbusService, &QDBusServiceWatcher::serviceRegistered, this, &NetworkThread::init);
    QDBusConnection::systemBus().connect("org.freedesktop.NetworkManager", "", "org.freedesktop.NetworkManager.VPN.Connection", "VpnStateChanged", this, SLOT(onVpnStateChanged(QDBusMessage)));
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner");
    message << "org.freedesktop.NetworkManager";
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(init()));
    moveToThread(m_thread);
    m_thread->start();
}

NetworkThread::~NetworkThread()
{
    m_thread->quit();
    m_thread->wait();
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
    NetworkManager::Device::Ptr dev = findDevice(pathOrIface);
    if (!dev) {
        dbusConnection().send(message.createErrorReply(QDBusError::Failed, "not found device"));
        return true;
    }
    dbusConnection().send(message.createReply(m_networkConfig->deviceEnabled(dev->interfaceName())));
    return true;
}

void NetworkThread::Ping(const QString &host, const QDBusMessage &message)
{
    dbusConnection().send(message.createReply());
}

bool NetworkThread::ToggleWirelessEnabled(const QDBusMessage &message)
{
    bool enabled = NetworkManager::isWirelessEnabled();
    enabled = !enabled;
    NetworkManager::setWirelessEnabled(enabled);

    auto list = NetworkManager::networkInterfaces();
    for (auto dev : list) {
        if (dev->type() == NetworkManager::Device::Wifi) {
            QString err;
            doEnableDevice(dev->uni(), enabled, err);
            if (!err.isEmpty()) {
                qCWarning(DSM()) << QString("failed to enable %1 device %2: %3").arg(enabled).arg(dev->uni()).arg(err);
            }
        }
    }

    dbusConnection().send(message.createReply(enabled));
    return true;
}

void NetworkThread::init()
{
    if (!m_isInitialized) {
        m_isInitialized = true;
        auto notifier = NetworkManager::notifier();
        connect(notifier, &NetworkManager::Notifier::deviceAdded, this, &NetworkThread::onDeviceAdded);
        connect(notifier, &NetworkManager::Notifier::deviceRemoved, this, &NetworkThread::onDeviceRemoved);
    }
    m_devices.clear();
    addDevicesWithRetry();
}

void NetworkThread::onDeviceAdded(const QString &uni)
{
    if (m_devices.contains(uni)) {
        return;
    }
    NetworkManager::Device::Ptr dev = NetworkManager::findNetworkInterface(uni);
    if (dev) {
        auto interface = dev->interfaceName();
        m_devices.insert(uni, interface);
        connect(dev.get(), &Device::stateChanged, this, &NetworkThread::onDevicestateChanged);
        connect(dev.get(), &Device::interfaceNameChanged, this, &NetworkThread::onInterfaceNameChanged);
        QString err;
        doEnableDevice(interface, m_networkConfig->deviceEnabled(interface), err);
    }
}

void NetworkThread::onDeviceRemoved(const QString &uni)
{
    Device *dev = qobject_cast<Device *>(sender());
    if (!dev) {
        qCWarning(DSM()) << "sender is not Device";
        return;
    }
    disconnect(dev, nullptr, this, nullptr);
    if (m_devices.contains(uni)) {
        m_devices.remove(uni);
    }
}

void NetworkThread::onDevicestateChanged(NetworkManager::Device::State newState, NetworkManager::Device::State oldState, NetworkManager::Device::StateChangeReason reason)
{
    Device *dev = qobject_cast<Device *>(sender());
    if (!dev) {
        qCWarning(DSM()) << "sender is not Device";
        return;
    }
    if ((oldState >= Device::Activated && reason == Device::DeviceRemovedReason) || (newState > oldState && newState == Device::Activated)) {
        restartIPWatchD();
    }
    bool enabled = m_networkConfig->deviceEnabled(dev->uni());
    if (!enabled) {
        Device::State state = dev->state();
        if (state >= Device::Preparing && state <= Device::Activated) {
            qCDebug(DSM()) << "disconnect device" << dev->uni();
            dev->disconnectInterface();
        }
    }
}

void NetworkThread::onInterfaceNameChanged()
{
    Device *dev = qobject_cast<Device *>(sender());
    if (!dev) {
        qCWarning(DSM()) << "sender is not Device";
        return;
    }
    QString uni = dev->uni();
    if (!m_devices.contains(uni)) {
        qCWarning(DSM()) << "device not exist, devPath:" << uni;
        return;
    }
    QString newIface = dev->interfaceName();
    QString oldItace = m_devices[uni];
    if (newIface.isEmpty() || newIface == "/") {
        m_networkConfig->removeDeviceEnabled(oldItace);
        QString err = m_networkConfig->saveConfig();
        if (!err.isEmpty()) {
            qCWarning(DSM()) << "save config failed, err:" << err;
        }
        return;
    }
    bool enabled = m_networkConfig->deviceEnabled(newIface);
    QString err;
    doEnableDevice(newIface, enabled, err);
}

void NetworkThread::addDevicesWithRetry()
{
    auto list = NetworkManager::networkInterfaces();
    for (auto dev : list) {
        onDeviceAdded(dev->uni());
    }
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
    NetworkManager::Device::Ptr dev = findDevice(pathOrIface);
    if (!dev) {
        err = "not found device";
        return "/";
    }
    m_networkConfig->setDeviceEnabled(dev->interfaceName(), enabled);
    m_networkConfig->saveConfig();
    Q_EMIT DeviceEnabled(QDBusObjectPath(dev->uni()), enabled);
    QString ret = "/";
    if (enabled) {
        ret = enableDevice(dev);
    } else {
        err = disableDevice(dev);
    }
    return ret;
}

void NetworkThread::disableVpn()
{
    auto activeConnectionList = NetworkManager::activeConnections();
    for (auto &&conn : activeConnectionList) {
        if (conn->vpn() && (conn->state() == NetworkManager::ActiveConnection::Activating || conn->state() == NetworkManager::ActiveConnection::Activated)) {
            NetworkManager::deactivateConnection(conn->path());
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

QString NetworkThread::enableDevice(NetworkManager::Device::Ptr device)
{
    bool enabled = NetworkManager::isNetworkingEnabled();
    if (!enabled) {
        NetworkManager::setNetworkingEnabled(true);
    }
    device->setAutoconnect(true);
    if (device->type() == NetworkManager::Device::Wifi) {
        bool wirelessEnabled = NetworkManager::isWirelessEnabled();
        if (!wirelessEnabled) {
            // 飞行模式开启，不激活wifi
            if (airplaneWifiEnabled()) {
                qCDebug(DSM()) << "disable wifi because airplane mode is on";
            } else {
                NetworkManager::setWirelessEnabled(true);
            }
        }
    }

    // device->setManaged(true); // TODO: 应该不需要
    auto connPaths = device->availableConnections();
    qCDebug(DSM()) << "available connections:" << connPaths;
    QString connPath0;
    QDateTime maxTs;
    for (auto &&connPath : connPaths) {
        auto settings = connPath->settings();
        if (!settings->autoconnect()) {
            continue;
        }
        QDateTime ts = settings->timestamp();
        if (maxTs < ts || connPath0.isEmpty()) {
            connPath0 = connPath->path();
        }
    }
    return connPath0.isEmpty() ? "/" : connPath0;
}

QString NetworkThread::disableDevice(NetworkManager::Device::Ptr device)
{
    device->setAutoconnect(false);
    // TODO:
    // cause of nm'bug, sometimes accessapoints list is nil
    // so add a judge in system network, if get nil in GetAllAccessPoints func, set wirelessEnable down.
    auto state = device->state();
    auto oldWirelessEnabled = NetworkManager::isWirelessEnabled();
    if (device->type() == NetworkManager::Device::Wifi && state > NetworkManager::Device::Unavailable) {
        NetworkManager::WirelessDevice::Ptr wDevice = device.objectCast<NetworkManager::WirelessDevice>();
        if (wDevice->mode() != WirelessDevice::ApMode) {
            auto accessPointsList = wDevice->accessPoints();
            if (accessPointsList.size() > 0) {
                m_count = 0;
                qCDebug(DSM()) << "have aplist existed!!!";
            } else if (m_count++; m_count > 2) {
                m_count = 0;
                qCInfo(DSM()) << "try to set wireless-enabled, because ap list is empty";
                NetworkManager::setWirelessEnabled(false);
                if (oldWirelessEnabled) {
                    NetworkManager::setWirelessEnabled(true);
                }
                return QString();
            }
        }
    }
    if (state >= NetworkManager::Device::Preparing && state <= NetworkManager::Device::Activated) {
        QDBusPendingReply<> reply = device->disconnectInterface();
        if (reply.isError()) {
            return reply.error().message();
        }
    }
    return QString();
}

bool NetworkThread::airplaneWifiEnabled()
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.deepin.dde.AirplaneMode1", "/org/deepin/dde/AirplaneMode1", "org.freedesktop.DBus.Properties", "Get");
    msg << "org.deepin.dde.AirplaneMode1" << "WifiEnabled";
    QDBusPendingReply<QDBusVariant> reply = QDBusConnection::systemBus().asyncCall(msg);
    reply.waitForFinished();
    if (!reply.isError()) {
        return reply.value().variant().toBool();
    } else {
        qCWarning(DSM()) << "get WifiEnabled err:" << reply.error().message();
    }
    return false;
}

void NetworkThread::restartIPWatchD()
{
    if (!QProcess::startDetached("systemctl", { "restart", "ipwatchd.service" })) {
        qCWarning(DSM()) << "restart IPWatchD failed";
    }
}

Device::Ptr NetworkThread::findDevice(QString pathOrIface)
{
    auto devs = NetworkManager::networkInterfaces();
    for (auto &&dev : devs) {
        if (dev->interfaceName() == pathOrIface || dev->uni() == pathOrIface) {
            return dev;
        }
    }
    qCWarning(DSM()) << "cant find device, pathOrIface: " << pathOrIface;
    return Device::Ptr();
}

} // namespace systemservice
} // namespace network
