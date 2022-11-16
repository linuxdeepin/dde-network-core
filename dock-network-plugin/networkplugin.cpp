/*
 * Copyright (C) 2011 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
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

#include "networkplugin.h"
#include "networkpluginhelper.h"
#include "networkdialog.h"
#include "quickpanel.h"
#include "item/devicestatushandler.h"
#include "networkdialog/thememanager.h"

#include <DDBusSender>

#include <QTime>

#include <networkcontroller.h>
#include <networkdevicebase.h>
#include <wireddevice.h>
#include <wirelessdevice.h>

#define STATE_KEY "enabled"

NETWORKPLUGIN_USE_NAMESPACE

NetworkPlugin::NetworkPlugin(QObject *parent)
    : QObject(parent)
    , m_networkHelper(Q_NULLPTR)
    , m_networkDialog(Q_NULLPTR)
    , m_quickPanel(Q_NULLPTR)
    , m_clickTime(-10000)
{
    NetworkController::setIPConflictCheck(true);
    QTranslator *translator = new QTranslator(this);
    translator->load(QString("/usr/share/dock-network-plugin/translations/dock-network-plugin_%1.qm").arg(QLocale::system().name()));
    QCoreApplication::installTranslator(translator);
}

NetworkPlugin::~NetworkPlugin()
{
}

const QString NetworkPlugin::pluginName() const
{
    return "network";
}

const QString NetworkPlugin::pluginDisplayName() const
{
    return tr("Network");
}

void NetworkPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_networkHelper)
        return;

    m_networkDialog = new NetworkDialog(this);
    m_networkDialog->setServerName("dde-network-dialog" + QString::number(getuid()) + "dock");
    m_networkHelper.reset(new NetworkPluginHelper(m_networkDialog));
    connect(m_networkHelper.data(), &NetworkPluginHelper::iconChanged, this, &NetworkPlugin::onIconUpdated);
    m_quickPanel = new QuickPanel();

    if (!pluginIsDisable())
        loadPlugin();

    connect(m_networkDialog, &NetworkDialog::requestShow, this, &NetworkPlugin::showNetworkDialog);

    connect(m_quickPanel, &QuickPanel::iconClicked, this, [this]() {
        m_networkHelper->invokeMenuItem(m_quickPanel->userData().toString());
    });
    connect(m_quickPanel, &QuickPanel::panelClicked, this, &NetworkPlugin::showNetworkDialog);

    m_networkHelper->setIconDark(Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType);
    // 主题发生变化触发的信号
    connect(Dtk::Gui::DGuiApplicationHelper::instance(), &Dtk::Gui::DGuiApplicationHelper::themeTypeChanged, this, [this]() {
        m_networkHelper->setIconDark(Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType);
    });
}

void NetworkPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(checked)

    if (itemKey == NETWORK_KEY)
        m_networkHelper->invokeMenuItem(menuId);
}

void NetworkPlugin::refreshIcon(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY)
        emit m_networkHelper->viewUpdate();
}

void NetworkPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());

    refreshPluginItemsVisible();
}

bool NetworkPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, STATE_KEY, true).toBool();
}

const QString NetworkPlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey)
    if (m_networkHelper->needShowControlCenter()) {
        return QString("dbus-send --print-reply "
                       "--dest=com.deepin.dde.ControlCenter "
                       "/com/deepin/dde/ControlCenter "
                       "com.deepin.dde.ControlCenter.ShowModule "
                       "\"string:network\"");
    }

    return QString();
}

const QString NetworkPlugin::itemContextMenu(const QString &itemKey)
{
    return QString();
}

QWidget *NetworkPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == QUICK_ITEM_KEY) {
        return m_quickPanel;
    }
    return Q_NULLPTR;
}

QWidget *NetworkPlugin::itemTipsWidget(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY && !m_networkDialog->panel()->isVisible())
        return m_networkHelper->itemTips();

    return Q_NULLPTR;
}

QWidget *NetworkPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_networkDialog->panel();
}

int NetworkPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 3).toInt();
}

void NetworkPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

void NetworkPlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void NetworkPlugin::loadPlugin()
{
    m_proxyInter->itemAdded(this, NETWORK_KEY);
}

void NetworkPlugin::refreshPluginItemsVisible()
{
    if (pluginIsDisable())
        m_proxyInter->itemRemoved(this, NETWORK_KEY);
    else
        m_proxyInter->itemAdded(this, NETWORK_KEY);
}

void NetworkPlugin::updateQuickPanel()
{
    QList<NetworkDeviceBase *> devices = NetworkController::instance()->devices();
    int wiredConnectionCount = 0;
    int wirelessConnectionCount = 0;
    QString wiredConnection;
    QString wirelessConnection;
    QList<WiredDevice *> wiredList;
    QList<WirelessDevice *> wirelessList;

    for (NetworkDeviceBase *device : devices) {
        switch (device->deviceType()) {
        case DeviceType::Wired: {
            WiredDevice *wiredDevice = static_cast<WiredDevice *>(device);
            wiredList.append(wiredDevice);
            if (wiredDevice->isConnected()) {
                QList<WiredConnection *> items = wiredDevice->items();
                for (WiredConnection *item : items) {
                    if (item->status() == ConnectionStatus::Activated) {
                        wiredConnectionCount++;
                        wiredConnection = item->connection()->id();
                    }
                }
            }
        } break;
        case DeviceType::Wireless: {
            WirelessDevice *wirelessDevice = static_cast<WirelessDevice *>(device);
            wirelessList.append(wirelessDevice);
            if (wirelessDevice->isConnected()) {
                QList<WirelessConnection *> items = wirelessDevice->items();
                for (WirelessConnection *item : items) {
                    if (item->status() == ConnectionStatus::Activated) {
                        wirelessConnectionCount++;
                        wirelessConnection = item->connection()->ssid();
                    }
                }
            }
        } break;
        default:
            break;
        }
    }

    if (!wirelessList.isEmpty()) {
        NetDeviceStatus status = DeviceStatusHandler::wirelessStatus(wirelessList);
        updateQuickPanelDescription(status, wirelessConnectionCount, wirelessConnection, NetworkPluginHelper::MenuWirelessEnable);
        m_quickPanel->setText(tr("Wireless Network"));
        m_quickPanel->setIcon(QIcon::fromTheme(ThemeManager::instance()->getIcon("wireless-80-symbolic")));
    } else if (wiredList.isEmpty()) {
        NetDeviceStatus status = DeviceStatusHandler::wiredStatus(wiredList);
        updateQuickPanelDescription(status, wiredConnectionCount, wiredConnection, NetworkPluginHelper::MenuWiredEnable);
        m_quickPanel->setText(tr("Wired Network"));
        m_quickPanel->setIcon(QIcon::fromTheme(ThemeManager::instance()->getIcon("network-wired-symbolic")));
    } else {
        m_quickPanel->setText(pluginDisplayName());
        m_quickPanel->setDescription(description());
        m_quickPanel->setActive(false);
        m_quickPanel->setUserData(NetworkPluginHelper::MenuSettings);
        m_quickPanel->setIcon(QIcon::fromTheme(ThemeManager::instance()->getIcon("wireless-disconnect")));
    }
}

void NetworkPlugin::updateQuickPanelDescription(NetDeviceStatus status, int connectionCount, const QString &Connection, int enableMenu)
{
    QString statusName = networkStateName(status);
    bool isEnabled = (status != NetDeviceStatus::Disabled
                      && status != NetDeviceStatus::Unknown
                      && status != NetDeviceStatus::Nocable);

    if (statusName.isEmpty() && connectionCount != 0) {
        if (connectionCount == 1) {
            m_quickPanel->setDescription(Connection);
        } else {
            m_quickPanel->setDescription(tr("Connected") + QString(" (%1)").arg(connectionCount));
        }
    } else {
        m_quickPanel->setDescription(statusName);
    }
    m_quickPanel->setActive(isEnabled);
    m_quickPanel->setUserData(isEnabled ? (enableMenu + 1) : enableMenu);
}

QString NetworkPlugin::networkStateName(NetDeviceStatus status) const
{
    switch (status) {
    case NetDeviceStatus::Disabled:
        return tr("Device disabled");
    case NetDeviceStatus::Unknown:
    case NetDeviceStatus::Nocable:
        return tr("Network cable unplugged");
    case NetDeviceStatus::Disconnected:
        return tr("Not connected");
    case NetDeviceStatus::Connecting:
    case NetDeviceStatus::Authenticating:
        return tr("Connecting");
    case NetDeviceStatus::ObtainingIP:
    case NetDeviceStatus::ObtainIpFailed:
        return tr("Obtaining address");
    case NetDeviceStatus::ConnectNoInternet:
        return tr("Connected but no Internet access");
    case NetDeviceStatus::IpConflicted:
        return tr("IP conflict");
    case NetDeviceStatus::ConnectFailed:
        return tr("Connection failed");
    default:
        break;
    }
    return QString();
}

void NetworkPlugin::onIconUpdated()
{
    // update quick panel
    m_proxyInter->updateDockInfo(this, DockPart::QuickPanel);
    // update quick plugin area
    m_proxyInter->updateDockInfo(this, DockPart::QuickShow);

    updateQuickPanel();
}

QIcon NetworkPlugin::icon(const DockPart &dockPart)
{
    switch (dockPart) {
    case DockPart::QuickShow:
        return *m_networkHelper->trayIcon();
    default:
        break;
    }
    return PluginsItemInterface::icon(dockPart);
}

PluginsItemInterface::PluginStatus NetworkPlugin::status() const
{
    // get the plugin status
    PluginState plugState = m_networkHelper->getPluginState();
    switch (plugState) {
    case PluginState::Unknown:
    case PluginState::Disabled:
    case PluginState::Nocable:
        return PluginStatus::Disabled;
    default:
        break;
    }

    return PluginStatus::Active;
}

QString NetworkPlugin::description() const
{
    return m_quickPanel ? m_quickPanel->description() : QString();
}

void NetworkPlugin::showNetworkDialog()
{
    if (m_networkDialog->panel()->isVisible())
        return;
    m_proxyInter->requestSetAppletVisible(this, NETWORK_KEY, true);
}
