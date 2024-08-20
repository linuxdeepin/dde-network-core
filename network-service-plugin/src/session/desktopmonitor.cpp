// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopmonitor.h"
#include "constants.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QTimer>

using namespace network::sessionservice;

#define SESSION_MANAG_SERVICE "com.deepin.SessionManager"

DesktopMonitor::DesktopMonitor(QObject *parent)
    : QObject (parent)
    , m_prepare(false)
    , m_checkFinished(false)
    , m_timer(nullptr)
{
    init();
}

bool DesktopMonitor::prepared() const
{
    return m_prepare && (!m_displayEnvironment.isEmpty() || m_checkFinished);
}

QString DesktopMonitor::displayEnvironment() const
{
    return m_displayEnvironment;
}

void DesktopMonitor::init()
{
    m_prepare = QDBusConnection::sessionBus().interface()->isServiceRegistered(SESSION_MANAG_SERVICE);
    qCDebug(DSM) << SESSION_MANAG_SERVICE << " prepare status" << m_prepare;
    if (m_prepare) {
        m_displayEnvironment = getDisplayEnvironment();
        qCDebug(DSM) << "get display environment" << m_displayEnvironment;
        if (m_displayEnvironment.isEmpty()) {
            m_timer = new QTimer(this);
            m_timer->setInterval(1000);
            m_timer->setProperty("checktime", 0);
            connect(m_timer, &QTimer::timeout, this, &DesktopMonitor::onCheckTimeout);
            m_timer->start();
        }
    }

    // 就算注销当前用户后，deepin-service-manager服务也不会退出，所以这里需要监控该服务的退出和启动的状态
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
    serviceWatcher->setConnection(QDBusConnection::sessionBus());
    serviceWatcher->addWatchedService(SESSION_MANAG_SERVICE);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &DesktopMonitor::onServiceRegistered);
    QDBusConnection::sessionBus().connect("com.deepin.SessionManager", "/com/deepin/SessionManager", "com.deepin.SessionManager",
                                          "PrepareLogout", this, SLOT(onPrepareLogout()));
}

QString DesktopMonitor::getDisplayEnvironment() const
{
    QString displayenv = qgetenv("DISPLAY");
    if (!displayenv.isEmpty()) {
        qCDebug(DSM) << "get DISPLAY Environment from local env";
        return displayenv;
    }

    qCDebug(DSM) << "get DISPLAY Environment from systemd1 service";
    // 有时候获取到的环境变量为空，此时就需要从systemd接口中来获取环境变量了
    QDBusInterface systemdInterface("org.freedesktop.systemd1", "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager");
    QStringList environments = systemdInterface.property("Environment").toStringList();
    for (const QString &env : environments) {
        QStringList envArray = env.split("=");
        if (envArray.size() < 2)
            continue;
        if (envArray.first() == "DISPLAY")
            return envArray[1];
    }

    return QString();
}

void DesktopMonitor::checkFinished()
{
    m_checkFinished = true;
    m_timer->stop();
    m_timer->deleteLater();
    m_timer = nullptr;
    if (prepared()) {
        qCDebug(DSM) << "display environment check finished";
        emit desktopChanged(true);
    }
}

void DesktopMonitor::onCheckTimeout()
{
    m_displayEnvironment = getDisplayEnvironment();
    if (m_displayEnvironment.isEmpty()) {
        // 连接检测20次，如果20次还是没有这个环境变量，就说明这个环境变量有问题，直接尝试打开
        int checktime = m_timer->property("checktime").toInt();
        qCDebug(DSM) << "in " << checktime << "times, the display environment is empty";
        if (checktime >= 20) {
            checkFinished();
        } else {
            checktime++;
            m_timer->setProperty("checktime", checktime);
        }
    } else {
        qCDebug(DSM) << "the display environment is not empty" << m_displayEnvironment;
        checkFinished();
    }
}

void DesktopMonitor::onServiceRegistered(const QString &service)
{
    if (service != SESSION_MANAG_SERVICE)
        return;

    if (m_prepare && !m_displayEnvironment.isEmpty()) {
        // 如果之前是已经准备好的，无需再次发送信号，这种情况一般发生在kill掉startdde进程或者重启SessionManager服务
        qCDebug(DSM) << "the prepare is true";
        return;
    }

    m_prepare = true;
    m_displayEnvironment = getDisplayEnvironment();
    qCInfo(DSM) << SESSION_MANAG_SERVICE << "start success, display envionment:" << m_displayEnvironment;
    if (prepared()) {
        qCDebug(DSM) << "desktop prepared";
        emit desktopChanged(true);
    }
}

void DesktopMonitor::onPrepareLogout()
{
    // 注销当前账户
    m_prepare = false;
    qCDebug(DSM) << "current user is logout";
    emit desktopChanged(false);
}
