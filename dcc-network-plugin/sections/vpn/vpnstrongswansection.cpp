// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnstrongswansection.h"
#include "../../widgets/passwdlineeditwidget.h"

#include <QComboBox>
#include <QHostAddress>

#include <widgets/contentwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/filechoosewidget.h>
#include <widgets/comboxwidget.h>
#include <widgets/switchwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnStrongSwanSection::VpnStrongSwanSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN"), parent)
    , m_vpnSetting(vpnSetting)
    , m_dataMap(vpnSetting->data())
    , m_secretMap(vpnSetting->secrets())
    , m_gateway(new LineEditWidget(this))
    , m_caCert(new FileChooseWidget(this))
    , m_authTypeChooser(new ComboxWidget(this))
    , m_userCert(new FileChooseWidget(this))
    , m_userKey(new FileChooseWidget(this))
    , m_userName(new LineEditWidget(this))
    , m_password(new PasswdLineEditWidget(this))
    , m_requestInnerIp(new SwitchWidget(this))
    , m_enforceUDP(new SwitchWidget(this))
    , m_useIPComp(new SwitchWidget(this))
    , m_enableCustomCipher(new SwitchWidget(this))
    , m_ike(new LineEditWidget(this))
    , m_esp(new LineEditWidget(this))
{
    initStrMaps();
    initUI();
    initConnection();

    onAuthTypeChanged(m_currentAuthType);
    onCustomCipherEnableChanged(m_enableCustomCipher->checked());
}

VpnStrongSwanSection::~VpnStrongSwanSection()
{
}

bool VpnStrongSwanSection::allInputValid()
{
    bool valid = true;

    if (m_gateway->text().isEmpty()) {
        valid = false;
        m_gateway->setIsErr(true);
        m_gateway->dTextEdit()->showAlertMessage(tr("Invalid gateway"), parentWidget(), 2000);
    } else {
        m_gateway->setIsErr(false);
    }

    return valid;
}

void VpnStrongSwanSection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();
    m_secretMap = m_vpnSetting->secrets();

    m_dataMap.insert("address", m_gateway->text());
    m_dataMap.insert("certificate", m_caCert->edit()->text());
    m_dataMap.insert("method", m_currentAuthType);

    if (m_currentAuthType == "key" || m_currentAuthType == "agent") {
        m_dataMap.insert("usercert", m_userCert->edit()->text());
        if (m_currentAuthType == "key")
            m_dataMap.insert("userkey", m_userKey->edit()->text());
        else
            m_dataMap.remove("userkey");
    } else {
        m_dataMap.remove("usercert");
        m_dataMap.remove("userkey");
    }

    if (m_currentAuthType == "eap" || m_currentAuthType == "psk") {
        m_dataMap.insert("user", m_userName->text());
        m_secretMap.insert("password", m_password->text());
    } else {
        m_dataMap.remove("user");
        m_secretMap.remove("password");
    }

    if (m_requestInnerIp->checked())
        m_dataMap.insert("virtual", "yes");
    else
        m_dataMap.remove("virtual");

    if (m_enforceUDP->checked())
        m_dataMap.insert("encap", "yes");
    else
        m_dataMap.remove("encap");

    if (m_useIPComp->checked())
        m_dataMap.insert("ipcomp", "yes");
    else
        m_dataMap.remove("ipcomp");

    if (m_enableCustomCipher->checked()) {
        m_dataMap.insert("proposal", "yes");
        m_dataMap.insert("ike", m_ike->text());
        m_dataMap.insert("esp", m_esp->text());
    } else {
        m_dataMap.remove("proposal");
        m_dataMap.remove("esp");
    }

    m_vpnSetting->setData(m_dataMap);
    m_vpnSetting->setSecrets(m_secretMap);
    m_vpnSetting->setInitialized(true);
}

void VpnStrongSwanSection::initStrMaps()
{
    AuthTypeStrMap = {
        { tr("Private Key"), "key" },
        { tr("SSH Agent"), "agent" },
        { tr("Smart Card"), "smartcard" },
        { tr("EAP"), "eap" },
        { tr("Pre-Shared Key"), "psk" },
    };
}

void VpnStrongSwanSection::initUI()
{
    m_gateway->setTitle(tr("Gateway"));
    m_gateway->setPlaceholderText(tr("Required"));
    m_gateway->setText(m_dataMap.value("address"));

    m_caCert->setTitle(tr("CA Cert"));
    m_caCert->edit()->setText(m_dataMap.value("certificate"));

    m_authTypeChooser->setTitle(tr("Auth Type"));
    m_currentAuthType = "key";
    QString curAuthOption = AuthTypeStrMap.at(0).first;
    for (auto it = AuthTypeStrMap.cbegin(); it != AuthTypeStrMap.cend(); ++it) {
        m_authTypeChooser->comboBox()->addItem(it->first, it->second);
        if (it->second == m_dataMap.value("method")) {
            m_currentAuthType = it->second;
            curAuthOption = it->first;
        }
    }

    m_authTypeChooser->setCurrentText(curAuthOption);

    m_userCert->setTitle(tr("User Cert"));
    m_userCert->edit()->setText(m_dataMap.value("usercert"));

    m_userKey->setTitle(tr("Private Key"));
    m_userKey->edit()->setText(m_dataMap.value("userkey"));

    m_userName->setTitle(tr("Username"));
    m_userName->setText(m_dataMap.value("user"));

    m_password->setTitle(tr("Password"));
    m_password->setText(m_secretMap.value("password"));

    m_requestInnerIp->setTitle(tr("Request an Inner IP Address"));
    m_requestInnerIp->setChecked(m_dataMap.value("virtual") == "yes");

    m_enforceUDP->setTitle(tr("Enforce UDP Encapsulation"));
    m_enforceUDP->setChecked(m_dataMap.value("encap") == "yes");

    m_useIPComp->setTitle(tr("Use IP Compression"));
    m_useIPComp->setChecked(m_dataMap.value("ipcomp") == "yes");

    m_enableCustomCipher->setTitle(tr("Enable Custom Cipher Proposals"));
    m_enableCustomCipher->setChecked(m_dataMap.value("proposal") == "yes");

    m_ike->setTitle(tr("IKE"));
    m_ike->setText(m_dataMap.value("ike"));

    m_esp->setTitle(tr("ESP"));
    m_esp->setText(m_dataMap.value("esp"));

    appendItem(m_gateway);
    appendItem(m_caCert);
    appendItem(m_authTypeChooser);

    appendItem(m_userCert);
    appendItem(m_userKey);
    appendItem(m_userName);
    appendItem(m_password);

    appendItem(m_requestInnerIp);
    appendItem(m_enforceUDP);
    appendItem(m_useIPComp);
    appendItem(m_enableCustomCipher);
    appendItem(m_ike);
    appendItem(m_esp);

    m_gateway->textEdit()->installEventFilter(this);
    m_userName->textEdit()->installEventFilter(this);
    m_password->textEdit()->installEventFilter(this);
    m_ike->textEdit()->installEventFilter(this);
    m_esp->textEdit()->installEventFilter(this);
    m_caCert->edit()->lineEdit()->installEventFilter(this);
    m_userCert->edit()->lineEdit()->installEventFilter(this);
    m_userKey->edit()->lineEdit()->installEventFilter(this);
}

void VpnStrongSwanSection::initConnection()
{
    connect(m_authTypeChooser, &ComboxWidget::onSelectChanged, this, [ = ](const QString &dataSelected) {
        for (auto it = AuthTypeStrMap.cbegin(); it != AuthTypeStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                onAuthTypeChanged(it->second);
                break;
            }
        }
    });

    connect(m_enableCustomCipher, &SwitchWidget::checkedChanged, this, &VpnStrongSwanSection::onCustomCipherEnableChanged);

    connect(m_caCert, &FileChooseWidget::requestFrameKeepAutoHide, this, &VpnStrongSwanSection::requestFrameAutoHide);
    connect(m_userCert, &FileChooseWidget::requestFrameKeepAutoHide, this, &VpnStrongSwanSection::requestFrameAutoHide);
    connect(m_userKey, &FileChooseWidget::requestFrameKeepAutoHide, this, &VpnStrongSwanSection::requestFrameAutoHide);

    connect(m_authTypeChooser, &ComboxWidget::onIndexChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_requestInnerIp, &SwitchWidget::checkedChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_enforceUDP, &SwitchWidget::checkedChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_useIPComp, &SwitchWidget::checkedChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_enableCustomCipher, &SwitchWidget::checkedChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_caCert->edit()->lineEdit(), &QLineEdit::textChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_userCert->edit()->lineEdit(), &QLineEdit::textChanged, this, &VpnStrongSwanSection::editClicked);
    connect(m_userKey->edit()->lineEdit(), &QLineEdit::textChanged, this, &VpnStrongSwanSection::editClicked);
}

void VpnStrongSwanSection::onAuthTypeChanged(const QString &type)
{
    m_currentAuthType = type;

    m_userCert->setVisible(m_currentAuthType == "key" || m_currentAuthType == "agent");
    m_userKey->setVisible(m_currentAuthType == "key");
    m_userName->setVisible(m_currentAuthType == "eap" || m_currentAuthType == "psk");
    m_password->setVisible(m_currentAuthType == "eap" || m_currentAuthType == "psk");
}

void VpnStrongSwanSection::onCustomCipherEnableChanged(const bool enabled)
{
    m_ike->setVisible(enabled);
    m_esp->setVisible(enabled);
}

bool VpnStrongSwanSection::isIpv4Address(const QString &ip)
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv4)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        return false;
    }

    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    return regExpIP.exactMatch(ip);
}

bool VpnStrongSwanSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
