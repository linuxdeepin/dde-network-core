// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COM_DEEPIN_DAEMON_NETWORK_PROXYCHAINS_H
#define COM_DEEPIN_DAEMON_NETWORK_PROXYCHAINS_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <DDBusInterface>
#include <QtDBus/QtDBus>

/*
 * Proxy class for interface com.deepin.daemon.Network.ProxyChains
 */
class __ProxyChainsPrivate;
class ProxyChains : public Dtk::Core::DDBusInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "org.deepin.dde.Network1.ProxyChains"; }

public:
    explicit ProxyChains(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~ProxyChains();
    void setSync(bool sync);
    Q_PROPERTY(bool Enable READ enable NOTIFY EnableChanged)
    bool enable();

    Q_PROPERTY(QString IP READ iP NOTIFY IPChanged)
    QString iP();

    Q_PROPERTY(QString Password READ password NOTIFY PasswordChanged)
    QString password();

    Q_PROPERTY(uint Port READ port NOTIFY PortChanged)
    uint port();

    Q_PROPERTY(QString Type READ type NOTIFY TypeChanged)
    QString type();

    Q_PROPERTY(QString User READ user NOTIFY UserChanged)
    QString user();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Set(const QString &in0, const QString &in1, uint in2, const QString &in3, const QString &in4)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1) << QVariant::fromValue(in2) << QVariant::fromValue(in3) << QVariant::fromValue(in4);
        return asyncCallWithArgumentList(QStringLiteral("Set"), argumentList);
    }

    inline void SetQueued(const QString &in0, const QString &in1, uint in2, const QString &in3, const QString &in4)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0) << QVariant::fromValue(in1) << QVariant::fromValue(in2) << QVariant::fromValue(in3) << QVariant::fromValue(in4);

        CallQueued(QStringLiteral("Set"), argumentList);
    }


    inline QDBusPendingReply<> SetEnable(bool in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("SetEnable"), argumentList);
    }

    inline void SetEnableQueued(bool in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);

        CallQueued(QStringLiteral("SetEnable"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    // begin property changed signals
    void EnableChanged(bool  value);
    void IPChanged(const QString & value);
    void PasswordChanged(const QString & value);
    void PortChanged(uint  value);
    void TypeChanged(const QString & value);
    void UserChanged(const QString & value);

public Q_SLOTS:
    void CallQueued(const QString &callName, const QList<QVariant> &args);

private Q_SLOTS:
    void onPendingCallFinished(QDBusPendingCallWatcher *w);
    void onPropertyChanged(const QString &propName, const QVariant &value);

private:
    __ProxyChainsPrivate *d_ptr;
};

#endif
