// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnsstpsection.h"

#include <widgets/filechoosewidget.h>
#include <widgets/switchwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnSSTPSection::VpnSSTPSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : VpnSection(vpnSetting, parent)
    , m_vpnSetting(vpnSetting)
    , m_dataMap(vpnSetting->data())
    , m_caFile(new FileChooseWidget(this))
    , m_ignoreCAWarnings(new SwitchWidget(this))
    , m_useTLSExt(new SwitchWidget(this))
{
    initUI();

    connect(m_caFile, &FileChooseWidget::requestFrameKeepAutoHide, this, &VpnSSTPSection::requestFrameAutoHide);
}

VpnSSTPSection::~VpnSSTPSection()
{
}

void VpnSSTPSection::saveSettings()
{
    // 在之前保存父类数据的设置
    VpnSection::saveSettings();

    // retrieve the data map
    m_dataMap = m_vpnSetting->data();

    if (m_caFile->edit()->text().isEmpty())
        m_dataMap.remove("ca-cert");
    else
        m_dataMap.insert("ca-cert", m_caFile->edit()->text());

    if (m_ignoreCAWarnings->checked())
        m_dataMap.insert("ignore-cert-warn", "yes");
    else
        m_dataMap.remove("ignore-cert-warn");

    if (m_useTLSExt->checked())
        m_dataMap.insert("tls-ext", "yes");
    else
        m_dataMap.remove("tls-ext");

    m_vpnSetting->setData(m_dataMap);
    m_vpnSetting->setInitialized(true);
}

void VpnSSTPSection::initUI()
{
    m_caFile->setTitle(tr("CA File"));
    m_caFile->edit()->setText(m_dataMap.value("ca-cert"));

    m_ignoreCAWarnings->setTitle(tr("Ignore Certificate Warnings"));
    m_ignoreCAWarnings->setChecked(m_dataMap.value("ignore-cert-warn") == "yes");

    m_useTLSExt->setTitle(tr("Use TLS Hostname Extensions"));
    m_useTLSExt->setChecked(m_dataMap.value("tls-ext") == "yes");

    appendItem(m_caFile);
    appendItem(m_ignoreCAWarnings);
    appendItem(m_useTLSExt);
}
