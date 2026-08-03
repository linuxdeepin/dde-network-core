// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSMODEWORKER_H
#define VPNDNSMODEWORKER_H

#include "vpndnsmodeapplytransaction.h"
#include "vpndnsmodequeue.h"

#include <QObject>
#include <QList>
#include <QHostAddress>

QT_BEGIN_NAMESPACE
class QDBusInterface;
class QDBusPendingCall;
class QDBusPendingCallWatcher;
QT_END_NAMESPACE

namespace network {
namespace sessionservice {

enum VpnDnsMode
{
    VpnDnsModeNotSet = 0,
    VpnDnsModeSecondary = 1,
    VpnDnsModePreferred = 2
};

// 工作线程中的 DNS Mode 执行器：串行消费请求队列，完成 Polkit 鉴权，
// 并依次调用 systemd-resolved 的 SetLinkDNS/Domains/DefaultRoute 应用配置。
class VpnDnsModeWorker : public QObject
{
    Q_OBJECT

public:
    explicit VpnDnsModeWorker(QObject *parent = nullptr);
    ~VpnDnsModeWorker() override;

public Q_SLOTS:
    void onApplyDnsMode(const QString &connectionPath, const QString &activeConnectionPath,
                        quint64 generation, int ifindex,
                        const QList<QHostAddress> &dnsServers, int dnsPriority);
    void removeConnection(const QString &connectionPath);
    void removeActiveConnection(const QString &activeConnectionPath);
    void stop();

Q_SIGNALS:
    void applyFinished(const QString &connectionPath, quint64 generation, int dnsPriority,
                       VpnDnsApplyResult result);

private:
    void ensureResolvedInterface();
    void startNextRequest();
    void startAuthorization();
    void onPolkitCheckFinished(QDBusPendingCallWatcher *watcher);
    void startDnsModeApply();
    void startSetLinkDns();
    void startSetLinkDomains();
    void startSetLinkDefaultRoute();
    void startRevertLink();
    void watchResolvedCall(const QDBusPendingCall &call, VpnDnsApplyStage stage);
    void onResolvedCallFinished(QDBusPendingCallWatcher *watcher);
    void finishActiveRequest(VpnDnsApplyResult result, bool notify = true);

    static void registerMetaTypesOnce();

    QDBusInterface *m_resolvedInterface = nullptr;
    QDBusPendingCallWatcher *m_pendingCallWatcher = nullptr;
    QDBusPendingCallWatcher *m_polkitWatcher = nullptr;
    VpnDnsModeRequestQueue m_requestQueue;
    VpnDnsModeRequest m_activeRequest;
    VpnDnsModeApplyTransaction m_applyTransaction;
    VpnDnsApplyStage m_stage = VpnDnsApplyStage::StageIdle;
    bool m_hasActiveRequest = false;
    bool m_activeRequestInvalidated = false;
    bool m_stopping = false;
};
} // namespace sessionservice
} // namespace network

Q_DECLARE_METATYPE(network::sessionservice::VpnDnsApplyResult)

#endif // VPNDNSMODEWORKER_H
