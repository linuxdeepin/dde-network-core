// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnsection.h"
#include "../../widgets/passwdlineeditwidget.h"

#include <widgets/contentwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/comboxwidget.h>

#include <QHostAddress>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnSection::VpnSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN"), parent)
    , m_vpnSetting(vpnSetting)
    , m_gateway(new LineEditWidget(this))
    , m_userName(new LineEditWidget(this))
    , m_passwordFlagsChooser(new ComboxWidget(this))
    , m_password(new PasswdLineEditWidget(this))
    , m_domain(new LineEditWidget(this))
{
    setAccessibleName("VpnSection");
    m_dataMap = vpnSetting->data();
    m_secretMap = vpnSetting->secrets();
    m_currentPasswordType = static_cast<Setting::SecretFlagType>(m_dataMap.value("password-flags", "0").toInt());

    initStrMaps();
    initUI();
    initConnection();

    onPasswordFlagsChanged(m_currentPasswordType);
}

VpnSection::~VpnSection()
{
}

bool VpnSection::allInputValid()
{
    bool valid = true;

    if (m_gateway->text().isEmpty()) {
        valid = false;
        m_gateway->setIsErr(true);
        m_gateway->dTextEdit()->showAlertMessage(tr("Invalid gateway"), parentWidget(), 2000);
    } else {
        m_gateway->setIsErr(false);
    }

    if (m_userName->text().isEmpty()) {
        valid = false;
        m_userName->setIsErr(true);
    } else {
        m_userName->setIsErr(false);
    }

    if (m_currentPasswordType == Setting::SecretFlagType::None
            && m_password->text().isEmpty()) {
        valid = false;
        m_password->setIsErr(true);
    } else {
        m_password->setIsErr(false);
    }

    return valid;
}

void VpnSection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();
    m_secretMap = m_vpnSetting->secrets();

    m_dataMap.insert("gateway", m_gateway->text());
    m_dataMap.insert("user", m_userName->text());
    m_dataMap.insert("password-flags", QString::number(m_currentPasswordType));
    if (m_currentPasswordType == Setting::SecretFlagType::None)
        m_secretMap.insert("password", m_password->text());
    else
        m_secretMap.remove("password");

    if (!m_domain->text().isEmpty())
        m_dataMap.insert("domain", m_domain->text());

    m_vpnSetting->setData(m_dataMap);
    m_vpnSetting->setSecrets(m_secretMap);
    m_vpnSetting->setInitialized(true);
}

void VpnSection::initStrMaps()
{
    PasswordFlagsStrMap = {
        //{tr("Saved"), Setting::AgentOwned},
        { tr("Saved"), Setting::SecretFlagType::None },
        { tr("Ask"), Setting::SecretFlagType::NotSaved },
        { tr("Not Required"), Setting::SecretFlagType::NotRequired }
    };
}

void VpnSection::initUI()
{
    m_gateway->setTitle(tr("Gateway"));
    m_gateway->setPlaceholderText(tr("Required"));
    m_gateway->setText(m_dataMap.value("gateway"));

    m_userName->setTitle(tr("Username"));
    m_userName->setPlaceholderText(tr("Required"));
    m_userName->setText(m_dataMap.value("user"));

    m_passwordFlagsChooser->setTitle(tr("Pwd Options"));
    QStringList comboxOptions;
    QString curOption;
    for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
        comboxOptions << it->first;
        if (it->second == m_currentPasswordType)
            curOption = it->first;
    }

    m_passwordFlagsChooser->setComboxOption(comboxOptions);
    m_passwordFlagsChooser->setCurrentText(curOption);

    m_password->setTitle(tr("Password"));
    m_password->setPlaceholderText(tr("Required"));
    m_password->setText(m_secretMap.value("password"));

    m_domain->setTitle(tr("NT Domain"));
    m_domain->setText(m_dataMap.value("domain"));

    appendItem(m_gateway);
    appendItem(m_userName);
    appendItem(m_passwordFlagsChooser);
    appendItem(m_password);
    appendItem(m_domain);

    m_gateway->textEdit()->installEventFilter(this);
    m_userName->textEdit()->installEventFilter(this);
    m_password->textEdit()->installEventFilter(this);
    m_domain->textEdit()->installEventFilter(this);
}

void VpnSection::initConnection()
{
    connect(m_passwordFlagsChooser, &ComboxWidget::onSelectChanged, [ this ] (const QString &passwordKey) {
        for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
            if (it->first == passwordKey) {
                onPasswordFlagsChanged(it->second);
                break;
            }
        }
    });

    connect(m_passwordFlagsChooser, &ComboxWidget::onIndexChanged, this, &VpnSection::editClicked);
}

void VpnSection::onPasswordFlagsChanged(Setting::SecretFlagType type)
{
    m_currentPasswordType = type;
    m_password->setVisible(m_currentPasswordType == Setting::SecretFlagType::None);
}

bool VpnSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}

bool VpnSection::isIpv4Address(const QString &ip)
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv4)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        return false;
    }

    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    return regExpIP.exactMatch(ip);
}
