// Copyright (C) 2022 ~ 2026 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef QUICKPANELWIDGET_H
#define QUICKPANELWIDGET_H

#include <DFloatingButton>
#include <DLabel>

#include <QWidget>
#include <QGraphicsOpacityEffect>

class QLabel;

namespace dde {
namespace network {
class QuickButton;

class QuickPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuickPanelWidget(QWidget *parent = nullptr);
    ~QuickPanelWidget() override;

public Q_SLOTS:
    void setIcon(const QIcon &icon);
    void setText(const QString &text);
    void setDescription(const QString &description);
    void setActive(bool active);

Q_SIGNALS:
    void panelClicked();
    void iconClicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void initUi();
    void initConnection();

private:
    QuickButton *m_iconWidget;
    Dtk::Widget::DLabel *m_nameLabel;
    Dtk::Widget::DLabel *m_stateLabel;
    Dtk::Widget::DIconButton *m_expandLabel;
    QGraphicsOpacityEffect *m_iconOpacityEffect;
    QGraphicsOpacityEffect *m_nameOpacityEffect;
    QGraphicsOpacityEffect *m_stateOpacityEffect;
    bool m_hover;
    bool m_active;
    static constexpr qreal kNormalOpacity = 0.7;
    static constexpr qreal kHoverOpacity = 1.0;
    QPoint m_clickPoint;
};
} // namespace network
} // namespace dde
#endif // QUICKPANELWIDGET_H
