/*
 * Copyright (C) 2011 ~ 2021 Deepin Technology Co., Ltd.
 *
 * Author:     donghualin <donghualin@uniontech.com>
 *
 * Maintainer: donghualin <donghualin@uniontech.com>
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

#ifndef NETWORKPLUGINHELPER_H
#define NETWORKPLUGINHELPER_H
#include "utils.h"
#include "item/devicestatushandler.h"

#include <DGuiApplicationHelper>

#include <NetworkManagerQt/Device>


DGUI_USE_NAMESPACE

namespace dde {
namespace network {
enum class DeviceType;
class NetworkDeviceBase;
class AccessPoints;
} // namespace network
} // namespace dde

class QTimer;
class NetItem;

namespace dde {
namespace networkplugin {

class NetworkDialog;
class TipsWidget;

class NetworkPluginHelper : public QObject
{
    Q_OBJECT

public:
    enum MenuItemKey : int {
        MenuSettings = 1,
        MenuEnable,
        MenuDisable,
        MenuWiredEnable,
        MenuWiredDisable,
        MenuWirelessEnable,
        MenuWirelessDisable,
    };

Q_SIGNALS:
    void sendIpConflictDect(int);
    void addDevice(const QString &devicePath);
    void viewUpdate();
    void iconChanged();

public:
    explicit NetworkPluginHelper(NetworkDialog *networkDialog, QObject *parent = Q_NULLPTR);
    ~NetworkPluginHelper();

    void invokeMenuItem(const QString &menuId);
    bool needShowControlCenter();
    const QString contextMenu(bool hasSetting) const;
    QWidget *itemTips();
    PluginState getPluginState();
    void updatePluginState();
    void updateTooltips(); // 更新提示的内容

    QIcon *trayIcon() const;
    QIcon icon(int colorType) const;
    void refreshIcon();
    void setIconDark(bool isDark);

private:
    void initUi();
    void initConnection();
    bool deviceEnabled(const DeviceType &deviceType) const;
    void setDeviceEnabled(const DeviceType &deviceType, bool enabeld);
    bool wirelessIsActive() const;
    QString getStrengthStateString(int strength) const;
    dde::network::AccessPoints *getStrongestAp() const;
    dde::network::AccessPoints *getConnectedAp() const;

    int deviceCount(const DeviceType &devType) const;
    QList<QPair<QString, QStringList>> ipTipsMessage(const DeviceType &devType);

private Q_SLOTS:
    void onDeviceAdded(QList<NetworkDeviceBase *> devices);
    void onUpdatePlugView();
    void onActiveConnectionChanged();

private:
    PluginState m_pluginState;

    TipsWidget *m_tipsWidget;
    bool m_switchWire;
    QPixmap m_iconPixmap;

    QSet<QString> m_devicePaths; // 记录无线设备Path,防止信号重复连接
    QString m_lastActiveWirelessDevicePath;
    NetworkDialog *m_networkDialog;
    QIcon *m_trayIcon;
    QTimer *m_refreshIconTimer;
    bool m_isDarkIcon; // 是深色主题
};
}
}

#endif // NETWORKPLUGINHELPER_H
