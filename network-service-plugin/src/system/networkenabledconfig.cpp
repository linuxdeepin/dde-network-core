// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networkenabledconfig.h"

#include "constants.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QString>
#include <QVariant>

namespace network {
namespace systemservice {
const QString configFile = "/var/lib/dde-daemon/network/config.json";

NetworkEnabledConfig::NetworkEnabledConfig()
{
    loadConfig();
}

NetworkEnabledConfig::~NetworkEnabledConfig() { }

bool NetworkEnabledConfig::vpnEnabled() const
{
    return m_map.value("VpnEnabled", QVariant(false)).toBool();
}

void NetworkEnabledConfig::setVpnEnabled(bool enabled)
{
    m_map.insert("VpnEnabled", enabled);
}

bool NetworkEnabledConfig::deviceEnabled(const QString &dev)
{
    qWarning() << __LINE__ << m_map;
    return m_map.value("Devices", QVariant::fromValue(QMap<QString, bool>())).value<QMap<QString, bool>>().value(dev, true);
}

void NetworkEnabledConfig::setDeviceEnabled(const QString &dev, bool enabled)
{
    qWarning() << __LINE__ << __FUNCTION__ << dev << enabled;
    QMap<QString, bool> map = m_map.value("Devices", QVariant::fromValue(QMap<QString, bool>())).value<QMap<QString, bool>>();
    qWarning() << __LINE__ << __FUNCTION__ << dev << enabled << map;
    qWarning() << __LINE__ << __FUNCTION__ << enabled << map.contains(dev);
    if (!enabled || map.contains(dev)) {
        map.insert(dev, enabled);
        m_map.insert("Devices", QVariant::fromValue(map));
    }
}

void NetworkEnabledConfig::removeDeviceEnabled(const QString &dev)
{
    QMap<QString, bool> map = m_map.value("Devices", QVariant::fromValue(QMap<QString, bool>())).value<QMap<QString, bool>>();
    map.remove(dev);
    m_map.insert("Devices", QVariant::fromValue(map));
}

void NetworkEnabledConfig::loadConfig()
{
    QFile file(configFile);
    if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            qCWarning(DSM()) << "failed to load config: Failed to parse JSON";
            return;
        }
        QJsonObject jsonObj = doc.object();
        bool vpnEnabled = jsonObj.value("VpnEnabled").toBool();
        setVpnEnabled(vpnEnabled);
        QJsonObject devsObj = jsonObj.value("Devices").toObject();
        QMap<QString, bool> map;
        for (auto &&key : devsObj.keys()) {
            QJsonObject valObj = devsObj.value(key).toObject();
            map.insert(key, valObj.value("Enabled").toBool());
        }
        m_map.insert("Devices", QVariant::fromValue(map));
    } else {
        qCWarning(DSM()) << "failed to load config:" << file.errorString();
    }
}

QString NetworkEnabledConfig::saveConfig()
{
    QJsonObject devsObj;
    QMap<QString, bool> map = m_map.value("Devices", QVariant::fromValue(QMap<QString, bool>())).value<QMap<QString, bool>>();
    for (auto it = map.constBegin(); it != map.constEnd(); it++) {
        if (!it.value()) {
            QJsonObject obj;
            obj.insert("Enabled", it.value());
            devsObj.insert(it.key(), obj);
        }
    }
    QJsonObject jsonObj;
    jsonObj.insert("VpnEnabled", vpnEnabled());
    jsonObj.insert("Devices", devsObj);
    QJsonDocument jsonDoc(jsonObj);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
    qWarning() << __LINE__ << m_map << jsonString;
    QFileInfo info(configFile);
    if (!info.dir().mkpath(info.dir().path())) {
        QString err = "failed to save config: failed to create the directory";
        qCWarning(DSM()) << err;
        return err;
    }
    QFile file(configFile);
    if (!file.open(QFile::WriteOnly) || file.write(jsonString.toUtf8()) < 0) {
        QString err = "failed to load config:" + file.errorString();
        qCWarning(DSM()) << err;
        return err;
    }
    return QString();
}
} // namespace systemservice
} // namespace network
