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
#include "networkinfomodule.h"

#include <networkcontroller.h>
#include <networkdetails.h>

#include <widgets/settingshead.h>
#include <widgets/widgetmodule.h>
#include <widgets/titlevalueitem.h>

using namespace dde::network;
DCC_USE_NAMESPACE

NetworkInfoModule::NetworkInfoModule(QObject *parent)
    : PageModule("networkDetails", tr("Network Details"), tr("Network Details"), QIcon::fromTheme("dcc_network"), parent)
{
    connect(NetworkController::instance(), &NetworkController::activeConnectionChange, this, &NetworkInfoModule::onUpdateNetworkInfo);

    onUpdateNetworkInfo();
}

void NetworkInfoModule::onUpdateNetworkInfo()
{
    while (getChildrenSize() > 0) {
        removeChild(0);
    }
    QList<NetworkDetails *> netDetails = NetworkController::instance()->networkDetails();
    int size = netDetails.size();
    for (int i = 0; i < size; i++) {
        NetworkDetails *detail = netDetails[i];
        appendChild(new WidgetModule<SettingsHead>("", tr(""), [detail](SettingsHead *head) {
            head->setEditEnable(false);
            head->setContentsMargins(10, 0, 0, 0);
            head->setTitle(detail->name());
        }));
        QList<QPair<QString, QString>> items = detail->items();
        for (const QPair<QString, QString> &item : items)
            appendChild(new WidgetModule<TitleValueItem>("", tr(""), [item](TitleValueItem *valueItem) {
                valueItem->setTitle(item.first);
                valueItem->setValue(item.second);
                valueItem->addBackground();
                if (item.first == "IPv6")
                    valueItem->setWordWrap(false);
            }));
        if (i < size - 1)
            appendChild(new WidgetModule<QWidget>());
    }
}
