// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKSTATEHANDLER_H
#define NETWORKSTATEHANDLER_H

#include <NetworkManagerQt/Device>

#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QObject>

namespace network {
namespace sessionservice {
class NetworkStateHandler : public QObject
{
    Q_OBJECT
public:
    explicit NetworkStateHandler(QObject *parent = nullptr);

    enum {
        CUSTOM_NM_DEVICE_STATE_REASON_CABLE_UNPLUGGED = 1000,
        CUSTOM_NM_DEVICE_STATE_REASON_WIRELESS_DISABLED,
        CUSTOM_NM_DEVICE_STATE_REASON_MODEM_NO_SIGNAL,
        CUSTOM_NM_DEVICE_STATE_REASON_MODEM_WRONG_PLAN,
    };

    enum ConnectionType {
        Unknown = 0,
        Adsl,
        Bluetooth,
        Bond,
        Bridge,
        Cdma,
        Gsm,
        Infiniband,
        OLPCMesh,
        Pppoe,
        Vlan,
        Vpn,
        Wimax,
        Wired,
        Wireless,
        Team,
        Generic,
        Tun,
        IpTunnel,
        WireGuard,
        // 自定义细分类型
        WirelessAdhoc = 100,
        WirelessHotspot,
        VpnL2TP,
        VpnOpenconnect,
        VpnOpenvpn,
        VpnPptp,
        VpnStrongswan,
        VpnVpnc,
        VpnSstp,
    };

    struct DeviceStateInfo
    {
        bool enabled;
        QString devUdi;
        NetworkManager::Device::Type devType;
        QString aconnId;
        bool aconnHasEap;
        ConnectionType connectionType;
    };

    struct ActiveConnectionInfo
    {
        QString path;
        NetworkManager::ConnectionSettings::ConnectionType typ;
        bool vpnFailed;
        QString vpnType;
        uint vpnState;

        QStringList Devices;
        QString conn;
        QString Id;
        QString Uuid;
        uint State;
        bool Vpn;
        QString SpecificObject;
    };

public Q_SLOTS:
    void init();
    void onDBusStateChanged(const QDBusMessage &msg);
    void onDeviceAdded(const QString &uni);
    void onDeviceRemoved(const QString &uni);
    void onActiveConnectionAdded(const QString &path);
    void onActiveConnectionRemoved(const QString &path);
    void onStateChanged(const QString &devPath, NetworkManager::Device::State newState, NetworkManager::Device::State oldState, uint reason);
    void onDeviceEnabled(const QDBusObjectPath &path, bool enabled);
    void onVpnStateChanged(const QDBusMessage &msg);
    void handleVpnStateChanged(const QString &path, uint state, uint reason);
    void onNetworkingEnabledChanged(bool enabled);
    void onWirelessHardwareEnabledChanged(bool enabled);
    void notify(const QString &icon, const QString &summary, const QString &body);
    void onNotify(uint replacesId);
    void onHandleForSleep(bool sleep);

protected:
    bool isVirtualDeviceIfc(NetworkManager::Device::Ptr dev);
    bool isInDeviceWhitelist(const QString &filename, const QString &ifc);
    bool isDeviceTypeValid(NetworkManager::Device::Type devType);
    void queryDeviceEnabled(const QString &path);
    void *nmGetDeviceActiveConnectionData(NetworkManager::Device::Ptr dev);
    ConnectionType getCustomConnectionType(NetworkManager::ConnectionSettings *settings);
    QString generalGetNotifyConnectedIcon(NetworkManager::Device::Type devType, NetworkManager::Device::Ptr dev);
    QString generalGetNotifyDisconnectedIcon(NetworkManager::Device::Type devType, NetworkManager::Device::Ptr dev);
    void notifyAirplanModeEnabled();
    void notifyWirelessHardSwitchOff();
    void disableNotify();
    void enableNotify();
    void notifyVpnConnected(const QString &id);
    void notifyVpnDisconnected(const QString &id);
    void notifyVpnFailed(const QString &id, uint reason);

private:
    bool m_notifyEnabled;
    QDBusServiceWatcher *m_dbusService;
    QString m_lastConnection;
    QString m_lastConnectionUuid;
    NetworkManager::Device::State m_lastState;
    QMap<uint, QString> m_deviceErrorTable;
    QMap<uint, QString> m_vpnErrorTable;
    QMap<QString, DeviceStateInfo> m_devices;
    QMap<QString, ActiveConnectionInfo> m_activeConnections;
    uint m_replacesId;
};
} // namespace sessionservice
} // namespace network
#endif // NETWORKSTATEHANDLER_H
