// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "constants.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QFile>

namespace network {
namespace service {
void dbusDebug(const QString &service, const QString &funName, const QDBusConnection &dbusConnection)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetConnectionUnixProcessID");
    msg << service;
    QDBusPendingReply<uint> reply = dbusConnection.asyncCall(msg);
    if (reply.isError()) {
        qWarning() << "API" << funName << "is called by" << service;
        qWarning() << funName << "error:" << reply.error();
        return;
    }
    uint pid = reply.value();
    QFile file(QString("/proc/%1/cmdline").arg(pid));
    if (file.open(QFile::ReadOnly)) {
        QByteArray cmd = file.readAll();
        qWarning() << "API" << funName << "is called by" << service << "(" << cmd.split('\0').join(" ") << ")";
    }
}
} // namespace service
} // namespace network
