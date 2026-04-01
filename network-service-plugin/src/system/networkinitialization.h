// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef NETWORKINITIALIZATION_H
#define NETWORKINITIALIZATION_H

#include "constants.h"

#include <QObject>

namespace NetworkManager {
class WiredDevice;
class Device;
class Connection;
}

namespace network {
namespace systemservice {

class NetworkInitialization : public QObject
{
    Q_OBJECT

public:
    explicit NetworkInitialization(QObject *parent = nullptr);
    ~NetworkInitialization() = default;
    void updateLanguage(const QString &locale);

protected:
    void initDeviceInfo();
    void initConnection();

private:
    void addFirstConnection();
    void addFirstConnection(NetworkManager::WiredDevice *device);
    bool hasConnection(NetworkManager::WiredDevice *device, QList<QSharedPointer<NetworkManager::Connection>> &unSaveDevices);
    QPair<int, QString> connectionMatchName(NetworkManager::WiredDevice *device) const;
    QVariant accountInterface(const QString &path, const QString &key, bool isUser = true) const;
    bool installUserTranslator(const QString &json);
    void installLaunguage(const QString &locale);
    void hideWirelessDevice(const QSharedPointer<NetworkManager::Device> &device, bool disableNetwork);
    void initDeviceConnection(const QSharedPointer<NetworkManager::WiredDevice> &device);
    void checkAccountStatus();
    void updateConnectionLaunguage(const QString &account);
    void updateConnectionLaunguage();
    bool isServerSystem() const;

private slots:
    void onUserChanged(const QString &json);
    void onUserAdded(const QString &json);
    void onInitDeviceConnection();

    void onWiredDevicePropertyChanged();
    void onDeviceAdded(const QString &uni);
    void onAvailableConnectionDisappeared(const QString &connectionUni);

private:
    QMap<QString, QString> m_newConnectionNames;
    bool m_initilized;
    bool m_accountServiceRegister;
    bool m_hasAddFirstConnection;
    QMap<QString, QDateTime> m_lastCreateTime;
    QMap<QString, int> m_untranslactionConnections;
};

}
}

#endif // SERVICE_H
