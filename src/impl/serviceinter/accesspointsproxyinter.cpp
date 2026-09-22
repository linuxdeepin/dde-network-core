// SPDX-FileCopyrightText: 2018 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "accesspointsproxyinter.h"
#include "netutils.h"

using namespace dde::network;

AccessPointsProxyInter::AccessPointsProxyInter(const QJsonObject &json, const QString &devicePath, QObject *parent)
    : AccessPointProxy(parent)
    , m_devicePath(devicePath)
    , m_json(json)
{
}

AccessPointsProxyInter::~AccessPointsProxyInter()
{
}

QString AccessPointsProxyInter::ssid() const
{
    return m_json.value("Ssid").toString();
}

QByteArray AccessPointsProxyInter::rawSsid() const
{
    // DSS 服务目前仅下发显示用的 Ssid 字符串，未提供原始字节；
    // 若服务端未来提供 RawSsid（base64）字段，在此解析返回。
    if (m_json.contains("RawSsid")) {
        return QByteArray::fromBase64(m_json.value("RawSsid").toString().toLatin1());
    }
    if (m_json.contains("Ssid")) {
        return m_json.value("Ssid").toString().toUtf8();
    }

    return QByteArray();
}

int AccessPointsProxyInter::strength() const
{
    if (m_json.isEmpty())
        return -1;

    return m_json.value("Strength").toInt();
}

bool AccessPointsProxyInter::secured() const
{
    return m_json.value("Secured").toBool();
}

bool AccessPointsProxyInter::securedInEap() const
{
    return m_json.value("SecuredInEap").toBool();
}

int AccessPointsProxyInter::frequency() const
{
    return m_json.value("Frequency").toInt();
}

QString AccessPointsProxyInter::path() const
{
    return m_json.value("Path").toString();
}

QString AccessPointsProxyInter::devicePath() const
{
    return m_devicePath;
}

bool AccessPointsProxyInter::connected() const
{
    return (m_status == ConnectionStatus::Activated);
}

ConnectionStatus AccessPointsProxyInter::status() const
{
    return m_status;
}

bool AccessPointsProxyInter::hidden() const
{
    if (m_json.contains("Hidden"))
        return m_json.value("Hidden").toBool();

    return false;
}

bool AccessPointsProxyInter::isWlan6() const
{
    // 根据需求，在当前Wlan未连接的情况下，才判断是否有同名Wlan中存在Wlan6，如果当前已连接，则直接判断
    if (!connected()) {
        // 如果是重名的Wlan，则判断同名的Wlan的Flags是否为Wlan6
        if (m_json.contains("extendFlags")) {
            int flag = m_json.value("extendFlags").toInt();
            if (flag & AP_FLAGS_HE)
                return true;
        }
    }

    if (m_json.contains("Flags")) {
        int flag = m_json.value("Flags").toInt();
        if (flag & AP_FLAGS_HE)
            return true;
    }

    return false;
}

void AccessPointsProxyInter::updateAccessPoints(const QJsonObject &json)
{
    int nOldStrength = strength();
    bool oldSecured = secured();
    m_json = json;
    int nStrength = strength();
    if (nOldStrength != -1 && nStrength != nOldStrength)
        Q_EMIT strengthChanged(nStrength);

    bool newSecured = secured();
    if (oldSecured != newSecured)
        Q_EMIT securedChanged(newSecured);
}

void AccessPointsProxyInter::updateConnectionStatus(ConnectionStatus status)
{
    if (m_status == status)
        return;

    m_status = status;
    Q_EMIT connectionStatusChanged(status);
}
