// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dccplugintestwidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>

DccPluginTestWidget::DccPluginTestWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

DccPluginTestWidget::~DccPluginTestWidget()
{
}

void DccPluginTestWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("DCC Plugin Test Widget", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    setLayout(layout);
    setWindowTitle("DCC Plugin Test");
    resize(400, 300);
}

void DccPluginTestWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    QWidget::paintEvent(event);
}