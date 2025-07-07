// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
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
    static void doInit();

protected:
    explicit NetworkInitialization(QObject *parent = nullptr);
    ~NetworkInitialization() = default;
    void initDeviceInfo();
    void initConnection();

private:
    void addFirstConnection();
    void addFirstConnection(const QSharedPointer<NetworkManager::WiredDevice> &device);
    bool hasConnection(const QSharedPointer<NetworkManager::WiredDevice> &device, QList<QSharedPointer<NetworkManager::Connection>> &unSaveDevices);
    QString connectionMatchName() const;
    bool installUserTranslator(const QString &json);
    bool installTranslator(const QString &locale);
    bool installSystemTranslator();
    void hideWirelessDevice(const QSharedPointer<NetworkManager::Device> &device, bool disableNetwork);
    void initDeviceConnection(const QSharedPointer<NetworkManager::WiredDevice> &device);
    void checkAccountStatus();

private slots:
    void onUserChanged(const QString &json);
    void onUserAdded(const QString &json);
    void onInitDeviceConnection();
    void onAddFirstConnection();
    void onManagedChanged();

private:
    QStringList m_newConnectionNames;
    bool m_initilized;
    bool m_accountServiceRegister;
    bool m_hasAddFirstConnection;
    QMap<QString, QDateTime> m_lastCreateTime;
    QMap<QString, QSharedPointer<NetworkManager::Device>> m_devs;
};

}
}

#endif // SERVICE_H
