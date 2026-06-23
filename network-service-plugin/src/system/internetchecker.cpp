// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "internetchecker.h"

#include "httpmanager.h"
#include "settingconfig.h"
#include "constants.h"

#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Ipv6Setting>
#include <NetworkManagerQt/Settings>

using namespace network::systemservice;

InternetChecker::InternetChecker(QObject *parent)
    : QObject(parent)
    , m_tryIndex(0)
    , m_switchTimer(nullptr)
    , m_isSwitching(false)
{
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::primaryConnectionChanged,
            this, &InternetChecker::onPrimaryConnectionChanged);
}

InternetChecker::~InternetChecker()
{
    if (m_switchTimer) {
        m_switchTimer->stop();
        m_switchTimer->deleteLater();
    }
}

void InternetChecker::resetAllNeverDefault() const
{
    const NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    for (const NetworkManager::Device::Ptr &device : devices) {
        NetworkManager::ActiveConnection::Ptr activeConn = device->activeConnection();
        if (!activeConn.isNull())
            setConnectionNeverDefault(activeConn->connection(), device, false);
    }
}

bool InternetChecker::setConnectionNeverDefault(const NetworkManager::Connection::Ptr &conn,
                                               const NetworkManager::Device::Ptr &device,
                                               bool neverDefault) const
{
    if (conn.isNull())
        return false;

    NMVariantMapMap settingsMap;
    bool changed = false;

    NetworkManager::Setting::Ptr ipv4Setting = conn->settings()->setting(NetworkManager::Setting::Ipv4);
    if (ipv4Setting) {
        NetworkManager::Ipv4Setting::Ptr ipv4 = ipv4Setting.dynamicCast<NetworkManager::Ipv4Setting>();
        if (ipv4->neverDefault() != neverDefault) {
            ipv4->setNeverDefault(neverDefault);
            settingsMap.insert(ipv4->name(), ipv4->toMap());
            changed = true;
        }
    }
    NetworkManager::Setting::Ptr ipv6Setting = conn->settings()->setting(NetworkManager::Setting::Ipv6);
    if (ipv6Setting) {
        NetworkManager::Ipv6Setting::Ptr ipv6 = ipv6Setting.dynamicCast<NetworkManager::Ipv6Setting>();
        if (ipv6->neverDefault() != neverDefault) {
            ipv6->setNeverDefault(neverDefault);
            settingsMap.insert(ipv6->name(), ipv6->toMap());
            changed = true;
        }
    }
    if (changed) {
        conn->updateUnsaved(settingsMap);
        if (!device.isNull())
            device->reapplyConnection(conn->settings()->toMap(), 0, 0);
        qCInfo(DSM) << "Set connection" << conn->name() << "never-default to" << neverDefault;
    }
    return changed;
}

void InternetChecker::setPrimaryDeviceNeverDefault(bool neverDefault) const
{
    NetworkManager::ActiveConnection::Ptr primaryConn = NetworkManager::primaryConnection();
    if (primaryConn.isNull())
        return;

    const QStringList deviceUnis = primaryConn->devices();
    for (const QString &uni : deviceUnis) {
        NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(uni);
        if (!device.isNull())
            changeDeviceNeverDefault(device, neverDefault);
    }
}

bool InternetChecker::checkInternetAccessible() const
{
    bool timedOut = false;
    return checkInternetAccessible(0, timedOut);
}

bool InternetChecker::checkInternetAccessible(int timeoutSec, bool &timedOut) const
{
    timedOut = false;

    const QStringList checkUrls = SettingConfig::instance()->networkCheckerUrls();
    for (const QString &url : checkUrls) {
        network::service::HttpManager http;
        network::service::HttpReply *httpReply = (timeoutSec > 0)
                ? http.get(url, timeoutSec)
                : http.get(url);
        if (httpReply->httpCode() != 0) {
            return true;
        }
        if (httpReply->isTimeout()) {
            timedOut = true;
            break;
        }
    }
    return false;
}

bool InternetChecker::checkInternetAccessibleWithRetry(int maxRetry) const
{
    int httpTimeout = SettingConfig::instance()->httpRequestTimeout();
    for (int i = 0; i < maxRetry; ++i) {
        // 每次检测前等待1秒，让NM路由表和DHCP先稳定
        QThread::sleep(1);
        bool timedOut = false;
        // 首次使用20秒超时，避免在无网线路由器的环境中长时间阻塞
        QElapsedTimer timer;
        timer.start();
        if (checkInternetAccessible((i == 0) ? httpTimeout : 0, timedOut)) {
            return true;
        }
        if (timedOut) {
            qCDebug(DSM) << "Request timed out after" << timer.elapsed() << "ms, skipping retries";
            return false;
        }
    }
    return false;
}

void InternetChecker::changeDeviceNeverDefault(const NetworkManager::Device::Ptr &device, bool neverDefault) const
{
    if (device.isNull())
        return;

    NetworkManager::ActiveConnection::Ptr activeConn = device->activeConnection();
    if (activeConn.isNull())
        return;

    NetworkManager::Connection::Ptr conn = activeConn->connection();
    setConnectionNeverDefault(conn, device, neverDefault);
}

void InternetChecker::onPrimaryConnectionChanged(const QString &uni)
{
    Q_UNUSED(uni)

    // 停止超时定时器
    if (m_switchTimer) {
        m_switchTimer->stop();
    }

    qCInfo(DSM) << "Primary connection changed:" << uni;
    // NM已切换主连接，通过默认路由检测整体是否可以上网（重试3次，等待DHCP/路由生效）
    if (checkInternetAccessibleWithRetry(3)) {
        qCInfo(DSM) << "Found accessible device, switching done";
        // 当前主设备已经是能上网的，它的never-default本来就是false，无需额外操作
        // 此处不能恢复其他设备never default, 因为如果恢复后，NM 内部会根据每个设备的metric值又将主链接设置回去了，导致无法上网
        m_tryIndex = 0;
        m_isSwitching = false;
        emit switchSuccess();
    } else if (m_tryIndex < m_tryDevices.size()) {
        // 当前设备不能上网，把当前主设备的never-default设为true，让下一个设备接住默认路由
        qCDebug(DSM) << "Current device still cannot access internet, trying next device, index:" << m_tryIndex;
        setPrimaryDeviceNeverDefault(true);
        NetworkManager::Device::Ptr nextDevice = m_tryDevices.at(m_tryIndex);
        changeDeviceNeverDefault(nextDevice, false);
        m_tryIndex++;
    } else {
        // 所有设备都尝试过了，全部恢复
        qCInfo(DSM) << "All devices tried, none can access internet, resetting all never-default";
        resetAllNeverDefault();
        m_tryIndex = 0;
        m_isSwitching = false;
        emit switchFailed();
    }
}

void InternetChecker::onPrimaryConnectionTimeout()
{
    // NM在超时时间内没有切换主连接，可能是只有一个可切换的设备
    qCWarning(DSM) << "PrimaryConnection change timeout, trying next device manually, index:" << m_tryIndex;
    if (m_tryIndex < m_tryDevices.size()) {
        // 把当前主设备的never-default设为true，让下一个设备接住默认路由
        setPrimaryDeviceNeverDefault(true);
        NetworkManager::Device::Ptr nextDevice = m_tryDevices.at(m_tryIndex);
        changeDeviceNeverDefault(nextDevice, false);
        m_tryIndex++;
        // 重置超时定时器
        m_switchTimer->start(5000);
    } else {
        resetAllNeverDefault();
        m_tryIndex = 0;
        m_isSwitching = false;
        emit switchFailed();
    }
}

void InternetChecker::switchInternetAccess()
{
    // 正在切换中，忽略新的切换请求
    if (m_isSwitching) {
        qCDebug(DSM) << "current is switching";
        return;
    }

    m_isSwitching = true;
    NetworkManager::Device::List availableDevice;
    NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    for (const NetworkManager::Device::Ptr &device : devices) {
        if (device->type() != NetworkManager::Device::Type::Ethernet
            && device->type() != NetworkManager::Device::Type::Wifi)
            continue;
        if (device->state() != NetworkManager::Device::State::Activated)
            continue;

        availableDevice << device;
    }

    if (availableDevice.size() <= 1) {
        qCDebug(DSM) << "only one available device,it will reset all never default";
        resetAllNeverDefault();
        m_isSwitching = false;
        return;
    }

    NetworkManager::ActiveConnection::Ptr primaryConnection = NetworkManager::primaryConnection();
    if (primaryConnection.isNull()) {
        qCWarning(DSM) << "primary connection is null, it will reset all never default";
        resetAllNeverDefault();
        m_isSwitching = false;
        return;
    }

    // 找到当前主连接使用的网卡
    QStringList primaryDeviceUnis = primaryConnection->devices();
    NetworkManager::Device::Ptr primaryDevice;
    for (const NetworkManager::Device::Ptr &device : availableDevice) {
        if (primaryDeviceUnis.contains(device->uni())) {
            primaryDevice = device;
            qWarning() << "get primary device: " << primaryDevice->interfaceName();
            break;
        }
    }

    if (primaryDevice.isNull()) {
        qWarning() << "primary device is null";
        resetAllNeverDefault();
        m_isSwitching = false;
        return;
    }

    // 保存需要遍历的设备列表（排除当前主设备）
    m_tryDevices.clear();
    int primaryIdx = availableDevice.indexOf(primaryDevice);
    for (int i = 1; i < availableDevice.size(); ++i) {
        m_tryDevices << availableDevice.at((primaryIdx + i) % availableDevice.size());
    }

    if (m_tryDevices.isEmpty()) {
        qCWarning(DSM) << "try device is empty";
        resetAllNeverDefault();
        m_isSwitching = false;
        return;
    }

    // 先将第一个候选设备的never-default恢复为false，确保它能接住默认路由
    changeDeviceNeverDefault(m_tryDevices.first(), false);
    changeDeviceNeverDefault(primaryDevice, true);
    m_tryIndex = 1;

    // 设置超时定时器，防止NM没有触发信号时卡住
    if (!m_switchTimer) {
        m_switchTimer = new QTimer(this);
        m_switchTimer->setSingleShot(true);
        connect(m_switchTimer, &QTimer::timeout, this, &InternetChecker::onPrimaryConnectionTimeout);
    }
    m_switchTimer->start(5000);
}
