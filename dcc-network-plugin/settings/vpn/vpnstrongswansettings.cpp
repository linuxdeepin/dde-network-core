// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnstrongswansettings.h"
#include "../../sections/genericsection.h"
#include "../../sections/vpn/vpnstrongswansection.h"
#include "../../sections/ipvxsection.h"
#include "../../sections/dnssection.h"

#include <widgets/contentwidget.h>

#include <QVBoxLayout>

using namespace NetworkManager;

VpnStrongSwanSettings::VpnStrongSwanSettings(ConnectionSettings::Ptr connSettings, QWidget *parent)
    : AbstractSettings(connSettings, parent)
{
    initSections();
}

VpnStrongSwanSettings::~VpnStrongSwanSettings()
{
}

void VpnStrongSwanSettings::initSections()
{
    VpnSetting::Ptr vpnSetting = m_connSettings->setting(Setting::SettingType::Vpn).staticCast<VpnSetting>();

    if (!vpnSetting)
        return;

    GenericSection *genericSection = new GenericSection(m_connSettings);
    genericSection->setConnectionType(ConnectionSettings::Vpn);

    VpnStrongSwanSection *vpnStrongSwanSection = new VpnStrongSwanSection(vpnSetting);

    IpvxSection *ipv4Section = new IpvxSection(m_connSettings->setting(Setting::SettingType::Ipv4).staticCast<Ipv4Setting>());
    ipv4Section->setIpv4ConfigMethodEnable(Ipv4Setting::ConfigMethod::Manual, false);
    ipv4Section->setNeverDefaultEnable(true);
    DNSSection *dnsSection = new DNSSection(m_connSettings, false);

    connect(genericSection, &GenericSection::editClicked, this, &VpnStrongSwanSettings::anyEditClicked);
    connect(vpnStrongSwanSection, &VpnStrongSwanSection::editClicked, this, &VpnStrongSwanSettings::anyEditClicked);
    connect(ipv4Section, &IpvxSection::editClicked, this, &VpnStrongSwanSettings::anyEditClicked);
    connect(dnsSection, &DNSSection::editClicked, this, &VpnStrongSwanSettings::anyEditClicked);

    connect(vpnStrongSwanSection, &VpnStrongSwanSection::requestNextPage, this, &VpnStrongSwanSettings::requestNextPage);
    connect(ipv4Section, &IpvxSection::requestNextPage, this, &VpnStrongSwanSettings::requestNextPage);
    connect(dnsSection, &DNSSection::requestNextPage, this, &VpnStrongSwanSettings::requestNextPage);

    connect(vpnStrongSwanSection, &VpnStrongSwanSection::requestFrameAutoHide, this, &VpnStrongSwanSettings::requestFrameAutoHide);
    connect(ipv4Section, &IpvxSection::requestFrameAutoHide, this, &VpnStrongSwanSettings::requestFrameAutoHide);
    connect(dnsSection, &DNSSection::requestFrameAutoHide, this, &VpnStrongSwanSettings::requestFrameAutoHide);

    m_sectionsLayout->addWidget(genericSection);
    m_sectionsLayout->addWidget(vpnStrongSwanSection);
    m_sectionsLayout->addWidget(ipv4Section);
    m_sectionsLayout->addWidget(dnsSection);

    m_settingSections.append(genericSection);
    m_settingSections.append(vpnStrongSwanSection);
    m_settingSections.append(ipv4Section);
    m_settingSections.append(dnsSection);
}
