// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "secretwirelesssection.h"
#include "../widgets/passwdlineeditwidget.h"

#include <QComboBox>

#include <DPasswordEdit>

#include <networkmanagerqt/utils.h>
#include <widgets/lineeditwidget.h>
#include <widgets/comboxwidget.h>
#include <widgets/contentwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

SecretWirelessSection::SecretWirelessSection(WirelessSecuritySetting::Ptr wsSeting, Security8021xSetting::Ptr sSetting, ParametersContainer::Ptr parameter, QFrame *parent)
    : Secret8021xSection(sSetting, parent)
    , m_keyMgmtChooser(new ComboxWidget(this))
    , m_passwdEdit(new PasswdLineEditWidget(this))
    , m_enableWatcher(new Secret8021xEnableWatcher(this))
    , m_authAlgChooser(new ComboxWidget(this))
    , m_currentKeyMgmt(WirelessSecuritySetting::KeyMgmt::WpaNone)
    , m_currentAuthAlg(WirelessSecuritySetting::AuthAlg::Shared)
    , m_wsSetting(wsSeting)
    , m_s8Setting(sSetting)
    , m_parameter(parameter)
{
    initStrMaps();

    // init KeyMgmt
    const WirelessSecuritySetting::KeyMgmt &keyMgmt = m_wsSetting->keyMgmt();
    // 当前选择的加密方式
    m_currentKeyMgmt = (keyMgmt == WirelessSecuritySetting::KeyMgmt::Unknown) ?
                       WirelessSecuritySetting::KeyMgmt::WpaNone : keyMgmt;

    // 当前选择的保密模式
    m_currentAuthAlg = (m_wsSetting->authAlg() == WirelessSecuritySetting::AuthAlg::Open) ?
                        m_wsSetting->authAlg() : WirelessSecuritySetting::AuthAlg::Shared;

    if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
        Setting::SecretFlags passwordFlags = m_wsSetting->wepKeyFlags();
        QString strKey = m_wsSetting->wepKey0();
        for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
            if (passwordFlags.testFlag(it->second)) {
                m_currentPasswordType = it->second;
                if (m_currentPasswordType == Setting::None && strKey.isEmpty())
                    m_currentPasswordType = Setting::None;

                break;
            }
        }
    } else if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk ||
#ifdef USE_DEEPIN_NMQT
                m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae) {
#else
                m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE) {
#endif
        NetworkManager::Setting::SecretFlags passwordFlags = m_wsSetting->pskFlags();
        QString strKey = m_wsSetting->psk();
        for (auto it = PasswordFlagsStrMap.cbegin(); it != PasswordFlagsStrMap.cend(); ++it) {
            if (passwordFlags.testFlag(it->second)) {
                m_currentPasswordType = it->second;
                if (m_currentPasswordType == NetworkManager::Setting::None && strKey.isEmpty()) {
                    m_currentPasswordType = NetworkManager::Setting::None;
                }
                break;
            }
        }

    }

    initUI();
    initConnection();

    onKeyMgmtChanged(m_currentKeyMgmt);
}

SecretWirelessSection::~SecretWirelessSection()
{
}

bool SecretWirelessSection::allInputValid()
{
    bool valid = true;

    if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
        if (m_currentPasswordType != Setting::NotSaved) {
            valid = wepKeyIsValid(m_passwdEdit->text(), WirelessSecuritySetting::WepKeyType::Passphrase);
            m_passwdEdit->setIsErr(!valid);
            if (!valid && !m_passwdEdit->text().isEmpty())
                m_passwdEdit->dTextEdit()->showAlertMessage(tr("Invalid password"), this);
        }
    }

    if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk ||
#ifdef USE_DEEPIN_NMQT
            m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae) {
#else
            m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE) {
#endif
        if (m_currentPasswordType != Setting::NotSaved) {
            valid = wpaPskIsValid(m_passwdEdit->text());
            m_passwdEdit->setIsErr(!valid);
            if (!valid && !m_passwdEdit->text().isEmpty())
                m_passwdEdit->dTextEdit()->showAlertMessage(tr("Invalid password"), this);
        }
    }

    return valid ? Secret8021xSection::allInputValid() : false;
}

void SecretWirelessSection::saveSettings()
{
    m_wsSetting->setKeyMgmt(m_currentKeyMgmt);

    if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaNone
            || m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::Unknown) {
        return m_wsSetting->setInitialized(false);
    }

    if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
        m_wsSetting->setWepKeyType(WirelessSecuritySetting::WepKeyType::Passphrase);
        m_wsSetting->setWepKeyFlags(m_currentPasswordType);
        if (m_currentPasswordType != Setting::NotSaved)
            m_wsSetting->setWepKey0(m_passwdEdit->text());
        else
            m_wsSetting->setWepKey0(QString());

        m_wsSetting->setPskFlags(Setting::NotRequired);
        m_wsSetting->setAuthAlg(m_currentAuthAlg);
    } else if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk ||
#ifdef USE_DEEPIN_NMQT
                    m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae) {
#else
                    m_currentKeyMgmt == NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE) {
#endif
        m_wsSetting->setPskFlags(m_currentPasswordType);
        if (m_currentPasswordType != Setting::NotSaved)
            m_wsSetting->setPsk(m_passwdEdit->text());
        else
            m_wsSetting->setPsk(QString());

        m_wsSetting->setWepKeyType(WirelessSecuritySetting::WepKeyType::NotSpecified);
        if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk) {
            m_wsSetting->setAuthAlg(WirelessSecuritySetting::AuthAlg::Open);
        } else {
            m_wsSetting->setAuthAlg(WirelessSecuritySetting::AuthAlg::None);
        }
    } else if (m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaEap) {
        m_wsSetting->setAuthAlg(WirelessSecuritySetting::AuthAlg::None);
    }

    m_wsSetting->setInitialized(true);

    Secret8021xSection::saveSettings();
}

void SecretWirelessSection::initStrMaps()
{
    KeyMgmtStrMap = {
        { tr("None"), WirelessSecuritySetting::KeyMgmt::WpaNone },
        { tr("WEP"), WirelessSecuritySetting::KeyMgmt::Wep },
        { tr("WPA/WPA2 Personal"), WirelessSecuritySetting::KeyMgmt::WpaPsk },
        { tr("WPA/WPA2 Enterprise"), WirelessSecuritySetting::KeyMgmt::WpaEap },
#ifdef USE_DEEPIN_NMQT
        { tr("WPA3 Personal"), NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae }
#else
        { tr("WPA3 Personal"), NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE }
#endif
    };

    AuthAlgStrMap = {
        { tr("Shared key"), WirelessSecuritySetting::AuthAlg::Shared },
        { tr("Open system"), WirelessSecuritySetting::AuthAlg::Open }
    };
}

void SecretWirelessSection::initUI()
{
    m_keyMgmtChooser->setTitle(tr("Security"));
    QString curMgmtOption = KeyMgmtStrMap.at(0).first;
    for (auto it = KeyMgmtStrMap.cbegin(); it != KeyMgmtStrMap.cend(); ++it) {
        m_keyMgmtChooser->comboBox()->addItem(it->first, it->second);
        if (m_currentKeyMgmt == it->second)
            curMgmtOption = it->first;
    }

    m_keyMgmtChooser->setCurrentText(curMgmtOption);

    m_passwdEdit->setPlaceholderText(tr("Required"));

    m_enableWatcher->setSecretEnable(m_currentKeyMgmt == WirelessSecuritySetting::KeyMgmt::WpaEap);

    QList<Security8021xSetting::EapMethod> eapMethodsWantedList;
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodTls);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodLeap);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodFast);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodTtls);
    eapMethodsWantedList.append(Security8021xSetting::EapMethod::EapMethodPeap);

    m_authAlgChooser->setTitle(tr("Authentication"));
    QString curAuthAlgOption = AuthAlgStrMap.at(0).first;
    for (auto it = AuthAlgStrMap.cbegin(); it != AuthAlgStrMap.cend(); ++it) {
        m_authAlgChooser->comboBox()->addItem(it->first, it->second);
        if (m_currentAuthAlg == it->second)
            curAuthAlgOption = it->first;
    }

    m_authAlgChooser->setCurrentText(curAuthAlgOption);

    appendItem(m_keyMgmtChooser);
    appendItem(m_passwordFlagsChooser);
    init(m_enableWatcher, eapMethodsWantedList);
    appendItem(m_passwdEdit);
    appendItem(m_authAlgChooser);

    m_passwdEdit->textEdit()->installEventFilter(this);
}

void SecretWirelessSection::initConnection()
{
    connect(m_keyMgmtChooser, &ComboxWidget::onSelectChanged, this, [ = ] (const QString &dataSelected) {
        for (auto it = KeyMgmtStrMap.cbegin(); it != KeyMgmtStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                onKeyMgmtChanged(it->second);
                break;
            }
        }
    });

    connect(m_authAlgChooser, &ComboxWidget::onSelectChanged, this, [ = ] (const QString &dataSelected) {
        for (auto it = AuthAlgStrMap.cbegin(); it != AuthAlgStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                m_currentAuthAlg = it->second;
                break;
            }
        }
    });

    connect(m_passwdEdit->textEdit(), &QLineEdit::editingFinished, this, &SecretWirelessSection::saveUserInputPassword, Qt::QueuedConnection);

    connect(m_passwdEdit->textEdit(), &QLineEdit::textChanged, this, [this] (const QString &text) {
        if (text.isEmpty())
            static_cast<DPasswordEdit*>(m_passwdEdit->dTextEdit())->setEchoButtonIsVisible(true);
    });
    connect(m_enableWatcher, &Secret8021xEnableWatcher::passwdEnableChanged, this,  [ = ] (const bool enabled) {
        switch (m_currentKeyMgmt) {
        case WirelessSecuritySetting::KeyMgmt::WpaNone:
        case WirelessSecuritySetting::KeyMgmt::WpaEap: {
            m_passwdEdit->setVisible(false);
            break;
        }
        case WirelessSecuritySetting::KeyMgmt::Wep: {
            m_passwdEdit->setText(m_wsSetting->wepKey0());
            m_passwdEdit->setTitle(tr("Key"));
            m_passwdEdit->setVisible(enabled);
            break;
        }
        case WirelessSecuritySetting::KeyMgmt::WpaPsk:
#ifdef USE_DEEPIN_NMQT
        case WirelessSecuritySetting::KeyMgmt::WpaSae: {
#else
        case WirelessSecuritySetting::KeyMgmt::SAE: {
#endif
            m_passwdEdit->setText(m_wsSetting->psk());
            m_passwdEdit->setTitle(tr("Password"));
            m_passwdEdit->setVisible(enabled);
            break;
        }
        default:
            break;
        }
    });

    connect(m_keyMgmtChooser, &ComboxWidget::onIndexChanged, this, &SecretWirelessSection::editClicked);
    connect(m_authAlgChooser, &ComboxWidget::onIndexChanged, this, &SecretWirelessSection::editClicked);
}

void SecretWirelessSection::onKeyMgmtChanged(WirelessSecuritySetting::KeyMgmt keyMgmt)
{
    if (m_currentKeyMgmt != keyMgmt)
        m_currentKeyMgmt = keyMgmt;

    switch (m_currentKeyMgmt) {
    case WirelessSecuritySetting::KeyMgmt::WpaNone: {
        m_passwdEdit->setVisible(false);
        m_enableWatcher->setSecretEnable(false);
        m_passwordFlagsChooser->setVisible(false);
        m_authAlgChooser->setVisible(false);
        break;
    }
    case WirelessSecuritySetting::KeyMgmt::Wep: {
        if (m_currentPasswordType != Setting::NotSaved) {
            m_passwdEdit->setText(m_wsSetting->wepKey0());
            m_passwdEdit->setTitle(tr("Key"));
            m_passwdEdit->setVisible(true);
        } else {
            m_passwdEdit->setVisible(false);
        }
        m_enableWatcher->setSecretEnable(false);
        m_passwordFlagsChooser->setVisible(true);
        m_authAlgChooser->setVisible(true);
        break;
    }
    case WirelessSecuritySetting::KeyMgmt::WpaPsk: {
        if (m_currentPasswordType != Setting::NotSaved) {
            m_passwdEdit->setText(m_wsSetting->psk());
            m_passwdEdit->setTitle(tr("Password"));
            m_passwdEdit->setVisible(true);
        } else {
            m_passwdEdit->setVisible(false);
        }
        m_enableWatcher->setSecretEnable(false);
        m_passwordFlagsChooser->setVisible(true);
        m_authAlgChooser->setVisible(false);
        break;
    }
    case WirelessSecuritySetting::KeyMgmt::WpaEap: {
        m_passwdEdit->setVisible(false);
        m_enableWatcher->setSecretEnable(true);
        m_authAlgChooser->setVisible(false);
        break;
    }
#ifdef USE_DEEPIN_NMQT
    case WirelessSecuritySetting::KeyMgmt::WpaSae: {
#else
    case WirelessSecuritySetting::KeyMgmt::SAE: {
#endif
        if (m_currentPasswordType != NetworkManager::Setting::NotSaved) {
            m_passwdEdit->setText(m_wsSetting->psk());
            m_passwdEdit->setTitle(tr("Password"));
            m_passwdEdit->setVisible(true);
        } else {
            m_passwdEdit->setVisible(false);
        }
        m_enableWatcher->setSecretEnable(false);
        m_passwordFlagsChooser->setVisible(true);
        m_authAlgChooser->setVisible(false);
        break;
    }
    default:
        break;
    }

    if (m_userInputPasswordMap.contains(m_currentKeyMgmt))
        m_passwdEdit->setText(m_userInputPasswordMap.value(m_currentKeyMgmt));
}

void SecretWirelessSection::saveUserInputPassword()
{
    m_userInputPasswordMap.insert(m_currentKeyMgmt, m_passwdEdit->text());
}
