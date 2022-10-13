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

#ifndef PPPSECTION_H
#define PPPSECTION_H

#include "abstractsection.h"

#include <networkmanagerqt/pppsetting.h>

namespace DCC_NAMESPACE {
class SwitchWidget;
}

class PPPSection : public AbstractSection
{
    Q_OBJECT

public:
    explicit PPPSection(NetworkManager::PppSetting::Ptr pppSetting, QFrame *parent = nullptr);
    virtual ~PPPSection() Q_DECL_OVERRIDE;

    bool allInputValid() Q_DECL_OVERRIDE;
    void saveSettings() Q_DECL_OVERRIDE;

private:
    void initStrMaps();
    void initUI();
    void initConnection();
    void onMppeEnableChanged(const bool checked);

private:
    QMap<QString, QString> OptionsStrMap;

    NetworkManager::PppSetting::Ptr m_pppSetting;

    DCC_NAMESPACE::SwitchWidget *m_mppeEnable;
    DCC_NAMESPACE::SwitchWidget *m_mppe128;
    DCC_NAMESPACE::SwitchWidget *m_mppeStateful;
    DCC_NAMESPACE::SwitchWidget *m_refuseEAP;
    DCC_NAMESPACE::SwitchWidget *m_refusePAP;
    DCC_NAMESPACE::SwitchWidget *m_refuseCHAP;
    DCC_NAMESPACE::SwitchWidget *m_refuseMSCHAP;
    DCC_NAMESPACE::SwitchWidget *m_refuseMSCHAP2;
    DCC_NAMESPACE::SwitchWidget *m_noBSDComp;
    DCC_NAMESPACE::SwitchWidget *m_noDeflate;
    DCC_NAMESPACE::SwitchWidget *m_noVJComp;
    DCC_NAMESPACE::SwitchWidget *m_lcpEchoInterval;
};

#endif /* PPPSECTION_H */
