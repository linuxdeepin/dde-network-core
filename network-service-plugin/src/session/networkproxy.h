// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKPROXY_H
#define NETWORKPROXY_H

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

class QGSettings;

namespace network {
namespace sessionservice {
class NetworkStateHandler;

class NetworkProxy : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Network1")
    Q_CLASSINFO("D-Bus Introspection",
                "<interface name='org.deepin.dde.Network1'>\n"
                "    <method name='GetProxyMethod'>\n"
                "        <arg name='proxyMode' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='SetProxyMethod'>\n"
                "        <arg name='proxyMode' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='GetAutoProxy'>\n"
                "        <arg name='proxyAuto' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='SetAutoProxy'>\n"
                "        <arg name='proxyAuto' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='GetProxyIgnoreHosts'>\n"
                "        <arg name='ignoreHosts' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='SetProxyIgnoreHosts'>\n"
                "        <arg name='ignoreHosts' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='GetProxy'>\n"
                "        <arg name='proxyType' type='s' direction='in'></arg>\n"
                "        <arg name='host' type='s' direction='out'></arg>\n"
                "        <arg name='port' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='SetProxy'>\n"
                "        <arg name='proxyType' type='s' direction='in'></arg>\n"
                "        <arg name='host' type='s' direction='in'></arg>\n"
                "        <arg name='port' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='GetProxyAuthentication'>\n"
                "        <arg name='proxyType' type='s' direction='in'></arg>\n"
                "        <arg name='user' type='s' direction='out'></arg>\n"
                "        <arg name='password' type='s' direction='out'></arg>\n"
                "        <arg name='enable' type='b' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='SetProxyAuthentication'>\n"
                "        <arg name='proxyType' type='s' direction='in'></arg>\n"
                "        <arg name='user' type='s' direction='in'></arg>\n"
                "        <arg name='password' type='s' direction='in'></arg>\n"
                "        <arg name='enable' type='b' direction='in'></arg>\n"
                "    </method>\n"
                "    <signal name='ProxyMethodChanged'>\n"
                "        <arg name='method' type='s'></arg>\n"
                "    </signal>\n"
                // 空实现
                "    <method name='ActivateAccessPoint'>\n"
                "        <arg name='uuid' type='s' direction='in'></arg>\n"
                "        <arg name='apPath' type='o' direction='in'></arg>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='connection' type='o' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='ActivateConnection'>\n"
                "        <arg name='uuid' type='s' direction='in'></arg>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='cpath' type='o' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='DeactivateConnection'>\n"
                "        <arg name='uuid' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='DebugChangeAPChannel'>\n"
                "        <arg name='band' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='DeleteConnection'>\n"
                "        <arg name='uuid' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='DisableWirelessHotspotMode'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='DisconnectDevice'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='EnableDevice'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='enabled' type='b' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='EnableWirelessHotspotMode'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='GetAccessPoints'>\n"
                "        <arg name='path' type='o' direction='in'></arg>\n"
                "        <arg name='apsJSON' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='GetActiveConnectionInfo'>\n"
                "        <arg name='acinfosJSON' type='s' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='GetSupportedConnectionTypes'>\n"
                "        <arg name='types' type='as' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='IsDeviceEnabled'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='enabled' type='b' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='IsWirelessHotspotModeEnabled'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='enabled' type='b' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='ListDeviceConnections'>\n"
                "        <arg name='devPath' type='o' direction='in'></arg>\n"
                "        <arg name='connections' type='ao' direction='out'></arg>\n"
                "    </method>\n"
                "    <method name='RequestIPConflictCheck'>\n"
                "        <arg name='ip' type='s' direction='in'></arg>\n"
                "        <arg name='ifc' type='s' direction='in'></arg>\n"
                "    </method>\n"
                "    <method name='RequestWirelessScan'></method>\n"
                "    <method name='SetDeviceManaged'>\n"
                "        <arg name='devPathOrIfc' type='s' direction='in'></arg>\n"
                "        <arg name='managed' type='b' direction='in'></arg>\n"
                "    </method>\n"
                "    <signal name='AccessPointAdded'>\n"
                "        <arg name='devPath' type='s'></arg>\n"
                "        <arg name='apJSON' type='s'></arg>\n"
                "    </signal>\n"
                "    <signal name='AccessPointRemoved'>\n"
                "        <arg name='devPath' type='s'></arg>\n"
                "        <arg name='apJSON' type='s'></arg>\n"
                "    </signal>\n"
                "    <signal name='AccessPointPropertiesChanged'>\n"
                "        <arg name='devPath' type='s'></arg>\n"
                "        <arg name='apJSON' type='s'></arg>\n"
                "    </signal>\n"
                "    <signal name='DeviceEnabled'>\n"
                "        <arg name='devPath' type='s'></arg>\n"
                "        <arg name='enabled' type='b'></arg>\n"
                "    </signal>\n"
                "    <signal name='ActiveConnectionInfoChanged'></signal>\n"
                "    <signal name='IPConflict'>\n"
                "        <arg name='ip' type='s'></arg>\n"
                "        <arg name='mac' type='s'></arg>\n"
                "    </signal>\n"
                "    <property name='WirelessAccessPoints' type='s' access='read'></property>\n"
                "    <property name='State' type='u' access='read'></property>\n"
                "    <property name='Connectivity' type='u' access='read'></property>\n"
                "    <property name='NetworkingEnabled' type='b' access='readwrite'></property>\n"
                "    <property name='VpnEnabled' type='b' access='readwrite'></property>\n"
                "    <property name='Devices' type='s' access='read'></property>\n"
                "    <property name='Connections' type='s' access='read'></property>\n"
                "    <property name='ActiveConnections' type='s' access='read'></property>\n"
                "</interface>\n")

    // 属性定义
    Q_PROPERTY(QString WirelessAccessPoints READ wirelessAccessPoints)
    Q_PROPERTY(uint State READ state)
    Q_PROPERTY(uint Connectivity READ connectivity)
    Q_PROPERTY(bool NetworkingEnabled READ networkingEnabled WRITE setNetworkingEnabled)
    Q_PROPERTY(bool VpnEnabled READ vpnEnabled WRITE setVpnEnabled)
    Q_PROPERTY(QString Devices READ devices)
    Q_PROPERTY(QString Connections READ connections)
    Q_PROPERTY(QString ActiveConnections READ activeConnections)
public:
    explicit NetworkProxy(QDBusConnection &dbusConnection, NetworkStateHandler *networkStateHandler, QObject *parent = nullptr);

    // 属性getter和setter方法
    QString wirelessAccessPoints() const;
    uint state() const;
    uint connectivity() const;
    bool networkingEnabled() const;
    void setNetworkingEnabled(bool enabled);
    bool vpnEnabled() const;
    void setVpnEnabled(bool enabled);
    QString devices() const;
    QString connections() const;
    QString activeConnections() const;
public Q_SLOTS:
    // GetProxyMethod get current proxy method, it would be "none", "manual" or "auto".
    QString GetProxyMethod();
    void SetProxyMethod(const QString &proxyMode);
    // GetAutoProxy get proxy PAC file URL for "auto" proxy mode, the
    // value will keep there even the proxy mode is not "auto".
    QString GetAutoProxy();
    void SetAutoProxy(const QString &proxyAuto);
    // GetProxyIgnoreHosts get the ignored hosts for proxy network which
    // is a string separated by ",".
    QString GetProxyIgnoreHosts();
    void SetProxyIgnoreHosts(const QString &ignoreHosts);
    // GetProxy get the host and port for target proxy type.
    void GetProxy(const QString &proxyType); // host, port string
    // SetProxy set host and port for target proxy type.
    void SetProxy(const QString &proxyType, const QString &host, const QString &port);
    void GetProxyAuthentication(const QString &proxyType); // user, password string, enable bool
    void SetProxyAuthentication(const QString &proxyType, const QString &user, const QString &password, bool enable);
    // 空实现
    QDBusObjectPath ActivateAccessPoint(const QString &uuid, const QDBusObjectPath &apPath, const QDBusObjectPath &devPath);
    QDBusObjectPath ActivateConnection(const QString &uuid, const QDBusObjectPath &devPath);
    void DeactivateConnection(const QString &uuid);
    void DebugChangeAPChannel(const QString &band);
    void DeleteConnection(const QString &uuid);
    void DisableWirelessHotspotMode(const QDBusObjectPath &devPath);
    void DisconnectDevice(const QDBusObjectPath &devPath);
    void EnableDevice(const QDBusObjectPath &devPath, bool enabled);
    void EnableWirelessHotspotMode(const QDBusObjectPath &devPath);
    QString GetAccessPoints(const QDBusObjectPath &path);
    QString GetActiveConnectionInfo();
    QStringList GetSupportedConnectionTypes();
    bool IsDeviceEnabled(const QDBusObjectPath &devPath);
    bool IsWirelessHotspotModeEnabled(const QDBusObjectPath &devPath);
    QList<QDBusObjectPath> ListDeviceConnections(const QDBusObjectPath &devPath);
    void RequestIPConflictCheck(const QString &ip, const QString &ifc);
    void RequestWirelessScan();
    void SetDeviceManaged(const QString &devPathOrIfc, bool managed);

Q_SIGNALS:
    void ProxyMethodChanged(const QString &proxyMode);

private Q_SLOTS:
    void onConfigChanged(const QString &key);

private:
    QGSettings *getProxyChildSettings(const QString &proxyType);

    inline QDBusConnection dbusConnection() const { return m_dbusConnection; }

private:
    QDBusConnection &m_dbusConnection;
    NetworkStateHandler *m_networkStateHandler;
    QGSettings *m_proxySettings;
    QGSettings *m_proxyChildSettingsHttp;
    QGSettings *m_proxyChildSettingsHttps;
    QGSettings *m_proxyChildSettingsFtp;
    QGSettings *m_proxyChildSettingsSocks;
};
} // namespace sessionservice
} // namespace network
#endif // NETWORKPROXY_H
