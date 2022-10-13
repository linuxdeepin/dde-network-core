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

#ifndef VPNSSTPSECTION_H
#define VPNSSTPSECTION_H

#include "vpnsection.h"

#include <networkmanagerqt/vpnsetting.h>

namespace dcc {
namespace network {
class FileChooseWidget;
}
}

namespace DCC_NAMESPACE {
class SwitchWidget;
}

class VpnSSTPSection : public VpnSection
{
    Q_OBJECT

public:
    explicit VpnSSTPSection(NetworkManager::VpnSetting::Ptr vpnSetting, QFrame *parent = nullptr);
    virtual ~VpnSSTPSection() Q_DECL_OVERRIDE;

    void saveSettings() Q_DECL_OVERRIDE;

private:
    void initUI();

private:
    NetworkManager::VpnSetting::Ptr m_vpnSetting;
    NMStringMap m_dataMap;

    dcc::network::FileChooseWidget *m_caFile;
    DCC_NAMESPACE::SwitchWidget *m_ignoreCAWarnings;
    DCC_NAMESPACE::SwitchWidget *m_useTLSExt;
};

#endif /* VPNSSTPSECTION_H */
