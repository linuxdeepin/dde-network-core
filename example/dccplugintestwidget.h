// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DCCPLUGINTESTWIDGET_H
#define DCCPLUGINTESTWIDGET_H

#include <QWidget>

class DccPluginTestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DccPluginTestWidget(QWidget *parent = nullptr);
    ~DccPluginTestWidget() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();
};

#endif // DCCPLUGINTESTWIDGET_H