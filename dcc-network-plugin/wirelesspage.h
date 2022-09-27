// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WIRELESSPAGE_H
#define WIRELESSPAGE_H

#include "widgets/contentwidget.h"

#include <DStandardItem>
#include <DStyleHelper>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTimer;
class QListView;
class QVBoxLayout;
class QStandardItemModel;
QT_END_NAMESPACE

DWIDGET_BEGIN_NAMESPACE
class DListView;
class DSpinner;
DWIDGET_END_NAMESPACE

class ConnectionWirelessEditPage;

namespace dde {
  namespace network {
    class AccessPoints;
    class NetworkDeviceBase;
    class NetworkController;
    class WirelessDevice;
    enum class DeviceStatus;
  }
}

namespace dcc {
  namespace widgets {
    class SettingsGroup;
    class SwitchWidget;
  }
}

using namespace dde::network;
using namespace dcc::widgets;

struct APSortInfo {
    int signalstrength;
    QString ssid;
    bool connected;
    bool operator <(const APSortInfo &other) {
        if (connected ^ other.connected)
            return connected;
        if(signalstrength == other.signalstrength)
            return ssid <= other.ssid;
        return signalstrength > other.signalstrength;
    }
};

class APItem : public DStandardItem
{
public:
    explicit APItem(const QString &text, QStyle *style, DListView *parent = nullptr);
    virtual ~APItem() Q_DECL_OVERRIDE;
    void setSecure(bool isSecure);
    bool secure() const;
    void setSignalStrength(int strength);
    int signalStrength() const;
    void setIsWlan6(const bool isWlan6);
    void setConnected(bool connected);
    bool isConnected();
    void setSortInfo(const APSortInfo &si);
    APSortInfo sortInfo();
    void setPath(const QString &path);
    QString path() const;
    void setUuid(const QString &uuid);
    QString uuid() const;
    QAction *action() const;
    bool operator<(const QStandardItem &other) const override;
    bool setLoading(bool isLoading);
    bool isLastRow() const;
    void setIsLastRow(const bool lastRow);

public:
    enum {
        SortRole = Dtk::UserRole + 1,
        PathRole,
        SecureRole
    };

private:
    DListView *m_parentView;
    DStyleHelper m_dStyleHelper;
    DViewItemAction *m_secureAction;
    bool m_preLoading;
    QString m_uuid;
    QPointer<DViewItemAction> m_loadingAction;
    QPointer<DViewItemAction> m_arrowAction;
    QPointer<DSpinner> m_loadingIndicator;
    bool m_isWlan6;
    bool m_isLastRow;
};

class WirelessPage : public dcc::ContentWidget
{
    Q_OBJECT

public:
    enum WifiStatus {
        Wifi_Unknown = 0,
        Wifi_Available,
        Wifi_Unavailable
    };

    explicit WirelessPage(WirelessDevice *dev, QWidget *parent = nullptr);
    virtual ~WirelessPage();

    void jumpByUuid(const QString &uuid);

Q_SIGNALS:
    void requestNextPage(ContentWidget *const w) const;
    void requestFrameKeepAutoHide(const bool autoHide) const;

public Q_SLOTS:
    void onAirplaneModeChanged(bool airplaneModeEnabled);

    void setIsAirplaneMode(bool isAirplaneMode);

private Q_SLOTS:
    void onHotspotEnableChanged(const bool enabled);
    void onCloseHotspotClicked();
    void onDeviceStatusChanged(const DeviceStatus &stat);
    void onUpdateAPItem();

    void sortAPList();
    void onApWidgetEditRequested(const QString &apPath, const QString &ssid);
    void onApWidgetConnectRequested(const QString &path, const QString &ssid);
    void showConnectHidePage();
    void onDeviceRemoved();
    void onActivateApFailed(const AccessPoints* pAccessPoints);
    void onNetworkAdapterChanged(bool checked);

    void onUpdateAccessPointInfo(const QList<AccessPoints *> &changeAps);

private:
    void updateApStatus();
    QString connectionUuid(const QString &ssid);
    QString connectionSsid(const QString &uuid);
    void updateLayout(bool enabled);
    bool isHiddenWlan(const QString &ssid) const;
    void appendConnectHidden();

private:
    WirelessDevice *m_device;
    NetworkController *m_pNetworkController; //网络硬件接口

    SwitchWidget *m_switch;
    SettingsGroup *m_tipsGroup;
    QPushButton *m_closeHotspotBtn;
    DListView *m_lvAP;
    APItem *m_clickedItem;
    QVBoxLayout *m_mainLayout;
    QStandardItemModel *m_modelAP;
    int m_layoutCount;
    WifiStatus m_preWifiStatus;
    QPointer<ConnectionWirelessEditPage> m_apEditPage;

    QString m_editingUuid;
    QString m_lastConnectSsid;
    QTimer *m_sortDelayTimer;
    QMap<QString, APItem *> m_apItems;
    QString m_autoConnectHideSsid;
    QTimer *m_wirelessScanTimer;

    bool m_isAirplaneMode;
};

Q_DECLARE_METATYPE(APSortInfo)

#endif // WIRELESSPAGE_H
