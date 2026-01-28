// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DSSTESTWIDGET_H
#define DSSTESTWIDGET_H

#include <QWidget>

namespace Dtk {
  namespace Widget {
    class DFloatingButton;
  }
}

class QPushButton;

class DssTestWidget : public QWidget
{
    Q_OBJECT

public:
    DssTestWidget(QWidget *parent = Q_NULLPTR);
    ~DssTestWidget();

private:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Dtk::Widget::DFloatingButton *m_button;
};

#endif // DSSTESTWIDGET_H
