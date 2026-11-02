// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "connectivityprober.h"

#include "constants.h"
#include "httpmanager.h"

#include <QDebug>

#include <mutex>

namespace network {
namespace systemservice {

namespace {
// libcurl 的全局初始化必须在使用前完成一次；探测线程是进程内唯一使用 curl 的地方。
void ensureCurlInitialized()
{
    static std::once_flag once;
    std::call_once(once, [] { network::service::HttpManager::init(); });
}
} // namespace
ConnectivityProber::ConnectivityProber(QObject *parent)
    : QObject(parent)
{
}

void ConnectivityProber::probe(const ConnectivityProbeRequest &request)
{
    ensureCurlInitialized();

    ConnectivityProbeResult result;
    // 必须回填代次，否则 StatusChecker 会把每一份结果都当成陈旧结果丢弃
    result.generation = request.generation;

    for (const QString &url : request.urls) {
        network::service::HttpManager http;
        network::service::HttpReply *httpReply = http.get(url, request.timeoutSec);
        if (httpReply->isTimeout()) {
            // 读取超时后无需再试下一个 URL，一般超时即代表网络不通
            qCWarning(DSM()) << "request network timeout";
            result.timedOut = true;
            break;
        }

        const int httpCode = httpReply->httpCode();
        if (httpCode == 0) {
            qCWarning(DSM()) << "Nework is unreachabel:" << url << httpReply->errorMessage();
            continue;
        }

        result.networkIsOk = true;
        result.httpCode = httpCode;
        result.portalUrl = httpReply->portal();
        qCDebug(DSM()) << "Http reply code:" << httpCode << ", portal url:" << result.portalUrl;
        break;
    }

    Q_EMIT finished(result);
}

} // namespace systemservice} // namespace network