// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSMODEAPPLYSTATE_H
#define VPNDNSMODEAPPLYSTATE_H

#include <QtGlobal>

namespace network {
namespace sessionservice {

// 记录某个 VPN Profile 的 DNS Mode 应用状态：请求版本号、已应用的 dns-priority，
// 以及失败回滚后的抑制标记，用于过滤过期的异步结果并控制重试。
class VpnDnsModeApplyState
{
public:
    quint64 beginRequest()
    {
        m_rolledBack = false;
        return ++m_generation;
    }

    bool commitApplied(quint64 generation, int priority)
    {
        if (generation != m_generation)
            return false;

        m_appliedPriority = priority;
        m_hasAppliedPriority = true;
        m_rolledBack = false;
        return true;
    }

    bool rollbackRequest(quint64 generation, int target)
    {
        if (generation != m_generation)
            return false;

        m_appliedPriority = target;
        m_hasAppliedPriority = true;
        m_rolledBack = true;
        return true;
    }

    void clearRolledBack()
    {
        m_rolledBack = false;
    }

    bool isRolledBack() const
    {
        return m_rolledBack;
    }

    bool hasAppliedPriority() const
    {
        return m_hasAppliedPriority;
    }

    int appliedPriority() const
    {
        return m_appliedPriority;
    }

private:
    quint64 m_generation = 0;
    int m_appliedPriority = 0;
    bool m_hasAppliedPriority = false;
    bool m_rolledBack = false;
};

} // namespace sessionservice
} // namespace network

#endif // VPNDNSMODEAPPLYSTATE_H
