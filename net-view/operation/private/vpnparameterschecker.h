// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef VPNPARAMETERSCHECKER_H
#define VPNPARAMETERSCHECKER_H

#include <networkmanagerqt/generictypes.h>

namespace NetworkManager {
class VpnSetting;
class Connection;
} // namespace NetworkManager

namespace dde {
namespace network {
class VPNParametersChecker
{
public:
    explicit VPNParametersChecker(const NetworkManager::VpnSetting *vpnSetting);
    virtual ~VPNParametersChecker();

    virtual bool isValid() const;

    static VPNParametersChecker *createVpnChecker(NetworkManager::Connection *connection);

protected:
    NMStringMap data() const;
    const NetworkManager::VpnSetting *setting() const;

private:
    const NetworkManager::VpnSetting *m_vpnSetting;
};

class L2TPChecker : public VPNParametersChecker
{
public:
    explicit L2TPChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class PPTPChecker : public VPNParametersChecker
{
public:
    explicit PPTPChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class VPNCChecker : public VPNParametersChecker
{
public:
    explicit VPNCChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class OpenVPNChecker : public VPNParametersChecker
{
public:
    explicit OpenVPNChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;

private:
    bool tlsIsValid() const;
    bool passIsValid() const;
    bool passTlsValid() const;
    bool staticKeyIsValid() const;
};

class StrongSwanChecker : public VPNParametersChecker
{
public:
    explicit StrongSwanChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class OpenConnectChecker : public VPNParametersChecker
{
public:
    explicit OpenConnectChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class SSTPChecker : public VPNParametersChecker
{
public:
    explicit SSTPChecker(const NetworkManager::VpnSetting *vpnSetting);
    bool isValid() const override;
};

class DefaultChecker : public VPNParametersChecker
{
public:
    explicit DefaultChecker(bool defaultValue = false);
    bool isValid() const override;

private:
    bool m_defaultValue;
};
} // namespace network
} // namespace dde

#endif // VPNPARAMETERSCHECKER_H
