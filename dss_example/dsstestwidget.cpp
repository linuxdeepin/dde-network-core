// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dsstestwidget.h"

#include "networkmodule.h"
#include "popupwindow.h"

#include <networkcontroller.h>

#include <DPalette>

#include <QHBoxLayout>
#include <QMouseEvent>

using namespace dde::network;
using namespace Dtk::Widget;
DGUI_USE_NAMESPACE

DssTestWidget::DssTestWidget(dde::network::NetworkPlugin *networkPlugin, QWidget *parent)
    : QWidget(parent)
    , m_pModule(networkPlugin)
    , m_container(nullptr)
    , m_tipContainer(nullptr)
{
    QWidget *iconWidget = new QWidget(this);
    m_iconButton = new DFloatingButton(iconWidget);
    m_iconButton->setIconSize(QSize(26, 26));
    m_iconButton->setFixedSize(QSize(52, 52));
    m_iconButton->setAutoExclusive(true);
    QPalette palette = m_iconButton->palette();
    palette.setColor(QPalette::ColorRole::Window, Qt::transparent);
    m_iconButton->setPalette(palette);
    m_iconButton->installEventFilter(this);
    connect(m_iconButton, &Dtk::Widget::DFloatingButton::clicked, this, &DssTestWidget::onClickButton);

    QHBoxLayout *iconLayout = new QHBoxLayout(iconWidget);
    iconLayout->setAlignment(Qt::AlignHCenter);
    iconLayout->addWidget(m_iconButton);

    QPalette paletteSelf = this->palette();
    paletteSelf.setColor(QPalette::ColorRole::Window, Qt::darkBlue);
    setAutoFillBackground(true);
    setPalette(paletteSelf);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(iconWidget);

    QHBoxLayout *iconButtonLayout = new QHBoxLayout(m_iconButton);
    QWidget *itemWidget = m_pModule->itemWidget();
    itemWidget->setParent(m_iconButton);
    iconButtonLayout->addWidget(itemWidget);

    installEventFilter(this);
    itemWidget->installEventFilter(this);
    m_iconButton->installEventFilter(this);
}

DssTestWidget::~DssTestWidget()
{
}

bool DssTestWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_iconButton) {
        // 当鼠标移入的时候显示提示信息
        switch (event->type()) {
        case QEvent::Enter: {
            if (!m_tipContainer) {
                QWidget *tipWidget = m_pModule->itemTipsWidget();
                m_tipContainer = new PopupWindow(this);
                m_tipContainer->setContent(tipWidget);
                m_tipContainer->resizeWithContent();
                m_tipContainer->setArrowX(tipWidget->width() / 2);
            }
            m_tipContainer->raise();
            m_tipContainer->show(QPoint(rect().center().x(), m_iconButton->parentWidget()->y()));
        } break;
        case QEvent::Leave: {
            m_tipContainer->hide();
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void DssTestWidget::resizeEvent(QResizeEvent *event)
{
    if (m_container && m_container->isVisible()) {
        m_container->resizeWithContent();
        m_container->show(QPoint(rect().width() / 2, m_iconButton->parentWidget()->y()));
    }
    QWidget::resizeEvent(event);
}

void DssTestWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_tipContainer && m_tipContainer->isVisible())
        m_tipContainer->hide();
    if (m_container && m_container->isVisible())
        m_container->hide();

    QWidget::mousePressEvent(event);
}

void DssTestWidget::onClickButton()
{
    if (!m_pModule->content())
        return;

    // 左键弹出菜单
    if (!m_container) {
        m_container = new PopupWindow(this);
        connect(m_container, &PopupWindow::contentDetach, this, [ this ] {
            m_container->setContent(nullptr);
            m_container->hide();
        });
    }
    static QWidget *netlistWidget = nullptr;
    if (!netlistWidget) {
        netlistWidget = m_pModule->content();
    }
    netlistWidget->setParent(m_container);
    netlistWidget->adjustSize();
    m_container->setContent(netlistWidget);
    m_container->resizeWithContent();
    m_container->setArrowX(m_container->width() / 2);

    if (m_container->isVisible()) {
        m_container->hide();
        if (m_tipContainer) {
            m_tipContainer->toggle();
        }
    } else {
        if (m_tipContainer) {
            m_tipContainer->hide();
        }
        QWidget *content = m_container->getContent();
        content->adjustSize();
        m_container->resizeWithContent();
        m_container->show(QPoint(rect().width() / 2, m_iconButton->parentWidget()->y()));
    }
}
