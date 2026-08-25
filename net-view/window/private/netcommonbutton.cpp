// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "netcommonbutton.h"

#include <DStyleOption>
#include <QPainter>

namespace dde {
namespace network {

NetCommonButton::NetCommonButton(QWidget *parent, const QString &text)
: QAbstractButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Dtk::Widget::DFontSizeManager::instance()->bind(this, Dtk::Widget::DFontSizeManager::T8);
}

void NetCommonButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();

    if (isDown()) {
        p.setBrush(QColor(0,0,0,255 * 0.2));
    } else if (underMouse()) {
        p.setBrush(QColor(0, 0, 0, 255 * 0.15));
    } else {
        p.setBrush(QColor(0, 0, 0, 255 * 0.1));
    }
        
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor(255, 255, 255, 255));
    p.drawText(rect, Qt::AlignCenter, text());
}

QSize NetCommonButton::sizeHint() const
{
    const QFontMetrics metrics(font());
    return QSize(metrics.horizontalAdvance(text()) + 24, metrics.height() + 6);
}

QSize NetCommonButton::minimumSizeHint() const
{
    return sizeHint();
}

} // namespace network
} // namespace dde
