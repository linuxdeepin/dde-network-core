// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipconfilctchecker.h"
#include "networkdevicebase.h"
#include "realize/netinterface.h"

#include <QtMath>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/device.h>

const static QString networkService = "com.deepin.daemon.Network";
const static QString networkPath = "/com/deepin/daemon/Network";

using namespace dde::network;

IPConfilctChecker::IPConfilctChecker(NetworkProcesser *networkProcesser, const bool ipChecked, QObject *parent)
    : QObject(parent)
    , m_networkInter(new NetworkInter(networkService, networkPath, QDBusConnection::sessionBus(), this))
    , m_networkProcesser(networkProcesser)
    , m_ipNeedCheck(ipChecked)
    , m_thread(new QThread(this))
{
    this->moveToThread(m_thread);

    Q_ASSERT(m_networkProcesser);
    connect(m_networkInter, &NetworkInter::IPConflict, this, &IPConfilctChecker::onIPConfilct);                      // IP冲突发出的信号
    // 构造所有的设备冲突检测对象
    connect(m_networkProcesser, &NetworkProcesser::deviceAdded, this, &IPConfilctChecker::onDeviceAdded, Qt::QueuedConnection);
    m_thread->start();
}

IPConfilctChecker::~IPConfilctChecker()
{
    m_thread->quit();
    m_thread->wait();
}

void IPConfilctChecker::onDeviceAdded(QList<NetworkDeviceBase *> devices)
{
    for (NetworkDeviceBase *device : devices) {
        DeviceIPChecker *ipChecker = new DeviceIPChecker(device, m_networkInter, this);
        connect(ipChecker, &DeviceIPChecker::conflictStatusChanged, this, &IPConfilctChecker::conflictStatusChanged);
        if (m_ipNeedCheck)
            connect(ipChecker, &DeviceIPChecker::ipConflictCheck, this, &IPConfilctChecker::onSenderIPInfo);
        m_deviceCheckers << ipChecker;
    }
}

void IPConfilctChecker::clearUnExistDevice()
{
    QList<NetworkDeviceBase *> devices = m_networkProcesser->devices();
    for (DeviceIPChecker *ipChecker : m_deviceCheckers) {
        if (!devices.contains(ipChecker->device())) {
            m_deviceCheckers.removeOne(ipChecker);
            delete ipChecker;
        }
    }
}

void IPConfilctChecker::onIPConfilct(const QString &ip, const QString &macAddress)
{
    // 此处通过调用后台获取IP地址，如果直接使用当前网络库中设备的IP地址，就有如下问题
    // 如果是在控制中心修改手动IP的话，从当前库中获取到的IP地址就不是最新的地址，因此此处需要从后台获取实时IP
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(m_networkInter->GetActiveConnectionInfo(), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ](QDBusPendingCallWatcher * w) {
        QDBusPendingReply<QString> reply = *w;
        QString activeConnectionInfo = reply.value();
        handlerIpConflict(ip, macAddress, activeConnectionInfo);
    });
}

void IPConfilctChecker::onSenderIPInfo(const QStringList &ips)
{
    for (const QString &ip : ips) {
        m_networkInter->RequestIPConflictCheck(ip, "");
        QThread::msleep(500);
    }
}

void IPConfilctChecker::handlerIpConflict(const QString &ip, const QString &macAddress, const QString &activeConnectionInfo)
{
    QMap<QString, NetworkDeviceBase *> deviceIps = parseDeviceIp(activeConnectionInfo);
    NetworkDeviceBase *conflictDevice = Q_NULLPTR;
    if (deviceIps.contains(ip))
        conflictDevice = deviceIps[ip];
    else {
        // 可能是修改前的ip，从现存的checker里找是否有存在的，并更新ip。
        for (DeviceIPChecker *checker : m_deviceCheckers) {
            if (checker->ipV4().contains(ip)) {
                QStringList ips;
                for (auto it = deviceIps.begin(); it != deviceIps.end(); it++) {
                    if (it.value() == checker->device())
                        ips << it.key();
                }

                if (ips.isEmpty()) {
                    // 已经没有checker 对应的device 了，应该销毁这个checker
                    m_deviceCheckers.removeOne(checker);
                    if (checker->ipConflicted()) {
                        conflictStatusChanged(checker->device(), false);
                    }
                    checker->deleteLater();
                } else {
                    checker->setDeviceInfo(ips, macAddress);
                    checker->handlerIpConflict();
                }
            }
        }
        return;
    }

    // 如果不是本机IP，不做任何处理
    if (!conflictDevice) {
        return;
    }

    // 先从列表中查找对应的设备检测对象
    DeviceIPChecker *ipChecker = Q_NULLPTR;
    for (DeviceIPChecker *checker : m_deviceCheckers) {
        if (checker->device() == conflictDevice) {
            ipChecker = checker;
            break;
        }
    }

    if (!ipChecker) {
        ipChecker = new DeviceIPChecker(conflictDevice, m_networkInter, this);
        connect(ipChecker, &DeviceIPChecker::conflictStatusChanged, this, &IPConfilctChecker::conflictStatusChanged);
        if (m_ipNeedCheck)
            connect(ipChecker, &DeviceIPChecker::ipConflictCheck, this, &IPConfilctChecker::onSenderIPInfo);
        m_deviceCheckers << ipChecker;
    }

    QStringList ips;
    for (auto it = deviceIps.begin(); it != deviceIps.end(); it++) {
        if (it.value() == conflictDevice)
            ips << it.key();
    }

    ipChecker->setDeviceInfo(ips, macAddress);
    ipChecker->handlerIpConflict();
}

QMap<QString, NetworkDeviceBase *> IPConfilctChecker::parseDeviceIp(const QString &activeConnectionInfo)
{
    QMap<QString, NetworkDeviceBase *> tmpDevicePath;
    QList<NetworkDeviceBase *> devices = m_networkProcesser->devices();
    for (NetworkDeviceBase *device : devices)
        tmpDevicePath[device->path()] = device;

    QJsonParseError error;
    QJsonDocument json = QJsonDocument::fromJson(activeConnectionInfo.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
        return QMap<QString, NetworkDeviceBase *>();

    QMap<QString, NetworkDeviceBase *> deviceIp;
    const QJsonArray &infoArray = json.array();
    for (const QJsonValue &infoValue : infoArray) {
        QJsonObject info = infoValue.toObject();
        if (!info.contains("IPv4") && !info.contains("Ip4"))
            continue;

        const QString devicePath = info.value("Device").toString();
        if (!tmpDevicePath.contains(devicePath))
            continue;

        NetworkDeviceBase *device = tmpDevicePath[devicePath];

        if (info.contains("IPv4")) {
            QJsonObject ipv4TopObject = info.value("IPv4").toObject();
            QJsonArray ipv4Addresses = ipv4TopObject.value("Addresses").toArray();
            for (const QJsonValue addr : ipv4Addresses) {
                const QJsonObject addrObject = addr.toObject();
                QString ipAddr = addrObject.value("Address").toString();
                ipAddr = ipAddr.remove("\"");
                deviceIp[ipAddr] = device;
            }
        } else {
            QJsonValue ipv4Info = info.value("Ip4");
            QJsonObject ipv4Object = ipv4Info.toObject();
            const QString ipv4 = ipv4Object.value("Address").toString();
            if (ipv4.isEmpty())
                continue;

            deviceIp[ipv4] = device;
        }
    }

    return deviceIp;
}

/**
 * @brief 构造函数
 * @param device 对应的设备
 * @param netInter
 * @param parent
 */
DeviceIPChecker::DeviceIPChecker(NetworkDeviceBase *device, NetworkInter *netInter, QObject *parent)
    : QObject(parent)
    , m_device(device)
    , m_networkInter(netInter)
    , m_conflictCount(0)
    , m_clearCount(0)
    , m_count(0)
    , m_ipConflicted(false)
{
    auto requestConflictCheck = [ this ] () {
        // 当设备的IP地址发生变化的时候，请求IP冲突， 只有在上一次请求和该次请求发生的时间大于2秒，才发出信号
        m_ipV4 = m_device->ipv4();
        if (!m_ipV4.isEmpty()) {
            PRINT_INFO_MESSAGE(QString("request Device:%1, IP: %2").arg(m_device->deviceName()).arg(m_ipV4.join(",")));
            m_changeIpv4s << m_ipV4;
            QTimer::singleShot(800, this, [ this ] {
                if (m_changeIpv4s.size() > 0) {
                    emit ipConflictCheck(m_changeIpv4s[m_changeIpv4s.size() - 1]);
                    m_changeIpv4s.clear();
                }
            });
        }
    };
    connect(device, &NetworkDeviceBase::ipV4Changed, this, requestConflictCheck);
    connect(device, &NetworkDeviceBase::enableChanged, this, requestConflictCheck);

    QTimer *timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, [this] () {
        if (m_ipConflicted) {
            // 如果确定是有冲突，则冷却至少5秒检测一次，直到冲突解除。
            PRINT_INFO_MESSAGE(QString("[on] check ip conflict:%1").arg(m_ipV4.join(",")));
            emit ipConflictCheck(m_ipV4);
        } else {
            // 如果无冲突, 则冷却至少180秒检测一次。
            if ((m_count++ % 36) == 0) {
                PRINT_INFO_MESSAGE(QString("[off] check ip conflict:%1").arg(m_ipV4.join(",")));
                emit ipConflictCheck(m_ipV4);
            }
        }
    });
    timer->start(5000);
}

DeviceIPChecker::~DeviceIPChecker()
{
}

NetworkDeviceBase *DeviceIPChecker::device()
{
    return m_device;
}

void DeviceIPChecker::setDeviceInfo(const QStringList &ipv4, const QString &macAddress)
{
    m_ipV4 = ipv4;
    m_macAddress = macAddress;
}

void DeviceIPChecker::handlerIpConflict()
{
    if (m_macAddress.isEmpty()) {
        m_conflictCount = 0;
        // 如果MAC地址为空，则表示IP冲突已经解决，则让每个网卡恢复之前的状态
        if (m_clearCount < 3 && m_ipConflicted) {
            emit ipConflictCheck(m_ipV4);
        } else {
            // 拿到最后一次设备冲突的状态
            bool lastConfilctStatus = m_ipConflicted;
            m_ipConflicted = false;

            // IP冲突状态发生变化的时候才会发送该信号
            if (lastConfilctStatus)
                Q_EMIT conflictStatusChanged(m_device, false);
        }
        m_clearCount++;
    } else {
        m_clearCount = 0;

        // 如果少于两次，则继续确认
        if (m_conflictCount < 1 && !m_ipConflicted) {
            emit ipConflictCheck(m_ipV4);
        } else {
            // 如果大于3次，则认为当前IP冲突了
            // 拿到最后一次设备冲突的状态
            bool lastConflictStatus = m_ipConflicted;
            m_ipConflicted = true;

            // 最后一次IP不冲突，本次IP冲突的时候才发送设备状态变化的信号
            if (!lastConflictStatus)
                Q_EMIT conflictStatusChanged(m_device, true);
        }
        m_conflictCount++;
    }
}

QStringList DeviceIPChecker::ipV4()
{
    return m_ipV4;
}

bool DeviceIPChecker::ipConflicted()
{
    return m_ipConflicted;
}
