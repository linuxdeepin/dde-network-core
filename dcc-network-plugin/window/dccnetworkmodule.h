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

#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include "interface/moduleobject.h"
#include "interface/plugininterface.h"

namespace dde {
namespace network {
class NetworkDeviceBase;
}
}

class NetworkModule : public DCC_NAMESPACE::ModuleObject
{
    Q_OBJECT
public:
    explicit NetworkModule(QObject *parent = nullptr);
    ~NetworkModule() override {}
    virtual void active() override;

private Q_SLOTS:
    void updateVisiable();
    void updateModel();

protected:
    bool event(QEvent *ev) override;
    void init();

private:
    QList<DCC_NAMESPACE::ModuleObject *> m_modules;
    QList<DCC_NAMESPACE::ModuleObject *> m_wiredModules;
    QList<DCC_NAMESPACE::ModuleObject *> m_wirelessModules;
    QMap<dde::network::NetworkDeviceBase *, DCC_NAMESPACE::ModuleObject *> m_deviceMap;
//    QTimer *m_timer;
};

class DccNetworkPlugin : public DCC_NAMESPACE::PluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PluginInterface_iid FILE "network.json")
    Q_INTERFACES(DCC_NAMESPACE::PluginInterface)
public:
    explicit DccNetworkPlugin(QObject *parent = nullptr);
    ~DccNetworkPlugin() override;

    virtual QString name() const override;
    virtual DCC_NAMESPACE::ModuleObject *module() override;

private:
    DCC_NAMESPACE::ModuleObject *m_moduleRoot;
};

#endif // NETWORKINTERFACE_H
