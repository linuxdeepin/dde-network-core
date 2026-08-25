// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef INTERNETCHECKER_H
#define INTERNETCHECKER_H

#include <QObject>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/Device>

namespace network {
namespace systemservice {

class InternetChecker : public QObject
{
    Q_OBJECT

public:
    explicit InternetChecker(QObject *parent = nullptr);
    ~InternetChecker() override = default;
    void switchInternetAccess(bool checkPrimaryConnection = false);

signals:
    void switchSuccess();
    void switchFailed();

private:
    QStringList getDeviceDnsList(const NetworkManager::Device::Ptr &device) const;
    bool checkInterfaceOnline(const NetworkManager::Device::Ptr &device) const;
    bool isIfaceReachable(const QString &ifName, const sockaddr_in &dest, int timeoutMs) const;
    bool checkNetCardOnline(const NetworkManager::Device::Ptr &device, const QString &domain, const QStringList &dnslist, int timeoutMs) const;
    bool resolveByBindIface(const NetworkManager::Device::Ptr &device, const QString &domain, const QStringList &dnslist, in_addr &outIp, int timeout) const;
    bool checkIpAddrByUDP(const NetworkManager::Device::Ptr &device, const QString &dnsIp, const QString &domain, int curTimeout, in_addr &outIp) const;
    bool checkIpAddrByTCP(const NetworkManager::Device::Ptr &device, const QString &dnsIp, const QString &domain, int curTimeout, in_addr &outIp) const;
    bool setPrimaryDevice(const NetworkManager::Device::Ptr &device, const NetworkManager::Device::List &allDevices);

    // DNS辅助函数：构造查询包，返回写入的字节数；出错返回-1
    static int buildDnsQueryPacket(const QString &domain, unsigned char *pkt, int pktSize, unsigned short txId);
    // DNS辅助函数：解析响应，成功返回true并填充outIp
    static bool parseDnsResponse(const unsigned char *buf, int len, unsigned short txId, in_addr &outIp);
};

}
}

#endif // INTERNETCHECKER_H
