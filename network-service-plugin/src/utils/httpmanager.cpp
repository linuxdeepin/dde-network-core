// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "httpmanager.h"
#include "constants.h"
#include "settingconfig.h"

#include <QRegularExpression>
#include <QDebug>
#include <QUrl>

#include <mutex>
#include <curl/curl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

using namespace network::service;

namespace {

struct GetAddrInfoParams {
    const char *node;
    const char *service;
    const addrinfo *hints;
    addrinfo *result = nullptr;
    int ret = 0;
};

static void *getaddrinfo_thread(void *arg)
{
    auto *params = static_cast<GetAddrInfoParams *>(arg);
    params->ret = getaddrinfo(params->node, params->service, params->hints, &params->result);
    return nullptr;
}

// 在独立线程中执行 getaddrinfo，主线程等待 timeoutMs 毫秒
// 如果超时返回 ETIMEDOUT，成功返回 0
static int getaddrinfo_with_timeout(const char *node, const char *service,
                                    const addrinfo *hints, addrinfo **result,
                                    int timeoutMs)
{
    GetAddrInfoParams params = {node, service, hints, nullptr, 0};
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (pthread_create(&thread, &attr, getaddrinfo_thread, &params) != 0) {
        pthread_attr_destroy(&attr);
        return EAI_SYSTEM;
    }
    pthread_attr_destroy(&attr);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }

    void *threadRet = nullptr;
    int joinRet = pthread_timedjoin_np(thread, &threadRet, &ts);
    if (joinRet == ETIMEDOUT) {
        // 线程超时，detach 让其自行结束（getaddrinfo 最终会返回）
        pthread_detach(thread);
        return ETIMEDOUT;
    }

    *result = params.result;
    return params.ret;
}

} // anonymous namespace

namespace network {
namespace service {

void HttpManager::init()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

void HttpManager::unInit()
{
    curl_global_cleanup();
}

HttpManager::HttpManager(QObject *parent)
    : QObject (parent)
{
}

static ulong write_data(char *data, size_t size, size_t nmemb, std::string *buffer)
{
    size_t total_size = size * nmemb;
    if (buffer) {
        buffer->append(data, total_size);
    }
    return total_size;
}

// 为底层 socket 设置收发超时，避免内核层面 read/write 长时间阻塞
struct CurlSockoptContext {
    long rwTimeoutSec { 10 };
};

static int sockopt_callback(void *clientp, curl_socket_t sockfd, curlsocktype /*purpose*/)
{
    const CurlSockoptContext *ctx = static_cast<const CurlSockoptContext *>(clientp);
    long timeoutSec = (ctx && ctx->rwTimeoutSec > 0) ? ctx->rwTimeoutSec : 10L;

    timeval tv;
    tv.tv_sec = static_cast<decltype(tv.tv_sec)>(timeoutSec);
    tv.tv_usec = 0;

    // 忽略 setsockopt 失败，继续走 libcurl 自有超时；这里主要作为额外保险
    (void)setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&tv), sizeof(tv));
    (void)setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char *>(&tv), sizeof(tv));

    return CURL_SOCKOPT_OK;
}

HttpReply *HttpManager::get(const QString &url)
{
    HttpReply *reply = new HttpReply(this);
    CURL *curl = curl_easy_init();
    if (!curl) {
        qCWarning(DSM) << "Curl initialization failed for URL" << url;
        reply->setErrorMessage("curl easy init failure");
        return reply;
    }

    // 请求URL
    curl_easy_setopt(curl, CURLOPT_URL, url.toStdString().c_str());
    // 禁用打印日志
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    // 当前请求方式设置为Get
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    // 禁用信号，防止在多线程环境中死锁
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // 设置超时（从配置文件读取）
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(SettingConfig::instance()->httpRequestTimeout()));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, static_cast<long>(SettingConfig::instance()->httpConnectTimeout()));
    // 设置回调函数，当写入头数据的时候，将会调用write_data方法来写入数据
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    std::string body_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body_data);
    // 需要写入header的数据，如果不设置此项，就不会写入header数据，导致无法解析header(类似 curl -Li命令)
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    // 跟随重定向
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // 设置套接字级别的收发超时，防止底层 read/write 长时间阻塞
    CurlSockoptContext sockCtx;
    sockCtx.rwTimeoutSec = static_cast<long>(SettingConfig::instance()->httpRequestTimeout());
    curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
    curl_easy_setopt(curl, CURLOPT_SOCKOPTDATA, &sockCtx);
    // 发送请求
    qCDebug(DSM) << "Send request to" << url;
    CURLcode curlRes = curl_easy_perform(curl);
    qCDebug(DSM) << "Request finished, ret:" << curlRes;
    if (curlRes == CURLE_OK) {
        reply->setHeader(QString::fromStdString(body_data));
    } else {
        if (curlRes == CURLE_OPERATION_TIMEDOUT)
            reply->setTimeout(true);
        const QString &errorMsg = curl_easy_strerror(curlRes);
        qCInfo(DSM) << "Request failed for" << url << ", error message:" << errorMsg;
        reply->setErrorMessage(errorMsg);
    }

    curl_easy_cleanup(curl);
    return reply;
}

HttpReply *HttpManager::get(const QString &url, int timeoutSec)
{
    HttpReply *reply = new HttpReply(this);

    // 解析域名，使用系统 getaddrinfo，手动设置超时
    QUrl qurl(url);
    QString host = qurl.host();
    int port = qurl.port(80);
    QString resolvedIp;

    if (!host.isEmpty()) {
        addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo *result = nullptr;

        // getaddrinfo 本身会调用系统 DNS，如果路由器不通，glibc 会等待很久
        // 用独立线程包装，只等待 timeoutSec 秒
        // 注意：必须用局部变量持有 std::string，确保 getaddrinfo_with_timeout
        // 返回前字符串生命周期有效，否则子线程会使用悬垂指针
        std::string hostStd = host.toStdString();
        int ret = getaddrinfo_with_timeout(hostStd.c_str(),
                                            nullptr,
                                            &hints,
                                            &result,
                                            timeoutSec * 1000);
        if (ret == 0 && result != nullptr) {
            char ipStr[INET6_ADDRSTRLEN];
            if (result->ai_family == AF_INET) {
                sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(result->ai_addr);
                inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr));
            } else {
                sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *>(result->ai_addr);
                inet_ntop(AF_INET6, &addr->sin6_addr, ipStr, sizeof(ipStr));
            }
            resolvedIp = QString::fromLatin1(ipStr);
            freeaddrinfo(result);
        } else if (ret == EAI_AGAIN || ret == EAI_NONAME || ret == ETIMEDOUT || ret == EAI_SYSTEM) {
            // DNS 解析失败（超时或无此域名），直接返回超时
            reply->setTimeout(true);
            reply->setErrorMessage("DNS resolution timeout");
            qCDebug(DSM) << "DNS resolution failed for" << host << "ret:" << ret;
            return reply;
        } else {
            reply->setErrorMessage(QString("DNS resolution failed: %1").arg(gai_strerror(ret)));
            qCInfo(DSM) << "DNS resolution failed for" << host << "error:" << gai_strerror(ret);
            return reply;
        }
    }

    CURL *curl = curl_easy_init();
    if (!curl) {
        qCWarning(DSM) << "Curl initialization failed for URL" << url;
        reply->setErrorMessage("curl easy init failure");
        return reply;
    }

    std::string urlStd = url.toStdString();
    curl_easy_setopt(curl, CURLOPT_URL, urlStd);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // 使用毫秒级超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeoutSec * 1000));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeoutSec * 1000));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    std::string body_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body_data);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CurlSockoptContext sockCtx;
    sockCtx.rwTimeoutSec = timeoutSec;
    curl_easy_setopt(curl, CURLOPT_SOCKOPTFUNCTION, sockopt_callback);
    curl_easy_setopt(curl, CURLOPT_SOCKOPTDATA, &sockCtx);

    // 如果域名解析成功，通过 CURLOPT_RESOLVE 告诉 curl 直接使用该 IP，跳过 DNS 阶段
    curl_slist *resolveList = nullptr;
    if (!resolvedIp.isEmpty()) {
        std::string resolveStr = host.toStdString() + ":" + std::to_string(port) + ":" + resolvedIp.toStdString();
        resolveList = curl_slist_append(nullptr, resolveStr.c_str());
        curl_easy_setopt(curl, CURLOPT_RESOLVE, resolveList);
        qCDebug(DSM) << "DNS resolved" << host << "to" << resolvedIp << ", using CURLOPT_RESOLVE";
    }

    CURLcode curlRes = curl_easy_perform(curl);
    if (curlRes == CURLE_OK) {
        reply->setHeader(QString::fromStdString(body_data));
    } else {
        if (curlRes == CURLE_OPERATION_TIMEDOUT)
            reply->setTimeout(true);
        const QString &errorMsg = curl_easy_strerror(curlRes);
        qCInfo(DSM) << "Request failed for" << url << ", error message:" << errorMsg;
        reply->setErrorMessage(errorMsg);
    }

    if (resolveList)
        curl_slist_free_all(resolveList);
    curl_easy_cleanup(curl);
    return reply;
}

/**
 * @brief HttpReply::HttpReply
 * @param parent
 */
HttpReply::HttpReply(QObject *parent)
    : QObject (parent)
    , m_httpCode(0)
    , m_timeout(false)
{
}

void HttpReply::setHeader(const QString &html)
{
    m_httpCode = 0;
    m_portalUrl.clear();
    QStringList headerLines = html.split("\n");
    for (const QString &line : headerLines) {
        if (line.startsWith("HTTP") && m_httpCode == 0) {
            QStringList httpOut = line.split(' ');
            if (httpOut.size() >= 2) {
                m_httpCode = httpOut.at(1).toInt();
            }
        } else if (line.startsWith("Location: ")) {
            m_portalUrl = line.split(' ').at(1).trimmed();
        } else if ((m_httpCode >= 200 && m_httpCode < 300)) {
            if (line.contains("top.self.location.href")) {
                m_portalUrl = line.trimmed();
                m_portalUrl = m_portalUrl.replace("<script>", "");
                m_portalUrl = m_portalUrl.replace("</script>", "");
                m_portalUrl = m_portalUrl.replace("top.self.location.href=", "");
                m_portalUrl = m_portalUrl.replace("'", "");
            } else if (line.startsWith("<META http-equiv=\"refresh\"")) {
                QString lineData = line.trimmed();
                // 创建正则表达式，提取 content 的值
                QRegularExpression regex(R"(content=\"([^\"]*)\")");
                // 执行匹配
                QRegularExpressionMatch match = regex.match(lineData);
                if (!match.hasMatch())
                    continue;

                // 提取匹配到的内容
                QString captureContent = match.captured(1);
                if (captureContent.contains(";")) {
                    QStringList contents = captureContent.split(";");
                    captureContent = contents.last();
                }
                captureContent = captureContent.trimmed();
                if (captureContent.toLower().startsWith("url="))
                    captureContent = captureContent.mid(QString("url=").length());

                m_portalUrl = captureContent;
            }
        }
    }
}

QString HttpReply::errorMessage() const
{
    return m_errorMessage;
}

int HttpReply::httpCode() const
{
    return m_httpCode;
}

QString HttpReply::portal() const
{
    return m_portalUrl;
}

void HttpReply::setErrorMessage(const QString &errorMessage)
{
    m_errorMessage = errorMessage;
}

void HttpReply::setTimeout(bool timeout)
{
    m_timeout = timeout;
}

bool HttpReply::isTimeout() const
{
    return m_timeout;
}

}
}
