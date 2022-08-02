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

#ifndef LOGIN_MODULE_H
#define LOGIN_MODULE_H

#include "tray_module_interface.h"
#include "../common-plugin/utils.h"

#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/WiredDevice>

NETWORKPLUGIN_BEGIN_NAMESPACE
class NetworkPluginHelper;
class NetworkDialog;
class SecretAgent;
NETWORKPLUGIN_END_NAMESPACE

namespace dss {
namespace module {

/**
 * @brief The NetworkModule class
 * 用于处理插件差异
 * NetworkModule处理信号槽有问题，固增加该类
 */
class NetworkModule : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void signalShowNetworkDialog();

public:
    explicit NetworkModule(QObject *parent = nullptr);

    QWidget *content();
    QWidget *itemWidget() const;
    QWidget *itemTipsWidget() const;
    const QString itemContextMenu() const;
    void invokedMenuItem(const QString &menuId, const bool checked) const;

public Q_SLOTS:
    void showNetworkDialog(QWidget *w) const;
    void updateLockScreenStatus(bool visible);
    void onDeviceStatusChanged(NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate, NetworkManager::Device::StateChangeReason reason);
    void onAddDevice(const QString &path);
    void onUserChanged(QString json);

private:
    void addFirstConnection(NetworkManager::WiredDevice *nmDevice);
    bool hasConnection(NetworkManager::WiredDevice *nmDevice, NetworkManager::Connection::List &unSaveDevices);
    const QString connectionMatchName() const;
    void installTranslator(QString locale);

public:
    NETWORKPLUGIN_NAMESPACE::NetworkPluginHelper *m_networkHelper;
    NETWORKPLUGIN_NAMESPACE::NetworkDialog *m_networkDialog;
    NETWORKPLUGIN_NAMESPACE::SecretAgent *m_secretAgent;

    bool m_isLockModel;  // 锁屏 or greeter
    bool m_isLockScreen; // 锁屏显示

    QSet<QString> m_devicePaths; // 记录无线设备Path,防止信号重复连接
    QString m_lastActiveWirelessDevicePath;
    QString m_lastConnection;
    NetworkManager::Device::State m_lastState;
    int m_clickTime;
};

class NetworkPlugin : public QObject
    , public TrayModuleInterface
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "com.deepin.dde.shell.Modules.Tray" FILE "network.json")
    Q_INTERFACES(dss::module::TrayModuleInterface)

public:
    explicit NetworkPlugin(QObject *parent = nullptr);
    ~NetworkPlugin() override { }
    void init() override;

    inline QString key() const override { return objectName(); }
    QWidget *content() override;
    inline QString icon() const override { return ":/wired/resources/wired/network-online-symbolic.svg"; }

    QWidget *itemWidget() const override;
    QWidget *itemTipsWidget() const override;
    const QString itemContextMenu() const override;
    void invokedMenuItem(const QString &menuId, const bool checked) const override;

private:
    void initUI();

private:
    NetworkModule *m_network;
};

} // namespace module
} // namespace dss
#endif // LOGIN_MODULE_H
