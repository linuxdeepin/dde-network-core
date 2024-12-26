// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "proxycontroller.h"

#include <QDBusConnection>
#include <QDBusInterface>

const static QString networkService     = "com.deepin.daemon.Network";
const static QString networkPath        = "/com/deepin/daemon/Network";
const static QString networkInterface   = "com.deepin.daemon.Network";

using namespace dde::network;

ProxyController::ProxyController(QObject *parent)
    : QObject(parent)
    , m_networkInter(new NetworkInter(networkService, networkPath, QDBusConnection::sessionBus(), this))
    , m_proxyMethod(ProxyMethod::Init)
    , m_systemProxyExist(false)
{
    Q_ASSERT(m_networkInter);
    // 判断是否存在proxychains4来决定是否存在应用代理
    m_appProxyExist = !QStandardPaths::findExecutable("proxychains4").isEmpty();
    QDBusConnection::sessionBus().connect(networkService, networkPath, networkInterface, "ProxyMethodChanged", this, SLOT(onProxyMethodChanged(const QString&)));

    connect(m_networkInter, &NetworkInter::serviceValidChanged, this, [this](bool valid) {
        // 设置快速登录，重启后，锁屏的网络插件进行初始化，但是dbus服务无效,导致系统代理等设置异常，等dbus有效后，重新同步数据
        if (valid) {
            querySysProxyData();
        }
    });
}

ProxyController::~ProxyController() = default;

void ProxyController::onProxyMethodChanged(const QString &method)
{
    ProxyMethod value = convertProxyMethod(method);
    if (value != m_proxyMethod) {
        m_proxyMethod = value;
        Q_EMIT proxyMethodChanged(value);
    }
    // 如果没有配置，则关掉代理，避免升级问题
    bool isConfExist = false;
    for (auto conf : m_sysProxyConfig) {
        if (!conf.url.isEmpty()) {
            isConfExist = true;
            break;
        }
    }
    bool systemProxyExist = isConfExist || !m_autoProxyURL.isEmpty();
    if (m_systemProxyExist != systemProxyExist) {
        m_systemProxyExist = systemProxyExist;
        Q_EMIT systemProxyExistChanged(m_systemProxyExist);
    }
}

void ProxyController::setProxyMethod(const ProxyMethod &pm)
{
    // 设置代理模式，手动模式，自动模式和关闭代理
    QString methodName = convertProxyMethod(pm);
    auto *w = new QDBusPendingCallWatcher(m_networkInter->SetProxyMethod(methodName), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        queryProxyMethod();
    });
}

void ProxyController::setProxyIgnoreHosts(const QString &hosts)
{
    auto *w = new QDBusPendingCallWatcher(m_networkInter->SetProxyIgnoreHosts(hosts), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ & ] {
        queryProxyIgnoreHosts();
    });
}

void ProxyController::setAutoProxy(const QString &proxy)
{
    auto *w = new QDBusPendingCallWatcher(m_networkInter->SetAutoProxy(proxy), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        queryAutoProxy();
    });
}

void ProxyController::setProxy(const SysProxyType &type, const QString &addr, const QString &port)
{
    QString uType = convertSysProxyType(type);
    auto *w = new QDBusPendingCallWatcher(m_networkInter->SetProxy(uType, addr, port), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        queryProxyDataByType(uType);
    });
}

void ProxyController::setProxyAuth(const SysProxyType &type, const QString &userName, const QString &password, const bool enable)
{
    QString uType = convertSysProxyType(type);
    auto *w = new QDBusPendingCallWatcher(m_networkInter->asyncCall("SetProxyAuthentication", uType, userName, password, enable), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ](QDBusPendingCallWatcher * self) {
        Q_UNUSED(self);

        QDBusPendingReply<QString, QString> reply = w->reply();
        if (reply.isError()) return ;

        queryProxyAuthByType(uType);
    });
}

AppProxyConfig ProxyController::appProxy() const
{
    return m_appProxyConfig;
}

SysProxyConfig ProxyController::proxy(const SysProxyType &type) const
{
    // 根据列表的类型返回实际的配置
    for (SysProxyConfig config : m_sysProxyConfig) {
        if (config.type == type)
            return config;
    }

    return SysProxyConfig();
}

bool ProxyController::appProxyEnabled() const
{
    return false;
}

bool ProxyController::appProxyExist() const
{
    return m_appProxyExist;
}

void ProxyController::querySysProxyData()
{
    // 查询系统代理的数据
    m_sysProxyConfig.clear();

    static QStringList proxyTypes = {"http", "https", "ftp", "socks"};

    // 依次获取每种类型的数据，并填充到列表中
    for (const QString &type : proxyTypes) {
        queryProxyDataByType(type);
        queryProxyAuthByType(type);
    }

    // 查询自动代理
    queryAutoProxy();
    // 查询代理模式
    queryProxyMethod();
    // 查询忽略的主机
    queryProxyIgnoreHosts();
}

void ProxyController::queryAutoProxy()
{
    auto *w = new QDBusPendingCallWatcher(m_networkInter->GetAutoProxy(), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        QDBusPendingReply<QString> reply = m_networkInter->GetAutoProxy();
        QString autoProxyURL = reply.value();
        if (m_autoProxyURL != autoProxyURL) {
            m_autoProxyURL = autoProxyURL;
            Q_EMIT autoProxyChanged(autoProxyURL);
        }
    });
}

QString ProxyController::convertProxyMethod(const ProxyMethod &method)
{
    switch (method) {
    case ProxyMethod::Auto:    return "auto";
    case ProxyMethod::Manual:  return "manual";
    default: break;
    }

    return "none";
}

QString ProxyController::convertSysProxyType(const SysProxyType &type)
{
    switch (type) {
    case SysProxyType::Ftp:    return "ftp";
    case SysProxyType::Http:   return "http";
    case SysProxyType::Https:  return "https";
    case SysProxyType::Socks:  return "socks";
    }

    return "http";
}

SysProxyType ProxyController::convertSysProxyType(const QString &type)
{
    if (type == "ftp")
        return SysProxyType::Ftp;

    if (type == "http")
        return SysProxyType::Http;

    if (type == "https")
        return SysProxyType::Https;

    if (type == "socks")
        return SysProxyType::Socks;

    return SysProxyType::Http;
}

void ProxyController::queryProxyDataByType(const QString &type)
{
    SysProxyType uType = convertSysProxyType(type);
    auto *w = new QDBusPendingCallWatcher(m_networkInter->asyncCall("GetProxy", type), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ](QDBusPendingCallWatcher * self) {
        Q_UNUSED(self);
        QDBusPendingReply<QString, QString> reply = w->reply();
        if (!reply.isValid()) {
            qCWarning(DNC) << "Dbus path:" << m_networkInter->path() << ". Method GetProxy return value error !" << reply.error();
            return;
        }

        bool finded = false;
        // 先查找原来是否存在响应的代理，如果存在，就直接更新最新的数据
        for (SysProxyConfig &conf : m_sysProxyConfig) {
            if (conf.type == uType) {
                QString url = reply.argumentAt(0).toString();
                uint port = reply.argumentAt(1).toUInt();
                if (url != conf.url || port != conf.port) {
                    conf.url = url;
                    conf.port = port;
                    Q_EMIT proxyChanged(conf);
                }
                finded = true;
                break;
            }
        }
        // 如果不存在，直接将数据放入内存中
        if (!finded) {
            SysProxyConfig proxyConfig;
            proxyConfig.url = reply.argumentAt(0).toString();
            proxyConfig.port = reply.argumentAt(1).toUInt();
            proxyConfig.type = uType;
            m_sysProxyConfig << proxyConfig;
            Q_EMIT proxyChanged(proxyConfig);
        }
    });
}

void ProxyController::queryProxyAuthByType(const QString &type)
{
    SysProxyType uType = convertSysProxyType(type);
    auto *w = new QDBusPendingCallWatcher(m_networkInter->asyncCall("GetProxyAuthentication", type), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ](QDBusPendingCallWatcher * self) {
        Q_UNUSED(self);

        QDBusPendingReply<QString, QString> reply = w->reply();
        if (!reply.isValid()) {
            qCWarning(DNC) << "Dbus path:" << m_networkInter->path() << ". Method GetProxyAuthentication return value error !" << reply.error();
            return;
        }

        // 先查找原来是否存在响应的代理，如果存在，就直接更新最新的数据
        auto iterator = std::find_if(m_sysProxyConfig.begin(), m_sysProxyConfig.end(), [ = ](SysProxyConfig &conf) {
            if (conf.type == uType) {
                QString userName = reply.argumentAt(0).toString();
                QString password = reply.argumentAt(1).toString();
                bool enableAuth = reply.argumentAt(2).toBool();
                if (enableAuth != conf.enableAuth ||
                    userName != conf.userName ||
                    password != conf.password) {
                    conf.enableAuth = enableAuth;
                    conf.userName = userName;
                    conf.password = password;
                    Q_EMIT proxyAuthChanged(conf);
                }
                return true;
            }

            return false;
        });

        if (iterator == m_sysProxyConfig.end()) {
            SysProxyConfig proxyConfig;
            proxyConfig.userName = reply.argumentAt(0).toString();
            proxyConfig.password = reply.argumentAt(1).toString();
            proxyConfig.enableAuth = reply.argumentAt(2).toBool();
            proxyConfig.type = uType;
            m_sysProxyConfig << proxyConfig;
            Q_EMIT proxyAuthChanged(proxyConfig);
        }
    });
}

ProxyMethod ProxyController::convertProxyMethod(const QString &method)
{
    // 根据传入的字符串获取代理模式的枚举值
    if (method == "auto")
        return ProxyMethod::Auto;

    if (method == "manual")
        return ProxyMethod::Manual;

    return ProxyMethod::None;
}

void ProxyController::queryProxyMethod()
{
    // 查询代理模式
    auto *w = new QDBusPendingCallWatcher(m_networkInter->GetProxyMethod(), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        QDBusPendingReply<QString> reply = w->reply();
        if (!reply.isValid()) {
            qCWarning(DNC) << "Dbus path:" << m_networkInter->path() << ". Method GetProxyMethod return value error !" << reply.error();
            return;
        }
        onProxyMethodChanged(reply.value());
    });
}

void ProxyController::queryProxyIgnoreHosts()
{
    // 查询忽略的代理主机
    auto *w = new QDBusPendingCallWatcher(m_networkInter->GetProxyIgnoreHosts(), this);
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
    connect(w, &QDBusPendingCallWatcher::finished, this, [ = ] {
        QDBusPendingReply<QString> reply = w->reply();
        QString proxyIgnoreHosts = reply.value();
        if (m_proxyIgnoreHosts != proxyIgnoreHosts) {
            m_proxyIgnoreHosts = proxyIgnoreHosts;
            Q_EMIT proxyIgnoreHostsChanged(proxyIgnoreHosts);
        }
    });
}

void ProxyController::onIPChanged(const QString &value)
{
    // 应用代理的IP发生变化
    if (value != m_appProxyConfig.ip) {
        m_appProxyConfig.ip = value;
        Q_EMIT appIPChanged(value);
    }
}

void ProxyController::onPasswordChanged(const QString &value)
{
    // 应用代理的密码发生变化
    if (value != m_appProxyConfig.password) {
        m_appProxyConfig.password = value;
        Q_EMIT appPasswordChanged(value);
    }
}

AppProxyType ProxyController::appProxyType(const QString &v)
{
    if (v == "http")
        return AppProxyType::Http;

    if (v == "socks4")
        return AppProxyType::Socks4;

    if (v == "socks5")
        return AppProxyType::Socks5;

    return AppProxyType::Http;
}

QString ProxyController::appProxyType(const AppProxyType &v)
{
    switch (v) {
    case AppProxyType::Http:     return "http";
    case AppProxyType::Socks4:   return "socks4";
    case AppProxyType::Socks5:   return "socks5";
    }

    return "http";
}

void ProxyController::onTypeChanged(const QString &value)
{
    // 应用代理类型发生变化
    AppProxyType t = appProxyType(value);
    if (t != m_appProxyConfig.type) {
        m_appProxyConfig.type = t;
        Q_EMIT appTypeChanged(t);
    }
}

void ProxyController::onUserChanged(const QString &value)
{
    // 应用代理用户名发生变化
    if (value != m_appProxyConfig.username) {
        m_appProxyConfig.username = value;
        Q_EMIT appUsernameChanged(value);
    }
}

void ProxyController::onPortChanged(uint value)
{
    // 应用代理端口发生变化
    if (value != m_appProxyConfig.port) {
        m_appProxyConfig.port = value;
        Q_EMIT appPortChanged(value);
    }
}
