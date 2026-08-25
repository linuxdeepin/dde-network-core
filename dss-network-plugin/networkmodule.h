// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#include "tray_module_interface.h"

namespace dde {
namespace network {

class NetManager;
class NetView;
class NetStatus;

/**
 * @brief The NetworkModule class
 * 用于处理插件差异
 * NetworkModule处理信号槽有问题，固增加该类
 */
class NetworkModule : public QObject
{
    Q_OBJECT

public:
    explicit NetworkModule(QObject *parent = nullptr);
    ~NetworkModule() override;

    virtual QWidget *content();
    virtual QWidget *itemWidget();
    virtual QWidget *itemTipsWidget() const;
    virtual const QString itemContextMenu() const;
    virtual void invokedMenuItem(const QString &menuId, const bool checked) const;
    void installTranslator(const QString &locale);

    inline dde::network::NetStatus *netStatus() const { return m_netStatus; }

Q_SIGNALS:
    void requestShow();
    void userChanged();

protected Q_SLOTS:
    virtual void updateLockScreenStatus(bool visible);
    void onUserChanged(const QString &json);
    void onNotify(uint replacesId);

protected:
    QWidget *m_contentWidget;
    dde::network::NetManager *m_manager;
    dde::network::NetView *m_netView;
    dde::network::NetStatus *m_netStatus;

    bool m_isLockModel;  // 锁屏 or greeter
    bool m_isLockScreen; // 锁屏显示
    uint m_replacesId;

    QSet<QString> m_devicePaths; // 记录无线设备Path,防止信号重复连接
    QString m_lastActiveWirelessDevicePath;
    QString m_lastConnection;
    QString m_lastConnectionUuid;
};

class NetworkPlugin : public QObject, public dss::module::TrayModuleInterface
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

    inline QString icon() const override { return "network-online-symbolic"; }

    QWidget *itemWidget() const override;
    QWidget *itemTipsWidget() const override;
    const QString itemContextMenu() const override;
    void invokedMenuItem(const QString &menuId, const bool checked) const override;
    void setMessageCallback(dss::module::MessageCallbackFunc messageCallback) override;

    void setAppData(dss::module::AppDataPtr appData) override { m_appData = appData; }

    void requestShowContent();
    void setMessage(bool visible);
    QString message(const QString &msgData) override;

Q_SIGNALS:
    void notifyNetworkStateChanged(bool);

private:
    void initUI();

private Q_SLOTS:
    void onDevicesChanged();
    void onPrepareForSleep(bool sleep);

private:
    NetworkModule *m_network;
    dss::module::MessageCallbackFunc m_messageCallback;
    dss::module::AppDataPtr m_appData;
};

} // namespace network
} // namespace dde
#endif // NETWORKMODULE_H
