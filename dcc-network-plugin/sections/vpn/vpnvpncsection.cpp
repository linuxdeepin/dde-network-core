// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnvpncsection.h"
#include "../../widgets/passwdlineeditwidget.h"

#include <QComboBox>
#include <QHostAddress>

#include <widgets/contentwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/comboxwidget.h>
#include <widgets/switchwidget.h>
#include <widgets/filechoosewidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnVPNCSection::VpnVPNCSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN"), parent)
    , m_vpnSetting(vpnSetting)
    , m_gateway(new LineEditWidget(this))
    , m_userName(new LineEditWidget(this))
    , m_passwordFlagsChooser(new ComboxWidget(this))
    , m_password(new PasswdLineEditWidget(this))
    , m_groupName(new LineEditWidget(this))
    , m_groupPasswordFlagsChooser(new ComboxWidget(this))
    , m_groupPassword(new PasswdLineEditWidget(this))
    , m_userHybrid(new SwitchWidget(this))
    , m_caFile(new FileChooseWidget(this))
{
    m_dataMap = vpnSetting->data();
    m_secretMap = vpnSetting->secrets();

    initStrMaps();

    m_currentPasswordType = static_cast<Setting::SecretFlagType>(m_dataMap.value("Xauth password-flags", "0").toInt());
    m_currentGroupPasswordType = static_cast<Setting::SecretFlagType>(m_dataMap.value("IPSec secret-flags", "0").toInt());

    initUI();
    initConnection();

    onPasswordFlagsChanged(m_currentPasswordType);
    onGroupPasswordFlagsChanged(m_currentGroupPasswordType);
}

VpnVPNCSection::~VpnVPNCSection()
{
}

bool VpnVPNCSection::allInputValid()
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
    if (m_groupName->text().isEmpty()) {
        valid = false;
        m_groupName->setIsErr(true);
    } else {
        m_groupName->setIsErr(false);
    }
    if (m_currentGroupPasswordType == Setting::SecretFlagType::None
            && m_groupPassword->text().isEmpty()) {
        valid = false;
        m_groupPassword->setIsErr(true);
    } else {
        m_groupPassword->setIsErr(false);
    }
    if (m_userHybrid->checked() && m_caFile->edit()->text().isEmpty()) {
        valid = false;
        m_caFile->setIsErr(true);
    } else {
        m_caFile->setIsErr(false);
    }

    return valid;
}

void VpnVPNCSection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();
    m_secretMap = m_vpnSetting->secrets();

    // save values about user
    m_dataMap.insert("IPSec gateway", m_gateway->text());
    m_dataMap.insert("Xauth username", m_userName->text());
    m_dataMap.insert("Xauth password-flags", QString::number(m_currentPasswordType));
    if (m_currentPasswordType == Setting::SecretFlagType::None) {
        m_dataMap.insert("xauth-password-type", "save");
        m_secretMap.insert("Xauth password", m_password->text());
    } else if (m_currentPasswordType == Setting::SecretFlagType::NotSaved) {
        m_dataMap.insert("xauth-password-type", "ask");
        m_secretMap.remove("Xauth password");
    } else if (m_currentPasswordType == Setting::SecretFlagType::NotRequired) {
        m_dataMap.insert("xauth-password-type", "unused");
        m_secretMap.remove("Xauth password");
    }
    // save values about gourp
    m_dataMap.insert("IPSec ID", m_groupName->text());
    m_dataMap.insert("IPSec secret-flags", QString::number(m_currentGroupPasswordType));
    if (m_currentGroupPasswordType == Setting::SecretFlagType::None) {
        m_dataMap.insert("ipsec-secret-type", "save");
        m_secretMap.insert("IPSec secret", m_groupPassword->text());
    } else if (m_currentGroupPasswordType == Setting::SecretFlagType::NotSaved) {
        m_dataMap.insert("ipsec-secret-type", "ask");
        m_secretMap.remove("IPSec secret");
    } else if (m_currentGroupPasswordType == Setting::SecretFlagType::NotRequired) {
        m_dataMap.insert("ipsec-secret-type", "unused");
        m_secretMap.remove("IPSec secret");
    }

    if (m_userHybrid->checked()) {
        m_dataMap.insert("IKE Authmode", "hybrid");
        m_dataMap.insert("CA-File", m_caFile->edit()->text());
    } else {
        m_dataMap.remove("IKE Authmode");
    }

    m_vpnSetting->setData(m_dataMap);
    m_vpnSetting->setSecrets(m_secretMap);
    m_vpnSetting->setInitialized(true);
}

void VpnVPNCSection::initStrMaps()
{
    PasswordFlagsStrMap = {
        //{"Saved", Setting::AgentOwned},
        { tr("Saved"), Setting::SecretFlagType::None },
        { tr("Ask"), Setting::SecretFlagType::NotSaved },
        { tr("Not Required"), Setting::SecretFlagType::NotRequired }
    };
}

void VpnVPNCSection::initUI()
{
    m_gateway->setTitle(tr("Gateway"));
    m_gateway->setPlaceholderText(tr("Required"));
    m_gateway->setText(m_dataMap.value("IPSec gateway"));

    m_userName->setTitle(tr("Username"));
    m_userName->setPlaceholderText(tr("Required"));
    m_userName->setText(m_dataMap.value("Xauth username"));

    m_passwordFlagsChooser->setTitle(tr("Pwd Options"));
    QString curPasswordOption = PasswordFlagsStrMap.at(0).first;
    for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
        m_passwordFlagsChooser->comboBox()->addItem(it->first, it->second);
        if (it->second == m_currentPasswordType)
            curPasswordOption = it->first;
    }

    m_passwordFlagsChooser->setCurrentText(curPasswordOption);

    m_password->setTitle(tr("Password"));
    m_password->setPlaceholderText(tr("Required"));
    m_password->setText(m_secretMap.value("Xauth password"));

    m_groupName->setTitle(tr("Group Name"));
    m_groupName->setPlaceholderText(tr("Required"));
    m_groupName->setText(m_dataMap.value("IPSec ID"));

    m_groupPasswordFlagsChooser->setTitle(tr("Pwd Options"));

    QString curGroupPasswordOption = PasswordFlagsStrMap.at(0).first;
    for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
        m_groupPasswordFlagsChooser->comboBox()->addItem(it->first, it->second);
        if (it->second == m_currentGroupPasswordType)
            curGroupPasswordOption = it->first;
    }

    m_groupPasswordFlagsChooser->setCurrentText(curGroupPasswordOption);

    m_groupPassword->setTitle(tr("Group Pwd"));
    m_groupPassword->setPlaceholderText(tr("Required"));
    m_groupPassword->setText(m_secretMap.value("IPSec secret"));

    m_userHybrid->setTitle(tr("Use Hybrid Authentication"));
    m_userHybrid->setChecked(m_dataMap.value("IKE Authmode") == "hybrid");

    m_caFile->setTitle(tr("CA File"));
    m_caFile->edit()->setText(m_dataMap.value("CA-File"));
    m_caFile->setVisible(m_userHybrid->checked());

    appendItem(m_gateway);
    appendItem(m_userName);
    appendItem(m_passwordFlagsChooser);
    appendItem(m_password);
    appendItem(m_groupName);
    appendItem(m_groupPasswordFlagsChooser);
    appendItem(m_groupPassword);
    appendItem(m_userHybrid);
    appendItem(m_caFile);

    m_gateway->textEdit()->installEventFilter(this);
    m_userName->textEdit()->installEventFilter(this);
    m_password->textEdit()->installEventFilter(this);
    m_groupName->textEdit()->installEventFilter(this);
    m_groupPassword->textEdit()->installEventFilter(this);
    m_caFile->edit()->lineEdit()->installEventFilter(this);
}

void VpnVPNCSection::initConnection()
{
    connect(m_passwordFlagsChooser, &ComboxWidget::onSelectChanged, this, [ = ](const QString &dataSelected) {
        for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                onPasswordFlagsChanged(it->second);
                break;
            }
        }
    });

    connect(m_groupPasswordFlagsChooser, &ComboxWidget::onSelectChanged, this, [ = ](const QString &dataSelected) {
        for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                onGroupPasswordFlagsChanged(it->second);
                break;
            }
        }
    });

    connect(m_userHybrid, &SwitchWidget::checkedChanged, this, [ = ](const bool checked) {
        m_caFile->setVisible(checked);
        allInputValid();
    });

    connect(m_caFile, &FileChooseWidget::requestFrameKeepAutoHide, this, &VpnVPNCSection::requestFrameAutoHide);

    connect(m_passwordFlagsChooser, &ComboxWidget::onIndexChanged, this, &VpnVPNCSection::editClicked);
    connect(m_groupPasswordFlagsChooser, &ComboxWidget::onIndexChanged, this, &VpnVPNCSection::editClicked);
    connect(m_userHybrid, &SwitchWidget::checkedChanged, this, &VpnVPNCSection::editClicked);
    connect(m_caFile->edit()->lineEdit(), &QLineEdit::textChanged, this, &VpnVPNCSection::editClicked);
}

void VpnVPNCSection::onPasswordFlagsChanged(Setting::SecretFlagType type)
{
    m_currentPasswordType = type;
    m_password->setVisible(m_currentPasswordType == Setting::SecretFlagType::None);
}

void VpnVPNCSection::onGroupPasswordFlagsChanged(Setting::SecretFlagType type)
{
    m_currentGroupPasswordType = type;
    m_groupPassword->setVisible(m_currentGroupPasswordType == Setting::SecretFlagType::None);
}

bool VpnVPNCSection::isIpv4Address(const QString &ip)
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv4)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        return false;
    }

    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    return regExpIP.exactMatch(ip);
}

bool VpnVPNCSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
