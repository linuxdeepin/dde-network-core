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

#ifndef VPNADVOPENVPNSECTION_H
#define VPNADVOPENVPNSECTION_H

#include "sections/abstractsection.h"

#include <networkmanagerqt/vpnsetting.h>

namespace DCC_NAMESPACE {
class SwitchWidget;
}

namespace dcc {
namespace network {
class SpinBoxWidget;
}
}

class VpnAdvOpenVPNSection : public AbstractSection
{
    Q_OBJECT

public:
    explicit VpnAdvOpenVPNSection(NetworkManager::VpnSetting::Ptr vpnSetting, QFrame *parent = nullptr);
    virtual ~VpnAdvOpenVPNSection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

private:
    void initUI();
    void initConnection();
    virtual bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

private:
    NetworkManager::VpnSetting::Ptr m_vpnSetting;
    NMStringMap m_dataMap;

    DCC_NAMESPACE::SwitchWidget *m_portSwitch;
    DCC_NAMESPACE::SwitchWidget *m_renegIntervalSwitch;
    DCC_NAMESPACE::SwitchWidget *m_compLZOSwitch;
    DCC_NAMESPACE::SwitchWidget *m_tcpProtoSwitch;
    DCC_NAMESPACE::SwitchWidget *m_useTapSwitch;
    DCC_NAMESPACE::SwitchWidget *m_tunnelMTUSwitch;
    DCC_NAMESPACE::SwitchWidget *m_udpFragSizeSwitch;
    DCC_NAMESPACE::SwitchWidget *m_restrictMSSSwitch;
    DCC_NAMESPACE::SwitchWidget *m_randomRemoteSwitch;

    dcc::network::SpinBoxWidget *m_port;
    dcc::network::SpinBoxWidget *m_renegInterval;
    dcc::network::SpinBoxWidget *m_tunnelMTU;
    dcc::network::SpinBoxWidget *m_udpFragSize;
};

#endif /* VPNADVOPENVPNSECTION_H */
