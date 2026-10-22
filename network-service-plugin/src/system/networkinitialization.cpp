// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "networkinitialization.h"

#include "nmaccess.h"

#include "settingconfig.h"
#include "systemservice.h"

#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/WiredDevice>
#include <NetworkManagerQt/WiredSetting>
#include <NetworkManagerQt/WirelessDevice>

#include <DSysInfo>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMutex>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>

#include <iterator>

using namespace network::systemservice;

// the interface is enabled from the administrative point of view. Corresponds to kernel IFF_UP.
#define DEVICE_INTERFACE_FLAG_UP 0x1

#define NETWORKMANAGER_SERVICE "org.freedesktop.NetworkManager"

// 这些查询运行在 NM 线程上，超时必须远低于 D-Bus 默认的 25 秒
static const int kAccountQueryTimeoutMs = 3000;

#define LOCKSERVICE "org.deepin.dde.LockService1"
#define LOCKPATH "/org/deepin/dde/LockService1"
#define LOCKINTERFACE "org.deepin.dde.LockService1"

#define DAEMONACCOUNTSERVICE "org.deepin.dde.Accounts1"
#define DAEMONACCOUNTPATH "/org/deepin/dde/Accounts1"
#define DAEMONACCOUNTINTERFACE "org.deepin.dde.Accounts1"

NetworkInitialization::NetworkInitialization(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_accountServiceRegister(false)
    , m_hasAddFirstConnection(false)
    , m_translator(new QTranslator(this))
{
    // 本对象直接调用 NetworkManagerQt，必须与 NM Qt 单例同线程。
    // 构造点在插件线程，因此初始化一律排队到 NM 线程执行。
    moveToThread(networkManagerThread());
    QMetaObject::invokeMethod(this, &NetworkInitialization::initDeviceInfo, Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, &NetworkInitialization::initConnection, Qt::QueuedConnection);
}

void NetworkInitialization::updateLanguage(const QString &locale)
{
    qWarning(DSM) << "update local language" << locale;
    installLanguage(locale);
    m_initialized = true;
    updateConnectionLanguage();
}

void NetworkInitialization::initDeviceInfo()
{
    // 处理无线网络
    if (QDBusConnection::systemBus().interface()->isServiceRegistered("org.desktopspec.ConfigManager")) {
        qCDebug(DSM) << "ConfigManager is start";
        onInitDeviceConnection();
    } else {
        qCWarning(DSM) << "ConfigManager is not start, wait for it start";
        QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
        serviceWatcher->setConnection(QDBusConnection::systemBus());
        serviceWatcher->addWatchedService("org.desktopspec.ConfigManager");
        connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &NetworkInitialization::onInitDeviceConnection);
    }
}

void NetworkInitialization::initConnection()
{
    QDBusMessage lock = QDBusMessage::createMethodCall(LOCKSERVICE, LOCKPATH, LOCKINTERFACE, "CurrentUser");
    QDBusConnection::systemBus().callWithCallback(lock, this, SLOT(onUserChanged(QString)));
    QDBusConnection::systemBus().connect(LOCKSERVICE, LOCKPATH, LOCKINTERFACE, "UserChanged", this, SLOT(onUserChanged(QString)));
    QDBusConnection::systemBus().connect(DAEMONACCOUNTSERVICE, DAEMONACCOUNTPATH, DAEMONACCOUNTINTERFACE, "UserAdded", this, SLOT(onUserAdded(QString)));

    m_accountServiceRegister = QDBusConnection::systemBus().interface()->isServiceRegistered(DAEMONACCOUNTINTERFACE);
    if (!m_accountServiceRegister) {
        // 如果服务未启动，则等待服务启动
        QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
        serviceWatcher->setConnection(QDBusConnection::systemBus());
        qCWarning(DSM) << m_accountServiceRegister << "service is not register";
        serviceWatcher->addWatchedService(DAEMONACCOUNTINTERFACE);
        connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [ this ](const QString &service) {
            if (service != DAEMONACCOUNTSERVICE)
                return;

            qCDebug(DSM) << service << "register, initilized" << m_initialized;
            m_accountServiceRegister = true;

            // 如果需要初始化设备，则初始化设备
            checkAccountStatus();
            if (m_initialized) {
                addFirstConnection();
            }
            updateConnectionLanguage();
        });
    }
    // 等待3秒过后再执行一次创建的动作，正常情况下，在500毫秒内系统肯定会发送当前账户变化的信号，
    // 但是在有的系统上没有当前账户的信息，一直不发送当前账户变化的信息，这种情况下就自己强制创建一个连接，默认就英文吧
    QTimer::singleShot(3000, this, [this] {
        // 3秒过后直接调用addFirstConnection函数，在这个函数中，如果连接已经创建了，就不再创建
        // 如果没有创建过连接，那么就执行创建的操作
        // 调用checkAccountStatus检查当前用户状态并安装当前用户的语言
        ensureTranslatorInstalled();
        addFirstConnection();
    });
}

void NetworkInitialization::addFirstConnection()
{
    NMQT_ASSERT_THREAD();
    qCDebug(DSM) << "add wired device connection: has add" << m_hasAddFirstConnection;

    if (m_hasAddFirstConnection) {
        return;
    }
    m_hasAddFirstConnection = true;
    NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    for (NetworkManager::Device::Ptr device : devices) {
        if (device->type() != NetworkManager::Device::Type::Ethernet)
            continue;

        qCDebug(DSM) << "device" << device->interfaceName() << "add first connection";
        NetworkManager::WiredDevice::Ptr wiredDevice = device.staticCast<NetworkManager::WiredDevice>();
        initDeviceConnection(wiredDevice);
        addFirstConnection(wiredDevice.data());
    }
}

void NetworkInitialization::addFirstConnection(NetworkManager::WiredDevice *device)
{
    NMQT_ASSERT_THREAD();
    if (!device)
        return;
    // 还没安装语言场景，信号变化触发创建连接。回退读取系统配置文件获取语言
    ensureTranslatorInstalled();
    // 如果设备不被管理，而且被down掉，或者没有插入网线的情况下，是无需创建连接的
    qCDebug(DSM) << "device:" << device->interfaceName() << "managed:" << device->managed()
             << "interfaceFlags:" << device->interfaceFlags() << "carrier:" << device->carrier();
    if (!device->managed() || !(device->interfaceFlags() & DEVICE_INTERFACE_FLAG_UP)
            || !device->carrier()) {
        // clear the unsave connection
        NetworkManager::Connection::List unSaveConnections;
        hasConnection(device, unSaveConnections);
        // 按照需求，需要将未保存的连接删除
        for (const NetworkManager::Connection::Ptr &conn : unSaveConnections)
            conn->remove();
        return;
    }

    static QMutex lock;
    QMutexLocker locker(&lock);

    // 只捕获设备路径：定时器最多延迟 5 秒，期间设备可能已被销毁
    //（NM 重启会清空 Device 表），裸指针会悬垂。回调里重新解析成 Ptr 保活。
    const QString deviceUni = device->uni();
    auto createConnection = [this, deviceUni] {
        const NetworkManager::WiredDevice::Ptr device =
                NetworkManager::findNetworkInterface(deviceUni).staticCast<NetworkManager::WiredDevice>();
        if (device.isNull()) {
            qCWarning(DSM()) << "device gone before creating connection:" << deviceUni;
            return;
        }

        // 先查找当前的设备下是否存在有线连接，如果不存在，则直接新建一个，因为按照要求是至少要有一个有线连接
        NetworkManager::Connection::List unSaveConnections;
        bool findConnection = hasConnection(device.data(), unSaveConnections);
        // 按照需求，需要将未保存的连接删除
        for (const NetworkManager::Connection::Ptr &conn : unSaveConnections)
            conn->remove();

        qCDebug(DSM()) << "find connection :" << findConnection << "current device:" << device->uni();

        if (findConnection)
            return;

        // 记录当前设备创建连接的时间，在下一次为该设备创建连接的时候，会等待5秒，否则会出现创建的连接的
        m_lastCreateTime[device->interfaceName()] = QDateTime::currentDateTime();
        // 如果发现当前的连接的数量为空,则自动创建以当前语言为基础的连接
        QPair<int, QString> matchName = connectionMatchName(device.data());
        qCDebug(DSM()) << "device:" << device->interfaceName() << "start create first connection" << matchName.second;
        NetworkManager::ConnectionSettings::Ptr conn(new NetworkManager::ConnectionSettings(NetworkManager::ConnectionSettings::ConnectionType::Wired));
        conn->setId(matchName.second);
        conn->setUuid(conn->createNewUuid());
        conn->setInterfaceName(device->interfaceName());
        conn->setAutoconnect(!SettingConfig::instance()->enableAccountNetwork());
        NetworkManager::WiredSetting::Ptr wiredSetting = conn->setting(NetworkManager::Setting::Wired).staticCast<NetworkManager::WiredSetting>();
        QString macAddress = device->permanentHardwareAddress();
        macAddress.remove(":");
        wiredSetting->setMacAddress(QByteArray::fromHex(macAddress.toUtf8()));
        wiredSetting->setInitialized(true);
        QDBusPendingReply<QDBusObjectPath> reply = NetworkManager::addConnection(conn->toMap());
        reply.waitForFinished();
        qDebug() << "device" << device->interfaceName() << "create connection success,count:" << device->availableConnections().size();
        if (!m_initialized && matchName.first >= 0) {
            qCDebug(DSM) << "can't found user, add " << matchName.second << conn->uuid() << " to cache";
            m_untranslactionConnections[conn->uuid()] = matchName.first;
        }
        m_newConnectionNames[reply.value().path()] = matchName.second;
    };
    if (m_lastCreateTime.contains(device->interfaceName())) {
        QDateTime lastCreateTime = m_lastCreateTime.value(device->interfaceName());
        qint64 passmseconds = lastCreateTime.msecsTo(QDateTime::currentDateTime());
        qCDebug(DSM) << "last create connection time" << lastCreateTime << ", pass time" << passmseconds << "millisecond";
        // 获取上一次为该设备创建的时间，如果是5秒钟之前，则直接调用创建的流程，否则，在上一次创建连接后等待5秒再进行创建
        // 这样做的目的是会出现这个连接同时间会多次进入这个函数进行创建，上一次的创建动作刚完成，此时设备上还没有新创建的连接
        // 但是就进行了下一次的创建，下一次的创建中此时无法检测上一次创建的连接，会导致重复创建连接，因此，
        // 此处等待至少5秒后再进行第二次连接的创建，这样有足够的时间保证了上一次创建的连接当前设备可以检测到
        if (passmseconds >= 5000) {
            createConnection();
        } else {
            QTimer::singleShot(5000 - passmseconds, this, createConnection);
        }
    } else {
        createConnection();
    }
}

bool NetworkInitialization::hasConnection(NetworkManager::WiredDevice *device, QList<QSharedPointer<NetworkManager::Connection> > &unSaveDevices)
{
    if (!device)
        return false;

    qCDebug(DSM) << "device permanent mac" << device->permanentHardwareAddress() << "mac" << device->hardwareAddress();
    bool hasConnection = false;
    QStringList uuids;
    NetworkManager::Connection::List connList = NetworkManager::listConnections();
    for (const NetworkManager::Connection::Ptr &conn : connList) {
        uuids << conn->uuid();
    }

    for (const NetworkManager::Connection::Ptr &conn : device->availableConnections()) {
        if (uuids.contains(conn->uuid()))
            continue;

        connList << conn;
    }

    for (const NetworkManager::Connection::Ptr &conn : connList) {
        // 如果UUID为空，则认为是空连接，不是本网卡的连接
        if (conn->settings()->uuid().isEmpty())
            continue;

        // 如果网卡名称存在且不为空，那么则判断配置中的名称是不是网卡的名称
        QString interfaceName = conn->settings()->interfaceName();
        if (!interfaceName.isEmpty() && interfaceName != device->interfaceName())
            continue;

        NetworkManager::WiredSetting::Ptr settings = conn->settings()->setting(NetworkManager::Setting::Wired).staticCast<NetworkManager::WiredSetting>();
        // 如果当前连接的MAC地址不为空且连接的MAC地址不等于当前设备的MAC地址，则认为不是当前的连接，跳过
        QString deviceMac = device->permanentHardwareAddress().isEmpty() ? device->hardwareAddress() : device->permanentHardwareAddress();
        if (settings.isNull() || (!settings->macAddress().isEmpty() && deviceMac.compare(settings->macAddress().toHex(':'), Qt::CaseInsensitive) != 0))
            continue;

        // 将未保存的连接放入到列表中，供外面调用删除
        if (conn->isUnsaved()) {
            unSaveDevices << conn;
            continue;
        }

        qCDebug(DSM) << "find device uuid" << conn->settings()->uuid() << "interface name" << interfaceName << settings->macAddress().toHex(':');
        // 只要走到这里，就认为当前网卡存在有线连接，就不再继续
        hasConnection = true;
    }

    return hasConnection;
}

QPair<int, QString> NetworkInitialization::connectionMatchName(NetworkManager::WiredDevice *device) const
{
    if (isServerSystem()) {
        return qMakePair(-1, device->interfaceName());
    }
    NetworkManager::Connection::List connList = NetworkManager::listConnections();
    QStringList connNameList = m_newConnectionNames.values();

    for (const NetworkManager::Connection::Ptr &conn : connList) {
        if (conn->settings()->connectionType() == NetworkManager::ConnectionSettings::ConnectionType::Wired
                && !connNameList.contains(conn->name()))
            connNameList.append(conn->name());
    }

    QString matchConnName = tr("Wired Connection");
    if (!connNameList.contains(matchConnName))
        return qMakePair(0, matchConnName);

    int connSuffixNum = 1;
    matchConnName = QString(tr("Wired Connection")) + QString(" %1");
    for (int i = 1; i <= connNameList.size(); ++i) {
        if (!connNameList.contains(matchConnName.arg(i))) {
            connSuffixNum = i;
            break;
        }
        if (i == connNameList.size())
            connSuffixNum = i + 1;
    }

    return qMakePair(connSuffixNum, matchConnName.arg(connSuffixNum));
}

QVariant NetworkInitialization::accountInterface(const QString &path, const QString &key, bool isUser) const
{
    const QString interfaceName = isUser ? QStringLiteral("org.deepin.dde.Accounts1.User")
                                         : QStringLiteral("org.deepin.dde.Accounts1");
    // 本函数运行在 NM 线程，任何阻塞都会停摆 NM 的事件循环（设备状态随之丢失）。
    // QDBusInterface 的构造会先做一次 Introspect、property() 再走一次 Get，
    // 这里直接发一次 Properties.Get，并把等待上限压到 3 秒（默认是 25 秒）。
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.deepin.dde.Accounts1"),
                                                         path,
                                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                                         QStringLiteral("Get"));
    message << interfaceName << key;
    const QDBusMessage reply = QDBusConnection::systemBus().call(message, QDBus::Block, kAccountQueryTimeoutMs);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        qCDebug(DSM()) << "query account property failed" << path << key << reply.errorMessage();
        return QVariant();
    }
    return reply.arguments().first().value<QDBusVariant>().variant();
}

bool NetworkInitialization::installUserTranslator(const QString &json)
{
    if (m_initialized) {
        qCDebug(DSM) << "environment is initialized";
        return true;
    }

    qCDebug(DSM) << "user changed " << json;
    QString locale;
    if (json.startsWith("/")) {
        QString localeValue = accountInterface(json, "Locale").toString();
        locale = localeValue.split(".").first().trimmed();
        qCDebug(DSM) << "get locale: " << locale;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (!locale.isEmpty() ) {
        //do nothing
    } else if (error.error == QJsonParseError::NoError && doc.isObject()) {
        int uid = doc.object().value("Uid").toInt();
        QVariant localeVariant = accountInterface(QString("%1/User%2").arg(DAEMONACCOUNTPATH).arg(uid), "Locale");
        locale = localeVariant.toString().split(".").first();
    } else if (m_accountServiceRegister) {
        // 如果是非法的json，就直接从Accounts服务中获取
        QVariant userListVariant = accountInterface(DAEMONACCOUNTPATH, "UserList", false);
        const QStringList userList = userListVariant.toStringList();
        qCDebug(DSM) << "found users" << userList;
        if (userList.isEmpty())
            return false;

        QVariant localeVariant = accountInterface(userList.first(), "Locale");
        locale = localeVariant.toString().split(".").first();
    } else {
        return false;
    }

    qCDebug(DSM) << "account locale" << locale;
    if (locale.isEmpty()) {
        return false;
    }

    static QString localTmp;

    if (localTmp != locale) {
        localTmp = locale;
        installLanguage(locale);
    }

    return true;
}

bool NetworkInitialization::installLanguage(const QString &locale)
{
    QCoreApplication::removeTranslator(m_translator);
    const QString qmFile = QString("%1/network-service-plugin_%2.qm").arg(QM_FILES_DIR).arg(locale);
    if (m_translator->load(qmFile)) {
        QCoreApplication::installTranslator(m_translator);
        qCDebug(DSM) << "install translation file" << qmFile;
        return true;
    }
    return false;
}

static QString getLocaleValue(const QString &filePath, const QStringList &keys, const QString &splitKey = "=", const QString &keywords = QString())
{
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qCWarning(DSM) << "Failed to open locale file:" << filePath << "-" << file.errorString();
        return QString();
    }

    QMap<QString, QString> localeMap;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!keywords.isEmpty() && !line.contains(keywords))
            continue;
        QStringList pair = line.split(splitKey, Qt::SkipEmptyParts);
        if (pair.size() == 2)
            localeMap.insert(pair.at(0).trimmed(), pair.at(1).trimmed());
    }
    for (const auto &key : keys) {
        if (localeMap.contains(key))
            return localeMap.value(key).section('.', 0, 0);
    }
    return QString();
}

bool NetworkInitialization::installSystemTranslator()
{
    QString locale = getLocaleValue("/etc/locale.conf", { "LANGUAGE", "LANG" }, "=", "LANG");
    if (locale.isEmpty())
        locale = getLocaleValue("/etc/deepin-installer/deepin-installer.conf", { "DI_LOCALE", "LIVE_LOCALES" }, "=", "LOCALE");
    if (!locale.isEmpty()) {
        qCInfo(DSM) << "Install system language:" << locale;
        return installLanguage(locale);
    }
    return false;
}

void NetworkInitialization::ensureTranslatorInstalled()
{
    if (!m_initialized) {
        qCDebug(DSM) << "check connection status";
        checkAccountStatus();
        if (!m_initialized) {
            qCWarning(DSM) << "can not found current user, used default language to create connection";
            m_initialized = installSystemTranslator();
        }
    }
}

void NetworkInitialization::hideWirelessDevice(const QSharedPointer<NetworkManager::Device> &device, bool disableNetwork)
{
    NMQT_ASSERT_THREAD();
    if (!disableNetwork)
        return;

    bool managed = !disableNetwork;
    qCDebug(DSM()) << "device" << device->interfaceName() << "manager" << device->managed();
    if (device->managed() != managed) {
        setDeviceManaged(device->uni(), managed);
    }
    // 捕获设备路径而非裸指针：device 可能先于本连接被销毁。
    // lambda 无法用 Qt::UniqueConnection，重复进入（ConfigManager 重启）时不再挂接。
    const QString deviceUni = device->uni();
    if (!m_managedHooked.contains(deviceUni)) {
        m_managedHooked.insert(deviceUni);
        connect(device.data(), &NetworkManager::Device::managedChanged, this, [this, deviceUni, managed] {
            const NetworkManager::Device::Ptr current = NetworkManager::findNetworkInterface(deviceUni);
            if (current.isNull() || current->managed() == managed)
                return;

            qCDebug(DSM()) << "device" << current->interfaceName() << "managed changed" << current->managed() << ", will set it managed" << managed;
            setDeviceManaged(deviceUni, managed);
        });
    }
}

void NetworkInitialization::setDeviceManaged(const QString &deviceUni, bool managed)
{
    NMQT_ASSERT_THREAD();
    // 异步写：NM 线程上任何同步 D-Bus 等待都会停摆设备状态处理
    QDBusMessage message = QDBusMessage::createMethodCall(NETWORKMANAGER_SERVICE,
                                                         deviceUni,
                                                         QStringLiteral("org.freedesktop.DBus.Properties"),
                                                         QStringLiteral("Set"));
    message << QStringLiteral("org.freedesktop.NetworkManager.Device")
            << QStringLiteral("Managed")
            << QVariant::fromValue(QDBusVariant(managed));
    QDBusConnection::systemBus().callWithCallback(message, this, SLOT(onSetDeviceManagedFinished(QDBusMessage)),
                                                  SLOT(onSetDeviceManagedError(QDBusError, QDBusMessage)));
}

void NetworkInitialization::onSetDeviceManagedFinished(const QDBusMessage &reply)
{
    Q_UNUSED(reply)
}

void NetworkInitialization::onSetDeviceManagedError(const QDBusError &error, const QDBusMessage &message)
{
    qCWarning(DSM()) << "set Managed failed" << message.arguments().value(0).toString() << error.message();
}

void NetworkInitialization::initDeviceConnection(const QSharedPointer<NetworkManager::WiredDevice> &device)
{
    NMQT_ASSERT_THREAD();
    QString deviceUni = device->uni();
    qCDebug(DSM) << "init device connection" << deviceUni;
    connect(device.data(), &NetworkManager::WiredDevice::interfaceFlagsChanged, this, &NetworkInitialization::onWiredDevicePropertyChanged, Qt::UniqueConnection);
    connect(device.data(), &NetworkManager::WiredDevice::managedChanged, this, &NetworkInitialization::onWiredDevicePropertyChanged, Qt::UniqueConnection);
    connect(device.data(), &NetworkManager::WiredDevice::carrierChanged, this, &NetworkInitialization::onWiredDevicePropertyChanged, Qt::UniqueConnection);
    connect(device.data(), &NetworkManager::WiredDevice::availableConnectionDisappeared, this, &NetworkInitialization::onAvailableConnectionDisappeared, Qt::UniqueConnection);
}

void NetworkInitialization::checkAccountStatus()
{
    // 已经确定过当前用户语言后，installUserTranslator() 只会立即返回 true，
    // 再查一次纯属在 NM 线程上白等一次 D-Bus 往返（设备热插拔会频繁触发）。
    if (m_initialized) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(LOCKSERVICE, LOCKPATH, LOCKINTERFACE, QStringLiteral("CurrentUser"));
    const QDBusMessage reply = QDBusConnection::systemBus().call(message, QDBus::Block, kAccountQueryTimeoutMs);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        qCDebug(DSM()) << "query current user failed" << reply.errorMessage();
        return;
    }
    m_initialized = installUserTranslator(reply.arguments().first().toString());
}

void NetworkInitialization::updateConnectionLanguage(const QString &accountPath)
{
    QVariant localeVariant = accountInterface(accountPath, "Locale");
    QString locale = localeVariant.toString().split(".").first();
    installLanguage(locale);
    m_initialized = true;

    if (m_untranslactionConnections.isEmpty()) {
        qDebug() << "can't found untranslation connections";
        return;
    }
    qDebug() << "update connection launguage";

    updateConnectionLanguage();
}

void NetworkInitialization::updateConnectionLanguage()
{
    NMQT_ASSERT_THREAD();
    qCWarning(DSM) << "cache connection count" << m_untranslactionConnections.size();
    for (auto it = m_untranslactionConnections.begin(); it != m_untranslactionConnections.end(); it++) {
        NetworkManager::Connection::Ptr connection = NetworkManager::findConnectionByUuid(it.key());
        if (connection.isNull()) {
            qWarning() << "can't found connection " << it.key();
            continue;
        }

        QString name = tr("Wired Connection");
        if (it.value() > 0) {
            name += QString(" %1").arg(it.value());
        }
        connection->settings()->setId(name);
        QDBusPendingReply<> reply = connection->isUnsaved() ? connection->updateUnsaved(connection->settings()->toMap()) : connection->update(connection->settings()->toMap());
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << reply.error().message();
        }
        m_newConnectionNames[connection->path()] = name;
    }

    m_untranslactionConnections.clear();
}

bool NetworkInitialization::isServerSystem() const
{
    return (Dtk::Core::DSysInfo::uosType() == Dtk::Core::DSysInfo::UosServer);
}

void NetworkInitialization::onInitDeviceConnection()
{
    NMQT_ASSERT_THREAD();
    QList<NetworkManager::WirelessDevice::Ptr> wirelessDevices;
    NetworkManager::Device::List devices = NetworkManager::networkInterfaces();
    // 阻塞 D-Bus 调用，每台设备各查一次纯属浪费（且会卡住 NM 事件循环）
    checkAccountStatus();
    for (NetworkManager::Device::Ptr device: devices) {
        if (device->type() == NetworkManager::Device::Type::Wifi) {
            wirelessDevices << device.staticCast<NetworkManager::WirelessDevice>();
        } else if (device->type() == NetworkManager::Device::Type::Ethernet) {
            qCDebug(DSM) << "Wired device" << device->interfaceName() << "initilized" << m_initialized << ",add first connection";
            NetworkManager::WiredDevice::Ptr wiredDevice = device.staticCast<NetworkManager::WiredDevice>();
            initDeviceConnection(wiredDevice);
            if (m_initialized) {
                addFirstConnection(wiredDevice.data());
            }
        }
    }

    bool disableNetwork = SettingConfig::instance()->disableNetwork();
    qCDebug(DSM) << "disbled wireless network" << disableNetwork;
    for (const NetworkManager::WirelessDevice::Ptr &device : wirelessDevices) {
        hideWirelessDevice(device, disableNetwork);
    }
    // 本函数已完成这些设备的初始化，登记后 devicesChanged 不再重复处理
    for (const NetworkManager::Device::Ptr &device : devices) {
        m_seenDevices.insert(device->uni());
    }

    // 库信号只在代理内部 connect；这里消费代理的自有信号。
    // 本函数可能因 ConfigManager 重启再次进入，连接只能建立一次。
    if (m_devicesChangedConnection) {
        return;
    }
    m_devicesChangedConnection = connect(NetworkManagerProxy::instance(), &NetworkManagerProxy::devicesChanged, this, [this] {
        const QList<DeviceSnapshot> devices = NetworkManagerProxy::instance()->devices();

        // 先清理已消失的设备，避免这些集合长期增长，
        // 也让 uni 被复用的新设备能重新走一次初始化
        QSet<QString> alive;
        QSet<QString> aliveInterfaces;
        alive.reserve(devices.size());
        aliveInterfaces.reserve(devices.size());
        for (const DeviceSnapshot &device : devices) {
            alive.insert(device.uni);
            if (!device.interfaceName.isEmpty()) {
                aliveInterfaces.insert(device.interfaceName);
            }
        }
        for (auto it = m_seenDevices.begin(); it != m_seenDevices.end();) {
            it = alive.contains(*it) ? std::next(it) : m_seenDevices.erase(it);
        }
        for (auto it = m_managedHooked.begin(); it != m_managedHooked.end();) {
            it = alive.contains(*it) ? std::next(it) : m_managedHooked.erase(it);
        }
        // 以接口名为 key，网卡重新出现时会被重新登记
        for (auto it = m_lastCreateTime.begin(); it != m_lastCreateTime.end();) {
            it = aliveInterfaces.contains(it.key()) ? std::next(it) : m_lastCreateTime.erase(it);
        }
        // created-connection 缓存按 NM 现值裁剪：连接被外部删除后条目不会自己消失
        if (!m_newConnectionNames.isEmpty()) {
            QSet<QString> existing;
            const NetworkManager::Connection::List connections = NetworkManager::listConnections();
            existing.reserve(connections.size());
            for (const NetworkManager::Connection::Ptr &connection : connections) {
                existing.insert(connection->path());
            }
            for (auto it = m_newConnectionNames.begin(); it != m_newConnectionNames.end();) {
                it = existing.contains(it.key()) ? std::next(it) : m_newConnectionNames.erase(it);
            }
        }

        for (const DeviceSnapshot &device : devices) {
            if (!m_seenDevices.contains(device.uni)) {
                m_seenDevices.insert(device.uni);
                onDeviceAdded(device.uni);
            }
        }
    });
}

void NetworkInitialization::onWiredDevicePropertyChanged()
{
    NMQT_ASSERT_THREAD();
    NetworkManager::WiredDevice *wiredDevice = qobject_cast<NetworkManager::WiredDevice *>(sender());
    if (!wiredDevice)
        return;

    qCDebug(DSM) << "device" << wiredDevice->interfaceName() << "carrier:" << wiredDevice->carrier()
                 << "interfaceFlags" << wiredDevice->interfaceFlags() << "managed:" << wiredDevice->managed();
    addFirstConnection(wiredDevice);
}

void NetworkInitialization::onDeviceAdded(const QString &uni)
{
    NMQT_ASSERT_THREAD();
    NetworkManager::Device::Ptr device = NetworkManager::findNetworkInterface(uni);
    // 这里只处理无线网卡
    if (!device)
        return;

    switch (device->type()) {
    case NetworkManager::Device::Type::Wifi:
        hideWirelessDevice(device, SettingConfig::instance()->disableNetwork());
        break;
    case NetworkManager::Device::Type::Ethernet: {
        checkAccountStatus();
        qCDebug(DSM) << "new Wired device" << device->interfaceName() << "initilized" << m_initialized << ",add first connection";
        NetworkManager::WiredDevice::Ptr wiredDevice = device.staticCast<NetworkManager::WiredDevice>();
        initDeviceConnection(wiredDevice);
        if (m_initialized) {
            addFirstConnection(wiredDevice.data());
        }
        break;
    }
    default:
        break;
    }
}

void NetworkInitialization::onAvailableConnectionDisappeared(const QString &connectionUni)
{
    NMQT_ASSERT_THREAD();
    if (m_newConnectionNames.contains(connectionUni))
        m_newConnectionNames.remove(connectionUni);
}

void NetworkInitialization::onUserChanged(const QString &json)
{
    qCDebug(DSM) << "onUserChanged:" << json << "initilized =" << m_initialized;
    m_initialized = installUserTranslator(json);

    if (!m_initialized)
        return;

    addFirstConnection();
}

void NetworkInitialization::onUserAdded(const QString &user)
{
    qCDebug(DSM) << "onUserAdded:" << user << "initilized =" << m_initialized;
    updateConnectionLanguage(user);
    if (m_hasAddFirstConnection) {
        qDebug() << "has add connection";
        return;
    }

    // 取第一个用户的locale
    m_initialized = installUserTranslator(user);

    if (!m_initialized)
        return;

    addFirstConnection();
}
