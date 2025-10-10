// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "servicefactory.h"

#include "accountnetworksessioncontainer.h"
#include "accountnetworksystemcontainer.h"
#include "accountnetworksystemservice.h"
#include "ipconflicthandler.h"
#include "networkdbus.h"
#include "networkproxy.h"
#include "networkproxychains.h"
#include "networksecretagent.h"
#include "networkstatehandler.h"
#include "sessioncontainer.h"
#include "sessionservice.h"
#include "settingconfig.h"
#include "systemcontainer.h"
#include "systemservice.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QTranslator>

ServiceFactory::ServiceFactory(bool isSystem, QDBusConnection *dbusConnection, QObject *parent)
    : QObject(parent)
    , m_isSystem(isSystem)
    , m_serviceObject(nullptr)
    , m_dbusConnection(dbusConnection)
{
    QTranslator *translator = new QTranslator(this);
    if (translator->load(QLocale::system(), "network-service-plugin", "_", "/usr/share/deepin-service-manager/network-service/translations")) {
        QCoreApplication::installTranslator(translator);
    }
}

QObject *ServiceFactory::serviceObject()
{
    m_serviceObject = createServiceObject(m_isSystem);
    return m_serviceObject;
}

QObject *ServiceFactory::createServiceObject(bool isSystem)
{
    if (SettingConfig::instance()->enableAccountNetwork()) {
        // 如果开启了用户私有账户网络（工银瑞信定制）
        if (isSystem)
            return new accountnetwork::systemservice::AccountNetworkSystemService(new accountnetwork::systemservice::AccountNetworkSystemContainer(this), this);

        return new network::sessionservice::SessionService(new accountnetwork::sessionservice::AccountNetworkSessionContainer(this), this);
    }
    if (isSystem) {
        auto obj = new network::systemservice::SystemService(new network::systemservice::SystemContainer(this), this);
        QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        m_dbusConnection->registerObject("/org/deepin/service/SystemNetwork", obj, opts);
    } else {
        auto obj = new network::sessionservice::SessionService(new network::sessionservice::SessionContainer(this), this);
    }

    if (isSystem) {
        bool ret = QDBusConnection::systemBus().registerService("org.deepin.dde.Network10");
        network::systemservice::NetworkDBus *networkDBus = new network::systemservice::NetworkDBus(*m_dbusConnection, this);
        QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        return networkDBus;
    } else {
        network::sessionservice::NetworkStateHandler *stateHandler = new network::sessionservice::NetworkStateHandler(this);
        network::sessionservice::NetworkSecretAgent *secretAgent = new network::sessionservice::NetworkSecretAgent(this);
        network::sessionservice::NetworkProxy *networkProxy = new network::sessionservice::NetworkProxy(*m_dbusConnection, stateHandler, this);
        QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
        network::sessionservice::NetworkProxyChains *networkProxyChains = new network::sessionservice::NetworkProxyChains(*m_dbusConnection, stateHandler, this);
        m_dbusConnection->registerObject("/org/deepin/dde/Network1/ProxyChains", networkProxyChains, opts);
        return networkProxy;
    }
}
