// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "deviceinterrealize.h"
#include "wireddevice.h"
#include "wirelessdevice.h"
#include "networkdbusproxy.h"

#include <QHostAddress>
#include <QJsonDocument>

using namespace dde::network;

#define UNKNOW_MODE 0
#define ADHOC_MODE  1
#define INFRA_MODE 2
#define AP_MODE 3

#define ACTIVATING 1
#define ACTIVATED 2
#define DEACTIVATING 3
#define DEACTIVATED 4

const QStringList DeviceInterRealize::ipv4()
{
    if (!isConnected() || !isEnabled())
        return QStringList();

    if (m_activeInfoData.contains("IPv4")) {
        QJsonObject ipv4TopObject = m_activeInfoData["IPv4"].toObject();
        QJsonArray ipv4Array = ipv4TopObject.value("Addresses").toArray();
        QStringList ipv4s;
        for (const QJsonValue ipv4Value : ipv4Array) {
            const QJsonObject ipv4Object = ipv4Value.toObject();
            QString ip = ipv4Object.value("Address").toString();
            ip = ip.remove("\"");
            ipv4s << ip;
        }
        ipv4s = getValidIPV4(ipv4s);
        return ipv4s;
    }

    // 返回IPv4地址
    QJsonValue ipJsonData = m_activeInfoData["Ip4"];
    QJsonObject objIpv4 = ipJsonData.toObject();
    return { objIpv4.value("Address").toString() };
}

const QStringList DeviceInterRealize::ipv6()
{
    if (!isConnected() || !isEnabled() || !m_activeInfoData.contains("Ip6"))
        return QStringList();

    if (m_activeInfoData.contains("IPv6")) {
        QJsonObject ipv6TopObject = m_activeInfoData["IPv6"].toObject();
        QJsonArray ipv6Array = ipv6TopObject.value("Addresses").toArray();
        QStringList ipv6s;
        for (const QJsonValue ipv6Value : ipv6Array) {
            const QJsonObject ipv6Object = ipv6Value.toObject();
            QString ip = ipv6Object.value("Address").toString();
            ip = ip.remove("\"");
            ipv6s << ip;
        }
        return ipv6s;
    }

    // 返回IPv4地址
    QJsonValue ipJsonData = m_activeInfoData["Ip6"];
    QJsonObject objIpv6 = ipJsonData.toObject();
    return { objIpv6.value("Address").toString() };
}

QJsonObject DeviceInterRealize::activeConnectionInfo() const
{
    return m_activeInfoData;
}

QStringList DeviceInterRealize::getValidIPV4(const QStringList &ipv4s)
{
    if (ipv4s.size() > 1 || ipv4s.size() == 0)
        return ipv4s;

    // 检查IP列表，如果发现有IP为0.0.0.0，则让其重新获取一次，保证IP获取正确
    // 这种情况一般发生在关闭热点后，因此在此处处理
    if (isIpv4Address(ipv4s[0]))
        return ipv4s;

    const QString activeConnInfo = m_networkInter->GetActiveConnectionInfo();
    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(activeConnInfo.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
        return ipv4s;

    if (!json.isArray())
        return ipv4s;

    QJsonArray infoArray = json.array();
    for (const QJsonValue ipInfo : infoArray) {
        const QJsonObject ipObject = ipInfo.toObject();
        if (ipObject.value("Device").toString() != this->path())
            continue;

        if (!ipObject.contains("IPv4"))
            return ipv4s;

        QJsonObject ipV4Object = ipObject.value("IPv4").toObject();
        if (!ipV4Object.contains("Addresses"))
            return ipv4s;

        QStringList ipAddresses;
        QJsonArray ipv4Addresses = ipV4Object.value("Addresses").toArray();
        for (const QJsonValue addr : ipv4Addresses) {
            const QJsonObject addressObject = addr.toObject();
            QString ip = addressObject.value("Address").toString();
            if (isIpv4Address(ip))
                ipAddresses << ip;
        }
        if (ipAddresses.size() > 0) {
            m_activeInfoData = ipObject;
            return ipAddresses;
        }
    }

    return ipv4s;
}

bool DeviceInterRealize::isIpv4Address(const QString &ip) const
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv4)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        return false;
    }

    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    return regExpIP.exactMatch(ip);
}

void DeviceInterRealize::setEnabled(bool enabled)
{
    m_networkInter->EnableDevice(QDBusObjectPath(path()), enabled);
}

Connectivity DeviceInterRealize::connectivity()
{
    return m_connectivity;
}

void DeviceInterRealize::deviceConnectionFailed()
{
    // 连接失败
//    Q_EMIT connectionFailed(item);
    Q_EMIT deviceStatusChanged(DeviceStatus::Failed);
}

void DeviceInterRealize::deviceConnectionSuccess()
{
    Q_EMIT deviceStatusChanged(DeviceStatus::Activated);
}

DeviceInterRealize::DeviceInterRealize(IPConfilctChecker *ipChecker, NetworkDBusProxy *networkInter, QObject *parent)
    : NetworkDeviceRealize(ipChecker, parent)
    , m_networkInter(networkInter)
    , m_enabled(true)
    , m_connectivity(Connectivity::Full)
{
}

DeviceInterRealize::~DeviceInterRealize()
{
}

NetworkDBusProxy *DeviceInterRealize::networkInter()
{
    return m_networkInter;
}

void DeviceInterRealize::updateDeviceInfo(const QJsonObject &info)
{
    m_data = info;
    DeviceStatus stat = convertDeviceStatus(info.value("State").toInt());

    setDeviceStatus(stat);
}

void DeviceInterRealize::initDeviceInfo()
{
    if (m_networkInter) {
        // 状态发生变化后，获取设备的实时信息
        m_enabled = m_networkInter->IsDeviceEnabled(QDBusObjectPath(path()));
    }
}

void DeviceInterRealize::setDeviceEnabledStatus(const bool &enabled)
{
    m_enabled = enabled;
    Q_EMIT enableChanged(enabled);
}

void DeviceInterRealize::updateActiveConnectionInfo(const QList<QJsonObject> &infos)
{
    const QStringList oldIpv4 = ipv4();
    m_activeInfoData = QJsonObject();
    for (const QJsonObject &info : infos) {
        if (info.value("ConnectionType").toString() == deviceKey()) {
            // 如果找到了当前硬件地址的连接信息，则直接使用这个数据
            m_activeInfoData = info;
            break;
        }
    }

    // 获取到完整的IP地址后，向外发送连接改变的信号
    if (!m_activeInfoData.isEmpty())
        Q_EMIT connectionChanged();

    QStringList ipv4s = ipv4();
    bool ipChanged = false;
    if (oldIpv4.size() != ipv4s.size()) {
        ipChanged = true;
    } else {
        for (const QString &ip : ipv4s) {
            if (!oldIpv4.contains(ip)) {
                ipChanged = true;
                break;
            }
        }
    }
    if (ipChanged)
        Q_EMIT ipV4Changed();
}

void DeviceInterRealize::setDeviceStatus(const DeviceStatus &status)
{
    // 如果是开启热点，就让其renweus断开的状态
    DeviceStatus stat = status;
    if (mode() == AP_MODE)
        stat = DeviceStatus::Disconnected;

    NetworkDeviceRealize::setDeviceStatus(stat);
}

int DeviceInterRealize::mode() const
{
    if (m_data.contains("Mode"))
        return m_data.value("Mode").toInt();

    return UNKNOW_MODE;
}

/**
 * @brief 有线设备类的具体实现
 */

WiredDeviceInterRealize::WiredDeviceInterRealize(IPConfilctChecker *ipChecker, NetworkDBusProxy *networkInter, QObject *parent)
    : DeviceInterRealize(ipChecker, networkInter, parent)
{
}

WiredDeviceInterRealize::~WiredDeviceInterRealize()
{
    for (auto item : m_connections) {
        delete item;
    }
    m_connections.clear();
}

bool WiredDeviceInterRealize::connectNetwork(WiredConnection *connection)
{
    if (!connection)
        return false;

    networkInter()->ActivateConnection(connection->connection()->uuid(), QDBusObjectPath(path()));
    return true;
}

void WiredDeviceInterRealize::disconnectNetwork()
{
    networkInter()->DisconnectDevice(QDBusObjectPath(path()));
}

bool WiredDeviceInterRealize::isConnected() const
{
    for (WiredConnection *connection : m_connections) {
        if (connection->connected())
            return true;
    }

    return false;
}

QList<WiredConnection *> WiredDeviceInterRealize::wiredItems() const
{
    return m_connections;
}

WiredConnection *WiredDeviceInterRealize::findConnection(const QString &path)
{
    for (WiredConnection *conn : m_connections) {
        if (conn->connection()->path() == path)
            return conn;
    }

    return Q_NULLPTR;
}

void WiredDeviceInterRealize::setDeviceEnabledStatus(const bool &enabled)
{
    if (!enabled) {
        // 禁用网卡的情况下，先清空连接信息
        for (WiredConnection *connection : m_connections)
            connection->setConnectionStatus(ConnectionStatus::Deactivated);
    }

    DeviceInterRealize::setDeviceEnabledStatus(enabled);
}

void WiredDeviceInterRealize::updateConnection(const QJsonArray &info)
{
    QList<WiredConnection *> newWiredConnections;
    QList<WiredConnection *> changedWiredConnections;
    QStringList connPaths;
    for (const QJsonValue &jsonValue : info) {
        const QJsonObject &jsonObj = jsonValue.toObject();
        const QString IfcName = jsonObj.value("IfcName").toString();
        if (!IfcName.isEmpty() && IfcName != interface())
            continue;

        const QString path = jsonObj.value("Path").toString();
        WiredConnection *conn = findConnection(path);
        if (!conn) {
            conn = new WiredConnection;
            m_connections << conn;
            newWiredConnections << conn;
        } else {
            if (conn->connection()->id() != jsonObj.value("Id").toString()
                    || conn->connection()->ssid() != jsonObj.value("Ssid").toString())
                changedWiredConnections << conn;
        }

        conn->setConnection(jsonObj);
        if (!connPaths.contains(path))
            connPaths << path;
    }

    QList<WiredConnection *> rmConns;
    for (WiredConnection *connection : m_connections) {
        if (!connPaths.contains(connection->connection()->path()))
            rmConns << connection;
    }

    for (WiredConnection *connection : rmConns)
        m_connections.removeOne(connection);

    if (changedWiredConnections.size())
        Q_EMIT connectionPropertyChanged(changedWiredConnections);

    if (newWiredConnections.size() > 0)
        Q_EMIT connectionAdded(newWiredConnections);

    if (rmConns.size() > 0)
        Q_EMIT connectionRemoved(rmConns);

    // 提交改变信号后，删除不在的连接
    for (WiredConnection *connection : rmConns)
        delete connection;

    // 排序
    sortWiredItem(m_connections);
}

WiredConnection *WiredDeviceInterRealize::findWiredConnectionByUuid(const QString &uuid)
{
    for (WiredConnection *connection : m_connections) {
        if (connection->connection()->uuid() == uuid)
            return connection;
    }

    return Q_NULLPTR;
}

static ConnectionStatus convertStatus(int status)
{
    if (status == ACTIVATING)
        return ConnectionStatus::Activating;

    if (status == ACTIVATED)
        return ConnectionStatus::Activated;

    if (status == DEACTIVATING)
        return ConnectionStatus::Deactivating;

    if (status == DEACTIVATED)
        return ConnectionStatus::Deactivated;

    return ConnectionStatus::Unknown;
}

void WiredDeviceInterRealize::updateActiveInfo(const QList<QJsonObject> &info)
{
    bool changeStatus = false;
    // 根据返回的UUID找到对应的连接，找到State=2的连接变成连接成功状态
    for (const QJsonObject &activeInfo : info) {
        const QString uuid = activeInfo.value("Uuid").toString();
        WiredConnection *connection = findWiredConnectionByUuid(uuid);
        if (!connection)
            continue;

        ConnectionStatus status = convertStatus(activeInfo.value("State").toInt());
        if (connection->status() != status) {
            connection->setConnectionStatus(status);
            changeStatus = true;
        }
    }

    // 调用基类的函数，更改设备的状态，同时向外发送信号
    DeviceInterRealize::updateActiveInfo(info);
    if (changeStatus)
        Q_EMIT activeConnectionChanged();
}

QString WiredDeviceInterRealize::deviceKey()
{
    return "wired";
}

/**
 * @brief WirelessDeviceInterRealize::WirelessDeviceInterRealize
 * @param networkInter
 * @param parent
 */

bool WirelessDeviceInterRealize::isConnected() const
{
    for (AccessPoints *ap : m_accessPoints) {
        if (ap->status() == ConnectionStatus::Activated)
            return true;
    }

    return false;
}

QList<AccessPoints *> WirelessDeviceInterRealize::accessPointItems() const
{
    if (needShowAccessPoints())
        return m_accessPoints;

    return QList<AccessPoints *>();
}

void WirelessDeviceInterRealize::scanNetwork()
{
    networkInter()->RequestWirelessScan();
}

void WirelessDeviceInterRealize::connectNetwork(const AccessPoints *item)
{
    WirelessConnection *wirelessConn = findConnectionByAccessPoint(item);
    if (!wirelessConn)
        return;

    const QString uuid = wirelessConn->connection()->uuid();
    const QString apPath = item->path();
    const QString devPath = path();

    qInfo() << "Connect to the AP, uuid:" << uuid << ", access point path:" << apPath << ", device path:" << devPath;
    networkInter()->ActivateAccessPoint(uuid, QDBusObjectPath(apPath), QDBusObjectPath(devPath), this, SLOT(deviceConnectionSuccess()), SLOT(deviceConnectionFailed()));
}

AccessPoints *WirelessDeviceInterRealize::activeAccessPoints() const
{
    // 如果网卡是关闭的状态下，肯定是没有连接
    if (!isEnabled())
        return Q_NULLPTR;

    for (AccessPoints *ap : m_accessPoints) {
        if (ap->connected())
            return ap;
    }

    return Q_NULLPTR;
}

void WirelessDeviceInterRealize::disconnectNetwork()
{
    networkInter()->DisconnectDevice(QDBusObjectPath(path()));
}

QList<WirelessConnection *> WirelessDeviceInterRealize::items() const
{
    QList<WirelessConnection *> lstItems;
    for (WirelessConnection *item : m_connections) {
        if (item->accessPoints())
            lstItems << item;
    }

    return lstItems;
}

WirelessDeviceInterRealize::WirelessDeviceInterRealize(IPConfilctChecker *ipChecker, NetworkDBusProxy *networkInter, QObject *parent)
    : DeviceInterRealize(ipChecker, networkInter, parent)
{
}

WirelessDeviceInterRealize::~WirelessDeviceInterRealize()
{
    clearListData(m_accessPoints);
    clearListData(m_connections);
}

WirelessConnection *WirelessDeviceInterRealize::findConnectionByPath(const QString &path)
{
    for (WirelessConnection *conn : m_connections) {
        if (conn->connection()->path() == path)
            return conn;
    }

    return Q_NULLPTR;
}

AccessPoints *WirelessDeviceInterRealize::findAccessPoint(const QString &ssid)
{
    for (AccessPoints *accessPoint : m_accessPoints) {
        if (accessPoint->ssid() == ssid)
            return accessPoint;
    }

    return Q_NULLPTR;
}

WirelessConnection *WirelessDeviceInterRealize::findConnectionByAccessPoint(const AccessPoints *accessPoint)
{
    if (!accessPoint)
        return Q_NULLPTR;

    for (WirelessConnection *connection : m_connections) {
        if (connection->accessPoints() == accessPoint)
            return connection;

        if (connection->connection()->ssid() == accessPoint->ssid())
            return connection;
    }

    return Q_NULLPTR;
}

/**
 * @brief 同步热点和连接的信息
 */
void WirelessDeviceInterRealize::syncConnectionAccessPoints()
{
    if (m_accessPoints.isEmpty()) {
        clearListData(m_connections);
        return;
    }

    QList<WirelessConnection *> connections;
    // 找到每个热点对应的Connection，并将其赋值
    for (AccessPoints *accessPoint : m_accessPoints) {
        WirelessConnection *connection = findConnectionByAccessPoint(accessPoint);
        if (!connection) {
            connection = WirelessConnection::createConnection(accessPoint);
            m_connections << connection;
        }

        connection->m_accessPoints = accessPoint;
        connections << connection;
    }
    updateActiveInfo();
    // 删除列表中没有AccessPoints的Connection，让两边保持数据一致
    QList<WirelessConnection *> rmConns;
    for (WirelessConnection *connection : m_connections) {
        if (!connections.contains(connection))
            rmConns << connection;
    }

    for (WirelessConnection *rmConnection : rmConns) {
        m_connections.removeOne(rmConnection);
        delete rmConnection;
    }
}

void WirelessDeviceInterRealize::updateActiveInfo()
{
    if (m_activeAccessPoints.isEmpty())
        return;

    QList<AccessPoints *> tmpApList = m_accessPoints;
    // 遍历活动连接列表，找到对应的wlan，改变其连接状态，State赋值即可
    bool changed = false;
    AccessPoints *activeAp = Q_NULLPTR;
    for (const QJsonObject &aapInfo : m_activeAccessPoints) {
        int connectionStatus = aapInfo.value("State").toInt();
        QString ssid = aapInfo.value("Id").toString();
        AccessPoints *ap = findAccessPoint(ssid);
        if (!ap)
            continue;

        tmpApList.removeAll(ap);
        ConnectionStatus status = convertConnectionStatus(connectionStatus);
        if (ap->status() == status)
            continue;

        ap->updateConnectionStatus(status);
        changed = true;
        if (ap->status() == ConnectionStatus::Activated)
            activeAp = ap;
    }

    // 将其它连接变成普通状态
    for (AccessPoints *ap : tmpApList)
        ap->updateConnectionStatus(ConnectionStatus::Unknown);

    if (changed) {
        Q_EMIT activeConnectionChanged();
    }

    // 如果发现其中一个连接成功，将这个连接成功的信号移到最上面，然后则向外发送连接成功的信号
    if (activeAp) {
        int pos = m_accessPoints.indexOf(activeAp);
        m_accessPoints.move(pos, 0);
        Q_EMIT connectionSuccess(activeAp);
    }

    // 调用基类的方法触发连接发生变化，同时向外抛出连接变化的信号
    DeviceInterRealize::updateActiveInfo(m_activeAccessPoints);
}

QList<WirelessConnection *> WirelessDeviceInterRealize::wirelessItems() const
{
    return m_connections;
}

bool WirelessDeviceInterRealize::needShowAccessPoints() const
{
    // Mode=3表示开启热点
    if (mode() == AP_MODE)
        return false;
    // 如果当前设备热点为空(关闭热点)，则让显示所有的网络列表
    return m_hotspotInfo.isEmpty();
}

void WirelessDeviceInterRealize::updateActiveConnectionInfo(const QList<QJsonObject> &infos)
{
    bool enabledHotspotOld = hotspotEnabled();

    m_hotspotInfo = QJsonObject();
    for (const QJsonObject &info : infos) {
        const QString devicePath = info.value("Device").toString();
        const QString connectionType = info.value("ConnectionType").toString();
        if (devicePath == this->path() && connectionType == "wireless-hotspot") {
            m_hotspotInfo = info;
            setDeviceStatus(DeviceStatus::Disconnected);
            break;
        }
    }

    bool enabledHotspot = hotspotEnabled();
    if (enabledHotspotOld != enabledHotspot)
        Q_EMIT hotspotEnableChanged(enabledHotspot);

    DeviceInterRealize::updateActiveConnectionInfo(infos);
}

bool dde::network::WirelessDeviceInterRealize::hotspotEnabled()
{
    return !m_hotspotInfo.isEmpty();
}

void WirelessDeviceInterRealize::updateAccesspoint(const QJsonArray &json)
{
    auto isWifi6 = [](const QJsonObject &json) {
        if (json.contains("Flags")) {
            int flag = json.value("Flags").toInt();
            if (flag & AP_FLAGS_HE)
                return true;
        }

        return false;
    };

    // 先过滤相同的ssid，找出信号强度最大的那个
    QMap<QString, int> ssidMaxStrength;
    QMap<QString, QString> ssidPath;
    QMap<QString, int> wifi6Ssids;
    for (const QJsonValue &jsonValue : json) {
        const QJsonObject obj = jsonValue.toObject();
        const QString ssid = obj.value("Ssid").toString();
        const int strength = obj.value("Strength").toInt();
        const QString path = obj.value("Path").toString();
        if (ssidMaxStrength.contains(ssid)) {
            const int nOldStrength = ssidMaxStrength.value(ssid);
            if (nOldStrength < strength) {
                // 找到了对应的热点，更新热点的信号强度
                ssidMaxStrength[ssid] = strength;
                ssidPath[ssid] = path;
            }
        } else {
            // 第一次直接插入SSID和信号强度和路径
            ssidMaxStrength[ssid] = strength;
            ssidPath[ssid] = path;
        }
        if (isWifi6(obj))
            wifi6Ssids[ssid] = obj.value("Flags").toInt();
    }

    QList<AccessPoints *> newAp;
    QList<AccessPoints *> changedAp;
    QStringList ssids;
    for (const QJsonValue &jsonValue : json) {
        QJsonObject accessInfo = jsonValue.toObject();
        const QString ssid = accessInfo.value("Ssid").toString();
        const QString maxSsidPath = ssidPath.value(ssid);
        const QString path = accessInfo.value("Path").toString();
        if (path != maxSsidPath)
            continue;

        // 如果当前的SSID存在WiFi6,就让其显示WiFi6的属性
        if (wifi6Ssids.contains(ssid))
            accessInfo["extendFlags"] = wifi6Ssids[ssid];

        // 从网络列表中查找现有的网络
        AccessPoints *accessPoint = findAccessPoint(ssid);
        if (!accessPoint) {
            // 如果没有找到这个网络，就新建一个网络，添加到网络列表
            accessPoint = new AccessPoints(accessInfo, this);
            accessPoint->m_devicePath = this->path();
            m_accessPoints << accessPoint;
            newAp << accessPoint;
        } else {
            int strength = accessInfo.value("Strength").toInt();
            if (accessPoint->strength() != strength)
                changedAp << accessPoint;

            accessPoint->updateAccessPoints(accessInfo);
        }

        if (!ssids.contains(ssid))
            ssids << ssid;
    }

    if (changedAp.size())
        Q_EMIT accessPointInfoChanged(changedAp);

    if (newAp.size() > 0)
        Q_EMIT networkAdded(newAp);

    // 更新网络和连接的关系
    QList<AccessPoints *> rmAccessPoints;
    for (AccessPoints *ap : m_accessPoints) {
        if (!ssids.contains(ap->ssid()))
            rmAccessPoints << ap;
    }

    if (rmAccessPoints.size() > 0) {
        for (AccessPoints *ap : rmAccessPoints)
            m_accessPoints.removeOne(ap);

        Q_EMIT networkRemoved(rmAccessPoints);
    }

    for (AccessPoints *ap : rmAccessPoints)
        ap->deleteLater();

    createConnection(m_connectionJson);
    syncConnectionAccessPoints();
}

void WirelessDeviceInterRealize::setDeviceEnabledStatus(const bool &enabled)
{
    if (!enabled) {
        // 禁用网卡的情况下，直接清空原来的连接信息
        m_activeAccessPoints.clear();
        // 向外抛出删除wlan连接的信号,这里暂时不清空AccessPoints列表，防止再打开网卡的时候重复创建
        Q_EMIT networkRemoved(m_accessPoints);
    }

    DeviceInterRealize::setDeviceEnabledStatus(enabled);
}

void WirelessDeviceInterRealize::updateConnection(const QJsonArray &info)
{
    m_connectionJson = info;

    createConnection(info);

    syncConnectionAccessPoints();
}

void WirelessDeviceInterRealize::createConnection(const QJsonArray &info)
{
    QStringList connPaths;
    for (const QJsonValue &jsonValue : info) {
        const QJsonObject &jsonObj = jsonValue.toObject();
        const QString hwAddress = jsonObj.value("HwAddress").toString();
        if (!hwAddress.isEmpty() && hwAddress != realHwAdr())
            continue;

        // only update its own connection instead of all devices connection.
        const QString ifcName = jsonObj.value("IfcName").toString();
        if (!ifcName.isEmpty() && ifcName != interface())
            continue;

        const QString path = jsonObj.value("Path").toString();
        WirelessConnection *connection = findConnectionByPath(path);
        if (!connection) {
            connection = new WirelessConnection;
            m_connections << connection;
        }

        connection->setConnection(jsonObj);
        if (!connPaths.contains(path))
            connPaths << path;
    }

    QList<WirelessConnection *> rmConns;
    for (WirelessConnection *conn : m_connections) {
        if (!connPaths.contains(conn->connection()->path()))
            rmConns << conn;
    }

    // 提交改变信号后，删除不在的连接
    for (WirelessConnection *conn : rmConns) {
        m_connections.removeOne(conn);
        delete conn;
    }
}

void WirelessDeviceInterRealize::updateActiveInfo(const QList<QJsonObject> &info)
{
    m_activeAccessPoints = info;
    updateActiveInfo();
}

QString WirelessDeviceInterRealize::deviceKey()
{
    return "wireless";
}
