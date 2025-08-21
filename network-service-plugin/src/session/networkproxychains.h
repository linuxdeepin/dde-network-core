// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKPROXYCHAINS_H
#define NETWORKPROXYCHAINS_H

#include <QDBusConnection>
#include <QDBusContext>
#include <QObject>

class QDBusInterface;
class QGSettings;

namespace network {
namespace sessionservice {
class NetworkStateHandler;

class NetworkProxyChains : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.Network1.ProxyChains")
    Q_PROPERTY(bool Enabled READ Enabled)
    Q_PROPERTY(QString IP READ IP)
    Q_PROPERTY(QString Password READ Password)
    Q_PROPERTY(QString Type READ Type)
    Q_PROPERTY(QString User READ User)
    Q_PROPERTY(uint Port READ Port)
public:
    explicit NetworkProxyChains(QDBusConnection &dbusConnection, NetworkStateHandler *networkStateHandler, QObject *parent = nullptr);

    bool Enabled() const;
    QString IP() const;
    QString Password() const;
    QString Type() const;
    QString User() const;
    uint Port() const;

public Q_SLOTS:
    void Set(const QString &type, const QString &ip, uint port, const QString &user, const QString &password);
    void SetEnable(bool enable);

private:
    void init();
    void emitPropertyChanged(const QString &prop, QVariant value);
    bool validType(const QString &type) const;
    bool validIPv4(const QString &ip) const;
    bool validUser(const QString &user) const;
    bool validPassword(const QString &password) const;
    bool fixConfig();
    bool checkConfig() const;
    QString saveConfig() const;
    QString removeConf() const;
    QString writeConf() const;
    QString set(QString type, const QString &ip, uint port, const QString &user, const QString &password);
    QString startProxy();
    void notifyAppProxyEnabled();
    void notifyAppProxyEnableFailed();

    inline QDBusConnection dbusConnection() const { return m_dbusConnection; }

private:
    QDBusConnection &m_dbusConnection;
    bool m_enable;
    QString m_ip;
    QString m_password;
    QString m_type;
    QString m_user;
    uint m_port;

    NetworkStateHandler *m_networkStateHandler;
    QDBusInterface *m_appProxy;
    QString m_jsonFile;
    QString m_confFile;
};
} // namespace sessionservice
} // namespace network
#endif // NETWORKPROXYCHAINS_H
