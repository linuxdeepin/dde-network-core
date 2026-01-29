// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONNECTIVITYCHECKER_H
#define CONNECTIVITYCHECKER_H

#include "constants.h"

#include <QObject>

class QTimer;

namespace NetworkManager {
class Device;
class ActiveConnection;
} // namespace NetworkManager

namespace network {
namespace systemservice {

class StatusChecker;

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

signals:
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

signals:
    void portalDetected(const QString &);

private slots:
    void onPortalDetected(const QString &portalUrl);
    void onConnectivityChanged(network::service::Connectivity connectivity);

private:
    StatusChecker *m_statusChecker;
    QString m_portalUrl;
    network::service::Connectivity m_connectivity;
    QThread *m_thread;
};

class StatusChecker : public QObject
{
    Q_OBJECT

public:
    explicit StatusChecker(QObject *parent = Q_NULLPTR);
    ~StatusChecker() override;
    void initConnectivityChecker();
    void stop();

    network::service::Connectivity connectivity() const;
    QString portalUrl() const;
    void checkConnectivity();
    QString detectionConnectionId() const;

signals:
    void portalDetected(const QString &);
    void connectivityChanged(const network::service::Connectivity &);

private:
    void initDeviceConnect(const QList<QSharedPointer<NetworkManager::Device>> &devices);
    void setConnectivity(const network::service::Connectivity &connectivity);
    void setPortalUrl(const QString &portalUrl);
    void initDefaultConnectivity();

private slots:
    void onUpdataActiveState(const QSharedPointer<NetworkManager::ActiveConnection> &networks);
    void onUpdateUrls(const QStringList &urls);
    void startCheck();
    void realStartCheck();
    void onActiveConnectionChanged();

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
    QString m_primaryId;
};

class NMConnectionvityChecker : public ConnectivityChecker
{
    Q_OBJECT

public:
    explicit NMConnectionvityChecker(QObject *parent = Q_NULLPTR);
    ~NMConnectionvityChecker() override = default;
    network::service::Connectivity connectivity() const override;

private:
    void initMember();
    void initConnection();

private:
    network::service::Connectivity m_connectivity;
};

} // namespace systemservice

} // namespace network

#endif // CONNECTIVITYCHECKER_H
