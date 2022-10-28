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
#include "trayicon.h"
#include "networkdialog.h"

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
    , m_trayIcon(Q_NULLPTR)
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

    if (!pluginIsDisable())
        loadPlugin();

    connect(m_networkDialog, &NetworkDialog::requestShow, this, &NetworkPlugin::showNetworkDialog);
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
    if (itemKey == NETWORK_KEY)
        return m_networkHelper->contextMenu(true);

    return QString();
}

QWidget *NetworkPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == NETWORK_KEY) {
        if (m_trayIcon.isNull()) {
            m_trayIcon.reset(new TrayIcon(m_networkHelper.data()));
        }
        return m_trayIcon.data();
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

QString NetworkPlugin::getConnectionName() const
{
    QList<NetworkDeviceBase *> devices = NetworkController::instance()->devices();
    for (NetworkDeviceBase *device : devices) {
        if (device->deviceType() == DeviceType::Wired) {
            WiredDevice *wiredDevice = static_cast<WiredDevice *>(device);
            if (wiredDevice->isConnected()) {
                QList<WiredConnection *> items = wiredDevice->items();
                for (WiredConnection *item : items) {
                    if (item->status() == ConnectionStatus::Activated)
                        return item->connection()->id();
                }
            }
        } else if (device->deviceType() == DeviceType::Wireless) {
            WirelessDevice *wirelessDevice = static_cast<WirelessDevice *>(device);
            if (wirelessDevice->isConnected()) {
                QList<WirelessConnection *> items = wirelessDevice->items();
                for (WirelessConnection *item : items) {
                    if (item->status() == ConnectionStatus::Activated)
                        return item->connection()->ssid();
                }
            }
        }
    }

    return QString();
}

void NetworkPlugin::onIconUpdated()
{
    // update quick panel
    m_proxyInter->updateDockInfo(this, DockPart::QuickPanel);
    // update quick plugin area
    m_proxyInter->updateDockInfo(this, DockPart::QuickShow);
}

QIcon NetworkPlugin::icon(const DockPart &)
{
    TrayIcon *trayIcon = static_cast<TrayIcon *>(itemWidget(NETWORK_KEY));
    QPixmap pixmap = trayIcon->pixmap();
    return QIcon(pixmap);
}

PluginsItemInterface::PluginStatus NetworkPlugin::status() const
{
    // get the plugin status
    PluginState plugState = m_networkHelper->getPluginState();
    switch (plugState) {
    case PluginState::Unknow:
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
    PluginState plugState = m_networkHelper->getPluginState();
    switch (plugState) {
    case PluginState::Disabled:
    case PluginState::WirelessDisabled:
    case PluginState::WiredDisabled:
            return tr("Device disabled");
    case PluginState::Unknow:
    case PluginState::Nocable:
            return tr("Network cable unplugged");
    case PluginState::Disconnected:
    case PluginState::WirelessDisconnected:
    case PluginState::WiredDisconnected:
            return tr("Not connected");
    case PluginState::Connecting:
    case PluginState::WirelessConnecting:
    case PluginState::WiredConnecting:
            return tr("Connecting");
    case PluginState::ConnectNoInternet:
    case PluginState::WirelessConnectNoInternet:
    case PluginState::WiredConnectNoInternet:
            return tr("Connected but no Internet access");
    case PluginState::Failed:
    case PluginState::WirelessFailed:
    case PluginState::WiredFailed:
            return tr("Connection failed");
    case PluginState::WiredIpConflicted:
    case PluginState::WirelessIpConflicted:
            return tr("IP conflict");
    default:
            break;
    }

    return getConnectionName();
}

void NetworkPlugin::showNetworkDialog()
{
    if (m_trayIcon.isNull() || m_networkDialog->panel()->isVisible())
        return;
    m_proxyInter->requestSetAppletVisible(this, NETWORK_KEY, true);
}
