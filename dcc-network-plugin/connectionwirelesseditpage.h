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

#ifndef CONNECTIONWIRELESSEDITPAGE_H
#define CONNECTIONWIRELESSEDITPAGE_H

#include "connectioneditpage.h"

#include <networkmanagerqt/accesspoint.h>

class ConnectionWirelessEditPage : public ConnectionEditPage
{
    Q_OBJECT

public:
    explicit ConnectionWirelessEditPage(const QString &devPath, const QString &connUuid, const QString &apPath, bool isHidden = false, QWidget *parent = nullptr);
    virtual ~ConnectionWirelessEditPage() Q_DECL_OVERRIDE;

    // This method must be called after initialization
    void initSettingsWidgetFromAp();

private:
    void initApSecretType(AccessPoint::Ptr nmAp);
};

#endif /* CONNECTIONWIRELESSEDITPAGE_H */
