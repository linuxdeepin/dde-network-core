/*
 * Copyright (C) 2011 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     listenerri <listenerri@gmail.com>
 *
 * Maintainer: listenerri <listenerri@gmail.com>
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

#ifndef VPNSSTPPROXYSECTION_H
#define VPNSSTPPROXYSECTION_H

#include "sections/abstractsection.h"

#include <networkmanagerqt/vpnsetting.h>

namespace DCC_NAMESPACE {
class LineEditWidget;
}

namespace dcc {
namespace network {
class SpinBoxWidget;
}
}

class VpnSstpProxySection : public AbstractSection
{
    Q_OBJECT

public:
    explicit VpnSstpProxySection(NetworkManager::VpnSetting::Ptr vpnSetting, QFrame *parent = nullptr);
    virtual ~VpnSstpProxySection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

private:
    void initUI();
    void initConnection();

private:
    NetworkManager::VpnSetting::Ptr m_vpnSetting;
    NMStringMap m_dataMap;
    NMStringMap m_secretMap;

    DCC_NAMESPACE::LineEditWidget *m_server;
    dcc::network::SpinBoxWidget *m_port;
    DCC_NAMESPACE::LineEditWidget *m_userName;
    DCC_NAMESPACE::LineEditWidget *m_password;
};

#endif /* VPNSSTPPROXYSECTION_H */
