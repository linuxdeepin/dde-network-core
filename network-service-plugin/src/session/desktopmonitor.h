// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DESKTOPMONITOR_H
#define DESKTOPMONITOR_H

#include <QObject>

class QTimer;

namespace network {
namespace sessionservice {
/**
 * @brief The StartMonitor class
 * 用于监控环境是否准备好，桌面环境准备好了，才能进行下一步的动作
 */
class DesktopMonitor : public QObject
{
    Q_OBJECT

public:
    DesktopMonitor(QObject *parent = Q_NULLPTR);
    ~DesktopMonitor() = default;
    bool prepared() const;               // 环境是否准备好
    QString displayEnvironment() const;  // DISPLAY环境变量

signals:
    void desktopChanged(bool);           // 桌面环境变化发出的信号,true为进入桌面后并且环境准备好了，false为注销当前环境

private:
    void init();
    QString getDisplayEnvironment() const;
    void checkFinished();

private slots:
    void onCheckTimeout();
    void onServiceRegistered(const QString &service);
    void onPrepareLogout();

private:
    bool m_prepare;
    bool m_checkFinished;
    QString m_displayEnvironment;
    QTimer *m_timer;
};

}
}

#endif
