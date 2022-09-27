// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ipvxsection.h"
#include "widgets/contentwidget.h"
#include "netutils.h"

#include <DSpinBox>

#include <QDBusMetaType>
#include <QComboBox>

#include <widgets/comboxwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/spinboxwidget.h>
#include <widgets/switchwidget.h>

#include <com_deepin_daemon_network.h>
#include <org_freedesktop_notifications.h>

Q_DECLARE_METATYPE(Ipv4Setting::ConfigMethod)
Q_DECLARE_METATYPE(Ipv6Setting::ConfigMethod)

const unsigned int ipConflictCheckTime = 500;

using Notifications = org::freedesktop::Notifications;

using namespace dcc::widgets;
using namespace dde::network;
using namespace NetworkManager;

IpvxSection::IpvxSection(Ipv4Setting::Ptr ipv4Setting, QFrame *parent)
    : AbstractSection(tr("IPv4"), parent)
    , m_methodLine(new ComboxWidget(this))
    , m_ipAddress(new LineEditWidget(this))
    , m_netmaskIpv4(new LineEditWidget(this))
    , m_prefixIpv6(nullptr)
    , m_gateway(new LineEditWidget(this))
    , m_neverDefault(new SwitchWidget(this))
    , m_currentIpvx(Ipv4)
    , m_ipvxSetting(ipv4Setting)
{
    initStrMaps();
    initUI();
    initConnection();

    onIpv4MethodChanged(Ipv4ConfigMethodStrMap.value(m_methodChooser->currentText()));
}

IpvxSection::IpvxSection(Ipv6Setting::Ptr ipv6Setting, QFrame *parent)
    : AbstractSection(tr("IPv6"), parent)
    , m_methodLine(new ComboxWidget(this))
    , m_ipAddress(new LineEditWidget(this))
    , m_netmaskIpv4(nullptr)
    , m_prefixIpv6(new SpinBoxWidget(this))
    , m_gateway(new LineEditWidget(this))
    , m_neverDefault(new SwitchWidget(this))
    , m_currentIpvx(Ipv6)
    , m_ipvxSetting(ipv6Setting)
{
    qDBusRegisterMetaType<IpV6DBusAddress>();
    qDBusRegisterMetaType<IpV6DBusAddressList>();
    initStrMaps();
    initUI();
    initConnection();

    onIpv6MethodChanged(Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText()));
}

IpvxSection::~IpvxSection()
{
    m_ipAddress->textEdit()->disconnect();
}

bool IpvxSection::allInputValid()
{
    bool valid = true;

    switch (m_currentIpvx) {
    case Ipv4:
        valid = ipv4InputIsValid();
        break;
    case Ipv6:
        valid = ipv6InputIsValid();
        break;
    }

    return valid;
}

void IpvxSection::saveSettings()
{
    bool initialized = true;

    switch (m_currentIpvx) {
    case Ipv4:
        initialized = saveIpv4Settings();
        break;
    case Ipv6:
        initialized = saveIpv6Settings();
        break;
    }

    m_ipvxSetting->setInitialized(initialized);
}

bool IpvxSection::saveIpv4Settings()
{
    Ipv4Setting::Ptr ipv4Setting = m_ipvxSetting.staticCast<Ipv4Setting>();

    Ipv4Setting::ConfigMethod method = Ipv4ConfigMethodStrMap.value(m_methodChooser->currentText());
    ipv4Setting->setMethod(method);

    if (method == Ipv4Setting::Manual) {
        IpAddress ipAddress;
        ipAddress.setIp(QHostAddress(m_ipAddress->text()));
        ipAddress.setNetmask(QHostAddress(m_netmaskIpv4->text()));
        ipAddress.setGateway(QHostAddress(m_gateway->text()));
        ipv4Setting->setAddresses(QList<IpAddress>() << ipAddress);
    }

    if (method == Ipv4Setting::Automatic) {
        QList<IpAddress>().clear();
        IpAddress ipAddressAuto;
        ipAddressAuto.setIp(QHostAddress(""));
        ipAddressAuto.setNetmask(QHostAddress(""));
        ipAddressAuto.setGateway(QHostAddress(""));
        ipv4Setting->setAddresses(QList<IpAddress>() << ipAddressAuto);
    }

    if (m_neverDefault->isVisible())
        ipv4Setting->setNeverDefault(m_neverDefault->checked());

    return true;
}

bool IpvxSection::saveIpv6Settings()
{
    Ipv6Setting::Ptr ipv6Setting = m_ipvxSetting.staticCast<Ipv6Setting>();

    Ipv6Setting::ConfigMethod method = Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText());
    ipv6Setting->setMethod(Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText()));

    if (method == Ipv6Setting::Ignored) {
        ipv6Setting->setAddresses(QList<IpAddress>());
        return true;
    }

    if (method == Ipv6Setting::Manual) {
        IpAddress ipAddress;
        ipAddress.setIp(QHostAddress(m_ipAddress->text()));
        ipAddress.setPrefixLength(m_prefixIpv6->spinBox()->value());
        ipAddress.setGateway(QHostAddress(m_gateway->text()));
        ipv6Setting->setAddresses(QList<IpAddress>() << ipAddress);
    }

    if (method == Ipv6Setting::Automatic) {
        QList<IpAddress> ipAddresses;
        ipAddresses.clear();
        IpAddress ipAddressAuto;
        ipAddressAuto.setIp(QHostAddress(""));
        ipAddressAuto.setPrefixLength(0);
        ipAddressAuto.setGateway(QHostAddress(""));
        ipAddresses.append(ipAddressAuto);
        ipv6Setting->setAddresses(ipAddresses);
    }

    if (m_neverDefault->isVisible())
        ipv6Setting->setNeverDefault(m_neverDefault->checked());

    return true;
}

void IpvxSection::setIpv4ConfigMethodEnable(Ipv4Setting::ConfigMethod method, const bool enabled)
{
    if (!Ipv4ConfigMethodStrMap.values().contains(method))
        return;

    if (enabled)
        m_methodChooser->addItem(Ipv4ConfigMethodStrMap.key(method), method);
    else
        m_methodChooser->removeItem(m_methodChooser->findData(method));
}

void IpvxSection::setIpv6ConfigMethodEnable(Ipv6Setting::ConfigMethod method, const bool enabled)
{
    if (!Ipv6ConfigMethodStrMap.values().contains(method))
        return;

    if (enabled)
        m_methodChooser->addItem(Ipv6ConfigMethodStrMap.key(method), method);
    else
        m_methodChooser->removeItem(m_methodChooser->findData(method));
}

void IpvxSection::setNeverDefaultEnable(const bool neverDefault)
{
    m_neverDefault->setVisible(neverDefault);
}

void IpvxSection::initStrMaps()
{
    Ipv4ConfigMethodStrMap = {
        { tr("Auto"), Ipv4Setting::ConfigMethod::Automatic },
        { tr("Manual"), Ipv4Setting::ConfigMethod::Manual }
    };

    Ipv6ConfigMethodStrMap = {
        { tr("Auto"), Ipv6Setting::ConfigMethod::Automatic },
        { tr("Manual"), Ipv6Setting::ConfigMethod::Manual },
        { tr("Ignore"), Ipv6Setting::ConfigMethod::Ignored }
    };
}

void IpvxSection::initUI()
{
    setAccessibleName("IpvxSection");
    m_ipAddress->setTitle(tr("IP Address"));
    m_ipAddress->textEdit()->setPlaceholderText(tr("Required"));
    m_gateway->setTitle(tr("Gateway"));
    m_neverDefault->setTitle(tr("Only applied in corresponding resources"));
    m_neverDefault->setVisible(false);

    m_methodChooser = m_methodLine->comboBox();
    m_methodLine->setTitle(tr("Method"));
    appendItem(m_methodLine);
    appendItem(m_ipAddress);

    switch (m_currentIpvx) {
    case Ipv4:
        initForIpv4();
        break;
    case Ipv6:
        initForIpv6();
        break;
    }

    appendItem(m_gateway);
    appendItem(m_neverDefault);

    m_ipAddress->textEdit()->installEventFilter(this);
    m_gateway->textEdit()->installEventFilter(this);
    if (m_netmaskIpv4)
        m_netmaskIpv4->textEdit()->installEventFilter(this);

    if (m_prefixIpv6)
        m_prefixIpv6->spinBox()->installEventFilter(this);
}

void IpvxSection::initForIpv4()
{
    Ipv4Setting::Ptr ipv4Setting = m_ipvxSetting.staticCast<Ipv4Setting>();

    for (const QString &key : Ipv4ConfigMethodStrMap.keys())
        m_methodChooser->addItem(key, Ipv4ConfigMethodStrMap.value(key));

    if (Ipv4ConfigMethodStrMap.values().contains(ipv4Setting->method()))
        m_methodChooser->setCurrentIndex(m_methodChooser->findData(ipv4Setting->method()));
    else
        m_methodChooser->setCurrentIndex(m_methodChooser->findData(Ipv4ConfigMethodStrMap.first()));

    QList<IpAddress> ipAddressList = ipv4Setting->addresses();
    if (!ipAddressList.isEmpty()) {
        // !! use the first ipaddress of list
        IpAddress ipAddress = ipAddressList.first();
        m_ipAddress->setText(ipAddress.ip().toString());
        m_netmaskIpv4->setText(ipAddress.netmask().toString());
        const QString &gateStr = ipAddress.gateway().toString();
        m_gateway->setText(isIpv4Address(gateStr) ? gateStr : "");
    } else {
        m_ipAddress->setText("0.0.0.0");
        m_netmaskIpv4->setText("255.255.255.0");
    }

    m_netmaskIpv4->setTitle(tr("Netmask"));
    m_netmaskIpv4->textEdit()->setPlaceholderText(tr("Required"));
    appendItem(m_netmaskIpv4);

    m_neverDefault->setChecked(ipv4Setting->neverDefault());

    onIpv4MethodChanged(Ipv4ConfigMethodStrMap.value(m_methodChooser->currentText()));
}

void IpvxSection::initForIpv6()
{
    m_prefixIpv6->setTitle(tr("Prefix"));
    m_prefixIpv6->spinBox()->setRange(1, 128);
    m_prefixIpv6->setDefaultVal(64);

    Ipv6Setting::Ptr ipv6Setting = m_ipvxSetting.staticCast<Ipv6Setting>();

    for (const QString &key : Ipv6ConfigMethodStrMap.keys())
        m_methodChooser->addItem(key, Ipv6ConfigMethodStrMap.value(key));

    if (Ipv6ConfigMethodStrMap.values().contains(ipv6Setting->method()))
        m_methodChooser->setCurrentIndex(m_methodChooser->findData(ipv6Setting->method()));
    else
        m_methodChooser->setCurrentIndex(m_methodChooser->findData(Ipv6ConfigMethodStrMap.first()));

    QList<IpAddress> ipAddressList = ipv6Setting->addresses();
    if (!ipAddressList.isEmpty()) {
        // !! use the first ipaddress of list
        IpAddress ipAddress = ipAddressList.first();
        m_ipAddress->setText(ipAddress.ip().toString());
        m_prefixIpv6->spinBox()->setValue(ipAddress.prefixLength());
        const QString &gateStr = ipAddress.gateway().toString();
        m_gateway->setText(isIpv6Address(gateStr) ? gateStr : "");
    }

    appendItem(m_prefixIpv6);

    m_neverDefault->setChecked(ipv6Setting->neverDefault());

    onIpv6MethodChanged(Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText()));
}

void IpvxSection::initConnection()
{
    connect(m_ipAddress->textEdit(), &QLineEdit::textChanged, this, [this] {
        if (!m_ipAddress->text().isEmpty())
            m_ipAddress->dTextEdit()->setAlert(false);
    });

    connect(m_ipAddress->textEdit(), &QLineEdit::selectionChanged, this, [this] {
        m_ipAddress->textEdit()->setFocus();
    });

    switch (m_currentIpvx) {
    case Ipv4:
        connect(m_methodChooser, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [ = ] {
            onIpv4MethodChanged(m_methodChooser->currentData().value<Ipv4Setting::ConfigMethod>());
        });
        connect(m_netmaskIpv4->textEdit(), &QLineEdit::textChanged, this, [this] {
            if (!m_netmaskIpv4->text().isEmpty())
                m_netmaskIpv4->dTextEdit()->setAlert(false);
        });
        break;
    case Ipv6:
        connect(m_methodChooser, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [ = ] {
            onIpv6MethodChanged(m_methodChooser->currentData().value<Ipv6Setting::ConfigMethod>());
        });
        break;
    }

    connect(m_neverDefault, &SwitchWidget::checkedChanged, this, &IpvxSection::editClicked);
    connect(m_methodChooser, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &IpvxSection::editClicked);
    connect(m_methodLine, &ComboxWidget::onIndexChanged, this, &IpvxSection::editClicked);
    if (m_prefixIpv6)
        connect(m_prefixIpv6->spinBox(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &IpvxSection::editClicked);
}

void IpvxSection::onIpv4MethodChanged(Ipv4Setting::ConfigMethod method)
{
    switch (method) {
    case Ipv4Setting::Automatic:
        m_ipAddress->setVisible(false);
        m_netmaskIpv4->setVisible(false);
        m_gateway->setVisible(false);
        break;
    case Ipv4Setting::Manual:
        m_ipAddress->setVisible(true);
        m_netmaskIpv4->setVisible(true);
        m_gateway->setVisible(true);
        break;
    default:
        break;
    }
}

void IpvxSection::onIpv6MethodChanged(Ipv6Setting::ConfigMethod method)
{
    switch (method) {
    case Ipv6Setting::Automatic:
        m_ipAddress->setVisible(false);
        m_prefixIpv6->setVisible(false);
        m_gateway->setVisible(false);
        break;
    case Ipv6Setting::Manual:
        m_ipAddress->setVisible(true);
        m_prefixIpv6->setVisible(true);
        m_gateway->setVisible(true);
        break;
    case Ipv6Setting::Ignored:
        m_ipAddress->setVisible(false);
        m_prefixIpv6->setVisible(false);
        m_gateway->setVisible(false);
        break;
    default:
        break;
    }
}

bool IpvxSection::ipv4InputIsValid()
{
    bool valid = true;

    if (Ipv4ConfigMethodStrMap.value(m_methodChooser->currentText()) == Ipv4Setting::Manual) {
        const QString &ip = m_ipAddress->text();
        if (m_ipAddress->text().isEmpty())
            m_ipAddress->dTextEdit()->setAlert(true);

        if (!isIpv4Address(ip)) {
            valid = false;
            m_ipAddress->setIsErr(true);
            m_ipAddress->dTextEdit()->showAlertMessage(tr("Invalid IP address"), m_ipAddress, 2000);
        } else {
            m_ipAddress->setIsErr(false);
        }

        const QString &netmask = m_netmaskIpv4->text();
        if (m_netmaskIpv4->text().isEmpty())
            m_netmaskIpv4->dTextEdit()->setAlert(true);

        if (!isIpv4SubnetMask(netmask)) {
            valid = false;
            m_netmaskIpv4->setIsErr(true);
            m_netmaskIpv4->dTextEdit()->showAlertMessage(tr("Invalid netmask"), m_netmaskIpv4, 2000);
        } else {
            m_netmaskIpv4->setIsErr(false);
        }

        const QString &gateway = m_gateway->text();
        if (!gateway.isEmpty() && !isIpv4Address(gateway)) {
            valid = false;
            m_gateway->setIsErr(true);
            m_gateway->dTextEdit()->showAlertMessage(tr("Invalid gateway"), parentWidget(), 2000);
        } else {
            m_gateway->setIsErr(false);
        }

        bool isIPConflict = false;
        const QString strCurrentIP = m_ipAddress->text();
        NetworkInter inter("com.deepin.daemon.Network", "/com/deepin/daemon/Network", QDBusConnection::sessionBus());
        inter.RequestIPConflictCheck(ip, "");
        connect(&inter, &NetworkInter::IPConflict, this, [&strCurrentIP,&isIPConflict] (const QString &strIP, const QString &strMac) {
            if (!strMac.isEmpty() && strIP == strCurrentIP && !isIPConflict) {
                Notifications notifications("org.freedesktop.Notifications", "/org/freedesktop/Notifications", QDBusConnection::sessionBus());
                notifications.Notify("dde-control-center", static_cast<uint>(QDateTime::currentMSecsSinceEpoch()), "preferences-system", tr("Network"), tr("IP conflict"), QStringList(), QVariantMap(), 3000);
            }
            isIPConflict = true;
        });

        QElapsedTimer et;
        et.start();
        while (!isIPConflict && et.elapsed() < ipConflictCheckTime) {
            QThread::msleep(50);
            QCoreApplication::sendPostedEvents(&inter);
        }
    }

    return valid;
}

bool IpvxSection::ipv6InputIsValid()
{
    bool valid = true;

    if (Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText()) == Ipv6Setting::Ignored)
        return valid;

    if (Ipv6ConfigMethodStrMap.value(m_methodChooser->currentText()) == Ipv6Setting::Manual) {
        const QString &ip = m_ipAddress->text();
        if (m_ipAddress->text().isEmpty())
            m_ipAddress->dTextEdit()->setAlert(true);

        if (!isIpv6Address(ip)) {
            valid = false;
            m_ipAddress->setIsErr(true);
            m_ipAddress->dTextEdit()->showAlertMessage(tr("Invalid IP address"), m_ipAddress, 2000);
        } else {
            m_ipAddress->setIsErr(false);
        }

        if (m_prefixIpv6->spinBox()->value() == 0) {
            valid = false;
            m_prefixIpv6->setIsErr(true);
        } else {
            m_prefixIpv6->setIsErr(false);
        }

        const QString &gateway = m_gateway->text();
        if (!gateway.isEmpty() && !isIpv6Address(gateway)) {
            valid = false;
            m_gateway->setIsErr(true);
            m_gateway->dTextEdit()->showAlertMessage(tr("Invalid gateway"), parentWidget(), 2000);
        } else {
            m_gateway->setIsErr(false);
        }
    }

    return valid;
}

bool IpvxSection::isIpv4Address(const QString &ip)
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv4)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
        return false;
    }

    QRegExp regExpIP("((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])[\\.]){3}(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])");
    return regExpIP.exactMatch(ip);
}

bool IpvxSection::isIpv6Address(const QString &ip)
{
    QHostAddress ipAddr(ip);
    if (ipAddr == QHostAddress(QHostAddress::Null) || ipAddr == QHostAddress(QHostAddress::AnyIPv6)
            || ipAddr.protocol() != QAbstractSocket::NetworkLayerProtocol::IPv6Protocol) {
        return false;
    }

    return (ipAddr != QHostAddress(QHostAddress::LocalHostIPv6));
}

bool IpvxSection::isIpv4SubnetMask(const QString &ip)
{
    bool done;
    quint32 mask = QHostAddress(ip).toIPv4Address(&done);

    if (!done)
        return false;

    for (; mask != 0; mask <<= 1) {
        if ((mask & (static_cast<uint>(1) << 31)) == 0)
            return false; // Highest bit is now zero, but mask is non-zero.
    }

    QRegExp regExpIP("^((128|192)|2(24|4[08]|5[245]))(\\.(0|(128|192)|2((24)|(4[08])|(5[245])))){3}$");
    return regExpIP.exactMatch(ip);
}

bool IpvxSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched) || dynamic_cast<QSpinBox *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
