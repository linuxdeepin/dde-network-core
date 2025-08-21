// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKDBUS_H
#define NETWORKDBUS_H

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

namespace network {
namespace systemservice {
const QString NETWORK_SYSTEM_DAEMON_SERVICE = "org.deepin.dde.Network1";
const QString NETWORK_SYSTEM_DAEMON_PATH = "/org/deepin/dde/Network1";
#define NETWORK_SYSTEM_DAEMON_INTERFACE_STRING "org.deepin.dde.Network1"
const QString NETWORK_SYSTEM_DAEMON_INTERFACE = NETWORK_SYSTEM_DAEMON_INTERFACE_STRING;

class NetworkThread;

class NetworkDBus : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", NETWORK_SYSTEM_DAEMON_INTERFACE_STRING)
    Q_PROPERTY(bool VpnEnabled READ VpnEnabled WRITE setVpnEnabled)
public:
    explicit NetworkDBus(QDBusConnection &dbusConnection, QObject *parent = nullptr);
    ~NetworkDBus() override;

    bool VpnEnabled() const;
    void setVpnEnabled(bool enabled);

public Q_SLOTS:
    QDBusObjectPath EnableDevice(const QString &pathOrIface, const bool enabled);
    bool IsDeviceEnabled(const QString &pathOrIface);
    void Ping(const QString &host);
    bool ToggleWirelessEnabled();

Q_SIGNALS:
    void DeviceEnabled(const QDBusObjectPath &devPath, bool enabled);

private:
    QScopedPointer<NetworkThread> m_networkThread;
};
} // namespace systemservice
} // namespace network

#endif // NETWORKDBUS_H
