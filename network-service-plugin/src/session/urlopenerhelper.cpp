// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "urlopenerhelper.h"
#include "constants.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QCoreApplication>

using namespace network::sessionservice;

#define SESSION_MANAG_SERVICE "com.deepin.SessionManager"

UrlOpenerHelper::UrlOpenerHelper(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_timer(nullptr)
    , m_startManagerIsPrepare(false)
    , m_checkComplete(false)
{
    init();
}

UrlOpenerHelper::~UrlOpenerHelper()
{
}

void UrlOpenerHelper::openUrl(const QString &url)
{
    if (url.isEmpty())
        return;

    static UrlOpenerHelper urlHelper;
    if (urlHelper.isPrepare()) {
        urlHelper.openUrlAddress(url);
    } else if (!urlHelper.m_cacheUrls.contains(url)) {
        urlHelper.m_cacheUrls << url;
    }
}

void UrlOpenerHelper::openUrlAddress(const QString &url)
{
    // 调用xdg-open来打开网页
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    if (!m_displayEnvironment.isEmpty())
        env.insert("DISPLAY", m_displayEnvironment);

    if (m_process->isOpen()) {
        m_process->close();
    }
    m_process->setProcessEnvironment(env);
    m_process->start("xdg-open", QStringList() << url);
    m_process->waitForFinished();
}

QString UrlOpenerHelper::getDisplayEnvironment() const
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

void UrlOpenerHelper::init()
{
    m_startManagerIsPrepare = QDBusConnection::sessionBus().interface()->isServiceRegistered(SESSION_MANAG_SERVICE);
    if (m_startManagerIsPrepare) {
        m_displayEnvironment = getDisplayEnvironment();
        qCDebug(DSM) << "get display environment" << m_displayEnvironment;
        if (m_displayEnvironment.isEmpty()) {
            m_timer = new QTimer(this);
            m_timer->setInterval(1000);
            m_timer->setProperty("checktime", 0);
            connect(m_timer, &QTimer::timeout, this, &UrlOpenerHelper::onCheckTimeout);
            m_timer->start();
        }
    } else {
        // 如果服务未启动，则等待服务启动
        qCWarning(DSM) << SESSION_MANAG_SERVICE << "service is not register, wait it for start";
        QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
        serviceWatcher->setConnection(QDBusConnection::sessionBus());
        serviceWatcher->addWatchedService(SESSION_MANAG_SERVICE);
        connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &UrlOpenerHelper::onServiceRegistered);
    }
}

bool UrlOpenerHelper::isPrepare() const
{
    return m_startManagerIsPrepare && (!m_displayEnvironment.isEmpty() || m_checkComplete);
}

void UrlOpenerHelper::onCheckTimeout()
{
    auto doOpenPortal = [this] {
        for (const QString &url : m_cacheUrls) {
            openUrlAddress(url);
        }
        m_checkComplete = true;
        m_cacheUrls.clear();
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    };

    m_displayEnvironment = getDisplayEnvironment();
    if (m_displayEnvironment.isEmpty()) {
        // 连接检测20次，如果20次还是没有这个环境变量，就说明这个环境变量有问题，直接尝试打开
        int checktime = m_timer->property("checktime").toInt();
        qCDebug(DSM) << "in " << checktime << "times, the display environment is empty";
        if (checktime >= 20) {
            doOpenPortal();
        } else {
            checktime++;
            m_timer->setProperty("checktime", checktime);
        }
    } else if (!m_displayEnvironment.isEmpty()) {
        qCDebug(DSM) << "the display environment is not empty" << m_displayEnvironment << ", open the url" << m_cacheUrls;
        doOpenPortal();
    }
}

void UrlOpenerHelper::onServiceRegistered(const QString &service)
{
    if (service != SESSION_MANAG_SERVICE)
        return;

    m_startManagerIsPrepare = true;
    m_displayEnvironment = getDisplayEnvironment();
    qCInfo(DSM) << SESSION_MANAG_SERVICE << "start success, display envionment:" << m_displayEnvironment << "urls:" << m_cacheUrls;
    for (const QString &url : m_cacheUrls) {
        openUrlAddress(url);
    }
}
