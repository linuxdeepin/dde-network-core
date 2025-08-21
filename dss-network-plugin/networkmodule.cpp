// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkmodule.h"

#include "netmanager.h"
#include "netstatus.h"
#include "netview.h"
#include "notification/notificationmanager.h"

// #include <com_deepin_daemon_accounts_user.h>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusServiceWatcher>
#include <QTime>
#include <QWidget>

#define NETWORK_KEY "network-item-key"

static QString networkService = "org.deepin.dde.Network1";
static QString networkPath = "/org/deepin/service/SystemNetwork";
static QString networkInterface = "org.deepin.service.SystemNetwork";

const int CONTENT_SPACING = 10;

static Q_LOGGING_CATEGORY(DNC, "org.deepin.dde.session-shell.network");

using namespace dde::network;

namespace dde {
namespace network {

NetworkModule::NetworkModule(QObject *parent)
    : QObject(parent)
    , m_replacesId(0)
    , m_contentWidget(new QWidget)
{
    m_isLockModel = QCoreApplication::applicationName().contains("lock", Qt::CaseInsensitive);

    m_contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    QVBoxLayout *mainLayout = new QVBoxLayout(m_contentWidget);
    mainLayout->setContentsMargins(0, CONTENT_SPACING, 0, CONTENT_SPACING);
    mainLayout->setSpacing(0);
    NetType::NetManagerFlags flags = m_isLockModel ? NetType::Net_LockFlags : NetType::Net_GreeterFlags;
    m_manager = new NetManager(flags, this);
    if (m_isLockModel) {
        m_manager->setServerKey("lock");
        // m_manager->setServiceLoadForNM(false);
        // m_manager->setMonitorNetworkNotify(false);
        QDBusConnection::sessionBus().connect("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront", "com.deepin.dde.lockFront", "Visible", this, SLOT(updateLockScreenStatus(bool)));
        connect(m_manager,
                &NetManager::networkNotify,
                this,
                [this](const QString &inAppName, int replacesId, const QString &appIcon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout) {
                    QDBusMessage notify = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");
                    uint id = replacesId == -1 ? m_replacesId : static_cast<uint>(replacesId);
                    notify << inAppName << id << appIcon << summary << body << actions << hints << expireTimeout;
                    QDBusConnection::sessionBus().callWithCallback(notify, this, SLOT(onNotify(uint)));
                });
    } else {
        QDBusMessage lock = QDBusMessage::createMethodCall("com.deepin.dde.LockService", "/com/deepin/dde/LockService", "com.deepin.dde.LockService", "CurrentUser");
        QDBusConnection::systemBus().callWithCallback(lock, this, SLOT(onUserChanged(QString)));
        QDBusConnection::systemBus().connect("com.deepin.dde.LockService", "/com/deepin/dde/LockService", "com.deepin.dde.LockService", "UserChanged", this, SLOT(onUserChanged(QString)));

        connect(m_manager, &NetManager::networkNotify, this, [](const QString &inAppName, int replacesId, const QString &appIcon, const QString &summary, const QString &body, const QStringList &actions, const QVariantMap &hints, int expireTimeout) {
            NotificationManager::Notify(appIcon, body);
        });
    }
    installTranslator(QLocale().name());

    m_netView = new NetView(m_manager);
    QPalette pa = m_netView->palette();
    pa.setColor(QPalette::Normal, QPalette::Button, QColor("#BBBBBBBB"));
    pa.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#B0B0B0"));
    pa.setColor(QPalette::Disabled, QPalette::Button, QColor("#E0E0E0"));
    pa.setColor(QPalette::Normal, QPalette::BrightText, Qt::white);
    m_netView->setPalette(pa);
    m_netStatus = new NetStatus(m_manager);
    mainLayout->addWidget(m_netView);
    connect(m_netView, &NetView::requestShow, this, &NetworkModule::requestShow);
    connect(m_netView, &NetView::updateSize, this, [this] {
        m_contentWidget->resize(m_netView->width(), m_netView->height() + CONTENT_SPACING * 2);
    });
}

NetworkModule::~NetworkModule()
{
    if (m_contentWidget)
        m_contentWidget->deleteLater();

    if (m_netView)
        m_netView->deleteLater();

    if (m_netStatus)
        delete m_netStatus;

    if (m_manager)
        delete m_manager;
}

QWidget *NetworkModule::content()
{
    return m_contentWidget;
}

QWidget *NetworkModule::itemWidget()
{
    QWidget *iconWidget = m_netStatus->createIconWidget();
    QPalette p = iconWidget->palette();
    p.setColor(QPalette::BrightText, QColor(255, 255, 255, 178)); // 178 = 255*0.7;
    iconWidget->setPalette(p);
    iconWidget->setFixedSize(26, 26);
    NotificationManager::InstallEventFilter(iconWidget);
    return iconWidget;
}

QWidget *NetworkModule::itemTipsWidget() const
{
    QWidget *itemTips = m_netStatus->createItemTips();
    QPalette p = itemTips->palette();
    p.setColor(QPalette::BrightText, Qt::black);
    itemTips->setPalette(p);
    return itemTips;
}

const QString NetworkModule::itemContextMenu() const
{
    return m_netStatus->contextMenu(false);
}

void NetworkModule::invokedMenuItem(const QString &menuId, const bool checked) const
{
    Q_UNUSED(checked);
    m_netStatus->invokeMenuItem(menuId);
}

void NetworkModule::updateLockScreenStatus(bool visible)
{
    m_isLockModel = true;
    m_isLockScreen = visible;
    m_manager->setEnabled(m_isLockScreen);
}

void NetworkModule::onUserChanged(const QString &json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject())
        return;
    int uid = doc.object().value("Uid").toInt();
    QDBusInterface user("org.deepin.dde.Accounts1", QString("/org/deepin/dde/Accounts1/User%1").arg(uid), "org.deepin.dde.Accounts1.User", QDBusConnection::systemBus());
    installTranslator(user.property("Locale").toString().split(".").first());
    Q_EMIT userChanged();
}

void NetworkModule::onNotify(uint replacesId)
{
    m_replacesId = replacesId;
}

void NetworkModule::installTranslator(const QString &locale)
{
    static QTranslator translator;
    static QString localTmp;
    if (locale.isEmpty() || localTmp == locale) {
        return;
    }
    localTmp = locale;
    QApplication::removeTranslator(&translator);
    if (translator.load(QLocale(locale), "dss-network-plugin", "_", "/usr/share/dss-network-plugin/translations")) {
        QApplication::installTranslator(&translator);
    }
    m_manager->updateLanguage(localTmp);
}

NetworkPlugin::NetworkPlugin(QObject *parent)
    : QObject(parent)
    , m_network(nullptr)
    , m_messageCallback(nullptr)
    , m_appData(nullptr)
{
    setObjectName(QStringLiteral(NETWORK_KEY));
}

void NetworkPlugin::init()
{
    initUI();
    onDevicesChanged();
    qCInfo(DNC) << "Connect org.freedesktop.login1.Manager, prepare for sleep reply: "
                << QDBusConnection::systemBus().connect("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "PrepareForSleep", this, SLOT(onPrepareForSleep(bool)));
    connect(m_network, &NetworkModule::requestShow, this, &NetworkPlugin::requestShowContent);
    connect(m_network, &NetworkModule::userChanged, this, &NetworkPlugin::onDevicesChanged);
    connect(m_network->netStatus(), &NetStatus::hasDeviceChanged, this, &NetworkPlugin::onDevicesChanged);
}

QWidget *NetworkPlugin::content()
{
    return m_network->content();
}

QWidget *NetworkPlugin::itemWidget() const
{
    return m_network->itemWidget();
}

QWidget *NetworkPlugin::itemTipsWidget() const
{
    return m_network->itemTipsWidget();
}

const QString NetworkPlugin::itemContextMenu() const
{
    return m_network->itemContextMenu();
}

void NetworkPlugin::invokedMenuItem(const QString &menuId, const bool checked) const
{
    m_network->invokedMenuItem(menuId, checked);
}

void NetworkPlugin::setMessageCallback(dss::module::MessageCallbackFunc messageCallback)
{
    m_messageCallback = messageCallback;
    // init时greeter和dde-lock还未启动无法收到回调函数消息，需要在加载插件设置回调函数时再获取一次数据
    onDevicesChanged();
}

void NetworkPlugin::initUI()
{
    if (m_network) {
        return;
    }

    m_network = new NetworkModule(this);
}

void NetworkPlugin::onDevicesChanged()
{
    bool visible = m_network->netStatus()->hasDevice();
    Q_EMIT notifyNetworkStateChanged(visible);
    setMessage(visible);
}

void NetworkPlugin::onPrepareForSleep(bool sleep)
{
    qCInfo(DNC) << "On prepare for sleep, whether sleep: " << sleep;
    if (!sleep) {
        onDevicesChanged();
    }
}

void NetworkPlugin::requestShowContent()
{
    if (!m_messageCallback || !m_appData) {
        qCWarning(DNC) << "Request show content, callback func or appdata is null, callback: " << m_messageCallback << ", app data: " << m_appData;
        return;
    }

    QJsonObject obj;
    obj["Cmd"] = "ShowContent";
    obj["PluginKey"] = NETWORK_KEY;
    QJsonDocument doc;
    doc.setObject(obj);
    m_messageCallback(doc.toJson(), m_appData);
    qCInfo(DNC) << "Request show content, message: " << doc.toJson();
}

void NetworkPlugin::setMessage(bool visible)
{
    if (!m_messageCallback || !m_appData) {
        qCWarning(DNC) << "Set message, callback func or appdata is null, callback: " << m_messageCallback << "app data: " << m_appData;
        return;
    }

    QJsonObject obj;
    obj["Cmd"] = "PluginVisible";
    obj["PluginKey"] = NETWORK_KEY;
    obj["Visible"] = visible;
    QJsonDocument doc;
    doc.setObject(obj);
    m_messageCallback(doc.toJson(), m_appData);
    qCInfo(DNC) << "Set message:" << doc.toJson();
}

QString NetworkPlugin::message(const QString &msgData)
{
    qDebug() << "message" << msgData;
    QJsonDocument json = QJsonDocument::fromJson(msgData.toLatin1());
    QJsonObject jsonObject = json.object();
    if (!jsonObject.contains("data")) {
        qWarning() << "msgData don't containt data" << msgData;
        QJsonDocument jsonResult;
        QJsonObject resultObject;
        resultObject.insert("data", QString("msgData don't containt data %1").arg(msgData));
        jsonResult.setObject(resultObject);
        return jsonResult.toJson();
    }
    QJsonObject dataObject = jsonObject.value("data").toObject();
    QString locale = dataObject.value("locale").toString();
    qDebug() << "read locale" << locale;
    m_network->installTranslator(locale);
    // 同时更新网络服务的语言
    if (QDBusConnection::systemBus().interface()->isServiceRegistered(networkService)) {
        qDebug() << "update SystemNetworm Language" << locale;
        QDBusInterface dbusInter(networkService, networkPath, networkInterface, QDBusConnection::systemBus());
        QDBusPendingCall reply = dbusInter.asyncCall("UpdateLanguage", locale);
        reply.waitForFinished();
    } else {
        qWarning() << networkService << "don't start, wait for it start";
        QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
        serviceWatcher->setConnection(QDBusConnection::systemBus());
        serviceWatcher->addWatchedService(networkService);
        connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [locale](const QString &service) {
            if (service == networkService) {
                QDBusInterface dbusInter(networkService, networkPath, networkInterface, QDBusConnection::systemBus());
                QDBusPendingCall reply = dbusInter.asyncCall("UpdateLanguage", locale);
                reply.waitForFinished();
            }
        });
    }

    QJsonDocument jsonResult;
    QJsonObject resultObject;
    resultObject.insert("data", "success");
    jsonResult.setObject(resultObject);
    return jsonResult.toJson();
}

} // namespace network
} // namespace dde
