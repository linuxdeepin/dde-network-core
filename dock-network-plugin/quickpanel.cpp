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
#include <DIconButton>

#include <QMouseEvent>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

const QSize IconSize(24, 24); // 图标大小

QuickPanel::QuickPanel(QWidget *parent)
    : QWidget(parent)
    , m_iconButton(new DIconButton(this))
    , m_text(new DLabel(this))
    , m_description(new DLabel(this))
    , m_hover(false)
    , m_active(false)
{
    initUi();
    initConnect();
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
}

void QuickPanel::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);
    const DPalette &dp = palette();
    painter.setPen(Qt::NoPen);
    painter.setBrush(dp.brush(m_hover ? DPalette::ObviousBackground : DPalette::ItemBackground));
    painter.drawRoundedRect(rect(), 8, 8);
}

void QuickPanel::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_iconButton->rect().contains(event->pos()) && rect().contains(event->pos())) {
        emit panelClicked();
    }
}

void QuickPanel::enterEvent(QEvent *event)
{
    setHover(true);
}

void QuickPanel::leaveEvent(QEvent *event)
{
    setHover(false);
}

bool QuickPanel::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Enter:
        setHover(false);
        break;
    case QEvent::Leave:
        setHover(true);
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void QuickPanel::initUi()
{
    // 文本
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
    labelWidget->setGeometry(46, 5, 80, 50);
    // 图标
    m_iconButton->setEnabledCircle(true);
    m_iconButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_iconButton->setIconSize(IconSize);
    m_iconButton->setGeometry(8, 10, 36, 36);
    m_iconButton->installEventFilter(this);
    // 进入图标
    QLabel *enterIcon = new QLabel(this);
    qreal ratio = devicePixelRatioF();
    QPixmap enterPixmap = DStyle::standardIcon(style(), DStyle::SP_ArrowEnter).pixmap(QSize(16, 16) * ratio);
    enterPixmap.setDevicePixelRatio(ratio);
    enterIcon->setPixmap(enterPixmap);
    enterIcon->setGeometry(127, 22, 16, 16);

    setFixedSize(150, 60);
}

void QuickPanel::initConnect()
{
    connect(m_iconButton, &DIconButton::clicked, this, &QuickPanel::iconClicked);
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
    m_iconButton->setIcon(QIcon(m_iconPixmap));
}

void QuickPanel::setHover(bool hover)
{
    if (hover == m_hover)
        return;

    m_hover = hover;
    update();
}
