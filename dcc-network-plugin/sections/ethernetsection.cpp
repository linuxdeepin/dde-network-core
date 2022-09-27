// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ethernetsection.h"

#include <DSpinBox>

#include <QIntValidator>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/wireddevice.h>

#include <widgets/contentwidget.h>
#include <widgets/comboxwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/optionitem.h>
#include <widgets/switchwidget.h>
#include <widgets/spinboxwidget.h>

#define NotBindValue "NotBind"

using namespace dcc::widgets;
using namespace NetworkManager;

#define DEVICE_INTERFACE_FLAG_UP 0x1

EthernetSection::EthernetSection(WiredSetting::Ptr wiredSetting, bool deviceAllowEmpty, QString devPath, QFrame *parent)
    : AbstractSection(tr("Ethernet"), parent)
    , m_deviceMacLine(new ComboxWidget(this))
    , m_clonedMac(new LineEditWidget(this))
    , m_customMtuSwitch(new SwitchWidget(this))
    , m_customMtu(new SpinBoxWidget(this))
    , m_wiredSetting(wiredSetting)
    , m_devicePath(devPath)
    , m_deviceAllowEmpty(deviceAllowEmpty)
{
    setAccessibleName("EthernetSection");
    // 获取所有有线网卡的Mac地址列表
    for (auto device : networkInterfaces()) {
        if (device->type() != Device::Ethernet)
            continue;

        WiredDevice::Ptr wDevice = device.staticCast<WiredDevice>();
        if (m_devicePath.isEmpty() || m_devicePath == "/") {
            if (!wDevice->managed()
#ifdef USE_DEEPIN_NMQT
                    || !(wDevice->interfaceFlags() & DEVICE_INTERFACE_FLAG_UP)
#endif
                    )
                continue;
        } else {
            if (wDevice->uni() != m_devicePath)
                continue;
        }

        /* Alt:  permanentHardwareAddress to get real hardware address which is connot be changed */
        QString mac = wDevice->permanentHardwareAddress();
        if (mac.isEmpty())
            mac = wDevice->hardwareAddress();

        const QString &macStr = mac + " (" + wDevice->interfaceName() + ")";
        m_macStrMap.insert(macStr, mac.remove(":"));
    }

    if (deviceAllowEmpty)
        m_macStrMap.insert(tr("Not Bind"), NotBindValue);

    m_macAddrRegExp = QRegExp("^([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$");

    initUI();
    initConnection();
}

EthernetSection::~EthernetSection()
{
    m_clonedMac->textEdit()->disconnect();
}

bool EthernetSection::allInputValid()
{
    if (!m_deviceAllowEmpty && m_deviceMacComboBox->currentIndex() < 0) {
        m_deviceMacComboBox->setProperty("isWarning", true);
        m_deviceMacComboBox->setFocus();
        return false;
    }

    const QString &clonedMacStr = m_clonedMac->text();
    if (clonedMacStr.isEmpty())
        return true;

    bool matched = m_macAddrRegExp.exactMatch(clonedMacStr);
    m_clonedMac->setIsErr(!matched);
    return matched;
}

void EthernetSection::saveSettings()
{
    QString hwAddr = m_macStrMap.value(m_deviceMacComboBox->currentText());
    if (hwAddr == NotBindValue)
        hwAddr.clear();

    if (!hwAddr.isEmpty()) {
        for (auto device : networkInterfaces()) {
            if (device->type() != Device::Ethernet)
                continue;

            WiredDevice::Ptr wDevice = device.staticCast<WiredDevice>();
            QString mac = wDevice->permanentHardwareAddress();
            if (mac.isEmpty())
                mac = wDevice->hardwareAddress();

            if (hwAddr == mac.remove(":"))
                m_devicePath = device->uni();
        }
    }

    m_wiredSetting->setMacAddress(QByteArray::fromHex(hwAddr.toUtf8()));

    QString clonedAddr = m_clonedMac->text().remove(":");
    m_wiredSetting->setClonedMacAddress(QByteArray::fromHex(clonedAddr.toUtf8()));

    m_wiredSetting->setMtu(m_customMtuSwitch->checked() ? static_cast<unsigned int>(m_customMtu->spinBox()->value()) : 0);

    m_wiredSetting->setInitialized(true);
}

void EthernetSection::initUI()
{
    m_deviceMacLine->setTitle(tr("Device MAC Addr"));
    m_deviceMacComboBox = m_deviceMacLine->comboBox();
    for (const QString &key : m_macStrMap.keys())
        m_deviceMacComboBox->addItem(key, m_macStrMap.value(key));

    // get the macAddress from existing Settings
    const QString &macAddr = QString(m_wiredSetting->macAddress().toHex()).toUpper();

    if (m_macStrMap.values().contains(macAddr)) {
        m_deviceMacComboBox->setCurrentIndex(m_deviceMacComboBox->findData(macAddr));
    } else if (!m_deviceAllowEmpty) {
        if (m_deviceMacComboBox->count() > 0)
            m_deviceMacComboBox->setCurrentIndex(0);
    } else {
        m_deviceMacComboBox->setCurrentIndex(m_deviceMacComboBox->findData(NotBindValue));
    }

    m_clonedMac->setTitle(tr("Cloned MAC Addr"));
    QString tmp = QString(m_wiredSetting->clonedMacAddress().toHex()).toUpper();
    QString clonedMacAddr;
    if (!tmp.isEmpty()) {
        for (int i = 0; i < tmp.size(); ++i) {
            if (i != 0 && i % 2 == 0)
                clonedMacAddr.append(":");

            clonedMacAddr.append(tmp.at(i));
        }
    }

    m_clonedMac->setText(clonedMacAddr);

    m_customMtuSwitch->setTitle(tr("Customize MTU"));
    m_customMtuSwitch->setChecked(!(m_wiredSetting->mtu() == 0));

    m_customMtu->setTitle(tr("MTU"));
    m_customMtu->spinBox()->setMinimum(0);
    m_customMtu->spinBox()->setMaximum(10000);
    m_customMtu->spinBox()->setValue(static_cast<int>(m_wiredSetting->mtu()));
    connect(m_customMtu->spinBox()->lineEdit(), &QLineEdit::textEdited, this, [this] (const QString &str) {
        if (str.contains("+"))
            m_customMtu->spinBox()->lineEdit()->clear();
    });

    onCostomMtuChanged(m_customMtuSwitch->checked());

    appendItem(m_deviceMacLine);
    appendItem(m_clonedMac);
    appendItem(m_customMtuSwitch);
    appendItem(m_customMtu);

    m_clonedMac->textEdit()->installEventFilter(this);
    m_customMtu->spinBox()->installEventFilter(this);
}

void EthernetSection::initConnection()
{
    connect(m_clonedMac->textEdit(), &QLineEdit::editingFinished, this, &EthernetSection::allInputValid);
    connect(m_customMtuSwitch, &SwitchWidget::checkedChanged, this, &EthernetSection::onCostomMtuChanged);
    connect(m_deviceMacComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &EthernetSection::editClicked);
    connect(m_deviceMacLine, &ComboxWidget::onIndexChanged, this, &EthernetSection::editClicked);
    connect(m_customMtuSwitch, &SwitchWidget::checkedChanged, this, &EthernetSection::editClicked);
    connect(m_customMtu->spinBox(), static_cast<void (DSpinBox::*)(int)>(&DSpinBox::valueChanged), this, &EthernetSection::editClicked);
}

void EthernetSection::onCostomMtuChanged(const bool enable)
{
    m_customMtu->setVisible(enable);
}

bool EthernetSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched) || dynamic_cast<DSpinBox *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}

QString EthernetSection::devicePath() const
{
    return m_devicePath;
}
