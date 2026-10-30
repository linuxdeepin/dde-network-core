// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "nmaccess.h"

#include "settingconfig.h"

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/IpConfig>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Ipv6Setting>
#include <NetworkManagerQt/WiredDevice>
#include <NetworkManagerQt/WirelessSetting>
#include <NetworkManagerQt/WirelessDevice>

#include <QHash>
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include <atomic>
#include <unistd.h>
#include <iterator>
#include <functional>
#include <mutex>

namespace network {
namespace systemservice {

namespace {

// NM 线程的宿主对象。QMetaObject::invokeMethod 需要一个亲和性在目标线程上的
// context 对象，QThread 自身亲和于创建它的线程，不能用它做 context。
class ManagerThreadHost : public QObject
{
public:
    explicit ManagerThreadHost(QThread *thread) { moveToThread(thread); }
};

// 只有 system 进程（root）有独立的 NM 访问线程。非 root 的 session 插件里
// NM Qt 的全部使用者本来就在插件线程上，引入第二条线程反而会制造跨线程访问。
bool isSystemProcess()
{
    static const bool system = (::geteuid() == 0);
    return system;
}

// 非 root（session 插件）进程的 NM 归属线程：就是首次初始化所在的线程
// （即插件线程）。记录一次，之后不再变化，哨兵仍然有意义。
QThread *sessionManagerThread()
{
    static QThread *thread = QThread::currentThread();
    return thread;
}

// NM 线程的启动点。函数内 static：不参与静态析构顺序，且刻意泄漏——
// NM Qt 单例与它的 QThreadData 都锚定在这条线程上，线程退出后无法迁移，
// 而库内静态对象可能在线程结束后仍访问 QThreadData。
QThread *ensureManagerThread()
{
    static QThread *thread = [] {
        QThread *t = new QThread();
        t->setObjectName(QStringLiteral("nmqt-access"));
        t->start();
        return t;
    }();
    return thread;
}

// 必须先于任何 invokeMethod 创建，且其亲和性在 NM 线程上
ManagerThreadHost *managerThreadHost()
{
    static ManagerThreadHost *host = new ManagerThreadHost(ensureManagerThread());
    return host;
}

std::once_flag g_initOnce;
NetworkManagerProxy *g_proxy = nullptr;
QMutex g_proxyMutex;

NetworkManagerProxy *createProxy()
{
    NetworkManagerProxy *proxy = new NetworkManagerProxy();
    QMutexLocker locker(&g_proxyMutex);
    g_proxy = proxy;
    return proxy;
}

QStringList ipv4AddressesOf(const NetworkManager::Device::Ptr &dev)
{
    QStringList result;
    const NetworkManager::IpConfig config = dev->ipV4Config();
    for (const NetworkManager::IpAddress &address : config.addresses()) {
        const QString ip = address.ip().toString();
        if (!ip.isEmpty()) {
            result << ip;
        }
    }
    return result;
}

DeviceSnapshot makeSnapshot(const NetworkManager::Device::Ptr &dev)
{
    DeviceSnapshot snapshot;
    if (dev.isNull()) {
        return snapshot;
    }

    snapshot.uni = dev->uni();
    snapshot.interfaceName = dev->interfaceName();
    snapshot.type = dev->type();
    snapshot.state = dev->state();
    snapshot.managed = dev->managed();
    snapshot.autoconnect = dev->autoconnect();
    snapshot.interfaceFlags = dev->interfaceFlags();
    snapshot.ipV4Addresses = ipv4AddressesOf(dev);

    const NetworkManager::ActiveConnection::Ptr activeConnection = dev->activeConnection();
    if (!activeConnection.isNull()) {
        snapshot.activeConnectionUni = activeConnection->path();
        const NetworkManager::IpConfig ipV4 = activeConnection->ipV4Config();
        for (const QHostAddress &address : ipV4.nameservers()) {
            const QString dns = address.toString();
            if (!dns.isEmpty() && !snapshot.dnsServers.contains(dns)) {
                snapshot.dnsServers << dns;
            }
        }
    }

    // 硬件地址只有有线和无线设备提供，其它类型留空
    if (dev->type() == NetworkManager::Device::Ethernet) {
        const NetworkManager::WiredDevice::Ptr wired = dev.staticCast<NetworkManager::WiredDevice>();
        snapshot.hardwareAddress = wired->hardwareAddress();
        snapshot.permanentHardwareAddress = wired->permanentHardwareAddress();
    } else if (dev->type() == NetworkManager::Device::Wifi) {
        const NetworkManager::WirelessDevice::Ptr wireless = dev.staticCast<NetworkManager::WirelessDevice>();
        snapshot.hardwareAddress = wireless->hardwareAddress();
        snapshot.permanentHardwareAddress = wireless->permanentHardwareAddress();
    }

    return snapshot;
}

} // namespace
bool DeviceSnapshot::operator==(const DeviceSnapshot &other) const
{
    return uni == other.uni
            && interfaceName == other.interfaceName
            && hardwareAddress == other.hardwareAddress
            && permanentHardwareAddress == other.permanentHardwareAddress
            && type == other.type
            && state == other.state
            && managed == other.managed
            && autoconnect == other.autoconnect
            && interfaceFlags == other.interfaceFlags
            && activeConnectionUni == other.activeConnectionUni
            && ipV4Addresses == other.ipV4Addresses
            && dnsServers == other.dnsServers;
}

QThread *networkManagerThread()
{
    if (isSystemProcess()) {
        return ensureManagerThread();
    }
    return sessionManagerThread();
}

QObject *networkManagerContext()
{
    if (isSystemProcess()) {
        return managerThreadHost();
    }
    // session 进程只有一条线程在用 NM Qt，宿主对象就建在那条线程上
    static ManagerThreadHost *host = new ManagerThreadHost(sessionManagerThread());
    return host;
}

bool isNetworkManagerThread()
{
    return QThread::currentThread() == networkManagerThread();
}

// 不变式：代理必须在任何 NM 线程对象存在之前创建完成（ServiceFactory 在构造
// 任何 system 组件前调用本函数）。否则 NM 线程可能已在 call_once 内部等待，
// 而调用方阻塞在 BlockingQueuedConnection 上 → 互等。
void ensureNetworkManagerInitialized()
{
    std::call_once(g_initOnce, [] {
        // session 进程直接在调用线程上创建；system 进程同步到 NM 线程创建
        if (!isSystemProcess() || isNetworkManagerThread()) {
            createProxy();
            return;
        }
        // 同步在 NM 线程上创建代理：NM Qt 单例随之锚定在该线程。
        // 该线程的事件循环在 ensureManagerThread() 中已启动，不会死锁。
        QMetaObject::invokeMethod(
                managerThreadHost(),
                [] { createProxy(); },
                Qt::BlockingQueuedConnection);
    });
}

NetworkManagerProxy *NetworkManagerProxy::instance()
{
    // 必须先完成线程锚定，否则单例可能落到调用方线程上
    ensureNetworkManagerInitialized();

    QMutexLocker locker(&g_proxyMutex);
    return g_proxy;
}

NetworkManagerProxy::NetworkManagerProxy()
    : QObject(nullptr)
{
    NMQT_ASSERT_THREAD();
    prime();
}

NetworkManagerProxy::~NetworkManagerProxy() = default;

void NetworkManagerProxy::prime()
{
    if (m_primed) {
        return;
    }
    m_primed = true;
    NMQT_ASSERT_THREAD();

    // 库信号只在这里 connect 一次；调用方一律使用本类的自有信号
    NetworkManager::Notifier *notifier = NetworkManager::notifier();
    m_notifierConnections
            << connect(notifier, &NetworkManager::Notifier::deviceAdded, this, &NetworkManagerProxy::onDeviceAdded)
            << connect(notifier, &NetworkManager::Notifier::deviceRemoved, this, &NetworkManagerProxy::onDeviceRemoved)
            << connect(notifier, &NetworkManager::Notifier::connectivityChanged, this, [this](NetworkManager::Connectivity connectivity) {
                   {
                       QWriteLocker locker(&m_lock);
                       m_connectivity = connectivity;
                   }
                   Q_EMIT connectivityChanged(connectivity);
               })
            << connect(notifier, &NetworkManager::Notifier::activeConnectionsChanged, this, [this] {
                   // 活跃连接集合变化会带来主连接变更，一并刷新
                   refreshPrimaryConnection();
                   Q_EMIT activeConnectionsChanged();
               })
            << connect(notifier, &NetworkManager::Notifier::statusChanged, this, [this](NetworkManager::Status status) {
                   Q_EMIT statusChanged(status);
               })
            << connect(notifier, &NetworkManager::Notifier::primaryConnectionChanged, this, [this](const QString &uni) {
                   // 设备列表随主连接变化，一并刷新
                   refreshPrimaryConnection();
                   Q_EMIT primaryConnectionChanged(uni);
               })
            // NM 重启后 Device 对象会重建，旧连接记录必须整体丢弃
            << connect(notifier, &NetworkManager::Notifier::serviceDisappeared, this, [this] {
                   qCInfo(DSM()) << "NetworkManager disappeared, dropping cached state";
                   clearDeviceConnections();
                   {
                       QWriteLocker locker(&m_lock);
                       m_devices.clear();
                       m_primaryConnectionUni.clear();
                       m_primaryConnectionDevices.clear();
                       m_connectivity = NetworkManager::UnknownConnectivity;
                   }
                   Q_EMIT devicesChanged();
                   Q_EMIT primaryConnectionChanged(QString());
               })
            << connect(notifier, &NetworkManager::Notifier::serviceAppeared, this, [this] {
                   qCInfo(DSM()) << "NetworkManager appeared, rebuilding cache";
                   clearDeviceConnections();
                   refreshDevices();
                   refreshConnectivity();
                   refreshPrimaryConnection();
               });

    refreshDevices();
    refreshConnectivity();
    refreshPrimaryConnection();
}


void NetworkManagerProxy::refreshConnectivity()
{
    NMQT_ASSERT_THREAD();
    const NetworkManager::Connectivity connectivity = NetworkManager::connectivity();
    QWriteLocker locker(&m_lock);
    m_connectivity = connectivity;
}

void NetworkManagerProxy::refreshPrimaryConnection()
{
    NMQT_ASSERT_THREAD();
    const NetworkManager::ActiveConnection::Ptr primary = NetworkManager::primaryConnection();
    const QString uni = primary.isNull() ? QString() : primary->path();
    const QStringList devices = primary.isNull() ? QStringList() : primary->devices();
    QWriteLocker locker(&m_lock);
    m_primaryConnectionUni = uni;
    m_primaryConnectionDevices = devices;
}

void NetworkManagerProxy::refreshDevices()
{
    NMQT_ASSERT_THREAD();

    QList<DeviceSnapshot> snapshots;
    const NetworkManager::Device::List interfaces = NetworkManager::networkInterfaces();
    snapshots.reserve(interfaces.size());
    for (const NetworkManager::Device::Ptr &dev : interfaces) {
        if (dev.isNull()) {
            continue;
        }
        snapshots << makeSnapshot(dev);

        // Device 的属性变化不经过 Notifier，需要在设备对象上单独跟踪
        const QString uni = dev->uni();
        if (!m_deviceConnections.contains(uni)) {
            m_deviceConnections.insert(
                    uni,
                    QList<QMetaObject::Connection>()
                            << connect(dev.data(), &NetworkManager::Device::stateChanged, this, &NetworkManagerProxy::onDevicePropertyChanged)
                            << connect(dev.data(), &NetworkManager::Device::interfaceNameChanged, this, &NetworkManagerProxy::onDevicePropertyChanged)
                            << connect(dev.data(), &NetworkManager::Device::activeConnectionChanged, this, &NetworkManagerProxy::onDevicePropertyChanged)
                            << connect(dev.data(), &NetworkManager::Device::managedChanged, this, &NetworkManagerProxy::onDevicePropertyChanged)
                            << connect(dev.data(), &NetworkManager::Device::ipV4AddressChanged, this, &NetworkManagerProxy::onDevicePropertyChanged)
                            // DNS 取自活跃连接的 IPv4 配置，配置变化不经过 ipV4AddressChanged
                            << connect(dev.data(), &NetworkManager::Device::ipV4ConfigChanged, this, &NetworkManagerProxy::onDevicePropertyChanged));
        }
    }

    // 设备消失时若没收到 deviceRemoved（例如 NM 侧异常），连接记录会悬垂，
    // 这里按当前设备集合对齐
    QSet<QString> alive;
    alive.reserve(snapshots.size());
    for (const DeviceSnapshot &snapshot : snapshots) {
        alive.insert(snapshot.uni);
    }
    for (auto it = m_deviceConnections.begin(); it != m_deviceConnections.end();) {
        if (alive.contains(it.key())) {
            ++it;
            continue;
        }
        for (const QMetaObject::Connection &connection : it.value()) {
            disconnect(connection);
        }
        it = m_deviceConnections.erase(it);
    }

    bool changed = false;
    {
        QWriteLocker locker(&m_lock);
        changed = (m_devices != snapshots);
        m_devices = snapshots;
    }
    if (changed) {
        Q_EMIT devicesChanged();
    }
}

void NetworkManagerProxy::requestActiveConnectionId(const QString &activeConnectionUni, bool isWireless)
{
    if (activeConnectionUni.isEmpty()) {
        return;
    }

    runOnManagerThread([this, activeConnectionUni, isWireless] {
        NMQT_ASSERT_THREAD();
        QString saveId;
        const NetworkManager::ActiveConnection::Ptr connection =
                NetworkManager::findActiveConnection(activeConnectionUni);
        if (!connection.isNull() && !connection->connection().isNull()) {
            const NetworkManager::ConnectionSettings::Ptr settings = connection->connection()->settings();
            if (isWireless) {
                // 无线网络用 SSID 作为保存 ID
                const NetworkManager::WirelessSetting::Ptr wireless =
                        settings->setting(NetworkManager::Setting::SettingType::Wireless)
                                .dynamicCast<NetworkManager::WirelessSetting>();
                if (wireless) {
                    saveId = QString::fromUtf8(wireless->ssid());
                }
            } else {
                saveId = settings->id();
            }
        }
        Q_EMIT activeConnectionIdReady(activeConnectionUni, saveId);
    });
}

void NetworkManagerProxy::clearDeviceConnections()
{
    for (auto it = m_deviceConnections.begin(); it != m_deviceConnections.end(); ++it) {
        for (const QMetaObject::Connection &connection : it.value()) {
            disconnect(connection);
        }
    }
    m_deviceConnections.clear();
}

void NetworkManagerProxy::onDeviceAdded(const QString &uni)
{
    Q_UNUSED(uni)
    NMQT_ASSERT_THREAD();
    refreshDevices();
}

void NetworkManagerProxy::onDeviceRemoved(const QString &uni)
{
    NMQT_ASSERT_THREAD();
    const QList<QMetaObject::Connection> connections = m_deviceConnections.take(uni);
    for (const QMetaObject::Connection &connection : connections) {
        disconnect(connection);
    }
    refreshDevices();
}

void NetworkManagerProxy::onDevicePropertyChanged()
{
    NMQT_ASSERT_THREAD();
    refreshDevices();
}

DeviceSnapshot NetworkManagerProxy::device(const QString &uniOrIface) const
{
    QReadLocker locker(&m_lock);
    for (const DeviceSnapshot &snapshot : m_devices) {
        if (snapshot.uni == uniOrIface || snapshot.interfaceName == uniOrIface) {
            return snapshot;
        }
    }
    return DeviceSnapshot();
}

QList<DeviceSnapshot> NetworkManagerProxy::devices() const
{
    QReadLocker locker(&m_lock);
    return m_devices;
}

QString NetworkManagerProxy::primaryConnectionUni() const
{
    QReadLocker locker(&m_lock);
    return m_primaryConnectionUni;
}

QStringList NetworkManagerProxy::primaryConnectionDevices() const
{
    QReadLocker locker(&m_lock);
    return m_primaryConnectionDevices;
}

NetworkManager::Connectivity NetworkManagerProxy::connectivity() const
{
    QReadLocker locker(&m_lock);
    return m_connectivity;
}

void NetworkManagerProxy::runOnManagerThread(std::function<void()> &&task)
{
    if (isNetworkManagerThread()) {
        task();
        return;
    }
    QMetaObject::invokeMethod(this, std::move(task), Qt::QueuedConnection);
}

void NetworkManagerProxy::setWirelessEnabled(bool enabled)
{
    runOnManagerThread([enabled] {
        NMQT_ASSERT_THREAD();
        NetworkManager::setWirelessEnabled(enabled);
    });
}

void NetworkManagerProxy::setNetworkingEnabled(bool enabled)
{
    runOnManagerThread([enabled] {
        NMQT_ASSERT_THREAD();
        NetworkManager::setNetworkingEnabled(enabled);
    });
}

void NetworkManagerProxy::activateConnection(const QString &connectionUni, const QString &deviceUni)
{
    // NM 的 D-Bus 签名要求对象路径，空串会构造出非法 QDBusObjectPath；
    // 沿用调用方原有约定，用 "/" 表示“不指定设备 / 连接参数”
    const QString normalizedDevice = deviceUni.isEmpty() ? QStringLiteral("/") : deviceUni;
    runOnManagerThread([connectionUni, normalizedDevice] {
        NMQT_ASSERT_THREAD();
        NetworkManager::activateConnection(connectionUni, normalizedDevice, QStringLiteral("/"));
    });
}

void NetworkManagerProxy::deactivateConnection(const QString &activeConnectionUni)
{
    runOnManagerThread([activeConnectionUni] {
        NMQT_ASSERT_THREAD();
        NetworkManager::deactivateConnection(activeConnectionUni);
    });
}

void NetworkManagerProxy::applyPrimaryDeviceMetrics(const QString &primaryUni)
{
    runOnManagerThread([primaryUni] {
        NMQT_ASSERT_THREAD();

        // 目标网卡必须真的已激活：否则下面会把其它网卡统一降到 secondaryMetric，
        // 反而让所有路由都被降级（原实现在这里直接返回 false 不改任何配置）。
        bool targetActivated = false;
        for (const NetworkManager::Device::Ptr &dev : NetworkManager::networkInterfaces()) {
            if (dev->uni() != primaryUni) {
                continue;
            }
            const NetworkManager::ActiveConnection::Ptr ac = dev->activeConnection();
            targetActivated = !ac.isNull() && ac->state() == NetworkManager::ActiveConnection::Activated;
            break;
        }
        if (!targetActivated) {
            qCWarning(DSM()) << "primary device is not activated, skip metrics:" << primaryUni;
            return;
        }

        const quint32 primaryMetric = 100;
        const quint32 secondaryMetric = 1000;
        QMap<NetworkManager::Device::Ptr, NMVariantMapMap> deviceSettings;

        const NetworkManager::Device::List allDevices = NetworkManager::networkInterfaces();
        for (const NetworkManager::Device::Ptr &dev : allDevices) {
            const NetworkManager::ActiveConnection::Ptr ac = dev->activeConnection();
            if (ac.isNull() || ac->state() != NetworkManager::ActiveConnection::Activated)
                continue;

            const quint32 metric = (dev->uni() == primaryUni) ? primaryMetric : secondaryMetric;
            const NetworkManager::Connection::Ptr conn = ac->connection();
            if (conn.isNull())
                continue;
            NetworkManager::ConnectionSettings::Ptr settings = conn->settings();
            if (settings.isNull())
                continue;

            bool needReapply = false;
            NetworkManager::Ipv4Setting::Ptr ipv4 = settings->setting(NetworkManager::Setting::Ipv4).staticCast<NetworkManager::Ipv4Setting>();
            if (ipv4) {
                ipv4->setRouteMetric(metric);
                ipv4->setInitialized(true);
                needReapply = true;
            }

            NetworkManager::Ipv6Setting::Ptr ipv6 = settings->setting(NetworkManager::Setting::Ipv6).staticCast<NetworkManager::Ipv6Setting>();
            if (ipv6) {
                ipv6->setRouteMetric(metric);
                ipv6->setInitialized(true);
                needReapply = true;
            }

            if (needReapply) {
                conn->update(settings->toMap());
                deviceSettings[dev] = conn->settings()->toMap();
            }
        }

        if (deviceSettings.isEmpty()) {
            qCWarning(DSM()) << "can't found device for change route";
            return;
        }

        // Reapply 签名: (a{sa{sv}}tu) — settings_map, version_id(0), flags
        const uint flags = static_cast<uint>(SettingConfig::instance()->reapplyFlags());
        for (auto it = deviceSettings.constBegin(); it != deviceSettings.constEnd(); ++it) {
            it.key()->reapplyConnection(it.value(), 0, flags);
        }
    });
}

} // namespace systemservice} // namespace network