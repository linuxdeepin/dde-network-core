// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpndnsmodehandler.h"
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Device>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QThread>
#include <net/if.h>
#include "constants.h"


using namespace network::sessionservice;

namespace {
constexpr const char *const NMService = "org.freedesktop.NetworkManager";
constexpr const char *const NMConnActiveInterface = "org.freedesktop.NetworkManager.Connection.Active";

QString getActiveConnectionIp4Config(const QString &acPath)
{
    QDBusInterface iface(NMService, acPath, NMConnActiveInterface, QDBusConnection::systemBus());
    return iface.property("Ip4Config").value<QDBusObjectPath>().path();
}

QList<QHostAddress> collectDnsFromDevice(const NetworkManager::Device::Ptr &dev)
{
    QList<QHostAddress> addrList;
    for (const QHostAddress &addr : dev->ipV4Config().nameservers()) {
        if (!addr.isNull())
            addrList << addr;
    }

    for (const QHostAddress &addr : dev->ipV6Config().nameservers()) {
        if (!addr.isNull())
            addrList << addr;
    }

    return addrList;
}

struct VpnDnsModeApplyTarget
{
    int ifindex = 0;
    QList<QHostAddress> dnsServers;
};

bool findDnsModeApplyTarget(const NetworkManager::ActiveConnection::Ptr &vpnAc, int dnsPriority,
                            VpnDnsModeApplyTarget *target)
{
    if (vpnAc.isNull() || vpnAc->connection().isNull() || !target)
        return false;

    const QString vpnIp4ConfigPath = getActiveConnectionIp4Config(vpnAc->path());
    if (vpnIp4ConfigPath.isEmpty() || vpnIp4ConfigPath == "/") {
        qCDebug(DSM()) << "[DNS-TRACE] VPN AC has no Ip4Config:" << vpnAc->id();
        return false;
    }

    const NetworkManager::ActiveConnection::List allActiveConnections = NetworkManager::activeConnections();
    for (const NetworkManager::ActiveConnection::Ptr &tunAc : allActiveConnections) {
        if (tunAc.isNull() || tunAc->connection().isNull() || tunAc == vpnAc)
            continue;

        if (tunAc->connection()->settings()->connectionType() != NetworkManager::ConnectionSettings::ConnectionType::Tun)
            continue;

        const QString tunIp4ConfigPath = getActiveConnectionIp4Config(tunAc->path());
        if (tunIp4ConfigPath != vpnIp4ConfigPath)
            continue;

        for (const QString &devPath : tunAc->devices()) {
            NetworkManager::Device::Ptr dev = NetworkManager::findNetworkInterface(devPath);
            if (dev.isNull())
                continue;

            if (dev->type() != NetworkManager::Device::Type::Tun &&
                dev->type() != NetworkManager::Device::Type::Generic &&
                dev->type() != NetworkManager::Device::Type::IpTunnel)
                continue;

            const int ifindex = static_cast<int>(if_nametoindex(dev->interfaceName().toStdString().c_str()));
            if (ifindex <= 0)
                continue;

            QList<QHostAddress> dnsList;
            if (dnsPriority != 0) {
                dnsList = collectDnsFromDevice(dev);
                if (dnsList.isEmpty())
                    continue;
            }

            target->ifindex = ifindex;
            target->dnsServers = dnsList;
            return true;
        }
    }
    qCDebug(DSM()) << "[DNS-TRACE] No matching Tun AC found for VPN:" << vpnAc->id();
    return false;
}
} // namespace

VpnDnsModeHandler::VpnDnsModeHandler(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QHostAddress>("QHostAddress");
    qRegisterMetaType<QList<QHostAddress>>("QList<QHostAddress>");
    init();
}

VpnDnsModeHandler::~VpnDnsModeHandler() 
{
    disconnect(this, nullptr, m_worker, nullptr);
    disconnect(m_worker, nullptr, this, nullptr);

    if (m_workerThread && m_workerThread->isRunning()) {
        Q_ASSERT(QThread::currentThread() != m_workerThread);
        if (m_worker) {
            QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);
        }

        m_workerThread->quit();
        m_workerThread->wait();
        m_worker = nullptr;
    }

    if (m_workerThread) {
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

void VpnDnsModeHandler::ensureWorkerThread()
{
    if (m_workerThread)
        return;

    m_workerThread = new QThread();
    m_worker = new VpnDnsModeWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &VpnDnsModeHandler::requestApplyDnsMode, m_worker, &VpnDnsModeWorker::onApplyDnsMode);
    connect(this, &VpnDnsModeHandler::requestRemoveConnection, m_worker, &VpnDnsModeWorker::removeConnection);
    connect(this, &VpnDnsModeHandler::requestRemoveActiveConnection,
            m_worker, &VpnDnsModeWorker::removeActiveConnection);
    connect(m_worker, &VpnDnsModeWorker::applyFinished, this, &VpnDnsModeHandler::onApplyFinished);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_workerThread->start();
}

void VpnDnsModeHandler::init()
{
    initNetworkConnections();
    initActiveConnections();
}

void VpnDnsModeHandler::initNetworkConnections()
{
    const auto connections = NetworkManager::listConnections();
    for (const auto &connection : connections) {
        if (connection->settings()->connectionType() != NetworkManager::ConnectionSettings::ConnectionType::Vpn) {
            continue;
        }

        connectConnectionSignals(connection);
    }

    connect(NetworkManager::settingsNotifier(), &NetworkManager::SettingsNotifier::connectionAdded,
            this, &VpnDnsModeHandler::onConnectionAdded);
    connect(NetworkManager::settingsNotifier(), &NetworkManager::SettingsNotifier::connectionRemoved,
            this, &VpnDnsModeHandler::onConnectionRemoved);
}

void VpnDnsModeHandler::initActiveConnections()
{
    const auto activeConns = NetworkManager::activeConnections();
    for (const auto &activeConn : activeConns)
        trackVpnActiveConnection(activeConn);

    connect(NetworkManager::notifier(), &NetworkManager::Notifier::activeConnectionAdded,
            this, &VpnDnsModeHandler::onActiveConnectionAdded);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::activeConnectionRemoved,
            this, &VpnDnsModeHandler::onActiveConnectionRemoved);
}

void VpnDnsModeHandler::connectConnectionSignals(const NetworkManager::Connection::Ptr &connection)
{
    if (connection.isNull() || m_vpnConnections.contains(connection->path()))
        return;

    m_vpnConnections[connection->path()] = connection;
    connect(connection.data(), &NetworkManager::Connection::updated, this, &VpnDnsModeHandler::onConnectionUpdated);
}

void VpnDnsModeHandler::trackVpnActiveConnection(const NetworkManager::ActiveConnection::Ptr &activeConnection)
{
    if (activeConnection.isNull() || activeConnection->connection().isNull()
        || activeConnection->connection()->settings()->connectionType()
            != NetworkManager::ConnectionSettings::ConnectionType::Vpn) {
        return;
    }

    m_activeVpnConnections[activeConnection->path()] = activeConnection->connection()->path();
    connect(activeConnection.data(), &NetworkManager::ActiveConnection::ipV4ConfigChanged,
            this, &VpnDnsModeHandler::onVpnIp4ConfigChanged, Qt::UniqueConnection);
}

void VpnDnsModeHandler::onConnectionAdded(const QString &path)
{
    NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(path);
    if (connection.isNull()
        || connection->settings()->connectionType() != NetworkManager::ConnectionSettings::ConnectionType::Vpn) {
        return;
    }

    connectConnectionSignals(connection);
}

void VpnDnsModeHandler::onConnectionRemoved(const QString &path)
{
    m_vpnConnections.remove(path);
    m_applyStates.remove(path);
    for (auto it = m_activeVpnConnections.begin(); it != m_activeVpnConnections.end();) {
        if (it.value() == path)
            it = m_activeVpnConnections.erase(it);
        else
            ++it;
    }
    if (m_worker)
        Q_EMIT requestRemoveConnection(path);
}

void VpnDnsModeHandler::onConnectionUpdated()
{
    auto *connection = qobject_cast<NetworkManager::Connection *>(sender());
    if (!connection) {
        return;
    }

    auto activeConn = findVpnActiveConnection(connection->path());
    if (!activeConn.isNull()) {
        requestApplyDnsModeIfChanged(activeConn, true);
    }
}     

void VpnDnsModeHandler::onActiveConnectionAdded(const QString &path)
{
    trackVpnActiveConnection(NetworkManager::findActiveConnection(path));
}

void VpnDnsModeHandler::onActiveConnectionRemoved(const QString &path)
{
    if (m_activeVpnConnections.remove(path) == 0)
        return;

    if (m_worker)
        Q_EMIT requestRemoveActiveConnection(path);
}

void VpnDnsModeHandler::onVpnIp4ConfigChanged()
{
    auto *activeConn = qobject_cast<NetworkManager::ActiveConnection *>(sender());
    if (!activeConn) {
        return;
    }

    NetworkManager::ActiveConnection::Ptr activeConnPtr = NetworkManager::findActiveConnection(activeConn->path());
    if (activeConnPtr.isNull()
        || activeConnPtr->state() != NetworkManager::ActiveConnection::State::Activated
        || activeConnPtr->connection().isNull()) {
        return;
    }
    requestApplyDnsModeIfChanged(activeConnPtr, false);
}

NetworkManager::ActiveConnection::Ptr VpnDnsModeHandler::findVpnActiveConnection(const QString &connectionPath) const
{
    const auto activeConns = NetworkManager::activeConnections();
    for (const auto &activeConn : activeConns) {
        if (!activeConn.isNull()
            && !activeConn->connection().isNull()
            && activeConn->connection()->path() == connectionPath) {
            return activeConn;
        }
    }
    return NetworkManager::ActiveConnection::Ptr();
}

void VpnDnsModeHandler::requestApplyDnsModeIfChanged(const NetworkManager::ActiveConnection::Ptr &vpnAc, bool isCompare)
{
    if (vpnAc.isNull() || vpnAc->connection().isNull())
        return;

    const auto conn = vpnAc->connection();
    const QString connPath = conn->path();

    NetworkManager::ConnectionSettings::Ptr settings = conn->settings();
    NetworkManager::Setting::Ptr ipv4Setting = settings->setting(NetworkManager::Setting::Ipv4);
    auto ipv4 = ipv4Setting.dynamicCast<NetworkManager::Ipv4Setting>();

    int dnsPriority = 0;
    if (ipv4 && ipv4->neverDefault()) {
        dnsPriority = ipv4->dnsPriority();
    }

    auto &state = m_applyStates[connPath];
    if (state.isRolledBack()) {
        if (dnsPriority == 0)
            return;
        state.clearRolledBack();
    }

    if (isCompare && state.hasAppliedPriority() && dnsPriority == state.appliedPriority()) {
        qCDebug(DSM()) << "[DNS-TRACE] dns-priority unchanged, skipping apply for:" << connPath
                         << "dnsPriority:" << dnsPriority;
        return;
    }

    VpnDnsModeApplyTarget target;
    if (!findDnsModeApplyTarget(vpnAc, dnsPriority, &target)) {
        return;
    }

    const quint64 generation = state.beginRequest();
    ensureWorkerThread();
    Q_EMIT requestApplyDnsMode(connPath, vpnAc->path(), generation,
                               target.ifindex, target.dnsServers, dnsPriority);
}

void VpnDnsModeHandler::onApplyFinished(const QString &connectionPath, quint64 generation, int dnsPriority,
                                        VpnDnsApplyResult result)
{
    auto stateIt = m_applyStates.find(connectionPath);
    if (stateIt == m_applyStates.end())
        return;

    auto &state = stateIt.value();
    switch (result) {
    case VpnDnsApplyResult::Success:
        state.commitApplied(generation, dnsPriority);
        return;
    case VpnDnsApplyResult::AuthorizationDenied:
    case VpnDnsApplyResult::AuthorizationError:
    case VpnDnsApplyResult::DbusError:
    case VpnDnsApplyResult::ServiceUnavailable: {
        // 只允许最新请求触发回滚；若已有更新的请求入队，过期失败结果必须被丢弃。
        const int target = state.hasAppliedPriority() ? state.appliedPriority() : 0;
        if (!state.rollbackRequest(generation, target))
            return;
        rollbackConnectionDnsPriority(connectionPath, target);
        return;
    }
    }
}

void VpnDnsModeHandler::rollbackConnectionDnsPriority(const QString &connectionPath, int target)
{
    NetworkManager::Connection::Ptr connection = NetworkManager::findConnection(connectionPath);
    if (connection.isNull()) {
        qCWarning(DSM()) << "[DNS-TRACE] DNS mode rollback skipped, connection gone:"
                         << connectionPath;
        return;
    }

    NMVariantMapMap settingsMap = connection->settings()->toMap();
    bool changed = false;
    auto it = settingsMap.find(QStringLiteral("ipv4"));
    if (it != settingsMap.end()) {
        QVariantMap ipv4Map = it.value();
        if (ipv4Map.value(QStringLiteral("dns-priority"), 0).toInt() != target) {
            ipv4Map[QStringLiteral("dns-priority")] = target;
            it.value() = ipv4Map;
            changed = true;
        }
    }
    it = settingsMap.find(QStringLiteral("ipv6"));
    if (it != settingsMap.end()) {
        QVariantMap ipv6Map = it.value();
        if (ipv6Map.value(QStringLiteral("dns-priority"), 0).toInt() != target) {
            ipv6Map[QStringLiteral("dns-priority")] = target;
            it.value() = ipv6Map;
            changed = true;
        }
    }

    if (!changed)
        return;

    const QDBusPendingReply<> reply = connection->update(settingsMap);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *w) {
                if (w->isError()) {
                    qCWarning(DSM()) << "[DNS-TRACE] DNS mode rollback update failed:"
                                     << w->error().message();
                }
                w->deleteLater();
            });
}
