// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DSSTESTWIDGET_H
#define DSSTESTWIDGET_H

#include <DFloatingButton>

#include <QWidget>

class PopupWindow;

namespace dde {
namespace network {
class NetworkPlugin;
} // namespace network
} // namespace dde

class QPushButton;
using namespace Dtk::Widget;

class DssTestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DssTestWidget(dde::network::NetworkPlugin *networkPlugin, QWidget *parent = Q_NULLPTR);
    ~DssTestWidget() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

protected slots:
    void onClickButton();

private:
    dde::network::NetworkPlugin *m_pModule;
    DFloatingButton *m_iconButton;
    PopupWindow *m_container;
    PopupWindow *m_tipContainer;
};

#endif // DSSTESTWIDGET_H
