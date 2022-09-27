// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnsstpsettings.h"
#include "../../sections/genericsection.h"
#include "../../sections/vpn/vpnsstpsection.h"
#include "../../sections/vpn/vpnpppsection.h"
#include "../../sections/vpn/vpnipsecsection.h"
#include "../../sections/vpn/vpnsstpproxysection.h"
#include "../../sections/ipvxsection.h"
#include "../../sections/dnssection.h"

#include <QVBoxLayout>

#include <widgets/contentwidget.h>

using namespace NetworkManager;

VpnSSTPSettings::VpnSSTPSettings(ConnectionSettings::Ptr connSettings, QWidget *parent)
    : AbstractSettings(connSettings, parent)
{
    initSections();
}

VpnSSTPSettings::~VpnSSTPSettings()
{
}

void VpnSSTPSettings::initSections()
{
    VpnSetting::Ptr vpnSetting = m_connSettings->setting(Setting::SettingType::Vpn).staticCast<VpnSetting>();

    if (!vpnSetting)
        return;

    GenericSection *genericSection = new GenericSection(m_connSettings);
    VpnSSTPSection *vpnSection = new VpnSSTPSection(vpnSetting);
    VpnPPPSection *vpnPPPSection = new VpnPPPSection(vpnSetting);
    QStringList supportOptions = {
        "refuse-eap", "refuse-pap", "refuse-chap", "refuse-mschap", "refuse-mschapv2",
        "nobsdcomp", "nodeflate", "no-vj-comp", "lcp-echo-interval"
    };

    vpnPPPSection->setSupportOptions(supportOptions);
    VpnSstpProxySection *vpnProxySection = new VpnSstpProxySection(vpnSetting);
    IpvxSection *ipv4Section = new IpvxSection(m_connSettings->setting(Setting::SettingType::Ipv4).staticCast<Ipv4Setting>());
    ipv4Section->setIpv4ConfigMethodEnable(Ipv4Setting::ConfigMethod::Manual, false);
    ipv4Section->setNeverDefaultEnable(true);
    DNSSection *dnsSection = new DNSSection(m_connSettings, false);

    connect(vpnSection, &VpnSSTPSection::requestNextPage, this, &VpnSSTPSettings::requestNextPage);
    connect(vpnPPPSection, &VpnPPPSection::requestNextPage, this, &VpnSSTPSettings::requestNextPage);
    connect(ipv4Section, &IpvxSection::requestNextPage, this, &VpnSSTPSettings::requestNextPage);
    connect(dnsSection, &DNSSection::requestNextPage, this, &VpnSSTPSettings::requestNextPage);

    connect(vpnSection, &VpnSSTPSection::requestFrameAutoHide, this, &VpnSSTPSettings::requestFrameAutoHide);
    connect(vpnPPPSection, &VpnPPPSection::requestFrameAutoHide, this, &VpnSSTPSettings::requestFrameAutoHide);
    connect(ipv4Section, &IpvxSection::requestFrameAutoHide, this, &VpnSSTPSettings::requestFrameAutoHide);
    connect(dnsSection, &DNSSection::requestFrameAutoHide, this, &VpnSSTPSettings::requestFrameAutoHide);

    m_sectionsLayout->addWidget(genericSection);
    m_sectionsLayout->addWidget(vpnSection);
    m_sectionsLayout->addWidget(vpnPPPSection);
    m_sectionsLayout->addWidget(vpnProxySection);
    m_sectionsLayout->addWidget(ipv4Section);
    m_sectionsLayout->addWidget(dnsSection);

    m_settingSections.append(genericSection);
    m_settingSections.append(vpnSection);
    m_settingSections.append(vpnPPPSection);
    m_settingSections.append(vpnProxySection);
    m_settingSections.append(ipv4Section);
    m_settingSections.append(dnsSection);
}
