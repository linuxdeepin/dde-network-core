// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vpnsecopenvpnsection.h"

#include <QComboBox>

#include <widgets/contentwidget.h>
#include <widgets/comboxwidget.h>

using namespace dcc::widgets;
using namespace NetworkManager;

VpnSecOpenVPNSection::VpnSecOpenVPNSection(VpnSetting::Ptr vpnSetting, QFrame *parent)
    : AbstractSection(tr("VPN Security"), parent)
    , m_vpnSetting(vpnSetting)
    , m_dataMap(vpnSetting->data())
    , m_cipherChooser(new ComboxWidget(this))
    , m_hmacChooser(new ComboxWidget(this))
{
    initStrMaps();
    initUI();
    initConnection();
}

VpnSecOpenVPNSection::~VpnSecOpenVPNSection()
{
}

bool VpnSecOpenVPNSection::allInputValid()
{
    return true;
}

void VpnSecOpenVPNSection::saveSettings()
{
    // retrieve the data map
    m_dataMap = m_vpnSetting->data();

    if (m_currentCipher == "default")
        m_dataMap.remove("cipher");
    else
        m_dataMap.insert("cipher", m_currentCipher);

    if (m_currentHMAC == "default")
        m_dataMap.remove("auth");
    else
        m_dataMap.insert("auth", m_currentHMAC);

    m_vpnSetting->setData(m_dataMap);

    m_vpnSetting->setInitialized(true);
}

void VpnSecOpenVPNSection::initStrMaps()
{
    CipherStrMap = {
        { tr("Default"), "default" },
        { tr("None"), "none" },
        { "DES-CBC", "DES-CBC" },
        { "RC2-CBC", "RC2-CBC" },
        { "DES-EDE-CBC", "DES-EDE-CBC" },
        { "DES-EDE3-CBC", "DES-EDE3-CBC" },
        { "DESX-CBC", "DESX-CBC" },
        { "BF-CBC", "BF-CBC" },
        { "RC2-40-CBC", "RC2-40-CBC" },
        { "CAST5-CBC", "CAST5-CBC" },
        { "RC2-64-CBC", "RC2-64-CBC" },
        { "AES-128-CBC", "AES-128-CBC" },
        { "AES-192-CBC", "AES-192-CBC" },
        { "AES-256-CBC", "AES-256-CBC" },
        { "CAMELLIA-128-CBC", "CAMELLIA-128-CBC" },
        { "CAMELLIA-192-CBC", "CAMELLIA-192-CBC" },
        { "CAMELLIA-256-CBC", "CAMELLIA-256-CBC" },
        { "SEED-CBC", "SEED-CBC" }
    };

    HMACStrMap = {
        { tr("Default"), "default" },
        { tr("None"), "none" },
        { "RSA MD-4", "RSA-MD4" },
        { "MD-5", "MD5" },
        { "SHA-1", "SHA1" },
        { "SHA-224", "SHA224" },
        { "SHA-256", "SHA256" },
        { "SHA-384", "SHA384" },
        { "SHA-512", "SHA512" },
        { "RIPEMD-160", "RIPEMD160" },
    };
}

void VpnSecOpenVPNSection::initUI()
{
    m_cipherChooser->setTitle(tr("Cipher"));
    m_currentCipher = "default";
    QString curCipherOption = CipherStrMap.at(0).first;
    for (auto it = CipherStrMap.cbegin(); it != CipherStrMap.cend(); ++it) {
        m_cipherChooser->comboBox()->addItem(it->first, it->second);
        if (it->second == m_dataMap.value("cipher")) {
            m_currentCipher = it->second;
            curCipherOption = it->first;
        }
    }

    m_cipherChooser->setCurrentText(curCipherOption);

    m_hmacChooser->setTitle(tr("HMAC Auth"));
    m_currentHMAC = "default";
    QString curHMACOption = HMACStrMap.at(0).first;
    for (auto it = HMACStrMap.cbegin(); it != HMACStrMap.cend(); ++it) {
        m_hmacChooser->comboBox()->addItem(it->first, it->second);
        if (it->second == m_dataMap.value("auth")) {
            m_currentHMAC = it->second;
            curHMACOption = it->first;
        }
    }
    m_hmacChooser->setCurrentText(curHMACOption);

    appendItem(m_cipherChooser);
    appendItem(m_hmacChooser);
}

void VpnSecOpenVPNSection::initConnection()
{
    connect(m_cipherChooser, &ComboxWidget::onSelectChanged, this, [ = ](const QString &dataSelected) {
        for (auto it = CipherStrMap.cbegin(); it != CipherStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                m_currentCipher = it->second;
                break;
            }
        }
    });
    connect(m_hmacChooser, &ComboxWidget::onSelectChanged, this, [ = ](const QString &dataSelected) {
        for (auto it = HMACStrMap.cbegin(); it != HMACStrMap.cend(); ++it) {
            if (it->first == dataSelected) {
                m_currentHMAC = it->second;
                break;
            }
        }
    });

    connect(m_cipherChooser, &ComboxWidget::onIndexChanged, this, &VpnSecOpenVPNSection::editClicked);
    connect(m_hmacChooser, &ComboxWidget::onIndexChanged, this, &VpnSecOpenVPNSection::editClicked);
}
