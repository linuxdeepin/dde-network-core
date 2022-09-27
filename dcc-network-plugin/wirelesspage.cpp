// SPDX-FileCopyrightText: 2018 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wirelesspage.h"
#include "connectionwirelesseditpage.h"
#include "window/utils.h"
#include "window/gsettingwatcher.h"

#include <DStyle>
#include <DStyleOption>
#include <DListView>
#include <DSpinner>

#include <QMap>
#include <QTimer>
#include <QDebug>
#include <QVBoxLayout>
#include <QPointer>
#include <QPushButton>
#include <DDBusSender>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QScroller>
#include <QGSettings>

#include <widgets/settingsgroup.h>
#include <widgets/switchwidget.h>
#include <widgets/translucentframe.h>
#include <widgets/tipsitem.h>
#include <widgets/titlelabel.h>

#include <wirelessdevice.h>
#include <networkdevicebase.h>
#include <networkcontroller.h>
#include <wirelessdevice.h>
#include <hotspotcontroller.h>

#include <networkmanagerqt/manager.h>
#include <networkmanagerqt/wirelesssetting.h>
#include <networkmanagerqt/setting.h>

DWIDGET_USE_NAMESPACE
using namespace dcc::widgets;
using namespace dde::network;
using namespace NetworkManager;

APItem::APItem(const QString &text, QStyle *style, DListView *parent)
        : DStandardItem(text)
        , m_parentView(nullptr)
        , m_dStyleHelper(style)
        , m_preLoading(false)
        , m_uuid("")
        , m_isWlan6(false)
        , m_isLastRow(false)
{
    setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    setCheckable(false);

    m_secureAction = new DViewItemAction(Qt::AlignCenter, QSize(), QSize(), false);
    setActionList(Qt::Edge::LeftEdge, { m_secureAction });

    m_parentView = parent;
    if (parent != nullptr) {
        m_loadingIndicator = new DSpinner();
        m_loadingIndicator->setFixedSize(20, 20);
        m_loadingIndicator->hide();
        m_loadingIndicator->stop();
        m_loadingIndicator->setParent(parent->viewport());
    }

    m_loadingAction = new DViewItemAction(Qt::AlignLeft | Qt::AlignVCenter, QSize(), QSize(), false);
    if (!m_loadingIndicator.isNull()) {
        m_loadingAction->setWidget(m_loadingIndicator);
    }
    m_loadingAction->setVisible(false);

    m_arrowAction = new DViewItemAction(Qt::AlignRight | Qt::AlignVCenter, QSize(), QSize(), true);
    QStyleOption opt;
    m_arrowAction->setIcon(m_dStyleHelper.standardIcon(DStyle::SP_ArrowEnter, &opt, nullptr));
    m_arrowAction->setClickAreaMargins(ArrowEnterClickMargin);
    setActionList(Qt::Edge::RightEdge, {m_loadingAction, m_arrowAction});
}

APItem::~APItem()
{
    if (!m_loadingIndicator.isNull()) {
        m_loadingIndicator->stop();
        m_loadingIndicator->hide();
        m_loadingIndicator->deleteLater();
    }
}

void APItem::setSecure(bool isSecure)
{
    if (m_secureAction)
        m_secureAction->setIcon(m_dStyleHelper.standardIcon(isSecure ? DStyle::SP_LockElement : DStyle::SP_CustomBase, nullptr, nullptr));

    setData(isSecure, SecureRole);
}

bool APItem::secure() const
{
    return data(SecureRole).toBool();
}

void APItem::setSignalStrength(int strength)
{
    if (strength < 0) {
        setIcon(QPixmap());
        return;
    }

    if (strength <= 5) {
        if (m_isWlan6)
            setIcon(QIcon::fromTheme(QString("dcc_wireless6-0")));
        else
            setIcon(QIcon::fromTheme(QString("dcc_wireless-0")));
    } else if (strength > 5 && strength <= 30) {
        if (m_isWlan6)
            setIcon(QIcon::fromTheme(QString("dcc_wireless6-2")));
        else
            setIcon(QIcon::fromTheme(QString("dcc_wireless-2")));
    } else if (strength > 30 && strength <= 55) {
        if (m_isWlan6)
            setIcon(QIcon::fromTheme(QString("dcc_wireless6-4")));
        else
            setIcon(QIcon::fromTheme(QString("dcc_wireless-4")));
    } else if (strength > 55 && strength <= 65) {
        if (m_isWlan6)
            setIcon(QIcon::fromTheme(QString("dcc_wireless6-6")));
        else
            setIcon(QIcon::fromTheme(QString("dcc_wireless-6")));
    } else if (strength > 65) {
        if (m_isWlan6)
            setIcon(QIcon::fromTheme(QString("dcc_wireless6-8")));
        else
            setIcon(QIcon::fromTheme(QString("dcc_wireless-8")));
    }
    APSortInfo si = data(SortRole).value<APSortInfo>();
    si.signalstrength = strength;
    si.ssid = text();
    si.connected = (checkState() == Qt::CheckState::Checked) || m_preLoading; // 连接或连接中
    setData(QVariant::fromValue(si), SortRole);
}

int APItem::signalStrength() const
{
    return data(SortRole).value<APSortInfo>().signalstrength;
}

void APItem::setIsWlan6(const bool isWlan6)
{
    m_isWlan6 = isWlan6;
}

void APItem::setConnected(bool connected)
{
    setCheckState(connected ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

bool APItem::isConnected()
{
    return checkState();
}

void APItem::setSortInfo(const APSortInfo &si)
{
    setData(QVariant::fromValue(si), SortRole);
}

APSortInfo APItem::sortInfo()
{
    return data(SortRole).value<APSortInfo>();
}

void APItem::setPath(const QString &path)
{
    setData(path, PathRole);
}

QString APItem::path() const
{
    return data(PathRole).toString();
}

void APItem::setUuid(const QString &uuid)
{
    m_uuid = uuid;
}
QString APItem::uuid() const
{
    return m_uuid;
}

QAction *APItem::action() const
{
    return m_arrowAction;
}

bool APItem::operator<(const QStandardItem &other) const
{
    APSortInfo thisApInfo = data(SortRole).value<APSortInfo>();
    APSortInfo otherApInfo = other.data(SortRole).value<APSortInfo>();
    bool bRet = thisApInfo < otherApInfo;
    return bRet;
}

bool APItem::setLoading(bool isLoading)
{
    bool isReconnect = false;
    if (m_loadingIndicator.isNull())
        return isReconnect;

    if (m_preLoading == isLoading)
        return isReconnect;

    m_preLoading = isLoading;
    if (isLoading) {
        if (m_parentView) {
            QModelIndex index;
            const QStandardItemModel *deviceModel = dynamic_cast<const QStandardItemModel *>(m_parentView->model());
            if (!deviceModel)
                return isReconnect;

            for (int i = 0; i < m_parentView->count(); ++i) {
                DStandardItem *item = dynamic_cast<DStandardItem *>(deviceModel->item(i));
                if (!item)
                    return isReconnect;

                if (this == item) {
                    index = m_parentView->model()->index(i, 0);
                    break;
                }
            }

            QRect itemrect = m_parentView->visualRect(index);
            QPoint point(itemrect.x() + itemrect.width(), itemrect.y());
            m_loadingIndicator->move(point);
            m_loadingIndicator->start();
            m_loadingIndicator->show();
        }

        m_loadingAction->setVisible(true);
    } else {
        if (!m_loadingIndicator.isNull()) {
            m_loadingIndicator->stop();
            m_loadingIndicator->hide();
        }

        m_loadingAction->setVisible(false);

        isReconnect = true;
    }

    if (m_parentView)
        m_parentView->update();

    return isReconnect;
}

bool APItem::isLastRow() const
{
    return m_isLastRow;
}

void APItem::setIsLastRow(const bool lastRow)
{
    m_isLastRow = lastRow;
}

WirelessPage::WirelessPage(WirelessDevice *dev, QWidget *parent)
        : ContentWidget(parent)
        , m_device(dev)
        , m_pNetworkController(NetworkController::instance())
        , m_closeHotspotBtn(new QPushButton)
        , m_lvAP(new DListView(this))
        , m_clickedItem(nullptr)
        , m_modelAP(new QStandardItemModel(m_lvAP))
        , m_sortDelayTimer(new QTimer(this))
        , m_autoConnectHideSsid("")
        , m_wirelessScanTimer(new QTimer(this))
        , m_isAirplaneMode(false)
{
    setAccessibleName("WirelessPage");
    qRegisterMetaType<APSortInfo>();
    m_preWifiStatus = Wifi_Unknown;
    m_lvAP->setAccessibleName("List_wirelesslist");
    m_lvAP->setModel(m_modelAP);
    m_lvAP->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_lvAP->setBackgroundType(DStyledItemDelegate::BackgroundType::ClipCornerBackground);
    m_lvAP->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    m_lvAP->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_lvAP->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lvAP->setSelectionMode(QAbstractItemView::NoSelection);

    QScroller *scroller = QScroller::scroller(m_lvAP->viewport());
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
    scroller->setScrollerProperties(sp);

    m_modelAP->setSortRole(APItem::SortRole);
    m_sortDelayTimer->setInterval(100);
    m_sortDelayTimer->setSingleShot(true);

    //~ contents_path /network/Wireless Network
    QLabel *lblTitle = new QLabel(tr("Wireless Network Adapter"));              // 无线网卡
    DFontSizeManager::instance()->bind(lblTitle, DFontSizeManager::T5, QFont::DemiBold);
    m_switch = new SwitchWidget(nullptr, lblTitle);
    m_switch->setChecked(dev->isEnabled());
    //因为swtichbutton内部距离右间距为4,所以这里设置6就可以保证间距为10
    m_switch->getMainLayout()->setContentsMargins(10, 0, 6, 0);

    QGSettings *gsettings = new QGSettings("com.deepin.dde.control-center", QByteArray(), this);
    GSettingWatcher::instance()->bind("wireless", m_switch);
    m_lvAP->setVisible(dev->isEnabled() && gsettings->get("wireless").toString() != "Hidden");
    connect(gsettings, &QGSettings::changed, this, [ = ] (const QString &key) {
        if ("wireless" == key) {
            m_lvAP->setVisible(dev->isEnabled() && gsettings->get("wireless").toString() != "Hidden");
            if (gsettings->get("wireless").toString() == "Enabled")
                m_lvAP->setEnabled(true);
            else if(gsettings->get("wireless").toString() == "Disabled")
                m_lvAP->setEnabled(false);
        }
    });

    connect(m_switch, &SwitchWidget::checkedChanged, this, &WirelessPage::onNetworkAdapterChanged);
    connect(m_device, & WirelessDevice::enableChanged, this, [ this ] (const bool enabled) {
        m_switch->blockSignals(true);
        m_switch->setChecked(m_device->isEnabled() && !m_isAirplaneMode);
        m_switch->blockSignals(false);
        if (m_lvAP) {
            onUpdateAPItem();
            m_lvAP->setVisible(enabled && !m_isAirplaneMode && QGSettings("com.deepin.dde.control-center", QByteArray(), this).get("wireless").toString() != "Hidden");
            updateLayout(!m_lvAP->isHidden());
        }
        if (!enabled)
            onHotspotEnableChanged(false);
    });

    m_closeHotspotBtn->setText(tr("Close Hotspot"));

    TipsItem *tips = new TipsItem;
    tips->setText(tr("Disable hotspot first if you want to connect to a wireless network"));

    m_tipsGroup = new SettingsGroup;
    m_tipsGroup->appendItem(tips);

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addWidget(m_switch, 0, Qt::AlignTop);
    m_mainLayout->addWidget(m_lvAP);
    m_mainLayout->addWidget(m_tipsGroup);
    m_mainLayout->addWidget(m_closeHotspotBtn);
    m_layoutCount = m_mainLayout->layout()->count();
    updateLayout(!m_lvAP->isHidden());
    m_mainLayout->setSpacing(10);//三级菜单控件间的间隙
    m_mainLayout->setMargin(0);
    m_mainLayout->setContentsMargins(QMargins(10, 0, 10, 0));

    QWidget *mainWidget = new TranslucentFrame;
    mainWidget->setLayout(m_mainLayout);

    setContent(mainWidget);

    setContentsMargins(0, 10, 0, 10);

    connect(m_lvAP, &QListView::clicked, this, [ this ](const QModelIndex & idx) {
        if (idx.data(APItem::PathRole).toString().length() == 0) {
            this->showConnectHidePage();
            return;
        }

        const QStandardItemModel *deviceModel = qobject_cast<const QStandardItemModel *>(idx.model());
        if (!deviceModel)
            return;

        m_autoConnectHideSsid = "";
        m_clickedItem = dynamic_cast<APItem *>(deviceModel->item(idx.row()));
        if (!m_clickedItem)
            return;

        if (m_clickedItem->isConnected()) {
            // 如果已连接并且正在编辑连接信息，切换编辑内容为此连接
            const QString uuid = connectionUuid(m_clickedItem->data(Qt::ItemDataRole::DisplayRole).toString());
            if (!m_apEditPage.isNull() && m_apEditPage->connectionUuid() != uuid) {
                this->onApWidgetEditRequested(m_clickedItem->data(APItem::PathRole).toString(), m_clickedItem->data(Qt::ItemDataRole::DisplayRole).toString());
            }
            return;
        }

        onApWidgetConnectRequested(idx.data(APItem::PathRole).toString(), idx.data(Qt::ItemDataRole::DisplayRole).toString());
    });

    connect(m_sortDelayTimer, &QTimer::timeout, this, &WirelessPage::sortAPList);

    connect(m_device, &WirelessDevice::networkAdded, this, &WirelessPage::onUpdateAPItem);
    connect(m_device, &WirelessDevice::networkRemoved, this, &WirelessPage::onUpdateAPItem);
    connect(m_device, &WirelessDevice::connectionSuccess, this, &WirelessPage::updateApStatus);
    connect(m_device, &WirelessDevice::accessPointInfoChanged, this, &WirelessPage::onUpdateAccessPointInfo);
    connect(m_device, &WirelessDevice::hotspotEnableChanged, this, &WirelessPage::onHotspotEnableChanged);
    connect(m_device, &WirelessDevice::removed, this, &WirelessPage::onDeviceRemoved);
    connect(m_device, &WirelessDevice::connectionFailed, this, &WirelessPage::onActivateApFailed);
    connect(m_device, &WirelessDevice::connectionChanged, this, &WirelessPage::updateApStatus);
    connect(m_device, &WirelessDevice::deviceStatusChanged, this, &WirelessPage::updateApStatus);
    connect(m_device, &WirelessDevice::activeConnectionChanged, this, &WirelessPage::updateApStatus);

    // init data
    const QList<AccessPoints *> lstUAccessPoints = m_device->accessPointItems();
    if (!lstUAccessPoints.isEmpty() && m_device->isEnabled())
        onUpdateAPItem();

    QGSettings *gsetting = new QGSettings("com.deepin.dde.control-center", QByteArray(), this);
    connect(gsetting, &QGSettings::changed, this, [ & ](const QString &key) {
        if (key == "wireless-scan-interval")
            m_wirelessScanTimer->setInterval(gsetting->get("wireless-scan-interval").toInt() * 1000);
    });

    connect(m_device, &WirelessDevice::destroyed, this, [ this ] {
        this->m_device = nullptr;
    });

    connect(m_wirelessScanTimer, &QTimer::timeout, this, [ this ] {
        if (m_device)
            m_device->scanNetwork();
    });

    m_wirelessScanTimer->start(gsetting->get("wireless-scan-interval").toInt() * 1000);

    m_lvAP->setVisible(m_switch->checked() && QGSettings("com.deepin.dde.control-center", QByteArray(), this).get("wireless").toString() != "Hidden");
    connect(m_device, &WirelessDevice::deviceStatusChanged, this, &WirelessPage::onDeviceStatusChanged);
    updateLayout(!m_lvAP->isHidden());
    m_switch->setChecked(m_device->isEnabled());
    onDeviceStatusChanged(m_device->deviceStatus());

    HotspotController *hotspotController = NetworkController::instance()->hotspotController();
    onHotspotEnableChanged(m_device->isEnabled() && hotspotController->enabled(m_device));
    connect(m_closeHotspotBtn, &QPushButton::clicked, this, [ = ] {
        // 此处抛出这个信号是为了让外面记录当前关闭热点的设备，因为在关闭热点过程中，当前设备会移除一次，然后又会添加一次，相当于触发了两次信号，
        // 此时外面就会默认选中第一个设备而无法选中当前设备，因此在此处抛出信号是为了让外面能记住当前选择的设备
        hotspotController->setEnabled(m_device, false);
    });
}

WirelessPage::~WirelessPage()
{
    QScroller *scroller = QScroller::scroller(m_lvAP->viewport());
    if (scroller)
        scroller->stop();

    m_wirelessScanTimer->stop();
}

void WirelessPage::updateLayout(bool enabled)
{
    int layCount = m_mainLayout->layout()->count();
    if (enabled) {
        if (layCount > m_layoutCount) {
            QLayoutItem *layItem = m_mainLayout->takeAt(m_layoutCount);
            if (layItem)
                delete layItem;
        }
    } else if (layCount <= m_layoutCount){
        m_mainLayout->addStretch();
    }

    m_mainLayout->invalidate();
}

bool WirelessPage::isHiddenWlan(const QString &ssid) const
{
    for (AccessPoints *ap : m_device->accessPointItems()) {
        if (ap->ssid() == ssid)
            return ap->hidden();
    }

    return false;
}

void WirelessPage::onDeviceStatusChanged(const DeviceStatus &stat)
{
    //当wifi状态切换的时候，刷新一下列表，防止出现wifi已经连接，三级页面没有刷新出来的情况，和wifi已经断开，但是页面上还是显示该wifi
    m_device->scanNetwork();

    const bool unavailable = stat <= DeviceStatus::Unavailable;
    if (m_preWifiStatus == Wifi_Unknown)
        m_preWifiStatus = unavailable ? Wifi_Unavailable : Wifi_Available;

    WifiStatus curWifiStatus = unavailable ? Wifi_Unavailable : Wifi_Available;
    if (curWifiStatus != m_preWifiStatus && stat > DeviceStatus::Disconnected) {
        m_switch->setChecked(!unavailable && !m_isAirplaneMode);
        onNetworkAdapterChanged(!unavailable && !m_isAirplaneMode);
        m_preWifiStatus = curWifiStatus;
    }

    if (stat == DeviceStatus::Failed) {
        for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
            if (m_clickedItem == it.value()) {
                it.value()->setLoading(false);
                m_clickedItem = nullptr;
            }
        }
    } else if (DeviceStatus::Prepare <= stat && stat < DeviceStatus::Activated) {
        if (m_device->activeAccessPoints() != nullptr && m_device->isEnabled()) {
            for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
                if (m_device->activeAccessPoints()->ssid() == it.key()) {
                    it.value()->setLoading(true);
                    m_clickedItem = it.value();
                }
            }
        }
    }

    // 连接状态变化后更新编辑界面按钮状态
    if (!m_apEditPage.isNull()) {
        m_apEditPage->initHeaderButtons();
    }
}

void WirelessPage::jumpByUuid(const QString &uuid)
{
    if (uuid.isEmpty()) return;

    QTimer::singleShot(50, this, [ = ] {
        if (uuid == QString("Connect to hidden network"))
            showConnectHidePage();
        else if (m_apItems.contains(connectionSsid(uuid)))
            onApWidgetEditRequested("", uuid);
    });
}

void WirelessPage::onNetworkAdapterChanged(bool checked)
{
    m_device->setEnabled(checked);

    if (checked) {
        m_device->scanNetwork();
        onUpdateAPItem();
    }

    m_clickedItem = nullptr;
    m_lvAP->setVisible(checked && QGSettings("com.deepin.dde.control-center", QByteArray(), this).get("wireless").toString() != "Hidden");
    updateLayout(!m_lvAP->isHidden());
}

void WirelessPage::onUpdateAccessPointInfo(const QList<AccessPoints *> &changeAps)
{
    QMap<QString, int> accessPoints;
    for (AccessPoints *ap : changeAps)
        accessPoints[ap->ssid()] = ap->strength();

    for (int i = 0; i < m_modelAP->rowCount(); i++) {
        APItem *apItem = static_cast<APItem *>(m_modelAP->item(i));
        if (!accessPoints.contains(apItem->text()))
            continue;

        apItem->setSignalStrength(accessPoints[apItem->text()]);
    }
}

void WirelessPage::onAirplaneModeChanged(bool airplaneModeEnabled)
{
    if (m_switch && m_switch->switchButton()) {
        m_switch->switchButton()->setDisabled(airplaneModeEnabled);
    }
    setIsAirplaneMode(airplaneModeEnabled);
}

void WirelessPage::onUpdateAPItem()
{
    QList<AccessPoints *> aps = m_device->accessPointItems();
    QList<QString> removeSsid = m_apItems.keys();
    for (AccessPoints *ap : aps) {
        const QString &ssid = ap->ssid();
        APItem *apItem = nullptr;
        if (!m_apItems.contains(ssid)) {
            apItem = new APItem(ssid, style(), m_lvAP);
            m_apItems[ssid] = apItem;
            m_modelAP->appendRow(apItem);
            if (ssid == m_autoConnectHideSsid) {
                if (m_clickedItem)
                    m_clickedItem->setLoading(false);

                m_clickedItem = apItem;
            }
            connect(apItem->action(), &QAction::triggered, this, [ this, apItem ] {
                this->onApWidgetEditRequested(apItem->data(APItem::PathRole).toString(), apItem->data(Qt::ItemDataRole::DisplayRole).toString());
            });
        } else {
            apItem = m_apItems[ssid];
            removeSsid.removeOne(ssid);
        }
        apItem->setIsWlan6(ap->type() == AccessPoints::WlanType::wlan6);
        apItem->setSecure(ap->secured());
        apItem->setPath(ap->path());
        apItem->setConnected(ap->status() == ConnectionStatus::Activated);
        apItem->setLoading(ap->status() == ConnectionStatus::Activating);
        apItem->setSignalStrength(ap->strength());

        m_sortDelayTimer->start();
    }

    for (const QString &ssid : removeSsid) {
        // 如果移除隐藏网络
        if (ssid == m_autoConnectHideSsid)
            m_autoConnectHideSsid = "";

        if (!m_apItems.contains(ssid))
            continue;

        if (m_clickedItem == m_apItems[ssid])
            m_clickedItem = nullptr;

        m_modelAP->removeRow(m_modelAP->indexFromItem(m_apItems[ssid]).row());
        m_apItems.erase(m_apItems.find(ssid));
    }

    // 连接状态变化后更新编辑界面按钮状态
    if (!m_apEditPage.isNull()) {
        m_apEditPage->initHeaderButtons();
    }

    appendConnectHidden();
}

void WirelessPage::appendConnectHidden()
{
    bool finded = false;
    for (int i = 0; i < m_modelAP->rowCount(); i++) {
        APItem *item = static_cast<APItem *>(m_modelAP->item(i));
        if (item && item->isLastRow()) {
            finded = true;
            break;
        }
    }
    if (finded)
        return;

    //~ contents_path /network/Connect to hidden network
    //~ child_page WirelessPage
    APItem *nonbc = new APItem(tr("Connect to hidden network"), style());
    nonbc->setSignalStrength(-1);
    nonbc->setPath("");
    nonbc->setSortInfo({ -1, "", false });
    nonbc->setIsLastRow(true);
    connect(nonbc->action(), &QAction::triggered, this, &WirelessPage::showConnectHidePage);
    m_modelAP->appendRow(nonbc);
}

void WirelessPage::setIsAirplaneMode(bool isAirplaneMode)
{
    if(m_isAirplaneMode != isAirplaneMode){
        m_switch->setChecked(m_device->isEnabled() && !isAirplaneMode);
    }
    m_isAirplaneMode = isAirplaneMode;
}

void WirelessPage::onHotspotEnableChanged(const bool enabled)
{
    m_closeHotspotBtn->setVisible(enabled);
    m_tipsGroup->setVisible(enabled);
    m_lvAP->setVisible(!enabled && m_device->isEnabled() && QGSettings("com.deepin.dde.control-center", QByteArray(), this).get("wireless").toString() != "Hidden");
    updateLayout(!m_lvAP->isHidden());
}

void WirelessPage::onCloseHotspotClicked()
{
    m_device->disconnectNetwork();
}

void WirelessPage::onDeviceRemoved()
{
    // back if ap edit page exist
    if (!m_apEditPage.isNull())
        m_apEditPage->onDeviceRemoved();

    //Q_EMIT requestWirelessScan();
    m_device->scanNetwork();
    // destroy self page
    Q_EMIT back();
}

void WirelessPage::onActivateApFailed(const AccessPoints* pAccessPoints)
{
    onApWidgetEditRequested(pAccessPoints->path(), connectionSsid(pAccessPoints->ssid()));
    for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
        if ((it.value()->path() == pAccessPoints->path()) && (it.value()->uuid() == pAccessPoints->ssid())) {
            bool isReconnect = it.value()->setLoading(false);
            if (isReconnect) {
                connect(it.value()->action(), &QAction::triggered, this, [ this, it ] {
                    this->onApWidgetEditRequested(it.value()->data(APItem::PathRole).toString(), it.value()->data(Qt::ItemDataRole::DisplayRole).toString());
                });
            }
        }

        it.value()->setConnected(false);
    }

    // 连接状态变化后更新编辑界面按钮状态
    if (!m_apEditPage.isNull()) {
        m_apEditPage->initHeaderButtons();
    }
}

void WirelessPage::sortAPList()
{
    m_modelAP->sort(0);
}

void WirelessPage::onApWidgetEditRequested(const QString &apPath, const QString &ssid)
{
    const QString uuid = connectionUuid(ssid);

    m_apEditPage = new ConnectionWirelessEditPage(m_device->path(), uuid, apPath, isHiddenWlan(ssid));
    connect(m_apEditPage, &ConnectionWirelessEditPage::disconnect, this, [ this ] {
        m_device->disconnectNetwork();
    });

    if (!uuid.isEmpty()) {
        m_editingUuid = uuid;
        m_apEditPage->initSettingsWidget();
    } else {
        m_apEditPage->initSettingsWidgetFromAp();
    }
    m_apEditPage->setLeftButtonEnable(true);

    connect(m_apEditPage, &ConnectionEditPage::requestNextPage, this, &WirelessPage::requestNextPage);
    connect(m_apEditPage, &ConnectionEditPage::requestFrameAutoHide, this, &WirelessPage::requestFrameKeepAutoHide);
    connect(m_switch, &SwitchWidget::checkedChanged, m_apEditPage, [ = ] (bool checked) {
        if (!checked)
            m_apEditPage->back();
    });

    Q_EMIT requestNextPage(m_apEditPage);
}

void WirelessPage::onApWidgetConnectRequested(const QString &path, const QString &ssid)
{
    Q_UNUSED(path);

    const QString uuid = connectionUuid(ssid);
    // uuid could be empty
    for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
        it.value()->setConnected(false);
        if (m_clickedItem == it.value())
            m_clickedItem->setUuid(uuid);
    }

    if (uuid.isEmpty()) {
        for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
            bool isReconnect = it.value()->setLoading(false);
            if (isReconnect) {
                connect(it.value()->action(), &QAction::triggered, this, [ this, it ] {
                    this->onApWidgetEditRequested(it.value()->data(APItem::PathRole).toString(), it.value()->data(Qt::ItemDataRole::DisplayRole).toString());
                });
            }
        }
    } else {
        for (auto it = m_apItems.cbegin(); it != m_apItems.cend(); ++it) {
            bool isReconnect = it.value()->setLoading(it.value() == m_clickedItem);
            if (isReconnect) {
                connect(it.value()->action(), &QAction::triggered, this, [ this, it ] {
                    this->onApWidgetEditRequested(it.value()->data(APItem::PathRole).toString(), it.value()->data(Qt::ItemDataRole::DisplayRole).toString());
                });
            }
        }
    }

    // 如果正在编辑状态，则切换到当前连接
    if (m_clickedItem && !m_apEditPage.isNull()) {
        this->onApWidgetEditRequested(m_clickedItem->data(APItem::PathRole).toString(), m_clickedItem->data(Qt::ItemDataRole::DisplayRole).toString());
    }

    if (m_switch && m_switch->checked())
        m_device->connectNetwork(ssid);
}

void WirelessPage::showConnectHidePage()
{
    m_apEditPage = new ConnectionWirelessEditPage(m_device->path(), QString(), QString(), true);
    m_apEditPage->initSettingsWidget();
    m_apEditPage->setLeftButtonEnable(true);
    connect(m_apEditPage, &ConnectionEditPage::activateWirelessConnection, this, [ this ] (const QString &ssid, const QString &uuid) {
        Q_UNUSED(uuid);
        m_autoConnectHideSsid = ssid;
    });
    connect(m_apEditPage, &ConnectionEditPage::requestNextPage, this, &WirelessPage::requestNextPage);
    connect(m_apEditPage, &ConnectionEditPage::requestFrameAutoHide, this, &WirelessPage::requestFrameKeepAutoHide);
    connect(m_switch, &SwitchWidget::checkedChanged, m_apEditPage, [ = ] (bool checked) {
        if (!checked)
            m_apEditPage->back();
    });

    Q_EMIT requestNextPage(m_apEditPage);
}

void WirelessPage::updateApStatus()
{
    QList<AccessPoints *> accessPoints = m_device->accessPointItems();
    onUpdateAPItem();
    QMap<QString, ConnectionStatus> connectionStatus;
    bool isConnecting = false;
    for (AccessPoints *ap : accessPoints) {
        connectionStatus[ap->ssid()] = ap->status();
        if (ap->status() == ConnectionStatus::Activating)
            isConnecting = true;
    }

    for (int i = 0; i < m_modelAP->rowCount(); i++) {
        APItem *apItem = dynamic_cast<APItem *>(m_modelAP->item(i, 0));
        if (!apItem || !connectionStatus.contains(apItem->text()))
            continue;

        ConnectionStatus status = connectionStatus[apItem->text()];

        apItem->setLoading(status == ConnectionStatus::Activating);
        apItem->setCheckState((!isConnecting && status == ConnectionStatus::Activated) ? Qt::Checked : Qt::Unchecked);

        apItem->action()->disconnect();
        connect(apItem->action(), &QAction::triggered, this, [ this, apItem ] {
            this->onApWidgetEditRequested(apItem->data(APItem::PathRole).toString(), apItem->data(Qt::ItemDataRole::DisplayRole).toString());
        });
    }

    // 连接状态变化时更新编辑界面按钮状态
    if (!m_apEditPage.isNull()) {
        m_apEditPage->initHeaderButtons();
    }

    m_sortDelayTimer->start();
}

QString WirelessPage::connectionUuid(const QString &ssid)
{
    for (auto conn : NetworkManager::activeConnections()) {
        if (conn->type() != ConnectionSettings::ConnectionType::Wireless || conn->id() != ssid)
            continue;

        NetworkManager::ConnectionSettings::Ptr connSettings = conn->connection()->settings();
        NetworkManager::WirelessSetting::Ptr wSetting = connSettings->setting(NetworkManager::Setting::SettingType::Wireless).staticCast<NetworkManager::WirelessSetting>();
        if (wSetting.isNull())
            continue;

        QString settingMacAddress = wSetting->macAddress().toHex().toUpper();
        QString deviceMacAddress = m_device->realHwAdr().remove(":");
        if (!settingMacAddress.isEmpty() && settingMacAddress != deviceMacAddress)
            continue;

        return conn->uuid();
    }
    const QList<WirelessConnection *> lstConnections = m_device->items();
    for (auto item : lstConnections) {
        if (item->connection()->ssid() != ssid) continue;

        QString uuid = item->connection()->uuid();
        if (!uuid.isEmpty())
            return uuid;
    }

    return QString();
}

QString WirelessPage::connectionSsid(const QString &uuid)
{
    const QList<WirelessConnection *> wirelessConnections = m_device->items();
    for (WirelessConnection *item : wirelessConnections) {
        if (item->connection()->uuid() != uuid) continue;

        QString ssid = item->connection()->ssid();
        if (!ssid.isEmpty())
            return ssid;
    }

    return QString();
}
