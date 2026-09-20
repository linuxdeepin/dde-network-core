// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "internetchecker.h"

#include "settingconfig.h"
#include "constants.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QVariantList>
#include <QRandomGenerator>
#include <QUrl>

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Ipv6Setting>
#include <NetworkManagerQt/Settings>

#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <netinet/in.h>
#include <cstring>
#include <netdb.h>

using namespace network::systemservice;

// 探测结果：Online=可上网，Portal=需 portal 认证，Unreachable=不可上网
enum class network::systemservice::ProbeResult {
    Unreachable = 0,
    Online = 1,
    Portal = -1
};

namespace {
// HTTP 响应体读取上限，超过则视为异常，避免异常/恶意响应导致内存无界增长
constexpr int kMaxResponseSize = 64 * 1024;
}

InternetChecker::InternetChecker(QObject *parent)
    : QObject(parent)
{
}

// 网络切换入口：检测到网络不通时，依次检查其他网卡是否能上网，
// 仅在确认目标网卡可上网后才切换主链接，避免网卡反复切换
void InternetChecker::switchInternetAccess(bool checkPrimaryConnection, bool switchPortal)
{
    // 获取当前主连接对应的网卡 uni 列表
    QStringList primaryDeviceUnis;
    NetworkManager::ActiveConnection::Ptr primaryConnection = NetworkManager::primaryConnection();
    if (!primaryConnection.isNull()) {
        primaryDeviceUnis = primaryConnection->devices();
    }

    // 遍历所有网卡，分离出主连接网卡和候选网卡（有线优先于无线）
    NetworkManager::Device::Ptr primaryDevice;
    NetworkManager::Device::List wiredDevice, wirelessDevice;
    NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    for (const NetworkManager::Device::Ptr &device : devices) {
        if (primaryDeviceUnis.contains(device->uni())) {
            primaryDevice = device;
            continue;
        }

        // 跳过未托管或未启动的网卡
        if (!device->managed() || !(device->interfaceFlags() & IFF_UP)) {
            continue;
        }

        // 跳过没有活跃连接的网卡
        if (device->activeConnection().isNull()) {
            continue;
        }

        if (device->type() == NetworkManager::Device::Type::Ethernet) {
            wiredDevice << device;
        } else if (device->type() == NetworkManager::Device::Type::Wifi) {
            wirelessDevice << device;
        }
    }
    // 有线网卡优先，无线网卡次之
    NetworkManager::Device::List checkedDevices;
    checkedDevices << wiredDevice << wirelessDevice;
    // 如果没有需要切换的网络，则无需继续切换
    if (checkedDevices.isEmpty()) {
        emit switchFailed();
        return;
    }

    if (checkPrimaryConnection && !primaryDevice.isNull()) {
        // 如果需要检查主链接，且此时主链接可以上网，则无需切换，直接告诉外面当前网络状况正常
        if (checkInterfaceOnline(primaryDevice) == ProbeResult::Online) {
            qCInfo(DSM) << "primary device " << primaryDevice->interfaceName() << " is online";
            emit switchSuccess();
            return;
        }
    }
    // 第一轮：检测每个候选网卡，优先切换到可以上网的网卡，同时记录需要 portal 认证的网卡
    NetworkManager::Device::List portalDevices;
    for (const NetworkManager::Device::Ptr &device : checkedDevices) {
        ProbeResult ret = checkInterfaceOnline(device);
        if (ret == ProbeResult::Online && setPrimaryDevice(device, devices)) {
            qCInfo(DSM) << device->interfaceName() << " is online, set it primary device";
            emit switchSuccess();
            return;
        }
        if (switchPortal && ret == ProbeResult::Portal) {
            portalDevices << device;
        }
    }
    // 第二轮：没有可以上网的网卡时，退而求其次切换到需要 portal 认证的网卡
    if (!portalDevices.isEmpty()) {
        const NetworkManager::Device::Ptr &portalDevice = portalDevices.first();
        if (setPrimaryDevice(portalDevice, devices)) {
            qCInfo(DSM) << portalDevice->interfaceName() << " needs portal authentication, set it primary device";
            // portal 网卡切换成功后不能发 switchSuccess，否则会被误判为已可上网；
            // portal 状态由 StatusChecker 自行检测识别（portalDetected / Limited）
            return;
        }
    }
    qCWarning(DSM) << "switch primary device failure";
    emit switchFailed();
}

// 获取网卡当前活跃连接的 IPv4 DNS 服务器列表
QStringList InternetChecker::getDeviceDnsList(const NetworkManager::Device::Ptr &device) const
{
    NetworkManager::ActiveConnection::Ptr activeConnection = device->activeConnection();
    if (activeConnection.isNull()) {
        return {};
    }

    QStringList dnsList;
    NetworkManager::IpConfig config = activeConnection->ipV4Config();
    QList<QHostAddress> addresses = config.nameservers();
    for (const QHostAddress &address : addresses) {
        QString ip = address.toString();
        if (!ip.isEmpty() && !dnsList.contains(ip)) {
            dnsList << ip;
        }
    }

    return dnsList;
}

// 检测指定网卡的联网状态：
// 1. 依次尝试配置的检测 URL（IP 地址直接 TCP 连接，域名则先 DNS 解析再 HTTP 验证）
// 2. 若检测到 portal 认证拦截页则返回 portal 状态
// 3. 若全部失败且未检测到 portal，使用公共 DNS 地址作为兜底 TCP 连接检测
// 返回值：ProbeResult::Online=可上网，ProbeResult::Portal=需 portal 认证，ProbeResult::Unreachable=不可上网
ProbeResult InternetChecker::checkInterfaceOnline(const NetworkManager::Device::Ptr &device) const
{
    QStringList dnsList = getDeviceDnsList(device);
    if (dnsList.isEmpty()) {
        qCWarning(DSM) << "interface " << device->interfaceName() << " doesn't have dns";
        return ProbeResult::Unreachable;
    }

    const int totalTimeout = SettingConfig::instance()->httpRequestTimeout() * 1000;
    QElapsedTimer totalElapsed;
    totalElapsed.start();

    QStringList remoteAddrs;
    QStringList networkUrls = SettingConfig::instance()->networkCheckerUrls();
    int perUrlTimeout = qMax(totalTimeout / qMax(networkUrls.size(), 1), 1000);
    for (const QString &url : networkUrls) {
        int remainTimeout = totalTimeout - static_cast<int>(totalElapsed.elapsed());
        if (remainTimeout <= 0)
            break;

        int curTimeout = qMin(perUrlTimeout, remainTimeout);
        QString host = url;
        quint16 port = 80;
        static QStringList schemePrefixes = {"https://", "http://"};
        for (const QString &prefix : schemePrefixes) {
            if (host.startsWith(prefix)) {
                host = host.remove(0, prefix.length());
                if (prefix.startsWith("https"))
                    port = 443;
                break;
            }
        }
        if (host.endsWith("/")) {
            host = host.left(host.length() - 1);
        }
        in_addr addr;
        if (inet_pton(AF_INET, host.toStdString().c_str(), &addr) == 1) {
            QString remoteIp = QString::fromLatin1(inet_ntoa(addr));
            if (remoteAddrs.contains(remoteIp))
                continue;

            remoteAddrs << remoteIp;
            sockaddr_in target {};
            memset(&target, 0, sizeof(target));
            target.sin_family = AF_INET;
            target.sin_port = htons(port);
            target.sin_addr = addr;
            if (checkReachability(device->interfaceName(), target, QString(), curTimeout, false) == ProbeResult::Online) {
                qDebug(DSM) << "interface " << device->interfaceName() << " test ip " << url << " ok";
                return ProbeResult::Online;
            }
        } else {
            // 如果是域名，先通过指定网卡的 DNS 解析域名，再 HTTP 验证（TCP 成功不一定能上网，portal 会拦截并返回认证页）
            ProbeResult httpRet = checkNetCardOnline(device, host, dnsList, curTimeout, port);
            if (httpRet == ProbeResult::Online) {
                qDebug(DSM) << "interface " << device->interfaceName() << " test url " << url << " ok";
                return ProbeResult::Online;
            }
            if (httpRet == ProbeResult::Portal) {
                qCWarning(DSM) << "interface " << device->interfaceName() << " test url " << url << " needs portal authentication";
                // 一旦识别出 portal 立即返回，不再继续尝试后续 https 检测：
                // portal 会透明代理任意 TCP 连接，443 可达不代表真正能上网，会被误判为 online 覆盖 portal 结果。
                return ProbeResult::Portal;
            }
        }
    }
    // 兜底方案：直接 TCP 连接公共 DNS 的 80 端口，检测网卡是否有基本出口连通性（在一些网络结构中例如手机热点，可能屏蔽了53端口，通过DNS获取IP失败，这里就使用兜底方案）
    static QStringList fallbackDnsList {"223.5.5.5", "223.6.6.6", "114.114.114.114", "8.8.8.8"};
    for (const QString &dns : fallbackDnsList) {
        int remainTimeout = totalTimeout - static_cast<int>(totalElapsed.elapsed());
        if (remainTimeout <= 0)
            break;

        if (remoteAddrs.contains(dns))
            continue;

        int curTimeout = qMin(1000, remainTimeout);
        sockaddr_in target {};
        memset(&target, 0, sizeof(target));
        target.sin_family = AF_INET;
        target.sin_port = htons(80);
        inet_pton(AF_INET, dns.toStdString().c_str(), &target.sin_addr);
        if (checkReachability(device->interfaceName(), target, QString(), curTimeout, false) == ProbeResult::Online) {
            qCDebug(DSM) << "interface " << device->interfaceName() << "test dns " << dns << " ok";
            return ProbeResult::Online;
        }
    }
    qCDebug(DSM) << "interface " << device->interfaceName() << " is offline";
    return ProbeResult::Unreachable;
}

// 通过非阻塞 TCP 连接检测从指定网卡是否可以到达目标地址，
// 使用 SO_BINDTODEVICE 绑定网卡确保流量从该网卡发出；
// httpCheck 为 true 时进一步发送 HTTP GET 请求，识别 portal 认证拦截页
ProbeResult InternetChecker::checkReachability(const QString &ifName, const sockaddr_in &dest, const QString &host, int timeoutMs, bool httpCheck) const
{
    int sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
        return ProbeResult::Unreachable;

    // 整个探测流程（TCP 连接 + HTTP 响应读取）共享同一个超时预算，
    // 避免连接阶段和响应读取阶段各自消耗一个完整超时导致总耗时翻倍
    QElapsedTimer totalTimer;
    totalTimer.start();

    // 绑定网卡设备，所有流量从该网卡发出
    struct ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifName.toStdString().c_str(), IFNAMSIZ - 1);
    if (setsockopt(sockFd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        close(sockFd);
        return ProbeResult::Unreachable;
    }

    // 设为非阻塞，用 select 控制连接超时
    int flags = fcntl(sockFd, F_GETFL, 0);
    fcntl(sockFd, F_SETFL, flags | O_NONBLOCK);

    int connectRet = ::connect(sockFd, (sockaddr*)&dest, sizeof(dest));
    if (connectRet < 0 && errno != EINPROGRESS) {
        close(sockFd);
        return ProbeResult::Unreachable;
    }

    if (connectRet != 0) {
        // 等待连接完成（非阻塞模式下 EINPROGRESS 需要通过 select 检测可写事件）
        int remainTimeout = timeoutMs - static_cast<int>(totalTimer.elapsed());
        if (remainTimeout <= 0) {
            close(sockFd);
            return ProbeResult::Unreachable;
        }
        fd_set wfds{};
        FD_ZERO(&wfds);
        FD_SET(sockFd, &wfds);
        timeval tv{};
        tv.tv_sec = remainTimeout / 1000;
        tv.tv_usec = (remainTimeout % 1000) * 1000;
        int selRet = select(sockFd + 1, nullptr, &wfds, nullptr, &tv);
        if (selRet <= 0) {
            close(sockFd);
            return ProbeResult::Unreachable;
        }
        // 检查连接是否真正成功（select 可写不代表连接成功，需通过 SO_ERROR 确认）
        int err = 0;
        socklen_t errLen = sizeof(err);
        getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &err, &errLen);
        if (err != 0) {
            close(sockFd);
            return ProbeResult::Unreachable;
        }
    }

    // 纯 TCP 探测：三次握手成功即视为可达
    if (!httpCheck) {
        close(sockFd);
        return ProbeResult::Online;
    }

    // HTTP 探测：portal 会把任意 HTTP 请求透明拦截并返回认证页，仅靠 TCP 握手无法识别，
    // 因此需要发 GET 并校验响应头/响应体。
    QByteArray hostUtf8 = host.toUtf8();
    QByteArray request = "GET / HTTP/1.1\r\n"
                         "Host: " + hostUtf8 + "\r\n"
                         "User-Agent: DdeNetworkService/1.0\r\n"
                         "Accept: */*\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    // 非阻塞 socket 上 send 可能只写入部分数据，循环发送剩余部分直到全部写完；
    // 发送缓冲区满（EAGAIN）时用 select 等待可写事件后再重试，避免忙等。
    size_t totalSent = 0;
    while (totalSent < static_cast<size_t>(request.size())) {
        ssize_t n = send(sockFd, request.constData() + totalSent, request.size() - totalSent, MSG_NOSIGNAL);
        if (n > 0) {
            totalSent += static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            int remainTimeout = timeoutMs - static_cast<int>(totalTimer.elapsed());
            if (remainTimeout <= 0) {
                close(sockFd);
                return ProbeResult::Unreachable;
            }
            fd_set wfds{};
            FD_ZERO(&wfds);
            FD_SET(sockFd, &wfds);
            timeval tv{};
            tv.tv_sec = remainTimeout / 1000;
            tv.tv_usec = (remainTimeout % 1000) * 1000;
            int selRet = select(sockFd + 1, nullptr, &wfds, nullptr, &tv);
            if (selRet <= 0) {
                close(sockFd);
                return ProbeResult::Unreachable;
            }
            continue;
        }
        close(sockFd);
        return ProbeResult::Unreachable;
    }

    QByteArray response;
    char buf[4096];
    while (response.size() < kMaxResponseSize) {
        int remainTimeout = timeoutMs - static_cast<int>(totalTimer.elapsed());
        if (remainTimeout <= 0)
            break;

        fd_set rfds{};
        FD_ZERO(&rfds);
        FD_SET(sockFd, &rfds);
        timeval tv{};
        tv.tv_sec = remainTimeout / 1000;
        tv.tv_usec = (remainTimeout % 1000) * 1000;
        int selRet = select(sockFd + 1, &rfds, nullptr, nullptr, &tv);
        if (selRet <= 0)
            break;

        ssize_t n = recv(sockFd, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                continue;
            break; // n == 0 表示对端关闭连接
        }
        response.append(buf, static_cast<int>(n));
    }
    close(sockFd);

    if (response.isEmpty())
        return ProbeResult::Unreachable;

    // 分离响应头和响应体
    int headerEnd = response.indexOf("\r\n\r\n");
    QByteArray headerBlock = headerEnd >= 0 ? response.left(headerEnd) : response;
    QByteArray bodyBlock = headerEnd >= 0 ? response.mid(headerEnd + 4) : QByteArray();

    // 解析状态码
    int statusLineEnd = headerBlock.indexOf("\r\n");
    QByteArray statusLine = statusLineEnd >= 0 ? headerBlock.left(statusLineEnd) : headerBlock;
    int code = 0;
    int sp1 = statusLine.indexOf(' ');
    if (sp1 >= 0) {
        int sp2 = statusLine.indexOf(' ', sp1 + 1);
        QByteArray codePart = sp2 >= 0 ? statusLine.mid(sp1 + 1, sp2 - sp1 - 1) : statusLine.mid(sp1 + 1);
        code = codePart.toInt();
    }
    if (code == 0)
        return ProbeResult::Unreachable;

    // 解析 Location 头，portal 通常会把请求重定向到认证服务器。
    // 需按行首匹配，避免误命中 Content-Location: 等其他包含 location 字样的头。
    QString location;
    QByteArray lowerHeader = headerBlock.toLower();
    int locIdx = lowerHeader.indexOf("\r\nlocation:");
    if (locIdx < 0 && lowerHeader.startsWith("location:"))
        locIdx = 0;
    if (locIdx >= 0) {
        int valStart = headerBlock.indexOf(':', locIdx) + 1;
        int valEnd = headerBlock.indexOf("\r\n", valStart);
        if (valEnd < 0)
            valEnd = headerBlock.size();
        location = QString::fromLatin1(headerBlock.mid(valStart, valEnd - valStart).trimmed());
    }

    QUrl locationUrl(location);
    QString locationHost = locationUrl.host();

    // 关键判断：portal 会把任意请求重定向到认证服务器（不同主机），
    // 而正常站点（如 http→https、加 www）的重定向通常仍在原主机上。
    // 因此“Location 主机与原请求主机不同”即可判为 portal，不区分 2xx/3xx。
    if (!locationHost.isEmpty() && locationHost.compare(host, Qt::CaseInsensitive) != 0) {
        qCWarning(DSM) << "interface " << ifName << " http check for " << host << " redirected to " << location;
        return ProbeResult::Portal;
    }

    // portal 的 200 页面可能没有 Location 头，而是通过 meta refresh 跳转到认证页
    QString lowerBody = QString::fromLatin1(bodyBlock).toLower();
    // portal 页面标记：优先采用明确的 portal 特征，避免 url=http、单独的 meta refresh 等宽泛子串误判正常网页。
    // meta refresh 仅在同时携带 url= 跳转参数时（portal 常见跳转方式）才视为 portal 标记。
    bool hasMetaRefresh = lowerBody.contains("meta http-equiv=\"refresh\"");
    bool hasRedirectUrl = lowerBody.contains("url=");
    bool portalMarker = lowerBody.contains("web authentication")
                        || lowerBody.contains("captive portal")
                        || lowerBody.contains("portal.html")
                        || lowerBody.contains("walled garden")
                        || (hasMetaRefresh && hasRedirectUrl);
    if (portalMarker) {
        qCWarning(DSM) << "interface " << ifName << " http check for " << host << " detected portal page";
        return ProbeResult::Portal;
    }

    qCDebug(DSM) << "interface " << ifName << " http check for " << host << " ok, status " << code;
    return ProbeResult::Online;
}

// 检测网卡是否可以访问指定域名：先通过该网卡绑定的 DNS 解析域名得到 IP，
// 再通过该网卡 TCP 连接解析出的 IP 地址；对 80 端口进一步做 HTTP 探测以识别 portal 认证拦截页
ProbeResult InternetChecker::checkNetCardOnline(const NetworkManager::Device::Ptr &device, const QString &domain, const QStringList &dnslist, int timeoutMs, int port) const
{
    in_addr targetIp{};
    QElapsedTimer elapsed;
    elapsed.start();
    int halfTimeout = timeoutMs / 2;
    if (!resolveByBindIface(device, domain, dnslist, targetIp, halfTimeout))
        return ProbeResult::Unreachable;

    int remainTimeout = timeoutMs - static_cast<int>(elapsed.elapsed());
    if (remainTimeout <= 0)
        return ProbeResult::Unreachable;

    sockaddr_in dest{};
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(static_cast<quint16>(port));
    dest.sin_addr = targetIp;

    const bool httpCheck = (port == 80);
    return checkReachability(device->interfaceName(), dest, httpCheck ? domain : QString(), remainTimeout, httpCheck);
}

// 绑定指定网卡进行 DNS 解析，按优先级依次尝试三种方式：
// 1. UDP DNS 查询（常规方式）
// 2. TCP DNS 查询（UDP 53 端口被屏蔽时的 fallback，如手机热点场景）
// 3. 系统 getaddrinfo（兜底方案，使用系统默认路由解析）
bool InternetChecker::resolveByBindIface(const NetworkManager::Device::Ptr &device, const QString &domain, const QStringList &dnslist, in_addr &outIp, int timeout) const
{
    // 将总超时平均分配给每个 DNS 服务器，每个至少 1 秒
    int perDnsTimeout = qMax(timeout / qMax(dnslist.size(), 1), 1000);
    int remainTimeout = timeout;
    QElapsedTimer elapsed;
    elapsed.start();

    // 第一步：通过 UDP 方式向各 DNS 服务器发送 A 记录查询
    for (const QString &dnsIp : dnslist) {
        if (remainTimeout <= 0)
            break;

        int curTimeout = qMin(perDnsTimeout, remainTimeout);
        if (checkIpAddrByUDP(device, dnsIp, domain, curTimeout, outIp)) {
            qCDebug(DSM) << "check dns success by step 1, elapse time: " << elapsed.elapsed() << "seconds";
            return true;
        }
        remainTimeout -= elapsed.elapsed();
        elapsed.restart();
    }

    // 第二步：如果 UDP 方式全部失败，改用 TCP 方式重试。
    // 适用场景：连接手机热点时，运营商可能屏蔽 UDP 53 端口，但 TCP 53 端口可用
    for (const QString &dnsIp : dnslist) {
        if (remainTimeout <= 0)
            break;

        int curTimeout = qMin(perDnsTimeout, remainTimeout);
        if (checkIpAddrByTCP(device, dnsIp, domain, curTimeout, outIp)) {
            qCDebug(DSM) << "check dns success by step 2, elapse time: " << elapsed.elapsed() << "seconds";
            return true;
        }
        remainTimeout -= elapsed.elapsed();
        elapsed.restart();
    }

    // 第三步：兜底方案，使用系统 getaddrinfo 解析（走系统默认路由，不绑定网卡）
    struct addrinfo hints{}, *res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int gaiRet = getaddrinfo(domain.toStdString().c_str(), nullptr, &hints, &res);
    if (gaiRet == 0 && res != nullptr) {
        for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
            if (p->ai_family == AF_INET) {
                outIp = ((struct sockaddr_in *)p->ai_addr)->sin_addr;
                freeaddrinfo(res);
                qCDebug(DSM) << "search dns success by step 3, elapse time: " << elapsed.elapsed() << "seconds";
                return true;
            }
        }
        freeaddrinfo(res);
    }

    return false;
}

// 构造标准 DNS A 记录查询包（RFC 1035 格式）
// 返回写入包的字节数，出错返回 -1（域名标签超过 63 字节或包超过 pktSize）
int InternetChecker::buildDnsQueryPacket(const QString &domain, unsigned char *pkt, int pktSize, unsigned short txId)
{
    memset(pkt, 0, pktSize);
    int needed = 12; // DNS 头部固定 12 字节

    // 计算问段域名编码后所需的长度：每个标签为 1 字节长度 + 标签内容
    QStringList seg = domain.split(".");
    for (const auto &s : seg) {
        QByteArray buf = s.toUtf8();
        if (buf.size() > 63) // RFC 1035: 单个标签不超过 63 字节
            return -1;
        needed += 1 + buf.size();
    }
    needed += 1 + 4; // 域名结尾 null 字节 + QTYPE(2) + QCLASS(2)

    if (needed > pktSize)
        return -1;

    // 填充 DNS 头部：ID、标准查询标志、QDCOUNT=1
    unsigned short id = htons(txId);
    memcpy(pkt, &id, 2);
    pkt[2] = 0x01; pkt[3] = 0x00; // Flags: RD=1, 其余为 0（标准查询）
    unsigned short qd = htons(1);
    memcpy(pkt + 4, &qd, 2);

    // 填充问段：将域名按 "." 分割，每段前加长度字节，以 null 字节结尾
    int ptr = 12;
    for (const auto &s : seg) {
        QByteArray buf = s.toUtf8();
        pkt[ptr++] = static_cast<unsigned char>(buf.size());
        memcpy(pkt + ptr, buf.data(), buf.size());
        ptr += buf.size();
    }
    pkt[ptr++] = 0; // 域名结束标记
    // QTYPE=A(1), QCLASS=IN(1)
    unsigned short typeA = htons(1), cls = htons(1);
    memcpy(pkt + ptr, &typeA, 2); ptr += 2;
    memcpy(pkt + ptr, &cls, 2); ptr += 2;

    return ptr;
}

// 解析 DNS 响应包，在 Answer Section 中查找 A 记录（TYPE=1），
// 找到后将 IPv4 地址写入 outIp 并返回 true
// 跳过非 A 记录（如 CNAME 等），遇到解析异常时跳过当前记录继续尝试，而非直接失败
bool InternetChecker::parseDnsResponse(const unsigned char *buf, int len, unsigned short txId, in_addr &outIp)
{
    if (len <= 12)
        return false;

    // 校验响应 ID 和 QR 标志位（bit 15: 1=响应）
    unsigned short respId;
    memcpy(&respId, buf, 2);
    respId = ntohs(respId);
    if (respId != txId || !(buf[2] & 0x80))
        return false;

    int p = 12; // 跳过 12 字节 DNS 头部
    unsigned short qnum;
    memcpy(&qnum, buf + 4, 2);
    qnum = ntohs(qnum);

    unsigned short anNum;
    memcpy(&anNum, buf + 6, 2);
    anNum = ntohs(anNum);

    // 跳过 Question Section：每个问题包含域名 + QTYPE(2) + QCLASS(2)
    for (int i = 0; i < qnum; i++) {
        bool compressed = false;
        // 跳过域名：逐标签跳过，遇到压缩指针（高两 bit 为 11）则跳过 2 字节
        while (p < len && buf[p] != 0) {
            if ((buf[p] & 0xC0) == 0xC0) {
                if (p + 2 > len)
                    break;
                p += 2;
                compressed = true;
                break;
            }
            p += buf[p] + 1;
        }
        // 压缩指针后紧跟 TYPE 字段，没有 null 终止符，所以只在非压缩时跳过 null
        if (!compressed && p < len && buf[p] == 0)
            p++;
        if (p + 4 > len)
            break;
        p += 4; // 跳过 QTYPE(2) + QCLASS(2)
    }

    // 遍历 Answer Section，查找 A 记录
    for (int i = 0; i < anNum && p + 12 <= len; i++) {
        bool compressed = false;
        // 跳过 Answer 中的域名（可能使用压缩指针指向 Question 中的域名）
        while (p < len && buf[p] != 0) {
            if ((buf[p] & 0xC0) == 0xC0) {
                if (p + 2 > len)
                    break;
                p += 2;
                compressed = true;
                break;
            }
            p += buf[p] + 1;
        }
        if (!compressed && p < len && buf[p] == 0)
            p++;
        // 剩余至少需要 10 字节：TYPE(2) + CLASS(2) + TTL(4) + RDLENGTH(2)
        if (p + 10 > len)
            break;

        unsigned short rtype;
        memcpy(&rtype, buf + p, 2);
        rtype = ntohs(rtype);
        p += 8; // 跳过 TYPE(2) + CLASS(2) + TTL(4)

        unsigned short rdLen;
        memcpy(&rdLen, buf + p, 2);
        rdLen = ntohs(rdLen);
        p += 2; // 跳过 RDLENGTH(2)

        // 找到 A 记录：TYPE=1，RDATA 长度固定 4 字节（IPv4 地址）
        if (rtype == 1 && rdLen == 4 && p + 4 <= len) {
            memcpy(&outIp, buf + p, 4);
            return true;
        }
        // 非 A 记录（如 CNAME、TXT 等），根据 RDLENGTH 跳过 RDATA
        if (p + rdLen > len)
            break;
        p += rdLen;
    }

    return false;
}

// 通过 UDP 向指定 DNS 服务器发送 A 记录查询，绑定网卡确保流量从该网卡发出
bool InternetChecker::checkIpAddrByUDP(const NetworkManager::Device::Ptr &device, const QString &dnsIp, const QString &domain,
    int curTimeout, in_addr &outIp) const
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return false;

    // 绑定网卡设备
    struct ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device->interfaceName().toStdString().c_str(), IFNAMSIZ - 1);
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        close(sock);
        return false;
    }

    // 使用网卡自身的 IPv4 地址绑定源地址，确保从该网卡发出
    bool bound = false;
    NetworkManager::IpAddresses ipv4AddrList = device->ipV4Config().addresses();
    for (const NetworkManager::IpAddress &ipAddr : ipv4AddrList) {
        sockaddr_in localAddr{};
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = 0;
        inet_pton(AF_INET, ipAddr.ip().toString().toUtf8().constData(), &localAddr.sin_addr);
        if (::bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) == 0) {
            bound = true;
            break;
        }
    }

    if (!bound) {
        close(sock);
        return false;
    }

    // 构造 DNS 服务器地址（端口 53）
    sockaddr_in dnsAddr;
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_port = htons(53);
    if (inet_pton(AF_INET, dnsIp.toStdString().c_str(), &dnsAddr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    // 构造并发送 DNS 查询包
    unsigned short txId = static_cast<unsigned short>(QRandomGenerator::global()->generate() % 65536);
    unsigned char pkt[512];
    int pktLen = buildDnsQueryPacket(domain, pkt, sizeof(pkt), txId);
    if (pktLen < 0) {
        qCWarning(DSM) << "DNS query packet too large for domain:" << domain;
        close(sock);
        return false;
    }

    ssize_t sent = sendto(sock, pkt, pktLen, MSG_NOSIGNAL, (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (sent < 0) {
        qCWarning(DSM) << "sendto failed for" << device->interfaceName() << "dns:" << dnsIp << "errno:" << errno << strerror(errno);
        close(sock);
        return false;
    }

    // 等待响应并用 select 控制超时
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    timeval tv;
    tv.tv_sec = curTimeout / 1000;
    tv.tv_usec = (curTimeout % 1000) * 1000;

    bool succ = false;
    if (select(sock + 1, &rfds, nullptr, nullptr, &tv) > 0) {
        unsigned char respBuf[512];
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);
        ssize_t rlen = recvfrom(sock, reinterpret_cast<char *>(respBuf), sizeof(respBuf), 0,
                               (sockaddr *)&fromAddr, &fromLen);
        if (rlen > 12
            && fromAddr.sin_family == AF_INET
            && fromAddr.sin_addr.s_addr == dnsAddr.sin_addr.s_addr
            && fromAddr.sin_port == dnsAddr.sin_port) {
            succ = parseDnsResponse(respBuf, static_cast<int>(rlen), txId, outIp);
        } else if (rlen > 0) {
            qCWarning(DSM) << "DNS response from unexpected source, discarded:" << inet_ntoa(fromAddr.sin_addr);
        }
    }

    close(sock);
    return succ;
}

// 通过 TCP 向指定 DNS 服务器发送 A 记录查询（DNS over TCP，RFC 7766）
// TCP DNS 消息前有 2 字节长度前缀，与 UDP 格式不同
// 适用场景：手机热点等环境下 UDP 53 端口可能被屏蔽，但 TCP 53 端口可用
bool InternetChecker::checkIpAddrByTCP(const NetworkManager::Device::Ptr &device, const QString &dnsIp, const QString &domain,
    int curTimeout, in_addr &outIp) const
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;

    // 绑定网卡设备
    struct ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device->interfaceName().toStdString().c_str(), IFNAMSIZ - 1);
    if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) < 0) {
        close(sock);
        return false;
    }

    // 使用网卡自身的 IPv4 地址绑定源地址
    bool bound = false;
    NetworkManager::IpAddresses ipv4AddrList = device->ipV4Config().addresses();
    for (const NetworkManager::IpAddress &ipAddr : ipv4AddrList) {
        sockaddr_in localAddr{};
        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = 0;
        inet_pton(AF_INET, ipAddr.ip().toString().toUtf8().constData(), &localAddr.sin_addr);
        if (::bind(sock, (sockaddr *)&localAddr, sizeof(localAddr)) == 0) {
            bound = true;
            break;
        }
    }

    if (!bound) {
        close(sock);
        return false;
    }

    // 构造 DNS 服务器地址（端口 53）
    sockaddr_in dnsAddr;
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    dnsAddr.sin_family = AF_INET;
    dnsAddr.sin_port = htons(53);
    if (inet_pton(AF_INET, dnsIp.toStdString().c_str(), &dnsAddr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    // 非阻塞连接 DNS 服务器，用 select 控制连接超时（最多 2 秒）
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    int connectRet = ::connect(sock, (sockaddr*)&dnsAddr, sizeof(dnsAddr));
    if (connectRet < 0 && errno != EINPROGRESS) {
        close(sock);
        return false;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    int connectTimeout = qMin(curTimeout, 2000);
    timeval connectTv;
    connectTv.tv_sec = connectTimeout / 1000;
    connectTv.tv_usec = (connectTimeout % 1000) * 1000;

    bool succ = false;
    int selRet = select(sock + 1, nullptr, &wfds, nullptr, &connectTv);
    if (selRet > 0) {
        int err = 0;
        socklen_t errLen = sizeof(err);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errLen);
        if (err == 0) {
            // 连接成功，构造 DNS 查询包并加上 TCP 2 字节长度前缀
            unsigned short txId = static_cast<unsigned short>(QRandomGenerator::global()->generate() % 65536);
            unsigned char pkt[512];
            int pktLen = buildDnsQueryPacket(domain, pkt, sizeof(pkt), txId);
            if (pktLen < 0) {
                qCWarning(DSM) << "DNS query packet too large for domain:" << domain;
                close(sock);
                return false;
            }

            // TCP DNS 消息格式：2 字节大端长度前缀 + DNS 报文内容
            unsigned char tcpPkt[514];
            unsigned short pktLenH = htons(pktLen);
            memcpy(tcpPkt, &pktLenH, 2);
            memcpy(tcpPkt + 2, pkt, pktLen);

            ssize_t sent = send(sock, tcpPkt, pktLen + 2, MSG_NOSIGNAL);
            if (sent == pktLen + 2) {
                // 用 select 等待响应可读，然后在非阻塞模式下一次性读完缓冲区。
                // 保持非阻塞模式的原因：如果对端返回非 DNS 数据（如手机热点拦截页面），
                // 非阻塞 recv 会立即返回可读数据或 EAGAIN，不会像阻塞 MSG_WAITALL 那样死等垃圾数据
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(sock, &rfds);
                timeval recvTv;
                recvTv.tv_sec = curTimeout / 1000;
                recvTv.tv_usec = (curTimeout % 1000) * 1000;

                if (select(sock + 1, &rfds, nullptr, nullptr, &recvTv) > 0) {
                    unsigned char respBuf[514];
                    // 非阻塞循环读取，EAGAIN 时自然退出
                    ssize_t totalRecv = 0;
                    while (totalRecv < 514) {
                        ssize_t n = recv(sock, reinterpret_cast<char *>(respBuf + totalRecv), 514 - totalRecv, 0);
                        if (n <= 0)
                            break;
                        totalRecv += n;
                    }
                    // 解析 TCP DNS 响应：前 2 字节为长度前缀，后面才是 DNS 报文
                    int parseOff = 0;
                    int parseLen = totalRecv;
                    if (totalRecv > 2) {
                        unsigned short hdrLen;
                        memcpy(&hdrLen, respBuf, 2);
                        hdrLen = ntohs(hdrLen);
                        if (hdrLen > 12 && hdrLen < 4096) {
                            parseOff = 2;
                            // 取实际接收量和声明长度的较小值，防止越界
                            parseLen = hdrLen < (totalRecv - 2) ? hdrLen : (totalRecv - 2);
                        }
                    }
                    if (parseLen > 12) {
                        succ = parseDnsResponse(respBuf + parseOff, parseLen, txId, outIp);
                    }
                }
            }
        }
    }

    close(sock);
    return succ;
}

// 通过调整网卡路由 metric 值切换主链接：
// 目标网卡设置低 metric（优先级高），其余网卡设置高 metric（优先级低），
// 然后通过 D-Bus 调用 NetworkManager 的 Reapply 接口使配置生效
bool InternetChecker::setPrimaryDevice(const NetworkManager::Device::Ptr &device, const NetworkManager::Device::List &allDevices)
{
    if (device->activeConnection().isNull() || device->activeConnection()->state() != NetworkManager::ActiveConnection::Activated) {
        qCWarning(DSM) << "device " << device->interfaceName() << "is not active";
        return false;
    }

    const quint32 primaryMetric = 100;
    const quint32 secondaryMetric = 1000;
    QMap<NetworkManager::Device::Ptr, NMVariantMapMap> deviceSettings;
    for (const NetworkManager::Device::Ptr &dev : allDevices) {
        // 跳过没有活跃连接的网卡
        if (dev->activeConnection().isNull())
            continue;

        auto ac = dev->activeConnection();
        if (!ac || ac->state() != NetworkManager::ActiveConnection::Activated)
            continue;

        bool isTarget = (dev == device);
        quint32 metric = isTarget ? primaryMetric : secondaryMetric;

        NetworkManager::Connection::Ptr conn = ac->connection();
        NetworkManager::ConnectionSettings::Ptr settings = conn->settings();

        bool needReapply = false;
        NetworkManager::Ipv4Setting::Ptr ipv4 = settings->setting(NetworkManager::Setting::Ipv4).staticCast<NetworkManager::Ipv4Setting>();
        if (ipv4) {
            ipv4->setRouteMetric(metric);
            ipv4->setInitialized(true);
            needReapply = true;
        }

        NetworkManager::Ipv6Setting::Ptr ipv6 = settings->setting(NetworkManager::Setting::Ipv6).staticCast<NetworkManager::Ipv6Setting>();
        if (ipv6) {
            ipv6->setRouteMetric(metric);
            ipv6->setInitialized(true);
            needReapply = true;
        }

        if (needReapply) {
            conn->update(settings->toMap());
            deviceSettings[dev] = conn->settings()->toMap();
        }
    }

    if (deviceSettings.isEmpty()) {
        qCWarning(DSM) << "can't found device for change route";
        return false;
    }

    // 通过 D-Bus 调用 Reapply 使修改的路由 metric 生效
    // Reapply 签名: (a{sa{sv}}tu) — settings_map, version_id(0), flags(0)
    for (auto it = deviceSettings.constBegin(); it != deviceSettings.constEnd(); ++it) {
        const NetworkManager::Device::Ptr &device = it.key();
        const NMVariantMapMap &settingMap = it.value();
        device->reapplyConnection(settingMap, 0, SettingConfig::instance()->reapplyFlags());
    }

    return true;
}
