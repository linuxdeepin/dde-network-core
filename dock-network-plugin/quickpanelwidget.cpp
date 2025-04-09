// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickpanelwidget.h"

#include <DFontSizeManager>
#include <DStyle>
#include <DToolTip>

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

const QSize IconSize(22, 22); // 图标大小

namespace dde {
namespace network {
class QuickButton : public DFloatingButton
{
public:
    QuickButton(QWidget *parent = nullptr)
        : DFloatingButton(parent)
    {
    }

protected:
    void initStyleOption(DStyleOptionButton *option) const override
    {
        DFloatingButton::initStyleOption(option);
        QColor bgColor = option->dpalette.color(backgroundRole());
        QColor textColor = option->dpalette.color(foregroundRole());
        if (backgroundRole() == QPalette::Highlight) {
            textColor = Qt::white;
            if (!option->state.testFlag(QStyle::State_Raised)) { // press
                bgColor.setHslF(bgColor.hslHueF(), bgColor.hslSaturationF(), bgColor.lightnessF() * 0.9);
                textColor.setAlphaF(0.8);
            } else if (option->state.testFlag(QStyle::State_MouseOver)) { // hover
                bgColor.setHslF(bgColor.hslHueF(), bgColor.hslSaturationF(), bgColor.lightnessF() * 1.1);
            }
        } else {
            textColor.setAlphaF(1);
            if (!option->state.testFlag(QStyle::State_Raised)) { // press
                bgColor.setAlphaF(0.2);
            } else if (option->state.testFlag(QStyle::State_MouseOver)) { // hover
                bgColor.setAlphaF(0.15);
            } else { // normal
                bgColor.setAlphaF(0.1);
            }
        }
        option->palette.setBrush(QPalette::Button, bgColor);
        option->palette.setBrush(QPalette::ButtonText, textColor);
        option->palette.setBrush(QPalette::WindowText, Qt::red);
        option->state.setFlag(QStyle::State_MouseOver, false);
        option->state.setFlag(QStyle::State_Sunken, false);
        option->state.setFlag(QStyle::State_Raised, true);
    }
};

QuickPanelWidget::QuickPanelWidget(QWidget *parent)
    : QWidget(parent)
    , m_iconWidget(new QuickButton(this))
    , m_nameLabel(new DLabel(this))
    , m_stateLabel(new DLabel(this))
    , m_expandLabel(new DIconButton(this))
{
    initUi();
    initConnection();
}

QuickPanelWidget::~QuickPanelWidget() { }

void QuickPanelWidget::setIcon(const QIcon &icon)
{

    // DDciIcon dciIcon = DDciIcon::fromTheme(icon.name());
    // if(dciIcon.isNull()){
        m_iconWidget->setIcon(icon);
    // }else{
    //     m_iconWidget->setIcon(dciIcon);
    // }
    // m_iconWidget->setIcon(icon);
    // // "network-wireless-6-offline-secure-symbolic"
    // DDciIcon::fromTheme("network-wireless-6-offline-secure-symbolic");
    // m_iconWidget->setIcon(DDciIcon::fromTheme("network-wireless-6-offline-secure-symbolic"));
    // DDciIcon::fromTheme("network-wireless-6-offline-secure-symbolic");
    // m_iconWidget->setIcon(QIcon::fromTheme("network-wireless-6-offline-secure-symbolic"));
    // network-wireless-6-signal-full-secure-symbolic_16px.svg
}

void QuickPanelWidget::setText(const QString &text)
{
    m_nameLabel->setText(text);
}

void QuickPanelWidget::setDescription(const QString &description)
{
    m_stateLabel->setText(description);
}

void QuickPanelWidget::setActive(bool active)
{
    m_iconWidget->setBackgroundRole(active ? QPalette::Highlight : QPalette::BrightText);
}

void QuickPanelWidget::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        m_clickPoint = event->pos();
        break;
    default:
        break;
    }
    QWidget::mousePressEvent(event);
}

void QuickPanelWidget::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::LeftButton:
        if (m_clickPoint == event->pos())
            Q_EMIT panelClicked();
        break;
    default:
        break;
    }
    m_clickPoint = QPoint();
    return QWidget::mouseReleaseEvent(event);
}

void QuickPanelWidget::initUi()
{
    QWidget *labelWidget = new QWidget(this);

    DFontSizeManager::instance()->bind(m_nameLabel, DFontSizeManager::T6, QFont::Normal);
    m_nameLabel->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_nameLabel, DToolTip::ShowWhenElided);
    m_nameLabel->setContentsMargins(0, 2, 0, 0);
    m_nameLabel->setForegroundRole(QPalette::BrightText);

    DFontSizeManager::instance()->bind(m_stateLabel, DFontSizeManager::T10);
    m_stateLabel->setElideMode(Qt::ElideRight);
    DToolTip::setToolTipShowMode(m_stateLabel, DToolTip::ShowWhenElided);

    QVBoxLayout *layout = new QVBoxLayout(labelWidget);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(0);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_stateLabel);

    // 图标
    m_iconWidget->setEnabledCircle(true);
    m_iconWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_iconWidget->setIconSize(IconSize);
    m_iconWidget->setCheckable(false);
    m_iconWidget->setFixedSize(QSize(40, 40));
    m_iconWidget->setFocusPolicy(Qt::NoFocus);

    // 进入图标
    m_expandLabel->setIcon(DStyle::standardIcon(style(), DStyle::SP_ArrowEnter));
    m_expandLabel->setFlat(true);
    m_expandLabel->setFocusPolicy(Qt::NoFocus);

    m_expandLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 0, 10, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_iconWidget);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(labelWidget);
    mainLayout->addStretch(1);
    mainLayout->addWidget(m_expandLabel);

    setActive(true);
    setFixedSize(150, 60);
}

void QuickPanelWidget::initConnection()
{
    connect(m_iconWidget, &DFloatingButton::clicked, this, &QuickPanelWidget::iconClicked);
}

} // namespace network
} // namespace dde
