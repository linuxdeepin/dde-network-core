// SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include "networkenabledconfig.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Device>

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>
#include <QMap>
#include <QObject>

namespace network {
namespace systemservice {

// 设备启用/禁用的执行体。
//
// 本对象必须运行在 networkManagerThread() 上：它直接调用 NetworkManagerQt。
// NetworkThread 由 NetworkDBus 构造，构造点会先调用
// ensureNetworkManagerInitialized() 完成单例锚定。
class NetworkThread : public QObject
{
    Q_OBJECT
public:
    explicit NetworkThread(QDBusConnection &dbusConnection, QObject *parent = nullptr);
    ~NetworkThread() override;

    bool VpnEnabled() const;
    void setVpnEnabled(bool enabled);

public Q_SLOTS:
    QDBusObjectPath EnableDevice(const QString &pathOrIface, const bool enabled, const QDBusMessage &message);
    bool IsDeviceEnabled(const QString &pathOrIface, const QDBusMessage &message);
    void Ping(const QString &host, const QDBusMessage &message);
    bool ToggleWirelessEnabled(const QDBusMessage &message);

Q_SIGNALS:
    void DeviceEnabled(const QDBusObjectPath &devPath, bool enabled);

protected Q_SLOTS:
    void init();
    void onDevicesChanged();
    void onDeviceRemoved(const QString &uni);
    void onDeviceStateChanged(const QString &uni);
    void onInterfaceNameChanged(const QString &uni, const QString &oldIface);
    void onVpnStateChanged(const QDBusMessage &msg);
    void handleVpnStateChanged(uint state);
    QString doEnableDevice(const QString &pathOrIface, const bool enabled, QString &err);
    QString setPropVpnEnabled(bool enabled);
    QString enableDevice(const QString &uniOrIface);
    QString disableDevice(const QString &uniOrIface);

    void onStatusChanged(NetworkManager::Status status);
    void onAirplaneWifiEnabledResolved(bool enabled, const QString &interfaceName);

protected:
    void disableVpn();
    void requestAirplaneWifiEnabled(const QString &interfaceName);

    inline QDBusConnection dbusConnection() const { return m_dbusConnection; }

private:
    bool m_isInitialized;
    QDBusConnection &m_dbusConnection;
    NetworkEnabledConfig *m_networkConfig;
    QDBusServiceWatcher *m_dbusService;
    // uni -> 上次观察到的接口名；与 m_deviceStates 一起驱动设备集合的增量收敛
    QMap<QString, QString> m_devices;
    QMap<QString, int> m_deviceStates;
    int m_count;
};
} // namespace systemservice} // namespace network
#endif // NETWORKTHREAD_H