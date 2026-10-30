// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "connectivitychecker.h"
#include "internetchecker.h"

#include "connectivityprober.h"
#include "nmaccess.h"
#include "settingconfig.h"

#include <NetworkManagerQt/Manager>

#include <QDebug>
#include <QThread>
#include <QTimer>

using namespace network::systemservice;

ConnectivityChecker::ConnectivityChecker(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<network::service::Connectivity>("network::service::Connectivity");
}

// ---------------------------------------------------------------------------
// LocalConnectionvityChecker
// ---------------------------------------------------------------------------

LocalConnectionvityChecker::LocalConnectionvityChecker(QObject *parent)
    : ConnectivityChecker(parent)
    , m_statusChecker(new StatusChecker)
    , m_connectivity(network::service::Connectivity::Unknownconnectivity)
    , m_probeThread(new QThread(this))
    , m_prober(new ConnectivityProber)
    , m_internetCheckerThread(nullptr)
    , m_internetChecker(nullptr)
{
    // 跨线程信号需要已注册的 metatype，否则 queued 连接会被静默丢弃
    qRegisterMetaType<ConnectivityProbeRequest>("ConnectivityProbeRequest");
    qRegisterMetaType<ConnectivityProbeResult>("ConnectivityProbeResult");

    // StatusChecker 直接调用 NetworkManagerQt，必须待在 NM 线程
    m_statusChecker->moveToThread(networkManagerThread());

    // HTTP 探测是同步阻塞调用，放在独立线程，避免堵死 NM 事件循环
    m_prober->moveToThread(m_probeThread);
    m_probeThread->start();

    connect(m_statusChecker, &StatusChecker::portalDetected, this, &LocalConnectionvityChecker::onPortalDetected);
    connect(m_statusChecker, &StatusChecker::connectivityChanged, this, &LocalConnectionvityChecker::onConnectivityChanged);
    connect(m_statusChecker, &StatusChecker::probeRequested, this, &LocalConnectionvityChecker::startProbe);
    connect(m_prober, &ConnectivityProber::finished, this, &LocalConnectionvityChecker::onProbeFinished);

    QMetaObject::invokeMethod(m_statusChecker, &StatusChecker::initConnectivityChecker, Qt::QueuedConnection);

    if (SettingConfig::instance()->needCheckNetwork()) {
        m_internetCheckerThread = new QThread(this);
        m_internetChecker = new InternetChecker;
        m_internetChecker->moveToThread(m_internetCheckerThread);
        m_internetCheckerThread->start();
        // 切换成功后立即重新检测连通性（跨线程自动排队到 NM 线程），
        // 否则要等周期性检查（最长 300s），状态会长时间停留在旧值
        connect(m_internetChecker, &InternetChecker::switchSuccess,
                m_statusChecker, &StatusChecker::checkConnectivity);
    }
}

LocalConnectionvityChecker::~LocalConnectionvityChecker()
{
    // StatusChecker 运行在 NM 线程，析构必须回到它的线程执行。
    // NM 线程常驻不退出，因此 deleteLater 能正常被处理。
    if (isNetworkManagerThread()) {
        m_statusChecker->stop();
        delete m_statusChecker;
    } else {
        QMetaObject::invokeMethod(m_statusChecker, [checker = m_statusChecker] {
            checker->stop();
            delete checker;
        }, Qt::BlockingQueuedConnection);
    }
    m_statusChecker = nullptr;

    // 探测线程即将退出，deleteLater 不会被执行，这里同步删除
    m_probeThread->quit();
    m_probeThread->wait();
    delete m_prober;
    m_prober = nullptr;

    if (m_internetCheckerThread) {
        m_internetCheckerThread->quit();
        m_internetCheckerThread->wait();
        delete m_internetChecker;
        m_internetChecker = nullptr;
    }
}

network::service::Connectivity LocalConnectionvityChecker::connectivity() const
{
    return m_connectivity;
}

QString LocalConnectionvityChecker::portalUrl() const
{
    if (!m_portalUrl.isEmpty()
            && NetworkManagerProxy::instance()->primaryConnectionUni() != m_portalPrimaryId) {
        // 主连接已经变了，旧认证地址不再适用
        return QString();
    }
    return m_portalUrl;
}

void LocalConnectionvityChecker::checkConnectivity()
{
    // m_statusChecker 在 NM 线程中执行，因此让其在自己的线程里运行
    QMetaObject::invokeMethod(m_statusChecker, &StatusChecker::checkConnectivity, Qt::QueuedConnection);
}

void LocalConnectionvityChecker::onPortalDetected(const QString &portalUrl)
{
    // 记录检测时所处的主连接；主连接一变，缓存的认证地址就过期了
    m_portalPrimaryId = NetworkManagerProxy::instance()->primaryConnectionUni();
    if (m_portalUrl == portalUrl)
        return;

    m_portalUrl = portalUrl;
    Q_EMIT portalDetected(m_portalUrl);
}

void LocalConnectionvityChecker::onConnectivityChanged(network::service::Connectivity connectivity)
{
    if (m_internetChecker && connectivity != network::service::Connectivity::Full) {
        const bool checkPrimary = m_statusChecker->primaryConnectionChanged();
        const bool switchPortal = connectivity != network::service::Connectivity::Portal;
        // 捕获指针而非 this：本对象可能先于排队调用被销毁
        QMetaObject::invokeMethod(m_internetChecker, [checker = m_internetChecker, checkPrimary, switchPortal]() {
            checker->switchInternetAccess(checkPrimary, switchPortal);
        }, Qt::QueuedConnection);
    }
    qCInfo(DSM) << "Connectivity changed, incomming: " << static_cast<int>(connectivity) << ", current: " << static_cast<int>(m_connectivity);
    if (m_connectivity == connectivity)
        return;

    m_connectivity = connectivity;
    Q_EMIT connectivityChanged(connectivity);
}

void LocalConnectionvityChecker::startProbe(const ConnectivityProbeRequest &request)
{
    QMetaObject::invokeMethod(m_prober, [prober = m_prober, request] {
        prober->probe(request);
    }, Qt::QueuedConnection);
}

void LocalConnectionvityChecker::onProbeFinished(const ConnectivityProbeResult &result)
{
    // 探测结果回到 NM 线程判定连通性；陈旧结果由 StatusChecker 丢弃
    QMetaObject::invokeMethod(m_statusChecker, [checker = m_statusChecker, result] {
        checker->onProbeResult(result);
    }, Qt::QueuedConnection);
}

// ---------------------------------------------------------------------------
// StatusChecker（全程运行在 NM 线程）
// ---------------------------------------------------------------------------

StatusChecker::StatusChecker(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
    , m_timer(new QTimer(this))
    , m_pendingCheckTimer(new QTimer(this))
    , m_pendingCheck(false)
    , m_connectivity(network::service::Connectivity::Unknownconnectivity)
    , m_checkCount(0)
    , m_isStop(false)
    , m_primaryConnectionChanged(false)
    , m_probeGeneration(0)
{
    qRegisterMetaType<NetworkManager::ActiveConnection::State>("NetworkManager::ActiveConnection::State");
    qRegisterMetaType<NetworkManager::Device::State>("NetworkManager::Device::State");
    qRegisterMetaType<NetworkManager::Device::StateChangeReason>("NetworkManager::Device::StateChangeReason");

    // 避免短时间内频繁请求网址，1s内的多次请求都是无意义的
    m_pendingCheckTimer->setSingleShot(true);
    m_pendingCheckTimer->setInterval(1000);  // 1秒防抖    connect(m_pendingCheckTimer, &QTimer::timeout, this, [this] {
        if (m_pendingCheck)
            startProbeCheck();
        m_pendingCheck = false;
    });
}

StatusChecker::~StatusChecker()
{
    for (QMetaObject::Connection connection : m_checkerConnection) {
        disconnect(connection);
    }
    m_checkerConnection.clear();
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
    NMQT_ASSERT_THREAD();
    qCDebug(DSM()) << "Check connectivity";
    m_checkCount = 0;
    initDefaultConnectivity();
    if (m_timer->isActive()) {
        startCheck();
    } else {
        m_timer->start();
    }
}

bool StatusChecker::primaryConnectionChanged() const
{
    return m_primaryConnectionChanged;
}

void StatusChecker::initConnectivityChecker()
{
    NMQT_ASSERT_THREAD();
    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();

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
    m_checkerConnection << connect(proxy, &NetworkManagerProxy::devicesChanged, this, &StatusChecker::onDevicesChanged);
    m_checkerConnection << connect(proxy, &NetworkManagerProxy::primaryConnectionChanged, this, &StatusChecker::onPrimaryConnectionChanged);
    // VPN 等活跃连接的建立/拆除同样需要触发检测（旧实现由 activeConnectionsChanged 驱动）
    m_checkerConnection << connect(proxy, &NetworkManagerProxy::activeConnectionsChanged, this, &StatusChecker::onActiveConnectionsChanged);
    initDefaultConnectivity();

    // 第一次进来的时候执行一次网络状态的检测
    startCheck();
}

void StatusChecker::stop()
{
    m_isStop = true;
    if (m_checkTimer && m_checkTimer->isActive())
        m_checkTimer->stop();
    if (m_timer && m_timer->isActive())
        m_timer->stop();
    if (m_pendingCheckTimer && m_pendingCheckTimer->isActive())
        m_pendingCheckTimer->stop();
}

// 以下三个槽收敛了原实现里分别直连 Device::stateChanged /
// ActiveConnection::activeConnectionChanged / Notifier::activeConnectionsChanged
// 的多条路径，逐条对齐原有语义：
//   - 设备状态变化：原先是 startCheck()，立刻发起一次探测（1 秒防抖）；
//   - 活跃连接集合变化：原先只重置计数、修正默认状态并启动 2 秒轮询定时器。
// 若统一成“只启动定时器”，连通性会推迟约 2 秒才刷新。
void StatusChecker::onDevicesChanged()
{
    NMQT_ASSERT_THREAD();
    startCheck();
}

void StatusChecker::onActiveConnectionsChanged()
{
    NMQT_ASSERT_THREAD();
    m_checkCount = 0;
    initDefaultConnectivity();
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

// 主连接变化总是伴随活跃连接集合变化，这条与原 activeConnectionsChanged 路径同义
void StatusChecker::onPrimaryConnectionChanged(const QString &uni)
{
    Q_UNUSED(uni)
    onActiveConnectionsChanged();
}

void StatusChecker::onUpdateUrls(const QStringList &urls)
{
    m_checkUrls = urls;
}

// 探测在独立线程完成，这里只负责发起与判定
void StatusChecker::startProbeCheck()
{
    NMQT_ASSERT_THREAD();
    if (m_isStop) {
        qCDebug(DSM) << "Stop check connectivity";
        return;
    }

    // 记录探测前的主连接，用于判定探测期间是否发生切换
    const NetworkManagerProxy *proxy = NetworkManagerProxy::instance();
    m_primaryId = proxy->primaryConnectionUni();

    ConnectivityProbeRequest request;
    request.urls = m_checkUrls;
    request.timeoutSec = SettingConfig::instance()->httpRequestTimeout();
    request.generation = ++m_probeGeneration;
    Q_EMIT probeRequested(request);
}

void StatusChecker::onProbeResult(const ConnectivityProbeResult &result)
{
    NMQT_ASSERT_THREAD();
    if (result.generation != m_probeGeneration) {
        qCDebug(DSM) << "drop stale probe result" << result.generation << m_probeGeneration;
        return;
    }
    if (m_isStop) {
        qCDebug(DSM) << "Stop check connectivity";
        return;
    }

    NetworkManagerProxy *proxy = NetworkManagerProxy::instance();

    // HTTP 检查完成后，再获取当前主连接，以检测检查期间是否发生了连接切换。
    // 必须先于 setConnectivity() 更新，否则信号消费者读到的是上一轮的结果。
    const QString currentPrimaryId = proxy->primaryConnectionUni();
    m_primaryConnectionChanged = (m_primaryId != currentPrimaryId);
    m_primaryId = currentPrimaryId;

    if (result.networkIsOk) {
        if (result.portalUrl.isEmpty()) {
            setConnectivity(network::service::Connectivity::Full);
        } else {
            setConnectivity(network::service::Connectivity::Portal);
        }
        setPortalUrl(result.portalUrl);
        return;
    }

    const QList<DeviceSnapshot> devices = proxy->devices();
    int disconnectCount = 0;
    for (const DeviceSnapshot &device : devices) {
        if (device.state == NetworkManager::Device::Disconnected
            || device.state == NetworkManager::Device::Failed
            || device.state == NetworkManager::Device::Unmanaged
            || device.state == NetworkManager::Device::Unavailable) {
            disconnectCount++;
        }
    }
    qCDebug(DSM) << "Network is unreachabel, disconnect count:" << disconnectCount;
    setPortalUrl(QString());
    if (disconnectCount == devices.size()) {
        setConnectivity(network::service::Connectivity::Noconnectivity);
    } else {
        setConnectivity(network::service::Connectivity::Limited);
    }
}

// 如果当前定时器没有激活，则立即发起探测，并把后续1s内的请求合并为一次
// 不是一个完美的方案，但是可以大大减少请求的次数（特别是待机唤醒的场景）。
void StatusChecker::startCheck()
{
    NMQT_ASSERT_THREAD();
    qCDebug(DSM) << "Start check connectivity, sender:"
                 << (sender() ? sender()->metaObject()->className() : "direct");
    if (!m_pendingCheckTimer->isActive()) {
        startProbeCheck();
        m_pendingCheck = false;
        m_pendingCheckTimer->start();
    } else {
        qCDebug(DSM) << "Debounce timer is active, mark pending check";
        m_pendingCheck = true;
    }
}

void StatusChecker::setConnectivity(const network::service::Connectivity &connectivity)
{
    qCDebug(DSM) << "connectivity changed" << static_cast<int>(connectivity);
    m_connectivity = connectivity;
    // 更新每个设备的联通性
    Q_EMIT connectivityChanged(m_connectivity);

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
    Q_EMIT portalDetected(m_portalUrl);
}

void StatusChecker::initDefaultConnectivity()
{
    NMQT_ASSERT_THREAD();
    // 如果上一次检测的是网络连接受限或者需要认证，则无需设置初始值，否则会出现在使用过程种，本来是无法上网的状态，而设置了错误的初始值导致状态错误
    // 这个函数是为了在断开所有的连接后，再开启连接后设置其初始值
    if (m_connectivity != network::service::Connectivity::Noconnectivity && m_connectivity != network::service::Connectivity::Unknownconnectivity)
        return;

    const QList<DeviceSnapshot> devices = NetworkManagerProxy::instance()->devices();
    if (devices.isEmpty())
        return;

    bool isFull = false;
    int disconnectCount = 0;
    for (const DeviceSnapshot &device : devices) {
        if (device.state == NetworkManager::Device::Activated) {
            isFull = true;
        } else if (device.state == NetworkManager::Device::Disconnected
                   || device.state == NetworkManager::Device::Failed
                   || device.state == NetworkManager::Device::Unmanaged
                   || device.state == NetworkManager::Device::Unavailable) {
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

// ---------------------------------------------------------------------------
// NMConnectionvityChecker（全程运行在 NM 线程）
// ---------------------------------------------------------------------------

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

NMConnectionvityChecker::NMConnectionvityChecker(QObject *parent)
    : ConnectivityChecker(parent)
    , m_connectivity(static_cast<int>(network::service::Connectivity::Full))
{
    moveToThread(networkManagerThread());
    // 构造点可能不在 NM 线程，NM 访问排队到该线程执行
    QMetaObject::invokeMethod(this, &NMConnectionvityChecker::initMember, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &NMConnectionvityChecker::initConnection, Qt::QueuedConnection);
}

network::service::Connectivity NMConnectionvityChecker::connectivity() const
{
    return static_cast<network::service::Connectivity>(m_connectivity.load());
}

void NMConnectionvityChecker::initMember()
{
    NMQT_ASSERT_THREAD();
    m_connectivity.store(static_cast<int>(convertConnectivity(NetworkManager::connectivity())));
}

void NMConnectionvityChecker::initConnection()
{
    NMQT_ASSERT_THREAD();
    // 库信号只在代理内部 connect；这里消费代理的自有信号
    connect(NetworkManagerProxy::instance(), &NetworkManagerProxy::connectivityChanged, this, [this](NetworkManager::Connectivity connectivity) {
        const auto converted = convertConnectivity(connectivity);
        m_connectivity.store(static_cast<int>(converted));
        Q_EMIT connectivityChanged(converted);
    });
}
