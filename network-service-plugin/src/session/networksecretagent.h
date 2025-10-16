// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NETWORKSECRETAGENT_H
#define NETWORKSECRETAGENT_H

#include <NetworkManagerQt/SecretAgent>

#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDateTime>
#include <QLocalServer>
#include <QObject>
#include <QProcess>

namespace network {
namespace sessionservice {
class SecretService;

class SecretsRequest
{
public:
    enum Type {
        GetSecrets,
        SaveSecrets,
        DeleteSecrets,
    };

    enum Status { Begin, WaitDialog, End };

    explicit SecretsRequest(Type _type)
        : type(_type)
        , flags(NetworkManager::SecretAgent::None)
        , saveSecretsWithoutReply(false)
        , createTime(QDateTime::currentDateTime().toMSecsSinceEpoch())
        , status(Begin)
        , process(nullptr)
    {
    }

    inline bool operator==(const QString &other) const { return callId == other; }

    inline bool operator==(const SecretsRequest &other) const { return callId == other.callId; }

    Type type;
    Status status;
    QString callId;
    NMVariantMapMap connection;
    QDBusObjectPath connectionPath;
    QString settingName;
    QStringList hints;
    NetworkManager::SecretAgent::GetSecretsFlags flags;
    /**
     * When a user connection is called on GetSecrets,
     * the secret agent is supposed to save the secrets
     * typed by user, when true proccessSaveSecrets
     * should skip the DBus reply.
     */
    bool saveSecretsWithoutReply;
    QDBusMessage message;
    QString ssid;
    qint64 createTime;
    NMVariantMapMap result; // 返回结果
    QByteArray inputCache;  // 输入缓存
    QByteArray outputCache; // 输出缓存
    QProcess *process;      // 密码输入框进程
};

// 注册网络密码代理
// 解锁密钥环，在密钥环读写密码
// 注册QLocalServer，供任务栏、锁屏插件通信
// 需要密码时，同时发给插件，由插件判断该谁处理
// 120s超时
class NetworkSecretAgent : public NetworkManager::SecretAgent
{
    Q_OBJECT
public:
    explicit NetworkSecretAgent(QObject *parent = nullptr);

public Q_SLOTS:
    NMVariantMapMap GetSecrets(const NMVariantMapMap &, const QDBusObjectPath &, const QString &, const QStringList &hints, uint flags) override;
    void SaveSecrets(const NMVariantMapMap &connection, const QDBusObjectPath &connectionPath) override;
    void DeleteSecrets(const NMVariantMapMap &connection, const QDBusObjectPath &connectionPath) override;
    void CancelGetSecrets(const QDBusObjectPath &connectionPath, const QString &settingName) override;

    void onGetSecretsTimeout();
    void askPasswords(SecretsRequest &request, const QStringList &settingKeys, bool requestNew, uint secretFlag, NMStringMap props);
    // QLocalSocket相关
    void newConnectionHandler();
    void disconnectedHandler();
    void readyReadHandler();

    void requestSecrets(QLocalSocket *socket, const QByteArray &data);
    void secretsResult(QLocalSocket *socket, const QByteArray &data);

    inline QDBusConnection dbusConnection() const { return QDBusConnection::systemBus(); }

private Q_SLOTS:
    void vpnDialogFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void vpnDialogReadOutput();
    void vpnDialogReadError();
    void vpnDialogError(QProcess::ProcessError error);
    void vpnDialogStarted();
    void vpnDialogReadAllOutput(bool isEnd);

    void authDialogFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void authDialogReadOutput();
    void authDialogReadError();
    void authDialogError(QProcess::ProcessError error);
    void authDialogStarted();
    void authDialogReadAllOutput(bool isEnd);
    void doSecretsResult(QString callId, const QByteArray &data, bool isEnd);

private:
    void processNext();
    /**
     * @brief processGetSecrets requests
     * @param request the request we are processing
     * @param ignoreWallet true if the code should avoid Wallet
     * normally if it failed to open
     * @return true if the item was processed
     */
    bool processGetSecrets(SecretsRequest &request);
    bool processSaveSecrets(SecretsRequest &request) const;
    bool processDeleteSecrets(SecretsRequest &request) const;
    /**
     * @brief hasSecrets verifies if the desired connection has secrets to store
     * @param connection map with or without secrets
     * @return true if the connection has secrets, false otherwise
     */
    bool hasSecrets(const NMVariantMapMap &connection) const;
    QString getSecretFlagsKeyName(const QString &key) const;
    bool isMustAsk(const NMVariantMapMap &connection, const QString &settingName, const QString &secretKey) const;
    QStringList askProps(const NMVariantMapMap &connection, const QString &settingName) const;

    bool createPendingKey(SecretsRequest &request);
    QString getVpnAuthDialogBin(const NMVariantMapMap &connection);

private:
    QList<SecretsRequest> m_calls;
    QLocalServer *m_server;
    QList<QLocalSocket *> m_clients;
    QByteArray m_lastData;
    SecretService *m_secretService;
};
} // namespace sessionservice
} // namespace network
#endif // NETWORKSECRETAGENT_H
