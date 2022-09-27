// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnsstpproxysection.h"
#include "../../widgets/passwdlineeditwidget.h"

#include <DSpinBox>

#include <widgets/contentwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/switchwidget.h>
#include <widgets/spinboxwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnSstpProxySection::VpnSstpProxySection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN Proxy"), parent)
    , m_vpnSetting(vpnSetting)
    , m_dataMap(vpnSetting->data())
    , m_secretMap(vpnSetting->secrets())
    , m_server(new LineEditWidget(this))
    , m_port(new SpinBoxWidget(this))
    , m_userName(new LineEditWidget(this))
    , m_password(new PasswdLineEditWidget(this))
{
    initUI();
    initConnection();
}

VpnSstpProxySection::~VpnSstpProxySection()
{
}

bool VpnSstpProxySection::allInputValid()
{
    bool valid = true;

    QString server = m_server->text();
    int port = m_port->spinBox()->value();

    valid = (server.isEmpty() == (port == 0));

    if (!valid) {
        m_server->setIsErr(server.isEmpty());
        m_port->setIsErr(port == 0);
    } else {
        m_server->setIsErr(false);
        m_port->setIsErr(false);
    }

    return valid;
}

void VpnSstpProxySection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();
    m_secretMap = m_vpnSetting->secrets();

    if (m_server->text().isEmpty() || m_port->spinBox()->value() == 0) {
        m_dataMap.remove("proxy-server");
        m_dataMap.remove("proxy-port");
        m_dataMap.remove("proxy-user");
        m_secretMap.remove("proxy-password");
    } else {
        m_dataMap.insert("proxy-server", m_server->text());
        m_dataMap.insert("proxy-port", QString::number(m_port->spinBox()->value()));
    }

    if (m_userName->text().isEmpty()) {
        m_dataMap.remove("proxy-user");
        m_secretMap.remove("proxy-password");
    } else {
        m_dataMap.insert("proxy-user", m_userName->text());
        if (m_password->text().isEmpty())
            m_secretMap.remove("proxy-password");
        else
            m_secretMap.insert("proxy-password", m_password->text());
    }

    m_vpnSetting->setData(m_dataMap);
    m_vpnSetting->setSecrets(m_secretMap);
    m_vpnSetting->setInitialized(true);
}

void VpnSstpProxySection::initUI()
{
    m_server->setTitle(tr("Server IP"));
    m_server->setText(m_dataMap.value("proxy-server"));

    m_port->setTitle(tr("Port"));
    m_port->spinBox()->setMinimum(0);
    m_port->spinBox()->setMaximum(65535);
    m_port->spinBox()->setValue(m_dataMap.value("proxy-port").toInt());

    m_userName->setTitle(tr("Username"));
    m_userName->setText(m_dataMap.value("proxy-user"));

    m_password->setTitle(tr("Password"));
    m_password->setText(m_secretMap.value("proxy-password"));

    appendItem(m_server);
    appendItem(m_port);
    appendItem(m_userName);
    appendItem(m_password);
}

void VpnSstpProxySection::initConnection()
{
    connect(m_server->textEdit(), &QLineEdit::editingFinished, this, &VpnSstpProxySection::allInputValid);
    connect(m_port->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &VpnSstpProxySection::allInputValid);
}
