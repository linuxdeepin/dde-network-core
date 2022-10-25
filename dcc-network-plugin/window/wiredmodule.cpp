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
#include "wiredmodule.h"
#include "controllitemsmodel.h"
#include "editpage/connectioneditpage.h"
#include "widgets/widgetmodule.h"
#include "widgets/switchwidget.h"
#include "widgets/settingsgroup.h"
#include "widgets/floatingbutton.h"

#include <DFloatingButton>
#include <DFontSizeManager>
#include <DListView>

#include <QApplication>
#include <QHBoxLayout>

#include <wireddevice.h>
#include <networkcontroller.h>

using namespace dde::network;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE

WiredModule::WiredModule(WiredDevice *dev, QObject *parent)
    : PageModule("networkWired", dev->deviceName(), dev->deviceName(), QIcon::fromTheme("dcc_ethernet"), parent)
    , m_device(dev)
{
    m_modules.append(new WidgetModule<SwitchWidget>("wired_adapter", tr("Wired Network Adapter"), [this](SwitchWidget *devEnabled) {
        QLabel *lblTitle = new QLabel(tr("Wired Network Adapter")); // 无线网卡
        DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
        devEnabled->setLeftWidget(lblTitle);
        devEnabled->setChecked(m_device->isEnabled());
        connect(devEnabled, &SwitchWidget::checkedChanged, m_device, &WiredDevice::setEnabled);
        connect(m_device, &WiredDevice::enableChanged, devEnabled, [devEnabled](const bool enabled) {
            devEnabled->blockSignals(true);
            devEnabled->setChecked(enabled);
            devEnabled->blockSignals(false);
        });
    }));
    m_modules.append(new WidgetModule<SettingsGroup>("nocable_tips", tr("Plug in the network cable first"), [](SettingsGroup *tipsGroup) {
        QLabel *tips = new QLabel;
        tips->setAlignment(Qt::AlignCenter);
        tips->setWordWrap(true);
        tips->setFixedHeight(80);
        tips->setText(tr("Plug in the network cable first"));
        tipsGroup->setBackgroundStyle(SettingsGroup::GroupBackground);
        tipsGroup->insertWidget(tips);
    }));
    m_modules.append(new WidgetModule<DListView>("List_wirelesslist", QString(), this, &WiredModule::initWirelessList));
    m_modules.append(new WidgetModule<QWidget>); // 加个空的保证下面有弹簧
    ModuleObject *extra = new WidgetModule<FloatingButton>("addWired", tr("Add Network Connection"), [this](FloatingButton *createBtn) {
        createBtn->setIcon(DStyle::StandardPixmap::SP_IncreaseElement);
        createBtn->setMinimumSize(QSize(47, 47));

        createBtn->setToolTip(tr("Add Network Connection"));
        connect(createBtn, &FloatingButton::clicked, this, [this]() {
            editConnection(nullptr);
        });
    });
    extra->setExtra();
    m_modules.append(extra);
    onDeviceStatusChanged(m_device->deviceStatus());
    connect(m_device, &WiredDevice::deviceStatusChanged, this, &WiredModule::onDeviceStatusChanged);
}

void WiredModule::onDeviceStatusChanged(const DeviceStatus &stat)
{
    const bool unavailable = stat <= DeviceStatus::Unavailable;
    int i = 0;
    for (ModuleObject *module : m_modules) {
        if (module->name() == "nocable_tips") {
            if (unavailable)
                insertChild(i++, module);
            else
                removeChild(module);
        } else {
            insertChild(i++, module);
        }
    }
}

void WiredModule::initWirelessList(DListView *lvProfiles)
{
    lvProfiles->setAccessibleName("lvProfiles");
    ControllItemsModel *model = new ControllItemsModel(lvProfiles);
    auto updateItems = [model, this]() {
        const QList<WiredConnection *> conns = m_device->items();
        QList<ControllItems *> items;
        for (WiredConnection *it : conns) {
            items.append(it);
            if (!m_newConnectionPath.isEmpty() && it->connection()->path() == m_newConnectionPath) {
                m_device->connectNetwork(it);
                m_newConnectionPath.clear();
            }
        }
        model->updateDate(items);
    };
    updateItems();
    connect(m_device, &WiredDevice::connectionAdded, model, updateItems);
    connect(m_device, &WiredDevice::connectionRemoved, model, updateItems);
    connect(m_device, &WiredDevice::activeConnectionChanged, model, &ControllItemsModel::updateStatus);
    connect(m_device, &WiredDevice::enableChanged, model, &ControllItemsModel::updateStatus);
    connect(m_device, &WiredDevice::connectionChanged, model, &ControllItemsModel::updateStatus);
    connect(m_device, &WiredDevice::deviceStatusChanged, model, &ControllItemsModel::updateStatus);
    connect(m_device, &WiredDevice::activeConnectionChanged, model, &ControllItemsModel::updateStatus);
    lvProfiles->setModel(model);
    lvProfiles->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lvProfiles->setBackgroundType(DStyledItemDelegate::BackgroundType::ClipCornerBackground);
    lvProfiles->setSelectionMode(QAbstractItemView::NoSelection);
    lvProfiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto adjustHeight = [lvProfiles]() {
        lvProfiles->setMinimumHeight(lvProfiles->model()->rowCount() * 41);
    };
    adjustHeight();
    connect(model, &ControllItemsModel::modelReset, lvProfiles, adjustHeight);
    connect(model, &ControllItemsModel::detailClick, this, &WiredModule::editConnection);

    // 点击有线连接按钮
    connect(lvProfiles, &DListView::clicked, this, [this](const QModelIndex &idx) {
        WiredConnection *connObj = static_cast<WiredConnection *>(idx.internalPointer());
        if (!connObj->connected())
            m_device->connectNetwork(connObj->connection()->path());
    });
}

void WiredModule::editConnection(dde::network::ControllItems *item)
{
    QString uuid = item ? item->connection()->uuid() : QString();
    ConnectionEditPage *m_editPage = new ConnectionEditPage(ConnectionEditPage::WiredConnection, m_device->path(), uuid, qApp->activeWindow());
    m_editPage->initSettingsWidget();
    m_editPage->setButtonTupleEnable(!item);
    connect(m_editPage, &ConnectionEditPage::activateWiredConnection, this, [this](const QString &connectPath, const QString &uuid) {
        Q_UNUSED(uuid);
        if (!m_device->connectNetwork(connectPath))
            m_newConnectionPath = connectPath;
    });
    connect(m_editPage, &ConnectionEditPage::disconnect, m_device, &NetworkDeviceBase::disconnectNetwork);
    m_editPage->exec();
    m_editPage->deleteLater();
}
