// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "networksecretagent.h"

#include "constants.h"
#include "secretservice.h"

#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Setting>

#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QProcess>
#include <QSettings>
#include <QTimer>
using namespace NetworkManager;

namespace network {
namespace sessionservice {
const QString NMSecretDialogBin = "/usr/lib/deepin-daemon/dde-network-secret-dialog";
const QString ServiceTypeL2TP = "org.freedesktop.NetworkManager.l2tp";
const QString ServiceTypePPTP = "org.freedesktop.NetworkManager.pptp";
const QString ServiceTypeVPNC = "org.freedesktop.NetworkManager.vpnc";
const QString ServiceTypeOpenVPN = "org.freedesktop.NetworkManager.openvpn";
const QString ServiceTypeStrongSwan = "org.freedesktop.NetworkManager.strongswan";
const QString ServiceTypeOpenConnect = "org.freedesktop.NetworkManager.openconnect";
const QString ServiceTypeSSTP = "org.freedesktop.NetworkManager.sstp";

const QMap<QString, QStringList> SecretSettingKeys = { { "802-11-wireless-security", { "psk", "wep-key0", "wep-key1", "wep-key2", "wep-key3", "leap-password" } },
                                                       { "802-1x", { "password", "password-raw", "ca-cert-password", "client-cert-password", "phase2-ca-cert-password", "phase2-client-cert-password", "private-key-password", "phase2-private-key-password", "pin" } },
                                                       // temporarily not supported password-raw
                                                       { "pppoe", { "password" } },
                                                       { "gsm", { "password", "pin" } },
                                                       { "cdma", { "password" } } };
const QStringList VpnSecretKeys = { "password", "proxy-password", "IPSec secret", "Xauth password", "cert-pass" };

const QStringList EapKeys = { "md5", "fast", "ttls", "peap", "leap" };

// GetSecrets超时时间，参考NM中nm-secret-agent.c中函数nm_secret_agent_get_secrets里的设置
#define GET_SECRETS_TIMEOUT 120000

static QMap<QString, void (NetworkSecretAgent::*)(QLocalSocket *, const QByteArray &)> s_FunMap = {
    { "requestSecrets", &NetworkSecretAgent::requestSecrets },
    { "secretsResult", &NetworkSecretAgent::secretsResult },
};

struct SettingItem
{
    QString settingName;
    QString settingKey;
    QString value;
    QString label;
};

NetworkSecretAgent::NetworkSecretAgent(QObject *parent)
    : NetworkManager::SecretAgent(QStringLiteral("com.deepin.system.network.SecretAgent"), parent)
    , m_secretService(new SecretService(this))
{
    m_server = new QLocalServer(this);
    connect(m_server, &QLocalServer::newConnection, this, &NetworkSecretAgent::newConnectionHandler);
    m_server->setSocketOptions(QLocalServer::WorldAccessOption);
    QString serverName = "org.deepin.dde.NetworkSecretAgent" + QString::number(getuid());
    qCDebug(DSM()) << "start server name" << serverName;
    if (!m_server->listen(serverName)) {
        qCWarning(DSM()) << "start listen server failure" << m_server->errorString();
    }
}

NMVariantMapMap NetworkSecretAgent::GetSecrets(const NMVariantMapMap &connection, const QDBusObjectPath &connectionPath, const QString &settingName, const QStringList &hints, uint flags)
{
    qCDebug(DSM()) << "Get secrets, path: " << connectionPath.path() << ", setting name: " << settingName << ", hints: " << hints << ", flags: " << flags;

    const QString callId = connectionPath.path() % settingName;
    for (const SecretsRequest &request : m_calls) {
        if (request == callId) {
            qCWarning(DSM()) << "Get secrets was called again! This should not happen, cancelling first call, connection path: " << connectionPath.path() << ", setting name: " << settingName;
            CancelGetSecrets(connectionPath, settingName);
            break;
        }
    }

    setDelayedReply(true);
    SecretsRequest request(SecretsRequest::GetSecrets);
    request.callId = callId;
    request.connection = connection;
    request.connectionPath = connectionPath;
    request.flags = static_cast<NetworkManager::SecretAgent::GetSecretsFlags>(flags);
    request.hints = hints;
    request.settingName = settingName;
    request.message = message();
    m_calls << request;

    processNext();
    QTimer::singleShot(GET_SECRETS_TIMEOUT, this, &NetworkSecretAgent::onGetSecretsTimeout);
    return {};
}

void NetworkSecretAgent::SaveSecrets(const NMVariantMapMap &connection, const QDBusObjectPath &connectionPath)
{
    setDelayedReply(true);
    SecretsRequest::Type type;
    if (hasSecrets(connection)) {
        type = SecretsRequest::SaveSecrets;
    } else {
        type = SecretsRequest::DeleteSecrets;
    }
    SecretsRequest request(type);
    request.connection = connection;
    request.connectionPath = connectionPath;
    request.message = message();
    m_calls << request;

    processNext();
}

void NetworkSecretAgent::DeleteSecrets(const NMVariantMapMap &connection, const QDBusObjectPath &connectionPath)
{
    setDelayedReply(true);
    SecretsRequest request(SecretsRequest::DeleteSecrets);
    request.connection = connection;
    request.connectionPath = connectionPath;
    request.message = message();
    m_calls << request;

    processNext();
}

void NetworkSecretAgent::CancelGetSecrets(const QDBusObjectPath &connectionPath, const QString &settingName)
{
    QString callId = connectionPath.path() % settingName;
    setDelayedReply(true);
    for (int i = 0; i < m_calls.size(); ++i) {
        SecretsRequest request = m_calls.at(i);
        if (request.type == SecretsRequest::GetSecrets && callId == request.callId) {
            qCDebug(DSM()) << "Process finished (agent canceled):" << request.ssid;
            sendError(SecretAgent::AgentCanceled, QStringLiteral("Agent canceled the password dialog"), request.message);
            QProcess *process = request.process;
            m_calls.removeAt(i);
            if (process) {
                process->kill();
            }
            break;
        }
    }
    dbusConnection().send(message().createReply()); //  非异步，
    processNext();
}

void NetworkSecretAgent::onGetSecretsTimeout()
{
    qint64 time = QDateTime::currentDateTime().toMSecsSinceEpoch() + 1000 - GET_SECRETS_TIMEOUT;
    for (int i = 0; i < m_calls.size(); ++i) {
        SecretsRequest request = m_calls.at(i);
        if (request.type == SecretsRequest::GetSecrets && request.createTime <= time) {
            qCDebug(DSM()) << "Process finished (Timeout):" << request.ssid;
            m_calls.removeAt(i);
            break;
        }
    }
}

void NetworkSecretAgent::askPasswords(SecretsRequest &request, const QStringList &settingKeys, bool requestNew, uint secretFlag, NMStringMap props)
{
    const QString &connPath = request.connectionPath.path();
    const NMVariantMapMap &connection = request.connection;
    const QString &connUUID = connection.value("connection").value("UUID").toString();
    const QString &connId = connection.value("connection").value("id").toString();
    const QString &connType = connection.value("connection").value("type").toString();
    const QString &vpnService = connection.value("vpn").value("service").toString();
    qCDebug(DSM()) << "askPasswords settingName:" << request.settingName << ", settingKeys:" << settingKeys;
    // search connection in active connections
    // if found, record device paths and specific object of this active connection
    QStringList devPaths;
    QString specific;
    for (auto &&active : NetworkManager::activeConnections()) {
        if (active->connection()->path() != connPath) {
            continue;
        }
        // copy device path slice
        devPaths = active->devices();
        // copy specific obj
        specific = active->specificObject();
        break;
    }

    // convert object path slice to string slice
    QJsonArray secrets = QJsonArray::fromStringList(settingKeys);
    QJsonArray devices = QJsonArray::fromStringList(devPaths);
    QJsonObject propsObj;
    for (auto it = props.begin(); it != props.end(); ++it) {
        propsObj.insert(it.key(), it.value());
    }
    QJsonObject req;
    req.insert("callId", request.callId);
    req.insert("connId", connId);
    req.insert("connType", connType);
    req.insert("connUUID", connUUID);
    req.insert("vpnService", vpnService);
    req.insert("settingName", request.settingName);
    req.insert("secrets", secrets);
    req.insert("requestNew", requestNew);
    req.insert("specific", specific);
    req.insert("devices", devices);
    req.insert("secretFlag", (int)secretFlag);
    req.insert("props", propsObj);
    QString reqJSON = QJsonDocument(req).toJson(QJsonDocument::Compact);
    qCDebug(DSM()) << "reqJSON:" << reqJSON;
    request.inputCache = reqJSON.toUtf8();
    // 无线网密码拉起任务栏网络面板，其他使用密码输入弹窗
    if (connType == "802-11-wireless" && !m_clients.isEmpty()) {
        for (auto &&client : m_clients) {
            client->write("\nrequestSecrets:" + reqJSON.toUtf8() + "\n");
        }
    } else {
        // run auth dialog
        qCInfo(DSM()) << "run auth dialog:" << NMSecretDialogBin;
        QProcess *process = new QProcess(this);
        process->setProperty("callId", request.callId);
        request.process = process;
        connect(process, &QProcess::finished, this, &NetworkSecretAgent::authDialogFinished);
        connect(process, &QProcess::readyReadStandardOutput, this, &NetworkSecretAgent::authDialogReadOutput);
        connect(process, &QProcess::readyReadStandardError, this, &NetworkSecretAgent::authDialogReadError);
        connect(process, &QProcess::errorOccurred, this, &NetworkSecretAgent::authDialogError);
        connect(process, &QProcess::started, this, &NetworkSecretAgent::authDialogStarted);
        QTimer::singleShot(GET_SECRETS_TIMEOUT, process, &QProcess::kill);
        process->start(NMSecretDialogBin);
    }
}

void NetworkSecretAgent::newConnectionHandler()
{
    QLocalSocket *socket = m_server->nextPendingConnection();
    connect(socket, &QLocalSocket::readyRead, this, &NetworkSecretAgent::readyReadHandler);
    connect(socket, &QLocalSocket::disconnected, this, &NetworkSecretAgent::disconnectedHandler);
    QTimer::singleShot(GET_SECRETS_TIMEOUT, socket, &QLocalSocket::disconnectFromServer);
    m_clients.append(socket);
}

void NetworkSecretAgent::disconnectedHandler()
{
    auto *socket = dynamic_cast<QLocalSocket *>(sender());
    if (socket) {
        m_clients.removeAll(socket);
        socket->deleteLater();
    }
}

void NetworkSecretAgent::readyReadHandler()
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

void NetworkSecretAgent::requestSecrets(QLocalSocket *socket, const QByteArray &data) { }

void NetworkSecretAgent::secretsResult(QLocalSocket *socket, const QByteArray &data)
{
    doSecretsResult(QString(), data, true);
}

void NetworkSecretAgent::vpnDialogFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    vpnDialogReadAllOutput(true);
}

void NetworkSecretAgent::vpnDialogReadOutput()
{
    vpnDialogReadAllOutput(false);
}

void NetworkSecretAgent::vpnDialogReadError()
{
    vpnDialogReadAllOutput(false);
}

void NetworkSecretAgent::vpnDialogError(QProcess::ProcessError error)
{
    qCWarning(DSM()) << "vpn dialog error:" << error;
    vpnDialogReadAllOutput(true);
}

void NetworkSecretAgent::vpnDialogStarted()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }
    QString callId = process->property("callId").toString();
    SecretsRequest *request = nullptr;
    for (auto &&call : m_calls) {
        if (call.callId == callId) {
            request = &call;
        }
    }
    if (!request) {
        return;
    }
    NMStringMap vpnData = request->connection.value("vpn").value("data").value<NMStringMap>();
    NMStringMap vpnSecretData = request->connection.value("vpn").value("secrets").value<NMStringMap>();
    // send vpn connection data to the authentication dialog binary
    for (auto it = vpnData.begin(); it != vpnData.end(); ++it) {
        QString data = "DATA_KEY=" + it.key() + "\n" + "DATA_VAL=" + it.value() + "\n\n";
        process->write(data.toUtf8());
    }
    for (auto it = vpnSecretData.begin(); it != vpnSecretData.end(); ++it) {
        QString data = "SECRET_KEY=" + it.key() + "\n" + "SECRET_VAL=" + it.value() + "\n\n";
        process->write(data.toUtf8());
    }
    process->write("DONE\n\n");
}

void NetworkSecretAgent::vpnDialogReadAllOutput(bool isEnd)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }
    QString callId = process->property("callId").toString();
    SecretsRequest *request = nullptr;
    for (auto &&call : m_calls) {
        if (call.callId == callId) {
            request = &call;
        }
    }
    if (!request) {
        return;
    }
    if (isEnd) {
        request->process = nullptr;
        process->deleteLater();
    }
    QByteArray data = process->readAll();
    request->outputCache += data;
    QList<QByteArray> dataAll = request->outputCache.split('\n');

    QVariantMap newVpnSecretData;
    QString lastKey;
    // read output until there are two empty lines printed
    int emptyLines = 0;
    for (auto &&lineBytes : dataAll) {
        QString line = lineBytes;
        if (line.isEmpty()) {
            emptyLines++;
        } else {
            // the secrets key and value are split as line
            if (lastKey.isEmpty()) {
                lastKey = line;
            } else {
                newVpnSecretData.insert(lastKey, line);
                lastKey.clear();
            }
        }
        if (emptyLines == 2) {
            break;
        }
    }
    if (isEnd || emptyLines == 2) {
        process->write("QUIT\n\n");
        request->result.insert("secrets", newVpnSecretData);
        dbusConnection().send(request->message.createReply(QVariant::fromValue(request->result)));
        request->status = SecretsRequest::End;
        m_calls.removeAll(*request);
    }
}

void NetworkSecretAgent::authDialogFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    authDialogReadAllOutput(true);
}

void NetworkSecretAgent::authDialogReadOutput()
{
    authDialogReadAllOutput(false);
}

void NetworkSecretAgent::authDialogReadError()
{
    authDialogReadAllOutput(false);
}

void NetworkSecretAgent::authDialogError(QProcess::ProcessError error)
{
    qCWarning(DSM()) << "auth dialog error:" << error;
    authDialogReadAllOutput(true);
}

void NetworkSecretAgent::authDialogStarted()
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }
    QString callId = process->property("callId").toString();
    SecretsRequest *request = nullptr;
    for (auto &&call : m_calls) {
        if (call.callId == callId) {
            request = &call;
        }
    }
    if (!request) {
        return;
    }
    process->write(request->inputCache);
    process->closeWriteChannel();
}

void NetworkSecretAgent::authDialogReadAllOutput(bool isEnd)
{
    QProcess *process = qobject_cast<QProcess *>(sender());
    if (!process) {
        return;
    }
    QString callId = process->property("callId").toString();
    QByteArray data = process->readAll();
    doSecretsResult(callId, data, isEnd);
    if (isEnd) {
        process->deleteLater();
    }
}

void NetworkSecretAgent::doSecretsResult(QString callId, const QByteArray &data, bool isEnd)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject rootObj = doc.object();
    if (callId.isEmpty()) {
        callId = rootObj.value("callId").toString();
    }
    SecretsRequest *request = nullptr;
    for (auto &&call : m_calls) {
        if (call.callId == callId) {
            request = &call;
        }
    }
    if (!request) {
        return;
    }
    if (isEnd) {
        request->process = nullptr;
    }
    if (!rootObj.contains("secrets")) { // 没有"secrets"时为用户取消
        if (isEnd) {
            sendError(Error::UserCanceled, "user canceled", request->message);
            request->status = SecretsRequest::End;
            m_calls.removeAll(*request);
        }
        return;
    }
    QJsonArray secrets = rootObj.value("secrets").toArray();
    QJsonDocument inputDoc = QJsonDocument::fromJson(request->inputCache);
    QJsonObject inputRoot = inputDoc.object();
    QJsonArray settingKeys = inputRoot.value("secrets").toArray();

    if (secrets.size() != settingKeys.size()) {
        sendError(Error::NoSecrets, "secretAgent.askPasswords: length not equal", request->message);
        request->status = SecretsRequest::End;
        m_calls.removeAll(*request);
        qCWarning(DSM()) << "secretAgent.askPasswords: length not equal" << secrets.size() << settingKeys.size() << secrets << settingKeys << request->inputCache;
        return;
    }
    NMStringMap result;
    QVariantMap setting;
    for (int i = 0; i < secrets.size(); ++i) {
        result.insert(settingKeys.at(i).toString(), secrets.at(i).toString());
    }

    if (request->settingName == "vpn") {
        QString connUUID = request->connection.value("connection").value("uuid").toString();
        QMap<QString, QString> resultSaved = m_secretService->getAll(connUUID, request->settingName);
        qCDebug(DSM()) << "getAll resultSaved:" << resultSaved;
        for (auto it = resultSaved.begin(); it != resultSaved.end(); ++it) {
            if (result.contains(it.key())) { // TODO: 待验证，不应该走到这
                result.insert(it.key(), it.value());
            } else {
                qCDebug(DSM()) << "not override key" << it.key();
            }
        }
        setting.insert("secrets", QVariant::fromValue(result));
    } else {
        QString connId = inputRoot.value("connId").toString();
        QString connUUID = request->connection.value("connection").value("uuid").toString();
        QJsonObject propsObj = inputRoot.value("props").toObject();
        NMStringMap propMap;
        for (auto it = propsObj.begin(); it != propsObj.end(); ++it) {
            propMap.insert(it.key(), it.value().toString());
        }
        QList<SettingItem> items;
        for (auto it = result.begin(); it != result.end(); ++it) {
            if (it.value() == propMap.value(it.key())) {
                continue;
            }
            setting.insert(it.key(), it.value());
            bool ok = false;
            uint secretFlags = request->connection.value(request->settingName).value(getSecretFlagsKeyName(it.key())).toUInt(&ok);
            if (secretFlags == Setting::AgentOwned) {
                QString valueStr = setting.value(it.key()).toString();
                QString label = QString("Network secret for %1/%2/%3").arg(connId).arg(request->settingName).arg(it.key());
                SettingItem item;
                item.settingName = request->settingName;
                item.settingKey = it.key();
                item.value = valueStr;
                item.label = label;
                items.append(item);
            }
        }
        for (auto &&item : items) {
            m_secretService->setData(item.label, connUUID, item.settingName, item.settingKey, item.value);
        }
        QMap<QString, QString> resultSaved = m_secretService->getAll(connUUID, request->settingName);
        // if (resultSaved.isEmpty()) {
        //     return ;
        // }
        qCDebug(DSM()) << "getAll resultSaved:" << resultSaved;
        for (auto it = resultSaved.begin(); it != resultSaved.end(); ++it) {
            bool ok = false;
            uint secretFlags = request->connection.value(request->settingName).value(getSecretFlagsKeyName(it.key())).toUInt(&ok);
            if (secretFlags == Setting::AgentOwned) {
                setting.insert(it.key(), it.value());
            }
        }
    }
    request->result.insert(request->settingName, setting);
    dbusConnection().send(request->message.createReply(QVariant::fromValue(request->result)));
    request->status = SecretsRequest::End;
    m_calls.removeAll(*request);
}

void NetworkSecretAgent::processNext()
{
    for (auto it = m_calls.begin(); it != m_calls.end();) {
        SecretsRequest &request = *it;
        if (request.status != SecretsRequest::Begin) {
            continue;
        }
        switch (request.type) {
        case SecretsRequest::GetSecrets:
            if (processGetSecrets(request)) {
                it = m_calls.erase(it);
                continue;
            }
            break;
        case SecretsRequest::SaveSecrets:
            if (processSaveSecrets(request)) {
                it = m_calls.erase(it);
                continue;
            }
            break;
        case SecretsRequest::DeleteSecrets:
            if (processDeleteSecrets(request)) {
                it = m_calls.erase(it);
                continue;
            }
            break;
        default:
            break;
        }
        ++it;
    }
}

bool NetworkSecretAgent::processGetSecrets(SecretsRequest &request)
{
    qCDebug(DSM()) << "call getSecrets";
    bool allowInteraction = false; // 允许交互
    bool requestNew = false;
    if (request.flags & SecretAgent::AllowInteraction) {
        qCDebug(DSM()) << "allow interaction";
        allowInteraction = true;
    }
    if (request.flags & SecretAgent::RequestNew) {
        qCDebug(DSM()) << "request new";
        requestNew = true;
    }

    qCDebug(DSM()) << "connection path:" << request.connectionPath;
    qCDebug(DSM()) << "setting Name:" << request.settingName;
    qCDebug(DSM()) << "hints:" << request.hints;
    qCDebug(DSM()) << "flags:" << request.flags;
    qCDebug(DSM()) << "connection Data:" << request.connection;
    QString connUUID = request.connection.value("connection").value("uuid").toString();
    if (connUUID.isEmpty()) {
        sendError(Error::InvalidConnection, "not found connection uuid", request.message);
        return true;
    }
    QString connId = request.connection.value("connection").value("id").toString();
    qCDebug(DSM()) << "uuid" << connUUID;
    QVariantMap setting;
    NMStringMap propMap;
    NMStringMap vpnSecretsData;
    uint secretFlag = 0;
    if (request.settingName == "vpn") {
        if (request.connection.value("vpn").value("service-type").toString() == ServiceTypeOpenConnect) {
            // 调用nm的VPN对话框
            request.status = SecretsRequest::WaitDialog;
            createPendingKey(request);
            return false;
        } else {
            vpnSecretsData = request.connection.value("vpn").value("secrets").value<NMStringMap>();
            NMStringMap vpnDataMap = request.connection.value("vpn").value("data").value<NMStringMap>();
            QStringList askItems;
            for (auto &&secretKey : VpnSecretKeys) {
                bool ok = false;
                auto secretFlag = vpnDataMap[getSecretFlagsKeyName(secretKey)].toUInt(&ok);
                if (secretFlag == Setting::NotSaved) {
                    qCDebug(DSM()) << "ask for password" << request.settingName << secretKey;
                    askItems.append(secretKey);
                }
            }
            if (allowInteraction && !askItems.isEmpty()) {
                // 调用密码输入对话框
                request.status = SecretsRequest::WaitDialog;
                askPasswords(request, askItems, requestNew, secretFlag, propMap);
                return false;
            }
        }
        QMap<QString, QString> resultSaved = m_secretService->getAll(connUUID, request.settingName);
        if (resultSaved.isEmpty()) {
            dbusConnection().send(request.message.createReply(QVariant::fromValue(NMVariantMapMap())));
            return true;
        }
        qCDebug(DSM()) << "getAll resultSaved:" << resultSaved;
        for (auto it = resultSaved.begin(); it != resultSaved.end(); ++it) {
            if (vpnSecretsData.contains(it.key())) {
                vpnSecretsData.insert(it.key(), it.value());
            } else {
                qCDebug(DSM()) << "not override key" << it.key();
            }
        }
        sendError(Error::InvalidConnection, "not found connection uuid", request.message);
        setting.insert("secrets", QVariant::fromValue(vpnSecretsData));
    } else if (SecretSettingKeys.contains(request.settingName)) {

        QStringList askItems;
        auto secretKeys = SecretSettingKeys.value(request.settingName);
        for (auto secretKey : secretKeys) {
            bool ok = false;
            uint secretFlags = request.connection.value(request.settingName).value(getSecretFlagsKeyName(secretKey)).toUInt(&ok);
            if (ok) {
                secretFlag = secretFlags;
            }
            switch (secretFlags) {
            case Setting::NotSaved:
                if (allowInteraction && isMustAsk(request.connection, request.settingName, secretKey)) {
                    askItems.append(secretKey);
                }
                break;
            case Setting::None: {
                QString secretStr = request.connection.value(request.settingName).value(secretKey).toString();
                if (!requestNew && !secretStr.isEmpty()) {
                    setting.insert(secretKey, secretStr);
                } else if (allowInteraction && isMustAsk(request.connection, request.settingName, secretKey)) {
                    askItems.append(secretKey);
                    if (!secretStr.isEmpty()) {
                        propMap.insert(secretKey, secretStr);
                    }
                }
            } break;
            case Setting::AgentOwned:
                if (requestNew) {
                    // check if NMSecretAgentGetSecretsFlags contains NM_SECRET_AGENT_GET_SECRETS_FLAG_REQUEST_NEW
                    // if is, means the password we set last time is incorrect, new password is needed
                    if (allowInteraction && isMustAsk(request.connection, request.settingName, secretKey)) {
                        askItems.append(secretKey);
                    }
                } else {
                    // 从密钥环获取
                    auto resultSaved = m_secretService->getAll(connUUID, request.settingName);
                    qCDebug(DSM()) << "getAll resultSaved: " << resultSaved;
                    if (resultSaved.isEmpty() && allowInteraction && isMustAsk(request.connection, request.settingName, secretKey)) {
                        askItems.append(secretKey);
                    }
                }
                break;
            default:
                break;
            }
        }

        if (allowInteraction && !askItems.isEmpty()) {
            // 把需要的属性加上去
            auto props = askProps(request.connection, request.settingName);
            for (auto key : props) {
                if (request.connection.value(request.settingName).contains(key)) {
                    QString val = request.connection.value(request.settingName).value(key).toString();
                    propMap.insert(key, val);
                }
            }
            // 属性放前面问询
            props.append(askItems);
            askItems = props;
            request.status = SecretsRequest::WaitDialog;
            askPasswords(request, askItems, requestNew, secretFlag, propMap);
            return false;
        }

        QMap<QString, QString> resultSaved = m_secretService->getAll(connUUID, request.settingName);
        if (resultSaved.isEmpty()) {
            dbusConnection().send(request.message.createReply(QVariant::fromValue(NMVariantMapMap())));
            return true;
        }
        qCDebug(DSM()) << "getAll resultSaved:" << resultSaved;
        for (auto it = resultSaved.begin(); it != resultSaved.end(); ++it) {
            bool ok = false;
            uint secretFlags = request.connection.value(request.settingName).value(getSecretFlagsKeyName(it.key())).toUInt(&ok);
            if (secretFlags == Setting::AgentOwned) {
                setting.insert(it.key(), it.value());
            }
        }
    }
    NMVariantMapMap secretsData;
    secretsData.insert(request.settingName, setting);
    dbusConnection().send(request.message.createReply(QVariant::fromValue(secretsData)));
    return true;
}

bool NetworkSecretAgent::processSaveSecrets(SecretsRequest &request) const
{
    qCDebug(DSM()) << "call saveSecrets" << request.connectionPath << request.connection;
    QString connUUID = request.connection.value("connection").value("uuid").toString();
    if (connUUID.isEmpty()) {
        sendError(Error::InvalidConnection, "not found connection uuid", request.message);
        return true;
    }
    qCDebug(DSM()) << "uuid:" << connUUID;
    QString connId = request.connection.value("connection").value("id").toString();
    qCDebug(DSM()) << "conn id:" << connId;
    QString vpnServiceType = request.connection.value("vpn").value("service-type").toString();
    int dotLastIdx = vpnServiceType.lastIndexOf(".");
    if (dotLastIdx != -1) {
        vpnServiceType = vpnServiceType.mid(dotLastIdx + 1);
    }
    QList<SettingItem> arr;
    for (auto it = request.connection.begin(); it != request.connection.end(); ++it) {
        if (it.key() == "vpn") {
            NMStringMap vpnDataMap = qdbus_cast<NMStringMap>(it.value()["data"]);
            if (!vpnDataMap.isEmpty()) {
                qCDebug(DSM()) << "vpn.data map:" << vpnDataMap;
            }
            NMStringMap secretMap = qdbus_cast<NMStringMap>(it.value()["secrets"]);
            if (!secretMap.isEmpty()) {
                qCDebug(DSM()) << "vpn.secret map:" << secretMap;
                for (auto secretIt = secretMap.begin(); secretIt != secretMap.end(); ++secretIt) {
                    bool ok = false;
                    uint secretFlags = vpnDataMap[getSecretFlagsKeyName(secretIt.key())].toUInt(&ok);
                    if (ok && secretFlags == Setting::AgentOwned) {
                        QString label = QString("VPN password secret for %1/%2/%3").arg(connId).arg(vpnServiceType).arg(secretIt.key());
                        SettingItem item;
                        item.settingName = it.key();
                        item.settingKey = secretIt.key();
                        item.value = secretIt.value();
                        item.label = label;
                        arr.append(item);
                    }
                }
            }
            continue;
        }
        auto secretKeys = SecretSettingKeys[it.key()];
        for (auto sIt = it.value().begin(); sIt != it.value().end(); ++sIt) {
            if (secretKeys.contains(sIt.key())) {
                // key is secret key
                bool ok = false;
                uint secretFlags = request.connection.value(it.key()).value(getSecretFlagsKeyName(sIt.key())).toUInt(&ok);
                if (secretFlags != Setting::AgentOwned) {
                    // not agent owned
                    continue;
                }
                if (sIt.value().canConvert(QMetaType(QMetaType::QString))) {
                    QString valueStr = sIt.value().toString();
                    SettingItem item;
                    item.settingName = it.key();
                    item.settingKey = sIt.key();
                    item.value = valueStr;
                    arr.append(item);
                }
            }
        }
    }
    for (auto &&item : arr) {
        QString label = item.label;
        if (label.isEmpty()) {
            label = QString("Network secret for %1/%2/%3").arg(connId).arg(item.settingName).arg(item.settingKey);
        }
        QString err = m_secretService->setData(label, connUUID, item.settingName, item.settingKey, item.value);
        if (!err.isEmpty()) {
            qCDebug(DSM()) << "failed to save Secret to keyring";
            sendError(Error::InvalidConnection, err, request.message);
            return true;
        }
    }
    // delete
    for (auto it = SecretSettingKeys.begin(); it != SecretSettingKeys.end(); ++it) {
        for (auto keyIt = it.value().begin(); keyIt != it.value().end(); ++keyIt) {
            bool ok = false;
            uint secretFlags = request.connection.value(it.key()).value(getSecretFlagsKeyName(*keyIt)).toUInt(&ok);
            if (ok && secretFlags != Setting::AgentOwned) {
                QString err = m_secretService->deleteData(connUUID, it.key(), *keyIt);
                if (!err.isEmpty()) {
                    qCDebug(DSM()) << "failed to delete secret";
                    sendError(Error::InvalidConnection, err, request.message);
                    return true;
                }
            }
        }
    }
    NMStringMap vpnData = request.connection.value("vpn").value("data").value<NMStringMap>();
    if (!vpnData.isEmpty()) {
        for (auto &&secretKey : VpnSecretKeys) {
            bool ok = false;
            uint secretFlags = vpnData[getSecretFlagsKeyName(secretKey)].toUInt(&ok);
            if (ok && secretFlags != Setting::AgentOwned) {
                QString err = m_secretService->deleteData(connUUID, "vpn", secretKey);
                if (!err.isEmpty()) {
                    qCDebug(DSM()) << "failed to delete secret";
                    sendError(Error::InvalidConnection, err, request.message);
                    return true;
                }
            }
        }
    }
    dbusConnection().send(request.message.createReply());
    return true;
}

bool NetworkSecretAgent::processDeleteSecrets(SecretsRequest &request) const
{
    qCDebug(DSM()) << "call DeleteSecrets" << request.connection;
    QString connUUID = request.connection.value("connection").value("uuid").toString();
    if (connUUID.isEmpty()) {
        sendError(Error::InvalidConnection, "not found connection uuid", request.message);
        return true;
    }
    QString err = m_secretService->deleteAll(connUUID);
    if (!err.isEmpty()) {
        sendError(Error::InvalidConnection, err, request.message);
        return true;
    }
    dbusConnection().send(request.message.createReply());
    return true;
}

bool NetworkSecretAgent::hasSecrets(const NMVariantMapMap &connection) const
{
    NetworkManager::ConnectionSettings connectionSettings(connection);
    for (const NetworkManager::Setting::Ptr &setting : connectionSettings.settings()) {
        if (!setting->secretsToMap().isEmpty()) {
            return true;
        }
    }
    return false;
}

QString NetworkSecretAgent::getSecretFlagsKeyName(const QString &key) const
{
    if (key.startsWith("wep-key")) {
        bool ok = false;
        int num = key.right(1).toInt(&ok);
        if (ok && num >= 0 && num <= 3) {
            // num in range [0,3]
            return "wep-key-flags";
        }
    }
    // case nm dont hve sae-flags, reuse psk-flags at this time
    if (key == "sae") {
        return "psk-flags";
    }
    return key + "-flags";
}

// 根据当前连接设置，找出必要的密码key。
bool NetworkSecretAgent::isMustAsk(const NMVariantMapMap &connection, const QString &settingName, const QString &secretKey) const
{
    QString mgmt = connection.value("802-11-wireless-security").value("key-mgmt").toString();
    if (settingName == "802-11-wireless-security") {
        uint wepTxKeyIdx = connection.value("802-11-wireless-security").value("wep-tx-keyidx").toUInt();
        if (mgmt == "wpa-psk") {
            if (secretKey == "psk") {
                return true;
            }
        } else if (mgmt == "sae") {
            if (secretKey == "psk") {
                return true;
            }
        } else if (mgmt == "none") {
            if (secretKey == "wep-key0" && wepTxKeyIdx == 0) {
                return true;
            } else if (secretKey == "wep-key1" && wepTxKeyIdx == 1) {
                return true;
            } else if (secretKey == "wep-key2" && wepTxKeyIdx == 2) {
                return true;
            } else if (secretKey == "wep-key3" && wepTxKeyIdx == 3) {
                return true;
            }
        }
    } else if (settingName == "802-1x") {
        QStringList eap = connection.value("802-1x").value("eap").toStringList();
        QString eap0;
        if (eap.size() >= 1) {
            eap0 = eap.at(0);
        }
        if (eap0 == "tls") {
            if (secretKey == "private-key-password") {
                return true;
            }
        }
        if (EapKeys.contains(eap0)) {
            if (secretKey == "password") {
                return true;
            }
        }
    }
    return false;
}

QStringList NetworkSecretAgent::askProps(const NMVariantMapMap &connection, const QString &settingName) const
{
    if (settingName == "802-1x") {
        QStringList eap = connection.value("802-1x").value("eap").toStringList();
        QString eap0;
        if (eap.size() >= 1) {
            eap0 = eap.at(0);
        }
        if (eap0 == "tls") {
            return { "identity" };
        }
        if (EapKeys.contains(eap0)) {
            return { "identity" };
        }
    }
    return {};
}

bool NetworkSecretAgent::createPendingKey(SecretsRequest &request)
{
    // for vpn connections, ask password for vpn auth dialogs
    QString vpnAuthDilogBin = getVpnAuthDialogBin(request.connection);
    if (vpnAuthDilogBin.isEmpty()) {
        return false;
    }
    QStringList args;
    QString connUUID = request.connection.value("connection").value("uuid").toString();
    QString connID = request.connection.value("connection").value("id").toString();
    QString vpnType = request.connection.value("vpn").value("service-type").toString();
    args << "-u" << connUUID << "-n" << connID << "-s" << vpnType;
    if (request.flags & SecretAgent::AllowInteraction) {
        args << "-i";
    }
    if (request.flags & SecretAgent::RequestNew) {
        args << "-r";
    }
    // add hints
    for (auto &&h : request.hints) {
        args << "-t" << h;
    }
    // run vpn auth dialog
    qCInfo(DSM()) << "run vpn auth dialog:" << vpnAuthDilogBin << args;
    QProcess *process = new QProcess(this);
    process->setProperty("callId", request.callId);
    request.process = process;
    connect(process, &QProcess::finished, this, &NetworkSecretAgent::vpnDialogFinished);
    connect(process, &QProcess::readyReadStandardOutput, this, &NetworkSecretAgent::vpnDialogReadOutput);
    connect(process, &QProcess::readyReadStandardError, this, &NetworkSecretAgent::vpnDialogReadError);
    connect(process, &QProcess::errorOccurred, this, &NetworkSecretAgent::vpnDialogError);
    connect(process, &QProcess::started, this, &NetworkSecretAgent::vpnDialogStarted);
    QTimer::singleShot(GET_SECRETS_TIMEOUT, process, &QProcess::kill);
    process->start(vpnAuthDilogBin, args);

    return true;
}

QString NetworkSecretAgent::getVpnAuthDialogBin(const NMVariantMapMap &connection)
{
    QString vpnType = connection.value("vpn").value("service-type").toString();
    QString baseName;
    if (vpnType == ServiceTypeL2TP) {
        baseName = "nm-l2tp-service.name";
    } else if (vpnType == ServiceTypeOpenConnect) {
        baseName = "nm-openconnect-service.name";
    } else if (vpnType == ServiceTypeOpenVPN) {
        baseName = "nm-openvpn-service.name";
    } else if (vpnType == ServiceTypePPTP) {
        baseName = "nm-pptp-service.name";
    } else if (vpnType == ServiceTypeStrongSwan) {
        baseName = "nm-strongswan-service.name";
    } else if (vpnType == ServiceTypeVPNC) {
        baseName = "nm-vpnc-service.name";
    } else {
        return QString();
    }
    QString vpnNameFile;
    const QStringList vpnDirs{ "/etc/NetworkManager/VPN/", "/usr/lib/NetworkManager/VPN/" };
    for (auto &&dir : vpnDirs) {
        if (QFile::exists(dir + baseName)) {
            vpnNameFile = dir + baseName;
            break;
        }
    }
    if (vpnNameFile.isEmpty()) {
        return QString();
    }
    // QFile vpnFile(vpnNameFile);
    QSettings vpnFile(vpnNameFile, QSettings::IniFormat);
    vpnFile.beginGroup("GNOME");
    QString authdialog = vpnFile.value("auth-dialog").toString();
    vpnFile.endGroup();
    return authdialog;
}

} // namespace sessionservice
} // namespace network
