// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnl2tpsettings.h"
#include "../../sections/genericsection.h"
#include "../../sections/vpn/vpnsection.h"
#include "../../sections/vpn/vpnpppsection.h"
#include "../../sections/vpn/vpnipsecsection.h"
#include "../../sections/ipvxsection.h"
#include "../../sections/dnssection.h"

#include <QVBoxLayout>

#include <widgets/contentwidget.h>

using namespace NetworkManager;

VpnL2tpSettings::VpnL2tpSettings(ConnectionSettings::Ptr connSettings, QWidget *parent)
    : AbstractSettings(connSettings, parent)
{
    setAccessibleName("VpnL2tpSettings");
    initSections();
}

VpnL2tpSettings::~VpnL2tpSettings()
{
}

void VpnL2tpSettings::initSections()
{
    VpnSetting::Ptr vpnSetting = m_connSettings->setting(Setting::SettingType::Vpn).staticCast<VpnSetting>();

    if (!vpnSetting)
        return;

    GenericSection *genericSection = new GenericSection(m_connSettings);
    genericSection->setConnectionType(ConnectionSettings::Vpn);

    VpnSection *vpnSection = new VpnSection(vpnSetting);

    VpnPPPSection *vpnPPPSection = new VpnPPPSection(vpnSetting);
    QStringList supportOptions = {
        "refuse-eap", "refuse-pap", "refuse-chap", "refuse-mschap", "refuse-mschapv2",
        "nobsdcomp", "nodeflate", "no-vj-comp", "nopcomp", "noaccomp", "lcp-echo-interval"
    };

    vpnPPPSection->setSupportOptions(supportOptions);

    VpnIpsecSection *vpnIpsecSection = new VpnIpsecSection(vpnSetting);

    IpvxSection *ipv4Section = new IpvxSection(m_connSettings->setting(Setting::SettingType::Ipv4).staticCast<Ipv4Setting>());
    ipv4Section->setIpv4ConfigMethodEnable(Ipv4Setting::ConfigMethod::Manual, false);
    ipv4Section->setNeverDefaultEnable(true);
    DNSSection *dnsSection = new DNSSection(m_connSettings, false);

    connect(genericSection, &GenericSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);
    connect(vpnSection, &VpnSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);
    connect(vpnPPPSection, &VpnPPPSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);
    connect(vpnIpsecSection, &VpnIpsecSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);
    connect(ipv4Section, &IpvxSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);
    connect(dnsSection, &DNSSection::editClicked, this, &VpnL2tpSettings::anyEditClicked);

    connect(vpnSection, &VpnSection::requestNextPage, this, &VpnL2tpSettings::requestNextPage);
    connect(vpnPPPSection, &VpnPPPSection::requestNextPage, this, &VpnL2tpSettings::requestNextPage);
    connect(vpnIpsecSection, &VpnIpsecSection::requestNextPage, this, &VpnL2tpSettings::requestNextPage);
    connect(ipv4Section, &IpvxSection::requestNextPage, this, &VpnL2tpSettings::requestNextPage);
    connect(dnsSection, &DNSSection::requestNextPage, this, &VpnL2tpSettings::requestNextPage);

    connect(vpnSection, &VpnSection::requestFrameAutoHide, this, &VpnL2tpSettings::requestFrameAutoHide);
    connect(vpnPPPSection, &VpnPPPSection::requestFrameAutoHide, this, &VpnL2tpSettings::requestFrameAutoHide);
    connect(vpnIpsecSection, &VpnIpsecSection::requestFrameAutoHide, this, &VpnL2tpSettings::requestFrameAutoHide);
    connect(ipv4Section, &IpvxSection::requestFrameAutoHide, this, &VpnL2tpSettings::requestFrameAutoHide);
    connect(dnsSection, &DNSSection::requestFrameAutoHide, this, &VpnL2tpSettings::requestFrameAutoHide);

    m_sectionsLayout->addWidget(genericSection);
    m_sectionsLayout->addWidget(vpnSection);
    m_sectionsLayout->addWidget(vpnPPPSection);
    m_sectionsLayout->addWidget(vpnIpsecSection);
    m_sectionsLayout->addWidget(ipv4Section);
    m_sectionsLayout->addWidget(dnsSection);

    m_settingSections.append(genericSection);
    m_settingSections.append(vpnSection);
    m_settingSections.append(vpnPPPSection);
    m_settingSections.append(vpnIpsecSection);
    m_settingSections.append(ipv4Section);
    m_settingSections.append(dnsSection);
}
