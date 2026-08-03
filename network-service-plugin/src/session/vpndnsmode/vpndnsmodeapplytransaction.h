// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VPNDNSMODEAPPLYTRANSACTION_H
#define VPNDNSMODEAPPLYTRANSACTION_H

namespace network {
namespace sessionservice {

enum class VpnDnsApplyResult
{
    Success,
    AuthorizationDenied,
    AuthorizationError,
    ServiceUnavailable,
    DbusError
};

// 一次应用链中的执行阶段（DNS、Domains、DefaultRoute 及整体 RevertLink 回退）。
enum class VpnDnsApplyStage
{
    StageIdle,
    StageSetLinkDns,
    StageSetLinkDomains,
    StageSetLinkDefaultRoute,
    StageRevertLink
};

// 应用事务状态机：记录本次已写入的阶段，在中间阶段失败时触发整链 RevertLink 回退，
// 并始终向上层保留原始的失败结果。
class VpnDnsModeApplyTransaction
{
public:
    bool beginRollback(VpnDnsApplyStage failedStage, VpnDnsApplyResult result)
    {
        if (m_rollingBack)
            return false;

        switch (failedStage) {
        case VpnDnsApplyStage::StageSetLinkDns:
        case VpnDnsApplyStage::StageSetLinkDomains:
        case VpnDnsApplyStage::StageSetLinkDefaultRoute:
            m_rollingBack = true;
            m_originalResult = result;
            return true;
        case VpnDnsApplyStage::StageIdle:
        case VpnDnsApplyStage::StageRevertLink:
            return false;
        }
        return false;
    }

    VpnDnsApplyResult complete(VpnDnsApplyResult directResult)
    {
        const VpnDnsApplyResult result = m_rollingBack ? m_originalResult : directResult;
        reset();
        return result;
    }

    void reset()
    {
        m_rollingBack = false;
        m_originalResult = VpnDnsApplyResult::DbusError;
    }

    bool isRollingBack() const
    {
        return m_rollingBack;
    }

private:
    VpnDnsApplyResult m_originalResult = VpnDnsApplyResult::DbusError;
    bool m_rollingBack = false;
};

} // namespace sessionservice
} // namespace network

#endif // VPNDNSMODEAPPLYTRANSACTION_H
