// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONNECTIVITYCHECKER_H
#define CONNECTIVITYCHECKER_H

#include "constants.h"

#include <QObject>
#include <QStringList>

#include <atomic>

class QThread;
class QTimer;

namespace NetworkManager {
class Device;
class ActiveConnection;
} // namespace NetworkManager
namespace network {
namespace systemservice {

class StatusChecker;
class InternetChecker;
class ConnectivityProber;
struct ConnectivityProbeRequest;
struct ConnectivityProbeResult;

// 网络连通性的检测
class ConnectivityChecker : public QObject
{
    Q_OBJECT

public:
    ConnectivityChecker(QObject *parent = Q_NULLPTR);
    virtual ~ConnectivityChecker() = default;

    virtual network::service::Connectivity connectivity() const = 0;

    virtual QString portalUrl() const { return ""; }

    virtual void checkConnectivity() {}

Q_SIGNALS:
    void connectivityChanged(const network::service::Connectivity &);
};

class LocalConnectionvityChecker : public ConnectivityChecker
{
    Q_OBJECT

public:
    explicit LocalConnectionvityChecker(QObject *parent = Q_NULLPTR);
    ~LocalConnectionvityChecker() override;

    network::service::Connectivity connectivity() const override;
    QString portalUrl() const override;
    void checkConnectivity() override;

Q_SIGNALS:
    void portalDetected(const QString &);

private Q_SLOTS:
    void onPortalDetected(const QString &portalUrl);
    void onConnectivityChanged(network::service::Connectivity connectivity);
    void onProbeFinished(const ConnectivityProbeResult &result);

private:
    void startProbe(const ConnectivityProbeRequest &request);

    // 以下状态只在插件线程读写：StatusChecker 的信号跨线程排队到本对象的线程
    StatusChecker *m_statusChecker;
    QString m_portalUrl;
    // 认证地址对应的主连接，变化后 m_portalUrl 视为过期
    QString m_portalPrimaryId;
    network::service::Connectivity m_connectivity;
    QThread *m_probeThread;
    ConnectivityProber *m_prober;
    QThread *m_internetCheckerThread;
    InternetChecker *m_internetChecker;
};

class StatusChecker : public QObject
{
    Q_OBJECT

public:
    explicit StatusChecker(QObject *parent = Q_NULLPTR);
    ~StatusChecker() override;

    // 以下接口均须在 NM 线程上执行
    void initConnectivityChecker();
    void stop();

    // 返回状态仅 NM 线程可信；跨线程读取请用 LocalConnectionvityChecker 的缓存
    network::service::Connectivity connectivity() const;
    QString portalUrl() const;
    void checkConnectivity();
    // 插件线程读取：原子量
    bool primaryConnectionChanged() const;

    // 探测结果回填（由 LocalConnectionvityChecker 排队至 NM 线程调用）
    void onProbeResult(const ConnectivityProbeResult &result);

Q_SIGNALS:
    void portalDetected(const QString &);
    void connectivityChanged(const network::service::Connectivity &);
    void probeRequested(const ConnectivityProbeRequest &request);

private:
    void setConnectivity(const network::service::Connectivity &connectivity);
    void setPortalUrl(const QString &portalUrl);
    void initDefaultConnectivity();

private Q_SLOTS:
    void onUpdateUrls(const QStringList &urls);
    void startCheck();
    void startProbeCheck();
    void onDevicesChanged();
    void onPrimaryConnectionChanged(const QString &uni);
    void onActiveConnectionsChanged();

private:
    QTimer *m_checkTimer;
    QTimer *m_timer;
    QTimer *m_pendingCheckTimer;
    bool m_pendingCheck;
    QList<QMetaObject::Connection> m_checkerConnection;
    network::service::Connectivity m_connectivity;
    int m_checkCount;
    QString m_portalUrl;
    QStringList m_checkUrls;
    bool m_isStop;
    QString m_primaryId;                 // 仅 NM 线程访问    std::atomic<bool> m_primaryConnectionChanged; // 插件线程读取    quint64 m_probeGeneration;
};

class NMConnectionvityChecker : public ConnectivityChecker
{
    Q_OBJECT

public:
    explicit NMConnectionvityChecker(QObject *parent = Q_NULLPTR);
    ~NMConnectionvityChecker() override = default;
    network::service::Connectivity connectivity() const override;

public Q_SLOTS:
    void initMember();
    void initConnection();

private:
    std::atomic<int> m_connectivity;
};

} // namespace systemservice
} // namespace network
#endif // CONNECTIVITYCHECKER_H