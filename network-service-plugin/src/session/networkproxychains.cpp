// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkproxychains.h"

#include "constants.h"
#include "networkstatehandler.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDir>
#include <QGSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QVariant>

namespace network {
namespace sessionservice {
const QString notifyIconProxyEnabled = "notification-network-proxy-enabled";
const QString notifyIconProxyDisabled = "notification-network-proxy-disabled";

NetworkProxyChains::NetworkProxyChains(QDBusConnection &dbusConnection, NetworkStateHandler *networkStateHandler, QObject *parent)
    : QObject(parent)
    , m_dbusConnection(dbusConnection)
    , m_enable(false)
    , m_port(0)
    , m_networkStateHandler(networkStateHandler)
    , m_appProxy(new QDBusInterface("org.deepin.dde.NetworkProxy1", "/org/deepin/dde/NetworkProxy1/App", "org.deepin.dde.NetworkProxy1.App", QDBusConnection::systemBus(), this))
{
    init();
}

bool NetworkProxyChains::Enabled() const
{
    return m_enable;
}

QString NetworkProxyChains::IP() const
{
    return m_ip;
}

QString NetworkProxyChains::Password() const
{
    return m_password;
}

QString NetworkProxyChains::Type() const
{
    return m_type;
}

QString NetworkProxyChains::User() const
{
    return m_user;
}

uint NetworkProxyChains::Port() const
{
    return m_port;
}

void NetworkProxyChains::Set(const QString &type, const QString &ip, uint port, const QString &user, const QString &password)
{
    QString err = set(type, ip, port, user, password);
    if (!err.isEmpty()) {
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
    }
}

void NetworkProxyChains::SetEnable(bool enable)
{
    m_enable = enable;
    QString err = set(m_type, m_ip, m_port, m_user, m_password);
    emitPropertyChanged("Enable", m_enable);
    if (!err.isEmpty()) {
        setDelayedReply(true);
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
    }
}

void NetworkProxyChains::init()
{
    QString userConfigPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (userConfigPath.isEmpty()) {
        qCWarning(DSM()) << "";
        return;
    }
    QDir configDir(userConfigPath + "/deepin");
    m_jsonFile = configDir.filePath("proxychains.json");
    m_confFile = configDir.filePath("proxychains.conf");
    qCDebug(DSM()) << "load proxychains config file:" << m_jsonFile;

    QFile jsonFile(m_jsonFile);
    if (jsonFile.open(QFile::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonFile.readAll());
        if (doc.isObject()) {
            QJsonObject rootObj = doc.object();
            m_enable = rootObj.value("Enable").toBool();
            m_type = rootObj.value("Type").toString();
            m_ip = rootObj.value("IP").toString();
            m_port = rootObj.value("Port").toInt();
            m_user = rootObj.value("User").toString();
            m_password = rootObj.value("Password").toString();
        }
    }
    bool changed = fixConfig();
    qCDebug(DSM()) << "fixConfig changed:" << changed;
    if (changed) {
        QString err = saveConfig();
        if (!err.isEmpty()) {
            qCWarning(DSM()) << "save config failed:" << err;
        }
    }
    if (!checkConfig()) {
        // config is invalid
        removeConf();
    }
    if (m_enable && !m_ip.isEmpty() && m_port != 0) {
        startProxy();
    }
}

void NetworkProxyChains::emitPropertyChanged(const QString &prop, QVariant value)
{

    QDBusMessage msg = QDBusMessage::createSignal("/org/deepin/dde/Network1/ProxyChains", "org.freedesktop.DBus.Properties", "PropertiesChanged");
    msg << "org.deepin.dde.Network1.ProxyChains" << QVariantMap({ { prop, value } }) << QStringList();
    QDBusConnection::sessionBus().asyncCall(msg);
}

bool NetworkProxyChains::validType(const QString &type) const
{
    const QStringList choicesTypes({ "http", "socks4", "socks5" });
    return choicesTypes.contains(type);
}

bool NetworkProxyChains::validIPv4(const QString &ip) const
{
    QHostAddress address(ip);
    return !address.isNull() && (address.protocol() == QAbstractSocket::IPv4Protocol);
}

bool NetworkProxyChains::validUser(const QString &user) const
{

    return !user.contains(' ') && user.contains('\t');
}

bool NetworkProxyChains::validPassword(const QString &password) const
{
    return validUser(password);
}

bool NetworkProxyChains::fixConfig()
{
    bool changed = false;
    if (!validType(m_type)) {
        m_type = "http";
        changed = true;
    }

    if (m_ip != "" && !validIPv4(m_ip)) {
        m_ip = "";
        changed = true;
    }

    if (!validUser(m_user)) {
        m_user = "";
        changed = true;
    }

    if (!validPassword(m_password)) {
        m_password = "";
        changed = true;
    }
    return changed;
}

bool NetworkProxyChains::checkConfig() const
{
    return validType(m_type) && validIPv4(m_ip) && validUser(m_user) && validPassword(m_password);
}

QString NetworkProxyChains::saveConfig() const
{
    QJsonObject cfg;
    cfg.insert("Enable", m_enable);
    cfg.insert("Type", m_type);
    cfg.insert("IP", m_ip);
    cfg.insert("Port", (int)m_port);
    cfg.insert("User", m_user);
    cfg.insert("Password", m_password);
    QByteArray cfgJSON = QJsonDocument(cfg).toJson(QJsonDocument::Compact);
    QFile jsonFile(m_jsonFile);
    if (jsonFile.open(QFile::WriteOnly)) {
        jsonFile.write(cfgJSON);
    } else {
        return jsonFile.errorString();
    }
    return QString();
}

QString NetworkProxyChains::removeConf() const
{
    return QFile::remove(m_confFile) ? QString() : "remove config failed";
}

QString NetworkProxyChains::writeConf() const
{
    const QString head = R"delimiter(# Written by org.deepin.dde.Network1.ProxyChains
strict_chain
quiet_mode
proxy_dns
remote_dns_subnet 224
tcp_read_time_out 15000
tcp_connect_time_out 8000
localnet 127.0.0.0/255.0.0.0

[ProxyList]
)delimiter";

    QStringList proxy;
    proxy.append(m_type);
    proxy.append(m_ip);
    proxy.append(QString::number(m_port));
    if (!m_user.isEmpty() && !m_password.isEmpty()) {
        proxy.append(m_user);
        proxy.append(m_password);
    }
    QString data = head + proxy.join('\t') + '\n';
    QFile file(m_confFile);
    if (file.open(QFile::WriteOnly)) {
        file.write(data.toUtf8());
        file.flush();
        file.close();
    } else {
        return file.errorString();
    }
    return QString();
}

QString NetworkProxyChains::set(QString type, const QString &ip, uint port, const QString &user, const QString &password)
{
    // allow type is empty
    if (type.isEmpty()) {
        type = "http";
    }
    if (!validType(type)) {
        return QString("invalid param Type");
    }
    bool disable = false;
    if (ip.isEmpty() && port == 0) {
        disable = true;
    }
    if (!m_enable) {
        disable = true;
    }
    if (!disable && !validIPv4(ip)) {
        notifyAppProxyEnableFailed();
        return QString("invalid param IP");
    }
    if (m_enable && !validUser(user)) {
        notifyAppProxyEnableFailed();
        return QString("invalid param User");
    }
    if (m_enable && !validUser(password)) {
        notifyAppProxyEnableFailed();
        return QString("invalid param Password");
    }
    if (m_enable && ((user == "" && password != "") || (user != "" && password == ""))) {
        notifyAppProxyEnableFailed();
        return "user and password are not provided at the same time";
    }
    // all params are ok
    if (m_type != type) {
        m_type = type;
        emitPropertyChanged("Type", type);
    }
    if (m_ip != ip) {
        m_ip = ip;
        emitPropertyChanged("IP", ip);
    }
    if (m_password != password) {
        m_password = password;
        emitPropertyChanged("Password", password);
    }
    if (m_type != type) {
        m_type = type;
        emitPropertyChanged("Type", type);
    }
    if (m_user != user) {
        m_user = user;
        emitPropertyChanged("User", user);
    }
    QString err = saveConfig();
    if (!err.isEmpty()) {
        notifyAppProxyEnableFailed();
        return err;
    }
    if (disable) {
        m_appProxy->call("StopProxy");
        m_appProxy->call("ClearProxy");
        return removeConf();
    }
    // enable
    err = writeConf();
    if (!err.isEmpty()) {
        notifyAppProxyEnableFailed();
    } else {
        notifyAppProxyEnabled();
    }
    startProxy();
    return QString();
}

QString NetworkProxyChains::startProxy()
{
    QJsonObject settings;
    settings.insert("ProtoType", m_type);
    settings.insert("Name", "default");
    settings.insert("Server", m_ip);
    settings.insert("Port", (int)m_port);
    settings.insert("UserName", m_user);
    settings.insert("Password", m_password);
    QByteArray buf = QJsonDocument(settings).toJson(QJsonDocument::Compact);
    auto addReply = m_appProxy->asyncCall("AddProxy", m_type, "default", buf);
    addReply.waitForFinished();
    if (addReply.isError()) {
        qCWarning(DSM()) << "add proxy failed, err:" << addReply.error();
        return addReply.error().message();
    }
    auto startReply = m_appProxy->asyncCall("StartProxy", m_type, "default", false);
    if (startReply.isError()) {
        qCWarning(DSM()) << "start proxy failed, err:" << startReply.error();
        return startReply.error().message();
    }
    return QString();
}

void NetworkProxyChains::notifyAppProxyEnabled()
{
    m_networkStateHandler->notify(notifyIconProxyEnabled, tr("Network"), tr("Application proxy is set successfully"));
}

void NetworkProxyChains::notifyAppProxyEnableFailed()
{
    m_networkStateHandler->notify(notifyIconProxyDisabled, tr("Network"), tr("Failed to set the application proxy"));
}

} // namespace sessionservice
} // namespace network
