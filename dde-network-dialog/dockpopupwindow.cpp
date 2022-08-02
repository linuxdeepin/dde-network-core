﻿/*
 * Copyright (C) 2011 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
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

#include "dockpopupwindow.h"
#include "thememanager.h"

#include <dregionmonitor.h>
#include <DWindowManagerHelper>

#include <QScreen>
#include <QApplication>
#include <QDesktopWidget>
#include <QAccessible>
#include <QAccessibleEvent>
#include <QString>
#include <QPainterPath>

#include <iostream>

QT_BEGIN_NAMESPACE
extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0);
QT_END_NAMESPACE

#define DEFAULT_RADIUS (35)
#define DEFAULT_OPACITY (0.6)
// DArrowRectangle圆角半径，无窗管时圆角设置需比实际小点
#define ARROWRECTANGLE_RADIUS (6)

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

DockPopupWindow::DockPopupWindow(QWidget *parent)
    : DArrowRectangle(ArrowBottom, parent)
    , m_model(false)
    , m_regionInter(new DRegionMonitor(this))
    , m_bgImage(nullptr)
    , m_srcImage(nullptr)
{
    setProperty("_d_radius_force", true); // 无特效模式时，让窗口圆角
    setMargin(0);
    m_wmHelper = DWindowManagerHelper::instance();

    compositeChanged();
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

    setShadowBlurRadius(20);
    setRadius(ARROWRECTANGLE_RADIUS);
    setShadowYOffset(2);
    setShadowXOffset(0);
    setArrowWidth(18);
    setArrowHeight(10);

    connect(m_wmHelper, &DWindowManagerHelper::hasCompositeChanged, this, &DockPopupWindow::compositeChanged);
    connect(m_regionInter, &DRegionMonitor::buttonPress, this, &DockPopupWindow::onGlobMouseRelease);
}

DockPopupWindow::~DockPopupWindow()
{
    if (m_bgImage) {
        delete m_bgImage;
    }
    if (m_srcImage) {
        delete m_srcImage;
    }
}

bool DockPopupWindow::model() const
{
    return isVisible() && m_model;
}

void DockPopupWindow::setContent(QWidget *content)
{
    QWidget *lastWidget = getContent();
    if (lastWidget)
        lastWidget->removeEventFilter(this);
    content->installEventFilter(this);

    QAccessibleEvent event(this, QAccessible::NameChanged);
    QAccessible::updateAccessibility(&event);

    if (!content->objectName().trimmed().isEmpty())
        setAccessibleName(content->objectName() + "-popup");

    DArrowRectangle::setContent(content);
}

void DockPopupWindow::show(const QPoint &pos, const bool model)
{
    if (m_model != model) {
        m_model = model;
        if (m_model) {
            m_regionInter->registerRegion();
        } else {
            m_regionInter->unregisterRegion();
        }
    }

    show(pos.x(), pos.y());
}

void DockPopupWindow::show(const int x, const int y)
{
    m_lastPoint = QPoint(x, y);
    // 无数据时不显示
    if (0 == getContent()->height()) {
        hide();
    } else {
        DArrowRectangle::show(x, y);
        activateWindow(); //在显示窗口后激活此窗口
    }
}

void DockPopupWindow::showEvent(QShowEvent *e)
{
    DArrowRectangle::showEvent(e);
    QTimer::singleShot(1, this, &DockPopupWindow::ensureRaised);
}

void DockPopupWindow::hideEvent(QHideEvent *e)
{
    DArrowRectangle::hideEvent(e);
    emit hideSignal();
}

void DockPopupWindow::enterEvent(QEvent *e)
{
    DArrowRectangle::enterEvent(e);

    QTimer::singleShot(1, this, &DockPopupWindow::ensureRaised);
}

bool DockPopupWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o != getContent() || e->type() != QEvent::Resize)
        return false;

    // FIXME: ensure position move after global mouse release event
    if (isVisible()) {
        QTimer::singleShot(10, this, [ = ] {
            // NOTE(sbw): double check is necessary, in this time, the popup maybe already hided.
            if (isVisible())
                show(m_lastPoint, m_model);
        });
    }

    return false;
}

void DockPopupWindow::onGlobMouseRelease(const QPoint &mousePos, const int flag)
{
    Q_ASSERT(m_model);

    if (!isVisible() || !((flag == DRegionMonitor::WatchedFlags::Button_Left) || (flag == DRegionMonitor::WatchedFlags::Button_Right))) {
        return;
    }

    const QRect rect = QRect(pos(), size());
    if (rect.contains(mousePos))
        return;

    emit accept();
    hide(); // 点击窗口以外先隐藏
    close();
}

void DockPopupWindow::compositeChanged()
{
    setBackgroundColor(ThemeManager::instance()->backgroundColor());
    setBorderColor(ThemeManager::instance()->backgroundColor());
}

void DockPopupWindow::ensureRaised()
{
    if (isVisible()) {
        raise();
        activateWindow(); // 只要需要显示就先 activate 确认可以获取到输入
    }
}

void DockPopupWindow::setBackground(const QImage &image)
{
    if (image.isNull())
        return;
    m_srcImage = new QImage(image.copy());
    QImage img(image);
    QImage blurred(img.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter pa(&blurred);
    int radius = DEFAULT_RADIUS;
    QColor maskColor = backgroundColor();
    DBlurEffectWidget *blurBackground = findChild<DBlurEffectWidget *>();
    if (blurBackground) {
        radius = blurBackground->radius();
        maskColor = blurBackground->maskColor();
        blurBackground->hide();
    }
    qt_blurImage(&pa, img, radius * 2, false, false);
    pa.setOpacity(DEFAULT_OPACITY);
    pa.fillRect(img.rect(), maskColor);
    pa.end();
    m_bgImage = new QImage(blurred);
    setRadius(ARROWRECTANGLE_RADIUS - 1);
    setBorderWidth(0);
}

void DockPopupWindow::paintEvent(QPaintEvent *event)
{
    if (m_bgImage) {
        QRect rect = this->rect();
        QPoint p = mapToGlobal(QPoint());
        if (p.x() < 0) {
            p.setX(-p.x());
        } else {
            rect.moveLeft(p.x());
            p.setX(0);
        }
        if (p.y() < 0) {
            p.setY(-p.y());
        } else {
            rect.moveTop(p.y());
            p.setY(0);
        }
        QPainter pa(this);
        pa.drawImage(p, *m_srcImage, rect);
        int r = ARROWRECTANGLE_RADIUS;
        QPainterPath path;
        path.addRoundedRect(0, 0, rect.width(), rect.height() - arrowHeight(), r, r);
        path.addRect(r, this->height() - arrowHeight(), this->width() - r * 2, arrowHeight());
        pa.setRenderHint(QPainter::Antialiasing);
        pa.setClipPath(path);

        pa.drawImage(p, *m_bgImage, rect);
        pa.end();
    }
    Dtk::Widget::DArrowRectangle::paintEvent(event);
}
