// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpndnsmodeworker.h"

#include "constants.h"

#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QHostAddress>
#include <QList>
#include <mutex>
#include <unistd.h>

using namespace network::sessionservice;

namespace {
constexpr const char *const ResolvedService = "org.freedesktop.resolve1";
constexpr const char *const ResolvedPath = "/org/freedesktop/resolve1";
constexpr const char *const ResolvedInterface = "org.freedesktop.resolve1.Manager";

constexpr const char *const PolkitService = "org.freedesktop.PolicyKit1";
constexpr const char *const PolkitPath = "/org/freedesktop/PolicyKit1/Authority";
constexpr const char *const PolkitInterface = "org.freedesktop.PolicyKit1.Authority";
constexpr const char *const PolkitDnsAction = "com.deepin.dde.network.configure-dns";
constexpr quint32 PolkitFlagAllowUserInteraction = 0x01;

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

VpnDnsMode vpnDnsModeFromPriority(int priority)
{
    if (priority < 0)
        return VpnDnsMode::VpnDnsModePreferred;
    if (priority > 0)
        return VpnDnsMode::VpnDnsModeSecondary;
    return VpnDnsMode::VpnDnsModeNotSet;
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

struct PolkitSubject
{
    QString kind;
    QMap<QString, QDBusVariant> properties;
};

static QDBusArgument &operator<<(QDBusArgument &arg, const PolkitSubject &subject)
{
    arg.beginStructure();
    arg << subject.kind << subject.properties;
    arg.endStructure();
    return arg;
}

static const QDBusArgument &operator>>(const QDBusArgument &arg, PolkitSubject &subject)
{
    arg.beginStructure();
    arg >> subject.kind >> subject.properties;
    arg.endStructure();
    return arg;
}

struct PolkitCheckResult
{
    bool isAuthorized = false;
    bool isChallenge = false;
    QMap<QString, QString> details;
};

static QDBusArgument &operator<<(QDBusArgument &arg, const PolkitCheckResult &result)
{
    arg.beginStructure();
    arg << result.isAuthorized << result.isChallenge << result.details;
    arg.endStructure();
    return arg;
}

static const QDBusArgument &operator>>(const QDBusArgument &arg, PolkitCheckResult &result)
{
    arg.beginStructure();
    arg >> result.isAuthorized >> result.isChallenge >> result.details;
    arg.endStructure();
    return arg;
}

void VpnDnsModeWorker::registerMetaTypesOnce()
{
    static std::once_flag s_typesRegisteredFlag;
    std::call_once(s_typesRegisteredFlag, []() {
        qRegisterMetaType<DnsEntry>("DnsEntry");
        qDBusRegisterMetaType<DnsEntry>();
        qRegisterMetaType<QList<DnsEntry>>("QList<DnsEntry>");
        qDBusRegisterMetaType<QList<DnsEntry>>();

        qRegisterMetaType<DomainEntry>("DomainEntry");
        qDBusRegisterMetaType<DomainEntry>();
        qRegisterMetaType<QList<DomainEntry>>("QList<DomainEntry>");
        qDBusRegisterMetaType<QList<DomainEntry>>();

        qDBusRegisterMetaType<PolkitSubject>();
        qDBusRegisterMetaType<PolkitCheckResult>();
        qDBusRegisterMetaType<QMap<QString, QString>>();
    });
}

VpnDnsModeWorker::VpnDnsModeWorker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<VpnDnsApplyResult>();
}

VpnDnsModeWorker::~VpnDnsModeWorker()
{
    stop();
}

void VpnDnsModeWorker::ensureResolvedInterface()
{
    if (m_resolvedInterface && m_resolvedInterface->isValid())
        return;

    m_resolvedInterface = new QDBusInterface(
        ResolvedService, ResolvedPath, ResolvedInterface,
        QDBusConnection::systemBus(), this);
}

void VpnDnsModeWorker::onApplyDnsMode(const QString &connectionPath,
                                      const QString &activeConnectionPath, quint64 generation,
                                      int ifindex, const QList<QHostAddress> &dnsServers,
                                      int dnsPriority)
{
    if (m_stopping)
        return;

    VpnDnsModeRequest request;
    request.connectionPath = connectionPath;
    request.activeConnectionPath = activeConnectionPath;
    request.generation = generation;
    request.ifindex = ifindex;
    request.dnsServers = dnsServers;
    request.dnsPriority = dnsPriority;

    if (!m_requestQueue.enqueue(request))
        return;

    startNextRequest();
}

void VpnDnsModeWorker::removeConnection(const QString &connectionPath)
{
    if (m_stopping)
        return;

    m_requestQueue.removeConnection(connectionPath);
    if (!m_hasActiveRequest || m_activeRequest.connectionPath != connectionPath)
        return;

    m_activeRequestInvalidated = true;
    if (m_polkitWatcher || m_pendingCallWatcher)
        return;

    finishActiveRequest(VpnDnsApplyResult::DbusError, false);
}

void VpnDnsModeWorker::removeActiveConnection(const QString &activeConnectionPath)
{
    if (m_stopping)
        return;

    m_requestQueue.removeActiveConnection(activeConnectionPath);
    if (!m_hasActiveRequest
        || m_activeRequest.activeConnectionPath != activeConnectionPath) {
        return;
    }

    m_activeRequestInvalidated = true;
    if (m_polkitWatcher || m_pendingCallWatcher)
        return;

    finishActiveRequest(VpnDnsApplyResult::DbusError, false);
}

void VpnDnsModeWorker::startNextRequest()
{
    if (m_stopping || m_hasActiveRequest)
        return;

    if (!m_requestQueue.takeNext(&m_activeRequest))
        return;

    m_hasActiveRequest = true;
    m_activeRequestInvalidated = false;
    m_applyTransaction.reset();
    ensureResolvedInterface();
    if (!m_resolvedInterface->isValid()) {
        finishActiveRequest(VpnDnsApplyResult::ServiceUnavailable);
        return;
    }

    startAuthorization();
}

// 本插件由 deepin-service-manager 的「用户会话实例」加载，以桌面用户身份常驻运行而非 root；
// polkit 主体即本进程（unix-process，uid = getuid() = 桌面用户），未授权时经 dde-polkit-agent 弹出认证框；
void VpnDnsModeWorker::startAuthorization()
{
    if (m_stopping || !m_hasActiveRequest || m_polkitWatcher)
        return;

    registerMetaTypesOnce();

    PolkitSubject subject;
    subject.kind = QStringLiteral("unix-process");
    subject.properties.insert(QStringLiteral("pid"),
                              QDBusVariant(QVariant::fromValue(static_cast<quint32>(QCoreApplication::applicationPid()))));
    // start-time 传 0：polkitd 会在构造 subject 时自行从 /proc/<pid>/stat 解析真实启动时刻（并优先用 pidfd 防 PID 复用）。
    subject.properties.insert(QStringLiteral("start-time"),
                              QDBusVariant(QVariant::fromValue(quint64(0))));
    subject.properties.insert(QStringLiteral("uid"),
                              QDBusVariant(QVariant::fromValue(static_cast<qint32>(getuid()))));

    QDBusMessage msg = QDBusMessage::createMethodCall(
        QString::fromLatin1(PolkitService), QString::fromLatin1(PolkitPath),
        QString::fromLatin1(PolkitInterface), QStringLiteral("CheckAuthorization"));

    msg << QVariant::fromValue(subject)
        << QString::fromLatin1(PolkitDnsAction)
        << QVariant::fromValue(QMap<QString, QString>())
        << QVariant::fromValue(PolkitFlagAllowUserInteraction)
        << QVariant(QString());

    m_polkitWatcher = new QDBusPendingCallWatcher(
        QDBusConnection::systemBus().asyncCall(msg), this);
    connect(m_polkitWatcher, &QDBusPendingCallWatcher::finished,
            this, &VpnDnsModeWorker::onPolkitCheckFinished);
}

void VpnDnsModeWorker::onPolkitCheckFinished(QDBusPendingCallWatcher *watcher)
{
    if (!watcher)
        return;

    if (watcher != m_polkitWatcher) {
        watcher->deleteLater();
        return;
    }
    m_polkitWatcher = nullptr;

    if (m_stopping || !m_hasActiveRequest) {
        watcher->deleteLater();
        return;
    }

    if (m_activeRequestInvalidated) {
        watcher->deleteLater();
        finishActiveRequest(VpnDnsApplyResult::DbusError, false);
        return;
    }

    if (watcher->isError()) {
        const QDBusError error = watcher->error();
        watcher->deleteLater();
        qCWarning(DSM()) << "[POLKIT] CheckAuthorization failed:" << error.name()
                         << error.message();
        finishActiveRequest(VpnDnsApplyResult::AuthorizationError);
        return;
    }

    const QDBusPendingReply<PolkitCheckResult> reply = *watcher;
    watcher->deleteLater();

    if (reply.argumentAt<0>().isAuthorized) {
        startDnsModeApply();
        return;
    }

    finishActiveRequest(VpnDnsApplyResult::AuthorizationDenied);
}

void VpnDnsModeWorker::startDnsModeApply()
{
    if (m_stopping || !m_hasActiveRequest)
        return;

    if (m_activeRequestInvalidated) {
        finishActiveRequest(VpnDnsApplyResult::DbusError, false);
        return;
    }

    if (m_activeRequest.ifindex <= 0) {
        qCWarning(DSM()) << "Invalid ifindex:" << m_activeRequest.ifindex;
        finishActiveRequest(VpnDnsApplyResult::DbusError);
        return;
    }

    if (vpnDnsModeFromPriority(m_activeRequest.dnsPriority) == VpnDnsMode::VpnDnsModeNotSet) {
        startRevertLink();
        return;
    }

    startSetLinkDns();
}

void VpnDnsModeWorker::startSetLinkDns()
{
    registerMetaTypesOnce();

    QList<DnsEntry> dnsEntries;
    for (const QHostAddress &addr : m_activeRequest.dnsServers) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            dnsEntries.append({2, packIpv4(addr.toIPv4Address())});
        } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            dnsEntries.append({10, packIpv6(addr.toIPv6Address())});
        }
    }

    watchResolvedCall(
        m_resolvedInterface->asyncCall(
            "SetLinkDNS", m_activeRequest.ifindex,
            QVariant::fromValue(dnsEntries)),
        VpnDnsApplyStage::StageSetLinkDns);
}

void VpnDnsModeWorker::startSetLinkDomains()
{
    registerMetaTypesOnce();

    QList<DomainEntry> domainEntries;
    if (vpnDnsModeFromPriority(m_activeRequest.dnsPriority) == VpnDnsMode::VpnDnsModePreferred)
        domainEntries.append({QStringLiteral("."), true});

    watchResolvedCall(
        m_resolvedInterface->asyncCall(
            "SetLinkDomains", m_activeRequest.ifindex,
            QVariant::fromValue(domainEntries)),
        VpnDnsApplyStage::StageSetLinkDomains);
}

void VpnDnsModeWorker::startSetLinkDefaultRoute()
{
    const bool enable = vpnDnsModeFromPriority(m_activeRequest.dnsPriority)
        == VpnDnsMode::VpnDnsModeSecondary;
    watchResolvedCall(
        m_resolvedInterface->asyncCall(
            "SetLinkDefaultRoute", m_activeRequest.ifindex, enable),
        VpnDnsApplyStage::StageSetLinkDefaultRoute);
}

void VpnDnsModeWorker::startRevertLink()
{
    watchResolvedCall(
        m_resolvedInterface->asyncCall("RevertLink", m_activeRequest.ifindex),
        VpnDnsApplyStage::StageRevertLink);
}

void VpnDnsModeWorker::watchResolvedCall(const QDBusPendingCall &call, VpnDnsApplyStage stage)
{
    if (m_stopping || !m_hasActiveRequest)
        return;

    m_stage = stage;
    m_pendingCallWatcher = new QDBusPendingCallWatcher(call, this);
    connect(m_pendingCallWatcher, &QDBusPendingCallWatcher::finished,
            this, &VpnDnsModeWorker::onResolvedCallFinished);
}

void VpnDnsModeWorker::onResolvedCallFinished(QDBusPendingCallWatcher *watcher)
{
    if (!watcher)
        return;

    if (watcher != m_pendingCallWatcher) {
        watcher->deleteLater();
        return;
    }

    const VpnDnsApplyStage completedStage = m_stage;
    const bool isError = watcher->isError();
    const QDBusError error = watcher->error();
    m_pendingCallWatcher = nullptr;
    watcher->deleteLater();

    if (m_stopping || !m_hasActiveRequest)
        return;

    if (m_activeRequestInvalidated) {
        finishActiveRequest(VpnDnsApplyResult::DbusError, false);
        return;
    }

    if (isError) {
        qCWarning(DSM()) << "[DNS-TRACE] resolved D-Bus call failed:"
                         << static_cast<int>(completedStage) << "error type:" << error.type();
        const bool serviceUnavailable = error.type() == QDBusError::ServiceUnknown
            || error.type() == QDBusError::NoServer
            || error.type() == QDBusError::Disconnected;
        const VpnDnsApplyResult result = serviceUnavailable
            ? VpnDnsApplyResult::ServiceUnavailable
            : VpnDnsApplyResult::DbusError;
        if (m_applyTransaction.beginRollback(completedStage, result)) {
            startRevertLink();
            return;
        }

        finishActiveRequest(m_applyTransaction.complete(result));
        return;
    }

    switch (completedStage) {
    case VpnDnsApplyStage::StageSetLinkDns:
        startSetLinkDomains();
        break;
    case VpnDnsApplyStage::StageSetLinkDomains:
        startSetLinkDefaultRoute();
        break;
    case VpnDnsApplyStage::StageSetLinkDefaultRoute:
        finishActiveRequest(VpnDnsApplyResult::Success);
        break;
    case VpnDnsApplyStage::StageRevertLink:
        finishActiveRequest(m_applyTransaction.complete(VpnDnsApplyResult::Success));
        break;
    case VpnDnsApplyStage::StageIdle:
        finishActiveRequest(VpnDnsApplyResult::DbusError);
        break;
    }
}

void VpnDnsModeWorker::finishActiveRequest(VpnDnsApplyResult result, bool notify)
{
    if (!m_hasActiveRequest)
        return;

    const VpnDnsModeRequest request = m_activeRequest;
    m_activeRequest = VpnDnsModeRequest();
    m_hasActiveRequest = false;
    m_activeRequestInvalidated = false;
    m_applyTransaction.reset();
    m_stage = VpnDnsApplyStage::StageIdle;

    if (notify && !m_stopping) {
        Q_EMIT applyFinished(request.connectionPath, request.generation,
                             request.dnsPriority, result);
    }

    startNextRequest();
}

void VpnDnsModeWorker::stop()
{
    if (m_stopping)
        return;

    m_stopping = true;
    m_requestQueue.clear();

    if (m_polkitWatcher) {
        delete m_polkitWatcher;
        m_polkitWatcher = nullptr;
    }
    if (m_pendingCallWatcher) {
        delete m_pendingCallWatcher;
        m_pendingCallWatcher = nullptr;
    }

    m_activeRequest = VpnDnsModeRequest();
    m_hasActiveRequest = false;
    m_activeRequestInvalidated = false;
    m_applyTransaction.reset();
    m_stage = VpnDnsApplyStage::StageIdle;
}
