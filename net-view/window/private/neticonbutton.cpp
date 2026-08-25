// SPDX-FileCopyrightText: 2019 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "neticonbutton.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

namespace dde {
namespace network {

NetIconButton::NetIconButton(QWidget *parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
    , m_rotateAngle(0)
    , m_clickable(false)
    , m_rotatable(false)
    , m_hover(false)
    , m_textType(true)
{
    setAccessibleName("NetIconButton");
    setFixedSize(24, 24);
    if (parent)
        setForegroundRole(parent->foregroundRole());
}

void NetIconButton::setIcon(const QIcon &icon)
{
    m_icon = icon;
    update();
}

void NetIconButton::setHoverIcon(const QIcon &icon)
{
    m_hoverIcon = icon;
}

void NetIconButton::setClickable(bool clickable)
{
    m_clickable = clickable;
}

void NetIconButton::setRotatable(bool rotatable)
{
    m_rotatable = rotatable;
    if (!m_rotatable) {
        if (m_refreshTimer)
            delete m_refreshTimer;
        m_refreshTimer = nullptr;
    }
}

void NetIconButton::setTextType(bool textType)
{
    m_textType = textType;
    update();
}

void NetIconButton::startRotate()
{
    if (!m_refreshTimer) {
        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(50);
        connect(m_refreshTimer, &QTimer::timeout, this, &NetIconButton::startRotate);
    }
    m_refreshTimer->start();
    m_rotateAngle += 54;
    update();
    if (m_rotateAngle >= 360) {
        stopRotate();
    }
}

void NetIconButton::stopRotate()
{
    m_refreshTimer->stop();
    m_rotateAngle = 0;
    update();
}

bool NetIconButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Leave:
    case QEvent::Enter:
        m_hover = e->type() == QEvent::Enter;
        update();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void NetIconButton::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);
    if (m_icon.isNull())
        return;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    QRect r = rect();
    if (m_rotateAngle != 0) {
        painter.translate(r.width() / 2, r.height() / 2);
        painter.rotate(m_rotateAngle);
        painter.translate(-(r.width() / 2), -(r.height() / 2));
    }
    const qreal scale = devicePixelRatio();
    auto pa = palette();
    pa.setColor(QPalette::WindowText, painter.pen().color());
    // 设置dci图标单独调色板 参考dtk源码
    // https://github.com/linuxdeepin/dtkgui/blob/9d3d659afedfa5813421e2430e26cd683246ab35/src/util/private/dciiconengine.cpp#L36C1-L37C1
    setProperty("palette", pa);
    if (m_hover && !m_hoverIcon.isNull()) {
        m_hoverIcon.paint(&painter, r, Qt::AlignCenter);
    } else {
      m_icon.paint(&painter, r, Qt::AlignCenter);
    }
}

void NetIconButton::mousePressEvent(QMouseEvent *event)
{
    m_pressPos = event->pos();
    return QWidget::mousePressEvent(event);
}

void NetIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clickable && rect().contains(m_pressPos) && rect().contains(event->pos()) && (!m_refreshTimer || !m_refreshTimer->isActive())) {
        if (m_rotatable)
            startRotate();
        Q_EMIT clicked();
        return;
    }
    return QWidget::mouseReleaseEvent(event);
}
} // namespace network
} // namespace dde
