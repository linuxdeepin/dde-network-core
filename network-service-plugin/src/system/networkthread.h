// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include "networkenabledconfig.h"

#include <NetworkManagerQt/Device>

#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusServiceWatcher>
#include <QMutex>
#include <QObject>

namespace network {
namespace systemservice {

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
    void onDeviceAdded(const QString &uni);
    void onDeviceRemoved(const QString &uni);
    void onDevicestateChanged(NetworkManager::Device::State newState, NetworkManager::Device::State oldState, NetworkManager::Device::StateChangeReason reason);
    void onInterfaceNameChanged();
    void addDevicesWithRetry();
    void onVpnStateChanged(const QDBusMessage &msg);
    void handleVpnStateChanged(uint state);
    QString doEnableDevice(const QString &pathOrIface, const bool enabled, QString &err);
    QString setPropVpnEnabled(bool enabled);
    QString enableDevice(NetworkManager::Device::Ptr device);
    QString disableDevice(NetworkManager::Device::Ptr device);

protected:
    void disableVpn();
    bool airplaneWifiEnabled();
    void restartIPWatchD();
    NetworkManager::Device::Ptr findDevice(QString pathOrIface);

    inline QDBusConnection dbusConnection() const { return m_dbusConnection; }

private:
    bool m_isInitialized;
    QThread *m_thread;
    QMutex m_mutex;
    QDBusConnection &m_dbusConnection;
    NetworkEnabledConfig *m_networkConfig;
    QDBusServiceWatcher *m_dbusService;
    QMap<QString, QString> m_devices;
    int m_count;
};
} // namespace systemservice
} // namespace network

#endif // NETWORKTHREAD_H
