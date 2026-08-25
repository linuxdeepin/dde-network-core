// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "secretservice.h"

#include "constants.h"

#include <QDBusMetaType>
#include <QDBusPendingReply>

namespace network {
namespace sessionservice {
const QString SecretsService = "org.freedesktop.secrets";
const QString SecretsPath = "/org/freedesktop/secrets";
const QString SecretsInterface = "org.freedesktop.Secret.Service";
const QString CollectionInterface = "org.freedesktop.Secret.Collection";

// keep keyring tags same with nm-applet
const QString KeyringTagConnUUID = "connection-uuid";
const QString KeyringTagSettingName = "setting-name";
const QString KeyringTagSettingKey = "setting-key";

struct SecretsData
{
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

QDBusArgument &operator<<(QDBusArgument &arg, const SecretsData &value)
{
    arg.beginStructure();
    arg << value.session << value.parameters << value.value << value.contentType;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SecretsData &value)
{
    arg.beginStructure();
    arg >> value.session >> value.parameters >> value.value >> value.contentType;
    arg.endStructure();
    return arg;
}

typedef QMap<QDBusObjectPath, SecretsData> SecretsDataMap;

SecretService::SecretService(QObject *parent)
    : QObject(parent)
    , m_secretService(nullptr)
    , m_defaultCollection(nullptr)
{
    qDBusRegisterMetaType<SecretsData>();
    qDBusRegisterMetaType<QMap<QDBusObjectPath, SecretsData>>();
    qDBusRegisterMetaType<QMap<QString, QDBusVariant>>();

    secretInit();
}

void SecretService::secretInit()
{
    m_secretService = new QDBusInterface(SecretsService, SecretsPath, SecretsInterface, QDBusConnection::sessionBus());
    QDBusPendingCall openReply = m_secretService->asyncCall("OpenSession", "plain", QVariant::fromValue(QDBusVariant("")));
    openReply.waitForFinished();
    if (openReply.isError()) {
        qCWarning(DSM()) << "" << openReply.error();
        return;
    }
    QList<QVariant> args = openReply.reply().arguments();
    if (args.size() != 2) {
        qCWarning(DSM()) << "secret init error" << openReply.error();
        return;
    }
    QDBusObjectPath result = args.at(1).value<QDBusObjectPath>();
    m_secretSessionPath = result.path();
    getDefaultCollection();
}

void SecretService::getDefaultCollection()
{
    getDefaultCollectionAux();
    if (m_defaultCollection) {
        return;
    }
    tryUnlockCollection(m_defaultCollection);
}

void SecretService::getDefaultCollectionAux()
{
    if (m_defaultCollection) {
        return;
    }
    QDBusPendingReply<QDBusObjectPath> collectionPathReply = m_secretService->asyncCall("ReadAlias", "default");
    collectionPathReply.waitForFinished();
    if (collectionPathReply.isError()) {
        qCDebug(DSM()) << "failed to get default collection path:" << collectionPathReply.error();
    }
    QString collectionPath = collectionPathReply.value().path();
    if (collectionPath == "/") {
        qCDebug(DSM()) << "failed to get default collection path";
    }
    m_defaultCollection = new QDBusInterface(SecretsService, collectionPath, CollectionInterface, QDBusConnection::sessionBus());
}

void SecretService::tryUnlockCollection(QDBusInterface *collection)
{
    // TODO:保证同时只有一个解锁对话框
    bool locked = collection->property("Locked").toBool();
    if (!locked) {
        // 未上锁，直接返回
        return;
    }
    QString collectionPath = collection->path();
    QDBusPendingCall reply = m_secretService->asyncCall("Unlock", QVariant::fromValue(QList<QDBusObjectPath>{ QDBusObjectPath(collectionPath) }));
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DSM()) << "failed to Unlock collection path:" << reply.error();
        return;
    }
    QList<QVariant> values = reply.reply().arguments();
    if (values.size() != 2) {
        qCWarning(DSM()) << "failed to Unlock collection path:" << values;
        return;
    }
    QList<QDBusObjectPath> unlocked = values.at(0).value<QList<QDBusObjectPath>>();
    QDBusObjectPath promptPath = values.at(1).value<QDBusObjectPath>();
    qCDebug(DSM()) << "call Unlock unlocked:" << unlocked << ", promptPath:" << promptPath;
    if (unlocked.contains(QDBusObjectPath(collectionPath))) {
        // 大概已经无密码自动解锁了
        return;
    }
    if (promptPath.path() == "/") {
        qCWarning(DSM()) << "invalid prompt path";
        return;
    }
    // TODO: 监听解锁完成
    QDBusInterface promptObj(SecretsService, promptPath.path(), "org.freedesktop.Secret.Prompt");
    QDBusConnection::sessionBus().connect(SecretsService, promptPath.path(), "org.freedesktop.Secret.Prompt", "Completed", this, SLOT(onCompleted(bool, QDBusVariant)));
    // 调用 Prompt 方法之后就会弹出密钥环解锁对话框
    QDBusMessage msg = QDBusMessage::createMethodCall(SecretsService, promptPath.path(), "org.freedesktop.Secret.Prompt", "Prompt");
    msg << QString();
    QDBusPendingReply<> promptReply = QDBusConnection::sessionBus().asyncCall(msg);
    promptReply.waitForFinished();
    if (promptReply.isError()) {
        qCWarning(DSM()) << "Prompt failed:" << promptReply.error();
    }
}

QMap<QString, QString> SecretService::getAll(const QString &uuid, const QString &settingName)
{
    getDefaultCollection();
    if (!m_defaultCollection) {
        qCWarning(DSM()) << "Unlock collection";
        return {};
    }
    QMap<QString, QString> attributes = { { KeyringTagConnUUID, uuid }, { KeyringTagSettingName, settingName } };
    QDBusPendingReply<QList<QDBusObjectPath>> reply = m_defaultCollection->asyncCall("SearchItems", QVariant::fromValue(attributes));
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DSM()) << "SearchItems failed:" << reply.error();
        return {};
    }
    QList<QDBusObjectPath> items = reply.value();
    QDBusPendingReply<SecretsDataMap> secretsDataReply = m_secretService->asyncCall("GetSecrets", QVariant::fromValue(items), QDBusObjectPath(m_secretSessionPath));
    if (secretsDataReply.isError()) {
        qCWarning(DSM()) << "GetSecrets error:" << secretsDataReply.error();
        return {};
    }
    QMap<QString, QString> result;
    SecretsDataMap secretsDataMap = secretsDataReply.value();
    for (auto it = secretsDataMap.begin(); it != secretsDataMap.end(); ++it) {
        QDBusMessage msg = QDBusMessage::createMethodCall(SecretsService, it.key().path(), "org.freedesktop.DBus.Properties", "Get");
        msg << "org.freedesktop.Secret.Item" << "Attributes";
        QDBusPendingReply<QDBusVariant> attributesReply = QDBusConnection::sessionBus().asyncCall(msg);
        attributesReply.waitForFinished();
        if (attributesReply.isError()) {
            qCWarning(DSM()) << "get Attributes" << it.key().path() << "failed:" << attributesReply.error();
            continue;
        }
        auto attributes = qdbus_cast<QMap<QString, QString>>(attributesReply.value().variant());
        QString settingKey = attributes.value(KeyringTagSettingKey);
        if (!settingKey.isEmpty()) {
            result.insert(settingKey, it.value().value);
        }
    }
    return result;
}

QString SecretService::deleteAll(const QString &uuid)
{
    getDefaultCollection();
    if (!m_defaultCollection) {
        qCWarning(DSM()) << "Unlock collection";
        return "Unlock collection";
    }
    QMap<QString, QString> attributes = { { KeyringTagConnUUID, uuid } };
    QDBusPendingReply<QList<QDBusObjectPath>> reply = m_defaultCollection->asyncCall("SearchItems", QVariant::fromValue(attributes));
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DSM()) << "SearchItems failed:" << reply.error();
        return "SearchItems failed:" + reply.error().message();
    }
    QList<QDBusObjectPath> items = reply.value();
    for (auto path : items) {
        QDBusMessage msg = QDBusMessage::createMethodCall(SecretsService, path.path(), "org.freedesktop.Secret.Item", "Delete");
        QDBusPendingReply<> deleteReply = QDBusConnection::sessionBus().asyncCall(msg);
        deleteReply.waitForFinished();
        if (deleteReply.isError()) {
            qCWarning(DSM()) << "delete item" << path.path() << "failed:" << reply.error();
            return "delete item " + path.path() + " failed:" + reply.error().message();
        }
    }
    return QString();
}

QString SecretService::deleteData(const QString &uuid, const QString &settingName, const QString &settingKey)
{
    getDefaultCollection();
    if (!m_defaultCollection) {
        qCWarning(DSM()) << "Unlock collection";
        return "Unlock collection";
    }
    QMap<QString, QString> attributes = { { KeyringTagConnUUID, uuid }, { KeyringTagSettingName, settingName }, { KeyringTagSettingKey, settingKey } };
    QDBusPendingReply<QList<QDBusObjectPath>> reply = m_defaultCollection->asyncCall("SearchItems", QVariant::fromValue(attributes));
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DSM()) << "SearchItems failed:" << reply.error();
        return "SearchItems failed: " + reply.error().message();
    }
    QList<QDBusObjectPath> items = reply.value();
    if (items.isEmpty()) {
        return QString();
    }
    qCDebug(DSM()) << "delete uuid:" << uuid << ", setting name:" << settingName << ", setting key:" << settingKey;
    QDBusObjectPath path = items.first();
    QDBusMessage msg = QDBusMessage::createMethodCall(SecretsService, path.path(), "org.freedesktop.Secret.Item", "Delete");
    QDBusPendingReply<> deleteReply = QDBusConnection::sessionBus().asyncCall(msg);
    deleteReply.waitForFinished();
    if (deleteReply.isError()) {
        qCWarning(DSM()) << "delete item" << path.path() << "failed:" << reply.error();
        return "delete item " + path.path() + " failed: " + reply.error().message();
    }
    return QString();
}

QString SecretService::setData(const QString &label, const QString &uuid, const QString &settingName, const QString &settingKey, const QString &value)
{
    qCDebug(DSM()) << "set label:" << label << ", uuid:" << uuid << ", setting name:" << settingName << ", setting key:" << settingKey << ", value:" << value;
    SecretsData itemSecret;
    itemSecret.session = QDBusObjectPath(m_secretSessionPath);
    itemSecret.value = value.toUtf8();
    itemSecret.contentType = "text/plain";

    QMap<QString, QString> attributes = { { KeyringTagConnUUID, uuid }, { KeyringTagSettingName, settingName }, { KeyringTagSettingKey, settingKey } };

    QMap<QString, QDBusVariant> properties;
    properties.insert("org.freedesktop.Secret.Item.Label", QDBusVariant(label));
    properties.insert("org.freedesktop.Secret.Item.Type", QDBusVariant("org.freedesktop.Secret.Generic"));
    properties.insert("org.freedesktop.Secret.Item.Attributes", QDBusVariant(QVariant::fromValue(attributes)));

    QDBusPendingCall reply = m_defaultCollection->asyncCall("CreateItem", QVariant::fromValue(properties), QVariant::fromValue(itemSecret), true);
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(DSM()) << "set item" << uuid << "failed:" << reply.error();
        return "set item " + uuid + " failed:" + reply.error().message();
    }
    return QString();
}

void SecretService::onCompleted(bool dismissed, QDBusVariant result)
{
    QDBusConnection::sessionBus().disconnect(SecretsService, "", "org.freedesktop.Secret.Prompt", "Completed", this, SLOT(onCompleted(bool, QDBusVariant)));
    // 用户取消解锁
    if (dismissed) {
        qCWarning(DSM()) << "prompt dismissed by user";
        return;
    }
    QList<QDBusObjectPath> paths = result.variant().value<QList<QDBusObjectPath>>();
    if (paths.isEmpty()) {
        qCWarning(DSM()) << "type of result.value() is not QList<QDBusObjectPath>";
        return;
    }
    for (auto path : paths) {
        if (path.path() == m_defaultCollection->path()) {
            // 正常解锁完成
            return;
        }
    }
    // 这里的情况不太可能发生
    qCWarning(DSM()) << "not found collection path in paths";
}
} // namespace sessionservice
} // namespace network
