// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wirelessconnect.h"
#include <pwd.h>

#include <wirelessdevice.h>

#include <NetworkManagerQt/AccessPoint>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/WirelessSecuritySetting>
#include <NetworkManagerQt/Security8021xSetting>
#include <NetworkManagerQt/WirelessSetting>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Ipv6Setting>
#include <NetworkManagerQt/Utils>
#include <QtDBus/qdbusmetatype.h>

#define LIGHTDM_USER "lightdm"

using namespace dde::network;
using namespace NetworkManager;

WirelessConnect::WirelessConnect(QObject *parent, dde::network::WirelessDevice *device, dde::network::AccessPoints *ap)
    : QObject(parent)
    , m_device(device)
    , m_accessPoint(ap)
    , m_needUpdate(false)
{
    qDBusRegisterMetaType<IpV6DBusAddress>();
    qDBusRegisterMetaType<IpV6DBusAddressList>();
}

WirelessConnect::~WirelessConnect()
{
}

void WirelessConnect::setSsid(const QString &ssid)
{
    m_ssid = ssid;
    m_connectionSettings.clear();
}

bool WirelessConnect::passwordIsValid(const QString &password)
{
    WirelessSecuritySetting::KeyMgmt keyMgmt = WirelessSecuritySetting::WpaPsk;
    if (m_connectionSettings) {
        WirelessSecuritySetting::Ptr wsSetting = m_connectionSettings->setting(Setting::SettingType::WirelessSecurity).staticCast<WirelessSecuritySetting>();
        keyMgmt = wsSetting->keyMgmt();
    } else {
        keyMgmt = getKeyMgmtByAp(m_accessPoint);
    }
    if (keyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
        return wepKeyIsValid(password, WirelessSecuritySetting::WepKeyType::Passphrase);
    }
    return wpaPskIsValid(password);
}

WirelessSecuritySetting::KeyMgmt WirelessConnect::getKeyMgmtByAp(dde::network::AccessPoints *ap)
{
    if (nullptr == ap) {
        return WirelessSecuritySetting::WpaPsk;
    }
    AccessPoint::Ptr nmAp(new AccessPoint(ap->path()));
    AccessPoint::Capabilities capabilities = nmAp->capabilities();
    AccessPoint::WpaFlags wpaFlags = nmAp->wpaFlags();
    AccessPoint::WpaFlags rsnFlags = nmAp->rsnFlags();

    WirelessSecuritySetting::KeyMgmt keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaNone;

    if (capabilities.testFlag(AccessPoint::Capability::Privacy) && !wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk) && !wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x)) {
        keyMgmt = WirelessSecuritySetting::KeyMgmt::Wep;
    }

    if (wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk) || rsnFlags.testFlag(AccessPoint::WpaFlag::KeyMgmtPsk)) {
        keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaPsk;
    }

#ifdef USE_DEEPIN_NMQT
    // 判断是否是wpa3加密的，因为wpa3加密方式，实际上是wpa2的扩展，所以其中会包含KeyMgmtPsk枚举值
    if (wpaFlags.testFlag(NetworkManager::AccessPoint::WpaFlag::keyMgmtSae) || rsnFlags.testFlag(NetworkManager::AccessPoint::WpaFlag::keyMgmtSae)) {
        keyMgmt = NetworkManager::WirelessSecuritySetting::KeyMgmt::WpaSae;
    }
#else
    if (wpaFlags.testFlag(NetworkManager::AccessPoint::WpaFlag::KeyMgmtSAE) || rsnFlags.testFlag(NetworkManager::AccessPoint::WpaFlag::KeyMgmtSAE)) {
        keyMgmt = NetworkManager::WirelessSecuritySetting::KeyMgmt::SAE;
    }
#endif

    if (wpaFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x) || rsnFlags.testFlag(AccessPoint::WpaFlag::KeyMgmt8021x)) {
        keyMgmt = WirelessSecuritySetting::KeyMgmt::WpaEap;
    }
    return keyMgmt;
}

void WirelessConnect::initConnection()
{
    // 隐藏网络直接创建settings
    if (m_accessPoint) {
        NetworkManager::Connection::Ptr conn;
        for (ActiveConnection::Ptr con : NetworkManager::activeConnections()) {
            if (con->type() != ConnectionSettings::ConnectionType::Wireless || con->id() != m_ssid)
                continue;

            NetworkManager::ConnectionSettings::Ptr connSettings = con->connection()->settings();
            NetworkManager::WirelessSetting::Ptr wSetting = connSettings->setting(NetworkManager::Setting::SettingType::Wireless).staticCast<NetworkManager::WirelessSetting>();
            if (wSetting.isNull())
                continue;

            QString settingMacAddress = wSetting->macAddress().toHex().toUpper();
            QString deviceMacAddress = m_device->realHwAdr().remove(":");
            if (!settingMacAddress.isEmpty() && settingMacAddress != deviceMacAddress)
                continue;

            conn = findConnectionByUuid(con->uuid());
            break;
        }

        if (conn.isNull()) {
            for (auto item : m_device->items()) {
                if (item->connection()->ssid() != m_ssid)
                    continue;

                QString uuid = item->connection()->uuid();
                if (!uuid.isEmpty()) {
                    conn = findConnectionByUuid(uuid);
                    if (!conn.isNull() && conn->isValid()) {
                        break;
                    }
                }
            }
        }
        if (!conn.isNull() && conn->isValid()) {
            m_connectionSettings = conn->settings();
        }
    }
    //　没连接过的需要新建连接
    if (m_connectionSettings.isNull()) {
        m_connectionSettings = QSharedPointer<ConnectionSettings>(new ConnectionSettings(ConnectionSettings::ConnectionType::Wireless));
        // 创建uuid
        QString uuid = m_connectionSettings->createNewUuid();
        while (findConnectionByUuid(uuid)) {
            qint64 second = QDateTime::currentDateTime().toSecsSinceEpoch();
            uuid.replace(24, QString::number(second).length(), QString::number(second));
        }
        m_connectionSettings->setUuid(uuid);
        m_connectionSettings->setId(m_ssid);
        if (!m_accessPoint) {
            m_connectionSettings->setting(Setting::SettingType::Wireless).staticCast<WirelessSetting>()->setHidden(true);
        }
        qInfo() << "create connect:" << m_ssid << uuid << m_accessPoint;
        if (m_accessPoint) {
            m_connectionSettings->setting(Setting::Security8021x).staticCast<Security8021xSetting>()->setPasswordFlags(Setting::AgentOwned);
            WirelessSecuritySetting::Ptr wsSetting = m_connectionSettings->setting(Setting::WirelessSecurity).dynamicCast<WirelessSecuritySetting>();
            WirelessSecuritySetting::KeyMgmt keyMgmt = getKeyMgmtByAp(m_accessPoint);
            if (keyMgmt != WirelessSecuritySetting::KeyMgmt::WpaNone) {
                wsSetting->setKeyMgmt(keyMgmt);
                if (keyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
                    wsSetting->setWepKeyFlags(Setting::None);
                } else if (keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk
#ifdef USE_DEEPIN_NMQT
                           || keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaSae) {
#else
                           || keyMgmt == WirelessSecuritySetting::KeyMgmt::SAE) {
#endif
                    wsSetting->setPskFlags(Setting::None);
                }
                wsSetting->setInitialized(true);
            }
        }
        WirelessSetting::Ptr wirelessSetting = m_connectionSettings->setting(Setting::Wireless).dynamicCast<WirelessSetting>();
        wirelessSetting->setSsid(m_ssid.toUtf8());
        wirelessSetting->setInitialized(true);
        m_needUpdate = true;
    }
}

void WirelessConnect::setPassword(const QString &password)
{
    WirelessSecuritySetting::Ptr wsSetting = m_connectionSettings->setting(Setting::SettingType::WirelessSecurity).staticCast<WirelessSecuritySetting>();
    bool isHidden = m_connectionSettings->setting(Setting::SettingType::Wireless).staticCast<WirelessSetting>()->hidden();
    WirelessSecuritySetting::KeyMgmt keyMgmt = isHidden ? getKeyMgmtByAp(m_accessPoint) : wsSetting->keyMgmt();
    wsSetting->setKeyMgmt(keyMgmt);
    if (keyMgmt == WirelessSecuritySetting::KeyMgmt::Wep) {
        wsSetting->setWepKey0(password);
    } else if (keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk
#ifdef USE_DEEPIN_NMQT
               || keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaSae) {
#else
               || keyMgmt == WirelessSecuritySetting::KeyMgmt::SAE) {
#endif
        wsSetting->setPsk(password);

        if (isHidden && keyMgmt == WirelessSecuritySetting::KeyMgmt::WpaPsk) {
            wsSetting->setAuthAlg(WirelessSecuritySetting::AuthAlg::Open);
        }
    }
    wsSetting->setInitialized(true);
    m_needUpdate = true;
}

/**
 * @brief WirelessConnect::hasPassword
 * @return 是否需要密码
 */
bool WirelessConnect::hasPassword()
{
    if (m_accessPoint && m_accessPoint->secured()) {
        // 已有Connection则尝试直接连接
        NetworkManager::Connection::Ptr conn = findConnectionByUuid(m_connectionSettings->uuid());
        return conn.isNull();
    }

    return (!m_accessPoint || m_accessPoint->secured());
}

void WirelessConnect::connectNetwork()
{
    initConnection();
    // 隐藏网络先尝试无密码连接
    if (m_accessPoint) {
        if ((hasPassword())) {
            emit passwordError(QString());
            return;
        }
    }
    activateConnection();
}

void WirelessConnect::connectNetworkPassword(const QString password)
{
    initConnection();
    setPassword(password);
    activateConnection();
}

void WirelessConnect::activateConnection()
{
    m_device->disconnectNetwork();
    NetworkManager::Connection::Ptr conn;
    QString id = m_connectionSettings->id();
    ConnectionSettings::ConnectionType type = m_connectionSettings->connectionType();
    for (auto it : NetworkManager::listConnections()) {
        if (type != it->settings()->connectionType() || id != it->name())
            continue;

        NetworkManager::WirelessSetting::Ptr wSetting = it->settings()->setting(NetworkManager::Setting::SettingType::Wireless).staticCast<NetworkManager::WirelessSetting>();
        if (wSetting.isNull())
            continue;

        const QString setingMacAddress = wSetting->macAddress().toHex().toUpper();
        const QString deviceMacAddress = m_device->realHwAdr().remove(":").toUpper();
        if (!setingMacAddress.isEmpty() && setingMacAddress != deviceMacAddress)
            continue;

        m_connectionSettings->setUuid(it->uuid());
        conn = it;
        break;
    }

    QString accessPointPath;
    if (m_accessPoint) {
        accessPointPath = m_accessPoint->path();
    }
    if (conn.isNull()) {
        conn = findConnectionByUuid(m_connectionSettings->uuid());
    }
    if (conn.isNull()) {
        qInfo() << "addAndActivateConnection" << m_device->path() << accessPointPath;
        addAndActivateConnection(m_connectionSettings->toMap(), m_device->path(), accessPointPath);
        return;
    }
    // 隐藏网络或设置过密码的需要update
    if (m_needUpdate) {
        m_needUpdate = false;
        QDBusPendingReply<> reply;
        reply = conn->update(m_connectionSettings->toMap());
        reply.waitForFinished();
        if (reply.isError()) {
            qInfo() << "error occurred while updating the connection" << reply.error();
            return;
        }
    }
    qInfo() << "activateConnection" <<conn->path()<< m_device->path() << accessPointPath;
    NetworkManager::activateConnection(conn->path(), m_device->path(), accessPointPath);
}

void WirelessConnect::getoldPassword()
{
    // 旧密码不展示
    emit passwordError(QString());
}
