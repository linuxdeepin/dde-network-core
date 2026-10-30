// SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkdbus.h"

#include "networkthread.h"
#include "nmaccess.h"

#include <QDBusContext>
#include <QDBusMessage>

namespace network {
namespace systemservice {

NetworkDBus::NetworkDBus(QDBusConnection &dbusConnection, QObject *parent)
    : QObject(parent)
    , m_networkThread(new NetworkThread(dbusConnection))
{
    connect(m_networkThread.data(), &NetworkThread::DeviceEnabled, this, &NetworkDBus::DeviceEnabled);
}

NetworkDBus::~NetworkDBus()
{
    NetworkThread *thread = m_networkThread.data();
    if (!thread) {
        return;
    }
    m_networkThread = nullptr;
    // NetworkThread 归属 NM 线程。必须同步删除：它持有 NetworkManager 的
    // QDBusServiceWatcher 和 VpnStateChanged 订阅，异步 deleteLater 会让这些
    // 订阅在本插件卸载后仍可能回调到已卸载的代码。
    if (isNetworkManagerThread()) {
        delete thread;
        return;
    }
    QMetaObject::invokeMethod(networkManagerContext(), [thread] { delete thread; }, Qt::BlockingQueuedConnection);
}

// D-Bus 入口运行在插件线程；NetworkThread 可能已在上面的析构路径中消失。
// setDelayedReply() 之后若不回复，客户端会一直挂到超时，因此每个入口都要兜底。
bool NetworkDBus::replyIfThreadGone()
{
    if (m_networkThread) {
        return false;
    }
    connection().send(message().createErrorReply(QDBusError::Failed, "network service is shutting down"));
    return true;
}

bool NetworkDBus::VpnEnabled() const
{
    if (!m_networkThread) {
        return false;
    }
    return m_networkThread->VpnEnabled();
}

void NetworkDBus::setVpnEnabled(bool enabled)
{
    if (!m_networkThread) {
        return;
    }
    QMetaObject::invokeMethod(m_networkThread.data(), &NetworkThread::setVpnEnabled, Qt::QueuedConnection, enabled);
}

QDBusObjectPath NetworkDBus::EnableDevice(const QString &pathOrIface, const bool enabled)
{
    setDelayedReply(true);
    if (replyIfThreadGone()) {
        return QDBusObjectPath();
    }
    QMetaObject::invokeMethod(m_networkThread.data(), &NetworkThread::EnableDevice, Qt::QueuedConnection, pathOrIface, enabled, message());
    return QDBusObjectPath();
}

bool NetworkDBus::IsDeviceEnabled(const QString &pathOrIface)
{
    setDelayedReply(true);
    if (replyIfThreadGone()) {
        return true;
    }
    QMetaObject::invokeMethod(m_networkThread.data(), &NetworkThread::IsDeviceEnabled, Qt::QueuedConnection, pathOrIface, message());
    return true;
}

void NetworkDBus::Ping(const QString &host)
{
    setDelayedReply(true);
    if (replyIfThreadGone()) {
        return;
    }
    QMetaObject::invokeMethod(m_networkThread.data(), &NetworkThread::Ping, Qt::QueuedConnection, host, message());
}

bool NetworkDBus::ToggleWirelessEnabled()
{
    setDelayedReply(true);
    if (replyIfThreadGone()) {
        return true;
    }
    QMetaObject::invokeMethod(m_networkThread.data(), &NetworkThread::ToggleWirelessEnabled, Qt::QueuedConnection, message());
    return true;
}

} // namespace systemservice} // namespace network