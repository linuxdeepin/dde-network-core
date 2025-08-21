// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkstatehandler.h"

#include "constants.h"
#include "settingconfig.h"

#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/ModemDevice>
#include <NetworkManagerQt/Security8021xSetting>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/VpnConnection>
#include <NetworkManagerQt/VpnSetting>
#include <NetworkManagerQt/WimaxDevice>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/WirelessSetting>

#include <QDBusConnection>
#include <QFile>
#include <QTimer>
using namespace NetworkManager;

namespace network {
namespace sessionservice {
const QString devWhitelistHuaweiFile = "/lib/vendor/interface";
const QString ServiceTypeL2TP = "org.freedesktop.NetworkManager.l2tp";
const QString ServiceTypePPTP = "org.freedesktop.NetworkManager.pptp";
const QString ServiceTypeVPNC = "org.freedesktop.NetworkManager.vpnc";
const QString ServiceTypeOpenVPN = "org.freedesktop.NetworkManager.openvpn";
const QString ServiceTypeStrongSwan = "org.freedesktop.NetworkManager.strongswan";
const QString ServiceTypeOpenConnect = "org.freedesktop.NetworkManager.openconnect";
const QString ServiceTypeSSTP = "org.freedesktop.NetworkManager.sstp";

const QString notifyIconNetworkOffline = "notification-network-offline";
const QString notifyIconWiredConnected = "notification-network-wired-connected";
const QString notifyIconWiredDisconnected = "notification-network-wired-disconnected";
const QString notifyIconWiredError = notifyIconWiredDisconnected;
const QString notifyIconWirelessConnected = "notification-network-wireless-full";
const QString notifyIconWirelessDisconnected = "notification-network-wireless-disconnected";
const QString notifyIconWirelessDisabled = "notification-network-wireless-disabled";
const QString notifyIconWirelessError = notifyIconWirelessDisconnected;
const QString notifyIconVpnConnected = "notification-network-vpn-connected";
const QString notifyIconVpnDisconnected = "notification-network-vpn-disconnected";
const QString notifyIconNetworkConnected = notifyIconWiredConnected;
const QString notifyIconNetworkDisconnected = notifyIconWiredDisconnected;
const QString notifyIconMobile2gConnected = "notification-network-mobile-2g-connected";
const QString notifyIconMobile2gDisconnected = "notification-network-mobile-2g-disconnected";
const QString notifyIconMobile3gConnected = "notification-network-mobile-3g-connected";
const QString notifyIconMobile3gDisconnected = "notification-network-mobile-3g-disconnected";
const QString notifyIconMobile4gConnected = "notification-network-mobile-4g-connected";
const QString notifyIconMobile4gDisconnected = "notification-network-mobile-4g-disconnected";
const QString notifyIconMobileUnknownConnected = "notification-network-mobile-unknown-connected";
const QString notifyIconMobileUnknownDisconnected = "notification-network-mobile-unknown-disconnected";

NetworkStateHandler::NetworkStateHandler(QObject *parent)
    : QObject(parent)
    , m_notifyEnabled(true)
    , m_dbusService(new QDBusServiceWatcher("org.freedesktop.NetworkManager", QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForOwnerChange, this))
    , m_replacesId(0)
{
    connect(m_dbusService, &QDBusServiceWatcher::serviceRegistered, this, &NetworkStateHandler::init);
    QDBusConnection::systemBus().connect("org.deepin.dde.Network1", "/org/deepin/dde/Network1", "org.deepin.dde.Network1", "DeviceEnabled", this, SLOT(onDeviceEnabled(QDBusObjectPath, bool)));
    QDBusConnection::systemBus().connect("org.freedesktop.NetworkManager", "", "org.freedesktop.NetworkManager.VPN.Connection", "VpnStateChanged", this, SLOT(onVpnStateChanged(QDBusMessage)));
    // 迁移dde-daemon/session/power1/sleep_inhibit.go ConnectHandleForSleep处理
    QDBusConnection::systemBus().connect("org.deepin.dde.Daemon1", "/org/deepin/dde/Daemon1", "org.deepin.dde.Daemon1", "HandleForSleep", this, SLOT(onHandleForSleep(bool)));
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner");
    message << "org.freedesktop.NetworkManager";
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(init()));
}

void NetworkStateHandler::init()
{
    if (m_deviceErrorTable.isEmpty()) {
        // device error table
        m_deviceErrorTable[Device::UnknownReason] = tr("Device state changed");
        m_deviceErrorTable[Device::NoReason] = tr("Device state changed, reason unknown");
        m_deviceErrorTable[Device::NowManagedReason] = tr("The device is now managed");
        m_deviceErrorTable[Device::NowUnmanagedReason] = tr("The device is no longer managed");
        m_deviceErrorTable[Device::ConfigFailedReason] = tr("The device has not been ready for configuration");
        m_deviceErrorTable[Device::ConfigUnavailableReason] = tr("IP configuration could not be reserved (no available address, timeout, etc)");
        m_deviceErrorTable[Device::ConfigExpiredReason] = tr("The IP configuration is no longer valid");
        m_deviceErrorTable[Device::NoSecretsReason] = tr("Passwords were required but not provided");
        m_deviceErrorTable[Device::AuthSupplicantDisconnectReason] = tr("The 802.1X supplicant disconnected from the access point or authentication server");
        m_deviceErrorTable[Device::AuthSupplicantConfigFailedReason] = tr("Configuration of the 802.1X supplicant failed");
        m_deviceErrorTable[Device::AuthSupplicantFailedReason] = tr("The 802.1X supplicant quitted or failed unexpectedly");
        m_deviceErrorTable[Device::AuthSupplicantTimeoutReason] = tr("The 802.1X supplicant took too long time to authenticate");
        m_deviceErrorTable[Device::PppStartFailedReason] = tr("The PPP service failed to start within the allowed time");
        m_deviceErrorTable[Device::PppDisconnectReason] = tr("The PPP service disconnected unexpectedly");
        m_deviceErrorTable[Device::PppFailedReason] = tr("The PPP service quitted or failed unexpectedly");
        m_deviceErrorTable[Device::DhcpStartFailedReason] = tr("The DHCP service failed to start within the allowed time");
        m_deviceErrorTable[Device::DhcpErrorReason] = tr("The DHCP service reported an unexpected error");
        m_deviceErrorTable[Device::DhcpFailedReason] = tr("The DHCP service quitted or failed unexpectedly");
        m_deviceErrorTable[Device::SharedStartFailedReason] = tr("The shared connection service failed to start");
        m_deviceErrorTable[Device::SharedFailedReason] = tr("The shared connection service quitted or failed unexpectedly");
        m_deviceErrorTable[Device::AutoIpStartFailedReason] = tr("The AutoIP service failed to start");
        m_deviceErrorTable[Device::AutoIpErrorReason] = tr("The AutoIP service reported an unexpected error");
        m_deviceErrorTable[Device::AutoIpFailedReason] = tr("The AutoIP service quitted or failed unexpectedly");
        m_deviceErrorTable[Device::ModemBusyReason] = tr("Dialing failed due to busy lines");
        m_deviceErrorTable[Device::ModemNoDialToneReason] = tr("Dialing failed due to no dial tone");
        m_deviceErrorTable[Device::ModemNoCarrierReason] = tr("Dialing failed due to the carrier");
        m_deviceErrorTable[Device::ModemDialTimeoutReason] = tr("Dialing timed out");
        m_deviceErrorTable[Device::ModemDialFailedReason] = tr("Dialing failed");
        m_deviceErrorTable[Device::ModemInitFailedReason] = tr("Modem initialization failed");
        m_deviceErrorTable[Device::GsmApnSelectFailedReason] = tr("Failed to select the specified GSM APN");
        m_deviceErrorTable[Device::GsmNotSearchingReason] = tr("No networks searched");
        m_deviceErrorTable[Device::GsmRegistrationDeniedReason] = tr("Network registration was denied");
        m_deviceErrorTable[Device::GsmRegistrationTimeoutReason] = tr("Network registration timed out");
        m_deviceErrorTable[Device::GsmRegistrationFailedReason] = tr("Failed to register to the requested GSM network");
        m_deviceErrorTable[Device::GsmPinCheckFailedReason] = tr("PIN check failed");
        m_deviceErrorTable[Device::FirmwareMissingReason] = tr("Necessary firmware for the device may be missed");
        m_deviceErrorTable[Device::DeviceRemovedReason] = tr("The device was removed");
        m_deviceErrorTable[Device::SleepingReason] = tr("NetworkManager went to sleep");
        m_deviceErrorTable[Device::ConnectionRemovedReason] = tr("The device's active connection was removed or disappeared");
        m_deviceErrorTable[Device::UserRequestedReason] = tr("A user or client requested to disconnect");
        m_deviceErrorTable[Device::CarrierReason] = tr("The device's carrier/link changed"); // TODO translate
        m_deviceErrorTable[Device::ConnectionAssumedReason] = tr("The device's existing connection was assumed");
        m_deviceErrorTable[Device::SupplicantAvailableReason] = tr("The 802.1x supplicant is now available"); // TODO translate: full stop
        m_deviceErrorTable[Device::ModemNotFoundReason] = tr("The modem could not be found");
        m_deviceErrorTable[Device::BluetoothFailedReason] = tr("The Bluetooth connection timed out or failed");
        m_deviceErrorTable[Device::GsmSimNotInserted] = tr("GSM Modem's SIM Card was not inserted");
        m_deviceErrorTable[Device::GsmSimPinRequired] = tr("GSM Modem's SIM PIN required");
        m_deviceErrorTable[Device::GsmSimPukRequired] = tr("GSM Modem's SIM PUK required");
        m_deviceErrorTable[Device::GsmSimWrong] = tr("SIM card error in GSM Modem");
        m_deviceErrorTable[Device::InfiniBandMode] = tr("InfiniBand device does not support connected mode");
        m_deviceErrorTable[Device::DependencyFailed] = tr("A dependency of the connection failed");
        m_deviceErrorTable[Device::Br2684Failed] = tr("RFC 2684 Ethernet bridging error to ADSL"); // TODO translate
        m_deviceErrorTable[Device::ModemManagerUnavailable] = tr("ModemManager did not run or quitted unexpectedly");
        m_deviceErrorTable[Device::SsidNotFound] = tr("The 802.11 WLAN network could not be found");
        m_deviceErrorTable[Device::SecondaryConnectionFailed] = tr("A secondary connection of the base connection failed");

        // works for nm 1.0+
        m_deviceErrorTable[Device::DcbFcoeFailed] = tr("DCB or FCoE setup failed");
        m_deviceErrorTable[Device::TeamdControlFailed] = tr("Network teaming control failed");
        m_deviceErrorTable[Device::ModemFailed] = tr("Modem failed to run or not available");
        m_deviceErrorTable[Device::ModemAvailable] = tr("Modem now ready and available");
        m_deviceErrorTable[Device::SimPinIncorrect] = tr("SIM PIN is incorrect");
        m_deviceErrorTable[Device::NewActivation] = tr("New connection activation is enqueuing");
        m_deviceErrorTable[Device::ParentChanged] = tr("Parent device changed");
        m_deviceErrorTable[Device::ParentManagedChanged] = tr("Management status of parent device changed");

        // device error table for custom state reasons
        m_deviceErrorTable[CUSTOM_NM_DEVICE_STATE_REASON_CABLE_UNPLUGGED] = tr("Network cable is unplugged");
        m_deviceErrorTable[CUSTOM_NM_DEVICE_STATE_REASON_MODEM_NO_SIGNAL] = tr("Please make sure SIM card has been inserted with mobile network signal");
        m_deviceErrorTable[CUSTOM_NM_DEVICE_STATE_REASON_MODEM_WRONG_PLAN] = tr("Please make sure a correct plan was selected without arrearage of SIM card");

        // vpn error table
        m_vpnErrorTable[VpnConnection::UnknownReason] = tr("Failed to activate VPN connection, reason unknown");
        m_vpnErrorTable[VpnConnection::NoneReason] = tr("Failed to activate VPN connection");
        m_vpnErrorTable[VpnConnection::UserDisconnectedReason] = tr("The VPN connection state changed due to being disconnected by users");
        m_vpnErrorTable[VpnConnection::DeviceDisconnectedReason] = tr("The VPN connection state changed due to being disconnected from devices");
        m_vpnErrorTable[VpnConnection::ServiceStoppedReason] = tr("VPN service stopped");
        m_vpnErrorTable[VpnConnection::IpConfigInvalidReason] = tr("The IP config of VPN connection was invalid");
        m_vpnErrorTable[VpnConnection::ConnectTimeoutReason] = tr("The connection attempt to VPN service timed out");
        m_vpnErrorTable[VpnConnection::ServiceStartTimeoutReason] = tr("The VPN service start timed out");
        m_vpnErrorTable[VpnConnection::ServiceStartFailedReason] = tr("The VPN service failed to start");
        m_vpnErrorTable[VpnConnection::NoSecretsReason] = tr("The VPN connection password was not provided");
        m_vpnErrorTable[VpnConnection::LoginFailedReason] = tr("Authentication to VPN server failed");
        m_vpnErrorTable[VpnConnection::ConnectionRemovedReason] = tr("The connection was deleted from settings");

        QDBusConnection::systemBus().connect("org.freedesktop.NetworkManager", "", "org.freedesktop.NetworkManager.Device", "StateChanged", this, SLOT(onDBusStateChanged(QDBusMessage)));
        auto notifier = NetworkManager::notifier();
        connect(notifier, &NetworkManager::Notifier::deviceAdded, this, &NetworkStateHandler::onDeviceAdded);
        connect(notifier, &NetworkManager::Notifier::deviceRemoved, this, &NetworkStateHandler::onDeviceRemoved);
        connect(notifier, &NetworkManager::Notifier::networkingEnabledChanged, this, &NetworkStateHandler::onNetworkingEnabledChanged);
        connect(notifier, &NetworkManager::Notifier::wirelessHardwareEnabledChanged, this, &NetworkStateHandler::onWirelessHardwareEnabledChanged);
        connect(notifier, &NetworkManager::Notifier::activeConnectionAdded, this, &NetworkStateHandler::onActiveConnectionAdded);
        connect(notifier, &NetworkManager::Notifier::activeConnectionRemoved, this, &NetworkStateHandler::onActiveConnectionRemoved);
    }
    m_devices.clear();
    auto list = NetworkManager::networkInterfaces();
    for (auto dev : list) {
        onDeviceAdded(dev->uni());
    }
    m_activeConnections.clear();
    auto aList = NetworkManager::activeConnectionsPaths();
    for (auto aConn : aList) {
        onActiveConnectionAdded(aConn);
    }
}

void NetworkStateHandler::onDBusStateChanged(const QDBusMessage &msg)
{
    auto args = msg.arguments();
    NetworkManager::Device::State newState = NetworkManager::Device::State(args.at(0).toUInt());
    NetworkManager::Device::State oldState = NetworkManager::Device::State(args.at(1).toUInt());
    NetworkManager::Device::StateChangeReason reason = NetworkManager::Device::StateChangeReason(args.at(2).toUInt());
    qCDebug(DSM()) << msg.path() << "dev state changed" << oldState << "=>" << newState << reason;
    onStateChanged(msg.path(), newState, oldState, reason);
}

void NetworkStateHandler::onDeviceAdded(const QString &uni)
{
    auto device = NetworkManager::findNetworkInterface(uni);
    if (!device) {
        return;
    }
    // dont notify message when virtual device state changed
    if (isVirtualDeviceIfc(device)) {
        return;
    }
    auto deviceType = device->type();
    if (!isDeviceTypeValid(deviceType)) {
        return;
    }
    DeviceStateInfo stateInfo;
    stateInfo.devType = deviceType;
    stateInfo.devUdi = device->udi();
    stateInfo.aconnHasEap = false;
    stateInfo.enabled = true;
    queryDeviceEnabled(uni);
    void *settings = nmGetDeviceActiveConnectionData(device);
    if (settings) {
        // remember active connection id and type if exists
        ConnectionSettings *connSettings = static_cast<ConnectionSettings *>(settings);
        stateInfo.aconnId = connSettings->id();
        stateInfo.connectionType = getCustomConnectionType(connSettings);
    }
    m_devices.insert(uni, stateInfo);
}

void NetworkStateHandler::onDeviceRemoved(const QString &uni)
{
    m_devices.remove(uni);
}

void NetworkStateHandler::onActiveConnectionAdded(const QString &path)
{
    auto aConn = NetworkManager::findActiveConnection(path);
    if (!aConn) {
        return;
    }
    ActiveConnectionInfo aConnInfo;
    aConnInfo.path = path;
    aConnInfo.conn = aConn->connection()->path();
    aConnInfo.State = aConn->state();
    aConnInfo.Devices = aConn->devices();
    aConnInfo.typ = aConn->type();
    aConnInfo.Uuid = aConn->uuid();
    aConnInfo.Vpn = aConn->vpn();

    if (aConn->connection()) {
        aConnInfo.Id = aConn->connection()->name();
        VpnSetting::Ptr vpnSetting = aConn->connection()->settings()->setting(Setting::Vpn).dynamicCast<VpnSetting>();
        if (vpnSetting) {
            aConnInfo.vpnType = vpnSetting->serviceType();
        }
    }
    aConnInfo.SpecificObject = aConn->specificObject();

    m_activeConnections.insert(path, aConnInfo);
}

void NetworkStateHandler::onActiveConnectionRemoved(const QString &path)
{
    m_activeConnections.remove(path);
}

void NetworkStateHandler::onStateChanged(const QString &devPath, NetworkManager::Device::State newState, NetworkManager::Device::State oldState, uint reason)
{
    if (!m_devices.contains(devPath)) {
        return;
    }
    auto device = NetworkManager::findNetworkInterface(devPath);
    if (!device) {
        return;
    }
    qCDebug(DSM()) << device->interfaceName() << "device state changed," << oldState << "=>" << newState << ", reason" << reason << m_deviceErrorTable[reason];
    QString id;

    DeviceStateInfo &dsi = m_devices[devPath];
    NetworkManager::ActiveConnection::Ptr conn = device->activeConnection();
    if (!conn.isNull()) {
        id = conn->id();
        dsi.connectionType = getCustomConnectionType(conn->connection()->settings().get());
    }
    if (!id.isEmpty() && id != "/") {
        dsi.aconnId = id;
    }
    if (dsi.aconnId.isEmpty()) {
        // the device already been removed
        return;
    }

    switch (newState) {
    case Device::State::Preparing: { // 正在连接
        if (SettingConfig::instance()->disableFailureNotify()) {
            // 如果禁用了失败的消息，则不提示正在连接的消息
            return;
        }
        if (conn) {
            Security8021xSetting::Ptr sSetting = conn->connection()->settings()->setting(Setting::SettingType::Security8021x).staticCast<Security8021xSetting>();
            if (sSetting) {
                dsi.aconnHasEap = !sSetting->eapMethods().isEmpty();
            }
            QString icon = generalGetNotifyDisconnectedIcon(dsi.devType, device);
            qCDebug(DSM()) << "--------[Prepare] Active connection info:" << dsi.aconnId << dsi.connectionType << device->uni();
            if (dsi.connectionType == ConnectionType::WirelessHotspot) {
                notify(icon, "", tr("Enabling hotspot"));
            } else {
                // 防止连接状态由60变40再次弹出正在连接的通知消息
                if (oldState == Device::Disconnected) {
                    notify(icon, "", tr("Connecting \"%1\"").arg(dsi.aconnId));
                }
            }
        }
    } break;
    case Device::State::Activated: { // 连接成功
        QString icon = generalGetNotifyConnectedIcon(dsi.devType, device);
        qCDebug(DSM()) << "--------[Activated] Active connection info:" << dsi.aconnId << dsi.connectionType << device->uni();
        if (dsi.connectionType == ConnectionType::WirelessHotspot) {
            notify(icon, "", tr("Hotspot enabled"));
        } else {
            notify(icon, "", tr("\"%1\" connected").arg(dsi.aconnId));
        }
    } break;
    case Device::State::Failed:
    case Device::State::Disconnected:
    case Device::State::NeedAuth:
    case Device::State::Unmanaged:
    case Device::State::Unavailable: {
        qCInfo(DSM()) << QString("device disconnected, type %1, %2 => %3, reason[%4] %5").arg(dsi.devType).arg(oldState).arg(newState).arg(reason).arg(m_deviceErrorTable[reason]);
        if (SettingConfig::instance()->disableFailureNotify()) {
            // 如果禁用了失败的消息，则不提示失败消息
            return;
        }
        // ignore device removed signals for that could not
        // query related information correct
        if (reason == Device::StateChangeReason::DeviceRemovedReason) {
            if (dsi.connectionType == ConnectionType::WirelessHotspot) {
                QString icon = generalGetNotifyDisconnectedIcon(dsi.devType, device);
                notify(icon, "", tr("Hotspot disabled"));
            }
            return;
        }

        // ignore if device's old state is not available
        if (oldState <= Device::State::Unavailable) {
            qCDebug(DSM()) << "No notify, old state is not available, old state: " << oldState;
            return;
        }

        // notify only when network enabled
        if (!NetworkManager::isNetworkingEnabled()) {
            qCDebug(DSM()) << "no notify, network disabled";
            return;
        }
        // notify only when device enabled
        if (oldState == Device::Disconnected && !dsi.enabled) {
            qCDebug(DSM()) << "no notify, notify only when device enabled";
            return;
        }

        // fix reasons
        switch (dsi.devType) {
        case Device::Ethernet:
            if (reason == Device::CarrierReason) {
                reason = CUSTOM_NM_DEVICE_STATE_REASON_CABLE_UNPLUGGED;
            }
            break;
        case Device::Modem:
            if (reason == Device::StateChangeReason::UnknownReason) {
                // TODO: go转c++ 未找到对应处理
                // // mobile device is specially, fix its reasons here
                // signalQuality, _ := mmGetModemDeviceSignalQuality(dbus.ObjectPath(dsi.devUdi))
                // if signalQuality == 0 {
                //     reason = CUSTOM_NM_DEVICE_STATE_REASON_MODEM_NO_SIGNAL
                // } else {
                //     reason = CUSTOM_NM_DEVICE_STATE_REASON_MODEM_WRONG_PLAN
                // }
            }
        default:
            break;
        }

        // ignore invalid reasons
        if (reason == Device::StateChangeReason::UnknownReason) {
            qCDebug(DSM()) << "No notify, device state reason invalid, reason: " << reason;
            return;
        }

        qCDebug(DSM()) << "--------[Disconnect] Active connection info:" << dsi.aconnId << dsi.connectionType << device->uni();
        QString icon;
        QString msg;
        icon = generalGetNotifyDisconnectedIcon(dsi.devType, device);
        switch (reason) {
        case Device::StateChangeReason::UnknownReason:
        case Device::StateChangeReason::UserRequestedReason:
        case Device::StateChangeReason::ConnectionRemovedReason:
            if (newState == Device::Disconnected || (oldState == Device::Activated && newState == Device::Unavailable)) {
                if (dsi.connectionType == NetworkStateHandler::WirelessHotspot) {
                    notify(icon, "", tr("Hotspot disabled"));
                } else {
                    msg = tr("\"%1\" disconnected").arg(dsi.aconnId);
                }
            }
            break;
        case Device::StateChangeReason::NewActivation:
            break;
        case Device::StateChangeReason::ConfigUnavailableReason:
            switch (dsi.connectionType) {
            case NetworkStateHandler::WirelessHotspot:
                msg = tr("Unable to share hotspot, please check dnsmasq settings");
                break;
            case NetworkStateHandler::Wireless:
                msg = tr("Unable to connect \"%1\", please keep closer to the wireless router").arg(dsi.aconnId);
                break;
            case NetworkStateHandler::Wired:
                msg = tr("Unable to connect \"%1\", please check your router or net cable.").arg(dsi.aconnId);
                break;
            default:
                break;
            }
            break;
        case Device::StateChangeReason::AuthSupplicantDisconnectReason:
            if ((oldState == Device::ConfiguringHardware || oldState == Device::Activated) && newState == Device::NeedAuth) {
                msg = tr("Connection failed, unable to connect \"%1\", wrong password").arg(dsi.aconnId);
            } else {
                msg = tr("Unable to connect \"%1\"").arg(dsi.aconnId);
            }
            break;
        case CUSTOM_NM_DEVICE_STATE_REASON_CABLE_UNPLUGGED:
            // disconnected due to cable unplugged
            // if device is ethernet,notify disconnected message
            qCDebug(DSM()) << "Disconnected due to unplugged cable";
            if (dsi.devType == Device::Ethernet) {
                qCDebug(DSM()) << "unplugged device is ethernet";
                msg = tr("\"%1\" disconnected").arg(dsi.aconnId);
            }
            break;
        case Device::StateChangeReason::NoSecretsReason:
            if (dsi.aconnHasEap) {
                msg = tr("To connect \"%1\", please set up your authentication info").arg(dsi.aconnId);
            } else {
                msg = tr("Password is required to connect \"%1\"").arg(dsi.aconnId);
            }
            break;
        case Device::StateChangeReason::SsidNotFound:
            msg = tr("The \"%1\" 802.11 WLAN network could not be found").arg(dsi.aconnId);
            break;
        default:
            // if (dsi.aconnId != "") {
            //     msg = tr("\"%1\" disconnected").arg(dsi.aconnId);
            // }
            break;
        }
        if (!msg.isEmpty()) {
            notify(icon, "", msg);
        }
    } break;
    default:
        break;
    }
}

void NetworkStateHandler::onDeviceEnabled(const QDBusObjectPath &path, bool enabled)
{
    QString uni = path.path();
    if (m_devices.contains(uni)) {
        m_devices[uni].enabled = enabled;
    }
}

void NetworkStateHandler::onVpnStateChanged(const QDBusMessage &msg)
{
    auto args = msg.arguments();
    uint state = args.at(0).toUInt();
    uint reason = args.at(1).toUInt();
    qCDebug(DSM()) << msg.path() << "vpn state changed" << (NetworkManager::VpnConnection::State)state << (NetworkManager::VpnConnection::StateChangeReason)reason;
    handleVpnStateChanged(msg.path(), state, reason);
}

void NetworkStateHandler::handleVpnStateChanged(const QString &path, uint state, uint reason)
{
    if (!m_activeConnections.contains(path)) {
        return;
    }
    // get the corresponding active connection
    auto &&aConn = m_activeConnections[path];
    // if is already state, ignore
    if (aConn.vpnState == state) {
        return;
    }
    // save current state
    aConn.vpnState = state;
    // notification for vpn
    switch (state) {
    case VpnConnection::Activated:
        // NetworkManager may has bug, VPN activated state is emitted unexpectedly when vpn is disconnected
        // vpn connected should not notified when reason is disconnected
        if (reason == VpnConnection::UserDisconnectedReason) {
            return;
        }
        notifyVpnConnected(aConn.Id);
        break;
    case VpnConnection::Disconnected:
        if (aConn.vpnFailed) {
            aConn.vpnFailed = false;
        } else {
            notifyVpnDisconnected(aConn.Id);
        }
        break;
    case VpnConnection::Failed:
        notifyVpnFailed(aConn.Id, reason);
        aConn.vpnFailed = true;
        break;
    default:
        break;
    }
}

void NetworkStateHandler::onNetworkingEnabledChanged(bool enabled)
{
    if (!enabled) {
        notifyAirplanModeEnabled();
    }
}

void NetworkStateHandler::onWirelessHardwareEnabledChanged(bool enabled)
{
    if (!enabled) {
        notifyWirelessHardSwitchOff();
    }
}

bool NetworkStateHandler::isVirtualDeviceIfc(NetworkManager::Device::Ptr dev)
{
    //// workaround for huawei pangu
    QString ifc = dev->interfaceName();
    if (isInDeviceWhitelist(devWhitelistHuaweiFile, ifc)) {
        return false;
    }
    const QStringList virtualDriver = { "dummy", "veth", "vboxnet", "vmnet", "bridge" };
    auto driver = dev->driver();
    if (virtualDriver.contains(driver)) {
        return true;
    }
    const QStringList unknownDriver = { "unknown", "vmxnet", "vmxnet2", "vmxnet3" };
    if (unknownDriver.contains(driver)) {
        // sometimes we could not get vmnet dirver name, so check the
        // udi sys path if is prefix with /sys/devices/virtual/net
        const QString &devUdi = dev->udi();
        if (devUdi.startsWith("/sys/devices/virtual/net") || devUdi.startsWith("/virtual/device") || ifc.startsWith("vmnet")) {
            return true;
        }
    }
    return false;
}

bool NetworkStateHandler::isInDeviceWhitelist(const QString &filename, const QString &ifc)
{
    if (ifc.isEmpty()) {
        return false;
    }
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
        QString line = file.readLine();
        line = line.trimmed();
        if (!line.isEmpty() && line == ifc) {
            return true;
        }
    }
    return false;
}

bool NetworkStateHandler::isDeviceTypeValid(NetworkManager::Device::Type devType)
{
    switch (devType) {
    case Device::Generic:
    case Device::UnknownType:
    case Device::Bluetooth:
    case Device::Team:
    case Device::Tun:
    case Device::IpTunnel:
    case Device::MacVlan:
    case Device::VxLan:
    case Device::Veth:
    case Device::Ppp:
    case Device::WifiP2P:
        return false;
        break;
    default:
        break;
    }
    return true;
}

void NetworkStateHandler::queryDeviceEnabled(const QString &path)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.deepin.dde.Network1", "/org/deepin/dde/Network1", "org.deepin.dde.Network1", "IsDeviceEnabled");
    msg << path;
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(msg), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [path, this](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<bool> reply = *w;
        if (reply.isError()) {
            qCDebug(DSM()) << "get IsDeviceEnabled error:" << path << reply.error();
        } else {
            onDeviceEnabled(QDBusObjectPath(path), reply.value());
        }
        w->deleteLater();
    });
}

bool isDeviceStateInActivating(Device::State state)
{
    return state >= Device::Preparing && state <= Device::Activated;
}

void *NetworkStateHandler::nmGetDeviceActiveConnectionData(NetworkManager::Device::Ptr dev)
{
    if (!isDeviceStateInActivating(dev->state())) {
        qCWarning(DSM()) << "device is inactivated" << dev->udi();
        return nullptr;
    }
    auto activeConn = dev->activeConnection();
    if (activeConn.isNull()) {
        return nullptr;
    }
    auto conn = activeConn->connection();
    ConnectionSettings::Ptr settings = conn->settings();
    return settings.get();
}

void NetworkStateHandler::notify(const QString &icon, const QString &summary, const QString &body)
{
    qCDebug(DSM()) << "notify icon:" << icon << ", summary:" << summary << ", body:" << body;
    if (!m_notifyEnabled) {
        qCDebug(DSM()) << "notify disabled";
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");
    const QStringList actions;
    const QVariantMap hints;
    msg << "dde-control-center" << m_replacesId << icon << summary << body << actions << hints << -1;
    bool ret = QDBusConnection::sessionBus().callWithCallback(msg, this, SLOT(onNotify(uint)));
}

void NetworkStateHandler::onNotify(uint replacesId)
{
    m_replacesId = replacesId;
}

void NetworkStateHandler::onHandleForSleep(bool sleep)
{
    if (sleep) {
        // suspend
        disableNotify();
        return;
    }
    // wakeup
    enableNotify();
    // value decided the strategy of the wirelessScan
    for (auto dev : NetworkManager::networkInterfaces()) {
        if (dev->type() == Device::Wifi) {
            WirelessDevice::Ptr wDev = dev.dynamicCast<WirelessDevice>();
            if (wDev) {
                wDev->requestScan();
            }
        }
    }
}

static QString getMobileConnectedNotifyIcon(ModemDevice::Capabilities mobileNetworkType)
{
    switch (mobileNetworkType) {
    case ModemDevice::Lte:
        return notifyIconMobile4gConnected;
    case ModemDevice::GsmUmts:
        return notifyIconMobile3gConnected;
    case ModemDevice::CdmaEvdo:
        return notifyIconMobile2gConnected;
    default:
        break;
    }
    return notifyIconMobileUnknownConnected;
}

static QString getMobileDisconnectedNotifyIcon(ModemDevice::Capabilities mobileNetworkType)
{
    switch (mobileNetworkType) {
    case ModemDevice::Lte:
        return notifyIconMobile4gDisconnected;
    case ModemDevice::GsmUmts:
        return notifyIconMobile3gDisconnected;
    case ModemDevice::CdmaEvdo:
        return notifyIconMobile2gDisconnected;
    default:
        break;
    }
    return notifyIconMobileUnknownDisconnected;
}

QString NetworkStateHandler::generalGetNotifyConnectedIcon(NetworkManager::Device::Type devType, NetworkManager::Device::Ptr dev)
{
    switch (devType) {
    case Device::Ethernet:
        return notifyIconWiredConnected;
    case Device::Wifi:
        return notifyIconWirelessConnected;
    case Device::Modem: {
        if (dev) {
            ModemDevice::Ptr d = dev.objectCast<ModemDevice>();
            return getMobileConnectedNotifyIcon(d->currentCapabilities());
        }
    }
    default:
        break;
    }
    return notifyIconNetworkConnected;
}

QString NetworkStateHandler::generalGetNotifyDisconnectedIcon(NetworkManager::Device::Type devType, NetworkManager::Device::Ptr dev)
{
    switch (devType) {
    case Device::Ethernet:
        return notifyIconWiredDisconnected;
    case Device::Wifi:
        return notifyIconWirelessDisconnected;
    case Device::Modem: {
        if (dev) {
            ModemDevice::Ptr d = dev.objectCast<ModemDevice>();
            return getMobileDisconnectedNotifyIcon(d->currentCapabilities());
        }
    }
    default:
        break;
    }
    qCWarning(DSM()) << "lost default notify icon for device" << devType;
    return notifyIconNetworkDisconnected;
}

void NetworkStateHandler::notifyAirplanModeEnabled()
{
    notify(notifyIconNetworkOffline, tr("Disconnected"), tr("Airplane mode enabled."));
}

void NetworkStateHandler::notifyWirelessHardSwitchOff()
{
    notify(notifyIconWirelessDisabled, tr("Network"), tr("The hardware switch of WLAN Card is off, please switch on as necessary."));
}

void NetworkStateHandler::disableNotify()
{
    m_notifyEnabled = false;
}

void NetworkStateHandler::enableNotify()
{
    QTimer::singleShot(5000, this, [this]() {
        m_notifyEnabled = true;
    });
}

void NetworkStateHandler::notifyVpnConnected(const QString &id)
{
    notify(notifyIconVpnConnected, tr("Connected"), id);
}

void NetworkStateHandler::notifyVpnDisconnected(const QString &id)
{
    notify(notifyIconVpnDisconnected, tr("Disconnected"), id);
}

void NetworkStateHandler::notifyVpnFailed(const QString &id, uint reason)
{
    notify(notifyIconVpnDisconnected, tr("Disconnected"), m_vpnErrorTable[reason]);
}

NetworkStateHandler::ConnectionType NetworkStateHandler::getCustomConnectionType(NetworkManager::ConnectionSettings *settings)
{
    auto t = settings->connectionType();
    switch (t) {
    case ConnectionSettings::Wired:
    case ConnectionSettings::Pppoe:
    case ConnectionSettings::Gsm:
    case ConnectionSettings::Cdma:
        return NetworkStateHandler::ConnectionType(t);
        break;
    case ConnectionSettings::Wireless: {
        auto wSetting = settings->setting(Setting::Wireless).staticCast<WirelessSetting>();
        switch (wSetting->mode()) {
        case WirelessSetting::Adhoc:
            return NetworkStateHandler::WirelessAdhoc;
        case WirelessSetting::Ap:
            return NetworkStateHandler::WirelessHotspot;
        default:
            break;
        }
        return NetworkStateHandler::ConnectionType(t);
    } break;
    case ConnectionSettings::Vpn: {
        auto vpnSetting = settings->setting(Setting::Vpn).staticCast<VpnSetting>();
        QString type = vpnSetting->serviceType();
        if (type == ServiceTypeL2TP) {
            return NetworkStateHandler::VpnL2TP;
        } else if (type == ServiceTypeOpenConnect) {
            return NetworkStateHandler::VpnOpenconnect;
        } else if (type == ServiceTypeOpenVPN) {
            return NetworkStateHandler::VpnOpenvpn;
        } else if (type == ServiceTypePPTP) {
            return NetworkStateHandler::VpnPptp;
        } else if (type == ServiceTypeStrongSwan) {
            return NetworkStateHandler::VpnStrongswan;
        } else if (type == ServiceTypeVPNC) {
            return NetworkStateHandler::VpnVpnc;
        } else if (type == ServiceTypeSSTP) {
            return NetworkStateHandler::VpnSstp;
        }
    } break;
    default:
        break;
    }
    return NetworkStateHandler::Unknown;
}
} // namespace sessionservice
} // namespace network
