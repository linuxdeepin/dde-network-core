/*
* Copyright (C) 2021 ~ 2023 Deepin Technology Co., Ltd.
*
* Author:     caixiangrong <caixiangrong@uniontech.com>
*
* Maintainer: caixiangrong <caixiangrong@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "networkdbusproxy.h"

#include <QMetaObject>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingReply>

const QString NetworkService = QStringLiteral("com.deepin.daemon.Network");
const QString NetworkPath = QStringLiteral("/com/deepin/daemon/Network");
const QString NetworkInterface = QStringLiteral("com.deepin.daemon.Network");

const QString ProxyChainsService = QStringLiteral("com.deepin.daemon.Network");
const QString ProxyChainsPath = QStringLiteral("/com/deepin/daemon/Network/ProxyChains");
const QString ProxyChainsInterface = QStringLiteral("com.deepin.daemon.Network.ProxyChains");

const QString AirplaneModeService = QStringLiteral("com.deepin.daemon.AirplaneMode");
const QString AirplaneModePath = QStringLiteral("/com/deepin/daemon/AirplaneMode");
const QString AirplaneModeInterface = QStringLiteral("com.deepin.daemon.AirplaneMode");

const QString PropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const QString PropertiesChanged = QStringLiteral("PropertiesChanged");
using namespace dde::network;

//"com.deepin.daemon.AirplaneMode", "/com/deepin/daemon/AirplaneMode", "com.deepin.daemon.AirplaneMode", QDBusConnection::systemBus());

NetworkDBusProxy::NetworkDBusProxy(QObject *parent)
    : QObject(parent)
    , m_networkInter(new QDBusInterface(NetworkService, NetworkPath, NetworkInterface, QDBusConnection::sessionBus(), this))
    , m_proxyChainsInter(new QDBusInterface(ProxyChainsService, ProxyChainsPath, ProxyChainsInterface, QDBusConnection::sessionBus(), this))
    , m_airplaneModeInter(new QDBusInterface(AirplaneModeService, AirplaneModePath, AirplaneModeInterface, QDBusConnection::systemBus(), this))
{
    QDBusConnection::sessionBus().connect(NetworkService, NetworkPath, PropertiesInterface, PropertiesChanged, this, SLOT(onPropertiesChanged(QDBusMessage)));
    QDBusConnection::sessionBus().connect(ProxyChainsService, ProxyChainsPath, PropertiesInterface, PropertiesChanged, this, SLOT(onPropertiesChanged(QDBusMessage)));
    QDBusConnection::systemBus().connect(AirplaneModeService, AirplaneModePath, PropertiesInterface, PropertiesChanged, this, SLOT(onPropertiesChanged(QDBusMessage)));

    connect(m_networkInter, SIGNAL(AccessPointAdded(const QString &, const QString &)), this, SIGNAL(AccessPointAdded(const QString &, const QString &)));
    connect(m_networkInter, SIGNAL(AccessPointPropertiesChanged(const QString &, const QString &)), this, SIGNAL(AccessPointPropertiesChanged(const QString &, const QString &)));
    connect(m_networkInter, SIGNAL(AccessPointRemoved(const QString &, const QString &)), this, SIGNAL(AccessPointRemoved(const QString &, const QString &)));
    connect(m_networkInter, SIGNAL(ActiveConnectionInfoChanged()), this, SIGNAL(ActiveConnectionInfoChanged()));
    connect(m_networkInter, SIGNAL(DeviceEnabled(const QString &, bool)), this, SIGNAL(DeviceEnabled(const QString &, bool)));
    connect(m_networkInter, SIGNAL(IPConflict(const QString &, const QString &)), this, SIGNAL(IPConflict(const QString &, const QString &)));
}

void NetworkDBusProxy::onPropertiesChanged(const QDBusMessage &message)
{
    QVariantMap changedProps = qdbus_cast<QVariantMap>(message.arguments().at(1).value<QDBusArgument>());
    for (QVariantMap::const_iterator it = changedProps.cbegin(); it != changedProps.cend(); ++it) {
        QMetaObject::invokeMethod(this, it.key().toLatin1() + "Changed", Qt::DirectConnection, QGenericArgument(it.value().typeName(), it.value().data()));
    }
}
// networkInter property
QString NetworkDBusProxy::activeConnections()
{
    return qvariant_cast<QString>(m_networkInter->property("ActiveConnections"));
}

QString NetworkDBusProxy::connections()
{
    return qvariant_cast<QString>(m_networkInter->property("Connections"));
}

uint NetworkDBusProxy::connectivity()
{
    return qvariant_cast<uint>(m_networkInter->property("Connectivity"));
}

QString NetworkDBusProxy::devices()
{
    return qvariant_cast<QString>(m_networkInter->property("Devices"));
}

bool NetworkDBusProxy::networkingEnabled()
{
    return qvariant_cast<bool>(m_networkInter->property("NetworkingEnabled"));
}
void NetworkDBusProxy::setNetworkingEnabled(bool value)
{
    m_networkInter->setProperty("NetworkingEnabled", QVariant::fromValue(value));
}

uint NetworkDBusProxy::state()
{
    return qvariant_cast<uint>(m_networkInter->property("State"));
}

bool NetworkDBusProxy::vpnEnabled()
{
    return qvariant_cast<bool>(m_networkInter->property("VpnEnabled"));
}
void NetworkDBusProxy::setVpnEnabled(bool value)
{
    m_networkInter->setProperty("VpnEnabled", QVariant::fromValue(value));
}

QString NetworkDBusProxy::wirelessAccessPoints()
{
    return qvariant_cast<QString>(m_networkInter->property("WirelessAccessPoints"));
}
// proxyChains property
QString NetworkDBusProxy::iP()
{
    return qvariant_cast<QString>(m_proxyChainsInter->property("IP"));
}

QString NetworkDBusProxy::password()
{
    return qvariant_cast<QString>(m_proxyChainsInter->property("Password"));
}

uint NetworkDBusProxy::port()
{
    return qvariant_cast<uint>(m_proxyChainsInter->property("Port"));
}

QString NetworkDBusProxy::type()
{
    return qvariant_cast<QString>(m_proxyChainsInter->property("Type"));
}

QString NetworkDBusProxy::user()
{
    return qvariant_cast<QString>(m_proxyChainsInter->property("User"));
}

bool NetworkDBusProxy::enabled()
{
    return qvariant_cast<bool>(m_airplaneModeInter->property("Enabled"));
}

void NetworkDBusProxy::showModule(const QString &module)
{
    QDBusInterface ControlCenter("com.deepin.dde.ControlCenter", "/com/deepin/dde/ControlCenter", "com.deepin.dde.ControlCenter", QDBusConnection::sessionBus());
    ControlCenter.call("ShowModule", QVariant::fromValue(module));
}
// networkInter property
void NetworkDBusProxy::EnableDevice(const QDBusObjectPath &devPath, bool enabled)
{
    m_networkInter->asyncCall(QStringLiteral("EnableDevice"), QVariant::fromValue(devPath), QVariant::fromValue(enabled));
}

QString NetworkDBusProxy::GetProxyMethod()
{
    return QDBusPendingReply<QString>(m_networkInter->asyncCall(QStringLiteral("GetProxyMethod")));
}

void NetworkDBusProxy::SetProxyMethod(const QString &proxyMode)
{
    m_networkInter->asyncCall(QStringLiteral("SetProxyMethod"), QVariant::fromValue(proxyMode));
}

void NetworkDBusProxy::SetProxyMethod(const QString &proxyMode, QObject *receiver, const char *member)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(proxyMode);
    m_networkInter->callWithCallback(QStringLiteral("SetProxyMethod"), argumentList, receiver, member);
}

QString NetworkDBusProxy::GetProxyIgnoreHosts()
{
    return QDBusPendingReply<QString>(m_networkInter->asyncCall(QStringLiteral("GetProxyIgnoreHosts")));
}

void NetworkDBusProxy::SetProxyIgnoreHosts(const QString &ignoreHosts)
{
    m_networkInter->asyncCall(QStringLiteral("SetProxyIgnoreHosts"), QVariant::fromValue(ignoreHosts));
}

void NetworkDBusProxy::SetProxyIgnoreHosts(const QString &ignoreHosts, QObject *receiver, const char *member)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(ignoreHosts);
    m_networkInter->callWithCallback(QStringLiteral("SetProxyIgnoreHosts"), argumentList, receiver, member);
}

QString NetworkDBusProxy::GetAutoProxy()
{
    return QDBusPendingReply<QString>(m_networkInter->asyncCall(QStringLiteral("GetAutoProxy")));
}

void NetworkDBusProxy::SetAutoProxy(const QString &proxyAuto)
{
    m_networkInter->asyncCall(QStringLiteral("SetAutoProxy"), QVariant::fromValue(proxyAuto));
}

void NetworkDBusProxy::SetAutoProxy(const QString &proxyAuto, QObject *receiver, const char *member)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(proxyAuto);
    m_networkInter->callWithCallback(QStringLiteral("SetAutoProxy"), argumentList, receiver, member);
}

QStringList NetworkDBusProxy::GetProxy(const QString &proxyType)
{
    QStringList out;
    QDBusMessage reply = m_networkInter->call(QDBus::Block, QStringLiteral("GetProxy"), QVariant::fromValue(proxyType));
    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == 2) {
        out << qdbus_cast<QString>(reply.arguments().at(0));
        out << qdbus_cast<QString>(reply.arguments().at(1));
    }
    return out;
}

void NetworkDBusProxy::SetProxy(const QString &proxyType, const QString &host, const QString &port)
{
    m_networkInter->asyncCall(QStringLiteral("SetProxy"), QVariant::fromValue(proxyType), QVariant::fromValue(host), QVariant::fromValue(port));
}

void NetworkDBusProxy::SetProxy(const QString &proxyType, const QString &host, const QString &port, QObject *receiver, const char *member)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(proxyType) << QVariant::fromValue(host) << QVariant::fromValue(port);
    m_networkInter->callWithCallback(QStringLiteral("SetProxy"), argumentList, receiver, member);
}

QString NetworkDBusProxy::GetActiveConnectionInfo()
{
    return QDBusPendingReply<QString>(m_networkInter->asyncCall(QStringLiteral("GetActiveConnectionInfo")));
}

QDBusObjectPath NetworkDBusProxy::ActivateConnection(const QString &uuid, const QDBusObjectPath &devicePath)
{
    return QDBusPendingReply<QDBusObjectPath>(m_networkInter->asyncCall(QStringLiteral("ActivateConnection"), QVariant::fromValue(uuid), QVariant::fromValue(devicePath)));
}

QDBusObjectPath NetworkDBusProxy::ActivateAccessPoint(const QString &uuid, const QDBusObjectPath &apPath, const QDBusObjectPath &devPath)
{
    return QDBusPendingReply<QDBusObjectPath>(m_networkInter->asyncCall(QStringLiteral("ActivateAccessPoint"), QVariant::fromValue(uuid), QVariant::fromValue(apPath), QVariant::fromValue(devPath)));
}

bool NetworkDBusProxy::ActivateAccessPoint(const QString &uuid, const QDBusObjectPath &apPath, const QDBusObjectPath &devPath, QObject *receiver, const char *member, const char *errorSlot)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(uuid) << QVariant::fromValue(apPath) << QVariant::fromValue(devPath);
    return m_networkInter->callWithCallback(QStringLiteral("ActivateAccessPoint"), argumentList, receiver, member, errorSlot);
}

void NetworkDBusProxy::DisconnectDevice(const QDBusObjectPath &devPath)
{
    m_networkInter->asyncCall(QStringLiteral("DisconnectDevice"), QVariant::fromValue(devPath));
}

void NetworkDBusProxy::RequestIPConflictCheck(const QString &ip, const QString &ifc)
{
    m_networkInter->asyncCall(QStringLiteral("RequestIPConflictCheck"), QVariant::fromValue(ip), QVariant::fromValue(ifc));
}

bool NetworkDBusProxy::IsDeviceEnabled(const QDBusObjectPath &devPath)
{
    return QDBusPendingReply<bool>(m_networkInter->asyncCall(QStringLiteral("IsDeviceEnabled"), QVariant::fromValue(devPath)));
}

void NetworkDBusProxy::RequestWirelessScan()
{
    m_networkInter->asyncCall(QStringLiteral("RequestWirelessScan"));
}

void NetworkDBusProxy::Set(const QString &type0, const QString &ip, uint port, const QString &user, const QString &password)
{
    m_proxyChainsInter->asyncCall(QStringLiteral("Set"), QVariant::fromValue(type0), QVariant::fromValue(ip), QVariant::fromValue(port), QVariant::fromValue(user), QVariant::fromValue(password));
}

uint NetworkDBusProxy::Notify(const QString &in0, uint in1, const QString &in2, const QString &in3, const QString &in4, const QStringList &in5, const QVariantMap &in6, int in7)
{
    return QDBusPendingReply<uint>(m_notificationsInter->asyncCall(QStringLiteral("Notify"), QVariant::fromValue(in0), QVariant::fromValue(in1), QVariant::fromValue(in2), QVariant::fromValue(in3), QVariant::fromValue(in4), QVariant::fromValue(in5), QVariant::fromValue(in6), QVariant::fromValue(in7)));
}
