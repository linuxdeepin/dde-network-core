// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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
#include "vpndnsmode/vpndnsmodehandler.h"
#include "nmaccess.h"

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
    if (isSystem) {
        // NM Qt 单例在首次触碰时锚定线程；必须在任何 system 组件构造之前完成，
        // 否则单例会落到插件线程，后续 NM 线程上的访问将跨线程。
        // 两条 system 分支（普通 / 账户网络定制）都需要先锚定。
        network::systemservice::ensureNetworkManagerInitialized();
    }

    if (SettingConfig::instance()->enableAccountNetwork()) {
        // 如果开启了用户私有账户网络（工银瑞信定制）
        if (isSystem) {
            // 定制分支同样要导出 SystemNetwork 路径与 NetworkDBus：
            // session 侧的 ActiveAccountNetwork 依赖 /org/deepin/service/SystemNetwork
            // 上的 AccountSystemNetwork 接口，NetworkActivator 依赖
            // /org/deepin/dde/Network1 的 IsDeviceEnabled。
            auto *obj = new accountnetwork::systemservice::AccountNetworkSystemService(
                    new accountnetwork::systemservice::AccountNetworkSystemContainer(this), this);
            QDBusConnection::RegisterOptions opts = QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties;
            m_dbusConnection->registerObject("/org/deepin/service/SystemNetwork", obj, opts);

            QDBusConnection::systemBus().registerService("org.deepin.dde.Network10");
            return new network::systemservice::NetworkDBus(*m_dbusConnection, this);
        }

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
        network::sessionservice::VpnDnsModeHandler *vpnDnsModeHandler = new network::sessionservice::VpnDnsModeHandler(this);
        return networkProxy;
    }
}
