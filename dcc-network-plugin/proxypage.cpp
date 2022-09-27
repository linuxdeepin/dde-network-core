// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proxypage.h"
#include "widgets/buttontuple.h"
#include "widgets/translucentframe.h"
#include "widgets/settingsgroup.h"
#include "widgets/settingsheaderitem.h"
#include "widgets/lineeditwidget.h"
#include "widgets/settingsitem.h"
#include "widgets/switchwidget.h"
#include "widgets/comboxwidget.h"
#include "window/utils.h"

#include <DPalette>
#include <DApplicationHelper>
#include <DButtonBox>
#include <DTextEdit>

#include <QVBoxLayout>
#include <QDebug>
#include <QComboBox>
#include <DTextEdit>

#include <networkcontroller.h>
#include <proxycontroller.h>
#include <networkconst.h>

DWIDGET_USE_NAMESPACE

using namespace dcc;
using namespace dcc::widgets;
using namespace dde::network;

const QString MANUAL = "manual";
const QString AUTO = "auto";
const QStringList ProxyMethodList = { MANUAL, AUTO };

ProxyPage::ProxyPage(QWidget *parent)
    : QWidget(parent)
    , m_autoWidget(new TranslucentFrame(this))
    , m_buttonTuple(new ButtonTuple(ButtonTuple::Save, this))
{
    ContentWidget *conentwidget = new ContentWidget(this);
    conentwidget->setAccessibleName("ProxyPage_ContentWidget");
    setWindowTitle(tr("System Proxy"));
    TranslucentFrame *contentFrame = new TranslucentFrame(conentwidget);

    m_buttonTuple->leftButton()->setText(tr("Cancel", "button"));
    m_buttonTuple->rightButton()->setText(tr("Save", "button"));
    m_buttonTuple->setVisible(false);
    m_buttonTuple->setEnabled(false);

    // 初始化代理开关、代理类型下拉框
    SettingsGroup *proxyTypeGroup = new SettingsGroup(contentFrame);
    m_proxySwitch = new SwitchWidget(proxyTypeGroup);
    m_proxyTypeBox = new ComboxWidget(proxyTypeGroup);
    m_proxySwitch->setTitle(tr("System Proxy"));
    //~ contents_path /network/System Proxy
    //~ child_page System Proxy
    m_proxyTypeBox->setTitle(tr("Proxy Type"));
    // 如果扩展，addItem添加顺序必须与ProxyMethodList顺序一致
    m_proxyTypeBox->comboBox()->addItem(tr("Manual"));
    m_proxyTypeBox->comboBox()->addItem(tr("Auto"));
    m_proxyTypeBox->setVisible(false);
    proxyTypeGroup->appendItem(m_proxySwitch);
    proxyTypeGroup->appendItem(m_proxyTypeBox);

    // 手动代理编辑界面处理逻辑提取
    auto initProxyGroup = [ this ](LineEditWidget *&proxyEdit, LineEditWidget *&portEdit, const QString &proxyTitle, SettingsGroup *&group) {
        proxyEdit = new LineEditWidget(group);
        proxyEdit->setPlaceholderText(tr("Optional"));
        proxyEdit->setTitle(proxyTitle);
        proxyEdit->textEdit()->installEventFilter(this);
        portEdit = new LineEditWidget;
        portEdit->setPlaceholderText(tr("Optional"));
        portEdit->setTitle(tr("Port"));
        portEdit->textEdit()->installEventFilter(this);
        group->appendItem(proxyEdit);
        group->appendItem(portEdit);
        connect(portEdit->textEdit(), &QLineEdit::textChanged, this, [ = ](const QString &str) {
            if (str.toInt() < 0) {
                portEdit->setText("0");
            } else if(str.toInt() > 65535) {
                portEdit->setText("65535");
            }
        });
    };

    //  手动代理编辑界面控件初始化
    SettingsGroup *httpGroup = new SettingsGroup(contentFrame);
    initProxyGroup(m_httpAddr, m_httpPort, tr("HTTP Proxy"), httpGroup);

    SettingsGroup *httpsGroup = new SettingsGroup(contentFrame);
    initProxyGroup(m_httpsAddr, m_httpsPort, tr("HTTPS Proxy"), httpsGroup);

    SettingsGroup *ftpGroup = new SettingsGroup(contentFrame);
    initProxyGroup(m_ftpAddr, m_ftpPort, tr("FTP Proxy"), ftpGroup);

    SettingsGroup *socksGroup = new SettingsGroup(contentFrame);
    initProxyGroup(m_socksAddr, m_socksPort, tr("SOCKS Proxy"), socksGroup);

    // 手动代理界面忽略主机编辑框初始化
    m_ignoreList = new DTextEdit(contentFrame);
    m_ignoreList->setAccessibleName("ProxyPage_ignoreList");
    m_ignoreList->installEventFilter(this);
    QLabel *ignoreTips = new QLabel(contentFrame);
    ignoreTips->setWordWrap(true);
    ignoreTips->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ignoreTips->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ignoreTips->setText(tr("Ignore the proxy configurations for the above hosts and domains"));

    // 自动代理界面控件初始化
    SettingsGroup *autoGroup = new SettingsGroup(contentFrame);
    m_autoUrl = new LineEditWidget;
    m_autoUrl->setPlaceholderText(tr("Optional"));

    //~ contents_path /network/System Proxy
    //~ child_page System Proxy
    m_autoUrl->setTitle(tr("Configuration URL"));
    m_autoUrl->textEdit()->installEventFilter(this);
    autoGroup->appendItem(m_autoUrl);

    // 手动代理界面布局
    QVBoxLayout *manualLayout = new QVBoxLayout(contentFrame);
    manualLayout->addWidget(httpGroup);
    manualLayout->addWidget(httpsGroup);
    manualLayout->addWidget(ftpGroup);
    manualLayout->addWidget(socksGroup);
    manualLayout->addWidget(m_ignoreList);
    manualLayout->addWidget(ignoreTips);
    manualLayout->setMargin(0);
    manualLayout->setSpacing(10);
    m_manualWidget = new TranslucentFrame(contentFrame);
    m_manualWidget->setLayout(manualLayout);
    m_manualWidget->setVisible(false);

    // 自动代理界面布局
    QVBoxLayout *autoLayout = new QVBoxLayout(m_autoWidget);
    autoLayout->setContentsMargins(ThirdPageContentsMargins);
    autoLayout->addWidget(autoGroup);
    autoLayout->setSpacing(10);
    autoLayout->setMargin(0);
    m_autoWidget->setLayout(autoLayout);
    m_autoWidget->setVisible(false);

    // 主窗口布局组合
    QVBoxLayout *mainLayout = new QVBoxLayout(contentFrame);
    mainLayout->addWidget(proxyTypeGroup);
    mainLayout->addWidget(m_manualWidget);
    mainLayout->addWidget(m_autoWidget);
    mainLayout->addSpacing(10);
    mainLayout->setContentsMargins(QMargins(10, 0, 10, 0));       // 设置列表项与背景左右间距分别为10
    mainLayout->addStretch();

    // 加入button布局
    contentFrame->setLayout(mainLayout);
    conentwidget->setContent(contentFrame);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setMargin(0);
    QVBoxLayout *btnLayout = new QVBoxLayout(this);
    btnLayout->setMargin(0);
    btnLayout->addWidget(m_buttonTuple);
    vLayout->addWidget(conentwidget);
    vLayout->addLayout(btnLayout);
    setContentsMargins(QMargins(0, 10, 0, 10)); // 圆角
    setLayout(vLayout);

    ProxyController *proxyController = NetworkController::instance()->proxyController();
    connect(proxyController, &ProxyController::proxyMethodChanged, this, &ProxyPage::onProxyMethodChanged);
    connect(proxyController, &ProxyController::proxyIgnoreHostsChanged, this, &ProxyPage::onIgnoreHostsChanged);
    connect(proxyController, &ProxyController::proxyChanged, this, &ProxyPage::onProxyChanged);
    connect(proxyController, &ProxyController::autoProxyChanged, m_autoUrl, &LineEditWidget::setText);

    onProxyMethodChanged(proxyController->proxyMethod());
    onIgnoreHostsChanged(proxyController->proxyIgnoreHosts());
    m_autoUrl->setText(proxyController->autoProxy());

    // 响应系统代理开关
    connect(m_proxySwitch, &SwitchWidget::checkedChanged, m_proxyTypeBox, [ = ](const bool checked) {
        m_buttonTuple->setEnabled(checked);
        if (checked) {
            // 打开代理默认手动
            onProxyMethodChanged(ProxyMethod::Manual);
        } else {
            // 关闭代理
            proxyController->setProxyMethod(ProxyMethod::None);
            m_proxyTypeBox->setVisible(false);
            m_manualWidget->setVisible(false);
            m_autoWidget->setVisible(false);
            m_buttonTuple->setVisible(false);
            m_buttonTuple->rightButton()->clicked();
        }
    });

    // 处理协议类型下拉框切换
    connect(m_proxyTypeBox->comboBox(), static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [ = ] (int index) {
        m_buttonTuple->setEnabled(true);
        int manualId = ProxyMethodList.indexOf(MANUAL);
        int autoId = ProxyMethodList.indexOf(AUTO);
        m_manualWidget->setVisible(index == manualId);
        m_autoWidget->setVisible(index == autoId);
        m_buttonTuple->setVisible(index == manualId || index == autoId);
    });

    // 取消、确定按钮响应
    connect(m_buttonTuple->rightButton(), &QPushButton::clicked, this, &ProxyPage::applySettings);
    connect(m_buttonTuple->leftButton(), &QPushButton::clicked, this, [ = ] {
        clearLineEditWidgetFocus();
        m_autoUrl->dTextEdit()->clearFocus();
        m_buttonTuple->setEnabled(false);
        m_buttonTuple->setFocus();
        if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodList.indexOf(MANUAL)) {
            auto currentProxyConfig = [ = ] (const SysProxyType &type) {
                SysProxyConfig config = proxyController->proxy(type);
                config.type = type;
                return config;
            };

            static QList<SysProxyType> types = { SysProxyType::Ftp, SysProxyType::Http, SysProxyType::Https, SysProxyType::Socks };

            for (const SysProxyType &type : types) {
                SysProxyConfig config = currentProxyConfig(type);
                onProxyChanged(config);
            }

            onIgnoreHostsChanged(proxyController->proxyIgnoreHosts());
        } else {
            m_autoUrl->setText(proxyController->autoProxy());
        }
    });

    QTimer::singleShot(1, this, [ = ] {
        proxyController->querySysProxyData();
    });
}

ProxyPage::~ProxyPage()
{
}

void ProxyPage::onProxyMethodChanged(const ProxyMethod &method)
{
    m_proxyTypeBox->comboBox()->blockSignals(true);
    if (method == ProxyMethod::None) {
        m_proxySwitch->blockSignals(true);
        m_proxySwitch->setChecked(false);
        m_proxySwitch->blockSignals(false);
        m_manualWidget->setVisible(false);
        m_autoWidget->setVisible(false);
        m_buttonTuple->setVisible(false);
        m_proxyTypeBox->setVisible(false);
    } else if (method == ProxyMethod::Manual) {
        m_proxySwitch->blockSignals(true);
        m_proxySwitch->setChecked(true);
        m_proxySwitch->blockSignals(false);
        m_manualWidget->setVisible(true);
        m_autoWidget->setVisible(false);
        m_buttonTuple->setVisible(true);
        m_proxyTypeBox->comboBox()->setCurrentIndex(ProxyMethodList.indexOf(MANUAL));
        m_proxyTypeBox->setVisible(true);
    } else if (method == ProxyMethod::Auto) {
        m_proxySwitch->blockSignals(true);
        m_proxySwitch->setChecked(true);
        m_proxySwitch->blockSignals(false);
        m_manualWidget->setVisible(false);
        m_autoWidget->setVisible(true);
        m_buttonTuple->setVisible(true);
        m_proxyTypeBox->comboBox()->setCurrentIndex(ProxyMethodList.indexOf(AUTO));
        m_proxyTypeBox->setVisible(true);
    }
    m_proxyTypeBox->comboBox()->blockSignals(false);
}

void ProxyPage::applySettings()
{
    ProxyController *proxyController = NetworkController::instance()->proxyController();
    this->setFocus();
    m_buttonTuple->setEnabled(false);
    if (!m_proxySwitch->checked()) {
        proxyController->setProxyMethod(ProxyMethod::None);
    } else if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodList.indexOf(MANUAL)) {
        proxyController->setProxy(SysProxyType::Http, m_httpAddr->text(), m_httpPort->text());
        proxyController->setProxy(SysProxyType::Https, m_httpsAddr->text(), m_httpsPort->text());
        proxyController->setProxy(SysProxyType::Ftp, m_ftpAddr->text(), m_ftpPort->text());
        proxyController->setProxy(SysProxyType::Socks, m_socksAddr->text(), m_socksPort->text());
        proxyController->setProxyIgnoreHosts(m_ignoreList->toPlainText());
        proxyController->setProxyMethod(ProxyMethod::Manual);
    } else if (m_proxyTypeBox->comboBox()->currentIndex() == ProxyMethodList.indexOf(AUTO)) {
        proxyController->setAutoProxy(m_autoUrl->text());
        proxyController->setProxyMethod(ProxyMethod::Auto);
    }
}

void ProxyPage::onIgnoreHostsChanged(const QString &hosts)
{
    const QTextCursor cursor = m_ignoreList->textCursor();

    m_ignoreList->blockSignals(true);
    m_ignoreList->setPlainText(hosts);
    m_ignoreList->setTextCursor(cursor);
    m_ignoreList->blockSignals(false);
}

void ProxyPage::onProxyChanged(const SysProxyConfig &config)
{
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
}

bool ProxyPage::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，捕捉FocusIn消息，DTextEdit没有鼠标点击消息
    if (event->type() == QEvent::FocusIn) {
        if ((dynamic_cast<QLineEdit*>(watched) || dynamic_cast<DTextEdit*>(watched)))
            m_buttonTuple->setEnabled(true);
    }

    return QWidget::eventFilter(watched, event);
}

void ProxyPage::clearLineEditWidgetFocus()
{
    if (m_autoUrl && m_autoUrl->dTextEdit())
        m_autoUrl->dTextEdit()->clearFocus();

    if (m_httpAddr && m_httpAddr->dTextEdit())
        m_httpAddr->dTextEdit()->clearFocus();

    if (m_httpPort && m_httpPort->dTextEdit())
        m_httpPort->dTextEdit()->clearFocus();

    if (m_httpsAddr && m_httpsAddr->dTextEdit())
        m_httpsAddr->dTextEdit()->clearFocus();

    if (m_httpsPort && m_httpsPort->dTextEdit())
        m_httpsPort->dTextEdit()->clearFocus();

    if (m_ftpAddr && m_ftpAddr->dTextEdit())
        m_ftpAddr->dTextEdit()->clearFocus();

    if (m_ftpPort && m_ftpPort->dTextEdit())
        m_ftpPort->dTextEdit()->clearFocus();

    if (m_socksAddr && m_socksAddr->dTextEdit())
        m_socksAddr->dTextEdit()->clearFocus();

    if (m_socksPort && m_socksPort->dTextEdit())
        m_socksPort->dTextEdit()->clearFocus();

    if (m_ignoreList)
        m_ignoreList->clearFocus();
}
