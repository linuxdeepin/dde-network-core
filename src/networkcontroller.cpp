// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "networkcontroller.h"

#include "configsetting.h"
#include "dslcontroller.h"
#include "hotspotcontroller.h"
#include "netutils.h"
#include "networkdetails.h"
#include "networkdevicebase.h"
#include "networkinterprocesser.h"
#include "networkmanagerprocesser.h"
#include "proxycontroller.h"
#include "vpncontroller.h"
#include "wireddevice.h"
#include "wirelessdevice.h"
#include "connectivityhandler.h"

#include <DLog>

// const static QString networkService = "org.deepin.dde.Network1";
// const static QString networkPath = "/org/deepin/dde/Network1";
static QString localeName;
Q_LOGGING_CATEGORY(DNC, "org.deepin.dde.dcc.network");
using namespace dde::network;

NetworkController *NetworkController::m_networkController = nullptr;
// 默认是异步方式
bool NetworkController::m_sync = false;
// 只有任务栏需要检测IP冲突，因此任务栏需要调用相关的接口来检测，其他的应用是不需要检测冲突的
bool NetworkController::m_checkIpConflicted = false;
QTranslator *NetworkController::m_translator = nullptr;

NetworkController::NetworkController()
    : QObject(Q_NULLPTR)
    , m_processer(Q_NULLPTR)
    , m_proxyController(Q_NULLPTR)
    , m_vpnController(Q_NULLPTR)
    , m_dslController(Q_NULLPTR)
    , m_hotspotController(Q_NULLPTR)
    , m_connectivityHandler(new ConnectivityHandler(this))
{
    Dtk::Core::loggerInstance()->logToGlobalInstance(DNC().categoryName(), true);
    retranslate(QLocale().name());

    if (ConfigSetting::instance()->serviceFromNetworkManager()) {
        m_processer = new NetworkManagerProcesser(m_sync, this);
    } else {
        // 不应该走该分支
        qCWarning(DNC())<<"error: use Network Inter !!!";
        m_processer = new NetworkInterProcesser(m_sync, this);
    }
    connect(m_processer, &NetworkProcesser::deviceAdded, this, &NetworkController::onDeviceAdded);
    connect(m_processer, &NetworkProcesser::deviceRemoved, this, &NetworkController::deviceRemoved);
    connect(m_processer, &NetworkProcesser::connectionChanged, this, &NetworkController::connectionChanged);
    connect(m_processer, &NetworkProcesser::activeConnectionChange, this, &NetworkController::activeConnectionChange);
    connect(m_connectivityHandler, &ConnectivityHandler::connectivityChanged, this, &NetworkController::connectivityChanged);

    initNetworkStatus();
}

void NetworkController::initNetworkStatus()
{
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(this);
    serviceWatcher->setConnection(QDBusConnection::systemBus());
    serviceWatcher->addWatchedService("org.deepin.dde.Network1");
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [ this ](const QString &service) {
        if (service != "org.deepin.dde.Network1")
            return;

        // 启动后过3秒再获取连接状态，因为在刚启动的时候，获取的状态不是正确的
        QTimer::singleShot(3000, m_connectivityHandler, &ConnectivityHandler::init);
        checkIpConflicted(m_processer->devices());
    });

    if (m_checkIpConflicted) {
        // 如果当前需要处理IP冲突，则直接获取信号连接方式即可
        QDBusConnection::systemBus().connect("org.deepin.dde.Network1", "/org/deepin/service/SystemNetwork",
                                    "org.deepin.service.SystemNetwork", "IpConflictChanged", m_processer, SLOT(onIpConflictChanged(const QString &, const QString &, bool)));
        checkIpConflicted(m_processer->devices());
    }
}

void NetworkController::checkIpConflicted(const QList<NetworkDeviceBase *> &devices)
{
    if (!m_checkIpConflicted)
        return ;

    static QDBusInterface dbusInter("org.deepin.dde.Network1", "/org/deepin/service/SystemNetwork",
                             "org.deepin.service.SystemNetwork", QDBusConnection::systemBus());
    // 如果需要处理IP冲突，依次检测每个设备的IP是否冲突
    for (NetworkDeviceBase *device : devices) {
        QDBusReply<bool> reply = dbusInter.call("IpConflicted", device->path());
        m_processer->onIpConflictChanged(device->path(), "", reply.value());
    }
}

void NetworkController::onDeviceAdded(QList<NetworkDeviceBase *> device)
{
    checkIpConflicted(device);
    emit deviceAdded(device);
}

NetworkController::~NetworkController() = default;

NetworkController *NetworkController::instance()
{
    static QMutex m;
    QMutexLocker locker(&m);
    if (!m_networkController) {
        m_networkController = new NetworkController;
    };
    return m_networkController;
}

void NetworkController::free()
{
    if (m_networkController) {
        m_networkController->deleteLater();
        m_networkController = nullptr;
    }
}

void NetworkController::setActiveSync(const bool sync)
{
    m_sync = sync;
}

void NetworkController::setIPConflictCheck(const bool &checkIp)
{
    m_checkIpConflicted = checkIp;
}

void NetworkController::alawaysLoadFromNM()
{
    ConfigSetting::instance()->alawaysLoadFromNM();
}

void NetworkController::installTranslator(const QString &locale)
{
    if (localeName == locale)
        return;

    localeName = locale;

    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
    } else {
        m_translator = new QTranslator;
    }
    if (m_translator->load(QLocale(localeName), "dde-network-core", "_", "/usr/share/dde-network-core/translations")) {
        QCoreApplication::installTranslator(m_translator);
        qInfo() << "Loaded translation file for dde-network-core:" << m_translator->filePath();
    } else {
        qWarning() << "Failed to load translation file for dde-network-core";
        m_translator->deleteLater();
        m_translator = nullptr;
    }
}

void NetworkController::updateSync(const bool sync)
{
    auto *processer = qobject_cast<NetworkInterProcesser *>(m_processer);
    if (processer)
        processer->updateSync(sync);
}

ProxyController *NetworkController::proxyController()
{
    return m_processer->proxyController();
}

VPNController *NetworkController::vpnController()
{
    return m_processer->vpnController();
}

DSLController *NetworkController::dslController()
{
    return m_processer->dslController();
}

HotspotController *NetworkController::hotspotController()
{
    return m_processer->hotspotController();
}

QList<NetworkDetails *> NetworkController::networkDetails()
{
    return m_processer->networkDetails();
}

QList<NetworkDeviceBase *> NetworkController::devices() const
{
    return m_processer->devices();
}

Connectivity NetworkController::connectivity()
{
    return m_connectivityHandler->connectivity();
}

void NetworkController::retranslate(const QString &locale)
{
    NetworkController::installTranslator(locale);
    if (m_processer)
        m_processer->retranslate();
}
