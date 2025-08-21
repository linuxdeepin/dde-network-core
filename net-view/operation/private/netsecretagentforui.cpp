// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "netsecretagentforui.h"

// #include "networkconst.h"

// #include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTimer>

#include <unistd.h>

namespace dde {
namespace network {

static QMap<QString, void (NetSecretAgentForUI::*)(QLocalSocket *, const QByteArray &)> s_FunMap = {
    { "requestSecrets", &NetSecretAgentForUI::requestSecrets },
};

NetSecretAgentForUI::NetSecretAgentForUI(PasswordCallbackFunc fun, const QString &serverKey, QObject *parent)
    : QObject(parent)
    , NetSecretAgentInterface(fun)
    , m_client(new QLocalSocket(this))
    , m_reconnectTimer(new QTimer(this))
{
    m_serverName = "org.deepin.dde.NetworkSecretAgent" + QString::number(getuid());
    m_reconnectTimer->setInterval(1000);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetSecretAgentForUI::ConnectToServer);

    connect(m_client, &QLocalSocket::stateChanged, this, &NetSecretAgentForUI::onStateChanged);
    connect(m_client, &QLocalSocket::readyRead, this, &NetSecretAgentForUI::readyReadHandler);
    ConnectToServer();
}

bool NetSecretAgentForUI::hasTask()
{
    return !m_connectSsid.isEmpty();
}

void NetSecretAgentForUI::inputPassword(const QString &key, const QVariantMap &password, bool input)
{
    sendSecretsResult(key, password, input);
}

NetSecretAgentForUI::~NetSecretAgentForUI() = default;

void NetSecretAgentForUI::ConnectToServer()
{
    m_client->connectToServer(m_serverName);
}

void NetSecretAgentForUI::onStateChanged(QLocalSocket::LocalSocketState socketState)
{
    switch (socketState) {
    case QLocalSocket::UnconnectedState:
    case QLocalSocket::ClosingState:
        if (!m_reconnectTimer->isActive()) {
            m_reconnectTimer->start();
        }
        break;
    default:
        break;
    }
}

void NetSecretAgentForUI::sendSecretsResult(const QString &key, const QVariantMap &password, bool input)
{
    Q_UNUSED(key)
    if (m_callId.isEmpty()) {
        return;
    }
    m_connectSsid.clear();
    QByteArray data;
    QJsonObject json;
    json.insert("callId", m_callId);
    if (input) {
        QStringList secrets;
        for (auto &&secret : m_secrets) {
            secrets << password.value(secret).toString();
        }
        json.insert("secrets", QJsonArray::fromStringList(secrets));
    }
    QJsonDocument doc;
    doc.setObject(json);
    data = doc.toJson(QJsonDocument::Compact);
    m_client->write("\nsecretsResult:" + data + "\n");
    m_callId.clear();
}

void NetSecretAgentForUI::readyReadHandler()
{
    auto *socket = dynamic_cast<QLocalSocket *>(sender());
    if (!socket)
        return;

    QByteArray allData = socket->readAll();
    allData = m_lastData + allData;
    QList<QByteArray> dataArray = allData.split('\n');
    m_lastData = dataArray.last();
    for (const QByteArray &data : dataArray) {
        int keyIndex = data.indexOf(':');
        if (keyIndex != -1) {
            QString key = data.left(keyIndex);
            QByteArray value = data.mid(keyIndex + 1);
            if (s_FunMap.contains(key)) {
                (this->*s_FunMap.value(key))(socket, value);
            }
        }
    }
}

void NetSecretAgentForUI::requestSecrets(QLocalSocket *socket, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();

        QVariantMap param;
        QStringList secrets;
        QMap<QString, QString> prop;
        QString dev;
        QJsonArray devices = obj.value("devices").toArray();
        if (!devices.isEmpty()) {
            dev = devices.first().toString();
        }
        const QJsonArray &jsonSecrets = obj.value("secrets").toArray();
        for (auto &&s : jsonSecrets) {
            secrets << s.toString();
        }
        const QJsonObject &jsonProps = obj.value("props").toObject();
        for (auto p = jsonProps.constBegin(); p != jsonProps.constEnd(); ++p) {
            prop.insert(p.key(), p.value().toString());
        }
        m_callId = obj.value("callId").toString();
        m_connectDev = dev;
        m_connectSsid = obj.value("connId").toString();
        m_secrets = secrets;
        param.insert("secrets", secrets);
        if (!prop.isEmpty()) {
            param.insert("prop", QVariant::fromValue(prop));
        }
        requestPassword(dev, m_connectSsid, param);
    }
    socket->write("\nreceive:" + data + "\n");
}

} // namespace network
} // namespace dde
