// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "connectionwirelesseditpage.h"
#include "settings/wirelesssettings.h"

#include <networkmanagerqt/wirelessdevice.h>
#include <networkmanagerqt/wirelesssetting.h>
#include <networkmanagerqt/wirelesssecuritysetting.h>
#include <networkmanagerqt/settings.h>
#include <networkmanagerqt/connectionsettings.h>

#include <QDebug>
#include <QVBoxLayout>

using namespace dcc::widgets;
using namespace NetworkManager;

ConnectionWirelessEditPage::ConnectionWirelessEditPage(const QString &devPath, const QString &connUuid, const QString &apPath, bool isHidden, QWidget *parent)
    : ConnectionEditPage(ConnectionEditPage::ConnectionType::WirelessConnection, devPath, connUuid, parent)
{
    m_tempParameter.reset(new ParametersContainer);
    m_tempParameter->saveParameters(ParametersContainer::ParamType::AccessPath, apPath);
    m_tempParameter->saveParameters(ParametersContainer::ParamType::isHidden, isHidden);
    if (isHidden)
        m_connectionSettings->setting(Setting::SettingType::Wireless).staticCast<WirelessSetting>()->setHidden(isHidden);
}

ConnectionWirelessEditPage::~ConnectionWirelessEditPage()
{
}

void ConnectionWirelessEditPage::initSettingsWidgetFromAp()
{
    const QString &apPath = m_tempParameter->getValue(ParametersContainer::ParamType::AccessPath).toString();
    AccessPoint::Ptr nmAp = QSharedPointer<AccessPoint>(new AccessPoint(apPath));

    if (!m_connectionSettings || !nmAp)
        return;

    // init connection setting from data of given ap
    m_connectionSettings->setId(nmAp->ssid());

    initApSecretType(nmAp);

    m_connectionSettings->setting(Setting::SettingType::Wireless).staticCast<WirelessSetting>()->setSsid(nmAp->rawSsid());

    m_settingsWidget = new WirelessSettings(m_connectionSettings, m_tempParameter, this);

    connect(m_settingsWidget, &WirelessSettings::requestNextPage, this, &ConnectionWirelessEditPage::onRequestNextPage);
    connect(m_settingsWidget, &AbstractSettings::requestFrameAutoHide, this, &ConnectionEditPage::requestFrameAutoHide);
    connect(m_settingsWidget, &WirelessSettings::anyEditClicked, this, [ this ] {
        setButtonTupleEnable(true);
    });

    m_settingsLayout->addWidget(m_settingsWidget);
}

void ConnectionWirelessEditPage::initApSecretType(AccessPoint::Ptr nmAp)
{
    WirelessSecuritySetting::Ptr wirelessSecuritySetting;
    wirelessSecuritySetting = m_connectionSettings->setting(Setting::SettingType::WirelessSecurity).staticCast<WirelessSecuritySetting>();

    AccessPoint::Capabilities capabilities = nmAp->capabilities();
    AccessPoint::WpaFlags wpaFlags = nmAp->wpaFlags();
    AccessPoint::WpaFlags rsnFlags = nmAp->rsnFlags();

    WirelessSecuritySetting::KeyMgmt keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaNone;

    if (capabilities.testFlag(AccessPoint::Capability::Privacy) &&
            !wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk) &&
            !wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x)) {
        keyMgmt = WirelessSecuritySetting::KeyMgmt::Wep;
    }

    if (wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk) ||
            rsnFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk))
        keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaPsk;

#ifdef USE_DEEPIN_NMQT
    NetworkManager::AccessPoint::WpaFlag accessPointWpaFlagState = NetworkManager::AccessPoint::WpaFlag::keyMgmtSae;
#else
    NetworkManager::AccessPoint::WpaFlag accessPointWpaFlagState = NetworkManager::AccessPoint::WpaFlag::KeyMgmtSAE;
#endif

    // 判断是否是wpa3加密的，因为wpa3加密方式，实际上是wpa2的扩展，所以其中会包含KeyMgmtPsk枚举值
    if (wpaFlags.testFlag(accessPointWpaFlagState) ||
        rsnFlags.testFlag(accessPointWpaFlagState)) {
#ifdef USE_DEEPIN_NMQT
        keyMgmt = NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae;
#else
        keyMgmt = NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE;
#endif
    }

    if (wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x) ||
            rsnFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x)) {
        keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaEap;
    }

    wirelessSecuritySetting->setKeyMgmt(keyMgmt);
}
