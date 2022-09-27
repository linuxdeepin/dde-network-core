// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wirelesssection.h"

#include <DSpinBox>

#include <QComboBox>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/wireddevice.h>

#include <widgets/contentwidget.h>
#include <widgets/comboxwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/optionitem.h>
#include <widgets/switchwidget.h>
#include <widgets/spinboxwidget.h>

#include <networkmanagerqt/wirelessdevice.h>

#define NotBindValue "NotBind"

using namespace dcc::widgets;
using namespace NetworkManager;

WirelessSection::WirelessSection(ConnectionSettings::Ptr connSettings, WirelessSetting::Ptr wiredSetting, QString devPath, bool isHotSpot, QFrame *parent)
    : AbstractSection(tr("WLAN"), parent)
    , m_apSsid(new LineEditWidget(this))
    , m_deviceMacLine(new ComboxWidget(this))
    , m_customMtuSwitch(new SwitchWidget(this))
    , m_customMtu(new SpinBoxWidget(this))
    , m_connSettings(connSettings)
    , m_wirelessSetting(wiredSetting)
{
    // get the macAddress list from all wireless devices
    for (auto device : networkInterfaces()) {
        if (device->type() != Device::Type::Wifi
            || (!devPath.isEmpty() && devPath != "/" && device->uni() != devPath))
            continue;

        WirelessDevice::Ptr wDevice = device.staticCast<WirelessDevice>();
        WirelessDevice::Capabilities cap = wDevice->wirelessCapabilities();
        if (isHotSpot && !cap.testFlag(WirelessDevice::ApCap))
            continue;

        /* Alt:  permanentHardwareAddress to get real hardware address which is connot be changed */
        const QString &macStr = wDevice->permanentHardwareAddress() + " (" + wDevice->interfaceName() + ")";
        m_macStrMap.insert(macStr, { wDevice->permanentHardwareAddress().remove(":"), wDevice->interfaceName() });
    }

    m_macStrMap.insert(tr("Not Bind"), { NotBindValue, QString() });

    // "^([0-9A-Fa-f]{2}[:-\\.]){5}([0-9A-Fa-f]{2})$"
    m_macAddrRegExp = QRegExp("^([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$");

    initUI();
    initConnection();
}

WirelessSection::~WirelessSection()
{
}

bool WirelessSection::allInputValid()
{
    bool valid = true;

    // available length is [1-32]
    const int length = m_apSsid->text().toUtf8().length();
    if (length > 0 && length < 33) {
        m_apSsid->setIsErr(false);
    } else {
        valid = false;
        m_apSsid->setIsErr(true);
    }

    return valid;
}

void WirelessSection::saveSettings()
{
    m_wirelessSetting->setSsid(m_apSsid->text().toUtf8());

    const QPair<QString, QString> &pair = m_macStrMap.value(m_deviceMacComboBox->currentText());
    QString hwAddr = pair.first;
    if (hwAddr == NotBindValue)
        hwAddr.clear();

    m_wirelessSetting->setMacAddress(QByteArray::fromHex(hwAddr.toUtf8()));

    //QString clonedAddr = m_clonedMac->text().remove(":");
    //m_wirelessSetting->setClonedMacAddress(QByteArray::fromHex(clonedAddr.toUtf8()));

    m_wirelessSetting->setMtu(m_customMtuSwitch->checked() ? static_cast<unsigned int>(m_customMtu->spinBox()->value()) : 0);

    m_wirelessSetting->setInitialized(true);
    if (hwAddr.isEmpty())
        m_connSettings->setInterfaceName(QString());
    else
        m_connSettings->setInterfaceName(pair.second);
}

void WirelessSection::setSsidEditable(const bool editable)
{
    m_apSsid->textEdit()->setClearButtonEnabled(editable);
    m_apSsid->textEdit()->setEnabled(editable);
}

bool WirelessSection::ssidIsEditable() const
{
    return m_apSsid->isEnabled();
}

const QString WirelessSection::ssid() const
{
    return m_apSsid->text();
}

void WirelessSection::setSsid(const QString &ssid)
{
    m_apSsid->setText(ssid);
}

void WirelessSection::initUI()
{
    m_apSsid->setTitle(tr("SSID"));
    m_apSsid->setPlaceholderText(tr("Required"));
    m_apSsid->setText(m_wirelessSetting->ssid());
    m_apSsid->textEdit()->setMaxLength(256);

    m_deviceMacLine->setTitle(tr("Device MAC Addr"));
    m_deviceMacComboBox = m_deviceMacLine->comboBox();
    for (const QString &key : m_macStrMap.keys())
        m_deviceMacComboBox->addItem(key, m_macStrMap.value(key).first);

    // get the macAddress from Settings
    const QString &macAddr = QString(m_wirelessSetting->macAddress().toHex()).toUpper();

    if (std::any_of(m_macStrMap.cbegin(), m_macStrMap.cend(), [macAddr](const QPair<QString, QString> &it) {
            return it.first == macAddr;
        }))
        m_deviceMacComboBox->setCurrentIndex(m_deviceMacComboBox->findData(macAddr));
    else
        m_deviceMacComboBox->setCurrentIndex(m_deviceMacComboBox->findData(NotBindValue));

    m_customMtuSwitch->setTitle(tr("Customize MTU"));
    m_customMtuSwitch->setChecked(!(m_wirelessSetting->mtu() == 0));

    m_customMtu->setTitle(tr("MTU"));
    m_customMtu->spinBox()->setMinimum(0);
    m_customMtu->spinBox()->setMaximum(10000);
    m_customMtu->spinBox()->setValue(static_cast<int>(m_wirelessSetting->mtu()));
    onCostomMtuChanged(m_customMtuSwitch->checked());

    appendItem(m_apSsid);
    appendItem(m_deviceMacLine);
    appendItem(m_customMtuSwitch);
    appendItem(m_customMtu);

    m_apSsid->textEdit()->installEventFilter(this);
    m_customMtu->spinBox()->installEventFilter(this);
}

void WirelessSection::initConnection()
{
    //connect(m_clonedMac->textEdit(), &QLineEdit::editingFinished, this, &WirelessSection::allInputValid);
    connect(m_customMtuSwitch, &SwitchWidget::checkedChanged, this, &WirelessSection::onCostomMtuChanged);
    connect(m_apSsid->textEdit(), &QLineEdit::textChanged, this, &WirelessSection::ssidChanged);
    connect(m_deviceMacComboBox, static_cast<void (QComboBox:: *)(int)>(&QComboBox::currentIndexChanged), this, &WirelessSection::editClicked);
    connect(m_customMtuSwitch, &SwitchWidget::checkedChanged, this, &WirelessSection::editClicked);
    connect(m_customMtu->spinBox(), static_cast<void (QSpinBox:: *)(int)>(&QSpinBox::valueChanged), this, &WirelessSection::editClicked);
}

void WirelessSection::onCostomMtuChanged(const bool enable)
{
    m_customMtu->setVisible(enable);
}

bool WirelessSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched) || dynamic_cast<QSpinBox *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
