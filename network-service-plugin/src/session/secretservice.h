// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SECRETSERVICE_H
#define SECRETSERVICE_H

#include <QDBusInterface>
#include <QObject>

namespace network {
namespace sessionservice {
// TODO: 该类DBus操作为阻塞的，考虑加线程
class SecretService : public QObject
{
    Q_OBJECT
public:
    explicit SecretService(QObject *parent = nullptr);

public Q_SLOTS:
    void secretInit();
    void getDefaultCollection();                          // 获取默认密钥环，并且尝试解锁它。
    void getDefaultCollectionAux();                       // 获取默认密钥环
    void tryUnlockCollection(QDBusInterface *collection); // 解锁密钥环
    QMap<QString, QString> getAll(const QString &uuid, const QString &settingName);
    QString deleteAll(const QString &uuid);
    QString deleteData(const QString &uuid, const QString &settingName, const QString &settingKey);
    QString setData(const QString &label, const QString &uuid, const QString &settingName, const QString &settingKey, const QString &value);

private Q_SLOTS:
    void onCompleted(bool dismissed, QDBusVariant result);

private:
    QString m_secretSessionPath;
    QDBusInterface *m_secretService;
    QDBusInterface *m_defaultCollection;
};
} // namespace sessionservice
} // namespace network
#endif // SECRETSERVICE_H
