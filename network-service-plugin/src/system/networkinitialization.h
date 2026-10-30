// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef NETWORKINITIALIZATION_H
#define NETWORKINITIALIZATION_H

#include "constants.h"

#include <QDBusError>
#include <QDBusMessage>
#include <QMetaObject>
#include <QObject>
#include <QSet>
#include <QTranslator>

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

public Q_SLOTS:
    // NM 访问入口：构造点排在 NM 线程上执行
    void initDeviceInfo();
    void initConnection();

private:
    void addFirstConnection();
    void addFirstConnection(NetworkManager::WiredDevice *device);
    bool hasConnection(NetworkManager::WiredDevice *device, QList<QSharedPointer<NetworkManager::Connection>> &unSaveDevices);
    QPair<int, QString> connectionMatchName(NetworkManager::WiredDevice *device) const;
    QVariant accountInterface(const QString &path, const QString &key, bool isUser = true) const;
    bool installUserTranslator(const QString &json);
    bool installLanguage(const QString &locale);
    bool installSystemTranslator();
    void ensureTranslatorInstalled();
    void hideWirelessDevice(const QSharedPointer<NetworkManager::Device> &device, bool disableNetwork);
    void initDeviceConnection(const QSharedPointer<NetworkManager::WiredDevice> &device);
    void setDeviceManaged(const QString &deviceUni, bool managed);
    void checkAccountStatus();
    void updateConnectionLanguage(const QString &account);
    void updateConnectionLanguage();
    bool isServerSystem() const;

private Q_SLOTS:
    void onSetDeviceManagedFinished(const QDBusMessage &reply);
    void onSetDeviceManagedError(const QDBusError &error, const QDBusMessage &message);
    void onUserChanged(const QString &json);
    void onUserAdded(const QString &json);
    void onInitDeviceConnection();

    void onWiredDevicePropertyChanged();
    void onDeviceAdded(const QString &uni);
    void onAvailableConnectionDisappeared(const QString &connectionUni);

private:
    QMap<QString, QString> m_newConnectionNames;
    bool m_initialized;
    bool m_accountServiceRegister;
    bool m_hasAddFirstConnection;
    QMap<QString, QDateTime> m_lastCreateTime;
    QMap<QString, int> m_untranslactionConnections;
    QSet<QString> m_seenDevices;
    // managedChanged 的 lambda 无法用 Qt::UniqueConnection，
    // 用集合防止 ConfigManager 重启后重复挂接
    QSet<QString> m_managedHooked;
    // 必须是本对象的子对象：构造时 parent 设好，moveToThread() 时随父一起迁移，
    // 之后在 NM 线程安装、在 NM 线程析构，不出现跨线程 QObject。
    // （值成员不带 parent，不会被 moveToThread 迁移。）
    QTranslator *m_translator;
    // devicesChanged 只连一次：onInitDeviceConnection 可能因 ConfigManager 重启再次进入
    QMetaObject::Connection m_devicesChangedConnection;
};

}
}

#endif // SERVICE_H