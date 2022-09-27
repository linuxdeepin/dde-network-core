// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WIRELESSDEVICE_H
#define WIRELESSDEVICE_H

#include "networkdevicebase.h"

namespace dde {

namespace network {

class AccessPoints;
class WirelessConnection;
class NetworkDeviceRealize;
/**
 * @brief 无线网络设备-无线网卡
 */
class WirelessDevice : public NetworkDeviceBase
{
    Q_OBJECT

    friend class NetworkController;
    friend class NetworkInterProcesser;
    friend class NetworkManagerProcesser;

public:
    bool isConnected() const override;                              // 是否连接网络，重写基类的虚函数
    DeviceType deviceType() const override;                         // 返回设备类型，适应基类统一的接口
    QList<AccessPoints *> accessPointItems() const;                 // 当前网卡上所有的网络列表
    void scanNetwork();                                             // 重新加载所有的无线网络列表
    void connectNetwork(const AccessPoints *item);                  // 连接网络，连接成功抛出deviceStatusChanged信号
    void connectNetwork(const QString &ssid);                       // 连接网络，重载函数
    QList<WirelessConnection *> items() const;                      // 无线网络连接列表
    AccessPoints *activeAccessPoints() const;                       // 当前活动的无线连接
    bool hotspotEnabled() const;                                    // 是否开启热点
    void disconnectNetwork();

Q_SIGNALS:
    void networkAdded(QList<AccessPoints *>);                       // wlan新增网络
    void networkRemoved(QList<AccessPoints *>);                     // wlan列表减少网络
    void connectionFailed(const AccessPoints *);                    // 连接无线wlan失败，第一个参数为失败的热点，第二个参数为对应的connection的Uuid
    void connectionSuccess(const AccessPoints *);                   // 连接无线网络wlan成功，参数为对应的wlan
    void hotspotEnableChanged(const bool &);                        // 热点是否可用发生变化
    void accessPointInfoChanged(const QList<AccessPoints *> &);     // wlan信号强度发生变化的网络

protected:
    WirelessDevice(NetworkDeviceRealize *networkInter, QObject *parent);
    ~WirelessDevice() override;

private:
    AccessPoints *findAccessPoint(const QString &ssid);
};

/**
 * @brief 无线网络连接-wlan
 */

class AccessPoints : public QObject
{
    Q_OBJECT

    friend class WirelessDevice;
    friend class WirelessDeviceInterRealize;
    friend class DeviceManagerRealize;

    Q_PROPERTY(QString ssid READ ssid)
    Q_PROPERTY(int strength READ strength)
    Q_PROPERTY(bool secured READ secured)
    Q_PROPERTY(bool securedInEap READ securedInEap)
    Q_PROPERTY(int frequency READ frequency)
    Q_PROPERTY(QString path READ path)
    Q_PROPERTY(QString devicePath READ devicePath)
    Q_PROPERTY(bool connected READ connected)

public:
    enum class WlanType { wlan, wlan6 };

protected:
    AccessPoints(const QJsonObject &json, QObject *parent = Q_NULLPTR);
    ~AccessPoints();

public:
    QString ssid() const;                                           // 网络SSID，对应于返回接口中的Ssid
    int strength() const;                                           // 信号强度，对应于返回接口中的Strength
    bool secured() const;                                           // 是否加密，对应于返回接口中的Secured
    bool securedInEap() const;                                      // 对应于返回接口中的SecuredInEap
    int frequency() const;                                          // 频率，对应于返回接口中的Frequency
    QString path() const;                                           // 路径，对应于返回接口中的Path
    QString devicePath() const;                                     // 对应的设备的路径，为返回接口中的key值
    bool connected() const;                                         // 网络是否连接成功
    ConnectionStatus status() const;                                // 当前网络的连接状态
    bool hidden() const;                                            // 是否为隐藏网络
    WlanType type() const;

Q_SIGNALS:
    void strengthChanged(const int) const;                          // 当前信号强度变化
    void connectionStatusChanged(ConnectionStatus);
    void securedChanged(bool);

protected:
    void updateAccessPoints(const QJsonObject &json);
    void updateConnectionStatus(ConnectionStatus);

private:
    QJsonObject m_json;
    QString m_devicePath;
    ConnectionStatus m_status;
};

/**
 * @brief 无线连接信息
 */

class WirelessConnection: public ControllItems
{
    friend class WirelessDevice;
    friend class WirelessDeviceInterRealize;
    friend class DeviceManagerRealize;

public:
    AccessPoints *accessPoints() const;                             // 返回当前对应的wlan的指针
    bool connected();                                               // 网络是否连接成功

protected:
    WirelessConnection();
    ~WirelessConnection();

    static WirelessConnection *createConnection(AccessPoints *ap);

private:
    AccessPoints *m_accessPoints;
};

}

}

#endif // UWIRELESSDEVICE_H
