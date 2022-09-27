// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnadvopenvpnsection.h"

#include <DSpinBox>

#include <QEvent>

#include <widgets/contentwidget.h>
#include <widgets/switchwidget.h>
#include <widgets/spinboxwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnAdvOpenVPNSection::VpnAdvOpenVPNSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN Advanced"), parent)
    , m_vpnSetting(vpnSetting)
    , m_dataMap(vpnSetting->data())
    , m_portSwitch(new SwitchWidget(this))
    , m_renegIntervalSwitch(new SwitchWidget(this))
    , m_compLZOSwitch(new SwitchWidget(this))
    , m_tcpProtoSwitch(new SwitchWidget(this))
    , m_useTapSwitch(new SwitchWidget(this))
    , m_tunnelMTUSwitch(new SwitchWidget(this))
    , m_udpFragSizeSwitch(new SwitchWidget(this))
    , m_restrictMSSSwitch(new SwitchWidget(this))
    , m_randomRemoteSwitch(new SwitchWidget(this))
    , m_port(new SpinBoxWidget(this))
    , m_renegInterval(new SpinBoxWidget(this))
    , m_tunnelMTU(new SpinBoxWidget(this))
    , m_udpFragSize(new SpinBoxWidget(this))
{

    initUI();
    initConnection();
}

VpnAdvOpenVPNSection::~VpnAdvOpenVPNSection()
{
}

bool VpnAdvOpenVPNSection::allInputValid()
{
    return true;
}

void VpnAdvOpenVPNSection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();

    if (m_portSwitch->checked())
        m_dataMap.insert("port", QString::number(m_port->spinBox()->value()));
    else
        m_dataMap.remove("port");

    if (m_renegIntervalSwitch->checked())
        m_dataMap.insert("reneg-seconds", QString::number(m_renegInterval->spinBox()->value()));
    else
        m_dataMap.remove("reneg-seconds");

    if (m_compLZOSwitch->checked())
        m_dataMap.insert("comp-lzo", "yes");
    else
        m_dataMap.remove("comp-lzo");

    if (m_tcpProtoSwitch->checked())
        m_dataMap.insert("proto-tcp", "yes");
    else
        m_dataMap.remove("proto-tcp");

    if (m_useTapSwitch->checked())
        m_dataMap.insert("dev-type", "tap");
    else
        m_dataMap.remove("dev-type");

    if (m_tunnelMTUSwitch->checked())
        m_dataMap.insert("tunnel-mtu", QString::number(m_tunnelMTU->spinBox()->value()));
    else
        m_dataMap.remove("tunnel-mtu");

    if (m_udpFragSizeSwitch->checked())
        m_dataMap.insert("fragment-size", QString::number(m_udpFragSize->spinBox()->value()));
    else
        m_dataMap.remove("fragment-size");

    if (m_restrictMSSSwitch->checked())
        m_dataMap.insert("mssfix", "yes");
    else
        m_dataMap.remove("mssfix");

    if (m_randomRemoteSwitch->checked())
        m_dataMap.insert("remote-random", "yes");
    else
        m_dataMap.remove("remote-random");

    m_vpnSetting->setData(m_dataMap);

    m_vpnSetting->setInitialized(true);
}

void VpnAdvOpenVPNSection::initUI()
{
    m_portSwitch->setTitle(tr("Customize Gateway Port"));
    m_portSwitch->setChecked(m_dataMap.keys().contains("port"));

    m_port->setTitle(tr("Gateway Port"));
    m_port->spinBox()->setMinimum(0);
    m_port->spinBox()->setMaximum(65535);
    m_port->spinBox()->setValue(m_dataMap.value("port", "1194").toInt());
    m_port->setVisible(m_portSwitch->checked());

    m_renegIntervalSwitch->setTitle(tr("Customize Renegotiation Interval"));
    m_renegIntervalSwitch->setChecked(m_dataMap.keys().contains("reneg-seconds"));

    m_renegInterval->setTitle(tr("Renegotiation Interval"));
    m_renegInterval->spinBox()->setMinimum(0);
    m_renegInterval->spinBox()->setMaximum(65535);
    m_renegInterval->spinBox()->setValue(m_dataMap.value("reneg-seconds", "0").toInt());
    m_renegInterval->setVisible(m_renegIntervalSwitch->checked());

    m_compLZOSwitch->setTitle(tr("Use LZO Data Compression"));
    m_compLZOSwitch->setChecked(m_dataMap.value("comp-lzo") == "yes" || m_dataMap.value("comp-lzo") == "adaptive");

    m_tcpProtoSwitch->setTitle(tr("Use TCP Connection"));
    m_tcpProtoSwitch->setChecked(m_dataMap.value("proto-tcp") == "yes");

    m_useTapSwitch->setTitle(tr("Use TAP Device"));
    m_useTapSwitch->setChecked(m_dataMap.value("dev-type") == "tap");

    m_tunnelMTUSwitch->setTitle(tr("Customize Tunnel MTU"));
    m_tunnelMTUSwitch->setChecked(m_dataMap.keys().contains("tunnel-mtu"));

    m_tunnelMTU->setTitle(tr("MTU"));
    m_tunnelMTU->spinBox()->setMinimum(0);
    m_tunnelMTU->spinBox()->setMaximum(65535);
    m_tunnelMTU->spinBox()->setValue(m_dataMap.value("tunnel-mtu", "1500").toInt());
    m_tunnelMTU->setVisible(m_tunnelMTUSwitch->checked());

    m_udpFragSizeSwitch->setTitle(tr("Customize UDP Fragment Size"));
    m_udpFragSizeSwitch->setChecked(m_dataMap.keys().contains("fragment-size"));

    m_udpFragSize->setTitle(tr("UDP Fragment Size"));
    m_udpFragSize->spinBox()->setMinimum(0);
    m_udpFragSize->spinBox()->setMaximum(65535);
    m_udpFragSize->spinBox()->setValue(m_dataMap.value("fragment-size", "1300").toInt());
    m_udpFragSize->setVisible(m_udpFragSizeSwitch->checked());

    m_restrictMSSSwitch->setTitle(tr("Restrict Tunnel TCP MSS"));
    m_restrictMSSSwitch->setChecked(m_dataMap.value("mssfix") == "yes");

    m_randomRemoteSwitch->setTitle(tr("Randomize Remote Hosts"));
    m_randomRemoteSwitch->setChecked(m_dataMap.value("remote-random") == "yes");

    appendItem(m_portSwitch);
    appendItem(m_port);
    appendItem(m_renegIntervalSwitch);
    appendItem(m_renegInterval);
    appendItem(m_compLZOSwitch);
    appendItem(m_tcpProtoSwitch);
    appendItem(m_useTapSwitch);
    appendItem(m_tunnelMTUSwitch);
    appendItem(m_tunnelMTU);
    appendItem(m_udpFragSizeSwitch);
    appendItem(m_udpFragSize);
    appendItem(m_restrictMSSSwitch);
    appendItem(m_randomRemoteSwitch);

    m_port->spinBox()->installEventFilter(this);
    m_renegInterval->spinBox()->installEventFilter(this);
    m_tunnelMTU->spinBox()->installEventFilter(this);
    m_udpFragSize->spinBox()->installEventFilter(this);
}

void VpnAdvOpenVPNSection::initConnection()
{
    connect(m_portSwitch, &SwitchWidget::checkedChanged, m_port, &SpinBoxWidget::setVisible);
    connect(m_renegIntervalSwitch, &SwitchWidget::checkedChanged, m_renegInterval, &SpinBoxWidget::setVisible);
    connect(m_tunnelMTUSwitch, &SwitchWidget::checkedChanged, m_tunnelMTU, &SpinBoxWidget::setVisible);
    connect(m_udpFragSizeSwitch, &SwitchWidget::checkedChanged, m_udpFragSize, &SpinBoxWidget::setVisible);

    connect(m_portSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_renegIntervalSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_compLZOSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_tcpProtoSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_useTapSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_tunnelMTUSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_udpFragSizeSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_restrictMSSSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_randomRemoteSwitch, &SwitchWidget::checkedChanged, this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_port->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_renegInterval->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_tunnelMTU->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VpnAdvOpenVPNSection::editClicked);
    connect(m_udpFragSize->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VpnAdvOpenVPNSection::editClicked);
}

bool VpnAdvOpenVPNSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QSpinBox *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
