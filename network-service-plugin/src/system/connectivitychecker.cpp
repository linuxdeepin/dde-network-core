// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectivitychecker.h"

#include "settingconfig.h"
#include "httpmanager.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/ActiveConnection>

#include <QTimer>
#include <QThread>

using namespace network::systemservice;

ConnectivityChecker::ConnectivityChecker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<network::service::Connectivity>("network::service::Connectivity");
}

LocalConnectionvityChecker::LocalConnectionvityChecker(QObject *parent)
    : ConnectivityChecker(parent)
    , m_statusChecker(new StatusChecker)
    , m_connectivity(network::service::Connectivity::Unknownconnectivity)
    , m_thread(new QThread)
{
    m_statusChecker->moveToThread(m_thread);
    connect(m_statusChecker, &StatusChecker::portalDetected, this, &LocalConnectionvityChecker::onPortalDetected);
    connect(m_statusChecker, &StatusChecker::connectivityChanged, this, &LocalConnectionvityChecker::onConnectivityChanged);
    m_thread->start();
    QMetaObject::invokeMethod(m_statusChecker, &StatusChecker::initConnectivityChecker, Qt::QueuedConnection);
}

LocalConnectionvityChecker::~LocalConnectionvityChecker()
{
    m_statusChecker->stop();
    m_thread->quit();
    m_thread->wait();
    m_statusChecker->deleteLater();
    m_thread->deleteLater();
}

network::service::Connectivity LocalConnectionvityChecker::connectivity() const
{
    return m_connectivity;
}

QString LocalConnectionvityChecker::portalUrl() const
{
    NetworkManager::ActiveConnection::Ptr primaryConnection = NetworkManager::primaryConnection();
    if (!primaryConnection.isNull() && primaryConnection->connection()->uuid() != m_statusChecker->detectionConnectionId()) {
        return QString();
    }
    return m_portalUrl;
}

void LocalConnectionvityChecker::checkConnectivity()
{
    // m_statusChecker 在单独的线程中执行，因此调用QMetaObject::invokeMethod方法让其在所属的线程中来执行
    QMetaObject::invokeMethod(m_statusChecker, &StatusChecker::checkConnectivity, Qt::QueuedConnection);
}

void LocalConnectionvityChecker::onPortalDetected(const QString &portalUrl)
{
    if (m_portalUrl == portalUrl)
        return;

    m_portalUrl = portalUrl;
    emit portalDetected(m_portalUrl);
}

void LocalConnectionvityChecker::onConnectivityChanged(network::service::Connectivity connectivity)
{
    qCInfo(DSM) << "Connectivity changed, incomming: " << static_cast<int>(connectivity) << ", current: " << static_cast<int>(m_connectivity);
    if (m_connectivity == connectivity)
        return;

    m_connectivity = connectivity;
    emit connectivityChanged(connectivity);
}

// 通过本地检测网络连通性
StatusChecker::StatusChecker(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
    , m_timer(new QTimer(this))
    , m_pendingCheckTimer(new QTimer(this))
    , m_pendingCheck(false)
    , m_connectivity(network::service::Connectivity::Unknownconnectivity)
    , m_checkCount(0)
    , m_isStop(false)
{
    qRegisterMetaType<NetworkManager::ActiveConnection::State>("NetworkManager::ActiveConnection::State");
    qRegisterMetaType<NetworkManager::Device::State>("NetworkManager::Device::State");
    qRegisterMetaType<NetworkManager::Device::StateChangeReason>("NetworkManager::Device::StateChangeReason");

    // 避免短时间内频繁请求网址，1s内的多次请求都是无意义的
    m_pendingCheckTimer->setSingleShot(true);
    m_pendingCheckTimer->setInterval(1000);  // 1秒防抖
    connect(m_pendingCheckTimer, &QTimer::timeout, this, [this] {
        if (m_pendingCheck)
            realStartCheck();
        m_pendingCheck = false;
    });
}

StatusChecker::~StatusChecker()
{
    for (QMetaObject::Connection connection : m_checkerConnection) {
        disconnect(connection);
    }
    m_checkerConnection.clear();

    m_checkTimer->stop();
    m_checkTimer->deleteLater();
    m_checkTimer = nullptr;

    if (m_timer->isActive())
        m_timer->stop();
    m_timer->deleteLater();
    m_timer = nullptr;

    if (m_pendingCheckTimer->isActive())
        m_pendingCheckTimer->stop();
    m_pendingCheckTimer->deleteLater();
    m_pendingCheckTimer = nullptr;
}

network::service::Connectivity StatusChecker::connectivity() const
{
    return m_connectivity;
}

QString StatusChecker::portalUrl() const
{
    return m_portalUrl;
}

void StatusChecker::checkConnectivity()
{
    qCDebug(DSM) << "Check connectivity";
    m_checkCount = 0;
    initDefaultConnectivity();
    if (m_timer->isActive()) {
        startCheck();
    } else {
        m_timer->start();
    }
}

QString StatusChecker::detectionConnectionId() const
{
    return m_primaryId;
}

void StatusChecker::initDeviceConnect(const QList<QSharedPointer<NetworkManager::Device>> &devices)
{
    for (const QSharedPointer<NetworkManager::Device> &device : devices) {
        if (device.isNull())
            continue;

        m_checkerConnection << connect(device.data(), &NetworkManager::Device::stateChanged, this, &StatusChecker::startCheck, Qt::UniqueConnection);
        m_checkerConnection << connect(device.data(), &NetworkManager::Device::activeConnectionChanged, this, [device, this] {
            qCInfo(DSM) << "Device active connection changed";
            m_checkCount = 0;
            initDefaultConnectivity();
            NetworkManager::ActiveConnection::Ptr activeConnection = device->activeConnection();
            onUpdataActiveState(activeConnection);
            if (!m_timer->isActive()) {
                m_timer->start();
            }
        });
        onUpdataActiveState(device->activeConnection());
    }
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::activeConnectionsChanged, this, &StatusChecker::onActiveConnectionChanged);
}

void StatusChecker::initConnectivityChecker()
{
    connect(SettingConfig::instance(), &SettingConfig::checkUrlsChanged, this, &StatusChecker::onUpdateUrls);
    onUpdateUrls(SettingConfig::instance()->networkCheckerUrls());
    // 定期检查网络的状态
    m_checkerConnection << connect(m_checkTimer, &QTimer::timeout, this, &StatusChecker::startCheck, Qt::UniqueConnection);
    // 这个定时器用于在设备状态发生变化后开启，每间隔2秒做一次网络检测，连续做8次然后再停下，目的是为了在设备状态发生变化的一瞬间网络检测不准确，因此做了这个机制,这样的话保证了在设备状态变化后一定时间内状态始终正确
    m_timer->setInterval(2000);
    m_checkerConnection << connect(m_timer, &QTimer::timeout, this, &StatusChecker::startCheck, Qt::UniqueConnection);
    m_checkerConnection << connect(m_timer, &QTimer::timeout, this, [this] {
        if (m_checkCount >= 8) {
            m_checkCount = 0;
            if (m_timer->isActive())
                m_timer->stop();
        } else {
            m_checkCount++;
        }
    });

    // 当网络设备状态发生变化的时候需要检查网络状态
    initDeviceConnect(NetworkManager::networkInterfaces());
    initDefaultConnectivity();

    m_checkerConnection << connect(NetworkManager::notifier(), &NetworkManager::Notifier::deviceAdded, this, [this](const QString &uni) {
        initDeviceConnect(QList<QSharedPointer<NetworkManager::Device>>() << NetworkManager::findNetworkInterface(uni));
    });
    // 第一次进来的时候执行一次网络状态的检测
    startCheck();
}

void StatusChecker::stop()
{
    m_isStop = true;
}

void StatusChecker::onUpdataActiveState(const QSharedPointer<NetworkManager::ActiveConnection> &networks)
{
    if (networks.isNull())
        return;

    connect(networks.data(), &NetworkManager::ActiveConnection::stateChanged, this, [this](NetworkManager::ActiveConnection::State state) {
        if (state == NetworkManager::ActiveConnection::State::Activated || state == NetworkManager::ActiveConnection::State::Deactivated) {
            checkConnectivity();
        }
    });
}

void StatusChecker::onUpdateUrls(const QStringList &urls)
{
    m_checkUrls = urls;
}

void StatusChecker::realStartCheck()
{
    qCDebug(DSM) << "Start check connectivity, real start check";
    if (m_isStop) {
        qCDebug(DSM) << "Stop check connectivity";
        return;
    }

    NetworkManager::ActiveConnection::Ptr pConnection = NetworkManager::primaryConnection();
    if (!pConnection.isNull()) {
        m_primaryId = pConnection->connection()->uuid();
    }
    bool networkIsOk = false;
    for (const QString &url : m_checkUrls) {
        network::service::HttpManager http;
        network::service::HttpReply *httpReply = http.get(url);
        if (m_isStop) {
            qCDebug(DSM) << "Stop check connectivity";
            break;
        }
        int httpCode = httpReply->httpCode();
        if (httpCode == 0) {
            qCWarning(DSM) << "Nework is unreachabel:" << url << httpReply->errorMessage();
            continue;
        }

        QString portalUrl = httpReply->portal();
        networkIsOk = true;
        qCDebug(DSM) << "Http reply code:" << httpCode << ", portal url:" << portalUrl;
        if (portalUrl.isEmpty()) {
            // if the portal is empty, I think it ok
            setConnectivity(network::service::Connectivity::Full);
        } else {
            setConnectivity(network::service::Connectivity::Portal);
        }
        setPortalUrl(portalUrl);
        break;
    }
    if (!m_isStop && !networkIsOk) {
        NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
        int disconnectCount = 0;
        for (NetworkManager::Device::Ptr device : devices) {
            if (device->state() == NetworkManager::Device::Disconnected || device->state() == NetworkManager::Device::Failed || device->state() == NetworkManager::Device::Unmanaged
                || device->state() == NetworkManager::Device::Unavailable) {
                disconnectCount++;
            }
        }
        qCDebug(DSM) << "Network is unreachabel, disconnect count:" << disconnectCount;
        setPortalUrl(QString());
        if (disconnectCount == devices.size()) {
            // 如果所有的网络设备都断开了，就默认让其变为断开的状态
            setConnectivity(network::service::Connectivity::Noconnectivity);
        } else {
            setConnectivity(network::service::Connectivity::Limited);
        }
    }
}

// 如果当前定时器没有激活，则立即执行请求，并把后续1s内的请求合并为一次请求
// 不是一个完美的方案，但是可以大大减少请求的次数（特别是待机唤醒的场景）。
void StatusChecker::startCheck()
{
    qCDebug(DSM) << "Start check connectivity, sender:"
                 << (sender() ? sender()->metaObject()->className() : "direct");
    if (!m_pendingCheckTimer->isActive()) {
        realStartCheck();
        m_pendingCheck = false;
        m_pendingCheckTimer->start();
    } else {
        qCDebug(DSM) << "Debounce timer is active, mark pending check";
        m_pendingCheck = true;
    }
}

void StatusChecker::onActiveConnectionChanged()
{
    qCInfo(DSM) << "Active connection changed";
    // VPN changed
    NetworkManager::ActiveConnection::List activeVpnConnections;
    NetworkManager::ActiveConnection::List activeConnections = NetworkManager::activeConnections();
    for (NetworkManager::ActiveConnection::Ptr activeConnection : activeConnections) {
        if (activeConnection->connection()->settings()->connectionType() != NetworkManager::ConnectionSettings::ConnectionType::Vpn)
            continue;

        activeVpnConnections << activeConnection;
    }

    if (activeVpnConnections.isEmpty())
        return;

    for (NetworkManager::ActiveConnection::Ptr activeVpnConnection : activeVpnConnections) {
        connect(activeVpnConnection.data(), &NetworkManager::ActiveConnection::stateChanged, this, [this](NetworkManager::ActiveConnection::State state) {
            if (state == NetworkManager::ActiveConnection::State::Activated || state == NetworkManager::ActiveConnection::State::Deactivated) {
                checkConnectivity();
            }
        });
    }
}

void StatusChecker::setConnectivity(const network::service::Connectivity &connectivity)
{
    qCDebug(DSM) << "connectivity changed" << static_cast<int>(connectivity);
    m_connectivity = connectivity;
    // 更新每个设备的联通性
    emit connectivityChanged(m_connectivity);

    // 根据实际情况，如果网络不通，就30秒刷新一次，网络通了，300秒刷新一次
    int interval = (connectivity == network::service::Connectivity::Full ?
                        SettingConfig::instance()->connectivityCheckInterval() * 1000 :
                        SettingConfig::instance()->connectivityIntervalWhenLimit() * 1000);
    qCDebug(DSM) << "check interval" << m_checkTimer->interval() << ",set interval" << interval;
    if (m_checkTimer->interval() != interval) {
        if (m_checkTimer->isActive())
            m_checkTimer->stop();

        m_checkTimer->setInterval(interval);
        m_checkTimer->start();
    }
}

void StatusChecker::setPortalUrl(const QString &portalUrl)
{
    qCDebug(DSM) << "pre url:" << m_portalUrl << "new url:" << portalUrl;
    if (m_portalUrl == portalUrl)
        return;

    // 每次检测时返回的认证连接可能不同，连接时会多次打开认证网站
    m_portalUrl = portalUrl;
    emit portalDetected(m_portalUrl);
}

void StatusChecker::initDefaultConnectivity()
{
    // 如果上一次检测的是网络连接受限或者需要认证，则无需设置初始值，否则会出现在使用过程种，本来是无法上网的状态，而设置了错误的初始值导致状态错误
    // 这个函数是为了在断开所有的连接后，再开启连接后设置其初始值
    if (m_connectivity != network::service::Connectivity::Noconnectivity && m_connectivity != network::service::Connectivity::Unknownconnectivity)
        return;

    NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    if (devices.isEmpty())
        return;

    bool isFull = false;
    int disconnectCount = 0;
    for (NetworkManager::Device::Ptr device : devices) {
        if (device->state() == NetworkManager::Device::Activated) {
            isFull = true;
        } else if (device->state() == NetworkManager::Device::Disconnected || device->state() == NetworkManager::Device::Failed || device->state() == NetworkManager::Device::Unmanaged
                   || device->state() == NetworkManager::Device::Unavailable) {
            disconnectCount++;
        }
    }
    if (isFull) {
        // 只要有一个网络已连接，就默认让其状态变为full状态
        setConnectivity(network::service::Connectivity::Full);
    } else if (disconnectCount == devices.size()) {
        // 如果所有的网络设备都断开了，就默认让其变为断开的状态
        setConnectivity(network::service::Connectivity::Noconnectivity);
    }
}

// 通过使用NM的方式来获取网络连通性
NMConnectionvityChecker::NMConnectionvityChecker(QObject *parent)
    : ConnectivityChecker(parent)
    , m_connectivity(network::service::Connectivity::Full)
{
    initMember();
    initConnection();
}

static network::service::Connectivity convertConnectivity(const NetworkManager::Connectivity &connectivity)
{
    switch (connectivity) {
    case NetworkManager::Connectivity::Full:
        return network::service::Connectivity::Full;
    case NetworkManager::Connectivity::Portal:
        return network::service::Connectivity::Portal;
    case NetworkManager::Connectivity::Limited:
        return network::service::Connectivity::Limited;
    case NetworkManager::Connectivity::NoConnectivity:
        return network::service::Connectivity::Noconnectivity;
    case NetworkManager::Connectivity::UnknownConnectivity:
        return network::service::Connectivity::Unknownconnectivity;
    }

    return network::service::Connectivity::Unknownconnectivity;
}

network::service::Connectivity NMConnectionvityChecker::connectivity() const
{
    return m_connectivity;
}

void NMConnectionvityChecker::initMember()
{
    m_connectivity = convertConnectivity(NetworkManager::connectivity());
}

void NMConnectionvityChecker::initConnection()
{
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::connectivityChanged, this, [this](NetworkManager::Connectivity connectivity) {
        m_connectivity = convertConnectivity(connectivity);
        emit connectivityChanged(m_connectivity);
    });
}
