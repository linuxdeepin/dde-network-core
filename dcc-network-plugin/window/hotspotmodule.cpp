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
#include "controllitemsmodel.h"
#include "hotspotmodule.h"
#include "editpage/connectionhotspoteditpage.h"
#include "widgets/floatingbutton.h"

#include <DFloatingButton>
#include <DFontSizeManager>
#include <DListView>

#include <QHBoxLayout>
#include <QApplication>
#include <QPushButton>

#include <widgets/switchwidget.h>
#include <widgets/widgetmodule.h>

#include <wirelessdevice.h>
#include <networkcontroller.h>
#include <hotspotcontroller.h>

using namespace dde::network;
using namespace DCC_NAMESPACE;
DWIDGET_USE_NAMESPACE

HotspotDeviceItem::HotspotDeviceItem(WirelessDevice *dev, QObject *parent)
    : QObject(parent)
    , m_device(dev)
{
    m_modules.append(new WidgetModule<SwitchWidget>("hotspotSwitch", tr("Hotspot"), [this](SwitchWidget *hotspotSwitch) {
        QLabel *lblTitle = new QLabel(tr("Hotspot")); // 个人热点
        DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
        hotspotSwitch->setLeftWidget(lblTitle);
        hotspotSwitch->setChecked(NetworkController::instance()->hotspotController()->enabled(m_device));

        connect(hotspotSwitch, &SwitchWidget::checkedChanged, this, &HotspotDeviceItem::onSwitchToggled);

        connect(m_device, &WirelessDevice::hotspotEnableChanged, hotspotSwitch, [hotspotSwitch, this](const bool &enable) {
            hotspotSwitch->blockSignals(true);
            hotspotSwitch->setChecked(NetworkController::instance()->hotspotController()->enabled(m_device));
            hotspotSwitch->blockSignals(false);
        });
        connect(NetworkController::instance()->hotspotController(), &HotspotController::enableHotspotSwitch, hotspotSwitch, &SwitchWidget::setEnabled);
    }));
    m_modules.append(new WidgetModule<DListView>("list_hotspot", QString(), this, &HotspotDeviceItem::initHotspotList));
    m_modules.append(new WidgetModule<QPushButton>("hotspot_createBtn", tr("Add Settings"), [this](QPushButton *createBtn) {
        createBtn->setText(tr("Add Settings"));
        connect(createBtn, &QPushButton::clicked, this, [this]() {
            openEditPage(nullptr);
        });
    }));
}

void HotspotDeviceItem::initHotspotList(DListView *lvProfiles)
{
    ControllItemsModel *model = new ControllItemsModel(lvProfiles);
    model->setDisplayRole(ControllItemsModel::ssid);
    auto updateItems = [model, this]() {
        HotspotController *hotspotController = NetworkController::instance()->hotspotController();
        QList<HotspotItem *> conns = hotspotController->items(m_device);
        QList<ControllItems *> items;
        for (HotspotItem *it : conns) {
            items.append(it);
        }
        model->updateDate(items);
    };
    updateItems();
    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    connect(hotspotController, &HotspotController::itemAdded, model, updateItems);
    connect(hotspotController, &HotspotController::itemRemoved, model, updateItems);
    connect(hotspotController, &HotspotController::itemChanged, model, &ControllItemsModel::updateStatus);
    connect(hotspotController, &HotspotController::activeConnectionChanged, model, &ControllItemsModel::updateStatus);

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
    connect(model, &ControllItemsModel::detailClick, this, &HotspotDeviceItem::openEditPage);

    connect(lvProfiles, &QListView::clicked, this, [this](const QModelIndex &index) {
        if (NetworkController::instance()->hotspotController()->enabled(m_device)) {
            NetworkController::instance()->hotspotController()->connectItem(static_cast<HotspotItem *>(index.internalPointer()));
        }
    });
}

void HotspotDeviceItem::onSwitchToggled(const bool checked)
{
    SwitchWidget *switchWidget = qobject_cast<SwitchWidget *>(sender());
    if (!switchWidget)
        return;

    switchWidget->setEnabled(false);
    if (checked) {
        openHotspot(switchWidget);
    } else {
        closeHotspot();
    }
}

void HotspotDeviceItem::closeHotspot()
{
    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    // 关闭热点
    hotspotController->setEnabled(m_device, false);
}

void HotspotDeviceItem::openHotspot(SwitchWidget *switchWidget)
{
    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    QList<HotspotItem *> items = hotspotController->items(m_device);
    if (items.isEmpty()) {
        switchWidget->setChecked(false);
        switchWidget->setEnabled(true);
        openEditPage(nullptr);
    } else {
        // 开启热点
        hotspotController->setEnabled(m_device, true);
    }
}

void HotspotDeviceItem::openEditPage(ControllItems *item)
{
    QString uuid;
    if (item)
        uuid = item->connection()->uuid();
    ConnectionHotspotEditPage *m_editPage = new ConnectionHotspotEditPage(m_device->path(), uuid, qApp->activeWindow());
    m_editPage->initSettingsWidget();
    m_editPage->setButtonTupleEnable(true);
    m_editPage->exec();
    m_editPage->deleteLater();
}
///////////////////////////////////////////////////////////////////////////
HotspotModule::HotspotModule(QObject *parent)
    : PageModule("personalHotspot", tr("Personal Hotspot"), tr("Personal Hotspot"), QIcon::fromTheme("dcc_hotspot"), parent)
{
    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    connect(hotspotController, &HotspotController::deviceAdded, this, &HotspotModule::onDeviceAdded);
    connect(hotspotController, &HotspotController::deviceRemove, this, &HotspotModule::onDeviceRemove);
    ModuleObject *extra = new WidgetModule<FloatingButton>("createHotspot", tr("Create Hotspot"), [this](FloatingButton *newprofile) {
        newprofile->setIcon(DStyle::StandardPixmap::SP_IncreaseElement);
        newprofile->setMinimumSize(QSize(47, 47));

        newprofile->setToolTip(tr("Create Hotspot"));
        connect(newprofile, &QAbstractButton::clicked, this, [this] {
            if (m_items.empty())
                return;

            m_items.front()->openEditPage(nullptr);
        });
    });
    extra->setExtra();
    connect(this, &HotspotModule::updateItemOnlyOne, extra, [extra](bool visiable) {
        extra->setHidden(!visiable);
    });
    appendChild(extra);
    onDeviceAdded(hotspotController->devices());
}

void HotspotModule::onDeviceAdded(const QList<WirelessDevice *> &devices)
{
    QSet<WirelessDevice *> currentDevices;
    for (HotspotDeviceItem *item : m_items)
        currentDevices << item->device();
    for (WirelessDevice *device : devices) {
        if (currentDevices.contains(device))
            continue;
        m_items.append(new HotspotDeviceItem(device, this));
    }
    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    QList<WirelessDevice *> hotspotDevices = hotspotController->devices();
    std::sort(m_items.begin(), m_items.end(), [=](HotspotDeviceItem *item1, HotspotDeviceItem *item2) {
        return hotspotDevices.indexOf(item1->device()) < hotspotDevices.indexOf(item2->device());
    });
    updateVisiable();
}

void HotspotModule::onDeviceRemove(const QList<WirelessDevice *> &rmDevices)
{
    for (auto it = m_items.begin(); it != m_items.end();) {
        if (rmDevices.contains((*it)->device())) {
            QList<ModuleObject *> &modules = (*it)->modules();
            for (auto *&module : modules) {
                removeChild(module);
            }
            delete (*it);
            it = m_items.erase(it);
        } else {
            ++it;
        }
    }
    updateVisiable();
}

void HotspotModule::updateVisiable()
{
    int row = 0;
    bool onlyOne = m_items.size() == 1;
    for (HotspotDeviceItem *&item : m_items) {
        QList<ModuleObject *> &modules = item->modules();
        insertChild(row++, modules.at(0));
        insertChild(row++, modules.at(1));
        if (onlyOne) {
            removeChild(modules.at(2));
        } else {
            insertChild(row++, modules.at(2));
        }
    }
    emit updateItemOnlyOne(onlyOne);
}
