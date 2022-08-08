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
#include "wirelessmodule.h"
#include "wirelessdevicemodel.h"
#include "widgets/widgetmodule.h"
#include "widgets/settingsitem.h"
#include "widgets/switchwidget.h"
#include "widgets/settingsgroup.h"
#include "editpage/connectionwirelesseditpage.h"

#include <DFontSizeManager>
#include <DListView>

#include <QPushButton>
#include <QApplication>
#include <QScroller>
#include <QLabel>

#include <wirelessdevice.h>
#include <networkcontroller.h>
#include <hotspotcontroller.h>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/wirelesssetting.h>
#include <networkmanagerqt/setting.h>

using namespace dde::network;
DCC_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

WirelessModule::WirelessModule(WirelessDevice *dev, QObject *parent)
    : PageModule("Wireless", tr("Wireless"), tr("Wireless"), QIcon::fromTheme("dcc_wifi"), parent)
    , m_device(dev)
{
    m_modules.append(new WidgetModule<SwitchWidget>("wireless_adapter", tr("Wireless Network Adapter"), [this](SwitchWidget *devEnabled) {
        QLabel *lblTitle = new QLabel(tr("Wireless Network Adapter")); // 无线网卡
        DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
        devEnabled->setLeftWidget(lblTitle);
        devEnabled->setChecked(m_device->isEnabled());
        connect(devEnabled, &SwitchWidget::checkedChanged, this, &WirelessModule::onNetworkAdapterChanged);
        connect(m_device, &WirelessDevice::enableChanged, devEnabled, [devEnabled](const bool enabled) {
            devEnabled->blockSignals(true);
            devEnabled->setChecked(enabled);
            devEnabled->blockSignals(false);
        });
    }));
    m_modules.append(new WidgetModule<DListView>("List_wirelesslist", tr("Wireless List"), this, &WirelessModule::initWirelessList));
    m_modules.append(new WidgetModule<SettingsGroup>("hotspot_tips", tr("Disable hotspot first if you want to connect to a wireless network"), [](SettingsGroup *tipsGroup) {
        QLabel *tips = new QLabel;
        tips->setAlignment(Qt::AlignCenter);
        tips->setWordWrap(true);
        tips->setText(tr("Disable hotspot first if you want to connect to a wireless network"));
        tipsGroup->insertWidget(tips);
    }));
    m_modules.append(new WidgetModule<QPushButton>("close_hotspot", tr("Close Hotspot"), [this](QPushButton *closeHotspotBtn) {
        closeHotspotBtn->setText(tr("Close Hotspot"));
        connect(closeHotspotBtn, &QPushButton::clicked, this, [=] {
            // 此处抛出这个信号是为了让外面记录当前关闭热点的设备，因为在关闭热点过程中，当前设备会移除一次，然后又会添加一次，相当于触发了两次信号，
            // 此时外面就会默认选中第一个设备而无法选中当前设备，因此在此处抛出信号是为了让外面能记住当前选择的设备
            NetworkController::instance()->hotspotController()->setEnabled(m_device, false);
        });
    }));
    m_modules.append(new WidgetModule<QWidget>); // 加个空的保证下面有弹簧
    updateVisible();
    connect(m_device, &WirelessDevice::enableChanged, this, &WirelessModule::updateVisible);
    connect(m_device, &WirelessDevice::hotspotEnableChanged, this, &WirelessModule::updateVisible);
}

void WirelessModule::initWirelessList(DListView *lvAP)
{
    lvAP->setAccessibleName("List_wirelesslist");
    WirelessDeviceModel *model = new WirelessDeviceModel(m_device, lvAP);
    lvAP->setModel(model);
    lvAP->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lvAP->setBackgroundType(DStyledItemDelegate::BackgroundType::ClipCornerBackground);
    lvAP->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    lvAP->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lvAP->setSelectionMode(QAbstractItemView::NoSelection);
    auto adjustHeight = [lvAP]() {
        lvAP->setMinimumHeight(lvAP->model()->rowCount() * 41);
    };
    adjustHeight();
    connect(model, &WirelessDeviceModel::modelReset, lvAP, adjustHeight);
    connect(model, &WirelessDeviceModel::detailClick, this, &WirelessModule::onApWidgetEditRequested);
    QScroller *scroller = QScroller::scroller(lvAP->viewport());
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    scroller->setScrollerProperties(sp);

    connect(lvAP, &DListView::clicked, this, [this](const QModelIndex &index) {
        AccessPoints *ap = static_cast<AccessPoints *>(index.internalPointer());
        if (!ap) { // nullptr 为隐藏网络
            onApWidgetEditRequested(nullptr);
            return;
        }
        if (ap->connected())
            return;
        m_device->connectNetwork(ap);
    });
}

void WirelessModule::updateVisible()
{
    bool devEnable = m_device->isEnabled();
    bool hotspotEnable = devEnable && m_device->hotspotEnabled();
    int i = 0;
    for (ModuleObject *module : m_modules) {
        if (module->name() == "List_wirelesslist") {
            if (devEnable && !hotspotEnable)
                insertChild(i++, module);
            else
                removeChild(module);
        } else if (module->name() == "hotspot_tips") {
            if (hotspotEnable)
                insertChild(i++, module);
            else
                removeChild(module);
        } else if (module->name() == "close_hotspot") {
            if (hotspotEnable)
                insertChild(i++, module);
            else
                removeChild(module);
        } else {
            insertChild(i++, module);
        }
    }
}
void WirelessModule::onNetworkAdapterChanged(bool checked)
{
    m_device->setEnabled(checked);
    if (checked) {
        m_device->scanNetwork();
    }
}

void WirelessModule::onApWidgetEditRequested(AccessPoints *ap)
{
    QString uuid;
    QString apPath;
    QString ssid;
    bool isHidden = true;
    if (ap) {
        ssid = ap->ssid();
        apPath = ap->path();
        isHidden = ap->hidden();

        for (auto conn : NetworkManager::activeConnections()) {
            if (conn->type() != NetworkManager::ConnectionSettings::ConnectionType::Wireless || conn->id() != ssid)
                continue;

            NetworkManager::ConnectionSettings::Ptr connSettings = conn->connection()->settings();
            NetworkManager::WirelessSetting::Ptr wSetting = connSettings->setting(NetworkManager::Setting::SettingType::Wireless).staticCast<NetworkManager::WirelessSetting>();
            if (wSetting.isNull())
                continue;

            QString settingMacAddress = wSetting->macAddress().toHex().toUpper();
            QString deviceMacAddress = m_device->realHwAdr().remove(":");
            if (!settingMacAddress.isEmpty() && settingMacAddress != deviceMacAddress)
                continue;

            uuid = conn->uuid();
            break;
        }
        if (uuid.isEmpty()) {
            const QList<WirelessConnection *> lstConnections = m_device->items();
            for (auto item : lstConnections) {
                if (item->connection()->ssid() != ssid)
                    continue;

                uuid = item->connection()->uuid();
                if (!uuid.isEmpty()) {
                    break;
                }
            }
        }
    }
    ConnectionWirelessEditPage *m_apEditPage = new ConnectionWirelessEditPage(m_device->path(), uuid, apPath, isHidden, qApp->activeWindow());

    connect(m_apEditPage, &ConnectionWirelessEditPage::disconnect, this, [this] {
        m_device->disconnectNetwork();
    });

    if (!uuid.isEmpty() || !ap) {
        //        m_editingUuid = uuid;
        m_apEditPage->initSettingsWidget();
    } else {
        m_apEditPage->initSettingsWidgetFromAp();
    }
    m_apEditPage->setLeftButtonEnable(true);

    auto devChange = [this, m_apEditPage]() {
        bool devEnable = m_device->isEnabled();
        bool hotspotEnable = devEnable && m_device->hotspotEnabled();
        if (!devEnable || hotspotEnable) {
            m_apEditPage->close();
        }
    };

    connect(m_device, &WirelessDevice::enableChanged, m_apEditPage, devChange);
    connect(m_device, &WirelessDevice::hotspotEnableChanged, m_apEditPage, devChange);

    m_apEditPage->exec();
    m_apEditPage->deleteLater();
}
