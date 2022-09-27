// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKINTERFACE_H
#define NETWORKINTERFACE_H

#include <QObject>
#include "interface/namespace.h"
#include "interface/moduleinterface.h"
#include "interface/frameproxyinterface.h"
#include "com_deepin_daemon_network.h"
#include "com_deepin_daemon_bluetooth.h"
#include "dtkcore_global.h"
#include <com_deepin_daemon_airplanemode.h>

class NetworkModuleWidget;
class WirelessPage;
class ConnectionEditPage;
DCORE_BEGIN_NAMESPACE
class DConfig;
DCORE_END_NAMESPACE

namespace dde {
  namespace network {
    class NetworkDeviceBase;
  }
}

namespace DCC_NAMESPACE {
  class ModuleInterface;
  class FrameProxyInterface;
}

enum class PageType;

using DBusAirplaneMode = com::deepin::daemon::AirplaneMode;
using NetworkInter = com::deepin::daemon::Network;
using BluetoothInter = com::deepin::daemon::Bluetooth;

using namespace DCC_NAMESPACE;
using namespace dde::network;

class DCCNetworkModule : public QObject, public ModuleInterface
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID ModuleInterface_iid FILE "network.json")
    Q_INTERFACES(DCC_NAMESPACE::ModuleInterface)

signals:
    void deviceChanged();
    void popAirplaneModePage();

public:
    explicit DCCNetworkModule();

    ~DCCNetworkModule() Q_DECL_OVERRIDE;

    void preInitialize(bool sync = false,FrameProxyInterface::PushType = FrameProxyInterface::PushType::Normal) Q_DECL_OVERRIDE;
    // 初始化函数
    void initialize() Q_DECL_OVERRIDE;

    void active() Q_DECL_OVERRIDE;

    QStringList availPage() const Q_DECL_OVERRIDE;
    // 返回插件名称，用于显示
    const QString displayName() const Q_DECL_OVERRIDE;

    // 返回插件图标，用于显示
    QIcon icon() const Q_DECL_OVERRIDE;

    // 返回插件翻译文件路径，用于搜索跳转
    QString translationPath() const Q_DECL_OVERRIDE;

    // 告知控制中心插件级别（一级模块还是二级菜单），必须实现
    QString path() const Q_DECL_OVERRIDE;

    // 告知控制中心插件插入位置，必须实现
    QString follow() const Q_DECL_OVERRIDE;

    const QString name() const Q_DECL_OVERRIDE;

    void showPage(const QString &pageName) Q_DECL_OVERRIDE;

    QWidget *moduleWidget() Q_DECL_OVERRIDE;

    int load(const QString &path) Q_DECL_OVERRIDE;

    void addChildPageTrans() const Q_DECL_OVERRIDE;

    void initSearchData() Q_DECL_OVERRIDE;

private:
    void removeConnEditPageByDevice(NetworkDeviceBase *dev);
    void initListConfig();
    bool hasModule(const PageType &type);
    bool supportAirplaneMode() const;
    bool getAirplaneDconfig() const;

private Q_SLOTS:
    void showWirelessEditPage(NetworkDeviceBase *dev, const QString &connUuid = QString(), const QString &apPath = QString());

    void showPppPage(const QString &searchPath);
    void showVPNPage(const QString &searchPath);
    void showDeviceDetailPage(NetworkDeviceBase *dev, const QString &searchPath = QString());
    void showChainsProxyPage();
    void showProxyPage();
    void showHotspotPage(const QString &searchPath);
    void showDetailPage();
    void showAirplanePage();
    void onWirelessAccessPointsOrAdapterChange();

private:
    NetworkModuleWidget *m_indexWidget;
    ConnectionEditPage *m_connEditPage;
    DBusAirplaneMode *m_airplaneMode;
    NetworkInter *m_networkInter;
    BluetoothInter *m_bluetoothInter;
    DTK_CORE_NAMESPACE::DConfig *m_dconfig;
};

#endif // NETWORKINTERFACE_H
