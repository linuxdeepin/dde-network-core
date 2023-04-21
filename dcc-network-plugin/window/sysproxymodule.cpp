// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sysproxymodule.h"

#include <widgets/buttontuple.h>
#include <widgets/comboxwidget.h>
#include <widgets/lineeditwidget.h>
#include <widgets/settingsgroup.h>
#include <widgets/switchwidget.h>
#include <widgets/widgetmodule.h>
#include <widgets/settingsgroupmodule.h>
#include <widgets/itemmodule.h>
#include <DFloatingButton>
#include <DFontSizeManager>
#include <DListView>
#include <DSwitchButton>
#include <DTextEdit>

#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include <networkcontroller.h>
#include <proxycontroller.h>

using namespace dde::network;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE
// 与m_ProxyMethodList对应
#define ProxyMethodIndex_Manual 0
#define ProxyMethodIndex_Auto 1

SysProxyModule::SysProxyModule(QObject *parent)
    : PageModule("systemProxy", tr("System Proxy"), tr("System Proxy"), QIcon::fromTheme("dcc_system_agent"), parent)
    , m_ProxyMethodList({ tr("Manual"), tr("Auto") })
    , m_uiMethod(dde::network::ProxyMethod::Init)
{
    auto initProxyGroup = [this] (LineEditWidget *&proxyEdit, LineEditWidget *&portEdit, const QString &proxyTitle) -> SettingsGroupModule* {
        SettingsGroupModule *module = new SettingsGroupModule("","");
        module->appendChild(new ItemModule("","", [&proxyEdit, this, proxyTitle](ModuleObject *module) {
            proxyEdit = new LineEditWidget;
            proxyEdit->setPlaceholderText(tr("Optional"));
            proxyEdit->setTitle(proxyTitle);
            proxyEdit->textEdit()->installEventFilter(this);
            return proxyEdit;
        },false));
        module->appendChild(new ItemModule("","", [&portEdit, this](ModuleObject *module) {
            portEdit = new LineEditWidget;
            portEdit->setPlaceholderText(tr("Optional"));
            portEdit->setTitle(tr("Port"));
            portEdit->textEdit()->installEventFilter(this);
            connect(portEdit->textEdit(), &QLineEdit::textChanged, this, [=](const QString &str) {
                if (str.toInt() < 0) {
                    portEdit->setText("0");
                } else if (str.toInt() > 65535) {
                    portEdit->setText("65535");
                }
            });
            return portEdit;
        },false));

        return module;
    };
    appendChild(new WidgetModule<SwitchWidget>("system_proxy", tr("System Proxy"), [this](SwitchWidget *proxySwitch) {
        m_proxySwitch = proxySwitch;
        QLabel *lblTitle = new QLabel(tr("System Proxy"));
        DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
        proxySwitch->setLeftWidget(lblTitle);
        proxySwitch->switchButton()->setAccessibleName(lblTitle->text());

        auto updateSwitch = [proxySwitch]() {
            ProxyMethod method = NetworkController::instance()->proxyController()->proxyMethod();
            proxySwitch->blockSignals(true);
            proxySwitch->setChecked(method != ProxyMethod::None);
            proxySwitch->blockSignals(false);
        };
        updateSwitch();
        connect(NetworkController::instance()->proxyController(), &ProxyController::proxyMethodChanged, proxySwitch, updateSwitch);
        connect(proxySwitch, &SwitchWidget::checkedChanged, proxySwitch, [this](const bool checked) {
            m_buttonTuple->setEnabled(checked);
            if (checked) {
                // 打开代理默认手动
                uiMethodChanged(ProxyMethod::Manual);
            } else {
                // 关闭代理
                NetworkController::instance()->proxyController()->setProxyMethod(ProxyMethod::None);
                uiMethodChanged(ProxyMethod::None);
                m_buttonTuple->setVisible(false);
                m_buttonTuple->rightButton()->clicked();
            }
        });
    }));
    m_systemProxyTypeModule = new WidgetModule<ComboxWidget>("system_proxy_box", tr("System Proxy"), [this](ComboxWidget *proxyTypeBox) {
        m_proxyTypeBox = proxyTypeBox;
        proxyTypeBox->setTitle(tr("Proxy Type"));
        proxyTypeBox->addBackground();
        proxyTypeBox->comboBox()->addItems(m_ProxyMethodList);
        auto updateBox = [proxyTypeBox]() {
            ProxyMethod method = NetworkController::instance()->proxyController()->proxyMethod();
            proxyTypeBox->comboBox()->blockSignals(true);
            switch (method) {
            case ProxyMethod::Auto:
                proxyTypeBox->comboBox()->setCurrentIndex(ProxyMethodIndex_Auto);
                break;
            case ProxyMethod::Manual:
                proxyTypeBox->comboBox()->setCurrentIndex(ProxyMethodIndex_Manual);
                break;
            default:
                break;
            }
            proxyTypeBox->comboBox()->blockSignals(false);
        };
        updateBox();
        connect(NetworkController::instance()->proxyController(), &ProxyController::proxyMethodChanged, proxyTypeBox, updateBox);
        connect(proxyTypeBox->comboBox(), static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int index) {
            switch (index) {
            case ProxyMethodIndex_Auto:
                uiMethodChanged(ProxyMethod::Auto);
                break;
            case ProxyMethodIndex_Manual:
                uiMethodChanged(ProxyMethod::Manual);
                break;
            }
            m_buttonTuple->setEnabled(true);
        });
    });
    appendChild(m_systemProxyTypeModule);
    m_urlConfigureModule = new WidgetModule<SettingsGroup>("system_proxy_auto_group", QString(), [this](SettingsGroup *autoGroup) {
        m_autoUrl = new LineEditWidget;
        m_autoUrl->setPlaceholderText(tr("Optional"));
        m_autoUrl->setTitle(tr("Configuration URL"));
        m_autoUrl->textEdit()->installEventFilter(this);
        ProxyController *proxyController = NetworkController::instance()->proxyController();
        connect(proxyController, &ProxyController::autoProxyChanged, m_autoUrl, &LineEditWidget::setText);
        autoGroup->appendItem(m_autoUrl);
        resetData(ProxyMethod::Auto);
    });
    appendChild(m_urlConfigureModule);

    m_httpProxyModule = initProxyGroup(m_httpAddr, m_httpPort, tr("HTTP Proxy")) ;
    appendChild(m_httpProxyModule);

    m_httpsProxyModule = initProxyGroup(m_httpsAddr, m_httpsPort, tr("HTTPS Proxy"));
    appendChild(m_httpsProxyModule);
    m_ftpProxyModule = initProxyGroup(m_ftpAddr, m_ftpPort, tr("FTP Proxy"));
    appendChild(m_ftpProxyModule);
    m_socketsModule = initProxyGroup(m_socksAddr, m_socksPort, tr("SOCKS Proxy"));
    appendChild(m_socketsModule);
    m_editLineModule = new ItemModule("", "", [ this ](ModuleObject *module) {
        m_ignoreList = new DTextEdit;
        m_ignoreList->setAccessibleName("ProxyPage_ignoreList");
        m_ignoreList->installEventFilter(this);
        QLabel *ignoreTips = new QLabel;
        ignoreTips->setWordWrap(true);
        ignoreTips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        ignoreTips->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        ignoreTips->setText(tr("Ignore the proxy configurations for the above hosts and domains"));
        resetData(ProxyMethod::Manual);
        ProxyController *proxyController = NetworkController::instance()->proxyController();
        connect(proxyController, &ProxyController::proxyIgnoreHostsChanged, m_ignoreList, [this](const QString &hosts) {
            const QTextCursor cursor = m_ignoreList->textCursor();
            m_ignoreList->blockSignals(true);
            m_ignoreList->setPlainText(hosts);
            m_ignoreList->setTextCursor(cursor);
            m_ignoreList->blockSignals(false);
        });
        connect(proxyController, &ProxyController::proxyChanged, m_httpAddr, [this]() {
            resetData(ProxyMethod::Manual);
        });
        return m_ignoreList;
    },false);
    appendChild(m_editLineModule);

    ModuleObject *extra = new WidgetModule<ButtonTuple>("save", tr("Save", "button"), [this](ButtonTuple *buttonTuple) {
        m_buttonTuple = buttonTuple;
        m_buttonTuple->setButtonType(ButtonTuple::Save);
        m_buttonTuple->leftButton()->setText(tr("Cancel", "button"));
        m_buttonTuple->rightButton()->setText(tr("Save", "button"));
        m_buttonTuple->setVisible(NetworkController::instance()->proxyController()->proxyMethod() != ProxyMethod::None);
        m_buttonTuple->setEnabled(false);

        connect(m_buttonTuple->rightButton(), &QPushButton::clicked, this, &SysProxyModule::applySettings);
        connect(m_buttonTuple->leftButton(), &QPushButton::clicked, this, [this]() {
            m_buttonTuple->setEnabled(false);
            if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodIndex_Auto)
                resetData(ProxyMethod::Auto);
            else
                resetData(ProxyMethod::Manual);
        });
    });
    extra->setExtra();
    appendChild(extra);
    NetworkController::instance()->proxyController()->querySysProxyData();
    uiMethodChanged(NetworkController::instance()->proxyController()->proxyMethod());
}

void SysProxyModule::active()
{
    NetworkController::instance()->proxyController()->querySysProxyData();
    QTimer::singleShot(1, this, [this]() {
        uiMethodChanged(NetworkController::instance()->proxyController()->proxyMethod());
    });
}

void SysProxyModule::deactive()
{

}

void SysProxyModule::applySettings()
{
    ProxyController *proxyController = NetworkController::instance()->proxyController();
    m_buttonTuple->setEnabled(false);
    if (!m_proxySwitch->checked()) {
        proxyController->setProxyMethod(ProxyMethod::None);
    } else if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodIndex_Manual) {
        proxyController->setProxy(SysProxyType::Http, m_httpAddr->text(), m_httpPort->text());
        proxyController->setProxy(SysProxyType::Https, m_httpsAddr->text(), m_httpsPort->text());
        proxyController->setProxy(SysProxyType::Ftp, m_ftpAddr->text(), m_ftpPort->text());
        proxyController->setProxy(SysProxyType::Socks, m_socksAddr->text(), m_socksPort->text());
        proxyController->setProxyIgnoreHosts(m_ignoreList->toPlainText());
        proxyController->setProxyMethod(ProxyMethod::Manual);
    } else if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodIndex_Auto) {
        proxyController->setAutoProxy(m_autoUrl->text());
        proxyController->setProxyMethod(ProxyMethod::Auto);
    }
}

void SysProxyModule::uiMethodChanged(ProxyMethod uiMethod)
{
    if (!m_buttonTuple.isNull()) {
        m_urlConfigureModule->setVisible(uiMethod == ProxyMethod::Auto);

        m_httpProxyModule->setVisible(uiMethod == ProxyMethod::Manual);
        m_httpsProxyModule->setVisible(uiMethod == ProxyMethod::Manual);
        m_ftpProxyModule->setVisible(uiMethod == ProxyMethod::Manual);
        m_socketsModule->setVisible(uiMethod == ProxyMethod::Manual);
        m_editLineModule->setVisible(uiMethod == ProxyMethod::Manual);

        m_systemProxyTypeModule->setVisible(uiMethod != ProxyMethod::None);
        m_buttonTuple->setVisible(uiMethod != ProxyMethod::None);
    }
}

void SysProxyModule::resetData(ProxyMethod uiMethod)
{
    ProxyController *proxyController = NetworkController::instance()->proxyController();
    switch (uiMethod) {
    case ProxyMethod::None:
        break;
    case ProxyMethod::Auto:
        m_autoUrl->setText(proxyController->autoProxy());
        break;
    case ProxyMethod::Manual: {
        auto onProxyChanged = [this](const SysProxyConfig &config) {
            switch (config.type) {
            case SysProxyType::Http:
                m_httpAddr->setText(config.url);
                m_httpPort->setText(QString::number(config.port));
                break;
            case SysProxyType::Https:
                m_httpsAddr->setText(config.url);
                m_httpsPort->setText(QString::number(config.port));
                break;
            case SysProxyType::Ftp:
                m_ftpAddr->setText(config.url);
                m_ftpPort->setText(QString::number(config.port));
                break;
            case SysProxyType::Socks:
                m_socksAddr->setText(config.url);
                m_socksPort->setText(QString::number(config.port));
                break;
            }
        };
        m_ignoreList->setPlainText(proxyController->proxyIgnoreHosts());
        static QList<SysProxyType> types = { SysProxyType::Ftp, SysProxyType::Http, SysProxyType::Https, SysProxyType::Socks };
        for (const SysProxyType &type : types) {
            SysProxyConfig config = proxyController->proxy(type);
            config.type = type;
            onProxyChanged(config);
        }
    } break;
    default:
        break;
    }
}

bool SysProxyModule::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，捕捉FocusIn消息，DTextEdit没有鼠标点击消息
    if (event->type() == QEvent::FocusIn) {
        if (m_buttonTuple && ((dynamic_cast<QLineEdit *>(watched) || dynamic_cast<DTextEdit *>(watched))))
            m_buttonTuple->setEnabled(true);
    }

    return ModuleObject::eventFilter(watched, event);
}
