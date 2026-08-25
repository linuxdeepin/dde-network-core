// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "vpnparameterschecker.h"

#include "networkconst.h"

#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/VpnSetting>

#include <QHostAddress>

using namespace NetworkManager;

namespace dde {
namespace network {
#define ServiceTypeL2TP "org.freedesktop.NetworkManager.l2tp"
#define ServiceTypePPTP "org.freedesktop.NetworkManager.pptp"
#define ServiceTypeVPNC "org.freedesktop.NetworkManager.vpnc"
#define ServiceTypeOpenVPN "org.freedesktop.NetworkManager.openvpn"
#define ServiceTypeStrongSwan "org.freedesktop.NetworkManager.strongswan"
#define ServiceTypeOpenConnect "org.freedesktop.NetworkManager.openconnect"
#define ServiceTypeSSTP "org.freedesktop.NetworkManager.sstp"

// 参数校验
VPNParametersChecker::VPNParametersChecker(const VpnSetting *vpnSetting)
    : m_vpnSetting(vpnSetting)
{
}

VPNParametersChecker::~VPNParametersChecker() { }

bool VPNParametersChecker::isValid() const
{
    return true;
}

VPNParametersChecker *VPNParametersChecker::createVpnChecker(NetworkManager::Connection *connection)
{
    if (connection->settings()->connectionType() != NetworkManager::ConnectionSettings::ConnectionType::Vpn)
        return new DefaultChecker;

    if (connection->settings()->id().isEmpty() || connection->settings()->name().isEmpty())
        return new DefaultChecker;

    QDBusPendingReply<NMVariantMapMap> reply = connection->secrets(connection->settings()->setting(NetworkManager::Setting::SettingType::Vpn)->name());
    reply.waitForFinished();
    NetworkManager::VpnSetting::Ptr setting = connection->settings()->setting(NetworkManager::Setting::SettingType::Vpn).staticCast<NetworkManager::VpnSetting>();
    setting->secretsFromMap(reply.value().value(setting->name()));
    NetworkManager::VpnSetting *vpnSetting = connection->settings()->setting(NetworkManager::Setting::SettingType::Vpn).staticCast<NetworkManager::VpnSetting>().data();
    NMStringMap vpnData = vpnSetting->data();
    QString serviceType = vpnSetting->serviceType();
    if (serviceType == ServiceTypeL2TP) {
        return new L2TPChecker(vpnSetting);
    }
    if (serviceType == ServiceTypePPTP) {
        return new PPTPChecker(vpnSetting);
    }
    if (serviceType == ServiceTypeVPNC) {
        return new VPNCChecker(vpnSetting);
    }
    if (serviceType == ServiceTypeOpenVPN) {
        return new OpenVPNChecker(vpnSetting);
    }
    if (serviceType == ServiceTypeStrongSwan) {
        return new StrongSwanChecker(vpnSetting);
    }
    if (serviceType == ServiceTypeOpenConnect) {
        return new OpenConnectChecker(vpnSetting);
    }
    if (serviceType == ServiceTypeSSTP) {
        return new SSTPChecker(vpnSetting);
    }

    return new DefaultChecker;
}

NMStringMap VPNParametersChecker::data() const
{
    return m_vpnSetting->data();
}

const VpnSetting *VPNParametersChecker::setting() const
{
    return m_vpnSetting;
}

L2TPChecker::L2TPChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool L2TPChecker::isValid() const
{
    NMStringMap vpnData = data();
    // L2TP中如果网关为空，
    const QString gateWay = vpnData.value("gateway");
    if (gateWay.isEmpty())
        return false;

    // 不支持IPV6
    if (QHostAddress(gateWay).protocol() == QAbstractSocket::IPv6Protocol)
        return false;

    // 用户名不能为空
    if (vpnData.value("user").isEmpty())
        return false;
    // 判断密码选项(如果密码为已保存的并且密码为空)
    QString passwordFlags = vpnData.value("password-flags");
    if ((passwordFlags == "0" || passwordFlags.isEmpty()) && setting()->secrets().value("password").isEmpty())
        return false;

    return true;
}

PPTPChecker::PPTPChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool PPTPChecker::isValid() const
{
    NMStringMap vpnData = data();
    // L2TP中如果网关为空，
    const QString gateWay = vpnData.value("gateway");
    if (gateWay.isEmpty())
        return false;

    // 不支持IPV6
    if (QHostAddress(gateWay).protocol() == QAbstractSocket::IPv6Protocol)
        return false;

    // 用户名不能为空
    if (vpnData.value("user").isEmpty())
        return false;
    // 判断密码选项(如果密码为已保存的并且密码为空)
    QString passwordFlags = vpnData.value("password-flags");
    if ((passwordFlags == "0" || passwordFlags.isEmpty()) && setting()->secrets().value("password").isEmpty())
        return false;

    return true;
}

VPNCChecker::VPNCChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool VPNCChecker::isValid() const
{
    NMStringMap dataMap = data();
    NMStringMap vpnSettingSecrets = setting()->secrets();
    qCInfo(DNC) << "Check VPNC validity, data map: " << dataMap << ", VPN setting secrets: " << vpnSettingSecrets;
    // 网关不能为空
    if (dataMap.value("IPSec gateway").isEmpty())
        return false;

    // 用户名不能为空
    if (dataMap.value("Xauth username").isEmpty())
        return false;

    // 密码选项如果是已保存的，密码不能为空
    QString passwordFlags = dataMap.value("Xauth password-flags");
    if ((passwordFlags.isEmpty() || passwordFlags == "0") && vpnSettingSecrets.value("Xauth password").isEmpty())
        return false;

    // 组名不能为空
    if (dataMap.value("IPSec ID").isEmpty())
        return false;

    passwordFlags = dataMap.value("IPSec secret-flags");
    if ((passwordFlags.isEmpty() || passwordFlags == "0") && (vpnSettingSecrets.value("IPSec secret").isEmpty()))
        return false;

    return true;
}

OpenVPNChecker::OpenVPNChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool OpenVPNChecker::isValid() const
{
    NMStringMap vpnData = data();
    qCInfo(DNC) << "Check openVPN validity, vpn data: " << vpnData << ", setting secrets: " << setting()->secrets();
    if (vpnData.value("remote").isEmpty())
        return false;

    const QString connectionType = vpnData.value("connection-type");
    if (connectionType == "tls") {
        return tlsIsValid();
    }
    if (connectionType == "password") {
        return passIsValid();
    }
    if (connectionType == "password-tls") {
        return passTlsValid();
    }
    if (connectionType == "static-key") {
        return staticKeyIsValid();
    }

    return true;
}

bool OpenVPNChecker::tlsIsValid() const
{
    NMStringMap vpnData = data();
    // 如果是tls的认证方式，则需要判断CA证书
    if (vpnData.value("ca").isEmpty())
        return false;
    // 用户证书
    if (vpnData.value("cert").isEmpty())
        return false;
    // 私钥
    if (vpnData.value("key").isEmpty())
        return false;
    // 密码选项为已保存并且密码为空
    QString certPassFlags = vpnData.value("cert-pass-flags");
    const VpnSetting *vpnSetting = setting();
    if ((certPassFlags.isEmpty() || certPassFlags == "0" || certPassFlags == "1") && vpnSetting && vpnSetting->secrets().value("cert-pass").isEmpty())
        return false;

    return true;
}

bool OpenVPNChecker::passIsValid() const
{
    NMStringMap vpnData = data();
    // 如果是tls的认证方式，则需要判断CA证书
    if (vpnData.value("ca").isEmpty())
        return false;
    // 用户名
    if (vpnData.value("username").isEmpty())
        return false;
    // 密码选项为已保存并且密码为空
    QString passFlags = vpnData.value("password-flags");
    const VpnSetting *vpnSetting = setting();
    if ((passFlags.isEmpty() || passFlags == "0" || passFlags == "1") && vpnSetting && vpnSetting->secrets().value("password").isEmpty())
        return false;

    return true;
}

bool OpenVPNChecker::passTlsValid() const
{
    NMStringMap vpnData = data();
    // 如果是tls的认证方式，则需要判断CA证书
    if (vpnData.value("ca").isEmpty())
        return false;
    // 用户名
    if (vpnData.value("username").isEmpty())
        return false;
    // 密码选项为已保存并且密码为空
    QString passFlags = vpnData.value("password-flags");
    const VpnSetting *vpnSetting = setting();
    if ((passFlags.isEmpty() || passFlags == "0" || passFlags == "1") && vpnSetting && vpnSetting->secrets().value("password").isEmpty())
        return false;

    // 用户证书
    if (vpnData.value("cert").isEmpty())
        return false;
    // 私钥
    if (vpnData.value("key").isEmpty())
        return false;
    // 密码选项为已保存并且密码为空
    QString certPassFlags = vpnData.value("cert-pass-flags");
    if ((certPassFlags.isEmpty() || certPassFlags == "0" || certPassFlags == "1") && vpnSetting && vpnSetting->secrets().value("cert-pass").isEmpty())
        return false;

    return true;
}

bool OpenVPNChecker::staticKeyIsValid() const
{
    NMStringMap vpnData = data();
    if (vpnData.value("static-key").isEmpty())
        return false;

    if (vpnData.value("remote-ip").isEmpty())
        return false;

    if (vpnData.value("local-ip").isEmpty())
        return false;

    return true;
}

StrongSwanChecker::StrongSwanChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool StrongSwanChecker::isValid() const
{
    // StrongSwan的vpn只需检测网关是否为空即可
    return !data().value("address").isEmpty();
}

OpenConnectChecker::OpenConnectChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool OpenConnectChecker::isValid() const
{
    NMStringMap dataMap = data();
    // 判断网关是否为空
    if (dataMap.value("gateway").isEmpty())
        return false;

    // 判断用户证书是否为空
    if (dataMap.value("usercert").isEmpty())
        return false;

    // 判断私钥是否为空
    if (dataMap.value("userkey").isEmpty())
        return false;

    return true;
}

SSTPChecker::SSTPChecker(const VpnSetting *vpnSetting)
    : VPNParametersChecker(vpnSetting)
{
}

bool SSTPChecker::isValid() const
{
    return true;
}

DefaultChecker::DefaultChecker(bool defaultValue)
    : VPNParametersChecker(nullptr)
    , m_defaultValue(defaultValue)
{
}

bool DefaultChecker::isValid() const
{
    return m_defaultValue;
}

} // namespace network
} // namespace dde
