/*
* Copyright (C) 2021 ~ 2023 Deepin Technology Co., Ltd.
*
* Author:     caixiangrong <caixiangrong@uniontech.com>
*
* Maintainer: caixiangrong <caixiangrong@uniontech.com>
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
#ifndef WIREDMODULE_H
#define WIREDMODULE_H

#include "interface/moduleobject.h"

#include <dtkwidget_global.h>

DWIDGET_BEGIN_NAMESPACE
class DListView;
DWIDGET_END_NAMESPACE

namespace dde {
namespace network {
class NetworkDeviceBase;
class NetworkController;
class WiredDevice;
class WiredConnection;
class ControllItems;
enum class DeviceStatus;
}
}

class WiredModule : public DCC_NAMESPACE::ModuleObject
{
    Q_OBJECT
public:
    explicit WiredModule(dde::network::WiredDevice *dev, QObject *parent = nullptr);

public Q_SLOTS:
    void onDeviceStatusChanged(const dde::network::DeviceStatus &stat);

private Q_SLOTS:
    void initWirelessList(DTK_WIDGET_NAMESPACE::DListView *lvProfiles);
    void editConnection(dde::network::ControllItems *item);

private:
    dde::network::WiredDevice *m_device;
    QList<DCC_NAMESPACE::ModuleObject *> m_modules;
    QString m_newConnectionPath;
};

#endif // WIREDMODULE_H
