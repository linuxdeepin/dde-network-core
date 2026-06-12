// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSROUTECONTROLLER_H
#define VPNDNSROUTECONTROLLER_H

#include <QObject>
#include <QMap>
#include <NetworkManagerQt/ActiveConnection>

QT_BEGIN_NAMESPACE
class QHostAddress;
class QDBusInterface;
class QDBusPendingCallWatcher;
class QThread;
class QTimer;
QT_END_NAMESPACE

namespace dde {
namespace network {

enum VpnDnsMode
{
    VpnDnsModeNotSet = 0,
    VpnDnsModeSecondary = 1,
    VpnDnsModePreferred = 2
};


class VpnDnsRouteWorker : public QObject
{
    Q_OBJECT

public:
    explicit VpnDnsRouteWorker(QObject *parent = nullptr);
    ~VpnDnsRouteWorker() override;

public Q_SLOTS:
    void onApplyDnsModeForVpnAc(const NetworkManager::ActiveConnection::Ptr &vpnAc);
    void onCheckAndApplyDnsModeIfChanged(const QString &vpnConnectionPath, bool isCompare);
    void onCleanupConnection(const QString &connectionPath);

private:
    VpnDnsMode getVpnDnsModeFromConnection(const QString &connectionPath) const;
    void ensureResolvedInterface();
    bool checkPolkitAuthorization();

    bool applyPreferredDnsMode(int ifindex, const QList<QHostAddress> &dnsServers);
    bool applySecondaryDnsMode(int ifindex, const QList<QHostAddress> &dnsServers);
    bool applyNotSetDnsMode(int ifindex);

    bool setLinkDns(int ifindex, const QList<QHostAddress> &dnsServers);
    bool setLinkDomains(int ifindex, const QStringList &domains, const QList<bool> &routes);
    bool setLinkDefaultRoute(int ifindex, bool enable);
    bool revertLink(int ifindex);
    static void registerMetaTypesOnce();

    QDBusInterface *m_resolvedInterface = nullptr;
    QMap<QString, int> m_lastDnsPriority; // 缓存每个连接的 dns-priority，worker 线程持有
};


class VpnDnsRouteController : public QObject
{
    Q_OBJECT

public:
    explicit VpnDnsRouteController(QObject *parent = nullptr);
    ~VpnDnsRouteController() override;
    void requestApplyDnsModeIfChanged(const NetworkManager::ActiveConnection::Ptr &vpnAc, bool isCompare);
    void cleanupConnection(const QString &connectionPath);

Q_SIGNALS:
    void requestCheckAndApplyDnsModeIfChanged(const QString &vpnConnectionPath, bool isCompare);
    void requestCleanupConnection(const QString &connectionPath);

private:
    QThread *m_workerThread = nullptr;
    VpnDnsRouteWorker *m_worker = nullptr;
};

} // namespace network
} // namespace dde
#endif // VPNDNSROUTECONTROLLER_H
