// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pppoesection.h"
#include "widgets/contentwidget.h"
#include "widgets/lineeditwidget.h"
#include "../widgets/passwdlineeditwidget.h"

#include <QDebug>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/wireddevice.h>

using namespace dcc::widgets;
using namespace NetworkManager;

PPPOESection::PPPOESection(PppoeSetting::Ptr pppoeSetting, QFrame *parent)
    : AbstractSection(tr("PPPoE"), parent)
    , m_pppoeSetting(pppoeSetting)
    , m_userName(new LineEditWidget(this))
    , m_service(new LineEditWidget(this))
    , m_password(new PasswdLineEditWidget(this))
{
    initUI();

    m_userName->textEdit()->installEventFilter(this);
    m_service->textEdit()->installEventFilter(this);
    m_password->textEdit()->installEventFilter(this);
}

PPPOESection::~PPPOESection()
{
}

bool PPPOESection::allInputValid()
{
    bool valid = true;

    if (m_userName->text().isEmpty()) {
        valid = false;
        m_userName->setIsErr(true);
        m_userName->dTextEdit()->setAlert(true);
    } else {
        m_userName->setIsErr(false);
    }

    if (m_password->text().isEmpty()) {
        valid = false;
        m_password->setIsErr(true);
        m_password->dTextEdit()->setAlert(true);
    } else {
        m_password->setIsErr(false);
    }

    return valid;
}

void PPPOESection::saveSettings()
{
    m_pppoeSetting->setUsername(m_userName->text());
    m_pppoeSetting->setService(m_service->text());
    m_pppoeSetting->setPasswordFlags(Setting::SecretFlagType::None);
    m_pppoeSetting->setPassword(m_password->text());
    m_pppoeSetting->setParent(getDeviceInterface());

    m_pppoeSetting->setInitialized(true);
}

void PPPOESection::initUI()
{
    setAccessibleName("PPPOESection");
    m_userName->setTitle(tr("Username"));
    m_userName->setPlaceholderText(tr("Required"));
    m_userName->setText(m_pppoeSetting->username());

    m_service->setTitle(tr("Service"));
    m_service->setText(m_pppoeSetting->service());

    m_password->setTitle(tr("Password"));
    m_password->setPlaceholderText(tr("Required"));
    m_password->setText(m_pppoeSetting->password());

    appendItem(m_userName);
    appendItem(m_service);
    appendItem(m_password);
}

QString PPPOESection::getDeviceInterface()
{
    // 只获取没有隐藏的有线网卡的interface
    NetworkManager::Device::List devices = networkInterfaces();
    for (NetworkManager::Device::Ptr device : devices) {
        if (device->type() != NetworkManager::Device::Type::Ethernet)
            continue;

        if (!device->managed())
            continue;

        return device->interfaceName();
    }

    return QString();
}

bool PPPOESection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    if (event->type() == QEvent::FocusIn) {
        if (dynamic_cast<QLineEdit *>(watched))
            Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
