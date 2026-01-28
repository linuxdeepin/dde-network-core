// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dsstestwidget.h"
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include <DPalette>
#include <DFloatingButton>
#include <QMouseEvent>

#include <QHBoxLayout>
#include <networkcontroller.h>

using namespace Dtk::Widget;
DGUI_USE_NAMESPACE

DssTestWidget::DssTestWidget(QWidget *parent)
    : QWidget(parent)
    , m_button(nullptr)
{
    m_button = new Dtk::Widget::DFloatingButton(this);
    m_button->setIconSize(QSize(26, 26));
    m_button->setFixedSize(QSize(52, 52));
    m_button->setAutoExclusive(true);
    m_button->setBackgroundRole(DPalette::Button);
    m_button->setIcon(QIcon("network-online-symbolic"));
    m_button->installEventFilter(this);
    
    // 创建一个简单的布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_button);
}

DssTestWidget::~DssTestWidget()
{
}

bool DssTestWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_button) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::RightButton) {
                    QMessageBox::information(this, "DSS Example", "右键点击 - 网络测试程序");
                } else if (mouseEvent->button() == Qt::LeftButton) {
                    QMessageBox::information(this, "DSS Example", "左键点击 - 网络测试程序");
                }
            }
            break;
        case QEvent::Enter:
                break;
        case QEvent::Leave:
            break;
        default: break;
        }
    }

    return QWidget::eventFilter(watched, event);
}
