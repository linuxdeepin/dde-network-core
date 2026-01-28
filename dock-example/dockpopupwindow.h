// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCKPOPUPWINDOW_H
#define DOCKPOPUPWINDOW_H

#include <QWidget>

#include <DArrowRectangle>
#include <DRegionMonitor>
#include <DWindowManagerHelper>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class DockPopupWindow : public Dtk::Widget::DArrowRectangle
{
    Q_OBJECT

public:
    explicit DockPopupWindow(QWidget *parent = nullptr);
    ~DockPopupWindow() override;
    bool model() const;
    void setContent(QWidget *content);
public slots:
    void show(const QPoint &pos, bool model = false);
    void show(int x, int y) override;
    void hide();
signals:
    void accept();
    // 在把专业版的仓库降级到debian的stable时, dock出现了一个奇怪的问题:
    // 在plugins/tray/system-trays/systemtrayitem.cpp中的showPopupWindow函数中
    // 无法连接到上面这个信号: "accept", qt给出一个运行时警告提示找不到信号
    // 目前的解决方案就是在下面增加了这个信号
    void unusedSignal();
protected:
    void showEvent(QShowEvent *e) override;
    void enterEvent(QEnterEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;
    void blockButtonRelease();
private slots:
    void onGlobMouseRelease(const QPoint &mousePos, int flag);
    void compositeChanged();
    void ensureRaised();
private:
    bool m_model;
    QPoint m_lastPoint;
    DRegionMonitor *m_regionInter;
    DWindowManagerHelper *m_wmHelper;
    bool m_enableMouseRelease;
};

#endif // DOCKPOPUPWINDOW_H
