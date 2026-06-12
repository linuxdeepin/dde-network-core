// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpndnsroutecontroller.h"

#include <QThread>
#include <QList>
#include <QHostAddress>
#include <QDBusInterface>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <net/if.h>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <QCoreApplication>
#include <PolkitQt1/Authority>
#include <PolkitQt1/Subject>
#include "networkconst.h"

using namespace dde::network;

namespace {
constexpr const char *const ResolvedService = "org.freedesktop.resolve1";
constexpr const char *const ResolvedPath = "/org/freedesktop/resolve1";
constexpr const char *const ResolvedInterface = "org.freedesktop.resolve1.Manager";
constexpr const char *const NMService = "org.freedesktop.NetworkManager";
constexpr const char *const NMConnActiveInterface = "org.freedesktop.NetworkManager.Connection.Active";
constexpr const char *const NMSettingsConnInterface = "org.freedesktop.NetworkManager.Settings.Connection";

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

QByteArray packIpv4(quint32 ipv4)
{
    QByteArray bytes(4, '\0');
    bytes[0] = static_cast<char>((ipv4 >> 24) & 0xFF);
    bytes[1] = static_cast<char>((ipv4 >> 16) & 0xFF);
    bytes[2] = static_cast<char>((ipv4 >> 8) & 0xFF);
    bytes[3] = static_cast<char>(ipv4 & 0xFF);
    return bytes;
}

QByteArray packIpv6(const Q_IPV6ADDR &ipv6)
{
    return QByteArray(reinterpret_cast<const char *>(ipv6.c), sizeof(ipv6.c));
}

int readConnectionDnsPriority(const QString &connectionPath)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(
        NMService, connectionPath, NMSettingsConnInterface, "GetSettings");
    QDBusPendingReply<NMVariantMapMap> reply = QDBusConnection::systemBus().call(msg, QDBus::Block, 500);
    if (reply.isError() || !reply.value().contains("ipv4"))
        return 0;

    return reply.value()["ipv4"].value("dns-priority", 0).toInt();
}

} // namespace

using DnsEntry = QPair<qint32, QByteArray>;
using DomainEntry = QPair<QString, bool>;

static QDBusArgument &operator<<(QDBusArgument &arg, const DnsEntry &entry)
{
    arg.beginStructure();
    arg << entry.first << entry.second;
    arg.endStructure();
    return arg;
}

static const QDBusArgument &operator>>(const QDBusArgument &arg, DnsEntry &entry)
{
    arg.beginStructure();
    arg >> entry.first >> entry.second;
    arg.endStructure();
    return arg;
}

static QDBusArgument &operator<<(QDBusArgument &arg, const DomainEntry &entry)
{
    arg.beginStructure();
    arg << entry.first << entry.second;
    arg.endStructure();
    return arg;
}

static const QDBusArgument &operator>>(const QDBusArgument &arg, DomainEntry &entry)
{
    arg.beginStructure();
    arg >> entry.first >> entry.second;
    arg.endStructure();
    return arg;
}

static bool s_typesRegistered = false;

void VpnDnsRouteWorker::registerMetaTypesOnce()
{
    if (s_typesRegistered)
        return;

    qRegisterMetaType<DnsEntry>("DnsEntry");
    qDBusRegisterMetaType<DnsEntry>();
    qRegisterMetaType<QList<DnsEntry>>("QList<DnsEntry>");
    qDBusRegisterMetaType<QList<DnsEntry>>();

    qRegisterMetaType<DomainEntry>("DomainEntry");
    qDBusRegisterMetaType<DomainEntry>();
    qRegisterMetaType<QList<DomainEntry>>("QList<DomainEntry>");
    qDBusRegisterMetaType<QList<DomainEntry>>();

    s_typesRegistered = true;
}


// ========== VpnDnsRouteWorker ==========

VpnDnsRouteWorker::VpnDnsRouteWorker(QObject *parent)
    : QObject(parent)
{
}

VpnDnsRouteWorker::~VpnDnsRouteWorker()
{
}

void VpnDnsRouteWorker::ensureResolvedInterface()
{
    if (!m_resolvedInterface) {
        m_resolvedInterface = new QDBusInterface(ResolvedService, ResolvedPath, ResolvedInterface, QDBusConnection::systemBus(), this);
    }
}

bool VpnDnsRouteWorker::checkPolkitAuthorization()
{
    PolkitQt1::Authority *authority = PolkitQt1::Authority::instance();
    if (!authority) {
        qCWarning(DNC) << "[POLKIT] Failed to get PolkitQt1::Authority instance";
        return false;
    }

    if (authority->hasError()) {
        qCDebug(DNC) << "[POLKIT] Clearing previous authority error:" << authority->errorDetails();
        authority->clearError();
    }

    PolkitQt1::UnixProcessSubject subject(QCoreApplication::applicationPid());
    PolkitQt1::Authority::Result result = authority->checkAuthorizationSync(
        QStringLiteral("com.deepin.dde.network.configure-dns"),
        subject,
        PolkitQt1::Authority::AllowUserInteraction);

    if (authority->hasError()) {
        qCWarning(DNC) << "[POLKIT] Authorization check error:" << authority->errorDetails();
        authority->clearError();
        return false;
    }

    return result == PolkitQt1::Authority::Yes;
}

VpnDnsMode VpnDnsRouteWorker::getVpnDnsModeFromConnection(const QString &connectionPath) const
{
    int dp = readConnectionDnsPriority(connectionPath);
    if (dp < 0) {
        return VpnDnsMode::VpnDnsModePreferred;
    } else if (dp > 0) {
        return VpnDnsMode::VpnDnsModeSecondary;
    }
    return VpnDnsMode::VpnDnsModeNotSet;
}

void VpnDnsRouteWorker::onCheckAndApplyDnsModeIfChanged(const QString &vpnConnectionPath, bool isCompare)
{
    int currentPriority = readConnectionDnsPriority(vpnConnectionPath);

    if (!m_lastDnsPriority.contains(vpnConnectionPath)) {
        m_lastDnsPriority[vpnConnectionPath] = currentPriority;
        qCDebug(DNC) << "[DNS-TRACE] onCheckAndApplyDnsModeIfChanged: first time caching dns-priority:"
                     << currentPriority << "for:" << vpnConnectionPath;
        return;
    }

    int lastPriority = m_lastDnsPriority.value(vpnConnectionPath);
    if (isCompare && currentPriority == lastPriority) {
        qCDebug(DNC) << "[DNS-TRACE] onCheckAndApplyDnsModeIfChanged: dns-priority unchanged,"
                     << "skipping apply for:" << vpnConnectionPath;
        return;
    }

    m_lastDnsPriority[vpnConnectionPath] = currentPriority;

    NetworkManager::ActiveConnection::List allActiveConns = NetworkManager::activeConnections();
    for (const auto &ac : allActiveConns) {
        if (!ac.isNull() && !ac->connection().isNull()
            && ac->connection()->path() == vpnConnectionPath) {
            onApplyDnsModeForVpnAc(ac);
            break;
        }
    }
}

void VpnDnsRouteWorker::onCleanupConnection(const QString &connectionPath)
{
    m_lastDnsPriority.remove(connectionPath);
    qCDebug(DNC) << "[DNS-TRACE] cleaned up dns-priority cache for:" << connectionPath;
}

void VpnDnsRouteWorker::onApplyDnsModeForVpnAc(const NetworkManager::ActiveConnection::Ptr &vpnAc)
{
    if (vpnAc.isNull() || vpnAc->connection().isNull()) {
        return;
    }

    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.freedesktop.resolve1")) {
        qCDebug(DNC) << "[DNS-TRACE] systemd-resolved is not available, skipping DNS route";
        return;
    }

    const QString vpnIp4ConfigPath = getActiveConnectionIp4Config(vpnAc->path());
    if (vpnIp4ConfigPath.isEmpty() || vpnIp4ConfigPath == "/") {
        qCDebug(DNC) << "[DNS-TRACE] VPN AC has no Ip4Config:" << vpnAc->id();
        return;
    }

    NetworkManager::ActiveConnection::List allActiveConnections = NetworkManager::activeConnections();
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

            int ifindex = static_cast<int>(if_nametoindex(dev->interfaceName().toStdString().c_str()));
            if (ifindex <= 0)
                continue;

            const QList<QHostAddress> dnsList = collectDnsFromDevice(dev);
            if (dnsList.isEmpty())
                continue;

            VpnDnsMode dnsMode = getVpnDnsModeFromConnection(vpnAc->connection()->path());
            ensureResolvedInterface();
            if (!checkPolkitAuthorization()) {
                qCDebug(DNC) << "[DNS-TRACE] Polkit authorization denied, skipping DNS route configuration";
                return;
            }
            
            switch (dnsMode) {
            case VpnDnsMode::VpnDnsModePreferred:
                applyPreferredDnsMode(ifindex, dnsList);
                break;
            case VpnDnsMode::VpnDnsModeSecondary:
                applySecondaryDnsMode(ifindex, dnsList);
                break;
            case VpnDnsMode::VpnDnsModeNotSet:
            default:
                applyNotSetDnsMode(ifindex);
                break;
            }
            return;
        }
    }
    qCDebug(DNC) << "[DNS-TRACE] No matching Tun AC found for VPN:" << vpnAc->id();
}

bool VpnDnsRouteWorker::applyPreferredDnsMode(int ifindex, const QList<QHostAddress> &dnsServers)
{
    if (ifindex <= 0) {
        qCWarning(DNC) << "Invalid ifindex:" << ifindex;
        return false;
    }

    qCDebug(DNC) << "[DNS-TRACE] Applying Preferred DNS route for ifindex" << ifindex << "dns:" << dnsServers;
    return setLinkDns(ifindex, dnsServers)
           && setLinkDomains(ifindex, {"."}, {true})
           && setLinkDefaultRoute(ifindex, false);
}

bool VpnDnsRouteWorker::applySecondaryDnsMode(int ifindex, const QList<QHostAddress> &dnsServers)
{
    if (ifindex <= 0) {
        qCWarning(DNC) << "Invalid ifindex:" << ifindex;
        return false;
    }

    qCDebug(DNC) << "[DNS-TRACE] Applying Secondary DNS route for ifindex" << ifindex << "dns:" << dnsServers;
    return setLinkDns(ifindex, dnsServers)
           && setLinkDomains(ifindex, {}, {})
           && setLinkDefaultRoute(ifindex, true);
}

bool VpnDnsRouteWorker::applyNotSetDnsMode(int ifindex)
{
    if (ifindex <= 0) {
        qCWarning(DNC) << "Invalid ifindex:" << ifindex;
        return false;
    }

    qCDebug(DNC) << "[DNS-TRACE] Applying Not Set DNS route (RevertLink) for ifindex" << ifindex;
    return revertLink(ifindex);
}

bool VpnDnsRouteWorker::setLinkDns(int ifindex, const QList<QHostAddress> &dnsServers)
{
    registerMetaTypesOnce();

    QList<DnsEntry> dnsEntries;
    for (const QHostAddress &addr : dnsServers) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            dnsEntries.append({2, packIpv4(addr.toIPv4Address())});
        } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            dnsEntries.append({10, packIpv6(addr.toIPv6Address())});
        }
    }

    QDBusPendingCall pending = m_resolvedInterface->asyncCall("SetLinkDNS", ifindex, QVariant::fromValue(dnsEntries));
    pending.waitForFinished();
    if (pending.isError()) {
        qCWarning(DNC) << "[DNS-TRACE] SetLinkDNS failed:" << pending.error().message();
        return false;
    }
    return true;
}

bool VpnDnsRouteWorker::setLinkDomains(int ifindex, const QStringList &domains, const QList<bool> &routes)
{
    registerMetaTypesOnce();

    QList<DomainEntry> domainEntries;
    for (int i = 0; i < domains.size() && i < routes.size(); ++i) {
        domainEntries.append({domains[i], routes[i]});
    }

    QDBusPendingCall pending = m_resolvedInterface->asyncCall("SetLinkDomains", ifindex, QVariant::fromValue(domainEntries));
    pending.waitForFinished();
    if (pending.isError()) {
        qCWarning(DNC) << "[DNS-TRACE] SetLinkDomains failed:" << pending.error().message();
        return false;
    }
    return true;
}

bool VpnDnsRouteWorker::setLinkDefaultRoute(int ifindex, bool enable)
{
    QDBusPendingCall pending = m_resolvedInterface->asyncCall("SetLinkDefaultRoute", ifindex, enable);
    pending.waitForFinished();
    if (pending.isError()) {
        qCWarning(DNC) << "[DNS-TRACE] SetLinkDefaultRoute failed:" << pending.error().message();
        return false;
    }
    return true;
}

bool VpnDnsRouteWorker::revertLink(int ifindex)
{
    QDBusPendingCall pending = m_resolvedInterface->asyncCall("RevertLink", ifindex);
    pending.waitForFinished();
    if (pending.isError()) {
        qCWarning(DNC) << "[DNS-TRACE] RevertLink failed:" << pending.error().message();
        return false;
    }
    return true;
}


// ========== VpnDnsRouteController ==========

VpnDnsRouteController::VpnDnsRouteController(QObject *parent)
    : QObject(parent)
{
    m_workerThread = new QThread();
    m_worker = new VpnDnsRouteWorker();
    m_worker->moveToThread(m_workerThread);

    connect(this, &VpnDnsRouteController::requestCheckAndApplyDnsModeIfChanged,
            m_worker, &VpnDnsRouteWorker::onCheckAndApplyDnsModeIfChanged);
    connect(this, &VpnDnsRouteController::requestCleanupConnection,
            m_worker, &VpnDnsRouteWorker::onCleanupConnection);

    // 正常退出路径：quit + wait → Worker 在子线程事件循环退出前被 deleteLater 删除
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater, Qt::QueuedConnection);

    m_workerThread->start();
}

VpnDnsRouteController::~VpnDnsRouteController()
{
    if (!m_workerThread)
        return;

    disconnect(this, nullptr, m_worker, nullptr);

    if (m_workerThread && m_workerThread->isRunning()) {
        m_workerThread->quit();
        
        if (m_workerThread->wait(3000)) {
            m_worker = nullptr;
        } else {
            m_workerThread->terminate();
            m_workerThread->wait(500);
            if (m_worker)
            {
                delete m_worker;
                m_worker = nullptr;
            }
        }
    }

    if (m_workerThread) {
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

void VpnDnsRouteController::requestApplyDnsModeIfChanged(const NetworkManager::ActiveConnection::Ptr &vpnAc, bool isCompare)
{
    if (vpnAc.isNull() || vpnAc->connection().isNull())
        return;

    const QString connPath = vpnAc->connection()->path();
    qCDebug(DNC) << "[DNS-TRACE] requestApplyDnsModeIfChanged delegating to worker thread for:" << connPath;
    Q_EMIT requestCheckAndApplyDnsModeIfChanged(connPath, isCompare);
}

void VpnDnsRouteController::cleanupConnection(const QString &connectionPath)
{
    Q_EMIT requestCleanupConnection(connectionPath);
}
