// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NMACCESS_H
#define NMACCESS_H

#include "constants.h"

#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Utils>

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMetaType>
#include <QObject>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
#include <QThread>

#include <functional>

class QMutex;

namespace network {
namespace systemservice {

/**
 * @brief NetworkManagerQt 的进程级访问线程。
 *
 * libKF6NetworkManagerQt 的内部设备表是无锁容器，由库自身的 D-Bus 连接在
 * “单例所属线程”的事件循环中增删。任何其它线程读取它都会与那条线程的无锁
 * 写入并发，加锁无法保护（写方在库内部，不持有调用方的锁）。
 *
 * 因此本进程内所有 NetworkManagerQt 调用都必须发生在该线程上。
 *
 * 该线程刻意不退出、不析构：NM Qt 单例与 QThreadData 一旦锚定即无法迁移，
 * 线程退出后残留对象仍可能被库内静态析构访问。
 */
QThread *networkManagerThread();

/// NM 线程上的宿主对象，作为 QMetaObject::invokeMethod 的 context。
/// QThread 自身亲和于创建它的线程，不能拿它当 context（会死锁）。
QObject *networkManagerContext();

/// 当前线程是否为 NM 访问线程。
bool isNetworkManagerThread();

/**
 * @brief 在 NM 线程上建立 NM Qt 单例并填充代理缓存。
 *
 * 必须在任何 NetworkManager:: 调用之前执行一次：单例被锚定在“首次触碰”
 * 的线程上，先于插件构造完成锚定，才能保证后续所有访问落在同一条线程。
 */
void ensureNetworkManagerInitialized();

/**
 * @brief 设备快照（纯值对象）。
 *
 * 可在任意线程读写副本；跨线程传递不进 NetworkManagerQt 内部结构。
 */
struct DeviceSnapshot
{
    QString uni;
    QString interfaceName;
    QString hardwareAddress;
    QString permanentHardwareAddress;
    NetworkManager::Device::Type type = NetworkManager::Device::UnknownType;
    NetworkManager::Device::State state = NetworkManager::Device::UnknownState;
    bool managed = false;
    bool autoconnect = false;
    quint32 interfaceFlags = 0;
    QString activeConnectionUni;
    QStringList ipV4Addresses;
    QStringList dnsServers;      // 来自活跃连接的 IPv4 配置

    bool isValid() const { return !uni.isEmpty(); }
    bool operator==(const DeviceSnapshot &other) const;
    bool operator!=(const DeviceSnapshot &other) const { return !(*this == other); }
};

/**
 * @brief NetworkManagerQt 的唯一入口与设备快照缓存。
 *
 * - 缓存由 NM 线程在信号驱动下刷新，读方只取副本；
 * - 写操作统一经本对象排队到 NM 线程执行；
 * - 库的 Notifier / Device 信号只在本对象内部 connect 一次，对外只发自有信号。
 *
 * 缓存自身用 QReadWriteLock 保护：这里保护的是本进程自己的容器，与
 * “库内部无锁容器不能跨线程”不是同一件事，也是必要的。
 */
class NetworkManagerProxy : public QObject
{
    Q_OBJECT
public:
    static NetworkManagerProxy *instance();

    // ---- 缓存读取（任意线程）----
    DeviceSnapshot device(const QString &uniOrIface) const;
    QList<DeviceSnapshot> devices() const;
    QString primaryConnectionUni() const;
    // 主连接（ActiveConnection）占用的设备 uni 列表。注意 primaryConnectionUni()
    // 返回的是 ActiveConnection 路径，不能拿来当设备标识查 device()。
    QStringList primaryConnectionDevices() const;
    NetworkManager::Connectivity connectivity() const;

    // ---- 写操作（任意线程；已在 NM 线程时同步执行）----
    void setWirelessEnabled(bool enabled);
    void setNetworkingEnabled(bool enabled);
    void activateConnection(const QString &connectionUni, const QString &deviceUni);
    void deactivateConnection(const QString &activeConnectionUni);
    // 通过调整路由 metric 把 primaryUni 设为首选连接（其余网卡降级）。
    // 全部写入在 NM 线程完成，调用方只传一个设备标识。
    void applyPrimaryDeviceMetrics(const QString &primaryUni);
    /// 计算活跃连接对应的保存 ID：无线取 SSID，有线取连接 id。
    /// 结果经 activeConnectionIdReady() 在 NM 线程发出；空串表示当前取不到。
    void requestActiveConnectionId(const QString &activeConnectionUni, bool isWireless);

Q_SIGNALS:
    void devicesChanged();
    void connectivityChanged(NetworkManager::Connectivity connectivity);
    void primaryConnectionChanged(const QString &uni);
    // 活跃连接集合变化（含 VPN 建立/拆除）。替代调用方直接 connect
    // NetworkManager::Notifier::activeConnectionsChanged。
    void activeConnectionsChanged();
    void activeConnectionIdReady(const QString &activeConnectionUni, const QString &saveId);
    // NM 整体状态（连接/断开）。替代调用方直接 connect
    // NetworkManager::Notifier::statusChanged。
    void statusChanged(NetworkManager::Status status);

public:
    // 仅供 ensureNetworkManagerInitialized() 在 NM 线程上调用
    NetworkManagerProxy();
    ~NetworkManagerProxy() override;

private:

    void prime();
    void refreshDevices();
    void refreshConnectivity();
    void refreshPrimaryConnection();
    void onDeviceAdded(const QString &uni);
    void onDeviceRemoved(const QString &uni);
    void onDevicePropertyChanged();
    /// 丢弃所有 Device 上的信号连接（NM 重启或缓存重建时使用）。
    void clearDeviceConnections();

    /// 在 NM 线程上执行（必要时排队）。
    void runOnManagerThread(std::function<void()> &&task);

    mutable QReadWriteLock m_lock;
    QList<DeviceSnapshot> m_devices;
    QString m_primaryConnectionUni;
    QStringList m_primaryConnectionDevices;
    NetworkManager::Connectivity m_connectivity = NetworkManager::UnknownConnectivity;
    QList<QMetaObject::Connection> m_notifierConnections;
    QHash<QString, QList<QMetaObject::Connection>> m_deviceConnections;
    bool m_primed = false;
};

} // namespace systemservice
} // namespace network

Q_DECLARE_METATYPE(network::systemservice::DeviceSnapshot)

/**
 * @brief 错线程访问哨兵。
 *
 * 每个直接调用 NetworkManager:: 的位置都应插入本宏：违反单线程不变量时
 * 打一条 warning 并带上函数名，便于线上定位回归。
 */
#define NMQT_ASSERT_THREAD()                                                       \
    do {                                                                           \
        if (!network::systemservice::isNetworkManagerThread()) {                    \
            qCWarning(DSM()) << "NM Qt wrong thread:" << __func__                   \
                             << QThread::currentThread();                          \
        }                                                                          \
    } while (0)

#endif // NMACCESS_H
