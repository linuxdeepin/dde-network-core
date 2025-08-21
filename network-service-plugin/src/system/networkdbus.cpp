// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkdbus.h"

#include "networkthread.h"

#include <QDBusMessage>

namespace network {
namespace systemservice {

NetworkDBus::NetworkDBus(QDBusConnection &dbusConnection, QObject *parent)
    : QObject(parent)
    , m_networkThread(new NetworkThread(dbusConnection))
{
    connect(m_networkThread.get(), &NetworkThread::DeviceEnabled, this, &NetworkDBus::DeviceEnabled);
}

NetworkDBus::~NetworkDBus() { }

bool NetworkDBus::VpnEnabled() const
{
    return m_networkThread->VpnEnabled();
}

void NetworkDBus::setVpnEnabled(bool enabled)
{
    QMetaObject::invokeMethod(m_networkThread.get(), &NetworkThread::setVpnEnabled, Qt::QueuedConnection, enabled);
}

QDBusObjectPath NetworkDBus::EnableDevice(const QString &pathOrIface, const bool enabled)
{
    setDelayedReply(true);
    QMetaObject::invokeMethod(m_networkThread.get(), &NetworkThread::EnableDevice, Qt::QueuedConnection, pathOrIface, enabled, message());
    return QDBusObjectPath();
}

bool NetworkDBus::IsDeviceEnabled(const QString &pathOrIface)
{
    setDelayedReply(true);
    QMetaObject::invokeMethod(m_networkThread.get(), &NetworkThread::IsDeviceEnabled, Qt::QueuedConnection, pathOrIface, message());
    return true;
}

void NetworkDBus::Ping(const QString &host)
{
    setDelayedReply(true);
    QMetaObject::invokeMethod(m_networkThread.get(), &NetworkThread::Ping, Qt::QueuedConnection, host, message());
}

bool NetworkDBus::ToggleWirelessEnabled()
{
    setDelayedReply(true);
    QMetaObject::invokeMethod(m_networkThread.get(), &NetworkThread::ToggleWirelessEnabled, Qt::QueuedConnection, message());
    return true;
}

} // namespace systemservice
} // namespace network
