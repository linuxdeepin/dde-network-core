/*
* Copyright (C) 2021 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     Zhang Qipeng <zhangqipeng@uniontech.com>
*
* Maintainer: Zhang Qipeng <zhangqipeng@uniontech.com>
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

#include "network_module.h"
#include "networkpluginhelper.h"
#include "networkdialog.h"
#include "trayicon.h"
#include "notificationmanager.h"

#include <QWidget>
#include <QTime>

#include <DArrowRectangle>

#include <networkcontroller.h>
#include <networkdevicebase.h>

#include <com_deepin_daemon_accounts_user.h>

#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/WiredDevice>
#include <NetworkManagerQt/Settings>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/WiredSetting>

#define NETWORK_KEY "network-item-key"

using namespace NetworkManager;
NETWORKPLUGIN_USE_NAMESPACE

namespace dss {
namespace module {

NetworkModule::NetworkModule(QObject *parent)
    : QObject(parent)
    , m_lastState(NetworkManager::Device::State::UnknownState)
{
    QDBusConnection::sessionBus().connect("com.deepin.dde.lockFront", "/com/deepin/dde/lockFront", "com.deepin.dde.lockFront", "Visible", this, SLOT(updateLockScreenStatus(bool)));
    m_isLockModel = (-1 == qAppName().indexOf("greeter"));
    if (!m_isLockModel) {
        dde::network::NetworkController::setServiceType(dde::network::ServiceLoadType::LoadFromManager);
    }

    m_networkDialog = new NetworkDialog(this);
    m_networkDialog->setRunReason(NetworkDialog::Lock);
    m_networkHelper = new NetworkPluginHelper(m_networkDialog, this);

    installTranslator(QLocale::system().name());
    if (!m_isLockModel) {
        QDBusMessage lock = QDBusMessage::createMethodCall("com.deepin.dde.LockService", "/com/deepin/dde/LockService", "com.deepin.dde.LockService", "CurrentUser");
        QDBusConnection::systemBus().callWithCallback(lock, this, SLOT(onUserChanged(QString)));
        QDBusConnection::systemBus().connect("com.deepin.dde.LockService", "/com/deepin/dde/LockService", "com.deepin.dde.LockService", "UserChanged", this, SLOT(onUserChanged(QString)));

        m_networkDialog->setRunReason(NetworkDialog::Greeter);
        connect(m_networkHelper, &NetworkPluginHelper::addDevice, this, &NetworkModule::onAddDevice);
        for (dde::network::NetworkDeviceBase *device : dde::network::NetworkController::instance()->devices()) {
            onAddDevice(device->path());
        }
    }
    m_networkDialog->runServer(true);
}

QWidget *NetworkModule::content()
{
    int msec = QTime::currentTime().msecsSinceStartOfDay();
    if (!m_networkDialog->isVisible() && abs(msec - m_clickTime) > 200) {
        m_clickTime = msec;
        emit signalShowNetworkDialog();
        m_networkDialog->show();
    }
    return nullptr;
}

QWidget *NetworkModule::itemWidget() const
{
    TrayIcon *trayIcon = new TrayIcon(m_networkHelper);
    trayIcon->setGreeterStyle(true);
    trayIcon->installEventFilter(m_networkDialog);
    connect(this, &NetworkModule::signalShowNetworkDialog, trayIcon, &TrayIcon::showNetworkDialog);
    connect(trayIcon, &TrayIcon::signalShowNetworkDialog, this, &NetworkModule::showNetworkDialog);
    connect(m_networkDialog, &NetworkDialog::requestPosition, trayIcon, &TrayIcon::showNetworkDialog);
    // ????????????
    connect(m_networkHelper, &NetworkPluginHelper::destroyed, trayIcon, &TrayIcon::deleteLater);
    return trayIcon;
}

QWidget *NetworkModule::itemTipsWidget() const
{
    if (m_networkDialog->isVisible())
        return nullptr;
    QWidget *itemTips = m_networkHelper->itemTips();
    if (itemTips) {
        QPalette palette = itemTips->palette();
        palette.setColor(QPalette::BrightText, Qt::black);
        itemTips->setPalette(palette);
    }
    return itemTips;
}

const QString NetworkModule::itemContextMenu() const
{
    return m_networkHelper->contextMenu(false);
}

void NetworkModule::invokedMenuItem(const QString &menuId, const bool checked) const
{
    Q_UNUSED(checked);
    m_networkHelper->invokeMenuItem(menuId);
}

void NetworkModule::showNetworkDialog(QWidget *w) const
{
    QPoint point = w->mapToGlobal(QPoint(w->width() / 2, 0));
    m_networkDialog->setPosition(point.x(), point.y(), Dtk::Widget::DArrowRectangle::ArrowBottom);
}

void NetworkModule::updateLockScreenStatus(bool visible)
{
    m_isLockModel = true;
    m_isLockScreen = visible;
    m_networkDialog->runServer(visible);
}

void NetworkModule::onAddDevice(const QString &devicePath)
{
    if (m_isLockModel) {
        return;
    }
    // ??????????????????????????????????????????????????????????????????
    if (!m_devicePaths.contains(devicePath)) {
        Device::Ptr device(new Device(devicePath));
        Device *nmDevice = nullptr;
        if (device->type() == Device::Wifi) {
            NetworkManager::WirelessDevice *wDevice = new NetworkManager::WirelessDevice(devicePath, this);
            nmDevice = wDevice;
            connect(wDevice, &NetworkManager::WirelessDevice::activeAccessPointChanged, this, [this](const QString &ap) {
                m_lastActiveWirelessDevicePath = static_cast<NetworkManager::WirelessDevice *>(sender())->uni() + NetworkManager::AccessPoint(ap).ssid();
            });
        } else if (device->type() == Device::Ethernet) {
            NetworkManager::WiredDevice *wDevice = new NetworkManager::WiredDevice(devicePath, this);
            nmDevice = wDevice;
            addFirstConnection(wDevice);
            connect(wDevice, &NetworkManager::WiredDevice::availableConnectionAppeared, this, [ this ] (const QString &) {
                NetworkManager::WiredDevice *device = qobject_cast<NetworkManager::WiredDevice *>(sender());
                addFirstConnection(device);
            });
        }
        if (nmDevice) {
            connect(nmDevice, &NetworkManager::Device::stateChanged, this, &NetworkModule::onDeviceStatusChanged);
            m_devicePaths.insert(devicePath);
        }
    }
}

void NetworkModule::onUserChanged(QString json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject())
        return;
    int uid = doc.object().value("Uid").toInt();
    com::deepin::daemon::accounts::User user("com.deepin.daemon.Accounts", QString("/com/deepin/daemon/Accounts/User%1").arg(uid), QDBusConnection::systemBus());
    installTranslator(user.locale().split(".").first());
}

void NetworkModule::installTranslator(QString locale)
{
    static QTranslator translator;
    static QString localTmp;
    if (localTmp == locale) {
        return;
    }
    localTmp = locale;
    m_networkDialog->setLocale(locale);
    QApplication::removeTranslator(&translator);
    translator.load(QString("/usr/share/dss-network-plugin/translations/dss-network-plugin_%1.qm").arg(locale));
    QApplication::installTranslator(&translator);
    dde::network::NetworkController::instance()->retranslate();
    m_networkHelper->updateTooltips();
}

const QString NetworkModule::connectionMatchName() const
{
    NetworkManager::Connection::List connList = listConnections();
    QStringList connNameList;
    int connSuffixNum = 1;

    for (NetworkManager::Connection::Ptr conn : connList) {
        if (conn->settings()->connectionType() == ConnectionSettings::ConnectionType::Wired)
            connNameList.append(conn->name());
    }

    QString matchConnName = tr("Wired Connection");
    if (!connNameList.contains(matchConnName))
        return matchConnName;

    matchConnName = QString(tr("Wired Connection")) + QString(" %1");
    for (int i = 1; i <= connNameList.size(); ++i) {
        if (!connNameList.contains(matchConnName.arg(i))) {
            connSuffixNum = i;
            break;
        }
        if (i == connNameList.size())
            connSuffixNum = i + 1;
    }

    return matchConnName.arg(connSuffixNum);
}

bool NetworkModule::hasConnection(NetworkManager::WiredDevice *nmDevice, NetworkManager::Connection::List &unSaveDevices)
{
    bool connIsEmpty = false;
    // ???????????????????????????,??????????????????,????????????????????????MAC??????????????????????????????????????????MAC????????????????????????????????????????????????
    // ?????????????????????????????????????????????????????????????????????availableConnections?????????????????????????????????????????????????????????(??????????????????)???
    nmDevice->availableConnections();
    NetworkManager::Connection::List connList = listConnections();
    for (NetworkManager::Connection::Ptr conn : connList) {
        WiredSetting::Ptr settings = conn->settings()->setting(Setting::Wired).staticCast<WiredSetting>();
        // ?????????????????????MAC???????????????????????????MAC??????????????????????????????MAC????????????????????????????????????????????????
        if (settings.isNull() || (!settings->macAddress().isEmpty() && nmDevice->hardwareAddress().compare(settings->macAddress().toHex(':'), Qt::CaseInsensitive) != 0))
            continue;

        // ???????????????????????????????????????????????????????????????
        if (conn->isUnsaved()) {
            unSaveDevices << conn;
            continue;
        }

        connIsEmpty = true;
    }

    return connIsEmpty;
}

void NetworkModule::addFirstConnection(NetworkManager::WiredDevice *nmDevice)
{
    // ???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
    NetworkManager::Connection::List unSaveConnections;
    bool findConnection = hasConnection(nmDevice, unSaveConnections);
    // ????????????????????????????????????????????????
    bool isRemoved = !unSaveConnections.isEmpty();
    for (NetworkManager::Connection::Ptr conn : unSaveConnections)
        conn->remove();

    static bool connectionCreated = false;
    // ??????????????????????????????,???????????????????????????,???????????????????????????????????????????????????
    if (connectionCreated)
        return;

    connectionCreated = true;
    auto autoCreateConnection = [ & ]() {
        // ??????????????????????????????????????????,????????????????????????????????????????????????
        ConnectionSettings::Ptr conn(new ConnectionSettings);
        conn->setId(connectionMatchName());
        conn->setUuid("");
        NetworkManager::addConnection(conn->toMap());
    };

    if (!findConnection) {
        if (isRemoved) {
            // ????????????????????????????????????1??????????????????
            QTimer::singleShot(1000, this, [ = ] {
                autoCreateConnection();
            });
        } else {
            autoCreateConnection();
        }
    }
}

void NetworkModule::onDeviceStatusChanged(NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate, NetworkManager::Device::StateChangeReason reason)
{
    if (m_isLockModel) {
        return;
    }
    NetworkManager::Device *device = static_cast<NetworkManager::Device *>(sender());
    NetworkManager::ActiveConnection::Ptr conn = device->activeConnection();
    if (!conn.isNull()) {
        m_lastConnection = conn->id();
        m_lastState = newstate;
    } else if (m_lastState != oldstate || m_lastConnection.isEmpty()) {
        m_lastConnection.clear();
        return;
    }
    switch (newstate) {
    case Device::State::Preparing: { // ????????????
        if (oldstate == Device::State::Disconnected) {
            switch (device->type()) {
            case Device::Type::Ethernet:
                NotificationManager::NetworkNotify(NotificationManager::WiredConnecting, m_lastConnection);
                break;
            case Device::Type::Wifi:
                NotificationManager::NetworkNotify(NotificationManager::WirelessConnecting, m_lastConnection);
                break;
            default:
                break;
            }
        }
    } break;
    case Device::State::Activated: { // ????????????
        switch (device->type()) {
        case Device::Type::Ethernet:
            NotificationManager::NetworkNotify(NotificationManager::WiredConnected, m_lastConnection);
            break;
        case Device::Type::Wifi:
            NotificationManager::NetworkNotify(NotificationManager::WirelessConnected, m_lastConnection);
            break;
        default:
            break;
        }
    } break;
    case Device::State::Failed:
    case Device::State::Disconnected:
    case Device::State::NeedAuth:
    case Device::State::Unmanaged:
    case Device::State::Unavailable: {
        if (reason == Device::StateChangeReason::DeviceRemovedReason) {
            return;
        }

        // ignore if device's old state is not available
        if (oldstate <= Device::State::Unavailable) {
            qDebug("no notify, old state is not available");
            return;
        }

        // ignore invalid reasons
        if (reason == Device::StateChangeReason::UnknownReason) {
            qDebug("no notify, device state reason invalid");
            return;
        }

        switch (reason) {
        case Device::StateChangeReason::UserRequestedReason:
            if (newstate == Device::State::Disconnected) {
                switch (device->type()) {
                case Device::Type::Ethernet:
                    NotificationManager::NetworkNotify(NotificationManager::WiredDisconnected, m_lastConnection);
                    break;
                case Device::Type::Wifi:
                    NotificationManager::NetworkNotify(NotificationManager::WirelessDisconnected, m_lastConnection);
                    break;
                default:
                    break;
                }
            }
            break;
        case Device::StateChangeReason::ConfigUnavailableReason:
        case Device::StateChangeReason::AuthSupplicantTimeoutReason: // ??????
            switch (device->type()) {
            case Device::Type::Ethernet:
                NotificationManager::NetworkNotify(NotificationManager::WiredUnableConnect, m_lastConnection);
                break;
            case Device::Type::Wifi:
                NotificationManager::NetworkNotify(NotificationManager::WirelessUnableConnect, m_lastConnection);
                break;
            default:
                break;
            }
            break;
        case Device::StateChangeReason::AuthSupplicantDisconnectReason:
            if (oldstate == Device::State::ConfiguringHardware && newstate == Device::State::NeedAuth) {
                switch (device->type()) {
                case Device::Type::Ethernet:
                    NotificationManager::NetworkNotify(NotificationManager::WiredConnectionFailed, m_lastConnection);
                    break;
                case Device::Type::Wifi:
                    NotificationManager::NetworkNotify(NotificationManager::WirelessConnectionFailed, m_lastConnection);
                    emit signalShowNetworkDialog();
                    m_networkDialog->setConnectWireless(device->uni(), m_lastConnection);
                    break;
                default:
                    break;
                }
            }
            break;
        case Device::StateChangeReason::CarrierReason:
            if (device->type() == Device::Ethernet) {
                qDebug("unplugged device is ethernet");
                NotificationManager::NetworkNotify(NotificationManager::WiredDisconnected, m_lastConnection);
            }
            break;
        case Device::StateChangeReason::NoSecretsReason:
            NotificationManager::NetworkNotify(NotificationManager::NoSecrets, m_lastConnection);
            emit signalShowNetworkDialog();
            m_networkDialog->setConnectWireless(device->uni(), m_lastConnection);
            break;
        case Device::StateChangeReason::SsidNotFound:
            NotificationManager::NetworkNotify(NotificationManager::SsidNotFound, m_lastConnection);
            break;
        default:
            break;
        }
    } break;
    default:
        break;
    }
}

NetworkPlugin::NetworkPlugin(QObject *parent)
    : QObject(parent)
    , m_network(nullptr)
{
    setObjectName(QStringLiteral(NETWORK_KEY));
}

void NetworkPlugin::init()
{
    initUI();
}

QWidget *NetworkPlugin::content()
{
    return m_network->content();
}

QWidget *NetworkPlugin::itemWidget() const
{
    return m_network->itemWidget();
}

QWidget *NetworkPlugin::itemTipsWidget() const
{
    return m_network->itemTipsWidget();
}

const QString NetworkPlugin::itemContextMenu() const
{
    return m_network->itemContextMenu();
}

void NetworkPlugin::invokedMenuItem(const QString &menuId, const bool checked) const
{
    m_network->invokedMenuItem(menuId, checked);
}

void NetworkPlugin::initUI()
{
    if (m_network) {
        return;
    }

    m_network = new NetworkModule(this);
}

} // namespace module
} // namespace dss
