// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONNECTIVITYPROBER_H
#define CONNECTIVITYPROBER_H

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

namespace network {
namespace systemservice {

/// 一次连通性探测的输入。纯值对象，可跨线程传递。
struct ConnectivityProbeRequest
{
    QStringList urls;
    int timeoutSec = 0;
    quint64 generation = 0;
};

/// 一次连通性探测的输出。纯值对象，可跨线程传递。
struct ConnectivityProbeResult
{
    quint64 generation = 0;
    bool networkIsOk = false;   // 拿到过任一非 0 HTTP 码
    bool timedOut = false;      // 首个 URL 读超时
    int httpCode = 0;
    QString portalUrl;
};

/**
 * @brief HTTP 连通性探测执行体。
 *
 * HttpManager 是同步阻塞调用，放在 NM 线程上会堵死 NetworkManagerQt 的整个
 * 事件循环，因此单独放在探测线程执行。本类不触碰 NetworkManagerQt。
 */
class ConnectivityProber : public QObject
{
    Q_OBJECT
public:
    explicit ConnectivityProber(QObject *parent = nullptr);
    ~ConnectivityProber() override = default;

public Q_SLOTS:
    void probe(const ConnectivityProbeRequest &request);

Q_SIGNALS:
    void finished(const ConnectivityProbeResult &result);
};

} // namespace systemservice
} // namespace network

Q_DECLARE_METATYPE(network::systemservice::ConnectivityProbeRequest)
Q_DECLARE_METATYPE(network::systemservice::ConnectivityProbeResult)

#endif // CONNECTIVITYPROBER_H
