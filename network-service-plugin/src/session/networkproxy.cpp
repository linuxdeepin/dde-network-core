// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkproxy.h"

#include "constants.h"
#include "networkstatehandler.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QFile>
#include <QGSettings>
#include <QVariant>

namespace network {
namespace sessionservice {

const QString ProxyTypeHttp = "http";
const QString ProxyTypeHttps = "https";
const QString ProxyTypeFtp = "ftp";
const QString ProxyTypeSocks = "socks";
// The Deepin proxy gsettings schemas use the same path with
// org.gnome.system.proxy which is /system/proxy. So in fact they
// control the same values, and we don't need to synchronize them
// at all.
const QByteArray GSettingsIdProxy = "com.deepin.wrap.gnome.system.proxy";

const QString GkeyProxyMode = "mode";
const QString ProxyModeNone = "none";
const QString ProxyModeManual = "manual";
const QString ProxyModeAuto = "auto";

const QString GKeyProxyAuto = "autoconfig-url";
const QString GKeyProxyIgnoreHosts = "ignore-hosts";
const QString GKeyProxyHost = "host";
const QString GKeyProxyPort = "port";
const QString GKeyProxyUseAuthentication = "use-authentication";
const QString GKeyProxyAuthenticationUser = "authentication-user";
const QString GKeyProxyAuthenticationPassword = "authentication-password";

const QByteArray GChildProxyHttp = "com.deepin.wrap.gnome.system.proxy.http";
const QByteArray GChildProxyHttps = "com.deepin.wrap.gnome.system.proxy.https";
const QByteArray GChildProxyFtp = "com.deepin.wrap.gnome.system.proxy.ftp";
const QByteArray GChildProxySocks = "com.deepin.wrap.gnome.system.proxy.socks";

const QString notifyIconProxyEnabled = "notification-network-proxy-enabled";
const QString notifyIconProxyDisabled = "notification-network-proxy-disabled";

NetworkProxy::NetworkProxy(QDBusConnection &dbusConnection, NetworkStateHandler *networkStateHandler, QObject *parent)
    : QObject(parent)
    , m_dbusConnection(dbusConnection)
    , m_networkStateHandler(networkStateHandler)
    , m_proxySettings(nullptr)
    , m_proxyChildSettingsHttp(nullptr)
    , m_proxyChildSettingsHttps(nullptr)
    , m_proxyChildSettingsFtp(nullptr)
    , m_proxyChildSettingsSocks(nullptr)
{
    if (!QGSettings::isSchemaInstalled(GSettingsIdProxy)) {
        return;
    }
    m_proxySettings = new QGSettings(GSettingsIdProxy, QByteArray(), this);
    connect(m_proxySettings, &QGSettings::changed, this, &NetworkProxy::onConfigChanged);
    m_proxyChildSettingsHttp = new QGSettings(GChildProxyHttp, QByteArray(), this);
    connect(m_proxyChildSettingsHttp, &QGSettings::changed, this, &NetworkProxy::onConfigChanged);
    m_proxyChildSettingsHttps = new QGSettings(GChildProxyHttps, QByteArray(), this);
    connect(m_proxyChildSettingsHttps, &QGSettings::changed, this, &NetworkProxy::onConfigChanged);
    m_proxyChildSettingsFtp = new QGSettings(GChildProxyFtp, QByteArray(), this);
    connect(m_proxyChildSettingsFtp, &QGSettings::changed, this, &NetworkProxy::onConfigChanged);
    m_proxyChildSettingsSocks = new QGSettings(GChildProxySocks, QByteArray(), this);
    connect(m_proxyChildSettingsSocks, &QGSettings::changed, this, &NetworkProxy::onConfigChanged);
    // 如果ip全为空，则自动设置代理为None
    QString proxyAuto = m_proxySettings->get(GKeyProxyAuto).toString();
    QString http = m_proxyChildSettingsHttp->get(GKeyProxyHost).toString();
    QString https = m_proxyChildSettingsHttps->get(GKeyProxyHost).toString();
    QString ftp = m_proxyChildSettingsFtp->get(GKeyProxyHost).toString();
    QString socks = m_proxyChildSettingsSocks->get(GKeyProxyHost).toString();
    if (proxyAuto.isEmpty() && http.isEmpty() && https.isEmpty() && ftp.isEmpty() && socks.isEmpty()) {
        m_proxySettings->set(GkeyProxyMode, ProxyModeNone);
    }
}

QString NetworkProxy::GetProxyMethod()
{
    QString proxyMode = m_proxySettings->get(GkeyProxyMode).toString();
    qCInfo(DSM()) << "GetProxyMethod" << proxyMode;
    return proxyMode;
}

void NetworkProxy::SetProxyMethod(const QString &proxyMode)
{
    qCInfo(DSM()) << "SetProxyMethod" << proxyMode;
    const QStringList choicesModes({ ProxyModeNone, ProxyModeManual, ProxyModeAuto });
    if (!choicesModes.contains(proxyMode)) {
        QString err = "invalid proxy method" + proxyMode;
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    // ignore if proxyModeNone already set
    QString currentMethod = GetProxyMethod();
    if (proxyMode == ProxyModeNone && currentMethod == ProxyModeNone) {
        return;
    }
    m_proxySettings->set(GkeyProxyMode, proxyMode);
    Q_EMIT ProxyMethodChanged(proxyMode);
    if (proxyMode == ProxyModeNone) {
        m_networkStateHandler->notify(notifyIconProxyDisabled, tr("Network"), tr("System proxy has been cancelled."));
    } else {
        m_networkStateHandler->notify(notifyIconProxyEnabled, tr("Network"), tr("System proxy is set successfully."));
    }
}

QString NetworkProxy::GetAutoProxy()
{
    return m_proxySettings->get(GKeyProxyAuto).toString();
}

void NetworkProxy::SetAutoProxy(const QString &proxyAuto)
{
    qCDebug(DSM()) << "set autoconfig-url for proxy" << proxyAuto;
    m_proxySettings->set(GKeyProxyAuto, proxyAuto);
}

QString NetworkProxy::GetProxyIgnoreHosts()
{
    QStringList array = m_proxySettings->get(GKeyProxyIgnoreHosts).toStringList();
    return array.join(", ");
}

void NetworkProxy::SetProxyIgnoreHosts(const QString &ignoreHosts)
{
    qCDebug(DSM()) << "set ignore-hosts for proxy" << ignoreHosts;
    QString ignoreHostsFixed = ignoreHosts;
    ignoreHostsFixed.remove(" ");
    QStringList array = ignoreHostsFixed.split(",", Qt::SkipEmptyParts);
    m_proxySettings->set(GKeyProxyIgnoreHosts, array);
}

void NetworkProxy::GetProxy(const QString &proxyType)
{
    setDelayedReply(true);
    QGSettings *childSettings = getProxyChildSettings(proxyType);
    if (!childSettings) {
        QString err = "not a valid proxy type:" + proxyType;
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    QString host = childSettings->get(GKeyProxyHost).toString();
    int port = childSettings->get(GKeyProxyPort).toInt();
    dbusConnection().send(message().createReply({ host, QString::number(port) }));
}

void NetworkProxy::SetProxy(const QString &proxyType, const QString &host, const QString &port)
{
    qCDebug(DSM()) << QString("Manager.SetProxy proxyType: %1, host: %2, port: %3").arg(proxyType).arg(host).arg(port);
    int portInt = port.toInt();
    if (portInt < 0 || portInt > 65535) {
        setDelayedReply(true);
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, "port number must be an integer between 0 and 65535"));
        return;
    }
    QGSettings *childSettings = getProxyChildSettings(proxyType);
    if (!childSettings) {
        setDelayedReply(true);
        QString err = "not a valid proxy type:" + proxyType;
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    childSettings->set(GKeyProxyHost, host);
    childSettings->set(GKeyProxyPort, portInt);
}

void NetworkProxy::GetProxyAuthentication(const QString &proxyType)
{
    setDelayedReply(true);
    QGSettings *childSettings = getProxyChildSettings(proxyType);
    if (!childSettings) {
        QString err = "not a valid proxy type:" + proxyType;
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    bool enable = childSettings->get(GKeyProxyUseAuthentication).toBool();
    QString user = childSettings->get(GKeyProxyAuthenticationUser).toString();
    QString password = childSettings->get(GKeyProxyAuthenticationPassword).toString();
    dbusConnection().send(message().createReply({ user, password, enable }));
}

void NetworkProxy::SetProxyAuthentication(const QString &proxyType, const QString &user, const QString &password, bool enable)
{
    qCDebug(DSM()) << QString("Manager.SetProxyAuthentication proxyType: %1, host: %2, port: %3, use: %4").arg(proxyType).arg(user).arg(password).arg(enable);
    QGSettings *childSettings = getProxyChildSettings(proxyType);
    if (!childSettings) {
        QString err = "not a valid proxy type:" + proxyType;
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    if (!childSettings->keys().contains(GKeyProxyUseAuthentication)) {
        QString err = QString("%s is not support authentication").arg(proxyType);
        qCWarning(DSM()) << err;
        dbusConnection().send(message().createErrorReply(QDBusError::InvalidArgs, err));
        return;
    }
    childSettings->set(GKeyProxyUseAuthentication, enable);
    childSettings->set(GKeyProxyAuthenticationUser, user);
    childSettings->set(GKeyProxyAuthenticationPassword, password);
}

void NetworkProxy::onConfigChanged(const QString &key) { }

QGSettings *NetworkProxy::getProxyChildSettings(const QString &proxyType)
{
    if (proxyType == ProxyTypeHttp) {
        return m_proxyChildSettingsHttp;
    } else if (proxyType == ProxyTypeHttps) {
        return m_proxyChildSettingsHttps;
    } else if (proxyType == ProxyTypeFtp) {
        return m_proxyChildSettingsFtp;
    } else if (proxyType == ProxyTypeSocks) {
        return m_proxyChildSettingsSocks;
    }
    qCWarning(DSM()) << "not a valid proxy type:" << proxyType;
    return nullptr;
}

QDBusObjectPath NetworkProxy::ActivateAccessPoint(const QString &uuid, const QDBusObjectPath &apPath, const QDBusObjectPath &devPath)
{

    network::service::dbusDebug(message().service(), __FUNCTION__);
    return QDBusObjectPath(QString("/"));
}

QDBusObjectPath NetworkProxy::ActivateConnection(const QString &uuid, const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
    return QDBusObjectPath(QString("/"));
}

void NetworkProxy::DeactivateConnection(const QString &uuid)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::DebugChangeAPChannel(const QString &band)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::DeleteConnection(const QString &uuid)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::DisableWirelessHotspotMode(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::DisconnectDevice(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::EnableDevice(const QDBusObjectPath &devPath, bool enabled)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::EnableWirelessHotspotMode(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

QString NetworkProxy::GetAccessPoints(const QDBusObjectPath &path)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return "[]";
}

QString NetworkProxy::GetActiveConnectionInfo()
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return "[]";
}

QStringList NetworkProxy::GetSupportedConnectionTypes()
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return QStringList() << "802-3-ethernet" << "802-11-wireless";
}

bool NetworkProxy::IsDeviceEnabled(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return true;
}

bool NetworkProxy::IsWirelessHotspotModeEnabled(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return false;
}

QList<QDBusObjectPath> NetworkProxy::ListDeviceConnections(const QDBusObjectPath &devPath)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);

    return QList<QDBusObjectPath>();
}

void NetworkProxy::RequestIPConflictCheck(const QString &ip, const QString &ifc)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::RequestWirelessScan()
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

void NetworkProxy::SetDeviceManaged(const QString &devPathOrIfc, bool managed)
{
    network::service::dbusDebug(message().service(), __FUNCTION__);
}

// 属性getter和setter方法
QString NetworkProxy::wirelessAccessPoints() const
{
    return "[]";
}

uint NetworkProxy::state() const
{
    return 0;
}

uint NetworkProxy::connectivity() const
{
    return 0;
}

bool NetworkProxy::networkingEnabled() const
{
    return true;
}

void NetworkProxy::setNetworkingEnabled(bool enabled) { }

bool NetworkProxy::vpnEnabled() const
{
    return true;
}

void NetworkProxy::setVpnEnabled(bool enabled) { }

QString NetworkProxy::devices() const
{
    return "[]";
}

QString NetworkProxy::connections() const
{
    return "[]";
}

QString NetworkProxy::activeConnections() const
{
    return "[]";
}

} // namespace sessionservice
} // namespace network
