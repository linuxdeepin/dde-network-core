// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "sessioncontainer.h"
#include "sessionipconfilct.h"
#include "constants.h"
#include "desktopmonitor.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcessEnvironment>
#include <QProcess>

using namespace network::sessionservice;

static QString networkService = "org.deepin.dde.Network1";
static QString networkPath = "/org/deepin/service/SystemNetwork";
static QString networkInterface = "org.deepin.service.SystemNetwork";

SessionContainer::SessionContainer(QObject *parent)
    : QObject (parent)
    , m_ipConflictHandler(new SessionIPConflict(this))
    , m_desktopMonitor(new DesktopMonitor(this))
{
    initConnection();
    initEnvornment();
}

SessionContainer::~SessionContainer()
{
}

SessionIPConflict *SessionContainer::ipConfilctedChecker() const
{
    return m_ipConflictHandler;
}

void SessionContainer::initConnection()
{
    QDBusConnection::systemBus().connect(networkService, networkPath, networkInterface, "IpConflictChanged", this, SLOT(onIPConflictChanged(const QString &, const QString &, bool)));
    QDBusConnection::systemBus().connect(networkService, networkPath, networkInterface, "PortalDetected", this, SLOT(onPortalDetected(const QString &)));
    // 当系统代理发生变化的时候需要主动调用SystemNetwork服务的检查网络连通性的接口
    QDBusConnection::sessionBus().connect("org.deepin.dde.Network1", "/org/deepin/dde/Network1", "org.deepin.dde.Network1", "ProxyMethodChanged", this, SLOT(onProxyMethodChanged(const QString &)));
    connect(m_desktopMonitor, &DesktopMonitor::desktopChanged, this, &SessionContainer::onDesktopChanged);
}

void SessionContainer::checkPortalUrl()
{
    // 检测初始化的状态是否为门户认证，这种情况下需要先打开Url
    QDBusInterface dbusInter("org.deepin.service.SystemNetwork", "/org/deepin/service/SystemNetwork", "org.deepin.service.SystemNetwork", QDBusConnection::systemBus());
    network::service::Connectivity connectivity = static_cast<network::service::Connectivity>(dbusInter.property("Connectivity").toInt());
    if (connectivity == network::service::Connectivity::Portal) {
        // 获取需要认证的网站的信息，并打开网页
        QString url = dbusInter.property("PortalUrl").toString();
        qCDebug(DSM) << "check portal url:" << url;
        openPortalUrl(url);
    }
}

void SessionContainer::initEnvornment()
{
    if (m_desktopMonitor->prepared())
        enterDesktop();
}

void SessionContainer::enterDesktop()
{
    qCInfo(DSM) << "enter desktop";
    // 进入桌面，检查本地portal连接
    checkPortalUrl();
}

void SessionContainer::leaveDesktop()
{
    qCInfo(DSM) << "leave desktop";
    // TODO: 注销后离开桌面
}

void SessionContainer::openPortalUrl(const QString &url)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString displayEnvironment = m_desktopMonitor->displayEnvironment();
    if (!displayEnvironment.isEmpty())
        env.insert("DISPLAY", displayEnvironment);

    QProcess process;
    process.setProcessEnvironment(env);
    process.start("xdg-open", QStringList() << url);
    process.waitForFinished();
}

void SessionContainer::onIPConflictChanged(const QString &devicePath, const QString &ip, bool conflicted)
{
    // TODO: 这里用于处理IP冲突，例如检测到IP冲突后，会给出提示的消息，等后期将任务栏种的相关处理删除后，再到这里来处理，目前此处暂时保留
    Q_UNUSED(devicePath)
    Q_UNUSED(ip)
    Q_UNUSED(conflicted)
}

void SessionContainer::onPortalDetected(const QString &url)
{
    if (!m_desktopMonitor->prepared()) {
        qCWarning(DSM) << "desktop is not login in";
        return;
    }

    qCDebug(DSM) << "detacted portal url" << url;
    openPortalUrl(url);
}

void SessionContainer::onProxyMethodChanged(const QString &method)
{
    Q_UNUSED(method);

    QDBusInterface dbusInter("org.deepin.service.SystemNetwork", "/org/deepin/service/SystemNetwork", "org.deepin.service.SystemNetwork", QDBusConnection::systemBus());
    QDBusPendingCall callReply = dbusInter.asyncCall("CheckConnectivity");
    callReply.waitForFinished();
}

void SessionContainer::onDesktopChanged(bool isLogin)
{
    isLogin ? enterDesktop() : leaveDesktop();
}
