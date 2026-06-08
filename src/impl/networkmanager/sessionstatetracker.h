// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SESSIONSTATETRACKER_H
#define SESSIONSTATETRACKER_H

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDebug>
#include <QObject>
#include <QVariantMap>

class SessionStateTracker : public QObject {
    Q_OBJECT
public:
    static SessionStateTracker *instance() {
        static SessionStateTracker *s_instance = new SessionStateTracker();
        return s_instance;
    }

    bool isSessionActive() const {
        return m_userActive;
    }

    bool isLocalUser() const {
        return m_isLocal;
    }

private:
    SessionStateTracker()
        : QObject(nullptr)
        , m_userActive(true)
        , m_isLocal(false)
    {
        // Step 1: 异步获取 User.Display（session 路径）
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.freedesktop.login1",
            "/org/freedesktop/login1/user/self",
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        msg << "org.freedesktop.login1.User" << "Display";
        QDBusConnection::systemBus().callWithCallback(msg, this, SLOT(onDisplayReady(QDBusVariant)));
    }

    ~SessionStateTracker() = default;
    SessionStateTracker(const SessionStateTracker &) = delete;
    SessionStateTracker &operator=(const SessionStateTracker &) = delete;

private slots:
    void onDisplayReady(const QDBusVariant &display) {
        const QDBusArgument arg = display.variant().value<QDBusArgument>();
        QString id;
        QDBusObjectPath path;
        arg.beginStructure();
        arg >> id >> path;
        arg.endStructure();

        if (path.path().isEmpty()) {
            qWarning() << "[SessionStateTracker] Display empty, keep default active";
            return;
        }

        // Step 2: 检查 Remote 属性
        QDBusMessage remoteMsg = QDBusMessage::createMethodCall(
            "org.freedesktop.login1",
            path.path(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        remoteMsg << "org.freedesktop.login1.Session" << "Remote";
        QDBusPendingReply<QDBusVariant> remoteReply = QDBusConnection::systemBus().asyncCall(remoteMsg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(remoteReply, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [path, this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusVariant> reply = *self;
            bool remote = !reply.isError() && reply.value().variant().toBool();
            if (!remote) {
                m_isLocal = true;
                // Step 3: 监听 Active 变化
                QDBusConnection::systemBus().connect(
                    "org.freedesktop.login1",
                    path.path(),
                    "org.freedesktop.DBus.Properties",
                    "PropertiesChanged",
                    this,
                    SLOT(onSessionPropertiesChanged(QString, QVariantMap, QStringList))
                );
                // 同时读一次初始 Active 状态
                QDBusMessage activeMsg = QDBusMessage::createMethodCall(
                    "org.freedesktop.login1",
                    path.path(),
                    "org.freedesktop.DBus.Properties",
                    "Get"
                );
                activeMsg << "org.freedesktop.login1.Session" << "Active";
                QDBusPendingReply<QDBusVariant> activeReply = QDBusConnection::systemBus().asyncCall(activeMsg);
                QDBusPendingCallWatcher *activeWatcher = new QDBusPendingCallWatcher(activeReply, this);
                connect(activeWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *sw) {
                    QDBusPendingReply<QDBusVariant> sr = *sw;
                    if (!sr.isError()) {
                        m_userActive = sr.value().variant().toBool();
                    }
                    sw->deleteLater();
                });
            }
            self->deleteLater();
        });
    }

    void onSessionPropertiesChanged(const QString &interface, const QVariantMap &properties, const QStringList &) {
        if (interface == "org.freedesktop.login1.Session" && properties.contains("Active")) {
            m_userActive = properties.value("Active").toBool();
            if (m_userActive) {
                qWarning() << "[SessionStateTracker] session resumed";
                Q_EMIT sessionResumed();
            } else {
                qWarning() << "[SessionStateTracker] session left";
            }
        }
    }

signals:
    void sessionResumed();

private:
    bool m_userActive;
    bool m_isLocal;
};

#endif // SESSIONSTATETRACKER_H