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
#include "dslmodule.h"
#include "wireddevice.h"
#include "controllitemsmodel.h"
#include <editpage/connectioneditpage.h>

#include <DFloatingButton>
#include <DListView>

#include <QHBoxLayout>
#include <QApplication>

#include <widgets/widgetmodule.h>
#include "widgets/floatingbutton.h"

#include <networkcontroller.h>
#include <dslcontroller.h>

using namespace dde::network;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE

DSLModule::DSLModule(QObject *parent)
    : PageModule("networkDsl", tr("DSL"), tr("DSL"), QIcon::fromTheme("dcc_dsl"), parent)
{
    appendChild(new WidgetModule<DListView>("List_pppoelist", tr("Wired List"), this, &DSLModule::initDSLList));
    ModuleObject *extra = new WidgetModule<FloatingButton>("createDSL", tr("Create PPPoE Connection"), [this](FloatingButton *createBtn) {
        createBtn->setIcon(DStyle::StandardPixmap::SP_IncreaseElement);
        createBtn->setMinimumSize(QSize(47, 47));

        createBtn->setToolTip(tr("Create PPPoE Connection"));
        createBtn->setAccessibleName(tr("Create PPPoE Connection"));
        connect(createBtn, &DFloatingButton::clicked, this, [this]() {
            editConnection(nullptr);
        });
    });
    extra->setExtra();
    appendChild(extra);
}

void DSLModule::initDSLList(DListView *lvsettings)
{
    lvsettings->setAccessibleName("List_pppoelist");
    ControllItemsModel *model = new ControllItemsModel(lvsettings);
    auto updateItems = [model]() {
        const QList<DSLItem *> conns = NetworkController::instance()->dslController()->items();
        QList<ControllItems *> items;
        for (DSLItem *it : conns) {
            items.append(it);
        }
        model->updateDate(items);
    };
    DSLController *dslController = NetworkController::instance()->dslController();
    updateItems();
    connect(dslController, &DSLController::itemAdded, model, updateItems);
    connect(dslController, &DSLController::itemRemoved, model, updateItems);
    connect(dslController, &DSLController::itemChanged, model, &ControllItemsModel::updateStatus);
    connect(dslController, &DSLController::activeConnectionChanged, model, &ControllItemsModel::updateStatus);

    lvsettings->setModel(model);
    lvsettings->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lvsettings->setBackgroundType(DStyledItemDelegate::BackgroundType::ClipCornerBackground);
    lvsettings->setSelectionMode(QAbstractItemView::NoSelection);
    lvsettings->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto adjustHeight = [lvsettings]() {
        lvsettings->setMinimumHeight(lvsettings->model()->rowCount() * 41);
    };
    adjustHeight();
    connect(model, &ControllItemsModel::modelReset, lvsettings, adjustHeight);
    connect(model, &ControllItemsModel::detailClick, this, &DSLModule::editConnection);
    connect(lvsettings, &DListView::clicked, this, [](const QModelIndex &idx) {
        DSLItem *item = static_cast<DSLItem *>(idx.internalPointer());
        if (!item->connected())
            NetworkController::instance()->dslController()->connectItem(item);
    });
}

void DSLModule::editConnection(dde::network::ControllItems *item)
{
    QString devPath = "/";
    QString connUuid;
    if (item) {
        QList<NetworkDeviceBase *> devices = NetworkController::instance()->devices();
        const QString macAddress = item->connection()->hwAddress();
        connUuid = item->connection()->uuid();
        for (NetworkDeviceBase *device : devices) {
            if (device->realHwAdr() == macAddress) {
                devPath = device->path();
                break;
            }
        }
    }
    ConnectionEditPage *m_editPage = new ConnectionEditPage(ConnectionEditPage::ConnectionType::PppoeConnection, devPath, connUuid, qApp->activeWindow());
    m_editPage->initSettingsWidget();
    connect(m_editPage, &ConnectionEditPage::disconnect, m_editPage, [] {
        NetworkController::instance()->dslController()->disconnectItem();
    });

    if (item) {
        m_editPage->setLeftButtonEnable(true);
    } else {
        m_editPage->setButtonTupleEnable(true);
    }
    m_editPage->exec();
    m_editPage->deleteLater();
}
