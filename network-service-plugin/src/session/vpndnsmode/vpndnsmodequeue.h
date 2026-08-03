// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSMODEQUEUE_H
#define VPNDNSMODEQUEUE_H

#include <QHostAddress>
#include <QMap>
#include <QQueue>

namespace network {
namespace sessionservice {

// DNS Mode 请求的描述，以 Profile 路径与 ActiveConnection 路径共同标识。
struct VpnDnsModeRequest
{
    QString connectionPath;
    QString activeConnectionPath;
    quint64 generation = 0;
    int ifindex = 0;
    QList<QHostAddress> dnsServers;
    int dnsPriority = 0;
};

// DNS Mode 请求队列：按 Profile 合并旧 pending 请求、拒绝过期 generation，
// 并按 Profile 或 ActiveConnection 精确删除请求。
class VpnDnsModeRequestQueue
{
public:
    bool enqueue(const VpnDnsModeRequest &request)
    {
        const auto latestIt = m_latestGenerations.constFind(request.connectionPath);
        if (latestIt != m_latestGenerations.cend() && request.generation <= latestIt.value())
            return false;

        for (auto it = m_requests.begin(); it != m_requests.end();) {
            if (it->connectionPath == request.connectionPath)
                it = m_requests.erase(it);
            else
                ++it;
        }

        m_latestGenerations[request.connectionPath] = request.generation;
        m_requests.enqueue(request);
        return true;
    }

    bool takeNext(VpnDnsModeRequest *request)
    {
        if (!request || m_requests.isEmpty())
            return false;

        *request = m_requests.dequeue();
        return true;
    }

    void removeConnection(const QString &connectionPath)
    {
        QQueue<VpnDnsModeRequest> retainedRequests;
        while (!m_requests.isEmpty()) {
            const VpnDnsModeRequest request = m_requests.dequeue();
            if (request.connectionPath != connectionPath)
                retainedRequests.enqueue(request);
        }
        m_requests.swap(retainedRequests);
        m_latestGenerations.remove(connectionPath);
    }

    void removeActiveConnection(const QString &activeConnectionPath)
    {
        QQueue<VpnDnsModeRequest> retainedRequests;
        while (!m_requests.isEmpty()) {
            const VpnDnsModeRequest request = m_requests.dequeue();
            if (request.activeConnectionPath != activeConnectionPath)
                retainedRequests.enqueue(request);
        }
        m_requests.swap(retainedRequests);
    }

    void clear()
    {
        m_requests.clear();
        m_latestGenerations.clear();
    }

private:
    QQueue<VpnDnsModeRequest> m_requests;
    QMap<QString, quint64> m_latestGenerations;
};

} // namespace sessionservice
} // namespace network

#endif // VPNDNSMODEQUEUE_H
