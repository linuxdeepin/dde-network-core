// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSMODEHANDLER_H
#define VPNDNSMODEHANDLER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QHostAddress>
#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include "vpndnsmodeapplystate.h"
#include "vpndnsmodeworker.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace network {
namespace sessionservice {

// 会话服务中的 VPN DNS Mode 协调器：监听 NetworkManager 的 Profile 与 ActiveConnection 生命周期，
// 发现 Tun/DNS 目标并提交应用请求，处理结果回写与失败回滚。
class VpnDnsModeHandler : public QObject
{
    Q_OBJECT

public:
    explicit VpnDnsModeHandler(QObject *parent = nullptr);
    ~VpnDnsModeHandler() override;

Q_SIGNALS:
    void requestApplyDnsMode(const QString &connectionPath, const QString &activeConnectionPath,
                             quint64 generation, int ifindex,
                             const QList<QHostAddress> &dnsServers, int dnsPriority);
    void requestRemoveConnection(const QString &connectionPath);
    void requestRemoveActiveConnection(const QString &activeConnectionPath);

private Q_SLOTS:
    void onConnectionAdded(const QString &path);
    void onConnectionRemoved(const QString &path);
    void onConnectionUpdated();
    void onActiveConnectionAdded(const QString &path);
    void onActiveConnectionRemoved(const QString &path);
    void onVpnIp4ConfigChanged();
    void onApplyFinished(const QString &connectionPath, quint64 generation, int dnsPriority,
                         VpnDnsApplyResult result);

private:
    void init();
    void initNetworkConnections();
    void initActiveConnections();
    void connectConnectionSignals(const NetworkManager::Connection::Ptr &connection);
    void trackVpnActiveConnection(const NetworkManager::ActiveConnection::Ptr &activeConnection);
    NetworkManager::ActiveConnection::Ptr findVpnActiveConnection(const QString &connectionPath) const;
    void requestApplyDnsModeIfChanged(const NetworkManager::ActiveConnection::Ptr &vpnAc, bool isCompare);
    void rollbackConnectionDnsPriority(const QString &connectionPath, int target);
    void ensureWorkerThread();

private:
    QMap<QString, NetworkManager::Connection::Ptr> m_vpnConnections;
    QMap<QString, QString> m_activeVpnConnections;
    QThread *m_workerThread = nullptr;
    VpnDnsModeWorker *m_worker = nullptr;
    QMap<QString, VpnDnsModeApplyState> m_applyStates;
};

} // namespace sessionservice
} // namespace network
#endif // VPNDNSMODEHANDLER_H
