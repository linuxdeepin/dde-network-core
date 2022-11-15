/*
 * Copyright (C) 2021 ~ 2023 Deepin Technology Co., Ltd.
 *
 * Author:     caixiangrong <caixiangrong@uniontech.com>
 *
 * Maintainer: caixiangrong <caixiangrong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "quickpanel.h"

#include <DLabel>
#include <DFontSizeManager>
#include <QMouseEvent>

#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

const QSize IconSize(24, 24);            // 图标大小
const QSize ExpandSize(16, 16);          // 展开图标大小
const QRect IconRect(10, 12, 36, 36);    // 图标位置
const QRect ExpandRect(127, 22, 16, 16); // 展开图标位置
// 图标绘制坐标
const QRect PixmapIconRect(IconRect.x() + (IconRect.width() - IconSize.width()) / 2, IconRect.y() + (IconRect.height() - IconSize.height()) / 2, IconSize.width(), IconSize.height());

QuickPanel::QuickPanel(QWidget *parent)
    : QWidget(parent)
    , m_text(new DLabel(this))
    , m_description(new DLabel(this))
    , m_clickIcon(false)
    , m_clickExpand(false)
    , m_hoverIcon(false)
    , m_hoverExpand(false)
    , m_active(false)
{
    initUi();
}

const QVariant &QuickPanel::userData() const
{
    return m_userData;
}

void QuickPanel::setUserData(const QVariant &data)
{
    m_userData = data;
}

const QString QuickPanel::text() const
{
    return m_text->text();
}

const QString QuickPanel::description() const
{
    return m_description->text();
}

void QuickPanel::setIcon(const QIcon &icon)
{
    m_icon = icon;
    updateIconPixmap();
    update();
}

void QuickPanel::setText(const QString &text)
{
    m_text->setText(text);
}

void QuickPanel::setDescription(const QString &description)
{
    m_description->setText(description);
    m_description->setToolTip(description);
}

void QuickPanel::setActive(bool active)
{
    m_active = active;
    updateIconPixmap();
    update();
}

void QuickPanel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette().brush(m_hoverIcon ? DPalette::Active : DPalette::Disabled, DPalette::ColorRole::Button));
    painter.drawEllipse(IconRect);
    painter.restore();
    painter.drawPixmap(PixmapIconRect, m_iconPixmap);
    painter.drawPixmap(ExpandRect, m_expandPixmap);
}

void QuickPanel::mouseMoveEvent(QMouseEvent *event)
{
    m_hoverIcon = IconRect.contains(event->pos());
    bool hoverExpand = ExpandRect.contains(event->pos());
    if (m_hoverExpand != hoverExpand) {
        m_hoverExpand = hoverExpand;
        updateExpandPixmap();
    }
    update();
}

void QuickPanel::mousePressEvent(QMouseEvent *event)
{
    if (IconRect.contains(event->pos())) {
        m_clickIcon = true;
    } else if (ExpandRect.contains(event->pos())) {
        m_clickExpand = true;
    }
}

void QuickPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_clickIcon && IconRect.contains(event->pos())) {
        emit iconClick();
    } else if (m_clickExpand && ExpandRect.contains(event->pos())) {
        emit panelClick();
    }
    m_clickIcon = false;
    m_clickExpand = false;
}

void QuickPanel::leaveEvent(QEvent *event)
{
    m_hoverIcon = false;
    if (m_hoverExpand != false) {
        m_hoverExpand = false;
        updateExpandPixmap();
    }
    update();
}

void QuickPanel::initUi()
{
    QWidget *labelWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(labelWidget);
    layout->addWidget(m_text, 0, Qt::AlignLeft | Qt::AlignVCenter);
    QFont nameFont = DFontSizeManager::instance()->t6();
    nameFont.setBold(true);
    m_text->setFont(nameFont);
    m_text->setElideMode(Qt::ElideRight);
    layout->addWidget(m_description, 0, Qt::AlignLeft | Qt::AlignVCenter);
    m_description->setFont(DFontSizeManager::instance()->t10());
    m_description->setElideMode(Qt::ElideRight);

    setFixedSize(150, 60);
    labelWidget->setGeometry(46, 5, 80, 50);
    setMouseTracking(true);
    updateExpandPixmap();
}

void QuickPanel::updateIconPixmap()
{
    qreal ratio = devicePixelRatioF();
    m_iconPixmap = m_icon.pixmap(IconSize * ratio);
    m_iconPixmap.setDevicePixelRatio(ratio);
    if (m_active) {
        QPainter pa(&m_iconPixmap);
        pa.setCompositionMode(QPainter::CompositionMode_SourceIn);
        pa.fillRect(m_iconPixmap.rect(), palette().highlight());
    }
}

void QuickPanel::updateExpandPixmap()
{
    qreal ratio = devicePixelRatioF();
    m_expandPixmap = DStyle::standardIcon(style(), DStyle::SP_ArrowEnter).pixmap(ExpandSize * ratio);
    m_expandPixmap.setDevicePixelRatio(ratio);
    if (m_hoverExpand) {
        QPainter expandPainter(&m_expandPixmap);
        expandPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        expandPainter.fillRect(m_expandPixmap.rect(), palette().highlight());
    }
}
