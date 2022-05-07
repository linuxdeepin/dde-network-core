/*
 * Copyright (C) 2020 ~ 2021 Uniontech Technology Co., Ltd.
 *
 * Author:     donghualin <donghualin@uniontech.com>
 *
 * Maintainer: donghualin <donghualin@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dccnetworkmodule.h"
#include "wirelessmodule.h"
#include "wiredmodule.h"
#include "dslmodule.h"
#include "vpnmodule.h"
#include "sysproxymodule.h"
#include "appproxymodule.h"
#include "hotspotmodule.h"
#include "networkinfomodule.h"

#include <QLayout>
#include <QMap>
#include <QEvent>
#include <QTimer>

#include <networkcontroller.h>
#include <networkdevicebase.h>
#include <wireddevice.h>
#include <wirelessdevice.h>
#include <dslcontroller.h>
#include <hotspotcontroller.h>

#include <NetworkManagerQt/Manager>

DCC_USE_NAMESPACE
using namespace dde::network;

NetworkModule::NetworkModule(QObject *parent)
    : ModuleObject("Network", tr("Network"), tr("Network"), QIcon::fromTheme("dcc_nav_network"), parent)
{
    setChildType(ModuleObject::ChildType::HList);
}
void NetworkModule::init()
{
    connect(NetworkController::instance(), &NetworkController::deviceAdded, this, &NetworkModule::updateModel);
    connect(NetworkController::instance(), &NetworkController::deviceRemoved, this, &NetworkModule::updateModel);
    connect(NetworkController::instance()->hotspotController(), &HotspotController::deviceAdded, this, &NetworkModule::updateVisiable);
    connect(NetworkController::instance()->hotspotController(), &HotspotController::deviceRemove, this, &NetworkModule::updateVisiable);

    m_modules.append(new DSLModule(this));         // DSL
    m_modules.append(new VPNModule(this));         // VPN
    m_modules.append(new SysProxyModule(this));    // 代理
    m_modules.append(new AppProxyModule(this));    // 应用代理
    m_modules.append(new HotspotModule(this));     // 热点
    m_modules.append(new NetworkInfoModule(this)); // 网络详情
}

void NetworkModule::active()
{
    if (childrens().isEmpty()) {
        updateModel();
    }
}

void NetworkModule::updateVisiable()
{
    int row = 0;
    int i = 1;
    bool emptyHotspot = NetworkController::instance()->hotspotController()->devices().isEmpty();
    for (ModuleObject *&module : m_wiredModules) {
        module->moduleData()->DisplayName = tr("Wired %1").arg(i++);
        insertChild(row++, module);
    }
    if (m_wiredModules.size() == 1) {
        m_wiredModules.first()->moduleData()->DisplayName = tr("Wired");
    }
    i = 1;
    for (ModuleObject *&module : m_wirelessModules) {
        module->moduleData()->DisplayName = tr("Wireless %1").arg(i++);
        insertChild(row++, module);
    }
    if (m_wirelessModules.size() == 1) {
        m_wirelessModules.first()->moduleData()->DisplayName = tr("Wireless");
    }
    for (ModuleObject *&module : m_modules) {
        if (emptyHotspot && module->name() == QStringLiteral("personalHotspot")) { // 热点根据设备显示
            removeChild(module);
        } else {
            insertChild(row++, module);
        }
    }
}

void NetworkModule::updateModel()
{
    QList<DCC_NAMESPACE::ModuleObject *> removeModules;
    removeModules.append(m_wirelessModules);
    removeModules.append(m_wiredModules);
    m_wirelessModules.clear();
    m_wiredModules.clear();
    QList<NetworkDeviceBase *> devices = NetworkController::instance()->devices();
    for (NetworkDeviceBase *&dev : devices) {
        if (dev->deviceType() == DeviceType::Wireless) {
            if (m_deviceMap.contains(dev)) {
                ModuleObject *module = m_deviceMap.value(dev);
                m_wirelessModules.append(module);
                removeModules.removeOne(module);
            } else {
                ModuleObject *module = new WirelessModule(static_cast<WirelessDevice *>(dev), this);
                m_wirelessModules.append(module);
                m_deviceMap.insert(dev, module);
            }
        } else if (dev->deviceType() == DeviceType::Wired) {
            if (m_deviceMap.contains(dev)) {
                ModuleObject *module = m_deviceMap.value(dev);
                m_wiredModules.append(module);
                removeModules.removeOne(module);
            } else {
                ModuleObject *module = new WiredModule(static_cast<WiredDevice *>(dev), this);
                m_wiredModules.append(module);
                m_deviceMap.insert(dev, module);
            }
        }
    }
    for (ModuleObject *&module : removeModules) {
        for (QMap<NetworkDeviceBase *, ModuleObject *>::iterator it = m_deviceMap.begin(); it != m_deviceMap.end(); ++it) {
            if (it.value() == module) {
                m_deviceMap.erase(it);
                break;
            }
        }
        removeChild(module);
        delete module;
    }
    updateVisiable();
}

bool NetworkModule::event(QEvent *ev)
{
    if (ev->type() == QEvent::ThreadChange) { // 网络中有用到单例，信号槽会受线程影响，需在主线程中初始化
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() {
            init();
            updateModel();
        });
        connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
        timer->setSingleShot(true);
        timer->start(10);
    }
    return ModuleObject::event(ev);
}

DccNetworkPlugin::DccNetworkPlugin(QObject *parent)
    : PluginInterface(parent)
    , m_moduleRoot(nullptr)
{
}

DccNetworkPlugin::~DccNetworkPlugin()
{
    m_moduleRoot = nullptr;
}

QString DccNetworkPlugin::name() const
{
    return QStringLiteral("network");
}

ModuleObject *DccNetworkPlugin::module()
{
    if (m_moduleRoot)
        return m_moduleRoot;

    m_moduleRoot = new NetworkModule;
    return m_moduleRoot;
}
