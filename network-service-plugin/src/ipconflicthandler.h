// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef IPCONFLICTHANDLER_H
#define IPCONFLICTHANDLER_H

#include <QObject>

#include <NetworkManagerQt/Settings>

#include <QHash>
#include <QMutex>
#include <QStringList>

namespace NetworkManager {
class Device;
class ActiveConnection;
}

class IPConflictHandler : public QObject
{
    Q_OBJECT

public:
    IPConflictHandler(QObject *parent);
    ~IPConflictHandler() = default;

public Q_SLOTS:
    // 构造点可能在插件线程，NM 访问统一排队到 NM 线程执行
    void initConnection();
    void initDevice();

protected:
    NetworkManager::Setting::SecretFlags secretFlags(const NetworkManager::ActiveConnection::Ptr &activeConnection) const;
    void requestReConnect(const NetworkManager::ActiveConnection::Ptr &activeConnection);
    virtual bool canReconnect(const QSharedPointer<NetworkManager::ActiveConnection> &activeConnection);
    virtual void onIPConflicted(const QString &ip, const QString &localMac, const QString &remoteMac) = 0;
    virtual void onIPConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac) = 0;
    virtual bool ipConflicted(const QString &devicePath, const QString &ip) const;
    virtual void ipv4Changed(const QString &deviceUni) { Q_UNUSED(deviceUni) }

private:
    // uni -> 上次看到的 IPv4 地址列表，用于在快照刷新时识别地址变化
    QHash<QString, QStringList> m_lastIpv4;

private slots:
    void onReceiveIPConflict(const QString &ip, const QString &localMac, const QString &remoteMac);
    void onReceiveIPConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac);
};

class IPConflictStore
{
public:
    IPConflictStore();
    ~IPConflictStore();
    bool doConfilcted(const QString &ip, const QString &localMac, const QString &remoteMac, bool &needReconnect);
    bool doConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac);
    bool isConflicted(const QString &devicePath) const;
    bool isConflicted(const QString &devicePath, const QString &ip) const;
    // 以下查询全部基于 NetworkManagerProxy 的快照，可在任意线程调用
    QString deviceUniOfMac(const QString &mac) const;
    void updateIpv4(const QString &deviceUni, const QStringList &ips, QStringList &resloveIps);

private:
    QString formatMac(const QString &macAddress) const;

private:
    typedef struct DeviceIpData
    {
        QString m_ip;
        QString m_localMac;
        QString m_remoteMac;
        QString m_uni;
        bool operator==(const struct DeviceIpData &data) const;
    } DeviceIpData;

    // 读写点分布在 NM 线程（冲突事件、快照刷新）与插件线程（D-Bus 查询），
    // 这里保护的是本进程自己的容器
    mutable QMutex m_mutex;
    QMap<QString, QList<DeviceIpData>> m_conflictData;
};

#endif // IPCONFLICTHANDLER_H
