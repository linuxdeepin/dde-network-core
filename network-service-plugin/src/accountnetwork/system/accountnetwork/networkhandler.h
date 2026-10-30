// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef NETWORKHANDLER_H
#define NETWORKHANDLER_H

#include "system/nmaccess.h"

#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QSet>

namespace accountnetwork {
namespace systemservice {

class AccountManager;
class NetworkConfig;

// 账户网络的设备状态记录。
//
// 全程运行在插件线程：AccountManager / NetworkConfig 都是插件线程的对象，
// 在这里读写才不需要加锁。一切 NetworkManagerQt 访问都经 NetworkManagerProxy
// 完成——设备状态读快照，活跃连接 ID 走 requestActiveConnectionId() 异步取回。
class NetworkHandler : public QObject
{
    Q_OBJECT

public:
    NetworkHandler(AccountManager *accountManager, NetworkConfig *conf, QObject *parent = Q_NULLPTR);
    ~NetworkHandler();

    // 可在任意线程调用；写操作由代理排队到 NM 线程
    void disconnectNetwork();

private Q_SLOTS:
    void onDevicesChanged();
    void onActiveConnectionIdReady(const QString &activeConnectionUni, const QString &saveId);

private:
    void scheduleRetry();

    AccountManager *m_accountManager;
    NetworkConfig *m_networkConfig;
    // 设备 uni -> 已记录的活跃连接，避免每次缓存刷新都重写配置文件
    QHash<QString, QString> m_savedConnections;
    // 已向代理发出、尚未回来的活跃连接查询
    QSet<QString> m_pendingQueries;
    QMetaObject::Connection m_devicesConnection;
    QMetaObject::Connection m_idConnection;
    // 取不到连接 ID 时的有限次退避重试
    static constexpr int kMaxRetries = 3;
    static constexpr int kRetryIntervalMs = 5000;
    int m_retryCount = 0;
    bool m_retryScheduled = false;
};

}
}

#endif // NETWORKHANDLER_H