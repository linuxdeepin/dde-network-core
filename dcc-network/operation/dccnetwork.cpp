// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dccnetwork.h"

#include "dde-control-center/dccfactory.h"
#include "netitemmodel.h"
#include "netmanager.h"

#include <NetworkManagerQt/GenericTypes>

#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QHostAddress>
#include <QThread>
#include <QtQml/QQmlEngine>

// #include "dccfactory.h"

namespace dde {
namespace network {
DccNetwork::DccNetwork(QObject *parent)
    : QObject(parent)
    , m_manager(nullptr)
{
    qRegisterMetaType<QList<QVariantMap>>("NMVariantMapList");
    qmlRegisterType<NetType>("org.deepin.dcc.network", 1, 0, "NetType");
    qmlRegisterType<NetItemModel>("org.deepin.dcc.network", 1, 0, "NetItemModel");
    qmlRegisterType<NetManager>("org.deepin.dcc.network", 1, 0, "NetManager");
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

NetItem *DccNetwork::root() const
{
    return m_manager->root();
}

bool DccNetwork::CheckPasswordValid(const QString &key, const QString &password)
{
    return NetManager::CheckPasswordValid(key, password);
}

void DccNetwork::exec(NetManager::CmdType cmd, const QString &id, const QVariantMap &param)
{
    switch (cmd) {
    case NetManager::SetConnectInfo: {
        QVariantMap tmpParam = param;
        if (param.contains("ipv4")) {
            QVariantMap ipData = param.value("ipv4").toMap();
            if (ipData.contains("addresses")) {
                const QVariantList &addressData = ipData.value("addresses").toList();
                QList<QList<uint>> addressesData;
                for (auto it : addressData) {
                    QList<uint> address;
                    for (auto num : it.toList()) {
                        address.append(num.toUInt());
                    }
                    addressesData.append(address);
                }
                ipData["addresses"] = QVariant::fromValue(addressesData);
            }
            if (ipData.contains("dns")) {
                const QVariantList &dnsData = ipData.value("dns").toList();
                QList<uint> dns;
                for (auto it : dnsData) {
                    // 支持两种DNS格式：数字格式（IPv4）和字符串格式（IPv6）
                    if (it.type() == QVariant::UInt || it.type() == QVariant::Int) {
                        dns.append(it.toUInt());
                    } else if (it.type() == QVariant::String) {
                        QString dnsStr = it.toString();
                        if (!dnsStr.isEmpty()) {
                            // IPv6地址需要通过QHostAddress转换为数字表示
                            QHostAddress addr(dnsStr);
                            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                                // IPv4字符串转换为数字
                                dns.append(addr.toIPv4Address());
                            } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
                                // IPv6地址转换为128位表示（当前系统可能不支持，先跳过）
                                qWarning() << "IPv6 DNS not fully implemented in backend, DNS:" << dnsStr;
                                // 这里需要实现IPv6 DNS的完整支持
                            }
                        }
                    }
                }
                ipData["dns"] = QVariant::fromValue(dns);
            }
            tmpParam["ipv4"] = QVariant::fromValue(ipData);
        }
        if (param.contains("ipv6")) {
            QVariantMap ipv6Data = param.value("ipv6").toMap();
            
            // 处理IPv6地址
            if (ipv6Data.contains("address-data")) {
                const QVariantList &addressData = ipv6Data.value("address-data").toList();
                QString gatewayStr = ipv6Data.value("gateway").toString();
                IpV6DBusAddressList ipv6AddressList;
                for (auto it : addressData) {
                    IpV6DBusAddress ipv6Address;
                    QVariantMap ipv6AddrData = it.toMap();
                    QHostAddress ip(ipv6AddrData.value("address").toString());
                    QIPv6Address ipv6Addr = ip.toIPv6Address();
                    QByteArray tmpAddress((char *)(ipv6Addr.c), 16);
                    ipv6Address.address = tmpAddress;
                    ipv6Address.prefix = ipv6AddrData.value("prefix").toUInt();
                    QHostAddress gateway(ipv6AddressList.isEmpty() ? gatewayStr : QString());
                    QByteArray tmpGateway((char *)(gateway.toIPv6Address().c), 16);
                    ipv6Address.gateway = tmpGateway;
                    ipv6AddressList.append(ipv6Address);
                }
                ipv6Data["addresses"] = QVariant::fromValue(ipv6AddressList);
            }
            
            // 处理IPv6 DNS
            if (ipv6Data.contains("dns")) {
                const QVariantList &dnsData = ipv6Data.value("dns").toList();
                QList<QHostAddress> ipv6DnsAddresses;
                for (auto it : dnsData) {
                    if (it.type() == QVariant::String) {
                        QString dnsStr = it.toString();
                        if (!dnsStr.isEmpty()) {
                            QHostAddress addr(dnsStr);
                            if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
                                ipv6DnsAddresses.append(addr);
                            }
                        }
                    }
                }
                if (!ipv6DnsAddresses.isEmpty()) {
                    // 直接使用字符串列表格式 - NetworkManager配置文件实际存储字符串
                    QStringList ipv6DnsStrings;
                    for (const QHostAddress &addr : ipv6DnsAddresses) {
                        ipv6DnsStrings.append(addr.toString());
                    }
                    
                    // 使用字符串列表格式保存IPv6 DNS
                    ipv6Data["dns"] = ipv6DnsStrings;
                    ipv6Data["ignore-auto-dns"] = true;
                } else {
                    // 没有有效的IPv6 DNS时，确保移除ignore-auto-dns设置
                    // 这样系统可以恢复自动获取DNS
                    ipv6Data.remove("dns");
                    ipv6Data.remove("ignore-auto-dns");
                }
                
                // 确保IPv6设置存在，即使没有DNS也要设置，以便被正确处理
                if (ipv6Data.isEmpty()) {
                    ipv6Data["method"] = "auto";  // 至少设置一个值确保IPv6部分存在
                }
            }
            
            tmpParam["ipv6"] = QVariant::fromValue(ipv6Data);
        }
        m_manager->exec(cmd, id, tmpParam);
    } break;
    default:
        m_manager->exec(cmd, id, param);
        break;
    }
}

void DccNetwork::setClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

bool DccNetwork::netCheckAvailable()
{
    if (!m_manager) {
        return false;
    }
    
    return m_manager->netCheckAvailable();
}

QVariantMap DccNetwork::toMap(QMap<QString, QString> map)
{
    QVariantMap retMap;
    for (auto it = map.cbegin(); it != map.cend(); it++) {
        retMap.insert(it.key(), it.value());
    }
    return retMap;
}

QMap<QString, QString> DccNetwork::toStringMap(QVariantMap map)
{
    QMap<QString, QString> retMap;
    for (auto it = map.cbegin(); it != map.cend(); it++) {
        retMap.insert(it.key(), it.value().toString());
    }
    return retMap;
}

void DccNetwork::init()
{
    m_manager = new NetManager(NetType::Net_DccFlags, this);
    m_manager->updateLanguage(QLocale().name());
    connect(m_manager, &NetManager::request, this, &DccNetwork::request);
    Q_EMIT managerChanged(m_manager);
    Q_EMIT rootChanged();
}
DCC_FACTORY_CLASS(DccNetwork)
} // namespace network
} // namespace dde

#include "dccnetwork.moc"
