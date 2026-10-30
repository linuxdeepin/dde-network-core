// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "systemipconfilct.h"
#include "settingconfig.h"

#include "nmaccess.h"

#include <QDebug>

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Ipv4Setting>

using namespace network::systemservice;

SystemIPConflict::SystemIPConflict(QObject *parent)
    : IPConflictHandler(parent)
{
}

SystemIPConflict::~SystemIPConflict()
{
}

bool SystemIPConflict::ipConflicted(const QString &devicePath) const
{
    return m_ipConfilctData.isConflicted(devicePath);
}

bool SystemIPConflict::canReconnect(const QSharedPointer<NetworkManager::ActiveConnection> &activeConnection)
{
    NMQT_ASSERT_THREAD();
    if (activeConnection.isNull() || activeConnection->connection().isNull())
        return false;

    NetworkManager::Ipv4Setting::Ptr setting =
            activeConnection->connection()->settings()->setting(NetworkManager::Setting::Ipv4).dynamicCast<NetworkManager::Ipv4Setting>();

    if (setting) {
        //获取IP地址的方式，这里只处理自动获取IP地址的方式，如果当前是手动获取IP地址，则无需再次连接
        NetworkManager::Ipv4Setting::ConfigMethod method = setting->method();
        if (method != NetworkManager::Ipv4Setting::ConfigMethod::Automatic)
            return false;
    }

    NetworkManager::Setting::SecretFlags secretSetting = secretFlags(activeConnection);
    // 当密码选项为为所用用户保存密码或者总是询问的时候，系统服务会让其自动重连
    return (secretSetting.testFlag(NetworkManager::Setting::None)
            || secretSetting.testFlag(NetworkManager::Setting::NotSaved));
}

void SystemIPConflict::onIPConflicted(const QString &ip, const QString &localMac, const QString &remoteMac)
{
    NMQT_ASSERT_THREAD();
    bool needReconnect = false;
    const bool conflicted = m_ipConfilctData.doConfilcted(ip, localMac, remoteMac, needReconnect);
    const QString uni = m_ipConfilctData.deviceUniOfMac(localMac);
    if (uni.isEmpty())
        return;

    if (conflicted)
        Q_EMIT ipConflictChanged(uni, ip, true);

    if (needReconnect) {
        const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(uni);
        const NetworkManager::ActiveConnection::Ptr activeConnection =
                snapshot.activeConnectionUni.isEmpty()
                ? NetworkManager::ActiveConnection::Ptr()
                : NetworkManager::findActiveConnection(snapshot.activeConnectionUni);
        requestReConnect(activeConnection);
    }
}

void SystemIPConflict::onIPConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac)
{
    const QString uni = m_ipConfilctData.deviceUniOfMac(localMac);
    if (uni.isEmpty())
        return;

    if (m_ipConfilctData.doConflictReslove(ip, localMac, remoteMac))
        Q_EMIT ipConflictChanged(uni, ip, false);
}

bool SystemIPConflict::ipConflicted(const QString &devicePath, const QString &ip) const
{
    return m_ipConfilctData.isConflicted(devicePath, ip);
}

void SystemIPConflict::ipv4Changed(const QString &deviceUni)
{
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(deviceUni);
    if (!snapshot.isValid())
        return;

    QStringList resloveIps;
    m_ipConfilctData.updateIpv4(deviceUni, snapshot.ipV4Addresses, resloveIps);
    for (const QString &resloveIp : resloveIps) {
        Q_EMIT ipConflictChanged(snapshot.uni, resloveIp, false);
    }
}
