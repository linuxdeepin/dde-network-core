// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "networkconst.h"

namespace dde {

namespace network {

Connection::Connection()
{
}

Connection::~Connection()
{
}

QString Connection::path()
{
    return m_data.value("Path").toString();
}

QString Connection::uuid()
{
    return m_data.value("Uuid").toString();
}

QString Connection::id()
{
    return m_data.value("Id").toString();
}

QString Connection::hwAddress()
{
    return m_data.value("HwAddress").toString();
}

QString Connection::clonedAddress()
{
    return m_data.value("ClonedAddress").toString();
}

QString Connection::ssid()
{
    return m_data.value("Ssid").toString();
}

void Connection::updateConnection(const QJsonObject &data)
{
    m_data = data;
}

// 连接具体项的基类
ControllItems::ControllItems()
    : m_connection(new Connection)
    , m_status(ConnectionStatus::Unknown)
{
}

ControllItems::~ControllItems()
{
    delete m_connection;
}

Connection *ControllItems::connection() const
{
    return m_connection;
}

QString ControllItems::activeConnection() const
{
    return m_activeConnection;
}

void ControllItems::setConnection(const QJsonObject &jsonObj)
{
    m_connection->updateConnection(jsonObj);
}

void ControllItems::setActiveConnection(const QString &activeConnection)
{
    m_activeConnection = activeConnection;
}

ConnectionStatus ControllItems::status() const
{
    return m_status;
}

bool ControllItems::connected() const
{
    return (status() == ConnectionStatus::Activated);
}

void ControllItems::setConnectionStatus(const ConnectionStatus &status)
{
    m_status = status;
}

}

}
