// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETSECRETAGENTFORUI_H
#define NETSECRETAGENTFORUI_H

#include "netsecretagentinterface.h"

#include <QLocalSocket>
#include <QObject>

class QWidget;
class QLocalServer;
class QLocalSocket;
class QTimer;

namespace dde {
namespace network {

class NetSecretAgentForUI : public QObject, public NetSecretAgentInterface
{
    Q_OBJECT
public:
    explicit NetSecretAgentForUI(PasswordCallbackFunc fun, const QString &serverKey, QObject *parent = Q_NULLPTR);
    ~NetSecretAgentForUI() override;

    bool hasTask() override;
    void inputPassword(const QString &key, const QVariantMap &password, bool input) override;

Q_SIGNALS:
    void requestShow();

private Q_SLOTS:
    void ConnectToServer();
    void onStateChanged(QLocalSocket::LocalSocketState socketState);

    void readyReadHandler();
    void sendSecretsResult(const QString &key, const QVariantMap &password, bool input);

public:
    void requestSecrets(QLocalSocket *socket, const QByteArray &data);

private:
    QString m_callId;
    QString m_connectDev;
    QString m_connectSsid;
    QStringList m_secrets;
    QByteArray m_lastData;

    QLocalSocket *m_client;

    QTimer *m_reconnectTimer;
    QString m_serverName;
};

} // namespace network
} // namespace dde
#endif // NETSECRETAGENTFORUI_H
