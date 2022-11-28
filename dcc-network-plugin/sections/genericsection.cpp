// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "genericsection.h"
#include "../window/gsettingwatcher.h"

#include <networkmanagerqt/settings.h>

#include <QLineEdit>

#include "widgets/lineeditwidget.h"
#include "widgets/switchwidget.h"

using namespace NetworkManager;
using namespace dcc::widgets;

GenericSection::GenericSection(ConnectionSettings::Ptr connSettings, QFrame *parent)
    : AbstractSection(tr("General"), parent)
    , m_connIdItem(new LineEditWidget(this))
    , m_autoConnItem(new SwitchWidget(this))
    , m_connSettings(connSettings)
    , m_connType(ConnectionSettings::Unknown)
{
    initUI();
    m_connIdItem->textEdit()->installEventFilter(this);
    connect(m_autoConnItem, &SwitchWidget::checkedChanged, this, &GenericSection::editClicked);
    connect(m_connIdItem->textEdit(), &QLineEdit::textChanged, this,  &GenericSection::ssidChanged);
}

GenericSection::~GenericSection()
{
    delete m_connIdItem;
    m_connIdItem = nullptr;
    delete m_autoConnItem;
    m_autoConnItem = nullptr;
}

void GenericSection::setConnectionType(ConnectionSettings::ConnectionType connType)
{
    m_connType = connType;
}

bool GenericSection::connectionNameIsEditable()
{
    return m_connIdItem->isEnabled();
}

const QString GenericSection::connectionName() const
{
    return m_connIdItem->text();
}

void GenericSection::setConnectionName(const QString &name)
{
    m_connIdItem->setText(name);
}

bool GenericSection::allInputValid()
{
    QString inputTxt = m_connIdItem->textEdit()->text();
    if (inputTxt.isEmpty()) {
        m_connIdItem->setIsErr(true);
        return false;
    }

    if (m_connType == ConnectionSettings::Vpn) {
        Connection::List connList = listConnections();
        QStringList connNameList;
        QString curUuid = "";
        if (!m_connSettings.isNull())
            curUuid = m_connSettings->uuid();

        for (auto conn : connList) {
            if (conn->settings()->connectionType() == m_connType) {
                if ((conn->name() == inputTxt) && (conn->uuid() != curUuid)) {
                    m_connIdItem->setIsErr(true);
                    m_connIdItem->dTextEdit()->showAlertMessage(tr("The name already exists"), m_connIdItem, 2000);
                    return false;
                }
            }
        }
    }

    return true;
}

void GenericSection::saveSettings()
{
    m_connSettings->setId(m_connIdItem->text());
    m_connSettings->setAutoconnect(m_autoConnItem->checked());
}

bool GenericSection::autoConnectChecked() const
{
    return m_autoConnItem->checked();
}

void GenericSection::setConnectionNameEditable(const bool editable)
{
    m_connIdItem->textEdit()->setClearButtonEnabled(editable);
    m_connIdItem->textEdit()->setEnabled(editable);
}

void GenericSection::initUI()
{
    setAccessibleName("GenericSection");
    if (m_connSettings->connectionType() == NetworkManager::ConnectionSettings::Wireless) {
        m_connIdItem->setTitle(tr("Name (SSID)"));
    } else {
        m_connIdItem->setTitle(tr("Name"));
    }

    m_connIdItem->setText(m_connSettings->id());
    m_connIdItem->setPlaceholderText(tr("Required"));
    m_autoConnItem->setChecked(m_connSettings->autoconnect());
    m_autoConnItem->setTitle(tr("Auto Connect"));

    appendItem(m_connIdItem);
    appendItem(m_autoConnItem);
}

bool GenericSection::eventFilter(QObject *watched, QEvent *event)
{
    // 实现鼠标点击编辑框，确定按钮激活，统一网络模块处理，捕捉FocusIn消息
    QLineEdit * lineEdit = qobject_cast<QLineEdit *>(watched);
    if (lineEdit && event->type() == QEvent::FocusIn && lineEdit->isEnabled()) {
        Q_EMIT editClicked();
    }

    return QWidget::eventFilter(watched, event);
}
