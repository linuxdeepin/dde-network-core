// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "ipconflicthandler.h"
#include "settingconfig.h"
#include "utils/constants.h"
#include "system/nmaccess.h"

#include <QDBusConnection>
#include <QMutexLocker>
#include <QSet>
#include <QDebug>

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Security8021xSetting>
#include <NetworkManagerQt/WirelessSecuritySetting>

using network::systemservice::DeviceSnapshot;
using network::systemservice::NetworkManagerProxy;
using network::systemservice::isNetworkManagerThread;
using network::systemservice::networkManagerThread;

const QString ipwatchdService = "org.deepin.dde.IPWatchD1";
const QString ipwatchdPath = "/org/deepin/dde/IPWatchD1";
const QString ipwatchdInterface = "org.deepin.dde.IPWatchD1";

IPConflictHandler::IPConflictHandler(QObject *parent)
    : QObject (parent)
{
    // system 分支（无 parent）迁移到 NM 线程；带 parent 的对象无法 moveToThread，
    // 显式保留在原线程（当前仅 session 分支，未纳入本次单线程改造）。
    if (!parent) {
        moveToThread(networkManagerThread());
    } else {
        qCWarning(DSM()) << "IPConflictHandler has a parent, stays on the creating thread";
    }
    QMetaObject::invokeMethod(this, &IPConflictHandler::initConnection, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &IPConflictHandler::initDevice, Qt::QueuedConnection);
}

NetworkManager::Setting::SecretFlags IPConflictHandler::secretFlags(const NetworkManager::ActiveConnection::Ptr &activeConnection) const
{
    if (activeConnection.isNull() || activeConnection->connection().isNull())
        return NetworkManager::Setting::SecretFlagType::None;

    NetworkManager::WirelessSecuritySetting::Ptr wirelessSecretSetting = activeConnection->connection()->settings()
            ->setting(NetworkManager::Setting::WirelessSecurity).dynamicCast<NetworkManager::WirelessSecuritySetting>();
    if (wirelessSecretSetting) {
        // init KeyMgmt
        const NetworkManager::WirelessSecuritySetting::KeyMgmt &keyMgmt = wirelessSecretSetting->keyMgmt();
        // 当前选择的加密方式
        NetworkManager::WirelessSecuritySetting::KeyMgmt currentKeyMgmt = (keyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::Unknown) ?
                           NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaNone : keyMgmt;

        // 当前选择的保密模式
        if (currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::Wep) {
            return wirelessSecretSetting->wepKeyFlags();
        }
        if (currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaPsk ||
                    currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE) {
            return wirelessSecretSetting->pskFlags();
        }
    } else {
        NetworkManager::Security8021xSetting::Ptr secretSetting = activeConnection->connection()->settings()
                ->setting(NetworkManager::Setting::Security8021x).dynamicCast<NetworkManager::Security8021xSetting>();

        if (secretSetting) {
            const QList<NetworkManager::Security8021xSetting::EapMethod> &eapMethods = secretSetting->eapMethods();
            NetworkManager::Security8021xSetting::EapMethod method = eapMethods.isEmpty() ? NetworkManager::Security8021xSetting::EapMethodTls : eapMethods.first();
            return (method == NetworkManager::Security8021xSetting::EapMethodTls ?
                                                     secretSetting->privateKeyPasswordFlags() : secretSetting->passwordFlags());
        }
    }

    // 默认是为所有用户保存密码
    return NetworkManager::Setting::SecretFlagType::None;
}

void IPConflictHandler::requestReConnect(const NetworkManager::ActiveConnection::Ptr &activeConnection)
{
    NMQT_ASSERT_THREAD();
    if (activeConnection.isNull()) {
        qCWarning(DSM) << "cannot get device.";
        return;
    }

    if (canReconnect(activeConnection)) {
        const QString activePath = activeConnection->path();
        const QString connectionPath = activeConnection->connection()->path();
        const QStringList devices = activeConnection->devices();
        if (devices.isEmpty()) {
            qCWarning(DSM()) << "active connection has no device" << activePath;
            return;
        }
        // 写操作统一经代理排队；本函数已在 NM 线程时同步执行
        NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
        proxy->deactivateConnection(activePath);
        proxy->activateConnection(connectionPath, devices.first());
    }
}

bool IPConflictHandler::canReconnect(const QSharedPointer<NetworkManager::ActiveConnection> &activeConnection)
{
    // 激活尚未完成或 profile 已被删除时 connection() 为空，必须判空
    return !activeConnection.isNull() && !activeConnection->connection().isNull();
}

bool IPConflictHandler::ipConflicted(const QString &devicePath, const QString &ip) const
{
    Q_UNUSED(devicePath)
    Q_UNUSED(ip)
    return false;
}

void IPConflictHandler::initConnection()
{
    NMQT_ASSERT_THREAD();
    QDBusConnection::systemBus().connect(ipwatchdService, ipwatchdPath, ipwatchdInterface, "IPConflict", this, SLOT(onReceiveIPConflict(QString, QString, QString)));
    QDBusConnection::systemBus().connect(ipwatchdService, ipwatchdPath, ipwatchdInterface, "IPConflictReslove", this, SLOT(onReceiveIPConflictReslove(QString, QString, QString)));
    // 库信号只在代理内部 connect；这里消费代理的自有信号
    connect(NetworkManagerProxy::instance(), &NetworkManagerProxy::devicesChanged, this, [this] {
        initDevice();
    });
}

void IPConflictHandler::initDevice()
{
    NMQT_ASSERT_THREAD();
    const QList<DeviceSnapshot> devices = NetworkManagerProxy::instance()->devices();

    QSet<QString> alive;
    for (const DeviceSnapshot &device : devices) {
        if (device.type != NetworkManager::Device::Ethernet && device.type != NetworkManager::Device::Wifi) {
            continue;
        }
        alive.insert(device.uni);

        // 设备对象的 ipV4AddressChanged 不经过通知器；这里比对快照刷新前后的地址列表
        auto it = m_lastIpv4.find(device.uni);
        if (it == m_lastIpv4.end()) {
            m_lastIpv4.insert(device.uni, device.ipV4Addresses);
            continue;
        }
        if (it.value() == device.ipV4Addresses) {
            continue;
        }
        it.value() = device.ipV4Addresses;
        ipv4Changed(device.uni);
    }

    for (auto it = m_lastIpv4.begin(); it != m_lastIpv4.end();) {
        it = alive.contains(it.key()) ? std::next(it) : m_lastIpv4.erase(it);
    }
}

void IPConflictHandler::onReceiveIPConflict(const QString &ip, const QString &localMac, const QString &remoteMac)
{
    onIPConflicted(ip, localMac, remoteMac);
}

void IPConflictHandler::onReceiveIPConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac)
{
    onIPConflictReslove(ip, localMac, remoteMac);
}

/**
 * @brief IP冲突的数据存储
 */
bool IPConflictStore::DeviceIpData::operator==(const struct DeviceIpData &data) const
{
    return this->m_ip == data.m_ip
           && this->m_uni == data.m_uni
           && this->m_localMac == data.m_localMac
           && this->m_remoteMac == data.m_remoteMac;
}
IPConflictStore::IPConflictStore()
{
}

IPConflictStore::~IPConflictStore()
{
}

namespace {

// 设备快照里与 MAC 相关的两个字段：永久地址（出厂）与当前地址（用户可能修改）。
// IP 冲突上报的是当前 MAC，但两种都做匹配以兼容未修改过的情况。
bool macMatches(const DeviceSnapshot &snapshot, const QString &mac)
{
    return (!snapshot.permanentHardwareAddress.isEmpty() && snapshot.permanentHardwareAddress == mac)
            || (!snapshot.hardwareAddress.isEmpty() && snapshot.hardwareAddress == mac);
}

} // namespace

bool IPConflictStore::doConfilcted(const QString &ip, const QString &localMac, const QString &remoteMac, bool &needReconnect)
{
    needReconnect = false;
    QString macAddress = formatMac(localMac);
    const QString uni = deviceUniOfMac(macAddress);
    if (uni.isEmpty()) {
        qCWarning(DSM) << "not found device by mac address";
        return false;
    }

    qCInfo(DSM()) << "ip conflicted, ip:" << ip << "mac:" << macAddress << "remote mac:" << formatMac(remoteMac);
    // 在锁外读取配置：该访问器自带锁，且首次调用会懒构造单例（同步 D-Bus），
    // 持 m_mutex 期间做这些会把 D-Bus 查询路径一起卡住。
    const bool reconnectOnFirstConflict = SettingConfig::instance()->reconnectIfIpConflicted();

    QMutexLocker locker(&m_mutex);
    if (m_conflictData.contains(macAddress)) {
        const QList<DeviceIpData> &datas = m_conflictData[macAddress];
        for (const DeviceIpData &data : datas) {
            if (data.m_ip == ip && data.m_localMac == macAddress && data.m_remoteMac == formatMac(remoteMac))
                return false;
        }
    } else if (reconnectOnFirstConflict) {
        // 如果是第一次IP冲突，则重新获取IP地址
        needReconnect = true;
    }
    DeviceIpData data;
    data.m_ip = ip;
    data.m_uni = uni;
    data.m_localMac = macAddress;
    data.m_remoteMac = formatMac(remoteMac);
    m_conflictData[macAddress] << data;
    return true;
}

bool IPConflictStore::doConflictReslove(const QString &ip, const QString &localMac, const QString &remoteMac)
{
    QMutexLocker locker(&m_mutex);
    QString macAddress = formatMac(localMac);
    if (!m_conflictData.contains(macAddress))
        return true;

    // 删除当前设备对应的解除IP
    int removeIndex = -1;
    QList<DeviceIpData> &datas = m_conflictData[macAddress];
    for (int i = 0; i < datas.size(); i++) {
        DeviceIpData data = datas[i];
        if (data.m_ip == ip && data.m_localMac == macAddress && data.m_remoteMac == formatMac(remoteMac)) {
            removeIndex = i;
            break;
        }
    }

    if (removeIndex >= 0) {
        datas.removeAt(removeIndex);
        if (datas.isEmpty()) {
            m_conflictData.remove(macAddress);
            return true;
        }
    }

    return false;
}

bool IPConflictStore::isConflicted(const QString &devicePath) const
{
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(devicePath);
    if (!snapshot.isValid())
        return false;

    QMutexLocker locker(&m_mutex);
    return m_conflictData.contains(snapshot.permanentHardwareAddress)
            || m_conflictData.contains(snapshot.hardwareAddress);
}

QString IPConflictStore::formatMac(const QString &macAddress) const
{
    QStringList macs = macAddress.split(":");
    for (QString &mac : macs) {
        mac = mac.toUpper();
        if (mac.length() == 1)
            mac = QString("0%1").arg(mac);
    }

    return macs.join(":");
}

bool IPConflictStore::isConflicted(const QString &devicePath, const QString &ip) const
{
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(devicePath);
    if (!snapshot.isValid())
        return false;

    QMutexLocker locker(&m_mutex);
    QList<DeviceIpData> conflictedIps;
    if (m_conflictData.contains(snapshot.permanentHardwareAddress)) {
        conflictedIps = m_conflictData[snapshot.permanentHardwareAddress];
    } else if (m_conflictData.contains(snapshot.hardwareAddress)) {
        conflictedIps = m_conflictData[snapshot.hardwareAddress];
    }

    for (const DeviceIpData &data : conflictedIps) {
        if (data.m_ip == ip) {
            return true;
        }
    }

    return false;
}

QString IPConflictStore::deviceUniOfMac(const QString &mac) const
{
    const QString permanentMac = formatMac(mac);
    const QList<DeviceSnapshot> devices = NetworkManagerProxy::instance()->devices();
    for (const DeviceSnapshot &snapshot : devices) {
        if (!macMatches(snapshot, permanentMac)) {
            continue;
        }
        return snapshot.uni;
    }
    return QString();
}

void IPConflictStore::updateIpv4(const QString &deviceUni, const QStringList &ips, QStringList &resloveIps)
{
    const DeviceSnapshot snapshot = NetworkManagerProxy::instance()->device(deviceUni);
    if (!snapshot.isValid())
        return;

    QMutexLocker locker(&m_mutex);
    QString macAddress;
    if (m_conflictData.contains(snapshot.permanentHardwareAddress)) {
        macAddress = snapshot.permanentHardwareAddress;
    } else if (m_conflictData.contains(snapshot.hardwareAddress)) {
        macAddress = snapshot.hardwareAddress;
    }

    if (macAddress.isEmpty() || !m_conflictData.contains(macAddress))
        return;

    qCDebug(DSM) << snapshot.interfaceName << snapshot.uni << "ip changed, reslove ip conflicted";
    QList<DeviceIpData> resloveData;
    QList<DeviceIpData> &conflictedData = m_conflictData[macAddress];
    for (const DeviceIpData &conflictData : conflictedData) {
        if (ips.contains(conflictData.m_ip))
            continue;

        resloveIps << conflictData.m_ip;
        resloveData << conflictData;
        qCDebug(DSM) << "reslove ip conflicted ip" << conflictData.m_ip;
    }

    for (const DeviceIpData &conflictData : resloveData) {
        conflictedData.removeAll(conflictData);
    }
    if (conflictedData.isEmpty()) {
        m_conflictData.remove(macAddress);
    }
}
