// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dslpppoesettings.h"
#include "../sections/genericsection.h"
#include "../sections/pppoesection.h"
#include "../sections/multiipvxsection.h"
#include "../sections/dnssection.h"
#include "../sections/pppsection.h"
#include "../sections/ethernetsection.h"
#include "../connectioneditpage.h"

#include <QVBoxLayout>

using namespace NetworkManager;

DslPppoeSettings::DslPppoeSettings(ConnectionSettings::Ptr connSettings, QString devicePath, QWidget *parent)
    : AbstractSettings(connSettings, parent)
    , m_devicePath(devicePath)
    , m_parent(parent)
{
    setAccessibleName("DslPppoeSettings");
    initSections();
}

DslPppoeSettings::~DslPppoeSettings()
{
}

void DslPppoeSettings::initSections()
{
    GenericSection *genericSection = new GenericSection(m_connSettings);
    PPPOESection *pppoeSection = new PPPOESection(m_connSettings->setting(Setting::Pppoe).staticCast<PppoeSetting>());
    MultiIpvxSection *ipv4Section = new MultiIpvxSection(m_connSettings->setting(Setting::Ipv4).staticCast<Ipv4Setting>());
    DNSSection *dnsSection = new DNSSection(m_connSettings, false);
    m_etherNetSection = new EthernetSection(m_connSettings->setting(Setting::Wired).staticCast<WiredSetting>(), false, m_devicePath);
    PPPSection *pppSection = new PPPSection(m_connSettings->setting(Setting::Ppp).staticCast<PppSetting>());

    connect(genericSection, &GenericSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(pppoeSection, &EthernetSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(ipv4Section, &MultiIpvxSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(dnsSection, &DNSSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(m_etherNetSection, &EthernetSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(pppSection, &MultiIpvxSection::editClicked, this, &DslPppoeSettings::anyEditClicked);
    connect(dnsSection, &DNSSection::editClicked, this, &DslPppoeSettings::anyEditClicked);

    connect(ipv4Section, &MultiIpvxSection::requestNextPage, this, &DslPppoeSettings::requestNextPage);
    connect(dnsSection, &DNSSection::requestNextPage, this, &DslPppoeSettings::requestNextPage);
    connect(m_etherNetSection, &EthernetSection::requestNextPage, this, &DslPppoeSettings::requestNextPage);

    m_sectionsLayout->addWidget(genericSection);
    m_sectionsLayout->addWidget(pppoeSection);
    m_sectionsLayout->addWidget(ipv4Section);
    m_sectionsLayout->addWidget(dnsSection);
    m_sectionsLayout->addWidget(m_etherNetSection);
    m_sectionsLayout->addWidget(pppSection);

    m_settingSections.append(genericSection);
    m_settingSections.append(pppoeSection);
    m_settingSections.append(ipv4Section);
    m_settingSections.append(dnsSection);
    m_settingSections.append(m_etherNetSection);
    m_settingSections.append(pppSection);
}

bool DslPppoeSettings::clearInterfaceName()
{
    ConnectionEditPage *page = dynamic_cast<ConnectionEditPage *>(m_parent);
    if (page)
        page->setDevicePath(m_etherNetSection->devicePath());

    WiredSetting::Ptr wiredSetting = m_connSettings->setting(Setting::Wired).staticCast<WiredSetting>();
    return wiredSetting->macAddress().isEmpty();
}
